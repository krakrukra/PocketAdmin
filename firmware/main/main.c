#include "../cmsis/stm32f0xx.h"
#include "../usb/usb.h"
#include "../usb/hid_rodata.h"
#include "../fatfs/ff.h"
#include "../fatfs/diskio.h"

extern ControlInfo_TypeDef ControlInfo;
extern DeviceDescriptor_TypeDef DeviceDescriptor;

static char PayloadBuffer[1024];//holds duckyscript commands to execute
static FATFS FATFSinfo;//holds info about filesystem on the medium
static FIL openedFileInfo;//holds info related to an opened file
static unsigned int BytesRead;//holds the number of bytes that were successfully read with f_read()
static FRESULT FATFSresult;//holds return values of FATFS related funtions

static unsigned int runDuckyCommand();
static void repeatDuckyCommand(unsigned int count);
static char checkKeyword(char* referenceString);
static unsigned int checkDecValue();
static unsigned int checkHexValue();
static void checkFilename();
static void sendString(char* stringStart);//prints out ASCII-printable string
static void sendKeycode(unsigned char modifiers, unsigned char keycode);//send host a report with specified HID keycode and modifier byte

static void skipString();//move PayloadPointer to the next newline character (not to the next symbol after)
static void autodelay();

static inline void delay_us(unsigned int delay) __attribute__((always_inline));
static void delay_ms(unsigned int delay);

static unsigned char checkOSfingerprint(char* directoryName);

PayloadInfo_TypeDef PayloadInfo =
  {
    .DefaultDelay = 0,//if DEFAULT_DELAY is not used wait 0ms before every command
    .PayloadPointer = (char*) &PayloadBuffer,//start interpreting commands at the beginning of PayloadBuffer
    .BytesLeft = 0,//nothing was saved in PayloadBuffer yet
    .ActiveBuffer = 0,//first 512 bytes of PayloadBuffer are being executed
    .FirstRead = 0,//first MSD read command was not received yet
    .RepeatSize = 1,//by default REPEAT command is applied only to 1 last command
    .HoldFlag = 0,//no need to update hold modifiers
    .HoldModifiers = 0,//no modifiers are applied
    .LayoutFilename = {0x00},//use US keyboard layout as default
    .UseFingerprinter = 0//do not try to detect OS unless USE_FINGERPRINTER command is present
  };

//----------------------------------------------------------------------------------------------------------------------

