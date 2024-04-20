#!/bin/bash 

#MODIFY TO FIT YOUR SETUP!
#IP addresses of devices.
export IP_ATTACKER="<INPUT IP ADDRESS>"
export IP_VICTIM1="<INPUT IP ADDRESS>"
export IP_VICTIM2="<INPUT IP ADDRESS>"
PASS_VICTIM1="<INPUT_PASSWORD>"
PASS_VICTIM2="<INPUT_PASSWORD>"

#Configure ssh.
(sshpass -p "$PASS_VICTIM1" ssh-copy-id -o "StrictHostKeyChecking=no" -i ~/.ssh/id_rsa.pub victim1@$IP_VICTIM1) &> /dev/null
(sshpass -p "$PASS_VICTIM2" ssh-copy-id -o "StrictHostKeyChecking=no" -i ~/.ssh/id_rsa.pub victim2@$IP_VICTIM2) &> /dev/null

#Create a fitting bitcoin.conf file, ports are by-default assigned as 18445.
cp ~/attacker/default_bitcoin.conf ~/.bitcoin/bitcoin.conf
echo "bind=$IP_ATTACKER:18445" >> ~/.bitcoin/bitcoin.conf
echo "addnode=$IP_VICTIM1:18445" >> ~/.bitcoin/bitcoin.conf
echo "addnode=$IP_VICTIM2:18445" >> ~/.bitcoin/bitcoin.conf

#Start bitcoin daemon in regtest mode.
bitcoind -daemon

#Wait for the start of the network.
sleep 1

#If a wallet exists, import it (we do not check default file location for wallet and create our own file "representing" it!). Otherwise, create new wallet.
if [ -f ~/attacker/wallet.out ]; then
	bitcoin-cli loadwallet $(cat ~/attacker/wallet.out | jq -r '.name') > /dev/null
else
    	bitcoin-cli createwallet "attacker_wallet" > ~/attacker/wallet.out
fi
