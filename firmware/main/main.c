#include "../cmsis/stm32f0xx.h"
#include "../usb/usb.h"
#include "../usb/hid_rodata.h"
#include "../fatfs/ff.h"
#include "../fatfs/diskio.h"

extern ControlInfo_TypeDef ControlInfo;
extern DeviceDescriptor_TypeDef DeviceDescriptor;
extern unsigned short StringDescriptor_1[13];

static char PayloadBuffer[2048] __attribute__(( aligned(2) ));//holds duckyscript commands to execute
static FATFS FATFSinfo;//holds info about filesystem on the medium
static FIL openedFileInfo;//holds info related to an opened file
static unsigned int BytesRead;//holds the number of bytes that were successfully read with f_read()
static FRESULT FATFSresult;//holds return values of FATFS related funtions

static void readConfigFile(char* filename);
static void waitForInit();
static unsigned short countCapsToggles(unsigned short toggleLimit);
static void runDuckyPayload(char* filename);
static void runDuckyCommand();
static void repeatDuckyCommands(unsigned int count);
static char checkKeyword(char* referenceString);
static unsigned int checkDecValue();
static unsigned int checkHexValue();
static void setFilename(char* newName);
static void setSerialNumber(char* newSerialNumber);
static void sendString(char* stringStart);
static void sendKBreport(unsigned short modifiers, unsigned int keycodes);
static void sendMSreport(unsigned int mousedata);
static void skipSpaces();
static void skipString();

static inline void delay_us(unsigned int delay) __attribute__((always_inline));
static void delay_ms(unsigned int delay);
static void restart_tim6(unsigned short time);
static void enter_bootloader();

static void saveOSfingerprint();
static void checkOSfingerprint();

PayloadInfo_TypeDef PayloadInfo =
  {
    .DefaultDelay = 0,//if  DEFAULT_DELAY is not explicitly set, use 0ms value
    .StringDelay = 0,//if STRING_DELAY is not explicitly set, use 0ms value
    .PayloadPointer = (char*) &PayloadBuffer,//start interpreting commands at the beginning of PayloadBuffer
    .BytesLeft = 0,//nothing was saved in PayloadBuffer yet
    .RepeatStart = 0,//repeat start is at the beginning of the file
    .RepeatCount = 0xFFFFFFFF,//RepeatCount was not specified yet
    .HoldMousedata = MOUSE_IDLE,//no mouse activity present
    .HoldKeycodes= KB_Reserved,//no keycodes are applied    
    .HoldModifiers = MOD_NONE,//no modifiers are applied
    .LBAoffset = 0,//all blocks are available to MSD interface
    .FakeCapacity = 0,//use real capacity by default
    .Filename = {0x00},//no target filename is specified yet
    .PayloadFlags = 0,//no payload specific flags are set
    .DeviceFlags = 0//no device wide flags are set
  };

//----------------------------------------------------------------------------------------------------------------------

