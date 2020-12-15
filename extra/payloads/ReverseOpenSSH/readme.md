This payload is intended for establishing a reverse SSH tunnel  
from windows **target machine** to a **public machine** (with public  
IP address). Target machine requests from public machine that  
port forwarding should be established on a particular TCP port.  
Any kind of traffic destined to this TCP port will then be  
redirected through the secure tunnel to the target machine,  
where a target ssh server will be listening. So, if somebody  
has the right private key and initiates ssh connection to the  
right port on the public machine they will be able to log into  
the target machine instead. This somebody is assumed to connect  
from another, **main machine** which runs on debian linux. Such  
setup will allow for remote command execution and SFTP file  
transfer even in situations where both main and target machines  
are in different LAN's and are unable to communicate directly.  
  
More information about this payload is available in this [video](https://www.youtube.com/watch?v=h9NPPq0q95o)  
This payload requires quite a bit of preliminary setup to work,  
so I encourage you to spend some time and learn all the details.  
  
#### preparations (short description)  
  
1. Rent a VPS from some hosting provider, make sure you select  
debian linux as it's operating system.  
  
2. Optionally, save IP of your public machine (VPS) into a  
local hosts file in **/etc/hosts**; Here it will be assumed  
that you did that, and you named your server **someVPS**.  
  
3. Download the whole repository, unzip it, navigate to the  
payload folder in a terminal.  
  
4. Use "sudo apt-get install openssh-client" command to  
install ssh client on the main machine.  
  
5. Run **keysetup.sh** bash script to generate id keys,  
host keys, authorized_keys and known_hosts files. All  
of the dedicated machine folders will be affected.  
  
6. Copy subdirectory called **publicMachine** to your VPS,  
eg. like this "scp -r ./publicMachine root@someVPS:/root"  
  
7. Connect to your public machine via ssh ("ssh root@someVPS"),  
go to publicMachine directory, run **configPublic.sh** as root.  
Confirm that you want to save firewall rules if asked.  
  
8. Optionally, install sudo and add username "user" to the  
**/etc/sudoers** file (on your VPS).  
  
9. Delete publicMachine directory from your VPS, reboot it  
with "systemctl reboot" command. You should be dropped back  
to the main machine's terminal after that.  
  
10. Go to **mainMachine** subdirectory, run **configMain.sh** script  
from there as a normal user, not root (use the same account that  
will log into public and all target machines later on).  
  
11. Go to **targetMachine** subdirectory and add your VPS  
IP address to insert.ps1 script, by replacing the  
default configuration string of "192.168.1.46".  
  
12. Mount your PocketAdmin's drive, eg. "sudo mount /dev/sda1 /mnt".  
That implies that device was assigned "sda" name on your machine.  
  
13. Copy all of the contents inside **targetMachine** subdirectory  
to USB storage of PocketAdmin (not the directory as a whole).  
  
14. Unmount your PocketAdmin's drive and set it's FAT  
volume label, eg. "sudo fatlabel /dev/sda1 POCKETADMIN"  
  
#### execution (short description)  
  
On insertion, **payload.txt** file opens up a windows runline  
dialog, where a command is typed in. This command will find  
the correct driveletter for PocketAdmin's USB storage and  
run an insert.ps1 script from the root directory over there.  
It will be run with administrative privileges, so make sure  
that currently logged in user on target machine has that.  
  
insert.ps1 script will copy some of the official [Win32-OpenSSH](https://github.com/PowerShell/Win32-OpenSSH)  
binaries and scripts to local folder "C:\Program Files\OpenSSH",  
copy ssh configuration files to "C:\ProgramData\ssh", configure  
file permissions and firewall rules. It will also set up a new  
scheduled task for ssh client and a new windows service for ssh  
server, so that they both launch at every bootup of the target.  
  
Once insert.ps1 script execution is done, there will be a  
new file on PocketAdmin's USB storage, inside **/portusers/**  
directory. The name of this file will contain a port number,  
and inside will be a list of all local users on a particular  
target machine. This information can be used later when the  
main machine connects to your VPS, as you need to specify  
some port and username to log in as. Effectively, this way  
you can choose where to connect, VPS or a particular target.  
  