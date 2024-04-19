#!/bin/bash

#If bitcoin core is still running, stop it.
bitcoin-cli stop 2> /dev/null

#Remove information about old regtest, existing wallet and existing addresses.
rm -r ~/.bitcoin/regtest 2> /dev/null
rm -r ~/victim1/addresses/* 2> /dev/null
rm -r ~/victim1/wallet.out 2> /dev/null
rm -r ~/victim1/finishedTransactions.txt 2> /dev/null
