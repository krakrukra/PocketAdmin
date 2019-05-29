Some example payloads can be found in /extra/examplePayloads/ but keep in mind  
that payloads usually need to be modified to fit your target machine (as well  
as your exact requirements), since they heavily depend on OS used, GUI settings,  
language, hardware capabilities, etc. If you are too lazy to make your own  
payloads, you can also use payloads originally designed for the USB rubber ducky,  
or some other badusb that accepts ducky script commands.  
  
Each example payload is placed in a dedicated directory, inside of which only  
corresponding script files and config.txt are included. If necessary, there  
also might be a readme.md file, explaining in more detail what the payload does.  
  
Always keep in mind, that in an actual application you MUST place  
all the right files in the right places! That means:  
  
1. If fingerprinter was not enabled (default), you need to place  
your script file (payload.txt) in the root directory.  
  
2. If fingerprinter was enabled, you need to place your scripts  
(windows.txt, linux.txt, mac.txt, other.txt ) in /fgscript/ directory,  
and you also need to provide a database of .fgp files in /fingerdb/  
directory. (you can copy it form /extra/ in this repository)  
  
3. If you have any pre-configuration commands, they must be  
in a config.txt file in the root directory.  
  
4. If you use different keyboard layouts, layout files must be  
placed in /kblayout/ directory. (available in /extra/ in this repo)  
  
  
So, let's say you wanted to check how OS fingerprinter would identify your  
system. There is an example payload for that, called "fingerprinterTest".  
To use it, you would copy /extra/fingerdb/, .../fingerprinterTest/config.txt  
and .../fingerprinterTest/fgscript/ into PocketAdmin's root directory.  
  
If you needed to use a different layout, you would to copy /extra/kblayout/ to  
PocketAdmin and added, a new command to config.txt, like this: "USE_LAYOUT fr_FR"  
