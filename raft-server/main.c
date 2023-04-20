#include <stdio.h>

int main() {
    // Initialize log
    // Initialize term with special start value
    // Parse hostfile
    // Init active hosts list and state
    // Create corresponding sockets
    // Create heartbeat channels
    // Create heartbeat cluster-monitor channels
    // Create manual input channel

    /* Event loop:
     *
     * > Receive special message from outdated masters on who is P
     *      > Update hosts list
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
     *      > check match index
     *          > update if out of date
     *      > check next index
     *          > if it doesn't match, log entries are missing
     *          > ask for next missing log entry (include pending index)
     *      > update log entry state if any (one by one ?)
     *      > ack back with log entry replication notice if any
     *      > Reset heartbeat timeout
     *
     * > Missing log entry received
     *      > Update log accordingly
     *      > Check if next index matches
     *          > If not, ask for next missing log entry (include pending index)
     *
     * > History snapshotting and message counter reset
     *
     * > Log entry commit
     *
     * > Log entry pending
     *
     * > Log entry proposition input
     *      > Send request with term and message numbers
     *      
     * > 
    */
    return 0;
}
