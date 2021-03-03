By default PocketAdmin shows up as a compound device with HID  
(keyboard + mouse) and MSD (flash drive) interfaces. For the most  
basic use, there must be a FAT16 or FAT32 filesystem on the first  
partition (MBR partition table), and in the root directory there  
must be a payload.txt file, which contains plaintext commands that  
the device should execute. These commands will instruct the device  
to type in a specified string of characters, press a particular key  
combination or do something like wait for 500 milliseconds, etc.  
  
Normally the payload script (payload.txt) is run whenever you plug the  
device into a PC. This style of payload selection will be referred to  
as on-insertion payloads. Sometimes you do not want to run a payload  
every single time, for example, if you just want to edit some files.  
To prevent an on-insertion script execution you could use the MSD-only  
button. You need to hold it down, insert the device, wait around 1  
second after that, then release. In that case the device will not type  
anything in, and instead will show up as a flash drive, even if you have  
enabled HID-only mode. This button will also disable features like fake  
capacity and hidden region, so you are always able to access the files.  
Another way to prevent on-insertion payloads is with the "ALLOW_EXIT"  
command, which allows you to avoid taking the device apart every time.  
  
Optionally, you can have a config.txt file alongside the payload.txt.  
This is the file where pre-configuration commands can be placed.  
PocketAdmin first reads this file, alters it's behavior based on what  
it found inside, and only then moves on to running some payload file.  
More advanced features like on-demand payloads, OS fingerprinter or  
using a different keyboard layout will require some extra files and  
directories stored on the device. You can find more info about that  
in dedicated wiki pages.  
  
#### Read other wiki pages:  
* [commands](https://github.com/krakrukra/PocketAdmin/wiki/commands)  
* [configuration](https://github.com/krakrukra/PocketAdmin/wiki/configuration)  
* [on-demand payloads](https://github.com/krakrukra/PocketAdmin/wiki/ondemand)  
* [OS fingerprinter](https://github.com/krakrukra/PocketAdmin/wiki/fingerprinter)  
* [keyboard layouts](https://github.com/krakrukra/PocketAdmin/wiki/layouts)  
* [example payloads](https://github.com/krakrukra/PocketAdmin/wiki/examples)  
* [DFU bootloader](https://github.com/krakrukra/PocketAdmin/wiki/bootloader)  
  
#### Some tutorial videos are available on my [youtube channel](https://www.youtube.com/channel/UC8HZCV1vNmZvp7ci1vNmj7g)  
  