#include <stdio.h>

int main() {
    // Initialize log
    // Initialize message counter
    // Parse hostfile
    // Init active hosts list
    // Create corresponding sockets
    // Create heartbeat channels
    // Create heartbeat cluster-monitor channels
    // Create manual input channel

    /* Event loop:
     *
     * > Consensus
     *
     * > Log replication
     *
     * > P heartbeat reception
     *      - as result of new P
     *          > update hosts list state
     *              - prevent possibility of DOS
     *              - maybe through checking if P is previous HS ?
     *      > update log entry state if any (one by one ?)
     *      > ack back with entry replication notice if any
     *
     * > History snapshotting and message counter reset
     *
     * > Log operation commit
     *
     * > Log operation pending
     *
     * > Log operation proposition input
    */
    return 0;
}
