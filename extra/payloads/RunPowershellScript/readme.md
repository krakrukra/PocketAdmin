This is a set of windows specific payloads, which will all find the  
drive that has it's FAT volume label set to POCKETADMIN, and then go  
to this drive and start a powershell script from **insert.ps1** file.  
  
Each subdirectory here contains a version of the payload that is  
slightly different from the rest. You should copy all the files  
from one particular subdirectory here to PocketAdmin's storage and  
set FAT volume label to POCKETADMIN before this payload can run.  
  
In order to change the FAT volume label on windows, you can just  
plug the device in, select the correct drive in the file manager  
(in "My Computer" -> "Devices and Drives") and press F2.  
Alternatively, in Linux you can use the fatlabel utility for that.  
  
GenericUserScript payload runs an empty insert.ps1 script without  
requesting any administrative privileges. It is up to the end user  
to replace it with something more useful. If you want something  
to start off at, you can always search for scripts online. As an  
example, here is a collection of penetration testing scripts  
by Nikhil Mittal: [https://github.com/samratashok/nishang](https://github.com/samratashok/nishang)  
  
GetInformationScript payload requests for administrative privileges  
to run insert.ps1, which in turn will collect some data from target  
machine and save it as exfil.txt on PocketAdmin's storage. This  
payload is described in more detail in [this video](https://www.youtube.com/watch?v=o4rd-4753e0), but some of the  
commands shown there were replaced with more modern commands.  
This payload only works if the currently logged in user has admin  
privileges, but this is quite a common situation.  
  
SilentUACbypass payload launches insert.ps1* script without requesting  
for admin rights explicitly, and instead attempts to do a silent UAC  
bypass inside the script (avoids showing a UAC confirmation window to  
the user). The currently logged in user still has to have admin rights  
on the system in order for this to work. Default script creates a new  
file located at C:\Windows\adminonly.txt with some text inside. To  
replace default script you should not modify the whole insert.ps1 file,  
but put your script in between two comments "# user script start/end"  
This payload might not work on windows versions older than windows 10.  
  