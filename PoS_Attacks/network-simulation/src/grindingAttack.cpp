#include <cstdlib>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <iterator>
#include <time.h>
#include <unistd.h>
#include <openssl/sha.h>
#include <openssl/evp.h>

#include "../libs/rang.hpp"

//Parameters to not be changed.
#define CHARSET "abcdef1234567890"
#define ADDRESS_SIZE 64
#define CHARSET_SIZE 15

//Parameters to be freely changed.
#define NUMBER_OF_NODES 20
#define NUMBER_OF_TRANSACTIONS_PER_CONSENSUS_ROUND 3


/**
 * Structure representing a single network node.
 */
struct CAccount {
    std::string m_address;
    int m_balance;


    CAccount(const std::string address) {
        m_address = address;
        m_balance = 0;
    }

    void print() {
        std::cout << "===ACCOUNT===" << std::endl
                  << "address: \"" << m_address << "\"" << std::endl
                  << "balance: \"" << m_balance << "\"" << std::endl
                  << "=============" << std::endl;
    }
};

/**
 * Structure representing a single block of the blockchains.
 * Transactions are simulated simply as hexadecimal strings.
 */
struct CBlock {
    std::string m_prevBlockHash;
    std::vector <std::string> m_transactions;

    CBlock(const std::string prevBlockHash, const std::vector <std::string> transactions) {
        m_prevBlockHash = prevBlockHash;
        m_transactions = transactions;
    }

    void print() {
        std::cout << "{" << std::endl
                  << "    \"prevBlockHash\":  " << m_prevBlockHash << std::endl
                  << "    \"transactions\":   [" << std::endl;
        for (const std::string &transaction: m_transactions) {
            std::cout << "                            " << transaction << "," << std::endl;
        }
        std::cout << "                        ]" << std::endl
                  << "}" << std::endl;
    }
};

/**
 * Structure representing the blockchain.
 */
struct CBlockchain {
    std::vector <CBlock> m_blockchain;

    CBlockchain(const CBlock genesis) {
        m_blockchain.push_back(genesis);
    }

    void print() {
        int i = 0;
        for (CBlock block: m_blockchain) {
            std::cout << "Block [" << i << "]" << std::endl;
            block.print();
            ++i;

        }
    }
};

/**
 * Function to generate a random hexadecimal string of specified length.
 * @return Random string of hexadecimal characters.
 */
std::string generateRandomHexString() {
    std::ostringstream oss;
    for (size_t i = 0; i < ADDRESS_SIZE; ++i) {
        oss << CHARSET[std::rand() % CHARSET_SIZE];
    }
    return oss.str();
}

/**
 * Function to generate sha256 hash of specified string.
 * @param stringToHash String to generate the hash of.
 * @return Hexadecimal hash of the received string.
 */
std::string sha256(const std::string stringToHash) {
    //Initialize the openssl objects.
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    const EVP_MD *md = EVP_sha256();
    unsigned int hash_len;
    unsigned char hashOutput[SHA256_DIGEST_LENGTH];

    //Hash the received string.
    EVP_DigestInit_ex(ctx, md, nullptr);
    EVP_DigestUpdate(ctx, stringToHash.c_str(), stringToHash.size());
    EVP_DigestFinal_ex(ctx, hashOutput, &hash_len);

    //Destroy the allocated openssl object.
    EVP_MD_CTX_free(ctx);

    //Convert the received output to hex string.
    std::ostringstream hashOutputOss;
    for (size_t i = 0; i < hash_len; ++i) {
        hashOutputOss << std::setfill('0') << std::setw(2) << std::hex << (int) hashOutput[i];
    }

    return hashOutputOss.str();
}

/**
 * Get hash of the specified CBlock object.
 * @param block CBlock to generate hash of.
 * @return Hash of the block as a hex string.
 */
std::string getBlockHash(CBlock block) {
    //Parse the block into a string.
    std::ostringstream stringToHash;

    stringToHash << block.m_prevBlockHash;

    for (const std::string &transaction: block.m_transactions) {
        stringToHash << transaction;
    }

    return sha256(stringToHash.str());
}

/**
 * Function to generate random block by a specified account.
 * @param account Account that is mining the block. Reward for validation is simulated as incrementing of the m_balance property.
 * @param blockchain Blockchain to append block to.
 * @param memPool Mempool with all the possible transactions to be embedded into a block.
 */
