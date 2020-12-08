#!/bin/bash

# create identity keys for all machines that will act as ssh client (main and target machines)
ssh-keygen -t rsa -b 4096 -f ../mainMachine/user/id_rsa -N "" -C "main_id_key"
ssh-keygen -t rsa -b 4096 -f ../targetMachine/global/id_rsa -N "" -C "target_id_key"

# make main machine authorized to log into public and target machines
cat ../mainMachine/user/id_rsa.pub > ../targetMachine/global/authorized_keys
cat ../mainMachine/user/id_rsa.pub > ../publicMachine/user/authorized_keys
# make target machine authorized to log into public machine
cat ../targetMachine/global/id_rsa.pub > ../publicMachine/peasant/authorized_keys

# create host keys for all machines that will act as ssh server (public and target machine)
ssh-keygen -t ecdsa -b 256 -f ../publicMachine/global/ssh_host_ecdsa_key -N "" -C "public_host_key"
ssh-keygen -t ecdsa -b 256 -f ../targetMachine/global/ssh_host_ecdsa_key -N "" -C "target_host_key"

# make sure public and target machines are known hosts for main machine
echo -n "* " >  ../mainMachine/user/known_hosts
cat ../publicMachine/global/ssh_host_ecdsa_key.pub >> ../mainMachine/user/known_hosts
echo -n "* " >> ../mainMachine/user/known_hosts
cat ../targetMachine/global/ssh_host_ecdsa_key.pub >> ../mainMachine/user/known_hosts

# make sure public machine is a known host for target machine
echo -n "* " > ../targetMachine/global/known_hosts
cat ../publicMachine/global/ssh_host_ecdsa_key.pub >> ../targetMachine/global/known_hosts
