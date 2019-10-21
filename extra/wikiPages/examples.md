Some example scripts can be found in /extra/examplePayloads/. You should keep in  
mind that payloads usually need to be modified to fit your particular application,  
since they heavily depend on target OS, GUI and language settings, hardware, etc.  
If you are too lazy to make your own payloads, you can always reuse payloads that  
were originally designed for the USB rubber ducky or some other badusb. Even if  
these original payloads are not in duckyscript format (eg. arduino based badusb's)  
you can convert them to duckyscript without too much effort.  
  
Each example payload is placed in a dedicated directory, inside of which only  
corresponding script files, config.txt and readme.md files are included. If  
necessary, you should also copy /extra/fingerdb/ or /extra/kblayout/ directories  
to the device's root directory. The readme.md file is there only to explain in  
more detail what the payload does and how it should be used.  
  
Always keep in mind, that in an actual application you MUST have  
all the right files in the right places! That means:  

1. If you have any pre-configuration commands, you need to place them  
separately in a config.txt file in the device's root directory.  

2. If fingerprinter was not enabled (default), you need to place  
your script file (payload.txt) in the device's root directory.  
  
3. If fingerprinter was enabled, you need to place your script files  
(windows.txt, linux.txt, mac.txt, other.txt) in /fgscript/ directory  
and also provide a database of .fgp files in /fingerdb/ directory.  
  
4. If you want to replace the default US keyboard layout with some other  
one, you need to place required layout files in /kblayout/ directory.  
  
So, let's say you want to check how the OS fingerprinter would identify  
your system. There is an example payload for that. To use it, you would  
go to /extra/examplePayloads/fingerprinterTest/, check out the information  
in readme.md file and then copy all the files inside to PocketAdmin's root  
directory. Since this payload does use the fingerprinter you will also  
need to copy /extra/fingerdb/ directory, otherwise you will have to build  
up your own database from scratch.  
But what if you have a French keyboard layout? You would then copy  
/extra/kblayout/ directory to the device as well, and then add this  
command (without quotes) to config.txt: "USE_LAYOUT fr_FR".  
The payload should now work as described in readme.md  
