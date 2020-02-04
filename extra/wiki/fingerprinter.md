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
is installed on. This allows you to build up a database and store it on  
the device. Once device is plugged in, it can compare incoming sequence  
of control requests with all the presaved patterns and guess which USB  
driver is used by the host machine, which in turn identifies the OS.  
  
These patterns are saved in fingerprint files and consist of the data  
from 10 of the very first control requests sent by the host, namely  
bmRequestType, bRequest and wValue fields. The byte order is the same  
as in actual USB control request. Only bytes 0, 1 and 3 are compared  
for every 4-byte member in the array.  
  
---
  
If "USE_FINGERPRINTER" pre-configuration command is present in config.txt,  
device ignores payload.txt and instead chooses a payload file based on  
target machine OS. These OS specific scripts must be placed in /fgscript/  
directory and their names must be in 8.3 format with .txt extention. The  
only allowed characters in the name are lowercase letters a-z, uppercase  
letters A-Z, digits 0-9, underscore and dot symbols. The names are not  
hardcoded though, so as long as the OS name is 8 characters or less you  
can use it, eg. linux.txt, windows.txt, mac.txt, someOS.txt, HP_bios.txt;  
  
The way one of these files will be selected is this. Once you insert the  
device it will collect the host machine's fingerprint and save it into a  
file called CURRENT.FGP inside /fingerdb/ directory; if CURRENT.FGP file  
already exists and does not match collected fingerprint, the previous data  
will be moved into PREVIOUS.FGP file (overwriting it if necessary). Device  
will then search through all the directories inside /fingerdb/ and if it  
finds a matching fingerprint file in one of these directories, it will use  
the directory name as the name of the OS. If a matching fingerprint file  
was not found anywhere, payload script named other.txt will be executed.  
  
So, if you have enabled the OS fingerprinter and you plug the device into  
some host machine, it will always create a new CURRENT.FGP file with data  
collected on that machine. If you didn't have any fingerprints in the  
database, device will try to run /fgscript/other.txt upon insertion. At  
this point you can add this new CURRENT.FGP file into your database by  
saving it into an OS specific directory. Let's say that the host was a  
linux machine. In that case, you would move CURRENT.FGP to /fingerdb/linux/  
directory, and possibly rename it someting more descriptive, like lin0.fgp;  
Fingerprint file names must be in the same 8.3 format, with .fgp extention.  
From this point on, the device should find the matching fingerprint file in  
/fingerdb/linux/ directory, so it would know that the correct on-insertion  
payload is /fgscript/linux.txt;  
  
Keep in mind that fingerprints can be different for different versions  
of some particular OS, and can change with time. For example, if your  
windows 10 machine had an update which modified it's USB driver,  
the fingerprint will probably change too, so a new .fgp file would  
have to be added into the database. This is also the reason why you  
may need several fingerprint files for the same OS.  
  
The fingerprint is only collected once, when the device powers on. Any  
other events like rebooting the host machine or any other kind of reset  
which involves no poweroff will not result in a new fingerprint collection  
process, and will not invoke any on-insertion payload again. This also  
includes restart into MSD-only mode invoked by capslock toggle sequence.  
So, if you plug the device in before the host machine is powered on, the  
fingerprint collected will belong to the BIOS, not the OS, since this is  
the first piece of software that will enumerate the device.  
  
PREVIOUS.FGP file comes in handy in situations where you cannot collect and  
relocate/rename CURRENT.FGP file on the same machine, eg. target machine is  
locked in some way. So you first collect the fingerprint on the locked  
machine, which overwrites CURRENT.FGP; then you plug the device into your  
other machine. Since fingerprint collected on your machine does not match  
the fingerprint of the locked machine, CURRENT.FGP will be moved into  
PREVIOUS.FGP, and CURRENT.FGP will be overwritten with the latest data. Now  
you can put PREVIOUS.FGP into the right directory and give it an appropriate  
name. If you plug the device into your machine several times over, data in  
CURRENT.FGP will match the collected fingerprint, so the PREVIOUS.FGP will  
stay untouched (up until you plug the device into some new machine again)  
  
There is an example OS fingerprint database included in this repository,  
in **/extra/payloads/fingerprinterTest/fingerdb/**; though, it is not  
particulary hard to build your own.  
  