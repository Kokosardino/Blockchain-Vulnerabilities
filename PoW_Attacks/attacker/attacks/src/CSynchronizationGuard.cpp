#include "CSynchronizationGuard.h"

void CSynchronizationGuard::waitForDisconnection(size_t expectedAttackerSize, size_t expectedVictim1Size,
                                                 size_t expectedVictim2Size) {
    bool connectedFlag = true;
    while (connectedFlag) {
        //Get info about connected nodes.
        redi::ipstream disconnectAttackerOutput("bitcoin-cli getaddednodeinfo",
                                                redi::pstream::pstdout), disconnectVictim1Output(
                "ssh victim1@$IP_VICTIM1 \"bitcoin-cli getaddednodeinfo\"",
                redi::pstream::pstdout), disconnectVictim2Output(
                "ssh victim2@$IP_VICTIM2 \"bitcoin-cli getaddednodeinfo\"", redi::pstream::pstdout);
        std::ostringstream disconnectAttackerOss, disconnectVictim1Oss, disconnectVictim2Oss;
        std::string line;

        //Parse the output as JSON (outputs from ipstream do not necessrilly have the same length).
        while (std::getline(disconnectAttackerOutput.out(), line)) {
            disconnectAttackerOss << line;
        }

        while (std::getline(disconnectVictim1Output.out(), line)) {
            disconnectVictim1Oss << line;
        }

        while (std::getline(disconnectVictim2Output.out(), line)) {
            disconnectVictim2Oss << line;
        }

        nlohmann::json disconnectAttackerJson = nlohmann::json::parse(
                disconnectAttackerOss.str()), disconnectVictim1Json = nlohmann::json::parse(
                disconnectVictim1Oss.str()), disconnectVictim2Json = nlohmann::json::parse(disconnectVictim2Oss.str());

        //ATTACKER must be connected exactly to [expectedAttackerSize], VICTIM1 must be connected exactly to [expectedVictim1Size] and VICTIM2 must be connected exactly to [expectedVictim2Size] to break the cycle.
        if (disconnectAttackerJson.size() == expectedAttackerSize &&
            disconnectVictim1Json.size() == expectedVictim1Size &&
            disconnectVictim2Json.size() == expectedVictim2Size) {
            connectedFlag = false;
        }

        //Sleep so we don't overload the OS.
        sleep(DELAY_SEC);
    }
}

void CSynchronizationGuard::waitForConnection(const std::string function1, const std::string function2) {
    bool notConnectedFlag = true;
    while (notConnectedFlag) {
        //Get information about added nodes.
        redi::ipstream connectNode1Output(function1, redi::pstream::pstdout), connectNode2Output(function2,
                                                                                                 redi::ipstream::pstdout);
        std::ostringstream connectNode1Oss, connectNode2Oss;
        std::string line;

        //Parse output as JSON.
        while (std::getline(connectNode1Output.out(), line)) {
            connectNode1Oss << line;
        }

        while (std::getline(connectNode2Output.out(), line)) {
            connectNode2Oss << line;
        }

        nlohmann::json connectNode1Json = nlohmann::json::parse(
                connectNode1Oss.str()), connectNode2Json = nlohmann::json::parse(connectNode2Oss.str());

        //Check that the both nodes are properly connected to at least one other node (it can be only the desired ones).
        if (!connectNode1Json.empty() && !connectNode2Json.empty() && connectNode1Json[0]["connected"] == true &&
            connectNode2Json[0]["connected"] == true) {
            notConnectedFlag = false;
        }

        //Sleep so we don't overload the OS.
        sleep(DELAY_SEC);
    }

}

void CSynchronizationGuard::waitForRawTxDelivery(const std::string function, const std::string txid) {
    bool txReceived = false;
    while (!txReceived) {
        //Get information about transactions on one of the victims.
        redi::ipstream txReceivedOutput(function, redi::pstream::pstdout);
        std::ostringstream txReceivedOss;
        std::string line;

        //Parse output as JSON.
        while (std::getline(txReceivedOutput.out(), line)) {
            txReceivedOss << line;
        }

        nlohmann::json txReceiveJson = nlohmann::json::parse(txReceivedOss.str());

        //For each listed transaction, check if transaction ID equals the searched transaction ID.
        for (size_t i = 0; i < txReceiveJson.size(); ++i) {
            if (txReceiveJson[i] == txid) {
                txReceived = true;
            }
        }

        //Sleep so we don't overload the OS.
        sleep(DELAY_SEC);
    }

}

void CSynchronizationGuard::waitForTxDelivery(const std::string function, const std::string txid) {
    bool txReceived = false;
    while (!txReceived) {
        //Get information about transactions on one of the victims.
        redi::ipstream txReceivedOutput(function, redi::pstream::pstdout);
        std::ostringstream txReceivedOss;
        std::string line;

        //Parse output as JSON.
        while (std::getline(txReceivedOutput.out(), line)) {
            txReceivedOss << line;
        }

        nlohmann::json txReceiveJson = nlohmann::json::parse(txReceivedOss.str());

        //For each listed transaction, check if transaction ID equals the searched transaction ID.
        for (size_t i = 0; i < txReceiveJson.size(); ++i) {
            if (txReceiveJson[i]["txid"] == txid) {
                txReceived = true;
            }
        }

        //Sleep so we don't overload the OS.
        sleep(DELAY_SEC);
    }
}

