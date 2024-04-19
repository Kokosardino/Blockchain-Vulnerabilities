## 1. Intro
This directory contains PoCs of attacks on PoW based blockchain networks. Three devices (`attacker`, `victim1` and `victim2`) exist within the network. Standard `bitcoin-core client` in a regtest mode was chosen for development and deployment. 

## 2. Setup
Attacks are tested to function correctly on devices running Ubuntu `22.04` OS, but should after slight modifications support most linux distributions. We offer two approaches to setting up the testing network:

<ol>
	<li>Virtual machine</li>
	<ul>
		<li>Recommended approach. Prepared virtual machines contain the whole testing environment.
		<ol>
			<li><a href="https://drive.google.com/file/d/1haxUF3g6XQUg09PI8bFQYEOCWzdU4Q4x/view?usp=sharing">Attacker</a></li>
			<li><a href="https://drive.google.com/file/d/1DJglvYGbhr_RDD9z9Mk-msb7CKH4oPJw/view?usp=sharing">Victim1</a></li>
			<li><a href="https://drive.google.com/file/d/1-_pEdBQ-ZgPXB8K7QgQYd6ybQpA_e5d-/view?usp=drive_link">Victim2</a></li>
		</ol> 
		</li>
	</ul>
	<li>Install script</li>
	<ul>
		<li>Each of the dedicated folders (attacker, victim1, victim2) contains install scripts with corresponding files to setup a testing machine. <b>We recommend running these scripts in an isolated environment, as they move, change and delete files on the device and may move, rewrite and delete important files (such as files in .ssh folder).</b></li>
	</ul>
</ol>

To finish setting up the environment and start the network, please change IP addresses and passwords in `~/scripts/start.sh` on each of the devices accordingly and run the script with command `. ~/scripts/start.sh`. Please note that the the script exports environment variables and it is therefore imperative to start attacks from the same shell that `start.sh` was run in. Network expects that the accounts created in the network are called attacker, victim1 and victim2. The PoCs will only work as long as this requirement is fullfilled. 

## 3. Attacks

To compile attack scripts, run `make` in the `attacks` folder and wait for the compilation. Then simply start an attack by executing `.PoC/<attackName>` and the program will comprehensively lead you trough the attack. At the start of each attack, synchonization sequence is initiated to ensure that all network nodes are connected and synchronized. If such synchronization fails, attack is not conducted. Each of the attacks offers comprehensive proof of concept. 

## 4. Stop

To stop the running network, simply run `~/scripts/stop.sh`. To reset the network information (wallets, addresses, blockchain) and start anew, run `~/scripts/reset.sh`.
