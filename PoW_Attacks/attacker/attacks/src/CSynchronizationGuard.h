#include <cstdlib>

#include "../libs/pstream.h"
#include "../libs/json.hpp"

#pragma once

#define DELAY_SEC 2

/**
 * Class to guard connection and transaction synchronization.
 * Effectively functioning as network-based mutex.
 */
class CSynchronizationGuard {
public:
    /**
     * Default constructor.
     */
    CSynchronizationGuard() = default;

    /**
     * Wait for the disconnection of specified nodes in the network.
     * Due to the fixed number of devices in the network ([3]), we can check disconnection simply by checking an expected number of connected nodes at given time.
     */
    void waitForDisconnection(size_t expectedAttackerSize, size_t expectedVictim1Size, size_t expectedVictim2Size);

    /**
     * Checks that exactly two nodes (and no other!) are connected in the network. If third node was to be added in any manner, this check might fail.
     * VERY FRAGILE IMPLEMENTATION!
     */
    void waitForConnection(const std::string function1, const std::string function2);

    /**
     * Wait until raw transaction is listed in specified mempool.
     */
    void waitForRawTxDelivery(const std::string function, const std::string txid);

    /**
     *Wait until raw transaction is listed in a specified wallet.
     */
    void waitForTxDelivery(const std::string function, const std::string txid);
};