int main()
{
  unsigned short toggleCount = 0;//holds number of times that capslock was switched on/off within a given toggle sequence
  
  //initialize W25N01GVZEIG flash memory chip, mount filesystem on the first partition, open and read payload.txt
  delay_ms(5);//wait until W25N01GVZEIG startup delay is over and it is able to accept commands
  __enable_irq();//enable interrupts globally (this is required for W25N01GVZEIG flash memory operations)
  
  if( f_mount(&FATFSinfo, "0:", 1) )//if no valid FAT is found
    {
      disk_initialize(0);//initialize flash memory for MSD use
      usb_init();//initialize USB
      
      while(1)
	{
	  sendKBreport(MOD_NONE, KB_Reserved);//send an empty report
	  
	  if(countCapsToggles(65535) > 19)//if there was 20 or more capslock toggles in a given sequence
	    { 
	      RCC->APB1RSTR |= (1<<23);//reset USB registers, detach from the USB bus
	      delay_ms(1000);//wait for 1000ms
	      RCC->APB1RSTR &= ~(1<<23);//deassert USB reset
	      enter_bootloader();//enter DFU mode right away
	    }
	}
    }
  else//if filesystem was successfully mounted
    { 
      f_chdir("0:/");
      readConfigFile("config.txt");
      
      usb_init();//initialize USB
      while(ControlInfo.OSfingerprintCounter < 10);//wait until OS fingerprint is collected      
      if(PayloadInfo.DeviceFlags & (1<<3)) waitForInit();
      
      if(PayloadInfo.DeviceFlags & (1<<2))//if there is a USE_FINGERPRINTER command in config.txt
	{
	  NVIC_DisableIRQ(31);//disable usb interrupt, so that MSD access and fatfs access do not collide      
	  saveOSfingerprint();//save collected OS fingerprint in /fingerdb/current.fgp
	  checkOSfingerprint();//set appropriate payload filename in PayladInfo.Filename[]
	  f_chdir("0:/fgscript");//go to /fgscript/ directory
	  NVIC_EnableIRQ(31);//enable USB interrupt
	  
	  //unless explicitly disabled, run OS specific ON-INSERTION script
	  if( !(PayloadInfo.DeviceFlags & (1<<0)) ) runDuckyPayload(PayloadInfo.Filename);
	}
      else//if fingerprinter is disabled
	{
	  NVIC_DisableIRQ(31);//disable USB interrupt so that MSD and fatfs data access do not collide
	  f_chdir("0:/");//go to the root directory
	  NVIC_EnableIRQ(31);//enable USB interrupt
	  
	  //unless explicitly disabled, run default ON-INSERTION script
	  if( !(PayloadInfo.DeviceFlags & (1<<0)) ) runDuckyPayload("payload.txt");
	}                  
      
      
      while(1)
	{
	  sendKBreport(MOD_NONE, KB_Reserved);//send an empty keyboard report
	  sendMSreport(MOUSE_IDLE);//send an empty mouse report
	  	  
	  toggleCount = countCapsToggles(65535);//count number of capslock toggles in one sequence
	  
	  //if there was from 3 to 19 capslock toggles in a given sequence
	  if( (toggleCount > 2) && (toggleCount < 20) )
	    {
	      NVIC_DisableIRQ(31);//disable USB interrupt to prevent f_mount() and MSD access collision
	      FATFSresult  = f_mount(&FATFSinfo, "0:", 1);//remount the filesystem, just in case new on-demand scripts were added
	      FATFSresult |= f_chdir("0:/ondemand");//go to the /ondemand/ directory
	      setFilename("script__.txt");//set next payload filename to script__.txt
	      PayloadInfo.Filename[6] = 48 + toggleCount / 10;//set correct on-demand payload name
	      PayloadInfo.Filename[7] = 48 + toggleCount % 10;//set correct on-demand payload name
	      NVIC_EnableIRQ(31);//enable USB interrupt
	      
	      if(!FATFSresult) runDuckyPayload(PayloadInfo.Filename);//execute specified on-demand script
	    }
	  else if(toggleCount > 19)//if there was 20 or more capslock toggles in a given sequence
	    {
	      PayloadInfo.LBAoffset = 0;//do not hide any blocks from MSD interface
	      PayloadInfo.FakeCapacity = 0;//show real capacity
	      ControlInfo.EnumerationMode = 0;//set enumeration mode to default	  
	      
	      RCC->APB1RSTR |= (1<<23);//reset USB registers, detach from the USB bus
	      delay_ms(1000);//wait for 1000ms
	      RCC->APB1RSTR &= ~(1<<23);//deassert USB reset
	      if(PayloadInfo.DeviceFlags & (1<<3)) enter_bootloader();//if DFUmodeFlag is set
	      else                                 usb_init();//if DFUmode flag was not set
	      
	      NVIC_DisableIRQ(31);//disable USB interrupt to prevent race condition
	      __DSB();//make sure NVIC registers are updated before ISB is executed
	      __ISB();//make sure the latest NVIC setting is used immediately
	      PayloadInfo.DeviceFlags |=  (1<<3);//set DFUmodeFlag
	      PayloadInfo.DeviceFlags &= ~(1<<1);//clear FirstReadFlag
	      NVIC_EnableIRQ(31);//enable USB interrupt again
	    }
	}
      
    }
  
  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

//open a file called filename inside current directory and execute all pre-configuration commands inside
static void readConfigFile(char* filename)
{
  unsigned int commandStart = (unsigned int) &PayloadBuffer;//remember where start of command is
  unsigned int firstLBA;//holds first LBA of the first partition
  unsigned int LBAcount;//holds size of first partition in LBA
  unsigned char MSDbutton;//holds state of the MSD-only button (1 = pressed, 0 = released)
  
  //sample the state of MSD-only button at the time of insertion
  if( !(GPIOA->IDR & (1<<2)) ) MSDbutton = 1;
  else                         MSDbutton = 0;
  
  if( !f_open(&openedFileInfo, filename, FA_READ | FA_OPEN_EXISTING) )//if configuration file was successfully opened
    {
      f_read(&openedFileInfo, (char*) &PayloadBuffer, 512, &BytesRead );
      PayloadInfo.PayloadPointer = (char*) &PayloadBuffer;//move PayloadPointer back to start
      PayloadInfo.BytesLeft = BytesRead + 1;//one extra newline character will be appended to the end of payload script
      //always append a newline after config file contents; that prevents bricking the device by skipString() if there is no newline in config.txt
      PayloadBuffer[BytesRead] = 0x0A;
      f_close(&openedFileInfo);
      
      //run pre-configuration commands from specified configuration file
      while(PayloadInfo.BytesLeft)
	{
	  commandStart = (unsigned int) PayloadInfo.PayloadPointer;//remember where start of current command is
	  skipSpaces();
	  
	  
	       if( checkKeyword("VID 0x") )     {DeviceDescriptor.idVendor  = (unsigned short) checkHexValue();}
	  else if( checkKeyword("PID 0x") )     {DeviceDescriptor.idProduct = (unsigned short) checkHexValue();}
	  else if( checkKeyword("SERIAL ") )    {setSerialNumber(PayloadInfo.PayloadPointer);}
	  else if( checkKeyword("MASS_ERASE") ) {mass_erase(); NVIC_SystemReset();}
	  else if( (MSDbutton == 0) && checkKeyword("HID_ONLY_MODE") ) {ControlInfo.EnumerationMode = 1;}
	  
	  else if( (MSDbutton == 0) && checkKeyword("USE_HIDDEN_REGION") )
	    {
	      //read LBA 0 into an unused area of PayloadBuffer
	      disk_read(0, (unsigned char*) &PayloadBuffer + 1024, 0, 1);
	      
	      //if a valid MBR signature is found, set LBAoffset value based on first partition entry
	      if( (PayloadBuffer[1534] == 0x55) && (PayloadBuffer[1535] == 0xAA) )
		{
		  firstLBA  = *( (unsigned short*) (&PayloadBuffer[1478]) ) <<  0;
		  firstLBA |= *( (unsigned short*) (&PayloadBuffer[1480]) ) << 16;
		  LBAcount  = *( (unsigned short*) (&PayloadBuffer[1482]) ) <<  0;
		  LBAcount |= *( (unsigned short*) (&PayloadBuffer[1484]) ) << 16;
		  //if LBAoffset is 16MiB of less, hide the specified LBA's
		  if( (firstLBA + LBAcount) <= 32768 ) PayloadInfo.LBAoffset = firstLBA + LBAcount;
		}
	    }
	  else if( (MSDbutton == 0) && checkKeyword("SHOW_FAKE_CAPACITY ") )
	    {
	      PayloadInfo.FakeCapacity = checkDecValue();//read fake capacity value (in MiB)
	      //only allow fake capacity from 97MiB to 32GiB
	      if(PayloadInfo.FakeCapacity > 32768) PayloadInfo.FakeCapacity = 0;
	      if(PayloadInfo.FakeCapacity < 97)    PayloadInfo.FakeCapacity = 0;
	    }
	  else if( checkKeyword("FIRST_INSERT_ONLY") )
	    {
	      //if 0:/noinsert file is already present, set NoInsertFlag
	      if( !f_open(&openedFileInfo, "noinsert", FA_READ | FA_OPEN_EXISTING ) ) PayloadInfo.DeviceFlags |= (1<<0);
	      //if 0:/noinsert file is not present, create noinsert file, but keep NoInsertFlag cleared
	      else f_open(&openedFileInfo, "noinsert", FA_WRITE | FA_CREATE_ALWAYS);
	      
	      f_close(&openedFileInfo);
	    }
	  else if( checkKeyword("USE_FINGERPRINTER") )
	    {
	      f_mkdir("0:/fgscript");
	      f_mkdir("0:/fingerdb");
	      PayloadInfo.DeviceFlags |= (1<<2);
	    }
	  else if( checkKeyword("USE_LAYOUT ") )
	    {	      	      
	      f_mkdir("0:/kblayout");//create /kbalyout/
	      f_chdir("0:/kblayout");//go to /kblayout/ directory
	      setFilename(PayloadInfo.PayloadPointer);
	      
	      //load new keymap from the specified file
	      if( !f_open(&openedFileInfo, (char*) &(PayloadInfo.Filename), FA_READ | FA_OPEN_EXISTING) )
		{
		  f_read(&openedFileInfo, (unsigned char*) &Keymap, 107, &BytesRead );
		  f_close( &openedFileInfo );
		}
	    }	  	 
	  
	  //go to the next line
	  skipString();
	  if( PayloadInfo.PayloadPointer < ((char*) &PayloadBuffer + 2047) ) PayloadInfo.PayloadPointer++;
	  else PayloadInfo.PayloadPointer = (char*) &PayloadBuffer;
	  
	  PayloadInfo.BytesLeft = PayloadInfo.BytesLeft - ((unsigned int) PayloadInfo.PayloadPointer - commandStart);
	}
    }
  
  //if MSD-only button was pressed set NoInsertFlag
  if(MSDbutton) PayloadInfo.DeviceFlags |= (1<<0);
  
  return;
}

static void waitForInit()
{
  unsigned char LEDstate;//holds last sampled value of capslock LED state
  
  sendKBreport(MOD_NONE, KB_Reserved);//send an empty report
  delay_ms(50);//wait for 50 milliseconds before sending any keys
  LEDstate = *((unsigned char*) (BTABLE_BaseAddr + BTABLE->ADDR1_RX + ControlInfo.HIDprotocol)) & (1<<1);//sample current capslock LED state
  
  while( !LEDstate )//keep toggling capslock until it is turned on
    {
      sendKBreport(MOD_NONE, KB_CAPSLOCK);//press capslock key
      delay_ms(5);//keep capslock pressed for 5 milliseconds
      sendKBreport(MOD_NONE, KB_Reserved);//send an empty report
      
      restart_tim6(200);//start TIM6, wait up to 200ms for host to acknowledge capslock press event (if not, try again)
      while( (TIM6->CR1 & (1<<0)) && !LEDstate ) LEDstate = *((unsigned char*) (BTABLE_BaseAddr + BTABLE->ADDR1_RX + ControlInfo.HIDprotocol)) & (1<<1);
    }
  
  while( LEDstate )//keep toggling capslock until it is turned off
    {
      sendKBreport(MOD_NONE, KB_CAPSLOCK);//press capslock key
      delay_ms(5);//keep capslock pressed for 5 milliseconds
      sendKBreport(MOD_NONE, KB_Reserved);//send an empty report
      
      restart_tim6(1010);//start TIM6, wait up to 1010ms for host to acknowledge capslock press event (if not, try again)
      while( (TIM6->CR1 & (1<<0)) &&  LEDstate ) LEDstate = *((unsigned char*) (BTABLE_BaseAddr + BTABLE->ADDR1_RX + ControlInfo.HIDprotocol)) & (1<<1);
    }
  
  delay_ms(50);//wait for 50 milliseconds for some late arriving LED toggle
  LEDstate = *((unsigned char*) (BTABLE_BaseAddr + BTABLE->ADDR1_RX + ControlInfo.HIDprotocol)) & (1<<1);//sample capslock state once more
  if( LEDstate )//if late arriving capslock was detected, toggle capslock for one more time
    {
      sendKBreport(MOD_NONE, KB_CAPSLOCK);//press capslock key
      delay_ms(5);//keep capslock pressed for 5 milliseconds
      sendKBreport(MOD_NONE, KB_Reserved);//send an empty report
    }
  
  //unless HID-only mode is used, make sure MSD interface has received at least 1 read command
  while( (ControlInfo.EnumerationMode == 0) && !(PayloadInfo.DeviceFlags & (1<<1)) );
  
  return;
}

static unsigned short countCapsToggles(unsigned short toggleLimit)
{
  unsigned short toggleCount = 0;//holds number of times that capslock was switched on/off    
  unsigned char oldLEDstate;//holds previous sampled value of capslock LED state
  unsigned char newLEDstate;//holds latest   sampled value of capslock LED state
  
  restart_tim6(1010);//start capslock sequence timer, run for 1010ms

  //keep going until capslock sequence timer runs out or toggleLimit is reached
  while( (TIM6->CR1 & (1<<0)) && (toggleCount < toggleLimit) )
    {
      //sample capslock LED states before and after a short delay
      oldLEDstate = *((unsigned char*) (BTABLE_BaseAddr + BTABLE->ADDR1_RX + ControlInfo.HIDprotocol)) & (1<<1);
      delay_ms(1);//wait for 1 millisecond
      garbage_collect();//erase an invalid block if found
      delay_ms(1);//wait for 1 millisecond
      newLEDstate = *((unsigned char*) (BTABLE_BaseAddr + BTABLE->ADDR1_RX + ControlInfo.HIDprotocol)) & (1<<1);
      
      //if capslock LED state have changed since previous sample
      if( oldLEDstate != newLEDstate )
	{
	  restart_tim6(1010);//restart capslock toggle timer, run for 1010ms
	  toggleCount++;//one more capslock toggle event was detected
	}
    }
  
  return toggleCount;
}

//open a file called filename inside current directory and execute all ducky commands inside
static void runDuckyPayload(char* filename)
{
  unsigned char replaceFlag = 0;//will be set to 1 every time there is a need to replace some data in PayloadBuffer

  //reinitialize PayloadInfo variables which are script specific
  PayloadInfo.DefaultDelay = 0;
  PayloadInfo.StringDelay = 0;
  PayloadInfo.PayloadPointer = (char*) &PayloadBuffer;
  PayloadInfo.BytesLeft = 0;
  PayloadInfo.RepeatStart = 0;
  PayloadInfo.RepeatCount = 0xFFFFFFFF;
  PayloadInfo.HoldMousedata = MOUSE_IDLE;
  PayloadInfo.HoldKeycodes = KB_Reserved;
  PayloadInfo.HoldModifiers = MOD_NONE;
  PayloadInfo.PayloadFlags = 0;
  
  NVIC_DisableIRQ(31);//disable usb interrupt, so f_open() and MSD access do not collide
  FATFSresult = f_open(&openedFileInfo, filename, FA_READ | FA_OPEN_EXISTING);//try to read the specified file
  NVIC_EnableIRQ(31);//enable usb interrupt
  
  //if appropriate payload file was successfully opened, run ducky interpreter
  if( !FATFSresult )
    {
      NVIC_DisableIRQ(31);//disable usb interrupt, so f_read() and MSD access do not collide
      f_read(&openedFileInfo, (char*) &PayloadBuffer, 2048, &BytesRead );
      PayloadInfo.BytesLeft = BytesRead + 1;//one extra newline character will be appended to the end of payload script
      if(BytesRead < 2048) PayloadBuffer[BytesRead] = 0x0A;//if end of file is detected, add an extra newline
      NVIC_EnableIRQ(31);//enable usb interrupt
      
      //keep executing commands until end of file is reached
      while(PayloadInfo.BytesLeft)
	{ 	  	  
	  //if at least half of PayloadBuffer was processed and can now be replaced with new data
               if( !(PayloadInfo.PayloadFlags & (1<<0)) && (PayloadInfo.PayloadPointer >= ((char*) &PayloadBuffer + 1024)) ) replaceFlag = 1;
	  else if(  (PayloadInfo.PayloadFlags & (1<<0)) && (PayloadInfo.PayloadPointer <  ((char*) &PayloadBuffer + 1024)) ) replaceFlag = 1;
	  
	  if(replaceFlag)
	    {
	      NVIC_DisableIRQ(31);//disable usb interrupt, so f_read() and MSD access do not collide
	      f_read( &openedFileInfo, (char*) &PayloadBuffer + (PayloadInfo.PayloadFlags % 2) * 1024, 1024, &BytesRead );//first 1024 bytes can now be overwritten
	      PayloadInfo.BytesLeft = PayloadInfo.BytesLeft + BytesRead;
	      if(BytesRead < 1024) PayloadBuffer[ (PayloadInfo.PayloadFlags % 2) * 1024 + BytesRead ] = 0x0A;//if end of file is detected, add an extra newline
	      
	      PayloadInfo.PayloadFlags ^= (1<<0);//commands are now being executed from the other half of PayloadBuffer	      
	      replaceFlag = 0;//reset loadFlag back to 0
	      NVIC_EnableIRQ(31);//enable usb interrupt again
	    }

	  runDuckyCommand();//execute one line of ducky script, subtract number of executed bytes from BytesLeft
	}
    }
  
  NVIC_DisableIRQ(31);//disable usb interrupt, so f_close() and MSD access do not collide
  f_close(&openedFileInfo);//close the specified file  
  NVIC_EnableIRQ(31);//enable usb interrupt
  
  return;
}

//run current command from PayloadBuffer, move PayloadPointer to the next line
static void runDuckyCommand()
{
  unsigned int commandStart = (unsigned int) PayloadInfo.PayloadPointer;//remember where start of command is
  unsigned int bytesUsed = 0;//number of characters inside current command (including newline termination byte)
  unsigned short limit = 11;//maximum number of keywords allowed to be in one line of ducky script
  
  unsigned int   mousedata = PayloadInfo.HoldMousedata;//mouse activity data to send in a next report
  unsigned char* mousedataPointer = (unsigned char*) &mousedata;//used to select a paricular byte of mousedata in a report
  unsigned int   keycodes = PayloadInfo.HoldKeycodes;//HID keycodes to send in a next report
  unsigned char* keycodePointer = (unsigned char*) &keycodes;//used to select a paricular keycode in a report
  unsigned short modifiers = PayloadInfo.HoldModifiers;//modifier byte to send in a next report
  
  //search for specific keywords, take appropriate actions if found; keep going until the end of line is found
  while( *(PayloadInfo.PayloadPointer) != 0x0A )
    {
      if(limit) limit--;//if limit of keywords is reached, release all buttons and freeze ducky interpreter
      else while(1) sendKBreport(MOD_NONE, KB_Reserved);
      
      skipSpaces();
      
      //if PayloadPointer is at some ASCII-nonprintable character, skip the rest of the line
      if( ( *(PayloadInfo.PayloadPointer) < 32 ) || ( *(PayloadInfo.PayloadPointer) > 126 ) ) skipString();

      //special functionality commands are only valid if they are at the very start of the line
      else if( (limit == 10) && checkKeyword("REM") )              {skipString();}
      else if( (limit == 10) && checkKeyword("REPEAT_START") )     {PayloadInfo.RepeatStart = openedFileInfo.fptr + 1 - PayloadInfo.BytesLeft; PayloadInfo.PayloadFlags |= (1<<1); skipString();}
      else if( (limit == 10) && checkKeyword("REPEAT ") )          {commandStart = (unsigned int) &PayloadBuffer; limit = 11; repeatDuckyCommands( checkDecValue() );}
      else if( (limit == 10) && checkKeyword("ONACTION_DELAY ") )  {PayloadInfo.DefaultDelay = checkDecValue(); PayloadInfo.DeviceFlags |=  (1<<4); skipString();}
      else if( (limit == 10) && checkKeyword("DEFAULT_DELAY ") )   {PayloadInfo.DefaultDelay = checkDecValue(); PayloadInfo.DeviceFlags &= ~(1<<4); skipString();}
      else if( (limit == 10) && checkKeyword("DEFAULTDELAY ") )    {PayloadInfo.DefaultDelay = checkDecValue(); PayloadInfo.DeviceFlags &= ~(1<<4); skipString();}
      else if( (limit == 10) && checkKeyword("STRING_DELAY ") )    {PayloadInfo.StringDelay  = checkDecValue(); skipString();}
      else if( (limit == 10) && checkKeyword("DELAY ") )           {delay_ms( checkDecValue() ); skipString();}
      else if( (limit == 10) && checkKeyword("WAITFOR_INIT") )     {waitForInit(); skipString();}
      else if( (limit == 10) && checkKeyword("WAITFOR_RESET") )    {while( ControlInfo.DeviceState != DEFAULT ); skipString();}
      else if( (limit == 10) && checkKeyword("WAITFOR_CAPSLOCK") ) {while( countCapsToggles(2) < 2 ); skipString();}
      else if( (limit == 10) && checkKeyword("ALLOW_EXIT") )       {if( countCapsToggles(65535) > 1 ) {PayloadInfo.BytesLeft = 0; return;} else skipString(); }
      
      else
	{
	  PayloadInfo.PayloadFlags |= (1<<4);//set ActionFlag
	  
	       if( (limit == 10) && checkKeyword("STRING ") ) {sendString( PayloadInfo.PayloadPointer ); skipString();}
	  else if( (limit == 10) && checkKeyword("HOLD ") )   {PayloadInfo.PayloadFlags |= (1<<2); modifiers = 0; keycodes = 0; mousedata = 0;}
	  else if( (limit == 10) && checkKeyword("RELEASE") ) {PayloadInfo.PayloadFlags |= (1<<2); modifiers = 0; keycodes = 0; mousedata = 0; skipString();}
	  
	  else if( checkKeyword("GUI") )        modifiers = modifiers | MOD_LGUI;
	  else if( checkKeyword("WINDOWS") )    modifiers = modifiers | MOD_LGUI;
	  else if( checkKeyword("CTRL") )       modifiers = modifiers | MOD_LCTRL;
	  else if( checkKeyword("CONTROL") )    modifiers = modifiers | MOD_LCTRL;
	  else if( checkKeyword("SHIFT") )      modifiers = modifiers | MOD_LSHIFT;
	  else if( checkKeyword("ALT") )        modifiers = modifiers | MOD_LALT;
	  else if( checkKeyword("RGUI") )       modifiers = modifiers | MOD_RGUI;
	  else if( checkKeyword("RCTRL") )      modifiers = modifiers | MOD_RCTRL;
	  else if( checkKeyword("RSHIFT") )     modifiers = modifiers | MOD_RSHIFT;
	  else if( checkKeyword("RALT") )       modifiers = modifiers | MOD_RALT;
	  
	  else if( checkKeyword("MOUSE_LEFTCLICK") )  {mousedata = mousedata | MOUSE_LEFTCLICK;  PayloadInfo.PayloadFlags |= (1<<3);}
	  else if( checkKeyword("MOUSE_RIGHTCLICK") ) {mousedata = mousedata | MOUSE_RIGHTCLICK; PayloadInfo.PayloadFlags |= (1<<3);}
	  else if( checkKeyword("MOUSE_MIDCLICK") )   {mousedata = mousedata | MOUSE_MIDCLICK;   PayloadInfo.PayloadFlags |= (1<<3);}
	  
	  else if( checkKeyword("MOUSE_RIGHT ") )      {mousedataPointer[1] = +(checkDecValue() % 128); PayloadInfo.PayloadFlags |= (1<<3);}
	  else if( checkKeyword("MOUSE_LEFT ") )       {mousedataPointer[1] = -(checkDecValue() % 128); PayloadInfo.PayloadFlags |= (1<<3);}
	  else if( checkKeyword("MOUSE_DOWN ") )       {mousedataPointer[2] = +(checkDecValue() % 128); PayloadInfo.PayloadFlags |= (1<<3);}
	  else if( checkKeyword("MOUSE_UP ") )         {mousedataPointer[2] = -(checkDecValue() % 128); PayloadInfo.PayloadFlags |= (1<<3);}
	  else if( checkKeyword("MOUSE_SCROLLUP ") )   {mousedataPointer[3] = +(checkDecValue() % 128); PayloadInfo.PayloadFlags |= (1<<3);}
	  else if( checkKeyword("MOUSE_SCROLLDOWN ") ) {mousedataPointer[3] = -(checkDecValue() % 128); PayloadInfo.PayloadFlags |= (1<<3);}
	  
	  //key press commands are only valid if no more than 4 non-modifier keys are pressed simultaneously
	  else if( (keycodePointer[3] == 0) && checkKeyword("KEYCODE 0x") )  keycodes = (keycodes << 8) | (checkHexValue() % 222);
	  else if( (keycodePointer[3] == 0) && checkKeyword("KEYCODE ") )    keycodes = (keycodes << 8) | (checkDecValue() % 222);
	  else if( (keycodePointer[3] == 0) && checkKeyword("MENU") )        keycodes = (keycodes << 8) | KB_COMPOSE;
	  else if( (keycodePointer[3] == 0) && checkKeyword("APP") )         keycodes = (keycodes << 8) | KB_COMPOSE;
	  else if( (keycodePointer[3] == 0) && checkKeyword("ENTER") )       keycodes = (keycodes << 8) | KB_RETURN;
	  else if( (keycodePointer[3] == 0) && checkKeyword("RETURN") )      keycodes = (keycodes << 8) | KB_RETURN;
	  else if( (keycodePointer[3] == 0) && checkKeyword("DOWNARROW") )   keycodes = (keycodes << 8) | KB_DOWNARROW;
	  else if( (keycodePointer[3] == 0) && checkKeyword("LEFTARROW") )   keycodes = (keycodes << 8) | KB_LEFTARROW;
	  else if( (keycodePointer[3] == 0) && checkKeyword("RIGHTARROW") )  keycodes = (keycodes << 8) | KB_RIGHTARROW;
	  else if( (keycodePointer[3] == 0) && checkKeyword("UPARROW") )     keycodes = (keycodes << 8) | KB_UPARROW;
	  else if( (keycodePointer[3] == 0) && checkKeyword("DOWN") )        keycodes = (keycodes << 8) | KB_DOWNARROW;
	  else if( (keycodePointer[3] == 0) && checkKeyword("LEFT") )        keycodes = (keycodes << 8) | KB_LEFTARROW;
	  else if( (keycodePointer[3] == 0) && checkKeyword("RIGHT") )       keycodes = (keycodes << 8) | KB_RIGHTARROW;
	  else if( (keycodePointer[3] == 0) && checkKeyword("UP") )          keycodes = (keycodes << 8) | KB_UPARROW;
	  else if( (keycodePointer[3] == 0) && checkKeyword("PAUSE") )       keycodes = (keycodes << 8) | KB_PAUSE;
	  else if( (keycodePointer[3] == 0) && checkKeyword("BREAK") )       keycodes = (keycodes << 8) | KB_PAUSE;
	  else if( (keycodePointer[3] == 0) && checkKeyword("CAPSLOCK") )    keycodes = (keycodes << 8) | KB_CAPSLOCK;
	  else if( (keycodePointer[3] == 0) && checkKeyword("DELETE") )      keycodes = (keycodes << 8) | KB_DELETE;
	  else if( (keycodePointer[3] == 0) && checkKeyword("END")  )        keycodes = (keycodes << 8) | KB_END;
	  else if( (keycodePointer[3] == 0) && checkKeyword("ESCAPE") )      keycodes = (keycodes << 8) | KB_ESCAPE;
	  else if( (keycodePointer[3] == 0) && checkKeyword("ESC") )         keycodes = (keycodes << 8) | KB_ESCAPE;
	  else if( (keycodePointer[3] == 0) && checkKeyword("HOME") )        keycodes = (keycodes << 8) | KB_HOME;
	  else if( (keycodePointer[3] == 0) && checkKeyword("INSERT") )      keycodes = (keycodes << 8) | KB_INSERT;
	  else if( (keycodePointer[3] == 0) && checkKeyword("NUMLOCK") )     keycodes = (keycodes << 8) | KP_NUMLOCK;
	  else if( (keycodePointer[3] == 0) && checkKeyword("PAGEUP") )      keycodes = (keycodes << 8) | KB_PAGEUP;
	  else if( (keycodePointer[3] == 0) && checkKeyword("PAGEDOWN") )    keycodes = (keycodes << 8) | KB_PAGEDOWN;
	  else if( (keycodePointer[3] == 0) && checkKeyword("PRINTSCREEN") ) keycodes = (keycodes << 8) | KB_PRINTSCREEN;
	  else if( (keycodePointer[3] == 0) && checkKeyword("SCROLLLOCK") )  keycodes = (keycodes << 8) | KB_SCROLLLOCK;	  
	  else if( (keycodePointer[3] == 0) && checkKeyword("SPACEBAR") )    keycodes = (keycodes << 8) | KB_SPACEBAR;
	  else if( (keycodePointer[3] == 0) && checkKeyword("SPACE") )       keycodes = (keycodes << 8) | KB_SPACEBAR;
	  else if( (keycodePointer[3] == 0) && checkKeyword("TAB") )         keycodes = (keycodes << 8) | KB_TAB;
	  else if( (keycodePointer[3] == 0) && checkKeyword("F12") )         keycodes = (keycodes << 8) | KB_F12;      
	  else if( (keycodePointer[3] == 0) && checkKeyword("F11") )         keycodes = (keycodes << 8) | KB_F11;
	  else if( (keycodePointer[3] == 0) && checkKeyword("F10") )         keycodes = (keycodes << 8) | KB_F10;
	  else if( (keycodePointer[3] == 0) && checkKeyword("F9") )          keycodes = (keycodes << 8) | KB_F9;
	  else if( (keycodePointer[3] == 0) && checkKeyword("F8") )          keycodes = (keycodes << 8) | KB_F8;
	  else if( (keycodePointer[3] == 0) && checkKeyword("F7") )          keycodes = (keycodes << 8) | KB_F7;
	  else if( (keycodePointer[3] == 0) && checkKeyword("F6") )          keycodes = (keycodes << 8) | KB_F6;     
	  else if( (keycodePointer[3] == 0) && checkKeyword("F5") )          keycodes = (keycodes << 8) | KB_F5;
	  else if( (keycodePointer[3] == 0) && checkKeyword("F4") )          keycodes = (keycodes << 8) | KB_F4;
	  else if( (keycodePointer[3] == 0) && checkKeyword("F3") )          keycodes = (keycodes << 8) | KB_F3;	       
	  else if( (keycodePointer[3] == 0) && checkKeyword("F2") )          keycodes = (keycodes << 8) | KB_F2;	       
	  else if( (keycodePointer[3] == 0) && checkKeyword("F1") )          keycodes = (keycodes << 8) | KB_F1;
	  
	  else if( keycodePointer[3] == 0)//if keyword is not recognized, but PayloadPointer is at some ASCII printable character
	    {
	      keycodes = (keycodes << 8) | (Keymap[ *(PayloadInfo.PayloadPointer) - 32 ] & 0x7F);//map ascii symbol to HID keycode, append this keycode to the list of similtaneous keycodes
	      if( Keymap[  *PayloadInfo.PayloadPointer - 32 ] & (1<<7) ) modifiers = modifiers | MOD_LSHIFT;//if most significant bit in Keymap[] is set, add LSHIFT modifier
	      if( Keymap[ (*PayloadInfo.PayloadPointer - 32) / 8 + 95 ] & (1 << (*PayloadInfo.PayloadPointer % 8)) ) modifiers = modifiers | MOD_RALT;//if corresponding AltGr bit is set, add RALT modifier
	      
	      //go to next character in the ducky script
	      if( PayloadInfo.PayloadPointer < ((char*) &PayloadBuffer + 2047) ) PayloadInfo.PayloadPointer++;
	      else PayloadInfo.PayloadPointer = (char*) &PayloadBuffer;
	    }
	  
	  //remove any repeating keycodes from new report data
	       if(keycodePointer[0] == keycodePointer[1]) keycodes = keycodes >> 8;
	  else if(keycodePointer[0] == keycodePointer[2]) keycodes = keycodes >> 8;
	  else if(keycodePointer[0] == keycodePointer[3]) keycodes = keycodes >> 8;
	}
    }
  
  //send specified key combo and mouse activity
  sendKBreport(modifiers, keycodes);
  if(PayloadInfo.PayloadFlags & (1<<3)) sendMSreport(mousedata);
  
  if(PayloadInfo.PayloadFlags & (1<<2))//if HoldFlag was set
    { 
      PayloadInfo.HoldModifiers = modifiers;//hold the specified modifiers for next commands
      PayloadInfo.HoldKeycodes = keycodes;//hold the specified keycodes for next commands
      PayloadInfo.HoldMousedata = mousedata & 0x000000FF;//only hold mouse buttons for next commands
      PayloadInfo.PayloadFlags &= ~(1<<2);//clear HoldFlag
    }

  //release specified key combo and mouse activity
  sendKBreport(PayloadInfo.HoldModifiers, PayloadInfo.HoldKeycodes);
  if(PayloadInfo.PayloadFlags & (1<<3)) sendMSreport(PayloadInfo.HoldMousedata);
  
  if(  (PayloadInfo.DeviceFlags & (1<<4)) && (PayloadInfo.PayloadFlags & (1<<4)) ) delay_ms(PayloadInfo.DefaultDelay);//if ONACTION_DELAY is used, only wait for default time if necessary
  if( !(PayloadInfo.DeviceFlags & (1<<4)) )                                        delay_ms(PayloadInfo.DefaultDelay);//if DEFAULT_DELAY  is used, always wait for default time

  PayloadInfo.PayloadFlags &= ~(1<<4);//clear ActionFlag
  PayloadInfo.PayloadFlags &= ~(1<<3);//clear MouseFlag
  
  //unless the start of a repeat block was explicitly specified, set current command as a RepeatStart
  if( !(PayloadInfo.PayloadFlags & (1<<1)) ) PayloadInfo.RepeatStart = openedFileInfo.fptr + 1 - PayloadInfo.BytesLeft;
  
  //go to next line in the ducky script
  if( PayloadInfo.PayloadPointer < ((char*) &PayloadBuffer + 2047) ) PayloadInfo.PayloadPointer++;
  else PayloadInfo.PayloadPointer = (char*) &PayloadBuffer;
  
  //calculate the size of current command, subtract it from BytesLeft
  if( (unsigned int) PayloadInfo.PayloadPointer >= commandStart) bytesUsed = (unsigned int) PayloadInfo.PayloadPointer - commandStart;
  else                                                           bytesUsed = (unsigned int) PayloadInfo.PayloadPointer - commandStart + 2048;  
  PayloadInfo.BytesLeft = PayloadInfo.BytesLeft - bytesUsed;
  
  return;
}

static void repeatDuckyCommands(unsigned int count)
{
  //remember where in the file current REPEAT command is (first byte outside of a repeat block)
  unsigned int repeatEnd = openedFileInfo.fptr + 1 - PayloadInfo.BytesLeft;
  
  //if REPEAT is the very first command in the payload file, skip to the next line and do nothing else
  if(repeatEnd == 0) return;
  
  NVIC_DisableIRQ(31);//disable usb interrupt, so f_read() and MSD access do not collide
  if(PayloadInfo.RepeatCount)//if a repeat block still needs to be repeated
    { 
      if(PayloadInfo.RepeatCount == 0xFFFFFFFF) PayloadInfo.RepeatCount = count;
      f_lseek(&openedFileInfo, PayloadInfo.RepeatStart);//move file read/write pointer to the previous REPEAT_START command
      PayloadInfo.RepeatCount--;//repeat block was repeated one more time
    }
  else//if a repeat block no longer needs to be repeated
    {
      f_lseek(&openedFileInfo, repeatEnd);//move file read/write pointer to the current REPEAT command      
      PayloadInfo.RepeatCount = 0xFFFFFFFF;//next REPEAT command will be able to set PayloadInfo.RepeatCount value
      PayloadInfo.RepeatStart = repeatEnd;//prevent previous repeat block from executing again
      PayloadInfo.PayloadFlags &= ~(1<<1);//clear the RepeatFlag, since current repeat block is over	  
    }
  
  //load the data from repeat block into the beginning of PayloadBuffer
  f_read(&openedFileInfo, (char*) &PayloadBuffer, 2048, &BytesRead);
  PayloadInfo.PayloadPointer = (char*) &PayloadBuffer;//move PayloadPointer back to start
  PayloadInfo.BytesLeft = BytesRead + 1;//one extra newline character will be appended to the end of payload script
  if(BytesRead < 2048) PayloadBuffer[BytesRead] = 0x0A;//if end of file is detected, add an extra newline
  PayloadInfo.PayloadFlags &= ~(1<<0);//commands are now being executed from first 1024 bytes of PayloadBuffer
  NVIC_EnableIRQ(31);//enable usb interrupt
  
       if( checkKeyword("REPEAT_START") ) skipString();
  else if( checkKeyword("REPEAT_SIZE") )  skipString();
  else if( checkKeyword("REPEAT") )       skipString();
  
  return;
}

//return 1 and move PayloadPointer to the next unread symbol if a referenceString is found at PayloadPointer
static char checkKeyword(char* referenceString)
{
  char* whereToCheck = PayloadInfo.PayloadPointer;

  //keep comparing characters until the end of reference string
  while(*referenceString)
    {
      //if a mismatch is found, stop the function and return 0
      if( *whereToCheck != *referenceString ) return 0;  
      referenceString++;//move to next character in referenceString
      
      //go to next character in a string. if the end of PayloadBuffer is reached, move pointer back to start of buffer
      if( whereToCheck < ((char*) &PayloadBuffer + 2047) ) whereToCheck++;
      else whereToCheck = (char*) &PayloadBuffer;
    }
  
  //if requested keyword was actually found, move PayloadPointer to next keyword
  PayloadInfo.PayloadPointer = whereToCheck;
  return 1;
}

//convert decimal string at PayloadPointer to unsigned integer, move PayloadPointer to the next unread symbol
static unsigned int checkDecValue()
{
  unsigned int result = 0;
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
      if( PayloadInfo.PayloadPointer < ((char*) &PayloadBuffer + 2047) ) PayloadInfo.PayloadPointer++;
      else PayloadInfo.PayloadPointer = (char*) &PayloadBuffer;
    }
  
  return result;
}