int main()
{
  unsigned char i;//used in for() loop
  
  //SPI1 configuration
  SPI1->CR2 = (1<<12)|(1<<10)|(1<<9)|(1<<8);//8 bit frames, software CS output, RXNE set after 8 bits received, interrupts disabled
  SPI1->CR1 = (1<<9)|(1<<8)|(1<<6)|(1<<3)|(1<<2);//bidirectional SPI, master mode, MSb first, 12Mhz clock, mode 0; enable SPI1
  
  //initialize W25Q256FVFG flash memory chip, mount filesystem on the first partition, open and read payload.txt
  delay_ms(5);//wait until W25Q256FVFG startup delay is over and it is able to accept commands
  if ( f_mount(&FATFSinfo, "0:", 1) ) disk_initialize(0);//initialize flash memory for MSD use, in case no valid FAT is found ( f_mount() fails in this case )

  //try to get pre-configuration commands from config.txt 
  if( !f_open(&openedFileInfo, "0:/config.txt", FA_READ) )
    {
      f_read(&openedFileInfo, (char*) &PayloadBuffer, 512, &BytesRead );
      f_close(&openedFileInfo);
    }
  
  //run up to 5 pre-configuration commands from config.txt
  for(i=0; i<5; i++)
    {
           if( checkKeyword("HID_ONLY_MODE") )     { ControlInfo.EnumerationMode = 1; PayloadInfo.FirstRead = 1; skipString(); }//set FirstRead to 1 so DELAY does not freeze the interpreter
      else if( checkKeyword("USE_FINGERPRINTER") ) { PayloadInfo.UseFingerprinter = 1; skipString(); }
      else if( checkKeyword("USE_LAYOUT ") )       checkFilename();
      else if( checkKeyword("VID 0x") )            DeviceDescriptor.idVendor  = (unsigned short) checkHexValue();
      else if( checkKeyword("PID 0x") )            DeviceDescriptor.idProduct = (unsigned short) checkHexValue();
      else break;//stop if no pre-configuration command was found
	   
      //go to the next line
      if( PayloadInfo.PayloadPointer < ((char*) &PayloadBuffer + 1023) ) PayloadInfo.PayloadPointer++;
      else PayloadInfo.PayloadPointer = (char*) &PayloadBuffer;        
    }

  if(PayloadInfo.LayoutFilename[0])//if there is a USE_LAYOUT command in config.txt
    {
      f_chdir("0:/kblayout");//go to /kblayout/ directory
      
      //load new keymap from the specified file
      if( !f_open(&openedFileInfo, (char*) &(PayloadInfo.LayoutFilename), FA_READ) )
	{
	  f_read(&openedFileInfo, (unsigned char*) &Keymap, 107, &BytesRead );
	  f_close( &openedFileInfo );
	}      
    }
  
  usb_init();//initialize USB
  __enable_irq();//enable interrupts globally  
  
  if(PayloadInfo.UseFingerprinter)//if there is a USE_FINGERPRINTER command in config.txt    
    {
      NVIC_EnableIRQ(31);//enable usb interrupt, so fingerprint can be collected
      while(ControlInfo.OSfingerprintCounter < 10);//wait until OS fingerprint is collected
      NVIC_DisableIRQ(31);//disable usb interrupt, so that MSD access and f_open() + f_write() do not collide

      //save current OS fingerprint into a file
      f_mkdir("0:/fgscript");
      f_mkdir("0:/fingerdb");
      if( !f_open(&openedFileInfo,  "0:/fingerdb/current.fgp", FA_WRITE | FA_CREATE_ALWAYS) )
	{
	  f_write( &openedFileInfo, (unsigned char*) &ControlInfo.OSfingerprintData, 40, &BytesRead );
	  f_close( &openedFileInfo );
	}

      //compare current OS fingerprint with a database, choose appropriate payload file
           if( checkOSfingerprint("0:/fingerdb/windows") ) FATFSresult = f_open(&openedFileInfo, "0:/fgscript/windows.txt", FA_READ);
      else if( checkOSfingerprint("0:/fingerdb/linux") )   FATFSresult = f_open(&openedFileInfo, "0:/fgscript/linux.txt", FA_READ);
      else if( checkOSfingerprint("0:/fingerdb/mac") )     FATFSresult = f_open(&openedFileInfo, "0:/fgscript/mac.txt", FA_READ);
      else                                                 FATFSresult = f_open(&openedFileInfo, "0:/fgscript/other.txt", FA_READ);       	   
    }
  //if USE_FINGERPRINTER command is not present
  else FATFSresult = f_open(&openedFileInfo, "0:/payload.txt", FA_READ);  

  
  //if MSD-only button is not pressed and appropriate payload file exists, run ducky interpreter
  if( !FATFSresult && (GPIOA->IDR & (1<<2)) )
    {
      f_read(&openedFileInfo, (char*) &PayloadBuffer, 1024, &BytesRead );
      PayloadInfo.BytesLeft = BytesRead;
      PayloadInfo.PayloadPointer = (char*) &PayloadBuffer;//move PayloadPointer back to start
      NVIC_EnableIRQ(31);//enable usb interrupt
      
      //keep executing commands until end of file is reached
      while(PayloadInfo.BytesLeft)
	{
	  PayloadInfo.BytesLeft = PayloadInfo.BytesLeft - runDuckyCommand();//execute one line of ducky script, subtract number of executed bytes from BytesLeft
	  
	  //go to next line in the ducky script
	  if( PayloadInfo.PayloadPointer < ((char*) &PayloadBuffer + 1023) ) PayloadInfo.PayloadPointer++;
	  else PayloadInfo.PayloadPointer = (char*) &PayloadBuffer;
	  
	  
	  //if first 512 bytes of PayloadBuffer were processed and can now be replaced with new data
	  if( (PayloadInfo.ActiveBuffer == 0) && (PayloadInfo.PayloadPointer >= ((char*) &PayloadBuffer + 512)) )
	    {
	      NVIC_DisableIRQ(31);//disable usb interrupt, so f_read() and MSD access do not collide
	      PayloadInfo.ActiveBuffer = 1;//commands are now being executed from last 512 bytes of PayloadBuffer
	      f_read( &openedFileInfo, (char*) &PayloadBuffer, 512, &BytesRead );//first 512 bytes can now be overwritten
	      PayloadInfo.BytesLeft = PayloadInfo.BytesLeft + BytesRead;
	      NVIC_EnableIRQ(31);//enable usb interrupt again
	    }	  
	  //if last 512 bytes of PayloadBuffer were processed and can now be replaced with new data 
	  else if( (PayloadInfo.ActiveBuffer == 1) && (PayloadInfo.PayloadPointer < ((char*) &PayloadBuffer + 512)) )
	    {
	      NVIC_DisableIRQ(31);//disable usb interrupt, so f_read() and MSD access do not collide
	      PayloadInfo.ActiveBuffer = 0;//commands are now being executed from first 512 bytes of PayloadBuffer
	      f_read( &openedFileInfo, (char*) &PayloadBuffer + 512, 512, &BytesRead );//last 512 bytes can now be overwritten
	      PayloadInfo.BytesLeft = PayloadInfo.BytesLeft + BytesRead;
	      NVIC_EnableIRQ(31);//enable usb interrupt again
	    }
	}      
    }
  
  NVIC_EnableIRQ(31);//enable usb interrupt

  while(1)
    {
      sendKeycode(MOD_NONE, KB_Reserved);
      delay_ms(100);
    }
  
  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

//run next command from PayloadBuffer, move PayloadPointer to the end of line, return length of executed command in bytes (including newline termination)
static unsigned int runDuckyCommand()
{
  unsigned char modbyte = MOD_NONE;//modifier byte to send in a next report
  unsigned char keycode = KB_Reserved;//HID keycode to send in a next report
  unsigned short limit = 5;//maximum number of keywords allowed to be in one line of ducky script
  unsigned int commandStart = (unsigned int) PayloadInfo.PayloadPointer;//remember where start of command is
  
  delay_ms(PayloadInfo.DefaultDelay);//wait for a default time. if ducky script does not use DEFAULT_DELAY wait 0ms
  
  while( *(PayloadInfo.PayloadPointer) != 0x0A )//keep searching for keywords until the end of line is found
    {
      if(limit) limit--;//if limit of keywords is reached, release all buttons and freeze ducky interpreter
      else while(1) sendKeycode(MOD_NONE, KB_Reserved);
      
      //search for specific keywords, take appropriate actions if found
      if     ( checkKeyword("REM ") )           skipString();
      else if( checkKeyword("REPEAT_SIZE ") )   PayloadInfo.RepeatSize = checkDecValue();
      else if( checkKeyword("DEFAULT_DELAY ") ) PayloadInfo.DefaultDelay = checkDecValue();
      else if( checkKeyword("DEFAULTDELAY ") )  PayloadInfo.DefaultDelay = checkDecValue();
      else if( checkKeyword("DELAY ") )         autodelay( checkDecValue() );
      else if( checkKeyword("STRING ") )        sendString( PayloadInfo.PayloadPointer );
      else if( checkKeyword("REPEAT ") )        repeatDuckyCommand( checkDecValue() );
      else if( checkKeyword("HOLD ") )          PayloadInfo.HoldFlag = 1;
      else if( checkKeyword("RELEASE") )        PayloadInfo.HoldModifiers = 0x00;
      
      else if( checkKeyword("GUI ") )        modbyte = modbyte | MOD_LGUI;
      else if( checkKeyword("WINDOWS ") )    modbyte = modbyte | MOD_LGUI;
      else if( checkKeyword("CTRL ") )       modbyte = modbyte | MOD_LCTRL;
      else if( checkKeyword("CONTROL ") )    modbyte = modbyte | MOD_LCTRL;
      else if( checkKeyword("SHIFT ") )      modbyte = modbyte | MOD_LSHIFT;
      else if( checkKeyword("ALT ") )        modbyte = modbyte | MOD_LALT;
      else if( checkKeyword("RGUI ") )       modbyte = modbyte | MOD_RGUI;
      else if( checkKeyword("RCTRL ") )      modbyte = modbyte | MOD_RCTRL;
      else if( checkKeyword("RSHIFT ") )     modbyte = modbyte | MOD_RSHIFT;
      else if( checkKeyword("RALT ") )       modbyte = modbyte | MOD_RALT;
      
      else if( checkKeyword("KEYCODE 0x") )  keycode = checkHexValue();
      else if( checkKeyword("KEYCODE ") )    keycode = checkDecValue();
      else if( checkKeyword("MENU") )        keycode = KB_COMPOSE;
      else if( checkKeyword("APP") )         keycode = KB_COMPOSE;
      else if( checkKeyword("ENTER") )       keycode = KB_RETURN;
      else if( checkKeyword("RETURN") )      keycode = KB_RETURN;
      else if( checkKeyword("DOWN") )        keycode = KB_DOWNARROW;
      else if( checkKeyword("LEFT") )        keycode = KB_LEFTARROW;
      else if( checkKeyword("RIGHT") )       keycode = KB_RIGHTARROW;
      else if( checkKeyword("UP") )          keycode = KB_UPARROW;
      else if( checkKeyword("DOWNARROW") )   keycode = KB_DOWNARROW;
      else if( checkKeyword("LEFTARROW") )   keycode = KB_LEFTARROW;
      else if( checkKeyword("RIGHTARROW") )  keycode = KB_RIGHTARROW;
      else if( checkKeyword("UPARROW") )     keycode = KB_UPARROW;
      else if( checkKeyword("PAUSE") )       keycode = KB_PAUSE;
      else if( checkKeyword("BREAK") )       keycode = KB_PAUSE;
      else if( checkKeyword("CAPSLOCK") )    keycode = KB_CAPSLOCK;
      else if( checkKeyword("DELETE") )      keycode = KB_DELETE;
      else if( checkKeyword("END")  )        keycode = KB_END;
      else if( checkKeyword("ESC") )         keycode = KB_ESCAPE;
      else if( checkKeyword("ESCAPE") )      keycode = KB_ESCAPE;
      else if( checkKeyword("HOME") )        keycode = KB_HOME;
      else if( checkKeyword("INSERT") )      keycode = KB_INSERT;
      else if( checkKeyword("NUMLOCK") )     keycode = KP_NUMLOCK;
      else if( checkKeyword("PAGEUP") )      keycode = KB_PAGEUP;
      else if( checkKeyword("PAGEDOWN") )    keycode = KB_PAGEDOWN;
      else if( checkKeyword("PRINTSCREEN") ) keycode = KB_PRINTSCREEN;
      else if( checkKeyword("SCROLLLOCK") )  keycode = KB_SCROLLLOCK;
      else if( checkKeyword("SPACE") )       keycode = KB_SPACEBAR;
      else if( checkKeyword("SPACEBAR") )    keycode = KB_SPACEBAR;
      else if( checkKeyword("TAB") )         keycode = KB_TAB;
      else if( checkKeyword("F1") )          keycode = KB_F1;
      else if( checkKeyword("F2") )          keycode = KB_F2;
      else if( checkKeyword("F3") )          keycode = KB_F3;
      else if( checkKeyword("F4") )          keycode = KB_F4;
      else if( checkKeyword("F5") )          keycode = KB_F5;
      else if( checkKeyword("F6") )          keycode = KB_F6;
      else if( checkKeyword("F7") )          keycode = KB_F7;
      else if( checkKeyword("F8") )          keycode = KB_F8;
      else if( checkKeyword("F9") )          keycode = KB_F9;
      else if( checkKeyword("F10") )         keycode = KB_F10;
      else if( checkKeyword("F11") )         keycode = KB_F11;
      else if( checkKeyword("F12") )         keycode = KB_F12;
      //if keyword is not recognized, but PayloadPointer is at some ASCII printable character
      else if( ( *(PayloadInfo.PayloadPointer) > 31 ) && ( *(PayloadInfo.PayloadPointer) < 127 ) )
	{
	  keycode = Keymap[ *(PayloadInfo.PayloadPointer) - 32 ];//map ascii symbol to HID keycode + LSHIFT indicator bit
	  if( keycode & (1<<7) ) modbyte = modbyte | MOD_LSHIFT;//if most significant bit is set use LSHIFT modifier
	  if( Keymap[ (*PayloadInfo.PayloadPointer - 32) / 8 + 95 ] & (1 << (*PayloadInfo.PayloadPointer % 8)) ) modbyte = modbyte | MOD_RALT;//if corresponding AltGr bit is set, use RALT modifier
	}
      //if keyword is not recognized and PayloadPointer is at some nonprintable character
      else skipString();
      
      //even if line is not over, but some keystroke was already specified ignore the rest of the line
      if(keycode != KB_Reserved) skipString();
    }
  
  //if HOLD command was executed, save the specified modifiers for next commands
  if(PayloadInfo.HoldFlag)
    {
      PayloadInfo.HoldFlag = 0;
      PayloadInfo.HoldModifiers = modbyte;
    }
  
  sendKeycode(MOD_NONE | PayloadInfo.HoldModifiers, KB_Reserved);//release all buttons, hold some modifiers if necessary
  sendKeycode(modbyte  | PayloadInfo.HoldModifiers, keycode);//send keystroke corresponding to current ducky command
  sendKeycode(MOD_NONE | PayloadInfo.HoldModifiers, KB_Reserved);//release all buttons, hold some modifiers if necessary

  if( (unsigned int) PayloadInfo.PayloadPointer >= commandStart) return (unsigned int) PayloadInfo.PayloadPointer - commandStart + 1;
  else                                                           return (unsigned int) PayloadInfo.PayloadPointer - commandStart + 1 + 1024;
}

static void repeatDuckyCommand(unsigned int count)
{
  unsigned short limit = 15;//maximum number of symbols by which PayloadPointer is allowed to move back
  unsigned int i;//used in a for() loop
  
  //move PayloadPointer to the newline right before REPEAT command
  do
    {
      if(limit) limit--;//if limit of symbols is reached, release all buttons and freeze ducky interpreter
      else while(1) sendKeycode(MOD_NONE, KB_Reserved);
      
      //go to previous character in a string. if the start of PayloadBuffer is reached, move pointer back to end of buffer
      if( PayloadInfo.PayloadPointer > (char*) &PayloadBuffer ) PayloadInfo.PayloadPointer--;
      else PayloadInfo.PayloadPointer = (char*) &PayloadBuffer + 1023;
    }
  while( *(PayloadInfo.PayloadPointer) != 0x0A );

  
  //execute a command block right before REPEAT count times (size of command block is set by REPEAT_SIZE command)
  while(count)
    {
      limit = 410;//maximum number of symbols in a previous command block
      
      //move to the newline right before previous command block
      for(i=0; i<PayloadInfo.RepeatSize; i++)
	{
	  do
	    {
	      if(limit) limit--;//if limit of symbols is reached, release all buttons and freeze ducky interpreter
	      else while(1) sendKeycode(MOD_NONE, KB_Reserved);
	      
	      //go to previous character in a string. if the start of PayloadBuffer is reached, move pointer back to end of buffer
	      if( PayloadInfo.PayloadPointer > (char*) &PayloadBuffer ) PayloadInfo.PayloadPointer--;
	      else PayloadInfo.PayloadPointer = (char*) &PayloadBuffer + 1023;
	    }
	  while( *(PayloadInfo.PayloadPointer) != 0x0A );//keep moving until newline is encountered
	}
      
      //move to the start of first command in a block
      if( PayloadInfo.PayloadPointer < ((char*) &PayloadBuffer + 1023) ) PayloadInfo.PayloadPointer++;
      else PayloadInfo.PayloadPointer = (char*) &PayloadBuffer;

      //execute ducky command block
      for(i=0; i<PayloadInfo.RepeatSize; i++)
	{
	  runDuckyCommand();

	  //go to the next command, but only if end of block is not reached yet
	  if( PayloadInfo.PayloadPointer < ((char*) &PayloadBuffer + 1023) ) PayloadInfo.PayloadPointer++;
	  else PayloadInfo.PayloadPointer = (char*) &PayloadBuffer;
	}

      //go to last newline inside a command block
      if( PayloadInfo.PayloadPointer > (char*) &PayloadBuffer ) PayloadInfo.PayloadPointer--;
      else PayloadInfo.PayloadPointer = (char*) &PayloadBuffer + 1023;
      
      count--;
    }

  //move to the newline right after REPEAT
  if( PayloadInfo.PayloadPointer < ((char*) &PayloadBuffer + 1023) ) PayloadInfo.PayloadPointer++;
  else PayloadInfo.PayloadPointer = (char*) &PayloadBuffer;
  skipString();
  
  return;
}

//return 1 and move PayloadPointer to the next unread symbol if a referenceString is found at PayloadPointer
static char checkKeyword(char* referenceString)
{
  char* whereToCheck = PayloadInfo.PayloadPointer;
  
  while(*referenceString)
    {
      //keep comparing characters until the end of reference string
      if( *whereToCheck != *referenceString ) return 0;      
      referenceString++;
      
      //go to next character in a string. if the end of PayloadBuffer is reached, move pointer back to start of buffer
      if( whereToCheck < ((char*) &PayloadBuffer + 1023) ) whereToCheck++;
      else whereToCheck = (char*) &PayloadBuffer;
    }
  
  //if requested keyword was actiually found, move PayloadPointer to next keyword
  PayloadInfo.PayloadPointer = whereToCheck;
  return 1;
}

//convert decimal string at PayloadPointer to unsigned integer, move to the newline
static unsigned int checkDecValue()
{
  int result = 0;
  unsigned char digit = 0;
  unsigned char limit = 6;//maximum number of digits in a string to be interpreted
  
  while(limit)
    {
      limit--;//one more digit is interpreted

      //convert ASCII symbol to integer
      if( ( *(PayloadInfo.PayloadPointer) > 47 ) && ( *(PayloadInfo.PayloadPointer) <  58 ) ) digit = *(PayloadInfo.PayloadPointer) - 48;
      else break;            
	    
      result = result * 10 + digit;//compute preliminary result

      //if the end of PayloadBuffer is reached, move pointer back to start of buffer
      if( PayloadInfo.PayloadPointer < ((char*) &PayloadBuffer + 1023) ) PayloadInfo.PayloadPointer++;
      else PayloadInfo.PayloadPointer = (char*) &PayloadBuffer;
    }

  skipString();//move to the end of line
  
  return result;
}

//convert hexadecimal string at PayloadPointer to unsigned integer, move to the newline
static unsigned int checkHexValue()
{
  int result = 0;
  unsigned char digit = 0;
  unsigned char limit = 4;//maximum number of digits in a string to be interpreted
  
  while(limit)
    {
      limit--;//one more digit is interpreted

      //convert ASCII symbol to integer
           if( ( *(PayloadInfo.PayloadPointer) > 47 ) && ( *(PayloadInfo.PayloadPointer) <  58 ) ) digit = *(PayloadInfo.PayloadPointer) - 48;
      else if( ( *(PayloadInfo.PayloadPointer) > 64 ) && ( *(PayloadInfo.PayloadPointer) <  71 ) ) digit = *(PayloadInfo.PayloadPointer) - 65 + 10;
      else if( ( *(PayloadInfo.PayloadPointer) > 96 ) && ( *(PayloadInfo.PayloadPointer) < 103 ) ) digit = *(PayloadInfo.PayloadPointer) - 97 + 10;
      else break;
      
      result = result * 16 + digit;//compute preliminary result
      
      //if the end of PayloadBuffer is reached, move pointer back to start of buffer
      if( PayloadInfo.PayloadPointer < ((char*) &PayloadBuffer + 1023) ) PayloadInfo.PayloadPointer++;
      else PayloadInfo.PayloadPointer = (char*) &PayloadBuffer;
    }
  
  skipString();//move to the end of line
  
  return result;
}

static void checkFilename()
{
  unsigned char limit = 8;//maximum number of symbols in a string to be interpreted

  while(limit)
    {            
      //allow only alphanumeric characters and underscores in the filename
           if(   *(PayloadInfo.PayloadPointer) == 95 )                                             PayloadInfo.LayoutFilename[8 - limit] = *(PayloadInfo.PayloadPointer);
      else if( ( *(PayloadInfo.PayloadPointer) > 47 ) && ( *(PayloadInfo.PayloadPointer) <  58 ) ) PayloadInfo.LayoutFilename[8 - limit] = *(PayloadInfo.PayloadPointer);
      else if( ( *(PayloadInfo.PayloadPointer) > 64 ) && ( *(PayloadInfo.PayloadPointer) <  91 ) ) PayloadInfo.LayoutFilename[8 - limit] = *(PayloadInfo.PayloadPointer);
      else if( ( *(PayloadInfo.PayloadPointer) > 96 ) && ( *(PayloadInfo.PayloadPointer) < 123 ) ) PayloadInfo.LayoutFilename[8 - limit] = *(PayloadInfo.PayloadPointer);
      else break;

      limit--;//one more symbol is interpreted
	   
      //if the end of PayloadBuffer is reached, move pointer back to start of buffer
      if( PayloadInfo.PayloadPointer < ((char*) &PayloadBuffer + 1023) ) PayloadInfo.PayloadPointer++;
      else PayloadInfo.PayloadPointer = (char*) &PayloadBuffer;
    }
  
  skipString();//move to the end of line
  
  return;
}

//prints out a string of ASCII printable characters, other symbols act as string terminators
//if PayloadPointer was used as argument ( as opposed to sendString("literal string"); ), use data from PayloadBuffer
static void sendString(char* stringStart)
{
  unsigned char modbyte = MOD_NONE;//modifier byte to send in a report
  unsigned char keycode = KB_Reserved;//HID keycode to send in a report
  unsigned short limit = 400;//maximum number of symbols by which PayloadPointer is allowed to move
  
  sendKeycode(MOD_NONE | PayloadInfo.HoldModifiers, KB_Reserved);//send keycode 0x00 to make sure next keystroke is registered as new
  
  while( ( *stringStart > 31 ) && ( *stringStart < 127 ) )//continue until first unsupported character is encountered
    {
      if(limit) limit--;//if limit of symbols is reached, release all buttons and freeze ducky interpreter
      else while(1) sendKeycode(MOD_NONE, KB_Reserved);
      
      keycode = Keymap[*stringStart - 32];//map ASCII symbol into HID keycode + LSHIFT indicator bit
      if( keycode & (1<<7) ) modbyte = MOD_LSHIFT;//if most significant bit is set use LSHIFT modifier
      else modbyte = MOD_NONE;
      if( Keymap[ (*stringStart - 32) / 8 + 95 ] & (1 << (*stringStart % 8)) ) modbyte = modbyte | MOD_RALT;//if corresponding AltGr bit is set, use RALT modifier
      
      sendKeycode(modbyte  | PayloadInfo.HoldModifiers, keycode);//send specified keycode (but keep HOLD modifiers, if present)
      sendKeycode(MOD_NONE | PayloadInfo.HoldModifiers, KB_Reserved);//release all buttons (but keep HOLD modifiers, if present)      
      
      //if argument to this fuction was PayloadInfo.PayloadPointer, move it to the next symbol
      if( stringStart == PayloadInfo.PayloadPointer)
	{
	  //go to next character in a string. if the end of PayloadBuffer is reached, move pointer back to start of buffer
	  if( PayloadInfo.PayloadPointer < ((char*) &PayloadBuffer + 1023) ) PayloadInfo.PayloadPointer++;
	  else PayloadInfo.PayloadPointer = (char*) &PayloadBuffer;
	  stringStart = PayloadInfo.PayloadPointer;
	}
      //if argument was a literal string, go to the next symbol
      else stringStart++;
    }

  sendKeycode(MOD_NONE | PayloadInfo.HoldModifiers, KB_Reserved);//release all buttons (but keep HOLD modifiers, if present)
  skipString();//move to the end of line, if not there already
  
  return;
}

//only accepts keycodes 0 to 101, other values are ignored
static void sendKeycode(unsigned char modifiers, unsigned char keycode)
{ 
  BTABLE->COUNT1_TX = 8;//data payload size for next IN transaction = 8 bytes

  keycode = keycode & 0x7F;//only use 7 least significant bits as keycode
  if(keycode > 101) keycode = KB_Reserved;//if keycode is not recognized send no keystroke

  //copy data from device descriptor in RAM to PMA buffer EP1_TX
  *( (unsigned short*) (BTABLE_BaseAddr + BTABLE->ADDR1_TX + 0) ) = modifiers;
  *( (unsigned short*) (BTABLE_BaseAddr + BTABLE->ADDR1_TX + 2) ) = keycode;
  *( (unsigned short*) (BTABLE_BaseAddr + BTABLE->ADDR1_TX + 4) ) = 0;
  *( (unsigned short*) (BTABLE_BaseAddr + BTABLE->ADDR1_TX + 6) ) = 0;
  
  USB->EP1R = (1<<10)|(1<<9)|(1<<4)|(1<<0);//respond with data to next IN transaction
  delay_us(1);//give USB peripheral time to update EP1R register
  while( (USB->EP1R & 0x0030) == 0x0030 );//wait until STAT_TX has changed from VALID to NAK (means host has received the report)
  
  return;
}

//move PayloadPointer to the next newline
static void skipString()
{
  unsigned short limit = 400;//maximum number of symbols by which PayloadPointer is allowed to move
  
  while( *(PayloadInfo.PayloadPointer) != 0x0A )
    {
      if(limit) limit--;//if limit of symbols is reached, release all buttons and freeze ducky interpreter
      else while(1) sendKeycode(MOD_NONE, KB_Reserved);
      
      //go to next character in a string. if the end of PayloadBuffer is reached, move pointer back to start of buffer
      if( PayloadInfo.PayloadPointer < ((char*) &PayloadBuffer + 1023) ) PayloadInfo.PayloadPointer++;
      else PayloadInfo.PayloadPointer = (char*) &PayloadBuffer;
    }
  
  return;
}

//wait a specified time in milliseconds, but extend wait time if MSD or HID interfaces are not completely initialized by the host yet
static void autodelay(unsigned int delay)
{
    while( !PayloadInfo.FirstRead );//wait until MSD interface has received at least 1 read command since poweron
    sendKeycode(MOD_NONE, KB_Reserved);//wait until host received at least 1 report from HID interface
    
    delay_ms(delay);//wait a specified time after that
    
    return;
  }

//----------------------------------------------------------------------------------------------------------------------

static inline void delay_us(unsigned int delay)
{
  if(delay)//if delay is nonzero
    {
      TIM2->PSC = 0;//TIM2 prescaler = 1
      TIM2->ARR = delay * 48 - 10;//TIM2 reload value
      TIM2->CR1 = (1<<7)|(1<<3)|(1<<0);//auto-reload enable, one pulse mode, start upcounting
      while(TIM2->CR1 & (1<<0));//wait until timer has finished counting
    }
  
  return;
}

static void delay_ms(unsigned int delay)
{
  if(delay)//if delay is nonzero
    {      
      TIM2->PSC = 999;//TIM2 prescaler = 1000
      TIM2->ARR = delay * 48;//TIM2 reload value
      TIM2->CR1 = (1<<7)|(1<<3)|(1<<0);//auto-reload enable, one pulse mode, start upcounting
      while(TIM2->CR1 & (1<<0));//wait until timer has finished counting
    }

  return;
}

//----------------------------------------------------------------------------------------------------------------------

//compare current OS fingerprint with those stored in the specified directory; return 1 on match, 0 otherwise
static unsigned char checkOSfingerprint(char* directoryName)
{
  unsigned char* currentFingerprint = (unsigned char*) &(ControlInfo.OSfingerprintData[0]);
  unsigned char i;

  DIR fingerprintDirInfo;
  FILINFO fingerprintFileInfo;

  //go to the specified directory and search for any *.fgp files
  f_chdir(directoryName);
  FATFSresult = f_findfirst(&fingerprintDirInfo, &fingerprintFileInfo, directoryName, "*.fgp");

  //keep searching until no more *.fgp files are found
  while( !FATFSresult && fingerprintFileInfo.fname[0] )
    {
      //read the contents of *.fgp file
      f_open( &openedFileInfo, &(fingerprintFileInfo.fname[0]), FA_READ );
      f_read( &openedFileInfo, (char*) &PayloadBuffer, 40, &BytesRead );
      f_close(&openedFileInfo);
      
      //compare current OS fingerprint with the data from *.fgp file
      i = 0;
      while(i < 40)
	{
	  if( currentFingerprint[i] != PayloadBuffer[i] ) break;
	  
	  //for every 4-byte fingerprint member check bytes 0, 1, 3; ignore byte 2
	  if( (i % 4) == 1 ) i = i + 2;
	  else i = i + 1;
	}
      if(i == 40) return 1;
      
      FATFSresult = f_findnext(&fingerprintDirInfo, &fingerprintFileInfo);
    }
  
  return 0;
}
