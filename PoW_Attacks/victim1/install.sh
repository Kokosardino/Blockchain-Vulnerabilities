#!/bin/bash

#Install required tools.
echo "------------------------------------------"
echo "Installing tools required by applications."
echo "------------------------------------------"

echo "y" | sudo apt install curl
echo "y" | sudo apt install jq
echo "y" | sudo apt install openssh-server
echo "y" | sudo apt install sshpass
#Install bitcoin-core.
echo "------------------------"
echo "Installing bitcoin core."
echo "------------------------"

touch bitcoin.tar.gz
curl 'https://bitcoin.org/bin/bitcoin-core-25.0/bitcoin-25.0-x86_64-linux-gnu.tar.gz' --output bitcoin.tar.gz
tar -xvf bitcoin.tar.gz
sudo install -m 0755 -o root -g root -t /usr/local/bin bitcoin-25.0/bin/*
rm -r bitcoin-25.0 bitcoin.tar.gz

echo "-------------------------------"
echo "Setting up rest of the network."
echo "-------------------------------"

#Create an ssh key. 
ssh-keygen -f "/home/victim1/.ssh/id_rsa" -P "" <<<y > /dev/null 2>&1

#Prepare rest of the network information.
mkdir -p ~/.bitcoin
cp -r scripts ~/scripts
cp -r victim1 ~/victim1

echo "---------------------------------------------------------------------------------------------------------------------"
echo "Network was set up. Try starting it by modifying IP addresses in ~/scripts/start.sh and executing ~/scripts/start.sh!"
echo "---------------------------------------------------------------------------------------------------------------------"
