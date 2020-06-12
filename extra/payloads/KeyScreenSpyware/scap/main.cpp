#include <stdio.h>
#include <windows.h>
#include <gdiplus.h>
#include <time.h>

using namespace Gdiplus;

void SaveScreenshot();
void SavePNG(HBITMAP hBmp, char *filename);
void SaveJPG(HBITMAP hBmp, char* filename, ULONG uQuality);
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);

void CALLBACK TimerProc(HWND hWnd, UINT msgCode, UINT_PTR IDevent, DWORD tickCount);
LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);


char TargetDirName[14];
char TargetFileName[27];
struct tm* TimePointer;
time_t LastEpochTime = 0;
unsigned int timerID;

int main()
{
  HHOOK KeyboardHook;
  HHOOK MouseHook;
  MSG msg;
  
  FreeConsole();
  timerID = SetTimer( 0, 0, 100, &TimerProc);
  KeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, &KeyboardHookProc, 0, 0);
  MouseHook = SetWindowsHookEx(WH_MOUSE_LL, &MouseHookProc, 0, 0);
  
  while(GetMessage(&msg, 0, 0, 0))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    
  
  UnhookWindowsHookEx(KeyboardHook);
  UnhookWindowsHookEx(MouseHook);
  return 0;
}

void SaveScreenshot()
{
  int Width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
  int Height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
  
  LastEpochTime = time(0);//retrieve and save the current epoch time
  TimePointer = localtime(&LastEpochTime);//extract structured time from epoch time
  //update target directory and file names based on current time
  sprintf( (char*) &TargetDirName, "%04u_%02u_%02u_%02u", TimePointer->tm_year + 1900, TimePointer->tm_mon + 1, TimePointer->tm_mday, TimePointer->tm_hour );
  sprintf( (char*) &TargetFileName, "%s\\%02u-%02u-%02u.jpg", (char*) &TargetDirName , TimePointer->tm_hour, TimePointer->tm_min, TimePointer->tm_sec );
  
  //copy screen to bitmap
  HDC ScreenDC = GetDC(0);
  HDC CompDC = CreateCompatibleDC(ScreenDC);
  HBITMAP CompBitmap =  CreateCompatibleBitmap(ScreenDC, Width, Height);
  HGDIOBJ old_obj = SelectObject(CompDC, CompBitmap);
  BitBlt(CompDC, 0, 0, Width, Height, ScreenDC, 0, 0, SRCCOPY);
  
  //add mouse cursor on the screenshot
  CURSORINFO cursor;
  cursor.cbSize = sizeof(cursor);
  GetCursorInfo(&cursor);
  if (cursor.flags == CURSOR_SHOWING) 
  {
    ICONINFO info;
    GetIconInfo(cursor.hCursor, &info);
    BITMAP bmpCursor;
    GetObject(info.hbmColor, sizeof(BITMAP), &bmpCursor);
    DrawIconEx(CompDC, cursor.ptScreenPos.x - info.xHotspot, cursor.ptScreenPos.y - info.yHotspot, cursor.hCursor, bmpCursor.bmWidth, bmpCursor.bmHeight, 0, 0, DI_NORMAL);
  }
  
  CreateDirectoryA( (char*) &TargetDirName, 0 );//make sure target directory exists
  SaveJPG(CompBitmap, (char*) &TargetFileName, 75);//save image data to JPG file in target directory
  
  DeleteDC(CompDC);
  DeleteDC(ScreenDC);
  SelectObject(CompDC, old_obj);
  
  return;
}

void SavePNG(HBITMAP hBmp, char* filename)
{
  ULONG_PTR gdiplusToken;
  GdiplusStartupInput gdiplusStartupInput;
  GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
  
  Bitmap* b = Bitmap::FromHBITMAP(hBmp, NULL);
  
  CLSID  encoderClsid;
  
  if (b && GetEncoderClsid(L"image/png", &encoderClsid) != -1)
  {
    int len = strlen(filename);
    WCHAR* wfilename = new WCHAR[len+1];
    MultiByteToWideChar(CP_ACP, 0, filename, len+1, wfilename, len+1);
    
    //save the buffer to a file
    b->Save(wfilename, &encoderClsid, NULL);
    delete [] wfilename;
  }
  if (b) delete b;
  
  GdiplusShutdown(gdiplusToken);
  return;
}

