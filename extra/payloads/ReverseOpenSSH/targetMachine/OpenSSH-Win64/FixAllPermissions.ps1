[CmdletBinding(SupportsShouldProcess=$true)]
param ()
Set-StrictMode -Version 2.0
If ($PSVersiontable.PSVersion.Major -le 2) {$PSScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path}
Import-Module $PSScriptRoot\OpenSSHUtils -Force

#check sshd config file
if(Test-Path "C:\ProgramData\ssh\sshd_config" -PathType Leaf)
{
  Repair-SshdConfigPermission -FilePath "C:\ProgramData\ssh\sshd_config" @psBoundParameters
}
 
#check host keys
Get-ChildItem $env:ProgramData\ssh\ssh_host_*_key -ErrorAction SilentlyContinue | % {
  Repair-SshdHostKeyPermission -FilePath $_.FullName @psBoundParameters
}

#check authorized_keys
if(Test-Path "C:\ProgramData\ssh\authorized_keys" -PathType Leaf)
  {
    Repair-AuthorizedKeyPermission -FilePath "C:\ProgramData\ssh\authorized_keys" @psBoundParameters
  }

#check ssh config file
if(Test-Path "C:\ProgramData\ssh\ssh_config" -PathType Leaf)
{
    Repair-UserSshConfigPermission -FilePath C:\ProgramData\ssh\ssh_config @psBoundParameters
}

#check identity keys
Get-ChildItem "C:\ProgramData\ssh\*" -Include "id_rsa","id_dsa" -ErrorAction SilentlyContinue | % {
  Repair-UserKeyPermission -FilePath $_.FullName @psBoundParameters
}

Write-Host "   Done."
