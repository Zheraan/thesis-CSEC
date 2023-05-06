#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 2
#endif

#include "overseer.h"
#include "server-events.h"

int main() {
    overseer_s overseer;
    if (overseer_init(&overseer) == EXIT_FAILURE) {
        fprintf(stderr, "Failed to initialize the program state\n");
        exit(EXIT_FAILURE);
    }

    // Initialize event loop
    if (server_reception_init(&overseer) == EXIT_FAILURE || // Initialize the message reception handler event
        server_random_ops_init(&overseer) == EXIT_FAILURE) { // Initialize the random ops generator
        fprintf(stderr, "Failed to initialized the event loop\n");
        overseer_wipe(&overseer);
        exit(EXIT_FAILURE);
    }

    // Run the loop
    int rv = event_base_dispatch(overseer.eb);
    if (rv == 1)
        fprintf(stdout, "No events pending or active in the base\n");
    else if (rv == -1)
        fprintf(stderr, "An error occurred while running the loop\n");

    // Clean program state and close socket
    fprintf(stdout, "Cleaning up and finishing...\n");
    overseer_wipe(&overseer);

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
     *          > send first queued element if any
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
     *      > Create op queue element
     *      > Check if queue is empty
     *      > Queue it up
     *      > Set timer for deletion
     *      - Entry queue was empty when checked
     *          - There is an active P
     *              > cache it
     *              > Send proposition to P
     *              > set timeout for retransmission
     *
     * > Replicate entry order
     *      - Term invalid
     *          - Incoming is lower (should not be possible)
     *              > Ack with right term and special flag
     *          - Local is lower
     *              > Adjust local
     *      - Indexes invalid
     *          > Ask for needed entries if any
     *              > Dequeue all queued ops if necessary
     *          > Fix the incoherences otherwise
     *      - All valid
     *          > create corresponding pending entry
     *          > Ack back
     *          > Transmit first queue element
     *
     * > Log entry proposal ack received
     *      - Special flag for invalid indexes and/or terms
     *          > Dequeue all queued ops if necessary
     *          > Apply fix, ask for next one if any
     *      - Rejected due to "proposed" state
     *          > Do nothing (commit entry order will trigger retransmission of first event in queue)
     *      - Validated
     *          > Make entry pending
     *
     * > Commit entry order
     *      - Term invalid
     *          - Incoming is lower (should not be possible)
     *              > Ack with right term and special flag
     *          - Local is lower
     *              > Adjust local
     *      - Indexes invalid
     *          > Ask for needed entries if any
     *              > Dequeue all queued ops if necessary
     *          > Fix the incoherences otherwise
     *      - All valid
     *          > commit corresponding pending entry
     *          > Ack back
     *          > Apply entry through data op module
     *
     *
     *
     * ------------- Data op queue ------------------------------------------------
     *
     * > Deletion timeout
     *      > Remove all elements from queue to prevent incoherences (subsequent data ops may depend on the head)
     *
     * > Retransmission timeout
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
     * ------------- Network topology ---------------------------------------------
     *
     *
     *      
     *
    */
    return EXIT_SUCCESS;
}
