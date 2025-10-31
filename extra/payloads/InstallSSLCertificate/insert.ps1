#find correct driveletter and path. Then import new root CA into target machine
$driveletter=(Get-Volume -FileSystemLabel POCKETADMIN).DriveLetter
Import-Certificate -FilePath "${driveletter}:\ca-certificate.pem" -CertStoreLocation Cert:\LocalMachine\Root

#add a new entry into hosts file on target machine. Replace default values for IP and domain name here with your own.
$text = "`n"
$text = $text + "192.168.0.102 website.here" + "`n"
$text = $text + "192.168.0.102 www.website.here" + "`n"
Add-Content -Path "C:\Windows\System32\drivers\etc\hosts" -Value "${text}"

#close chrome, edge and firefox browsers, so that when user opens them again they refresh their list of root certificates
Stop-Process -Name chrome
Stop-Process -Name msedge
Stop-Process -Name firefox

#eject the PocketAdmin's usb drive automatically
(New-Object -comObject Shell.Application).NameSpace(17).ParseName("${driveletter}:").InvokeVerb("Eject")
