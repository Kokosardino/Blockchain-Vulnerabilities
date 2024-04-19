#!/bin/bash

#Read info about added nodes.
ADDED_NODE_INFO=$(bitcoin-cli getaddednodeinfo)
NODE_CNT=0

#For each JSON in the received output, check if the nodes are properly connected.
for JSON in $(echo "$ADDED_NODE_INFO" | jq -c '.[]'); do
	if [[ $(echo $JSON | jq ".connected == true") != true ]]; then
		exit 1
	fi
	NODE_CNT=$(( NODE_CNT + 1 ))
done

#Two nodes should be connected. For any other number read, return a fault.
if [ $NODE_CNT != 2 ]; then
	exit 1;
fi

#Return success.
exit 0
