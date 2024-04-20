## 1. Introduction
This directory contains PoCs of attacks on PoW based blockchain networks. Three devices (`attacker`, `victim1` and `victim2`) exist within the network. Standard `bitcoin-core client` in a regtest mode was chosen for development. 

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

Before executing the PoC scripts, it is important to start the bitcoin client and load wallet. It is also important to add generated ssh key as trusted to all the other nodes. In aim to achieve high flexibility of the testing environment, we decided to save IP addresses as environmental variables. This approach left us with the need to define a configuration file that sets up the network properties and starts the bitcoin client. To find it, navigate into `~/scripts/start.sh` and change the values to fit your network setup. After doing so, run the script **in the current shell** with command:

`. start.sh`

Repeat this procedure on each of the machines. Then and only then is the environment set up. Please note that nodes may take some time to connect to each other. If the PoC scripts return network error, try debugging the network setup by issuing command:

`bitcoin-cli getaddednodeinfo`

If any of the nodes return `connected: "false"`, then they are not connected. In such cases, check that the IP addresses fit. If they do, try waiting for connection and if the waiting time is long (over three minutes), try reseting the network. You can do so easily by running `~/scripts/reset.sh`. Resets deletes all files associated with the network instance. If you wish to only stop the bitcoin client, run `~/scripts/stop.sh` instead.