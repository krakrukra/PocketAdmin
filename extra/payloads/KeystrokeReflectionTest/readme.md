This is a windows specific payload, which is intended to show  
how keystroke reflection can be performed. You can directly copy  
all the files here to PocketAdmin's storage and it should work.  
  
The payload consists of invoking windows runline window and  
sending powershell commands through that 3 times in a row.  
First time it will get the username and save it into a temporary  
file on the host machine. Second time it will transform this  
file into a correct sequence of key presses and save it in a  
file again. Then payload enables keystroke reflection and runs  
a third line of powershell commands to actually send the key  
sequence specified in a file. This process usually takes some  
time. After the data transfer is over it will be saved on  
PocketAdmin's USB storage in a file /keyref/username.txt  
  
Since this payload uses HID_ONLY_MODE, you will need to use the  
MSD-only button (or long capslock toggle sequence) to make  
the device show up as a flash drive again.  
  