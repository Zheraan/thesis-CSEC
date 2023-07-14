// TODO Extension Insert file description, copyright, etc

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 4
#endif

#include "lib-csec.h"
#include "master-events.h"

int main() {
    debug_log(1, stdout, "Starting program state initialization ...\n");
    // Initialize program state
    overseer_s overseer;
    if (overseer_init(&overseer) != EXIT_SUCCESS) {
        fprintf(stderr, "Failed to initialize the program state\n");
        exit(EXIT_FAILURE);
    }
    debug_log(1, stdout, "Done.\nStarting event loop initialization ...\n");

    // TODO Test remove test statement
    overseer.hl->hosts[overseer.hl->localhost_id].status = HOST_STATUS_P;

    // Initialize event loop
    // TODO Test Remove heartbeat to have it on only initialized when transitioning to HS (and thus it remains for P)
    //  then remove when demoted to CS
    if (master_heartbeat_init(&overseer) != EXIT_SUCCESS || // Initialize heartbeat events
        cm_reception_init(&overseer) != EXIT_SUCCESS || // Initialize control message reception events
        p_liveness_set_timeout(&overseer) != EXIT_SUCCESS || // Initialize P liveness check
        etr_reception_init(&overseer) != EXIT_SUCCESS) // Initialize the entry transmission event
    {
        fprintf(stderr, "Failed to initialized the event loop\n");
        overseer_wipe(&overseer);
        exit(EXIT_FAILURE);
    }
    debug_log(1, stdout, "Done.\nLaunching raft-master.\n");

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
    // Create consensus lead channels
    // Create elections channels
    // Create control channel
    // Create close sync channels
    // Create CS-side backup channels

    /* Events:
     *
     * ------------- P Takeover ---------------------------------------------------
     *
     * > P heartbeat timeout as HS
     *      > Transition to P candidate
     *      > Queries other masters for reachability and for who is master with P-term number
     *      > Set query timeout
     *
     * > Reachability query timeout (as P candidate mode)
     *      > Check if queries indicate right P-term and right master
     *          > Step down if local terms are outdated compared to the majority of answers
     *          > Update hosts and indexes accordingly
     *      ? transition to partition mode if received less than half queries back
     *      - Majority of negative reachability
     *          ?! transitions to P status
     *      - Majority of positive reachability
     *          > rollback to HS
     *
     * > Reachability query reception as CS
     *      - Term is outdated
     *          > Ack back with hosts list and terms
     *      > Ack back
     *      > Query master for reachability
     *
     * > Reachability query reception as P
     *      > Ack back with special flag
     *
     * > Reachability query answer reception as HS
     *      > Update with result
     *          - Majority attained: unreachable
     *              ! Transition to P
     *          - Majority attained: reachable
     *              > Rollback to HS, query P for reachability
     *                  ? Partition mode if still unreachable after several attempts
     *
     * > Transition to P
     *      > transitions to P status, sends heartbeat as new P with special flag
     *          > remove any HS or CS related events
     *          > create P related events
     *          > Set non-proposed state
     *          > remove HS-side backup channels
     *
     *
     *
     * ------------- HS Election --------------------------------------------------
     *
     * > HS election bid reception
     *      > check P-term, ack back with hosts status and terms if lower than current
     *      > otherwise respond favorably
     *          X find solution to unlimited bid requests with increasing P-term numbers, possibility of DOS
     *      > set P-term timeout
     *
     * > HS election
     *      - as result of old HS takeover as P (trigger through callback of heartbeat)
     *      - because of deployment phase, trigger through setup
     *      - as result of HS heartbeat timeout
     *          > update hosts state
     *      > enter candidate state, increase P-term and vote for self
     *      > send bid to all, set P-term timeout
     *          > otherwise starts new P-term at timeout
     *
     * > Election bid answer
     *      > Check P-term, ignore if wrong
     *      - positive
     *          > increase positive vote count
     *          - If majority reached
     *              > transitions to HS, sends heartbeat as new HS
     *                  > remove any CS related events
     *                  > create HS related events
     *                  > remove CS-side backup channels
     *                  > create HS-side backup channels
     *      - negative
     *          > increase negative vote count
     *
     * > HS-term timeout
     *      - first time
     *          > try reaching again unreachable nodes
     *          > reset timeout
     *      - second time
     *          ! HS election
     *      X Find a way to identify when partitioned (maybe when receiving no ack back?)
     *
     *
     *
     * ------------- P Handling of log entry proposition --------------------------
     *
     * > Receive log entry proposal
     *      - indexes and P-term are invalid
     *          > heartbeat back with correct indexes and terms and special flag
     *          > send the oldest log entry to be repaired if any
     *      - indexes and P-term are valid
     *          - if under proposed state
     *              > ack back with negative answer
     *          - otherwise
     *              > ack proposition
     *              > enter proposed state and set timeout
     *              > create pending proposition, and broadcast to servers for voting (replicate_entry)
     *
     * > Timeout of ongoing election ("proposed state")
     *      > Check majority of votes
     *          - Reached
     *              > Check if entry is marked as safe
     *                  - Yes
     *                      > Commit locally
     *                      > send commit order
     *                      > Set timer for commit ack
     *          - Not reached
     *              > Reiterate update to servers not responding
     *      > Exit "proposed" state
     *
     * > Receive replication Ack
     *      > Verify indexes
     *      > Update replication status
     *      > Check majority of votes
     *          - Reached
     *              > Check if entry is marked as safe
     *                  - Yes
     *                      > Commit locally
     *                      > send commit order
     *                      > Set timer for commit ack
     *      > Exit "proposed" state
     *
     * > Log entry commit ack
     *      > update commit data
     *      > remove retransmission event
     *
     * > Log entry safe message from HS
     *      > mark entry as safe
     *      > check majority status on entry
     *          - Reached
     *              > Broadcast commit order
     *
     *
     *
     * ------------- HS and CS entry replication ----------------------------------
     *
     * > Replicate entry order
     *      - Term invalid
     *          > Ack with right P-term
     *      - Indexes invalid
     *          > Ask for needed entries if any
     *              - from P if HS
     *              - from HS if CS
     *          > Fix the incoherences otherwise
     *      - All valid
     *          > create corresponding pending entry
     *              - If local is HS
     *                  > Set entry's master pool replication count to 1
     *          > Ack back
     *
     * > Commit entry order
     *      - Term invalid
     *          > Ack with right P-term
     *      - Indexes invalid
     *          > Ask for needed entries if any
     *              - from P if HS
     *              - from HS if CS
     *          > Fix the incoherences otherwise
     *      - All valid
     *          > commit corresponding pending entry
     *          > Ack back
     *
     * > Replicate entry ack (as HS from CS)
     *      > Increase entry's master pool replication count
     *          - Majority attained
     *              > Mark as safe
     *              > Signify entry is safe to P
     *
     *
     *
     * ------------- Heartbeats and log repair ------------------------------------
     *
     * > Periodic standard heartbeat (P to servers and HS, HS to CS)
     *      > Include:
     *          - Provenance
     *          - Flags (status, special flags)
     *          - Indexes
     *          - Terms
     *          ? Hosts list and their status
     *
     * > Heartbeat reception
     *      - as result of new P (special flag)
     *          > update hosts list state
     *          ! HS election
     *      - as result of new HS (special flag)
     *          > update hosts list state
     *      - new host special flag (recovery from crash or at startup)
     *          > update hosts list & state
     *          > ack back with log data and full hosts list
     *      - if from other node operating in the same mode (P or HS)
     *          ? Compare terms and step down from role if necessary
     *          - Shouldn't happen with partition mode, though important for security
     *      - If in P candidate mode
     *          >rollback to HS
     *      > compare terms and indexes
     *          - if indexes wrong
     *              > heartbeat ack with missing log entry message
     *                  - possibility of erroneous log entries, check P-term from entries past match index
     *              > update accordingly
     *      > compare hosts lists
     *          > adjust if necessary
     *      > ack back with eventual log status changes
     *
     * > Heartbeat Ack reception
     *      > Update log entry replication if any
     *          > check if commit status makes an entry safe or reach majority
     *              - trigger related events if so
     *      - Contains missing log entry message
     *          ? Check for coherence with match index
     *          > Send corresponding entry with current indexes
     *
     * > Heartbeat reception from P as HS
     *      > Reset heartbeat timeout
     *      > Ack back
     *
     * > HS and CS: if receive message with wrong P-term addressed to P with update on who is P
     *      > Heartbeat back with update on who is P
     *
     * > Heartbeat reception from HS as CS
     *      > Check
     *      > Reset heartbeat timeout
     *
     * > Heartbeat reception from P as CS
     *      > Update hosts list
     *      > Special flag if queried from reachability
     *          > Answer HS query with positive reachability
     *
     *
     *
     * ------------- P-HS close sync ----------------------------------------------
     *
     *
     *
     *
     * ------------- HS-CS backup sync --------------------------------------------
     *
     *
     *
     * ------------- History snapshotting and message counter reset ---------------
     *
     *
     *
     * ------------- Partition mode -----------------------------------------------
     *
     * ? Transition back to CS
     * ? Reset P-term to special partition value (>init value, <any online value)
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
    */
    return EXIT_SUCCESS;
}
