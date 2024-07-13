Besides simply reading a payload file and executing contained commands,  
PocketAdmin also provides several configuration options with which you  
can further specify how exactly the device should behave once plugged in.  
  
Such behaviour changes are done through the use of **pre-configuration**  
commands. Their format is similar to **special functionality** commands  
with no more than one command allowed on the same line, but they are  
different from all the normal commands because they act on the device  
itself and require to be executed before the device starts enumeration.  
Therefore, **pre-configuration** commands are not intended to be used in  
payload scripts, and instead must be placed in a separate config.txt file.  
Similarly, normal payload commands are not allowed inside of config.txt  
  
Pressing the MSD-only button prevents any on-insertion payload execution,  
but it also disables some **pre-configuration** commands. Specifically, these  
are "USE_HIDDEN_REGION", "SHOW_FAKE_CAPACITY **n**", "HID_ONLY_MODE"  
and "READ_ONLY_MODE" commands. The state of the button is read during  
device initialization, so you actually have to hold it down while inserting  
the device, and for 1-2 seconds after that. Rebooting into mass storage mode  
through long capslock toggle sequence (press 20 times or more) has the same  
effect and allows you to get normal access to device again.  
  
### Below is a complete list of all pre-configuration commands:  
  
* "VID **x**"   --- sets VID value for enumeration  
* "PID **x**"   --- sets PID value for enumeration  
* "SERIAL **s**"   --- sets serial number for enumeration  
* "HID_ONLY_MODE"   --- switches device to HID-only mode  
* "READ_ONLY_MODE"   --- makes USB storage read-only  
* "USE_FINGERPRINTER"   --- enables OS fingerprinter  
* "USE_LAYOUT **s**"   --- loads new keyboard layout from a file  
* "USE_HIDDEN_REGION"   --- make some memory unaccessible to host machine  
* "SHOW_FAKE_CAPACITY **n**"   --- lie to the host about device's capacity  
* "FIRST_INSERT_ONLY"   --- makes on-insertion payload run only one time  
* "MASS_ERASE"   --- unrecoverably erase all memory blocks  
  
---
  
1. With "VID **x**", "PID **x**", "SERIAL **s**" commands user can replace default  
VID, PID values and a serial number used for enumeration. VID, PID values  
are 4 digit hex numbers, while the serial number is a 12 character  
string, although only digits 0-9 and capital A-F letters are allowed in it.  
Any missing or invalid characters will be replaced by "0" in a serial number.  
Here are some examples: "VID 0x046D", "PID 0xC228", "SERIAL 32500A4B11FF"  
  
2. "HID_ONLY_MODE" command makes the device enumerate in HID-only mode (keyboard + mouse).  
Normally the device will enumerate in HID+MSD mode (keyboard + mouse + flash disk).  
It is useful in environments where USB storage devices are completely prohibited or  
whitelisted and you do not know which VID+PID values are allowed. So, by not showing  
MSD interface you can bypass such restrictions and still operate the device.  
  
3. "READ_ONLY_MODE" command makes the device's USB storage read-only (if available at all).  
Normally device's USB storage allows both read and write operations by the host PC. This  
command can be useful in case you want to protect files on your USB disk, eg. from being  
deleted by antivirus or some other software.  
  
4. "USE_FINGERPRINTER" command enables target OS detection.  
For more information check this [wiki page](https://github.com/krakrukra/PocketAdmin/wiki/fingerprinter)  
  
5. "USE_LAYOUT **s**" command replaces the default  
ASCII-to-HIDkeycode mapping with a new one.  
For more information check this [wiki page](https://github.com/krakrukra/PocketAdmin/wiki/layouts)  
  
6. "USE_HIDDEN_REGION" command reads the partition table (must be in MBR  
format), finds the first block right after the end of first partition and assigns  
it to be block number 0 for the host machine's read/write commands. This  
effectively makes the entire first partition and the partition table itself  
unaccessible by the host machine. In that case you can use the mass storage  
capability of the device, but without the end user seeing any payload, config,  
or other files that you use. Maximum size of hidden region is 16MiB, so you  
might need to edit the partition table before using this command. For example,  
if first 1MiB of memory contains the partition table and padding area, then  
the first partition of 15MiB or less can be hidden. If the partition is too large,  
this command is ignored.  
  
7. "SHOW_FAKE_CAPACITY **n**" command makes the device lie to the host machine  
about it's capacity. It takes a decimal number as an argument, which is new fake  
capacity in MiB (eg. "SHOW_FAKE_CAPACITY 2048" means pretend to be a 2GiB  
drive). The argument can be any value from 97 to 65535, otherwise the command  
is ignored. This command can be used if you want to make the device look more  
like an ordinary flash drive, since the real 96MiB of capacity can look suspicious  
to the end user. "SHOW_FAKE_CAPACITY **n**" and "USE_HIDDEN_REGION" do not  
interfere with each other, so you can use both at the same time, if necessary.  
  
8. "FIRST_INSERT_ONLY" command is yet another comouflage feature. With this  
command the device will create a new file in the root directory, called NOINSERT.  
If such file already exists when device is plugged in, then any on-insertion payloads  
will be blocked. This gives you an opportunity to run a payload on the first time  
the device is inserted, but then prevent payload execution until NOINSERT file is  
manually deleted. This can make the device less suspicious to the end user, since  
there will be no weird popup windows every single time that device is inserted.  
Note that the on-demand payloads are not blocked by this command.  
  
9. "MASS_ERASE" command erases all the data blocks on the device which contain,  
or have previously contained any data. The process will be started on the next  
time that device is plugged in, and it should take just a few seconds. After  
that, the device should show up as a flash drive again. Keep in mind, of course,  
that any stored files and partition table will be lost. This command is mainly  
intended to be a quick way to unrecoverably erase all the data on the device,  
without having to manually overwrite all logical blocks for a couple of times.  
  