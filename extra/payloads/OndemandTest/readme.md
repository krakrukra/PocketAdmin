This payload is intended for testing on-demand payload functionality.  
Once you plug the device in, default on-insertion payload will open  
up notepad and do nothing else. If you input a sequence of several  
capslock toggles, device should call an appropriate payload from  
/ondemand/ directory, which will type out the following string:  
"this is an ON-DEMAND payload number **n**!", where **n** is the number  
of the payload script that was called.  
  
Since this payload uses HID_ONLY_MODE, you will need to use the  
MSD-only button (or long capslock toggle sequence) to make  
the device show up as a flash drive again.  
   