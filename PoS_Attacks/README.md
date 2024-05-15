## 1. Introduction
This directory contains the PoCs of attacks on a PoS-powered blockchain networks. Due to the problematically specific nature of the attacks and ethical concerns, We decided to approach the attacks differently than the attacks on the PoW protocol. Instead of using an existing client, we wrote our own (<a href=https://github.com/Kokosardino/vulnCoin>vulnCoin</a>). As was outlined in the thesis, the client itself is very unsafe and should be used for no other purposes than conducting the prepared attacks on it. Whole setup of the environment can be found in the folder `Network-real`. Troughout the testing, we found out that the network client brings unnecessary complexity to the attacks. That is why we also offer version of the grinding attack that only simulates the network. It can be found in the folder `Network-simulation`.

## 2. Block Validation
The following is a implementation of a very vulnerable and inherently flawed consensus protocol. Under no circumstances should you attempt to build your consensus protocol to implement the concepts described below.

As a quick reference, the following is an explanation of how the consensus mechanism we have implemented works following:

<img src="vulncoin_consensus_validation.png"> </img>

`t`
 * The `genesis` block is automatically created by the network.
 * Network users can stake for a chance to be selected as the creator of the block `#1`, but the stakePool must to be finalised before the time `t+1`.

 `t+1`
 * The stakepool for the `genesis` block and the `genesis` block are used as inputs for the pseudo-random function that selects the block creator from the stakepool `#1`.
 * The stakepool for the block `#2` is opened.

 `t+2`
 * The stakepool for the block `#1` and the block `#1` are used as inputs for the pseudorandom function that selects the block creator from the stakepool `#2`.
 * The stakepool for the block `#3` is opened.

 The whole process continues in the same way until the network is stopped.
 
 ## 3. Pseudorandom Function

Let $H$ represent the `SHA256` hash of the last block in the blockchain, $\{a_1, a_2, \dots,a_n\} \in S$ a stakepool, where each vulnCoin address $a$ of a coin holder is represented as a hexadecimal string and $N$ the number of addresses in the current stakepool. Furthermore, please consider the function $\text{first32bits}()$ which returns the first 32 bits of a string. The pseudorandom selection function is implemented as:

 $$ \text{IndexOfValidator} \equiv (\text{first32bits}(H) + \text{first32bits}(a_1) + \dots + \text{first32bits}(a_n))\mod{N} $$

## 4. Setup
**Only follow this section if you choose not to use prepared VMs!**

On the `attacker` machine, navigate to the `network-real` folder and run:
 
`./scripts/installAttacker.sh`

On the `victim` machines, navigate to the `network-real` folder and run:
 
`./installVictim.sh`

Check that a `~/attacks` folder has been created on the `attacker` machine. Please note that the PoCs in the `network-simulation` do not require any additional setup. You can simply run `make` in the folder and start them!

## 5. Environment

Before executing the PoC scripts, it is also important to add the generated ssh key as trusted to all the other nodes. In order to achieve high flexibility of the testing environment, we decided to save IP addresses as environmental variables. This approach left us with the need to define a configuration file that sets up the network properties. To find it, navigate to `~/attacks/exportVariables.sh` and change the values to fit your network setup. Then run the script **in the current shell** with the command:

`. exportVariables.sh`

During the PoCs, vulnCoin servers are started and stopped on a specified port. After stopping a server running on a given port, the system needs time to clean up the port. Therefore, issuing PoCs on the same port in a short period of time may cause connectivity problems. The `exportVariables.sh` addresses these issues by decrementing a `PORT` variable by one each time it is run. We recommend setting up the initial port number high (e.g. 10000) and running `exportVariables.sh` before each start of a PoC script.