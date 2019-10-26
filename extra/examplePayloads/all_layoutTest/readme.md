This payload is intended for testing keybard layout files, which  
means you will have to copy /extra/kblayout/ onto the device.  
You should have some text editor open before inserting the device,  
and both PocketAdmin and target machine's GUI have to be configured  
to use the same keyboard layout. The device should then type a long  
string containing all of the available printable characters.  
  
I recommend to first open the payload.txt and copy-paste the string  
from there to serve as a reference. Then you should move your cursor  
to the line below and plug the device in. If everything went well,  
the string that was typed in by the device would be the same as the  
reference string above.  
  
If any characters are not the same as above you should either use a  
different layout file, or modify it so it works. In this example  
French layout is expected. To change it you should edit the  
config.txt and replace "fr_FR" with a different layout filename.  
  
Note: on Italian keyboard there is no way to input tilde and  
backtick symbols ( ~ and ` ), so spacebars are used instead.  
  
This payload also uses HID_ONLY_MODE, so you will need to use  
the MSD-only button to make it show up as a flash drive again.  
  