Keystroke reflection is a method for exfiltrating data out of a host  
machine using only HID keyboard interface, rather than relying on USB  
mass storage or network. It is done by listening to incoming LED toggle  
commands sent to the device and interpreted in a special way.  
Specifically, once device receives a request from the host to toggle  
NumLock state it is treated as receiving a bit of value 1, and  
receiving a request to toggle CapsLock state is treated as receiving  
a bit of value 0. Bits are written from most significant bit first to  
least significant bit last in every byte.  
  
"KEYREFLECT_START" command makes the device start listening and writing  
incoming bits. Device should then make the PC send the key sequence,  
which will take some time. This can be done by running powershell or  
bash commands inside host's CLI. To mark the end of data host must send  
ScrollLock LED toggle request. Once the keystroke reflection sequence  
is over, "KEYREFLECT_SAVE **n**" command will save this data to a file  
inside /keyref/ directory. Name of the file can be specified as an  
argument, but this is optional. If specified, it must be in 8.3 format  
and the contents of it will be overwritten every time payload runs.  
If not set explicitly, device will make a new file with previously  
unused numeric name, eg. /keyref/002.ref, /keyref/003.ref, etc.  
  
Maximum size of reflection data received in one sequence is 512 bytes.  
If "KEYREFLECT_SAVE **n**" command is run before key sequence is over  
it will wait until ScrollLock toggle request is received or a limit of  
512 bytes is reached, whichever comes first. Only then the device will  
procede to save the data into a file. Keep in mind that the process of  
data exfiltration using this method is usually very slow, so only use  
it if you really have to avoid using mass storage or other means (eg.  
air-gapped and USB storage restricted PC).  
Support for keystroke reflection on macOS is very limited, as on that  
OS keyboard LED state is usually not shared between different keyboards.  
  
There is an [example payload](https://github.com/krakrukra/PocketAdmin/tree/master/extra/payloads/FeatureTesting/KeystrokeReflectionTest) for testing keystroke reflection on windows.  
  