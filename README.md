# PocketAdmin  
  
This is a keystroke injection device (also called badusb). It is similar to a well-known  
[USB rubber ducky](https://shop.hak5.org/products/usb-rubber-ducky-deluxe) made by hak5, but has much extended functionality, lower price  
and is also completely open source. It looks and feels like an ordinary USB flash drive,  
but acts as a keyboard that types in a preprogrammed payload. This payload can do anything  
from configuring a network to installing a reverse shell, since the device can basically  
do whatever an admin can with a terminal, but taking only a few seconds. This makes  
it a very powerful tool for automating sysadmin tasks or use in penetration testing.  
  
![outside.jpg](extra/pictures/photos/outside.jpg)  
![inside.jpg](extra/pictures/photos/inside.jpg)  
  
Here is quick summary of how PocketAdmin is different from USB rubber ducky (and many others):  
  
1. Made from inexpensive off-the-shelf parts, with not only open source firmware,  
but hardware design files as well. This allows the user to do substantial  
modifications to the design, as well as provides an option to build your own units.  
  
2. Has a built-in interpreter (compatible with existing ducky script) which takes text files directly,  
so you never have to install any encoder software and keep converting payload.txt to inject.bin.  
  
3. Has USB mass storage capability, allowing for better payloads; the memory chip is integrated,  
so there is no need to keep sticking SD card in/out of various devices while developing payloads.  
  
4. Has an OS detection mechanism, which allows you to store multiple on-insertion payloads  
simultaneously and have the device automatically pick the correct payload to run.  
  
5. You can save up to 17 extra on-demand payloads and execute them by tapping on a capslock key  
the appropriate number of times. You can also use this functionality for more convenient device  
operation, such as rebooting into mass storage of DFU modes without taking the device apart.  
  
6. Extended set of commands for extra functionality, such as mouse control, "HOLD" and "RELEASE"  
commands, dynamic delays, ability to repeat blocks of commands; you can have several non-modifier  
keys pressed simultaneously and you are not limited in keyword order or by a set of hardcoded key
combinations, only by the maximum of 10 commands on a single line.
  
7. User has several configuration options available, none of which require a firmware update.  
You can set which serial number and VID / PID values to use, how the device should show up  
(keyboard+mouse+disk or keyboard+mouse only), change keyboard layout, hide a particular  
memory region on the USB disk, show fake storage capacity to the host, etc.  

**CHECK THE [WIKI](https://github.com/krakrukra/PocketAdmin/wiki) FOR HOW-TO-USE INFORMATION**  

---

## hardware

project is designed using KiCad 5.1.5  
check KiCad pcb file for PCB manufacturing info  
check KiCad sch file + BOM.txt for component info  
  
uses integrated full-speed (12Mbit/s) USB2.0 peripheral,  
with on-board 128MiB flash memory chip for data storage;  
only 96MiB are available for use due to overprovisioning reasons.  
measured speeds for MSD access : read 800-850 KiB/s, write 700-750 KiB/s,  
  
There is an LED and a pushbutton on the device. The LED is a mass storage  
status indicator, which lights up any time the device reads or writes flash  
memory. It is recommended to make sure this LED is turned off before pulling  
the device out of host machine, to prevent filesystem corruption.  
The pushbutton on the device is referred to as MSD-only button. Normally  
the payload is run whenever you plug the device into a PC. But if you press  
and hold this button while inserting the device, it prevents any keystrokes  
from being typed in and makes the device show up as a flash drive. It also  
prevents the use of camouflage features like hidden region or fake capacity.  
  
Fully assembled unit has dimensions of 59x18x9mm and weight of 8 grams.  
When opening up the case, be careful no to break the plastic studs near  
the USB connector and at the opposite (from USB) end of enclosure.  
  
hardware programmer device used in this project is [ST-Link V2](https://www.aliexpress.com/item/1PCS-ST-LINK-Stlink-ST-Link-V2-Mini-STM8-STM32-Simulator-Download-Programmer-Programming-With-Cover/32792513237.html?ws_ab_test=searchweb0_0,searchweb201602_2_10152_10151_10065_10344_10068_10342_10546_10343_10340_10548_10341_10696_10084_10083_10618_10307_10135_10133_10059_100031_10103_10624_10623_10622_10621_10620,searchweb201603_55,ppcSwitch_3&algo_expid=448b8f37-4a09-4701-bf7f-8b2ce2770a23-0&algo_pvid=448b8f37-4a09-4701-bf7f-8b2ce2770a23&priceBeautifyAB=0)  
For instructions on how to build and flash the device go check this video:  
[https://www.youtube.com/watch?v=cfud5Dq_w2M](https://www.youtube.com/watch?v=cfud5Dq_w2M)  
  
## firmware  
  
programming language used = C  
flashing software used = openocd  
IDE used = emacs text editor + Makefile  
  
the firmware was developed on debian 10.3 system, using gcc-arm-none-eabi toolchain  
(compiler, linker, binutils) and it does use gcc specific extentions.  
it was successfully compiled and tested with arm-none-eabi-gcc version 7.3.1  
  
depends on libgcc.a, which is included in this repository. linker script,  
startup code and openocd configuration files are included here as well.  
  
files usb\_rodata.h, hid\_rodata.h, msd\_rodata.h are not really  
headers, but integral parts of usb.c, main.c, msd.c respectively.  
they are not intended to be included in any other files.  
  
for easy in-field updates, you can use the DFU bootloader. There is a dfu  
firmware image available in /firmware/firmware\_RRNNN.dfu file. The name  
format is this: RR stands for board revision (13 = rev 1.3) , NNN stands  
for firmware version. For example, firmware\_13000.dfu means  
board revision 1.3, firmware version 0  
  
## directories info

#### /firmware/ --------------- contains makefile, linker script, source files; this is a build directory  
  
/firmware/cmsis/ ------- necessary header files from CMSIS compliant [STM32F0xx standard peripherals library](https://www.st.com/content/st_com/en/products/embedded-software/mcus-embedded-software/stm32-embedded-software/stm32-standard-peripheral-libraries/stsw-stm32048.html)  
/firmware/stdlib/ ---------- standard statically linked libraries (libgcc.a)  
/firmware/openocd/ ------- standard configuration files for openocd  
/firmware/fatfs/ ---------- [chan fatfs](http://www.elm-chan.org/fsw/ff/00index_e.html) filesystem module + custom W25N01GVZEIG disk driver  
/firmware/usb/ ------------ custom USB stack, implementation of MSD and HID class devices  
/firmware/main/ ------- main application file, interrupt vector table, IRQ handlers and startup code  
/firmware/dfuse-pack.py ------- python script to create .dfu firmware images  
/firmware/firmware_13NNN.dfu ------ pre-compiled firmware image in DfuSe format (STM32)  
  
#### /hardware/ ------------------- contains KiCad project, schematic, PCB files  
  
/hardware/PocketAdmin.symbols/ -- project specific symbol library  
/hardware/PocketAdmin.pretty/ --- project specific footprint library  
/hardware/gerbers/ ----------- gerber+excellon fabrication output files  
  
#### /extra/ -------------------  contains pictures, payload files, documents, etc.  
  
/extra/pictures/ ---------------- photos and mechanical drawings  
/extra/wiki/ ------------------ github wiki pages  
/extra/payloads/ ----------- example and test payloads for PocketAdmin  
/extra/payloads/fingerprinterTest/fingerdb/ -- example OS fingerprint database  
/extra/payloads/layoutTest/kblayout/ --- alternative keyboard layout files  
  
## contact info  
  
if you have a problem / question / feature request, here are your options for contacting me:  
send me an email to krakrukra@tutanota.com  
create a new github issue, or use of the existing one called [general discussion](https://github.com/krakrukra/PocketAdmin/issues/1)  
also, you can check out my [youtube channel](https://www.youtube.com/channel/UC8HZCV1vNmZvp7ci1vNmj7g)  
  
For extra security, you could use my PGP public key saved in [/extra/pubkey.asc](https://github.com/krakrukra/PocketAdmin/blob/master/extra/pubkey.asc).  
  
#### if you want to buy:  
  
My tindie store: [https://www.tindie.com/stores/krakrukra/](https://www.tindie.com/stores/krakrukra/)  
  
My openbazaar store: ob://QmR671bca56jLqQfcN4t1WRxKsaYtYDRBQQcLcrVR9HiXe/store/pocketadmin-badusb  
You need openbazaar app installed to use this; web preview is available [here](https://openbazaar.com/QmR671bca56jLqQfcN4t1WRxKsaYtYDRBQQcLcrVR9HiXe/store/pocketadmin-badusb)  
Openbazaar store is online whenever my PC is powered on.  
  