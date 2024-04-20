#!/bin/bash

#Set the port the first time the script is run.
if [[ $(echo $PORT) == "" ]]; then
	PORT=<INPUT_STARTING_PORT>
fi

#Set the IP addresses.
export IP_ATTACKER="<INPUT_IP_ADDRESS>"
export IP_VICTIM1="<INPUT_IP_ADDRESS>"
export IP_VICTIM2="<INPUT_IP_ADDRESS>"

#Set the passwords.
export PASS_ATTACKER="<INPUT_PASSWORD>"
export PASS_VICTIM1="<INPUT_PASSWORD>"
export PASS_VICTIM2="<INPUT_PASSWORD>"

#Set up ssh.
(sshpass -p "$PASS_ATTACKER" ssh-copy-id -o "StrictHostKeyChecking=no" -i ~/.ssh/id_rsa.pub attacker@$IP_ATTACKER) &> /dev/null
(sshpass -p "$PASS_VICTIM1" ssh-copy-id -o "StrictHostKeyChecking=no" -i ~/.ssh/id_rsa.pub victim1@$IP_VICTIM1) &> /dev/null
(sshpass -p "$PASS_VICTIM2" ssh-copy-id -o "StrictHostKeyChecking=no" -i ~/.ssh/id_rsa.pub victim2@$IP_VICTIM2) &> /dev/null

#Set the port, same for all IP addresses.
export PORT=$(($PORT - 1))
