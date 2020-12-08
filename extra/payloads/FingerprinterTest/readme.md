This payload is intended for testing the OS detection mechanism.  
You can directly copy all the files here to PocketAdmin's storage,  
then make sure some text editor is open before inserting the device.  
Based on what the device thinks your OS is, it will type in  
"WINDOWS/LINUX/MAC/OTHER machine detected!"  
  
Since this payload does enable the fingerprinter, you will need to  
have a fingerprint database in /fingerdb/ directory on the device.  
Some example fingerprints are available in this payload directory,  
but it might be necessary to add your own fingerprint files there.  
To add a new OS called "someOS" you would place it's .fgp files in  
/fingerdb/someOS/ and a it's script file in /fgscript/someOS.txt  
  
This payload also uses HID_ONLY_MODE, so you will need to use the  
MSD-only button (or long capslock toggle sequence) to make  
the device show up as a flash drive again.  
  