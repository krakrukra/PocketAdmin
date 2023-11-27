This is a set of windows specific payloads, which will all find the  
drive that has it's FAT volume label set to POCKETADMIN, and then go  
to this drive and start a powershell script from **insert.ps1** file.  
It is up to the end user to replace default powershell scripts here  
with something more useful. If you want something to start off at,  
you can always search for scripts online. As an example, here is a  
collection of penetration testing scripts by Nikhil Mittal: [https://github.com/samratashok/nishang](https://github.com/samratashok/nishang)  
  
Each subdirectory here contains a version of the payload that is  
slightly different from the rest. You should copy all the files  
from one particular subdirectory here to PocketAdmin's storage and  
set FAT volume label to POCKETADMIN before this payload can run.  
In order to change the FAT volume label on windows, you can just  
plug the device in, select the correct drive in the file manager  
(in "My Computer" -> "Devices and Drives") and press F2.  
Alternatively, in Linux you can use the fatlabel utility for that.  
  
[VisibleNonAdminScript](https://github.com/krakrukra/PocketAdmin/tree/master/extra/payloads/RunPowershellScript/VisibleNonAdminScript) payload runs the insert.ps1 script without  
requesting any administrative privileges, and also without any
attempt to hide the powershell window. By default it only  
outputs a string into a terminal. 
  
[HiddenNonAdminScript](https://github.com/krakrukra/PocketAdmin/tree/master/extra/payloads/RunPowershellScript/HiddenNonAdminScript) payload runs the insert.ps1 script without  
requesting any administrative privileges, and also hides  
the powershell window as soon as possible for obfuscation.  
By default it creates a new file  called "testfile.txt" on  
a desktop of a currently logged in user.  
  
[DirectUACbypass](https://github.com/krakrukra/PocketAdmin/tree/master/extra/payloads/RunPowershellScript/DirectUACbypass) payload is trying to launch insert.ps1 script  
with administrative privileges, and also hides the powershell  
window as soon as possible for obfuscation. After a delay for  
UAC confirmation window to appear, payload tries to say YES.  
The currently logged in user has to have admin rights on the  
system in order for this to work. Default script creates a new  
file located at C:\Windows\adminonly.txt with some text inside.  
  
[SilentUACbypass](https://github.com/krakrukra/PocketAdmin/tree/master/extra/payloads/RunPowershellScript/SilentUACbypass) payload launches insert.ps1 script without requesting  
for admin rights explicitly, and instead attempts to do a silent UAC  
bypass inside the script (avoids showing a UAC confirmation window to  
the user). It also hides the powershell window as soon as possible for  
obfuscation. The currently logged in user still has to have admin rights  
on the system in order for this to work. Default script performs the UAC  
bypass, and then transfers execution to elevated.ps1 script, which is  
where end user has to put the code to be executed with admin rights. By  
default elevated.ps1 creates a file located at C:\Windows\adminonly.txt  
with some text inside. This payload might not work on windows versions  
older than windows 10 and might become obsolete in the future. It uses  
powershell command obfuscation in order to avoid detection by antivirus.  
  
[GetInformationScript](https://github.com/krakrukra/PocketAdmin/tree/master/extra/payloads/RunPowershellScript/GetInformationScript) is a demonstration script described in more  
detail in [this video](https://www.youtube.com/watch?v=o4rd-4753e0), though some of the commands shown  
there were replaced with more modern versions. It is an extention  
of DirectUACbypass basic script, showing how you can make it fit  
your needs in one particular situation and using some more tricks  
and commands in the process. Payload will collect some data from  
target machine and save it as exfil.txt on PocketAdmin's storage.  
It will only work if the currently logged in user has admin  
privileges, but this is a rather common situation.  
  