void generateRandomBlock(CAccount &account, CBlockchain &blockchain, std::vector <std::string> &memPool) {
    //Generate one new transaction hash representing coinbase transaction and choose random number of transactions from memPool to be embedded into a block.
    std::vector <std::string> embeddedTransactions({generateRandomHexString()});
    int nmbrOfTransactions = std::rand() % NUMBER_OF_TRANSACTIONS_PER_CONSENSUS_ROUND;
    while (nmbrOfTransactions == 0) {
        nmbrOfTransactions = std::rand() % NUMBER_OF_TRANSACTIONS_PER_CONSENSUS_ROUND;
    }

    //Embed [nmbrOfTransactions] random transactions into a block. Delete them from the mempool.
    for (int i = 0; i < nmbrOfTransactions && memPool.size() != 0; ++i) {
        std::vector<std::string>::iterator randIt = memPool.begin();;
        std::advance(randIt, std::rand() % memPool.size());
        embeddedTransactions.emplace_back(*randIt);
        memPool.erase(randIt);
    }

    //Add reward to the validator and append the blockchain with the newly created block.
    account.m_balance += 1;
    blockchain.m_blockchain.push_back(
            CBlock(getBlockHash(blockchain.m_blockchain[blockchain.m_blockchain.size() - 1]), embeddedTransactions));
}

/**
 * Delete [n] random addresses from the stakePool and generate [n] new ones. [n] is chosen randomly.
 * @param stakePool Vector that the operation is conducted above.
 */
void shuffleStakePool(std::vector <CAccount> &stakePool) {
    //Delete random number of accounts, but do not delete attacker.
    int shuffleNumber = std::rand() % NUMBER_OF_NODES;
    for (int i = 0; i < shuffleNumber; ++i) {
        int popRandom = std::rand() % stakePool.size();
        while (popRandom == 0) {
            popRandom = std::rand() % stakePool.size();
        }

        std::vector<CAccount>::iterator it = stakePool.begin();

        std::advance(it, popRandom);
        stakePool.erase(it);
    }

    //Create same number of new accounts.
    for (int i = 0; i < shuffleNumber; ++i) {
        stakePool.push_back(CAccount(generateRandomHexString()));
    }
}

/**
 * Count the new validator. To properly understand the consensus process, please read README.md. The index of the selected user is counted as: (first_32bits_of(account) + for_each_address_in_stakePool(first_32bits_of(m_address))) % NUMBER_OF_NODES
 * @param lastBlockHash Hash of the last block in the blockchain.
 * @param stakePool Pool with the accounts participating in the last consensus round.
 * @return Index of the selected block validator.
 */
size_t countValidator(const std::string lastBlockHash, const std::vector <CAccount> &stakePool) {
    unsigned int validator = 0, x;

    //Convert first 32 bits of lastBlockHash into u_int and modulate it. Will overflow and break the attack on architectures that implement u_int as 16-bit instead of 32-bit.
    sscanf(lastBlockHash.substr(0, 16).c_str(), "%x", &x);
    validator = x % NUMBER_OF_NODES;

    //Convert first 32 bits of each address into u_int and modulate it. Will overflow and break the attack on architectures that implement u_int as 16-bit instead of 32-bit.
    for (size_t i = 0; i < stakePool.size(); ++i) {
        sscanf(stakePool[i].m_address.substr(0, 16).c_str(), "%x", &x);
        validator += x % NUMBER_OF_NODES;
    }

    //Return result.
    return validator % NUMBER_OF_NODES;
}

/**
 * Process of grinding for a block that guarantees attacker win of the next consensus round. Expects that the attacker is at index [0].
 * Implementation grinds trough permutations of the transactions to generate a fitting block hash. Further enhancements may include attacker grinding trough permutations of possible combinations of transactions. For simulation purposes, such approach would bring unnecessary complexity.
 * If attacker fails to generate a winning block, they generate a standard block.
 * @param blockchain Blockchain to grind above.
 * @param currentStakePool currentStakePool will be used as input for the next validator selection. -> We want to use it for prediction.
 * @param memPool memPool with transactions.
 */
