# PocketAdmin (rev 1.1)  

This is an open source keystroke injection device, similar to a well known [USB rubber ducky](https://www.youtube.com/watch?v=z5UUTUmGQlY&list=PLW5y1tjAOzI0YaJslcjcI4zKI366tMBYk)  
made by hak5. It looks and feels like an ordinary USB flash drive but acts as a keyboard that  
types in a preprogrammed payload. This can be very useful for automating sysadmin tasks or  
in penetration testing applications.  

![1.jpg](extra/pictures/1.jpg)  
![2.jpg](extra/pictures/2.jpg)  

The device here is intended to be a much improved verison of USB rubber ducky, namely:  

1. Made from inexpensive off-the-shelf parts, with not only open source firmware,  
but hardware design files as well. This allows the user to do substantial  
modifications to the design, as well as provides an option to build these yourself.  

2. Has a built-in interpreter (compatible with existing ducky script) which takes text files directly,  
so you never have to install any encoder software and keep converting payload.txt to inject.bin.  

3. Can act as both keyboard and USB disk, allowing for better payloads; the memory chip is integrated,  
so there is no need to keep sticking SD card in/out of various devices while developing payloads.  

4. Has an OS detection mechanism, which allows you to store multiple payloads simultaneously and  
have the device automatically pick the correct payload to run.  

5. Extended set of commands for extra functionality; for example:  
without doing any firmware update the user can set which VID / PID values to use, or  
configure how the device should show up (keyboard only / flash disk only / keyboard+disk);  

---

## hardware

project is designed using KiCad 5.0.2  
check KiCad pcb file for PCB manufacturing info  
check KiCad sch file + BOM.txt for component info  

programmer device used in this project is [ST-Link V2](https://www.aliexpress.com/item/1PCS-ST-LINK-Stlink-ST-Link-V2-Mini-STM8-STM32-Simulator-Download-Programmer-Programming-With-Cover/32792513237.html?ws_ab_test=searchweb0_0,searchweb201602_2_10152_10151_10065_10344_10068_10342_10546_10343_10340_10548_10341_10696_10084_10083_10618_10307_10135_10133_10059_100031_10103_10624_10623_10622_10621_10620,searchweb201603_55,ppcSwitch_3&algo_expid=448b8f37-4a09-4701-bf7f-8b2ce2770a23-0&algo_pvid=448b8f37-4a09-4701-bf7f-8b2ce2770a23&priceBeautifyAB=0)  
you can use single pin male-female jumpers or a 1x5pin jumper cable  
make sure to plug the programming cable into the header the right way  

based on full-speed (12Mbit/s) USB2.0 peripheral, uses on-board 32MiB flash memory chip for data storage;  
measured speeds for MSD access : read ~262.7 KiB/s, write ~66.8KiB/s. While not very fast, it is enough  
for most badusb applications.  

The pushbutton on the device will be referred to as MSD-only button. Normally the payload is run  
whenever you plug the device into a PC. But if you press and hold this button while inserting the  
device, it prevents any keystrokes from being typed in.  

## firmware

firmware (written in C) was developed on debian 9.7 system, using gcc-arm-none-eabi toolchain  
(compiler, linker, binutils) and it does use gcc specific extentions.  
was successfully compiled and tested with arm-none-eabi-gcc version 7.3.1  

flashing software used = openocd  
IDE used = emacs text editor + Makefile  

depends on libgcc.a, which together with the linker script, startup code  
and openocd configuration files is included in this repository.  

files usb\_rodata.h, hid\_rodata.h, msd\_rodata.h are not really  
headers, but integral parts of usb.c, main.c, msd.c respectively.  
they are not intended to be included in any other files.  

to build the firmware cd into the /firmware/ directory, then type:  

> make  

this will produce several output files, among which is firmware.bin  
this is a file that contains the firmware to flash. To do it,  
connect ST-LINKv2 programmer to the board, then to computer and type:  

> make upload  

for your convenience, a pre-built binary firmware image is available in /extra/ directory.  

## usage

By default PocketAdmin shows up as a compound device with HID (keyboard) and MSD (flash drive) interfaces.  
For the most basic use, there must be a FAT filesystem on the first partition, and in the root directory  
there must be a payload.txt file, which contains the commands that the device should run at every insertion.  
Optionally, alongside the payload.txt you can have a config.txt file. If this file if present, the device  
first runs pre-configuration commands from there, and then moves on to running some payload file.  

If "USE_FINGERPRINTER" pre-configuration command is present, the device will ignore payload.txt and  
instead choose a payload file from /fgscript/ directory. Available choises are: /fgscript/linux.txt,  
/fgscript/windows.txt, /fgscript/mac.txt or /fgscript/other.txt (if OS could not be determined);  
The OS detection works by comparing current OS fingerprint with a database stored in /fingerdb/ directory.  
First, the device will create /fingerdb/current.fgp file, which contains OS fingerprint of the machine it  
is currently plugged into, and then it will search directories /fingerdb/linux/, /fingerdb/windows/,  
/fingerdb/mac/ for a matching fingerprint file. The right script is chosen based on what directory the  
matching file was found in, or set to /fgscript/other.txt if no matching fingerprint was found at all.  

Fingerprint files are based on how a given USB driver reacts to PocketAdmin being inserted. That means  
fingerprints can be different for different versions of some particular OS, and can change with time.  
For example, if your windows 10 machine had an update which modified it's USB driver the fingerprint  
will possibly change too, so a new .fgp file has to be added into the database. To do that, you plug  
the device in and then copy /fingerdb/current.fgp into an appropriate /fingerdb/osname/ directory.  
Make sure that the file keeps .fgp extention and the new name is no longer than 8 characters (without .fgp);  
Once the fingerprint file is present in /fingerdb/osname your OS should be correctly detected.  

There is an OS fingerprint database included in this repository ( in /extra/fingerdb/ );  
If you have collected more fingerprints, I welcome you to share them, so they can be  
included here for everybody else's use.  

## commands

The commands in payload files are mostly the same as in existing [ducky script](https://github.com/hak5darren/USB-Rubber-Ducky/wiki/Duckyscript)  
There are however, some differences/extentions to ducky script:  

1. Pre-configuration commands are available: "HID\_ONLY\_MODE", "VID ", "PID ", "USE_FINGERPRINTER".  
With "VID ", "PID " commands user can change VID / PID values used for enumeration. They take hex  
numbers as argument, and 0x prefix is mandatory (e.g. "VID 0x046D" or "PID 0xC228").  
With "HID\_ONLY\_MODE" command, device will either enumerate in HID-only mode or in MSD-only mode  
(depends if MSD-only button is pressed); if such command is not present device enumerates in  
HID+MSD mode, but will not type any keystrokes if MSD-only button is pressed.  

2. Pre-configuration commands (if present) must be placed separately in the config.txt file and come  
as one contiguous block of up to 4 commands (no other types of commands are allowed in config.txt)  
For example:  
"HID\_ONLY\_MODE"  
"VID 0x2385"  
"PID 0x5635"  

3. "DELAY " command first waits extra time until host had sent at least 1 read command to MSD interface  
(in MSD+HID configuration) and received at least 1 report from HID interface (all configurations),  
and only then waits for a specified number of milliseconds. This is done to help payloads run at the  
very first insertion, even into a completely new machine. The problem is, it might take many seconds  
before host will do its initializations (e.g. install drivers). This way you dont have to put huge  
delays at the start of the payload, and the "DELAY " command only waits for GUI elements to update.  

4. "STRING " command only accepts ASCII-printable characters, max length of the string is 400 characters.  
If you want to type a string in a different language, make sure that target machine's GUI is set to your  
desired language and then use "STRING " command with ASCII symbols that are bound to the same  
physical keys as the symbols you are trying to print. For example, if GUI is configured to use  
RU layout, "STRING Dtkjcbgtl CNTKC" command will result in "Велосипед СТЕЛС" string typed.  
(because you have to press the same key to type 'l' or 'д', same key for 'k' or 'л', etc)  

5. Single ASCII-printable character commands are available, for which no SHIFT modifier will be used,  
that is, (unlike in "STRING " command) both commands "M" and "m"  will type "m". This also means  
that commands such as "GUI r" or "GUI R" are the same.  
Any non ASCII-printable character causes the rest of the line to be ignored.  

6. There can be multiple kewords on one line (up to 5), but only if modifier key commands are used.  
All other commands (including single character commands) execute and skip the rest of the line.  
Modifier key commands only work if followed by a press key command or newline, for example,  
"CTRL ALT DELETE", "CONTROL SHIFT T" or "ALT " are valid commands.  

7. If you want a modifier key pressed (CTRL, SHIFT, ALT, GUI), but no keycode sent along with it,  
always keep a spacebar after the keyword, such as "CTRL ", "ALT " instead of "CTRL", "ALT"  

8. Commands "DEFAULT\_DELAY", "DEFAULTDELAY", "DELAY ", "REPEAT\_SIZE ", "REPEAT ",  
take numeric arguments (decimal number string). Length of these decimal strings can not  
be more than 6 symbols. That means "DELAY 1234567" is not valid, but "DELAY 123456" is.  

9. "REPEAT " command can repeat a block of commands (max block size = 400 characters). "REPEAT\_SIZE "  
command specifies the number of commands in this block, for example the following script will execute  
3 commands right before REPEAT for 11 times (once normally + repeated 10 times):  
"REPEAT\_SIZE 3"  
"STRING this command block of 3 commands is repeated"  
"ENTER"  
"DELAY 300"  
"REPEAT 10"  

Every command must start at the beginning of a line and end with a newline character.  
Quotation marks used here are not parts of commands and are just used to indicate start and  
end of a command. Full list of available keywords is available in /extra/listOfKeywords.txt  

Some example payloads can be found in /extra/examplePayloads/ but keep in mind that payloads  
need to be modified to fit your target machine (as well as your exact requirements),  
since they heavily depend on OS used, GUI settings, language, hardware capabilities, etc.  

Of course, there are plenty other scripts/executables/original ducky payloads available  
on the internet for you to use, if you are too lazy to make your own payloads. Remember,  
it can do anything that your keyboard can (even more than that, actiually)  

## directories info

#### /firmware/ --------------- contains makefile, linker script, source files; this is a build directory  

/firmware/cmsis/ ------- necessary header files from CMSIS compliant [STM32F0xx standard peripherals library](https://www.st.com/content/st_com/en/products/embedded-software/mcus-embedded-software/stm32-embedded-software/stm32-standard-peripheral-libraries/stsw-stm32048.html)  

/firmware/stdlib/ ---------- standard statically linked libraries (libgcc.a)  

/firmware/openocd/ ------- standard configuration files for openocd  

/firmware/fatfs/ ----------- [chan fatfs](http://www.elm-chan.org/fsw/ff/00index_e.html) module for working with FAT filesystem, along with diskio.c + diskio.h  
(custom low level driver for communication with W25Q256FVFG flash memory chip over SPI)  

/firmware/usb/ ------------ custom USB stack, implementation of MSD and HID class devices  

/firmware/main/main.c ------- file that contains main application  
/firmware/main/support.c ------- file that contains interrupt vector table, IRQ handlers and startup code  

#### /hardware/ ------------------- contains KiCad project, schematic, PCB files  

/hardware/PocketAdmin.symbols/ -- project specific symbol library  

/hardware/PocketAdmin.pretty/ --- project specific footprint library  

/hardware/gerbers/ ----------- gerber+excellon fabrication output files  

#### /extra/ -------------------  contains pictures, various extra documents, etc.  

/extra/examplePayloads/ ----------- contains some example PocketAdmin payloads  
/extra/pictures/ ------------------ contains device photos  
/extra/mechanicalDrawings/ -------- contains info for various mechanical parts  
/extra/fingerdb/ ------------------ contains OS fingerprint database  

/extra/pocketadmin.pdf ----------- pdf version of schematic  
/extra/firmware_rev1.1 ----------- precompiled firmware image for pocketadmin revision 1.1  
/extra/listOfKeywords.txt -------- list of available payload commands  


## contact info

if you have a problem / question / feature request, here are your options for contacting me:  
send me an email to krakrukra@tutanota.com  
create a new github issue, or use of the existing one called [general discussion](https://github.com/krakrukra/PocketAdmin/issues/1)  
use EEVBlog forum [post](https://www.eevblog.com/forum/oshw/pocketadmin-an-open-source-keystroke-injection-device-badusb/), dedicated to the project  
also, you can check out my [youtube channel](https://www.youtube.com/channel/UCpx3VbcqgMQ-Zv4x2wwBbzA)  
  
#### if you want to buy:  
openbazaar shop link (online whenever my PC is running):  
ob://QmeCrxkz8J1pvBx4nVE7EgZNkLfMftmKtz3dc5oo4bPgqr/store  
or, you can preview the store [here](https://openbazaar.com/store/QmeCrxkz8J1pvBx4nVE7EgZNkLfMftmKtz3dc5oo4bPgqr), if you do not have openbazaar app installed yet  

tindie shop link:  
ebay shop link:  
