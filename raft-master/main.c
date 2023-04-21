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
     *      > Queries other masters for reachability and for who is master with term number
     *      > Set query timeout
     *
     * > Reachability query timeout (as P candidate mode)
     *      > Check if queries indicate right term and right master
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
     *      > check term, ack back with hosts status and terms if lower than current
     *      > otherwise respond favorably
     *          X find solution to unlimited bid requests with increasing term numbers, possibility of DOS
     *      > set term timeout
     *
     * > HS election
     *      - as result of old HS takeover as P (trigger through callback of heartbeat)
     *      - because of deployment phase, trigger through setup
     *      - as result of HS heartbeat timeout
     *          > update hosts state
     *      > enter candidate state, increase term and vote for self
     *      > send bid to all, set term timeout
     *          > otherwise starts new term at timeout
     *
     * > Election bid answer
     *      > Check term, ignore if wrong
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
     * > HS term timeout
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
     *      - indexes and term are invalid
     *          > heartbeat back with correct indexes and terms and special flag
     *          > send the oldest log entry to be repaired if any
     *      - indexes and term are valid
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
     *              > Commit locally and send commit order
     *              > Set commit timeout
     *          - Not reached
     *              > Reiterate update to servers not responding
     *      > Exit "proposed" state
     *
     * > Receive replication Ack
     *      > Verify indexes
     *      > Update replication status
     *      > Check majority of votes
     *          - Reached
     *              X Edge case error possible: only send commit order if HS and the majority of CS have it replicated too
     *                  - may stunt performance in favor of coherence
     *              > Commit locally and send commit order
     *              > Set timer for commit ack
     *      > Exit "proposed" state
     *
     * > Log operation commit ack
     *      > update commit data
     *      > remove retransmission event
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
     *          - Hosts list and their status
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
     *      > compare terms and indexes
     *          - if indexes wrong
     *              > heartbeat ack with missing log entry message
     *                  - possibility of erroneous log entries, check term from entries past match index
     *              > update accordingly
     *      > compare hosts lists
     *          > adjust if necessary
     *      > ack back with eventual log status changes
     *
     * > Heartbeat Ack reception
     *      > Update log entry replication if any
     *      - Contains missing log entry message
     *          ? Check for coherence with match index
     *          > Send corresponding entry with current indexes
     *
     * > Heartbeat reception from P as HS
     *      > If in P candidate mode, rollback to HS
     *      > Reset heartbeat timeout
     *      > Ack back
     *
     * > HS and CS: if receive message with wrong term addressed to P with update on who is P
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
     * X Edge case necessity: When entry is replicated by majority of CS, send special message to allow for commitment
     *      - may stunt performance in favor of coherence
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
     * ? Reset term to special partition value (>init value, <any online value)
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
    return 0;
}