//convert hexadecimal string at PayloadPointer to unsigned integer, move PayloadPointer to the next unread symbol
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
      if( PayloadInfo.PayloadPointer < ((char*) &PayloadBuffer + 2047) ) PayloadInfo.PayloadPointer++;
      else PayloadInfo.PayloadPointer = (char*) &PayloadBuffer;
    }
  
  return result;
}

//sets PayloadInfo.Filename[] to newName;
static void setFilename(char* newName)
{
  unsigned char i;//used in a for() loop
  
  for(i=0; i<13; i++) PayloadInfo.Filename[i] = 0x00;//pre-fill target filename with zeroes
  
  for(i=0; i<12; i++)
    { 
      //allow only alphanumeric characters, underscores and dot symbols in the filename
           if(   *(newName) == 46 )                           PayloadInfo.Filename[i] = *(newName);
      else if(   *(newName) == 95 )                           PayloadInfo.Filename[i] = *(newName);
      else if( ( *(newName) >  47 ) && ( *(newName) <  58 ) ) PayloadInfo.Filename[i] = *(newName);
      else if( ( *(newName) >  64 ) && ( *(newName) <  91 ) ) PayloadInfo.Filename[i] = *(newName);
      else if( ( *(newName) >  96 ) && ( *(newName) < 123 ) ) PayloadInfo.Filename[i] = *(newName);
      else break;
      
      newName++;//move to the next character
    }
  
  return;
}

