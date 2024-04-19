#!/bin/bash

#BASH SCRIPT TO CHECK THAT ALL NODES IN THE NETWORK ARE RUNNING, CONNECTED AND SYNCHRONIZED.

#Get information about connection of each node. 
ATTACKER_SYNC_CHECK=$(~/scripts/checkConnections.sh; echo $?)
VICTIM1_SYNC_CHECK=$(ssh victim1@$IP_VICTIM1 2> /dev/null "~/scripts/checkConnections.sh; echo \$?")
VICTIM2_SYNC_CHECK=$(ssh victim2@$IP_VICTIM2 2> /dev/null "~/scripts/checkConnections.sh; echo \$?")

#Check whether returned numbers of the blocks equal.
if [ "$VICTIM1_SYNC_CHECK" == 0 ] && [ "$VICTIM2_SYNC_CHECK" == 0 ] && [ "$ATTACKER_SYNC_CHECK" == 0 ]; then
	exit 0
fi
exit 1 
