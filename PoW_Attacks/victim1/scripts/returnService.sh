#!/bin/bash 

#Default price for the service.
#Handle with caution, changing this value may break some PoCs (but it is encouraged for better understanding on how the PoCs work)!
PRICE=10
REQUIRED_CONFIRMATIONS=0

#Create a valid service file. 
echo "Service from victim1!" > ~/victim1/serviceVictim1.txt 
echo "Timestamp: [$(date +%s)]" >> ~/victim1/serviceVictim1.txt


#For each of the transactions concerning the loaded wallet, capped at [200].
for JSON in $(echo "$(bitcoin-cli listtransactions '*' 200)" | jq -c '.[]'); do
	
	#Save raw version of the transaction and the paid ammount of bitcoins.
	RAW_TX_JSON=$(bitcoin-cli decoderawtransaction $(bitcoin-cli gettransaction $(echo $JSON | jq -re ".txid") | jq -re ".hex"))
	AMMOUNT_PAID=$(echo $JSON | jq -r ".amount")
	EXISTING_CONFIRMATIONS=$(echo $JSON | jq -r ".confirmations")

	#Check if the transaction ID is in the solved transactions, to prevent duplicate service provision.
	TRANSACTION_FINISHED="false"
	while read line; do
		if [[ "$line" == "$(echo $JSON | jq -r '.txid')" ]]; then
			TRANSACTION_FINISHED="true"
		fi
	done < ~/victim1/finishedTransactions.txt
	
	#If [[ loaded transaction is not a coinbase transaction ]] and [[ paid ammount of bitcoins is greater than price for the service ]] and [[ transaction was not yet processed ]] and [[ transaction has the required ammount of confirmations ]]
	if [[ $(echo "$RAW_TX_JSON" | jq -e '.vin[0] | has("coinbase")') == "false" ]] && [[ $(echo "$AMMOUNT_PAID>=$PRICE" | bc) == "1" ]] && [[ $TRANSACTION_FINISHED == "false" ]] && [[ $(echo "$EXISTING_CONFIRMATIONS>=$REQUIRED_CONFIRMATIONS" | bc) == "1" ]];  then
		
		#IMPORTANT: Only works if the transactions have exactly three outputs and third one is the "data" segment. All PoCs follow this pattern, but for other transactions, this script will break.
		ENCODED_MESSAGE=$(echo $RAW_TX_JSON | jq -re ".vout[2] | .scriptPubKey" | jq -re ".asm" | tr -d "OP_RETURN ")
		
		#Copy file representing the service to the PC.
		scp ~/victim1/serviceVictim1.txt $(echo $ENCODED_MESSAGE | xxd -r -p):~/attacker/victim1/serviceVictim1.txt &> /dev/null

		#Add the transaction id into the finishedTrasactions.txt file.
		echo $(echo $JSON | jq -r '.txid') >> ~/victim1/finishedTransactions.txt

	fi
done
