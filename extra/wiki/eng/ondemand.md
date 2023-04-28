Most badusb devices start executing a payload script once they are  
inserted into a target machine. In some sense, this is a perfect  
time to run a payload, since this is exactly the time when user can  
expect some kind of popup windows appear, the screen would not be  
locked and the user is not actively typing / clicking with any real  
HID input devices. However, this approach puts a limitation on the  
number of payloads that you can carry on the device simultaneously.  
  
In situations where you need more payload scripts (eg. one with a  
keylogger, other with reverse shell, third with data extraction)  
and when you also have an opportunity to select appropriate payload  
manually, you can use a different approach. This approach is called  
on-demand payloads (as opposed to normal on-insertion payloads).  
  
This new approach allows you to store up to 17 extra payload scripts  
in a dedicated /ondemand/ directory. The names of these payload files  
should be in the format "scriptNN.txt", where NN is a number from 3  
to 19, with the first digit set to 0 if necessary. That is, path to  
the file should be /ondemand/script03.txt, /ondemand/script04.txt,  
/ondemand/script05.txt, etc, going up to /ondemand/script19.txt.  
In order to call a particular script you should use the capslock key  
on a real keyboard, tapping on it the same number of times as is the  
number of payload script that you want to execute. So, if you want to  
call script05.txt, you would press the capslock key 5 times in a row.  
Once there was 1 second without any more capslock toggles, the device  
considers it to be the end of sequence and starts to execute whatever  
particular payload that you have selected. Once this payload script  
has finished execution, the device will go back to idle state, waiting  
for a new on-demand payload to be selected.  
  
If you will tap on capslock 1 or 2 times, then device will do nothing.  
A sequence of 20 or more capslock toggles will result in a special  
behaviour. The device will disconnect itself from the bus and reconnect  
as if MSD-only button was pressed. This effectively allows you to use  
MSD-only button without actually having to take the device apart and  
press the actual hardware button. If you input a sequence of 20 or more  
capslock toggles again, the device will enter a DFU mode, without any  
other way to come back except pulling the device out again.  
  
All the on-demand payloads work no matter if MSD-only button was used  
or not and are completely independent from all the other functions, eg.  
there is no difference if you have enbled OS fingerprinter, HID-only  
mode, etc. If the script file that you have requested was not found,  
the device will stay in the idle state, but sequences of 20 or more  
toggles will function even if there is no valid filesystem found on device.  
  
Support for on-demand payloads on macOS is very limited, as on that  
OS keyboard LED state is usually not shared between different keyboards.  
  