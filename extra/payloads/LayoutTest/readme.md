This payload is intended for testing keyboard layout files, which is  
why the layout file database is stored here in /kblayout/ directory.  
You can directly copy all the files here to PocketAdmin's storage,  
then make sure some text editor is open before inserting the device.  
Device will try to type out a long string containing all of the  
available ASCII-printable characters.  
  
I recommend to first open the payload.txt file and copy-paste the  
string from there to serve as a reference. Then, you should move your  
cursor to the line below and plug the device in. If both PocketAdmin  
and target machine's GUI are configured to use the same keyboard  
layout, the string that was typed in by the device should be the same  
as the reference string above.  
  
If any characters are not the same as above you should either use a  
different layout file, or modify it so it works. In this example  
French layout is expected. To change it you should edit the  
config.txt and replace "fr_FR" with a different layout filename.  
  
Note: on Italian keyboard there is no way to input tilde and  
backtick symbols ( ~ and ` ), so spacebars are used instead.  
  
This payload also uses HID_ONLY_MODE, so you will need to use  
the MSD-only button (or long capslock toggle sequence) to make  
the device show up as a flash drive again.  
  