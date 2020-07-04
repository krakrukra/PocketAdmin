This is a windows specific payload, which will find the drive  
that has a FAT volume label set to POCKETADMIN, and then go to  
this drive and start a powershell script from **insert.ps1** file  
with administrative privileges. That will clearly require that  
the currently logged in user has admin rights, but it is quite  
a common situation. This payload uses a dynamic delay before UAC  
bypass, so you would have to manually double-tap on capslock key  
after the UAC window appears.  
  
In order to change the FAT volume label on windows, you can just  
plug the device in, select the correct drive in the file manager  
(in "My Computer" -> "Devices and Drives") and press F2.  
Alternatively, in Linux you can use the fatlabel utility for that.  
  
The default **insert.ps1** script does not do anything, you should  
replace it with something that fits your desires. If you do not  
know powershell or how to write scripts for it, you can always  
search for scripts online. Though, realistically, you still would  
have to learn some powershell to reuse them properly. As an  
example, here is a collection of penetration testing scripts  
by Nikhil Mittal: [https://github.com/samratashok/nishang](https://github.com/samratashok/nishang)  
And here is an example of how one of these scripts can be used with  
PocketAdmin: [GetInformationScript payload](https://github.com/krakrukra/PocketAdmin/tree/master/extra/payloads/GetInformationScript)  
  