//set StringDescriptor_1 (serial number) to a new value
static void setSerialNumber(char* newSerialNumber)
{
  unsigned char i;//used in a for() loop
  
  for(i=0; i<12; i++) StringDescriptor_1[i+1] = '0';//set serial to string with 12 ASCII zero symbols
  
  for(i=0; i<12; i++)
    { 
      //allow only digits 0-9 and capital A-F letters in the serial number
           if( ( *(newSerialNumber) >  47 ) && ( *(newSerialNumber) <   58 ) ) StringDescriptor_1[i+1] = *(newSerialNumber);
      else if( ( *(newSerialNumber) >  64 ) && ( *(newSerialNumber) <   71 ) ) StringDescriptor_1[i+1] = *(newSerialNumber);
      else break;//if an unsupported character was encountered, stop reading the new serial number
      
      newSerialNumber++;//move to the next character
    }
  
  return;
}

//prints out a string of ASCII printable characters, other symbols act as string terminators
//if PayloadPointer was used as argument ( as opposed to sendString("literal string"); ), use data from PayloadBuffer
static void sendString(char* stringStart)
{
  unsigned int keycodes;//HID keycodes to send in a next report
  unsigned char* keycodePointer = (unsigned char*) &keycodes;//used to select a paricular keycode in a report
  unsigned short modifiers;//modifier byte to send in a next report
  unsigned short limit = 1000;//maximum number of symbols by which PayloadPointer is allowed to move
  
  if(PayloadInfo.HoldKeycodes >> 24) return;//if there is not enough space for one more keycode, do nothing and return  
  
  while( ( *stringStart > 31 ) && ( *stringStart < 127 ) )//continue until first unsupported character is encountered
    {
      if(limit) limit--;//if limit of symbols is reached, release all buttons and freeze ducky interpreter
      else while(1) sendKBreport(MOD_NONE, KB_Reserved);
      
      keycodes  = PayloadInfo.HoldKeycodes;//HID keycodes to send in a next report
      modifiers = PayloadInfo.HoldModifiers;//modifier byte to send in a next report
      
      keycodes = (keycodes << 8) | (Keymap[*stringStart - 32] & 0x7F);//map ASCII symbol into HID keycode
      if( Keymap[*stringStart - 32] & (1<<7) ) modifiers = modifiers | MOD_LSHIFT;//if most significant bit in the Keymap[] is set, add LSHIFT modifier
      if( Keymap[ (*stringStart - 32) / 8 + 95 ] & (1 << (*stringStart % 8)) ) modifiers = modifiers | MOD_RALT;//if corresponding AltGr bit is set, add RALT modifier
      
      //remove any repeating keycodes from new report data
           if(keycodePointer[0] == keycodePointer[1]) keycodes = keycodes >> 8;
      else if(keycodePointer[0] == keycodePointer[2]) keycodes = keycodes >> 8;
      else if(keycodePointer[0] == keycodePointer[3]) keycodes = keycodes >> 8;
            
      sendKBreport(modifiers, keycodes);//send correct keycodes and modifiers for the current symbol
      delay_ms(PayloadInfo.StringDelay);//if necessary, wait for STRING_DELAY
      sendKBreport(PayloadInfo.HoldModifiers, PayloadInfo.HoldKeycodes);//release current symbol keys
      delay_ms(PayloadInfo.StringDelay);//if necessary, wait for STRING_DELAY
      
      //if argument to this function was PayloadInfo.PayloadPointer, move it to the next symbol
      if(stringStart == PayloadInfo.PayloadPointer)
	{
	  //go to next character in a string. if the end of PayloadBuffer is reached, move pointer back to start of buffer
	  if( PayloadInfo.PayloadPointer < ((char*) &PayloadBuffer + 2047) ) PayloadInfo.PayloadPointer++;
	  else PayloadInfo.PayloadPointer = (char*) &PayloadBuffer;
	  stringStart = PayloadInfo.PayloadPointer;
	}
      else stringStart++;//if argument was a literal string, go to the next symbol
    }
  
  return;
}

