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
     * ------------- Heartbeats ---------------------------------------------------
     *
     * > P heartbeat reception
     *      - as result of new P (check term)
     *          > update hosts list state
     *              - prevent possibility of DOS
     *              - maybe through checking if P is previous HS ?
     *          > send queued element if any
     *      > Check term
     *          > Ack with correct term and who is P if wrong
     *      > check match index
     *          > update if out of date
     *      > check next index
     *          > if it doesn't match, log entries are missing
     *          > ask for next missing log entry (include replication index)
     *      > update log entry state if any (one by one ?)
     *      > Check, hosts list if there is, flags, status
     *          > update hosts if need be
     *      > ack back with log entry replication notice if any
     *      > Normal ack back if nothing was wrong
     *      > Reset heartbeat timeout
     *
     * > Receive special message from outdated masters on who is P
     *      - May happen in case local node sent a message to a node that is no longer P
     *      > Update hosts list
     *
     * > Missing log entry received
     *      > Update log accordingly
     *      > Check if next index matches
     *          > If not, ask for next missing log entry (include pending index)
     *
     *
     *
     * ------------- Log entries --------------------------------------------------
     *
     * > Data Op reception
     *      > Create entry with created status
     *      - Entry queue not empty
     *          > queue it up
     *      - Entry queue empty
     *          > Keep it cached
     *          - There is an active P
     *              > Send proposition to P
     *          - There is no P
     *              > Queue it up with no P flag
     *              > Set timeout for deletion but not for retransmission
     *
     * > Replicate entry order
     *      - Term invalid
     *          > Ack with right term
     *      - Indexes invalid
     *          > Ask for needed entries if any
     *          > Fix the incoherences otherwise
     *      - All valid
     *          > create corresponding pending entry
     *          > Ack back
     *
     * > Log entry proposal ack received
     *      - Special flag for invalid indexes and/or terms
     *          > Abandon created entry
     *          > Apply fix, ask for next one if any
     *      - Rejected due to "proposed" state
     *          > Queue the cached proposition
     *      - Validated
     *          > Make entry pending
     *
     *
     *
     * ------------- History snapshotting and message counter reset ---------------
     *
     *
     *
     * ------------- Manual input channel -----------------------------------------
     *
     *
     *
     * ------------- Log application ----------------------------------------------
     *
     *
     *
     * ------------- Entry queue --------------------------------------------------
     *
     * > Deletion timeout
     *
     * > Retransmission timeout
     *
     *
     *
     * ------------- Network topology ---------------------------------------------
     *
     *
     *
     *
     *
     * > Consensus
     *
     * > Log replication
     *
     *
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
