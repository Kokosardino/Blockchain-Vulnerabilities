#include "CParameterPreparator.h"

std::string CParameterPreparator::chooseAddress() {
    std::cout << rang::fg::gray << rang::style::bold << "Listing usable addresses: " << std::endl << rang::style::reset;

    //For each file in the addresses directory, try to load it.
    int addressCount = 0;
    std::ostringstream oss;
    oss << "/home/attacker/attacker/addresses/address_0.txt";
    while (std::filesystem::exists(oss.str())) {
        //Open and read the file.
        std::ifstream addressFile(oss.str(), std::ios_base::in);
        std::string line;
        std::getline(addressFile, line);
        std::cout << rang::fg::green << rang::style::bold << "[" << addressCount << "] -> " << rang::style::reset
                  << line << std::endl;

        //Construct next file to be opened.
        ++addressCount;
        oss.str(std::string());
        oss << "/home/attacker/attacker/addresses/address_" << addressCount << ".txt";
    }
    std::cout << rang::fg::gray << rang::style::bold
              << "Select an address from the listed by typing the number representing it, or type \"generate\" to generate a new address to be used: "
              << rang::style::reset;

    //Get the response.
    std::string decision;
    std::cin >> decision;

    //Act accordingly to the decision.
    if (decision == "generate") {
        std::ostringstream oss;

        //Generate new address and save it.
        oss << "bitcoin-cli getnewaddress > ~/attacker/addresses/address_" << addressCount << ".txt";
        system(oss.str().c_str());
        oss.str(std::string());

        // Read the newly generated address, save it into a string and return it. 
        oss << "/home/attacker/attacker/addresses/address_" << addressCount << ".txt";
        std::ifstream addressFile(std::filesystem::path(oss.str()), std::ios_base::in);
        std::string address;
        std::getline(addressFile, address);
        std::cout << rang::fg::gray << rang::style::bold << "[using] -> " << rang::style::reset << address << std::endl;
        return address;

    }

    //Check that the input is a number.
    int decisionNmbr;
    try {
        decisionNmbr = std::stoi(decision);
    }
    catch (...) {
        std::cerr << rang::fg::red << rang::style::bold << "Unable to interpret the input, existing."
                  << rang::style::reset << std::endl;
        exit(1);
    }

    //Check that the input number is a valid number.
    if (decisionNmbr < addressCount) {
        std::ostringstream oss;

        //Load the chosen address and return it.
        oss << "/home/attacker/attacker/addresses/address_" << stoi(decision) << ".txt";
        std::ifstream addressFile(std::filesystem::path(oss.str()), std::ios_base::in);
        std::string address;
        std::getline(addressFile, address);
        std::cout << rang::fg::gray << rang::style::bold << "[using] -> " << rang::style::reset << address << std::endl;
        return address;
    }

    //If the input number was not a valid number, exit.
    std::cerr << rang::fg::red << rang::style::bold << "The number you entered is invalid, exiting."
              << rang::style::reset << std::endl;
    exit(1);
}


