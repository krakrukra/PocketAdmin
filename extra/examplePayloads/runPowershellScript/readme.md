This is a windows specific payload, which will find the drive  
that has a FAT volume label set to POCKETADMIN, and then go to  
this drive and start a powershell script from insert.ps1 file  
with administrative privileges. That will clearly require that  
the currently logged in user has admin rights, but it is quite  
a common situation. The default script does not do anything,  
you should replace it with something that fits your desires.  
  
In order to change the FAT volume label on windows, you can just  
plug the device in, select the correct drive in the file manager  
(in "My Computer" -> "Devices and Drives") and press F2.  
Alternatively, in Linux you can use the fatlabel utility for that.  
  
If you do not know powershell or how to write scripts for it,  
you can always search for scripts online. Though, it is always  
beneficial to learn some powershell anyway. As an example, here  
is a collection of penetration testing scripts by Nikhil Mittal:  
[https://github.com/samratashok/nishang](https://github.com/samratashok/nishang)  