//send specified keyboard report to host machine
static void sendKBreport(unsigned short modifiers, unsigned int keycodes)
{
  if(ControlInfo.HIDprotocol)//if report protocol is currently selected
    { 
      //write keyboard report data into a new report
      *( (unsigned short*) (BTABLE_BaseAddr + BTABLE->ADDR1_TX +  0) ) = (modifiers << 8) | 0x01;
      *( (unsigned short*) (BTABLE_BaseAddr + BTABLE->ADDR1_TX +  2) ) = keycodes <<  8;
      *( (unsigned short*) (BTABLE_BaseAddr + BTABLE->ADDR1_TX +  4) ) = keycodes >>  8;
      *( (unsigned short*) (BTABLE_BaseAddr + BTABLE->ADDR1_TX +  6) ) = keycodes >> 24;
      *( (unsigned short*) (BTABLE_BaseAddr + BTABLE->ADDR1_TX +  8) ) = 0;
      
      BTABLE->COUNT1_TX = 9;//data payload size for next IN transaction = 9 bytes
      USB->EP1R = (1<<15)|(1<<10)|(1<<9)|(1<<7)|(1<<4)|(1<<0);//respond with data to next IN packet
      while( (USB->EP1R & 0x0030) == 0x0030 );//wait until STAT_TX has changed from VALID to NAK (means host has received the report)            
    }
  
  else//if boot protocol is currently selected
    {
      //write keyboard report data into a new report
      *( (unsigned short*) (BTABLE_BaseAddr + BTABLE->ADDR1_TX +  0) ) = modifiers;
      *( (unsigned short*) (BTABLE_BaseAddr + BTABLE->ADDR1_TX +  2) ) = keycodes >>  0;
      *( (unsigned short*) (BTABLE_BaseAddr + BTABLE->ADDR1_TX +  4) ) = keycodes >> 16;
      *( (unsigned short*) (BTABLE_BaseAddr + BTABLE->ADDR1_TX +  6) ) = 0;
      
      BTABLE->COUNT1_TX = 8;//data payload size for next IN transaction = 9 bytes
      USB->EP1R = (1<<15)|(1<<10)|(1<<9)|(1<<7)|(1<<4)|(1<<0);//respond with data to next IN packet
      while( (USB->EP1R & 0x0030) == 0x0030 );//wait until STAT_TX has changed from VALID to NAK (means host has received the report)      
    }
  
  return;
}

