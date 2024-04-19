#include <utility>
#include <string>
#include <sstream>
#include <iostream>

#include "../libs/pstream.h"
#include "../libs/json.hpp"
#include "../libs/rang.hpp"

#pragma once

/**
 * Class that handles transaction related functions, including creating, signing, sending and deleting of raw transactions.
 */
class CTransactionHandler {
public:
    /**
     * Deafult constructor.
     */
    CTransactionHandler() = default;

    /**
     * Function that appends encoded information data segment with attacker information into transactions, creates and signs them.
     * Returnes hexes of the signed transactions.
     */
    std::pair<std::string, std::string>
    createSignedTransactions(std::ostringstream &tx1Oss, std::ostringstream &tx2Oss);

    /**
     * Function to broadcast raw transaction into the network.
     */
    std::string sendTransaction(const std::string &signedTransactionHex);

    /**
     * Function to delete transaction from wallet.
     */
    void deleteTransaction(const std::string txid);

    /**
     * Function to print information of transaction specified by txid.
     */
    void printTransaction(const std::string function, const std::string &searchedTxid);

    /**
     * Function to print comparison of two specified transactions.
     */
    void printTransactions(const std::string printName1, const std::string printName2, const std::string listFunction1,
                           const std::string listFunction2, const std::string &sentTxid1, const std::string &sentTxid2);
};
