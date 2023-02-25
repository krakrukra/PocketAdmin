#### IMPORTANT: for previous 1.2 revision devices, use the [old upgrade process](https://github.com/krakrukra/12004-pocketadmin-wiki)  
  
With new rev 1.3 hardware, you do not have to use St-Link V2  
hardware programmer device for performing firmware updates.  
You may instead use the DFU bootloader and send new updated  
firmware directly over USB. To do so, you will need to install  
some dedicated software on your PC, plug the PocketAdmin into  
this same PC and make sure that your device is in DFU mode.  
Then, you feed a particular firmware image in DfuSe format to  
this dedicated software and wait for it to finish the update.  
The latest firmware image is available in the repository, in  
**/firmware/firmware_13nnn.dfu** file, where nnn is a number  
which stands for firmware version (eg. firmware_13000.dfu);  
  
There are 2 ways to enter DFU mode. If your device is already  
functional, you can get into DFU mode by performing a long  
capslock toggle sequence **twice**. That is, first time you tap  
on a capslock key 20 or more times in a row, device will reboot  
into MSD mode. You tap capslock 20 or more times again,  
and device reboots into DFU mode.  
If the previous firmware on the device is corrupted/nonexistent,  
you will have to first short out boot0 pin to 3.3V line, and  
only then plug the device in (it will enter DFU mode right away);  
This can be done at the programming holes, using a piece of metal  
wire or just with tweezers. The 2 holes you need to connect are  
in the corner of PCB and they both have round copper pads around  
them, square pad should be on the other side and is not used.  
  
You can check [this video](https://www.youtube.com/watch?v=t0oajBFZcZY) for a demonstration of  
both old and new firmware update procedures.  
  
---
  
#### linux (debian) DFU update procedure  
  
1. install dfu-util (sudo apt-get install dfu-util)  
2. download this repository (.zip) to your PC and unpack it  
3. open terminal window in /firmware/ directory  
4. connect the device to PC via USB port  
5. tap on capslock 20 times or more  
6. wait at least 5 seconds  
7. tap on capslock 20 times or more again  
8. run command: dfu-util -a 0 -D firmware_13nnn.dfu  
9. wait for update process to finish, pull the device out  
  
#### windows DFU update procedure  
  
1. download [DfuSe demo](https://www.st.com/en/development-tools/stsw-stm32080.html) software  
2. install it and start the program  
3. download this repository (.zip) to your PC and unpack it  
4. connect the device to PC via USB port  
5. tap on capslock 20 times or more  
6. wait at least 5 seconds  
7. tap on capslock 20 times or more again  
8. click on "Choose" button (bottom right)  
9. select the /firmware/firmware_13nnn.dfu file  
10. select "Internal Flash" as target region (Target ID = 00)  
11. click on "Upgrade" button (bottom right)  
12. confirm by clicking "yes"  
13. wait for update process to finish, pull the device out  
  