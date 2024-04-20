#!/bin/bash

#Install required tools. 
echo "------------------------------------------"
echo "Installing tools required by applications."
echo "------------------------------------------"

echo "y" | sudo apt install git
echo "y" | sudo apt install make
echo "y" | sudo apt install openssh-server
echo "y" | sudo apt-get install libssl-dev
#Install vulnCoin.
echo "--------------------"
echo "Installing vulnCoin!"
echo "--------------------"

git clone https://github.com/Kokosardino/vulnCoin.git
cd vulnCoin
make
sudo mv bin/vulnCoin-server /bin/vulnCoin-server
cd ..

echo "-----------------------------------"
echo "Setting up the rest of the network!"
echo "-----------------------------------"

#Prepare ssh key.
ssh-keygen -f ~/.ssh/id_rsa -P "" <<<y > /dev/null 2>&1

#Set up the rest of the properties.
yes | rm -r vulnCoin

echo "------------------------------------------------------------------------------------"
echo "Victim node has been set up!"
echo "------------------------------------------------------------------------------------"

