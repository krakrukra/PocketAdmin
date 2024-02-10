Some example scripts can be found in [/extra/payloads/](https://github.com/krakrukra/PocketAdmin/tree/master/extra/payloads) directory.  
Every example payload is placed in a dedicated directory, inside of which  
all the relevant files are included, along with a readme.md documentation  
file which is optional and only exists to explain what the payload does.  
You can read these documentation files by browsing into each specific  
payload directory. Sometimes you can simply copy all the contents inside  
of such directory to the device and have it run the payload, but not always.  
You might also need to take some initial preparation steps (such as setting  
FAT volume label, etc) before your payload can actually run. I recommend  
to always read the readme.md file and possibly all the in-script comments  
before using any of the examples.  
  
Keep in mind that it is not possible to make some one-size-fits-all  
payloads that will magically do exactly what you want, without you  
ever touching the code or doing whatever preparations that are needed.  
Everybody has their own personal preferences and requirements, such as  
local language settings, different target hardware / OS, etc. Basically,  
you should not just copy-paste examples, but actually understand what  
you are doing. All these example payloads only serve one purpose, which  
is to give you an idea of how the device can be used. Many of them are  
intentionally made to be simple and generic. In any kind of real  
application, you will have to build up your own payload, possibly reusing  
some of the code or techniques that you have learned from example payloads.  
  
Since the command language used by PocketAdmin is based on duckyscript 1.0,  
you can also quite easily reuse payloads that were originally designed for  
the [USB rubber ducky](https://github.com/hak5/usbrubberducky-payloads), or some of the payloads for [HAK5 bash bunny](https://github.com/hak5/bashbunny-payloads).  
Though, you could also take some inspiration from payloads which are in a  
substantially different format, such as scripts for [arduino style](https://github.com/samratashok/Kautilya) badusb's;  
if necessary, you can convert them to duckyscript without too much effort.  
The exact same know-what-you-do precautions apply here; if somebody made  
a payload back in 2012 for windows7 does not necessarily mean that you  
can blindly copy-paste it and it will work.  
  
---
  
Always keep in mind, that for everything to work you MUST have  
all the right files in the right places! That means:  
  
1. If you have any pre-configuration commands, you need to place them  
separately in a **config.txt** file in the device's root directory.  
  
2. If fingerprinter was not enabled (default), you need to place  
your script file (**payload.txt**) in the device's root directory.  
  
3. If fingerprinter was enabled, you need to place your script files  
(windows.txt, linux.txt, other.txt, etc) in **/fgscript/** directory  
and also provide a database of .fgp files in **/fingerdb/** directory.  
  
4. If you want to replace the default US keyboard layout with some other  
one, you need to place required layout files in **/kblayout/** directory. The  
database of layout files is available in [/extra/payloads/FeatureTesting/LayoutTest/kblayout/](https://github.com/krakrukra/PocketAdmin/tree/master/extra/payloads/FeatureTesting/LayoutTest/kblayout)  
  
5. If you want to use on-demand payloads, you need to place them inside  
**/ondemand/** directory, with names from **script03.txt** up to **script19.txt**  
  