If "USE_FINGERPRINTER" pre-configuration command is present, device ignores  
payload.txt and instead chooses a payload file based on target machine OS.  
These OS specific scripts must be placed in /fgscript/ directory. Available  
script choises are: /fgscript/linux.txt, /fgscript/windows.txt,  
/fgscript/mac.txt or /fgscript/other.txt (if OS not detected)  
  
First, the device will create /fingerdb/current.fgp file, which contains OS  
fingerprint of the machine it is currently plugged into, and then it will  
search directories /fingerdb/linux/, /fingerdb/windows/, /fingerdb/mac/  
for a matching fingerprint file. The right script is chosen based on what  
directory the matching file was found in, or set to /fgscript/other.txt if  
no matching fingerprint was found at all.  
  
---
  
The OS detection mechanism is based on the fact that different  
USB host drivers react differently to insertion of PocketAdmin.  
For every device the host machine sends a sequence of control requests.  
  
For example, a windows machine might send this:  
* GET DEVICE DESCRIPTOR  
* SET ADDRESS  
* GET DEVICE DESCRIPTOR  
* GET CONFIGURATION DESCRIPTOR  
* GET STRING DESCRIPTOR  
* GET DEVICE QUALIFIER  
* GET DEVICE DESCRIPTOR  
* GET CONFIGURATION DESCRIPTOR  
* GET CONFIGURATION DESCRIPTOR  
* GET STRING DESCRIPTOR  
  
While a linux machine might send:  
* GET DEVICE DESCRIPTOR  
* SET ADDRESS  
* GET DEVICE DESCRIPTOR  
* GET DEVICE QUALIFIER  
* GET DEVICE QUALIFIER  
* GET DEVICE QUALIFIER  
* GET CONFIGURATION DESCRIPTOR  
* GET CONFIGURATION DESCRIPTOR  
* GET STRING DESCRIPTOR  
* GET STRING DESCRIPTOR  
  
These sequences are persistent between different times you insert the  
device in, as well as different physical machines that a particular OS  
is installed on. This allows to build up a database and store it on  
the device. Once device is plugged in it can compare incoming sequence  
of control requests with a presaved pattern and guess which USB driver  
is used by the host machine, which in turn identifies the OS.  
  
These patterns are saved in fingerprint files, which must have .fgp extention  
and their name must not be longer than 8 characters (without .fgp);  
Inside these files is data from 10 of the very first  
control requests sent by the host, namely  
bmRequestType, bRequest and wValue fields. The byte  
order is the same as in actual USB control request.  
  
Keep in mind that fingerprints can be different for different versions  
of some particular OS, and can change with time. For example, if your  
windows 10 machine had an update which modified it's USB driver,  
the fingerprint will probably change too, so a new .fgp file has to  
be added into the database.  
  
If you have enabled the OS fingerprinter the device will create a  
directory /fingerdb/ and store the fingerprint of the machine it is  
plugged into in /fingerdb/current.fgp; To add this file in the  
database copy it to appropriate /fingerdb/osname/ directory, available  
choises are: /fingerdb/linux/, /fingerdb/windows/, /fingerdb/mac/;  
From that point on your OS should be correctly detected.  
  
There is an OS fingerprint database included in this  
repository ( in /extra/fingerdb/ ); If you have collected  
more fingerprints, I welcome you to share them, so they can  
be included here for everybody else's use.  
