By default PocketAdmin shows up as a compound device with HID (keyboard) and MSD (flash drive) interfaces.  
For the most basic use, there must be a FAT filesystem on the first partition, and in the root directory  
there must be a payload.txt file, which contains the commands that the device should run at every insertion.  
Optionally, alongside the payload.txt you can have a config.txt file. If this file if present, the device  
first runs pre-configuration commands from there, and then moves on to running some payload file.  



Some example payloads can be found in /extra/examplePayloads/ but keep in mind that payloads  
need to be modified to fit your target machine (as well as your exact requirements),  
since they heavily depend on OS used, GUI settings, language, hardware capabilities, etc.  

Of course, there are plenty other scripts/executables/original ducky payloads available  
on the internet for you to use, if you are too lazy to make your own payloads. Remember,  
it can do anything that your keyboard can (even more than that, actiually)  
