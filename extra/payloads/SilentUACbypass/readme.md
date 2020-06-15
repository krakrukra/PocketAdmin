This is a windows 10 specific payload, which is a variation on [RunPowershellScript payload](https://github.com/krakrukra/PocketAdmin/tree/master/extra/payloads/RunPowershellScript)  
  
The difference is, this payload launches **insert.ps1** script without requesting for  
admin rights explicitly, and instead attempts to do a silent UAC bypass inside the script  
(avoids showing a UAC confirmation window to the user). The currently logged in user  
still has to have admin rights on the system in order for this to work.  
  
This payload might not work with previous versions of windows OS.  
  