void
grind(CBlockchain &blockchain, const std::vector <CAccount> &currentStakePool, std::vector <std::string> &memPool) {
    bool successfullGrind = false;

    std::cout << rang::fg::blue << rang::style::bold << "Attacker has [" << memPool.size() + 1
              << "] transactions in their mempool. They can grind trough " << memPool.size() + 1 << "! permutations."
              << std::endl;

    //Prepare a transaction pool to grind above. Add one extra hex string to simulate a coinbase transaction.
    std::vector <std::string> memPoolCopy = memPool;
    memPoolCopy.push_back(generateRandomHexString());
    std::sort(memPoolCopy.begin(), memPoolCopy.end());

    size_t i = 0;
    do {
        std::cout << rang::fg::blue << rang::style::bold << "Attacker is trying permutation [" << i << "]."
                  << std::endl;
        //Create block with permutated set of transactions and check it against the desired selection.
        CBlock proposedBlock(getBlockHash(blockchain.m_blockchain[blockchain.m_blockchain.size() - 1]), memPoolCopy);
        if (countValidator(getBlockHash(proposedBlock), currentStakePool) == 0) {
            std::cout << rang::fg::blue << rang::style::bold << "Attacker found good block hash -> "
                      << rang::fg::green << "They are guaranteed to win the next consensus round!" << rang::fg::reset
                      << std::endl;
            //Append block to the blockchain.
            blockchain.m_blockchain.push_back(proposedBlock);
            successfullGrind = true;
            break;
        }
        ++i;
    } while (std::next_permutation(memPoolCopy.begin(), memPoolCopy.end()));

    //If no fitting block was found, create block with the last permutation of transactions.
    if (!successfullGrind) {
        std::cout << "Attacker has not found a block hash to ensure win of the next consensus round." << std::endl;
        blockchain.m_blockchain.push_back(
                CBlock(getBlockHash(blockchain.m_blockchain[blockchain.m_blockchain.size() - 1]), memPoolCopy));
    }
}

int main() {
    //Initiate random.
    std::srand(std::time(nullptr));

    //Generate random accounts. Attacker is the one at the index [0].
    std::vector <CAccount> oldStakePool, currentStakePool;
    for (size_t i = 0; i < NUMBER_OF_NODES; ++i) {
        currentStakePool.push_back(generateRandomHexString());
    }

    //Create the blockchain with the genesis block.
    CBlockchain blockchain(CBlock("0000000000000000000000000000000000000000000000000000000000000000", {}));

    //Create a memPool of random txids symbolizing the traffic within the network.
    std::vector <std::string> memPool;
    for (size_t i = 0; i < NUMBER_OF_TRANSACTIONS_PER_CONSENSUS_ROUND; ++i) {
        memPool.push_back(generateRandomHexString());
    }

    //Ask for specific number of consensus rounds.
    size_t consensusRounds;
    std::cout << rang::style::bold << rang::fg::gray << "Please input the number of consensus rounds: "
              << rang::style::reset;
    std::cin >> consensusRounds;

    //Count number of validated blocks.
    int attackerTotal = 0, networkTotal = 0;

    //Simulate consensus for the specified number of rounds.
    for (size_t passedRounds = 0; passedRounds < consensusRounds; ++passedRounds) {
        std::cout << rang::style::bold << rang::fg::gray << "=======================================" << std::endl
                  << "Starting [" << passedRounds << ".] consensus round." << std::endl
                  << "=======================================" << rang::style::reset << std::endl;

        //Execute function to select next block validator.
        size_t validator = countValidator(getBlockHash(blockchain.m_blockchain[blockchain.m_blockchain.size() - 1]),
                                          oldStakePool);

        //If validator is attacker, grind. Else, generate a randomized block.
        if (validator == 0) {
            std::cout << rang::fg::green << rang::style::bold << "Attacker was selected as the block validator."
                      << rang::style::reset << std::endl;
            ++attackerTotal;
            grind(blockchain, currentStakePool, memPool);
        } else {
            std::cout << rang::fg::red << rang::style::bold << "Random node was selected as a block validator."
                      << std::endl << rang::fg::magenta << "Selected block validator is creating a randomized block."
                      << rang::style::reset << std::endl;
            ++networkTotal;
            generateRandomBlock(currentStakePool[validator], blockchain, memPool);
        }

        //Save the state of the current mempool to use it as random input for the next consensus round.
        oldStakePool = currentStakePool;

        //Shuffle current mempool to simulate random users staking within the network.
        shuffleStakePool(currentStakePool);

        //Create new transaction ids to simulate network flow.
        while (memPool.size() < NUMBER_OF_TRANSACTIONS_PER_CONSENSUS_ROUND) {
            memPool.push_back(generateRandomHexString());
        }

        //Sleep to make output more readable.
        sleep(1);
    }

    //Print the results. Attack is deemed successful if the attacker managed to validate more blocks than the rest of the network.
    if (attackerTotal > networkTotal) {
        std::cout << rang::fg::green << rang::style::bold << "Attack successful!" << std::endl;
    } else {
        std::cout << rang::fg::red << rang::style::bold << "Attack unsuccessful!" << std::endl;
    }
    std::cout << rang::fg::blue << rang::style::bold << "Attacker" << rang::fg::gray << " has created ["
              << rang::fg::blue << attackerTotal << rang::fg::gray << "] blocks, while " << rang::fg::magenta
              << "rest of the network" << rang::fg::gray << " has created [" << rang::fg::magenta << networkTotal
              << rang::fg::gray << "] blocks." << std::endl;
}
