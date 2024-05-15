## 1. Introduction
This directory contains PoCs of attacks on PoW based blockchain networks. Three devices (`attacker`, `victim1` and `victim2`) exist within the network. Standard `bitcoin-core` client in a regtest mode was chosen for development. 

## 2. Setup
**Only follow this section if you choose not to use prepared VMs!**

On the `attacker` machine, navigate into the folder `attacker` and run:
 
`./install.sh`

On the `victim1` machine, navigate into the folder `victim1` and run:
 
`./install.sh`

On the `victim2` machine, navigate into the folder `victim2` and run:
 
`./install.sh`

Check that a folder `~/scripts` was created on each of the machines and that the folder `~/attacks` was created on the `attacker` machine.
## 3. Environment

Before executing the PoC scripts, starting the bitcoin client and loading the wallet is essential. Adding the generated ssh key as trusted to all the other nodes is also important. To achieve high flexibility of the testing environment, we decided to save IP addresses as environmental variables. This approach left us with the need to define a configuration file that sets up the network properties and starts the bitcoin client. To find it, navigate into `~/scripts/start.sh` and change the values to fit your network setup. After doing so, run the script **in the current shell** with the command:

`. start.sh`

Repeat this procedure on each of the machines. Only then is the environment set up. Please note that nodes may take some time to connect. If the PoC scripts return a network error, try debugging the network setup by issuing the command:

`bitcoin-cli getaddednodeinfo`

If any of the nodes return `connected: "false"` they are not connected. In such cases, check that the IP addresses fit. If they do, try waiting for a connection, and if the waiting time is long (over three minutes), try resetting the network. You can do so easily by running `~/scripts/reset.sh`. Reset deletes all files associated with the network instance. If you wish only to stop the bitcoin client, run `~/scripts/stop.sh` instead.