#include <string>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <fstream>

#include "../libs/pstream.h"
#include "../libs/rang.hpp"
#include "../libs/json.hpp"

#pragma once

/**
 * Simple wrapper for the main attributes of listed UTXOs.
 */
struct transactionInfo {
    int m_btcValue;
    int m_vout;
    std::string m_txID;
    std::string m_address;

    //Constructor.
    transactionInfo(const int btcValue, const int vout, const std::string txID, const std::string address) {
        m_btcValue = btcValue;
        m_vout = vout;
        m_txID = txID;
        m_address = address;
    }
};

/**
 * Class that provides functions for generating/choosing parameters used in transactions.
 */
class CParameterPreparator {
public:
    /**
     * Default constructor.
     */
    CParameterPreparator() = default;

    /*
     * Function to list addresses and return chosen one.
     */
    std::string chooseAddress();

    /*
     * Function to list usable UTXOs and return chosen one.
     */
    transactionInfo chooseUTXO();

    /*
     * Function to generate two addresses based on a specified input.
     */
    std::pair<std::string, std::string>
    generateAddresses(const std::string generateAddressFunction1, const std::string generateAddressFunction2);

    /*
     * Function to calculate ammounts to be paid/returned back to account based on UTXOs and fees.
     */
    std::pair<float, float> calculateAmmounts(const transactionInfo &UTXO, const float expectedFullFee);
};
