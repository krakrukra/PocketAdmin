This is a windows specific payload, which will install several  
softwares on the target machine, to make screenshots of user's  
desktop, log keystrokes and then periodically send collected  
data back to a specified dropbox account. It is described in  
more detail in [this video](https://www.youtube.com/watch?v=pKTy7eIpTOI).  
**UPDATE:** due to dropbox changing their authentication method the  
procedure of configuring upload.ps1 will be somewhat different now   
  
After insertion, device will execute commands from **payload.txt**  
file. This will bring up windows runline dialog and use it to find  
the correct USB drive based on FAT volume label (POCKETADMIN)  
and start a powershell script contained in **/scripts/insert.ps1**  
This script will then copy all the spyware files into a directory  
**C:\ProgramData\WindowsUserAssist**, start these softwares right  
away and also place a vbs script from **/scripts/autostart.vbs** into  
the current user's startup folder to restart spyware on every login.  
  
Before you can use the payload you have to make an account with  
dropbox and create a new dropbox app. All of this can be done  
through their website. You will also have to go to permissions tab  
for your app and enable any files.metadata and files.content access.  
You **must** then modify upload.ps1 script to make it work correctly.  
  
#### screen capture software  
  
Screen capture software is stored in **/binaries/scap.exe**, while  
it's source code is available in **/scap-src/** directory. Source code  
is not required for the actual execution of the payload, so you do  
not have to copy it into the PocketAdmin's USB storage. This software  
was compiled on a windows machine using MinGW compiler toolchain.  
  
It will make a screenshot of current user's desktop at least once  
per minute, but not more often than once in 15 seconds. Actions  
that can trigger a new screenshot capture are: mouse left click,  
keyboard ENTER press or 1 minute timeout. The screenshots will be  
in JPG format and placed in the directory containing a timestamp  
in it's name, in this format: "YYYY_MM_DD_HH", where YYYY stands for  
the year, MM - month, DD - day of month, HH - hour in 24-hour format;  
Screenshots themselves have timestamps in their names too, in the  
following format: "HH-mm-ss.jpg", where HH is hour in 24-hour format,  
mm - minute, ss - second of when the screenshot was taken.  
  
#### keylogger software
  
Keylogger software is stored in **/binaries/klog.exe**, while it's  
source code is available in **/klog-src/** directory. Source code is  
not required for the actual execution of the payload, so you do  
not have to copy it into PocketAdmin's USB storage. This software  
was compiled on a windows machine using MinGW compiler toolchain.  
  
It will capture user keystrokes and log them into a file inside  
a directory containing a timestamp in it's name, in this format:  
"YYYY_MM_DD_HH", where YYYY stands for the year, MM - month,  
DD - day of month, HH - hour in 24-hour format. This is the  
same directory name as is used by screen capture software.  
  
Keylog files have timestamps in their names too, in the  
following format: HH-00.txt, where HH is hour in 24-hour format,  
00 is a literal string (eg. "22-00.txt" or "23-00.txt", etc)  
More precise timestamps, along with what window were the keystrokes  
typed into and what language was selected are placed inside the  
text file. The keystrokes are mostly logged when the key is **released**,  
so if you hold a key down it will register as only one character typed.  
Modifier keys, such as CTRL, SHIFT, ALT, GUI are tracked on both press  
and release events, with "/" character prefix in case the key was  
released. For example, you might see this: [LCTRL]a[/LCTRL] for LCTRL-a  
key combo, or this: [LALT][F4][/LALT] for LALT-F4 key combo.  
  
#### uploader script
  
To upload all the collected data from target machine back to you,  
there is a powershell script stored in **/scripts/upload.ps1**  
Once started, it runs continuously, similar to other softwares.  
When it detects a hour-of-the-day boundary (eg time goes from  
13:59 to 14:00), the folder that screencapture and keylogger  
software have been using is now written completely and can be  
sent back to a remote server. This folder will then be compressed  
into a zip archive and sent out to a specific dropbox app, depending  
on **app_key, app_secret, refresh_token** values of your application.  
Both the original directory and the zip archive will be removed from  
target machine's local storage after the data was sent to dropbox.  
  
You **must** replace APP_KEY_HERE, APP_SECRET_HERE, REFRESH_TOKEN_HERE  
strings with correct values inside your upload.ps1 file in order to  
receive anything. Values of app_key and app_secret are available via  
dropbox webpage dedicated to your app. To get refresh token you will  
have to run interactive powershell script called get_refresh_token.ps1  
and follow the instructions it provides.  
  
  
#### stop the softwares
  
klog.exe, scap.exe and the powershell script will all show up  
in the task manager and can be stopped from there. Keep in mind  
that when **insert.ps1** script installs the spyware on the  
target PC it will rename executables to some more innocuous names,  
such as "UserAssist Klg Host" or "UserAssist Scc Host"; the uploader  
script will be named "Windows PowerShell", just like any other  
powershell instance that is running.  
  