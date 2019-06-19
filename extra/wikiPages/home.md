By default PocketAdmin shows up as a compound device with HID (keyboard)  
and MSD (flash drive) interfaces. For the most basic use, there must be  
a FAT filesystem on the first partition, and in the root directory there  
must be a payload.txt file, which contains the commands that the device  
should run at every insertion.  
  
Optionally, alongside the payload.txt you can have a config.txt file.  
If this file if present, the device first runs pre-configuration commands  
from there, and then moves on to running some payload file.  
  
#### Read other wiki pages:  
* [commands](https://github.com/krakrukra/PocketAdmin/wiki/commands)  
* [OS fingerprinter](https://github.com/krakrukra/PocketAdmin/wiki/fingerprinter)  
* [keyboard layouts](https://github.com/krakrukra/PocketAdmin/wiki/layouts)  
* [example payloads](https://github.com/krakrukra/PocketAdmin/wiki/examples)  
  
#### Some videos are available on my [youtube channel](https://www.youtube.com/channel/UC8HZCV1vNmZvp7ci1vNmj7g)  
  