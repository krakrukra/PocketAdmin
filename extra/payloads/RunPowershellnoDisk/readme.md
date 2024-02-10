This is a set of windows specific payloads, which will all work in  
HID-only mode. They are intended for use in situations where normal  
operation of built-in USB disk is not possible, eg due to security  
policies of whitelisting or complete blocking of USB storage devices.  
Those payloads will input a powershell script into the target machine  
some other way and then run it. It is up to the end user to replace  
default powershell scripts here with something more useful. If you  
want something to start off at, you can always search for scripts  
online. As an example, here is a collection of penetration testing  
scripts by Nikhil Mittal: [https://github.com/samratashok/nishang](https://github.com/samratashok/nishang)  
  
[HIDdownloadRun](https://github.com/krakrukra/PocketAdmin/tree/master/extra/payloads/RunPowershellnoDisk/HIDdownloadRun) downloads insert.ps1 file from a web server  
and runs it without requesting any administrative privileges, while  
hiding the powershell window as soon as possible for obfuscation.  
Default script is provided in payload directory, it creates a new  
file called "file.txt" on a desktop of a currently logged in user.  
In order to make this payload work, you will have to post insert.ps1  
script on a publicly available web server (eg github, dropbox, etc)  
and then write correct download link inside of your payload.txt  
  
[HIDrunlineScript](https://github.com/krakrukra/PocketAdmin/tree/master/extra/payloads/RunPowershellnoDisk/HIDrunlineScript) payload will type some powershell  
commands directly into the runline window as a script to execute.  
The size of a script like this must be small enough to fit there.  
Script runs without requesting administrative privileges, while  
hiding the powershell window as soon as possible for obfuscation.  
Default powershell script creates a new file called "file.txt"  
on a desktop of a currently logged in user.  
  
[HIDtypeAndRun](https://github.com/krakrukra/PocketAdmin/tree/master/extra/payloads/RunPowershellnoDisk/HIDtypeAndRun) payload will open a powershell window,  
pull it to the very bottom of the screen for obfuscation and  
then manually type a script and run it. You can use this payload  
in case your script is too long to fit in a runline window, and  
the internet connection is unavailable as well. Default script  
collects some information about target PC and saves it into  
"file.txt" on a desktop of a currently logged in user.  
  
  