#!/bin/bash

# make sure script is executing with root privileges
if (( $EUID != 0 ))
then
  printf "You must run configurePublic.sh as root\n"
  exit
fi

# create a new user called "user", create a .ssh config directory for him
mkdir /home/user
mkdir /home/user/.ssh
useradd user -d /home/user
printf "setting a new password for user\n"
passwd user
# make main machine authorized to log in as user, give user a normal shell
cp ./user/authorized_keys /home/user/.ssh/authorized_keys
chmod 644 /home/user/authorized_keys
chown -R user:user /home/user
chsh -s /bin/bash user

# create a new user called "peasant", create a .ssh config directory for him
mkdir /home/peasant
mkdir /home/peasant/.ssh
useradd peasant -d /home/peasant
printf "setting a new password for peasant\n"
passwd peasant
# make target machine authorized to log in as peasant, prevent any command execution
cp ./peasant/authorized_keys /home/peasant/.ssh/authorized_keys
chmod 644 /home/peasant/authorized_keys
chown -R peasant:peasant /home/peasant
chsh -s /bin/true peasant

# install global ssh configuration files for public machine
mkdir /etc/ssh
cp ./global/* /etc/ssh
chmod 600 /etc/ssh/ssh_host_ecdsa_key
chmod 644 /etc/ssh/ssh_host_ecdsa_key.pub
chmod 644 /etc/ssh/sshd_config
chown -R root:root /etc/ssh

# prevent access to ssh host key inside setup directory
chmod 600 ./global/ssh_host_ecdsa_key
chown root:root ./global/ssh_host_ecdsa_key

# configure firewall rules on public machine, allow TCP port 22 and ports 2200 through 2205
iptables-restore  < ./rules.v4
ip6tables-restore < ./rules.v6

# install a package to make firewall rules persistent, make sure correct config files are used
apt-get install iptables-persistent sudo
mkdir /etc/iptables/
cp ./rules.v4 /etc/iptables
cp ./rules.v6 /etc/iptables
