# Blockchain-Vulnerabilities
This is a repository for the practical part of the bachelor thesis **Security Analysis of Blockchain Consensus Protocols**. As was stated in the thesis, the practical part concerns attacks on the Proof of Work (PoW) and Proof of Stake (PoS) consensus protocols. The attacks are demonstrated on a LAN network. The consensus protocols use different clients and the network setup is different for each of them.

## 2. Setup
The attacks were tested to function correctly on devices running Ubuntu `22.04` OS, but should after slight modifications be supported on most linux distributions. We offer two approaches of setting up the testing network:

<ol>
	<li>Virtual machine</li>
	<ul>
		<li>The recommended approach. The prepared virtual machines contain the whole testing environment.
		<ol>
			<li><a href="https://drive.google.com/file/d/1HVkepO7HxPVoy-m7bZKDLM-6-YwXo6Yk/view?usp=sharing">Attacker</a></li>
			<li><a href="https://drive.google.com/file/d/11KOadLxpXeOh8ngwCxDh9kcufj4MIhpW/view?usp=sharing">Victim1</a></li>
			<li><a href="https://drive.google.com/file/d/1Du0rF0998bvuVo6-fJRVqO4c61KRIMf6/view?usp=sharing">Victim2</a></li>
		</ol> 
		</li>
	</ul>
	<li>Installation scripts</li>
	<ul>
		<li>Each of the dedicated folders (PoW_Attacks, PoS_Attacks) contains installation scripts with the files corresponding to the setup of a testing machine. For further reference on how to use install scripts, please check the dedicated folder. Please note that the PoC scripts use ssh to communicate and will not work if the ssh cannot connect properly. To avoid such problems, please ensure that the linux accounts running the scripts are named "attacker", "victim1" and "victim2".<b> We recommend running these scripts in an isolated testing environment, as they move, change and delete files on the device and may move, rewrite and delete important files (such as the files in the .ssh folder).</b></li> 
	</ul>
</ol>

## 3. Start an Attack 
To start an attack, navigate into a folder `~/attacks` (`~/PoW_attacks` and `~/PoS_attacks` if using the VMs) on the attacker machine and run `make`. Folder called PoC containing the executable PoC scripts will be created. Before running these scripts, it is important to ensure that the environmental variables are correctly configured (and in case of the PoW consensus protocol, that the bitcoin server is running). That is, once again, client specific, so please navigate into the dedicated folders of this repository to find out how to do so.
