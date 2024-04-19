#!/bin/bash

#Set the port the first time the script is run.
if [[ $(echo $PORT) == "" ]]; then
	PORT=10000
fi

#Set the IP addresses.
export IP_ATTACKER=10.0.2.15
export IP_VICTIM1=10.0.2.27
export IP_VICTIM2=10.0.2.28

#Set the port, same for all IP addresses.
export PORT=$(($PORT - 1))
