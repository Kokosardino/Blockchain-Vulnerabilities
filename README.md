# Blockchain-Vulnerabilities
This is a repository for practical part of bachelor thesis **Security Analysis of Blockchain Consensus Protocols**. As was outllined in the thesis, our practical part concerns attacks on Proof of Work (PoW) and Proof of Stake (PoS) consensus protocols. Attacks are demonstrated on a LAN network. Consensus protocols use different clients and the setup for the networks differs based on the cosensus protocols.

## 2. Setup
Attacks are tested to function correctly on devices running Ubuntu `22.04` OS, but should after slight modifications be supported on most linux distributions. We offer two approaches to setting up the testing network:

<ol>
	<li>Virtual machine</li>
	<ul>
		<li>Recommended approach. Prepared virtual machines contain the whole testing environment.
		<ol>
			<li><a href="">Attacker</a></li>
			<li><a href="">Victim1</a></li>
			<li><a href="">Victim2</a></li>
		</ol> 
		</li>
	</ul>
	<li>Install script</li>
	<ul>
		<li>Each of the dedicated folders (PoW_Attacks, PoS_Attacks) contains install scripts with corresponding files to setup a testing machine. For further reference on how to use install scripts, check the dedicated folder. Please note that the PoC scripts use ssh and will not work if the ssh cannot connect properly. To avoid such problems, please make sure that the linux accounts running the scripts have names "attacker", "victim1" and "victim2".<b> We recommend running these scripts in an isolated testing environment, as they move, change and delete files on the device and may move, rewrite and delete important files (such as files in .ssh folder).</b></li> 
	</ul>
</ol>

## 3. Start an Attack 
To start an attack, navigate into a folder `~/attacks` (`PoW_attacks` and `PoS_attacks` if using the VMs) on the attacker machine and run `make`. Folder called PoC containing the executable PoC scripts will be created. Before running these scripts, it is important to ensure that environmental variables are correctly configured (and in case of PoW, that the bitcoin server is running). That is, once again, client specific, so please navigate into the dedicated folders of this repository to find out how to do so.