//send specified mouse report to host machine
static void sendMSreport(unsigned int mousedata)
{
  //write mouse report data into a new report
  *( (unsigned short*) (BTABLE_BaseAddr + BTABLE->ADDR1_TX + 0) ) = (mousedata <<  8) | 0x02;
  *( (unsigned short*) (BTABLE_BaseAddr + BTABLE->ADDR1_TX + 2) ) = mousedata >>  8;
  *( (unsigned short*) (BTABLE_BaseAddr + BTABLE->ADDR1_TX + 4) ) = mousedata >> 24;
  
  BTABLE->COUNT1_TX = 5;//data payload size for next IN transaction = 5 bytes
  USB->EP1R = (1<<15)|(1<<10)|(1<<9)|(1<<7)|(1<<4)|(1<<0);//respond with data to next IN packet
  while( (USB->EP1R & 0x0030) == 0x0030 );//wait until STAT_TX has changed from VALID to NAK (means host has received the report)
  
  return;
}

//move PayloadPointer to the first non-spacebar character
static void skipSpaces()
{
  unsigned short limit = 10;//maximum number of symbols by which PayloadPointer is allowed to move
  
  while( *(PayloadInfo.PayloadPointer) == 32 )
    {
      //if limit of symbols is reached, skip the entire line
      if(limit == 0) skipString();
      else limit--;
      
      //go to next character in a string. if the end of PayloadBuffer is reached, move pointer back to start of buffer
      if( PayloadInfo.PayloadPointer < ((char*) &PayloadBuffer + 2047) ) PayloadInfo.PayloadPointer++;
      else PayloadInfo.PayloadPointer = (char*) &PayloadBuffer;
    }
  
  return;
}

