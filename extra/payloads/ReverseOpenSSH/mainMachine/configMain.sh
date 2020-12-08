#!/bin/bash

# copy user-specific ssh configuration files to current user's directory
username=$(whoami)
mkdir "/home/$username/.ssh"
cp ./user/* "/home/$username/.ssh/"
sudo chmod 600 "/home/$username/.ssh/id_rsa"
sudo chmod 644 "/home/$username/.ssh/id_rsa.pub"
sudo chmod 644 "/home/$username/.ssh/known_hosts"
sudo chown -R "$username:$username" "/home/$username/.ssh"

# copy global ssh configuration files into /etc/ssh
sudo mkdir /etc/ssh
sudo cp ./global/* /etc/ssh
sudo chmod 644 /etc/ssh/ssh_config
sudo chown root:root /etc/ssh/ssh_config