transactionInfo CParameterPreparator::chooseUTXO() {
    //Execute "listunspent" command and save its outputs.
    redi::ipstream listUtxo("bitcoin-cli listunspent", redi::pstreams::pstdout | redi::pstreams::pstderr);

    //Convert output into output stream, for better displaying and parsing purposes.
    std::string line;
    std::ostringstream listUtxoOss;
    while (std::getline(listUtxo.out(), line)) {
        listUtxoOss << line << std::endl;
    }

    //Parse the received data as json.
    nlohmann::json listUtxoJson = nlohmann::json::parse(listUtxoOss.str());
    std::string decision;
    size_t UTXOcount = 0;


    //Check whether valid UTXOs exit.
    if (listUtxoJson.empty()) {
        std::cout << rang::fg::gray << rang::style::bold
                  << "No usable unspent outputs exist, the program will automatically generate one linked to address of your choosing."
                  << rang::style::reset << std::endl;
        decision = "generate";
    } else {
        //List valid UTXOs.
        std::cout << rang::fg::gray << rang::style::bold << "Number of usable unspent outputs is ["
                  << listUtxoJson.size() << "]:" << rang::style::reset << std::endl;
        for (; UTXOcount < listUtxoJson.size(); ++UTXOcount) {
            std::cout << rang::fg::green << rang::style::bold << "-------------------------[" << UTXOcount
                      << "]-------------------------" << std::endl << rang::style::reset
                      << listUtxoJson[UTXOcount].dump(15) << std::endl;
        }
        std::cout << rang::fg::green << rang::style::bold << "-----------------------------------------------------"
                  << rang::style::reset << std::endl;
        std::cout << rang::fg::gray << rang::style::bold
                  << "To continue, choose one of the listed inputs to be double-spended or write \"generate\" to generate a new one to address of your choosing: "
                  << rang::style::reset;
        std::cin >> decision;
    }

    //Generate blocks to the specified address until usable UTXO is found. When usable UTXO is found, return it as a asset to double-spend.
    if (decision == "generate") {
        //Choose address to which the blocks are generated and construct command to generate block linked to it.
        std::string address = chooseAddress();
        std::ostringstream oss;
        oss << "bitcoin-cli generatetoaddress 1 " << address << " &> /dev/null";

        //generatedBlocksCount serves as protection against program stunlock. After using network for a long time, generating new block may no longer generate new UTXOs -> In such cases the program ends and urges user to restart the network.
        int generatedBlocksCount = 0;

        //[102] has a specific purpose, [100] blocks is the UTXO maternity age. Mining exactly [102] blocks means, that at least one mature UTXO linked to the address is certainly created.
        while (generatedBlocksCount < 102) {
            //Execute "listunspent" command and save its outputs.
            redi::ipstream listUtxos("bitcoin-cli listunspent", redi::pstreams::pstdout | redi::pstreams::pstderr);

            //Parse the received data as json.
            listUtxoOss.str("");
            while (std::getline(listUtxos.out(), line)) {
                listUtxoOss << line << std::endl;
            }

            nlohmann::json listUtxosJson = nlohmann::json::parse(listUtxoOss.str());

            //Return first valid UTXO.
            for (size_t i = 0; i < listUtxosJson.size(); ++i) {
                if (listUtxosJson[i]["address"] == address) {
                    return transactionInfo(listUtxosJson[i]["amount"], listUtxosJson[i]["vout"],
                                           listUtxosJson[i]["txid"], listUtxosJson[i]["address"]);
                }
            }

            //Generate new block using ipstream to silence the outputs in cmdline. 
            redi::ipstream generateBlock(oss.str().c_str(), redi::pstreams::pstdout | redi::pstreams::pstderr);
            ++generatedBlocksCount;
        }


        //Return an error, network probably no longer generates valid UTXOs.
        std::cerr << rang::fg::red << rang::style::bold
                  << "No usable UTXO was generated, even though it should have. Consider restarting the network to resolve this issue."
                  << rang::style::reset << std::endl;
        exit(1);
    }

    //Check that the input is a number.
    size_t decisionNmbr;
    try {
        decisionNmbr = std::stoi(decision);
    }
    catch (...) {
        std::cerr << rang::fg::red << rang::style::bold << "Unable to interpret the input, existing."
                  << rang::style::reset << std::endl;
        exit(1);
    }

    //Check that the input number is a valid number.
    if (decisionNmbr < UTXOcount) {
        return transactionInfo(listUtxoJson[decisionNmbr]["amount"], listUtxoJson[decisionNmbr]["vout"],
                               listUtxoJson[decisionNmbr]["txid"], listUtxoJson[decisionNmbr]["address"]);
    }

    //If the input number was not a valid number, exit.
    std::cerr << rang::fg::red << rang::style::bold << "The number you entered is invalid, exiting."
              << rang::style::reset << std::endl;
    exit(1);
}

std::pair<std::string, std::string> CParameterPreparator::generateAddresses(const std::string generateAddressFunction1,
                                                                            const std::string generateAddressFunction2) {

    //Issue system calls to generate new addresses.
    redi::ipstream address1Output(generateAddressFunction1, redi::pstreams::pstdout), address2Output(
            generateAddressFunction2, redi::pstreams::pstdout);

    //Save the received outputs.
    std::pair<std::string, std::string> addresses;
    std::getline(address1Output.out(), addresses.first);
    std::getline(address2Output.out(), addresses.second);

    return addresses;
}

std::pair<float, float>
CParameterPreparator::calculateAmmounts(const transactionInfo &UTXO, const float expectedFullFee) {
    float paidAmmount, returnAmmount;

    //Ask for the ammount to pay and wait for the response.
    std::cout << rang::fg::gray << rang::style::bold
              << "Please input the ammount of bitcoins required to pay for the service. The default value is set to [10]: "
              << rang::style::reset;
    std::cin >> paidAmmount;

    //Check whether the chosen output can pay for the required service.
    if ((paidAmmount + expectedFullFee) > UTXO.m_btcValue) {
        std::cerr << rang::fg::red << rang::style::bold
                  << "The chosen UTXO does not have enough value to pay for the requested service, exiting."
                  << rang::style::reset << std::endl;
        exit(1);
    } else {
        returnAmmount = UTXO.m_btcValue - paidAmmount - 0.1;
    }

    return std::make_pair(paidAmmount, returnAmmount);
}