void SaveJPG(HBITMAP hBmp, char* filename, ULONG uQuality)
{
  ULONG_PTR gdiplusToken;
  GdiplusStartupInput gdiplusStartupInput;
  GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
  
  Bitmap* b = Bitmap::FromHBITMAP(hBmp, NULL);
  
  CLSID encoderClsid;
  Status stat = GenericError;
  
  if (b && GetEncoderClsid(L"image/jpeg", &encoderClsid) != -1)
  {
    int len = strlen(filename);
    WCHAR* wfilename = new WCHAR[len+1];
    MultiByteToWideChar(CP_ACP, 0, filename, len+1, wfilename, len+1);
    
    //save the buffer to a file    
    EncoderParameters encoderParams;
    encoderParams.Count = 1;
    encoderParams.Parameter[0].NumberOfValues = 1;
    encoderParams.Parameter[0].Guid  = EncoderQuality;
    encoderParams.Parameter[0].Type  = EncoderParameterValueTypeLong;
    encoderParams.Parameter[0].Value = &uQuality;
    GetEncoderClsid(L"image/jpeg", &encoderClsid);
    b->Save(wfilename, &encoderClsid, &encoderParams);
    delete [] wfilename;
  }
  if (b) delete b;
  
  GdiplusShutdown(gdiplusToken);
  return;
}

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
  UINT  num = 0;  // number of image encoders
  UINT  size = 0; // size of the image encoder array in bytes
  
  ImageCodecInfo* pImageCodecInfo = NULL;
  
  GetImageEncodersSize(&num, &size);
  if(size == 0)
  return -1;  // Failure
  
  pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
  if(pImageCodecInfo == NULL)
  return -1;  // Failure
  
  GetImageEncoders(num, size, pImageCodecInfo);
  
  for(UINT j = 0; j < num; ++j)
  {
    if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
    {
      *pClsid = pImageCodecInfo[j].Clsid;
      free(pImageCodecInfo);
      return j;  // Success
    }    
  }
  
  free(pImageCodecInfo);
  return -1;  // Failure
}

void TimerProc(HWND hWnd, UINT msgCode, UINT_PTR timerID, DWORD tickCount)
{  
  KillTimer(0, timerID);//destroy previous timer
  
  //if 1 minute has passed since last saved screensot, make a new one
  if( (time(0) - LastEpochTime) >= 60 ) SaveScreenshot();
  
  timerID = SetTimer( 0, 0, 100, &TimerProc);//restart the timer to 100ms
  return;
}

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
  MSLLHOOKSTRUCT* MouseEvent = (MSLLHOOKSTRUCT*) lParam;
  
  //if left mouse button was pressed and at least 15 seconds have passed since last saved screenshot
  if( (wParam == WM_LBUTTONDOWN) && ((time(0) - LastEpochTime) >= 15) ) SaveScreenshot();
  
  return CallNextHookEx(0, nCode, wParam, lParam);
}

LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) 
{ 
  KBDLLHOOKSTRUCT* KeyboardEvent = (KBDLLHOOKSTRUCT*) lParam;
  
  //if some key was released and at least 15 seconds have passed since last saved screenshot
  if( ((wParam == WM_KEYDOWN) || (wParam == WM_SYSKEYDOWN)) && ((time(0) - LastEpochTime) >= 15) )
  {
    //save screenshot if ENTER was pressed
    if(KeyboardEvent->vkCode == 0x0D) SaveScreenshot();
  }
  
  return CallNextHookEx(0, nCode, wParam, lParam);
}