//move PayloadPointer to the next newline
static void skipString()
{
  unsigned short limit = 1000;//maximum number of symbols by which PayloadPointer is allowed to move
  
  while( *(PayloadInfo.PayloadPointer) != 0x0A )
    {
      if(limit) limit--;//if limit of symbols is reached, release all buttons and freeze ducky interpreter
      else while(1) sendKBreport(MOD_NONE, KB_Reserved);
      
      //go to next character in a string. if the end of PayloadBuffer is reached, move pointer back to start of buffer
      if( PayloadInfo.PayloadPointer < ((char*) &PayloadBuffer + 2047) ) PayloadInfo.PayloadPointer++;
      else PayloadInfo.PayloadPointer = (char*) &PayloadBuffer;
    }
  
  return;
}

//----------------------------------------------------------------------------------------------------------------------

static inline void delay_us(unsigned int delay)
{
  if(delay)//if delay is nonzero
    {
      TIM2->CR1 = (1<<7)|(1<<3)|(1<<2);//disable TIM2 (in case it was running)
      TIM2->ARR = delay * 48 - 10;//TIM2 reload value
      TIM2->PSC = 0;//TIM2 prescaler = 1
      TIM2->EGR = (1<<0);//generate update event
      TIM2->SR = 0;//clear overflow flag
      TIM2->CR1 = (1<<7)|(1<<3)|(1<<2)|(1<<0);//auto-reload enable, one pulse mode, start upcounting
      while(TIM2->CR1 & (1<<0));//wait until timer has finished counting
    }
  
  return;
}

static void delay_ms(unsigned int delay)
{
  if(delay)//if delay is nonzero
    {      
      TIM2->CR1 = (1<<7)|(1<<3)|(1<<2);//disable TIM2 (in case it was running)
      TIM2->ARR = delay * 48;//TIM2 reload value
      TIM2->PSC = 999;//TIM2 prescaler = 1000
      TIM2->EGR = (1<<0);//generate update event
      TIM2->SR = 0;//clear overflow flag
      TIM2->CR1 = (1<<7)|(1<<3)|(1<<2)|(1<<0);//auto-reload enable, one pulse mode, start upcounting
      while(TIM2->CR1 & (1<<0));//wait until timer has finished counting
    }
  
  return;
}

//start the TIM6 timer, run for specified amount of milliseconds (max argument = 1365)
static void restart_tim6(unsigned short time)
{
  //TIM6 configuration: ARR is buffered, one pulse mode, only overflow generates interrupt, start upcounting
  TIM6->CR1 = (1<<7)|(1<<3)|(1<<2);//disable TIM6 (in case it was running)
  TIM6->ARR = time * 48 - 1;//run for specified amount of milliseconds
  TIM6->PSC = 999;//TIM6 prescaler = 1000
  TIM6->EGR = (1<<0);//generate update event
  TIM6->SR = 0;//clear overflow flag
  TIM6->CR1 = (1<<7)|(1<<3)|(1<<2)|(1<<0);//enable TIM6
  
  return;
}

