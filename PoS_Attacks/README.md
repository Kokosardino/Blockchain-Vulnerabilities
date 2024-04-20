## 1. Introduction
This directory contains PoCs of attacks on PoS based blockchain networks. Due to the problematically specific nature of the attacks and ethical concerns, I decided to approach the attacks differently than the attacks on the PoW protocol. Instead of using an existing client, I wrote my own (<a href=https://github.com/Kokosardino/vulnCoin>vulnCoin</a>). As was outlined in the thesis, the client itself is very unsafe and should be used for no other purposes than conducting the prepared attacks on it. Whole setup of the environment can be found in the folder `Network-real`. Troughout the testing, I found out that the network client brings unnecessary complexity to the attacks. That is why I also offer version of the grinding attack that only simulates the network. It can be found in the folder `Network-simulation`.

## 2. Block Validation
 **DISCLAIMER**: The following is a implementation of a very vulnerable and inherently flawed consensus protocol. Under any circumstances do not attempt to build your consensus protocol to implement concepts described below.

For quick reference, the explanation of how the consensus mechanism we implemented works following:

<img src="vulncoin_consensus_validation.png"> </img>

`t`
 * A `genesis` block is created automatically by the network.
 * Network users can stake for creation of block `#1` and, but the stakePool has to be finalised before the time `t+1`.

 `t+1`
 * Stakepool for `genesis` block and `genesis` block are used as inputs for the pseudorandom function that selects the block creator from stakepool `#1`.
 * Stakepool for block `#2` is opened.

 `t+2`
 * Stakepool for block `#1` and block `#1` are used as inputs for the pseudorandom function that selects the block creator from stakepool `#2`.
 * Stakepool for block `#3` is opened.

 The whole process continues in the same fashion until the network is stopped.

## 3. Pseudorandom Function

 Let $H$ represent `SHA256` hash of the last block in the blockchain, $\{a_1, a_2, \dots,a_n\} \in M$ stakepool where each address $a$ is represented as hexadecimal string and $N$ number of addresses in the current stakepool. Consider the function $\text{first32bits}()$ that returns the first 32 bits of a string. The pseudorandom selection function is implemented as:

 $$ \text{IndexOfValidator} \equiv (\text{first32bits}(H) + \text{first32bits}(a_1) + \dots + \text{first32bits}(a_n))\mod{N} $$

## 4. Setup
**Only follow this section if you choose not to use prepared VMs!**

On the `attacker` machine, navigate into the folder `network-real` and run:
 
`./scripts/installAttacker.sh`

On the `victim` machines, navigate into the folder `network-real` and run:
 
`./installVictim.sh`

Check that a folder `~/attacks` was created on the `attacker` machine. Please note that the PoCs in the `network-simulation` do not require any additional setup. You can simply run make in the folder and start them!

## 5. Environment

Before executing the PoC scripts, it is also important to add generated ssh key as trusted to all the other nodes. In aim to achieve high flexibility of the testing environment, we decided to save IP addresses as environmental variables. This approach left us with the need to define a configuration file that sets up the network properties and starts the bitcoin client. To find it, navigate into `~/attacks/exportVariables.sh` and change the values to fit your network setup. After doing so, run the script **in the current shell** with command:

`. exportVariables.sh`

During PoCs, we are starting and stoping a server on specified port via `cpp` program. After stopping the server on a port, the system needs time to clean it up. Therefore, issuing PoC in a short time on the same port can cause connectivity issues. The `exportVariables.sh` fights these issues this by decrementing a `PORT` variable by one each time it is run. We recommend setting up the initial port number high (e.g. 10000) and running `exportVariables.sh` before each start of a PoC script.