// TODO Extension Insert file description, copyright, etc

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 4
#endif

#include "lib-csec.h"
#include "server-events.h"

int main() {
    debug_log(1, stdout, "Starting program state initialization ...\n");

    if ((TIMEOUT_VALUE_PROP_RETRANSMISSION_SEC * 1000000 + TIMEOUT_VALUE_PROP_RETRANSMISSION_USEC)
        * PROPOSITION_RETRANSMISSION_DEFAULT_ATTEMPTS >
        TIMEOUT_VALUE_PROPOSITION_SEC * 1000000 + TIMEOUT_VALUE_PROPOSITION_USEC) {
        debug_log(0, stdout, "Warning: timeout for queued propositions is smaller than the timeout"
                             "for retransmissions multiplied by the number of retransmission attempts. This may cause "
                             "propositions to be sent in a different order than they are sent. This will be guarded "
                             "against in future versions of this program.\n");
        // TODO Optimization: guard against the above
    }

    // Initialize program state
    overseer_s overseer;
    if (overseer_init(&overseer) != EXIT_SUCCESS) {
        fprintf(stderr, "Failed to initialize the program state\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }
    debug_log(1, stdout, "Done.\nStarting event loop initialization ...\n");

    // Initialize event loop
    if (cm_reception_init(&overseer) != EXIT_SUCCESS || // Initialize the message reception handler event
        server_random_ops_init(&overseer) != EXIT_SUCCESS || // Initialize the random ops generator
        etr_reception_init(&overseer) != EXIT_SUCCESS) // Initialize the entry transmission event
    {
        fprintf(stderr, "Failed to initialize the event loop\n");
        overseer_wipe(&overseer);
        exit(EXIT_FAILURE);
    }
    debug_log(1, stdout, "Done.\nLaunching raft-server.\n");

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
    // Initialize P-term with special start value
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
     *      - as result of new P (check P-term)
     *          > update hosts list state
     *              - prevent possibility of DOS
     *              - maybe through checking if P is previous HS ?
     *          > send first queued element if any
     *      > Check P-term
     *          > Ack with correct P-term and who is P if wrong
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
     *              > Ack with right P-term and special flag
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
     *              > Ack with right P-term and special flag
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