static void enter_bootloader()
{
  //set function pointer to a value specified in the vector table of System Memory
  void (*bootloader)(void) = (void (*)(void)) ( *((unsigned int*) (0x1FFFC804U)) );
  
  //disable all interrupts in the NVIC
  NVIC_DisableIRQ(31);
  NVIC_DisableIRQ(18);
  NVIC_DisableIRQ(10);

  //clear all pending interrupts
  NVIC_ClearPendingIRQ(31);
  NVIC_ClearPendingIRQ(18);
  NVIC_ClearPendingIRQ(10);
  
  //return GPIO pins to default configuration
  GPIOB->ODR   = 0x00000000;
  GPIOB->MODER = 0x00000000;
  GPIOA->ODR   = 0x00000000;
  GPIOA->MODER = 0x28000000;
  GPIOA->PUPDR = 0x24000000;

  //disable all peripheral clocks
  RCC->AHBENR  = 0x00000014;
  RCC->APB1ENR = 0x00000000;
  RCC->APB2ENR = 0x00000000;
  
  RCC->CR |= (1<<0);//enable HSI clock
  while( !(RCC->CR & (1<<1)) );//wait until HSI is ready
  RCC->CFGR = 0;//set HSI as system clock
  while( !((RCC->CFGR & 0x0F) == 0b0000) );//wait until HSI is used as system clock  
  RCC->CR = 0x0083;//disable PLL, HSE clocks; disable CSS

  __DSB();//make sure all outstanding memory transfers are over before changing MSP value
  __set_MSP(0x20003FFC);//move main stack pointer back to the top
  __ISB();//make sure the effect of changing MSP value is visible immediately
  bootloader();//jump to bootloader code
  
  NVIC_SystemReset();//reset the system if CPU ever returns
  return;
}

//----------------------------------------------------------------------------------------------------------------------

static void saveOSfingerprint()  
{
  unsigned char* currentFingerprint = (unsigned char*) &ControlInfo.OSfingerprintData;
  unsigned char i;//used in a for() loop
  
  //if current.fgp already exists
  if( !f_open(&openedFileInfo, "0:/fingerdb/current.fgp", FA_READ | FA_OPEN_EXISTING) )
    {
      //read current.fgp data and store it into PayloadBuffer
      f_read(&openedFileInfo, (char*) &PayloadBuffer, 40, &BytesRead );
      f_close( &openedFileInfo );
      
      //check if data in current.fgp matches OS fingerprint collected just now
      for(i=0; i<40; i++)
	{
	  if( (i % 4) == 2 ) continue;//for every 4-byte fingerprint member check bytes 0, 1, 3; ignore byte 2
	  if( currentFingerprint[i] != PayloadBuffer[i] ) break;//if a mismatch is found, stop comparing the file
	}
      
      if(i<40)//if current.fgp exists, but does not match collected fingerprint
	{
	  //save contents of current.fgp into previous.fgp, replacing it if necessary
	  if( !f_open(&openedFileInfo,  "0:/fingerdb/previous.fgp", FA_WRITE | FA_CREATE_ALWAYS) )
	    {
	      f_write( &openedFileInfo, (unsigned char*) &PayloadBuffer, 40, &BytesRead );
	      f_close( &openedFileInfo );
	    }
	}
    }
  
  //save newly collected fingerprint in /fingerdb/current.fgp file, replacing it if necessaary
  if( !f_open(&openedFileInfo,  "0:/fingerdb/current.fgp", FA_WRITE | FA_CREATE_ALWAYS) )
    {
      f_write( &openedFileInfo, (unsigned char*) &ControlInfo.OSfingerprintData, 40, &BytesRead );
      f_close( &openedFileInfo );
    }
  
  return;
}

//compare current OS fingerprint with the database, set PayloadInfo.Filename[] to the appropriate OS specific payload filename
static void checkOSfingerprint()
{
  unsigned char* currentFingerprint = (unsigned char*) &ControlInfo.OSfingerprintData;
  unsigned char i;//used in a for() loop
  
  DIR     fingerdbDirInfo;   //holds information about /fingerdb/ directory
  FILINFO fingerdbFileInfo;  //holds information about files or directories inside of /fingerdb/
  DIR     OSspecificDirInfo; //holds information about /fingerdb/OSname/ directory
  FILINFO OSspecificFileInfo;//holds information about files or directories inside of /fingerdb/OSname/
  
  for(i=0; i<13; i++) PayloadInfo.Filename[i] = 0x00;//pre-fill target filename with zeroes
  
  //open and read /fingerdb/ directory to search for OS specific subdirectories
  FATFSresult  = f_opendir(&fingerdbDirInfo, "0:/fingerdb");
  FATFSresult |= f_readdir(&fingerdbDirInfo, &fingerdbFileInfo);  
  
  //keep going until no new files/directories are found inside /fingerdb/
  while( !FATFSresult && fingerdbFileInfo.fname[0] )
    {
      if(fingerdbFileInfo.fattrib & AM_DIR)//if a directory is found inside /fingerdb/
	{
	  FATFSresult  = f_chdir("0:/fingerdb");//set current directory to /fingerdb/
	  FATFSresult |= f_findfirst(&OSspecificDirInfo, &OSspecificFileInfo, fingerdbFileInfo.fname, "*.fgp");
	  FATFSresult |= f_chdir(fingerdbFileInfo.fname);//set current directory to /fingerdb/OSname/
	  
	  while( !FATFSresult && OSspecificFileInfo.fname[0] )
	    {
	      //read the contents of *.fgp file
	      f_open( &openedFileInfo, OSspecificFileInfo.fname, FA_READ | FA_OPEN_EXISTING);
	      f_read( &openedFileInfo, (char*) &PayloadBuffer, 40, &BytesRead );
	      f_close(&openedFileInfo);
	      
	      //compare current OS fingerprint with the data from *.fgp file
	      for(i=0; i<40; i++)
		{
		  if( (i % 4) == 2 ) continue;//for every 4-byte fingerprint member check bytes 0, 1, 3; ignore byte 2
		  if( currentFingerprint[i] != PayloadBuffer[i] ) break;//if a mismatch is found, stop comparing the file
		}
	      
	      if(i == 40)//if the *.fgp file matches current OS fingerprint
		{
		  for(i=0; i<13; i++)
		    {
		      //set OS specific payload filename to the name of directory
		      PayloadInfo.Filename[i] = fingerdbFileInfo.fname[i];
		      
		      //transform directory name into script filename by appending .txt extention at the end
		      if( (PayloadInfo.Filename[i] == 0x00) && (i < 9) )
			{
			  PayloadInfo.Filename[i + 0] = '.';
			  PayloadInfo.Filename[i + 1] = 't';
			  PayloadInfo.Filename[i + 2] = 'x';
			  PayloadInfo.Filename[i + 3] = 't';
			  PayloadInfo.Filename[i + 4] = 0x00;
			  break;
			}
		    }
		}
	      
	      //if correct OS specific payload is already found, stop searching
	      if( PayloadInfo.Filename[0] ) break;
	      else FATFSresult = f_findnext(&OSspecificDirInfo, &OSspecificFileInfo);
	    }
	  
	  f_closedir(&OSspecificDirInfo);
	}
      
      //if correct OS specific payload is already found, stop searching
      if( PayloadInfo.Filename[0] ) break;
      else FATFSresult = f_readdir(&fingerdbDirInfo, &fingerdbFileInfo);//move to the new file
    }

  //if correct OS fingerprint was not found anywhere in database, set payload filename to other.txt
  if( PayloadInfo.Filename[0] == 0 ) setFilename("other.txt");
  
  return;
}
