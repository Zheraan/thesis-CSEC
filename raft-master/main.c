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
     *                  ? Rollback to CS if still unreachable after several attempts
     *
     * > Transition to P
     *      > transitions to P status, sends heartbeat as new P
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
     *      > check term, ignore if lower than current
     *      > otherwise respond favorably
     *          - find solution to unlimited bid requests, possibility of DOS
     *      > set term timeout
     *
     * > HS election
     *      - as result of old HS takeover as P (trigger through callback of heartbeat)
     *      - because of deployment phase, trigger through setup
     *      - as result of HS heartbeat timeout
     *          > update hosts state
     *      > enter candidate state, increase term and vote for self
     *      > send bid to all, set term timeout
     *          > receives votes from majority before bid timeout
     *              > transitions to HS, sends heartbeat as new HS
     *                  > remove any CS related events
     *                  > create HS related events
     *                  > remove CS-side backup channels
     *                  > create HS-side backup channels
     *          > if it doesn't get majority, try reaching again unreachable nodes
     *          > otherwise starts new term at timeout
     *
     *
     *
     * ------------- P Handling of log entry proposition --------------------------
     *
     * > Receive log entry proposal
     *      - indexes and term are invalid
     *          > heartbeat back with correct indexes and terms
     *      - indexes and term are valid
     *          > reject if under proposed state
     *          > otherwise ack proposition
     *          > enter proposed state and set timeout
     *          > create pending proposition, and broadcast to servers for voting
     *
     * > Timeout of ongoing election ("proposed state")
     *      > Check majority of votes
     *          - Reached
     *              > Commit locally and send commit order
     *      > Exit "proposed" state
     *
     * > Receive replication Ack
     *
     * > Log operation voting timeout ("proposed" state)
     *
     * > Log operation commit
     *
     * > Log operation pending
     *
     *
     *
     * ------------- Heartbeats and log repair ------------------------------------
     *
     * > Periodic standard heartbeat
     *      > Include:
     *          - Indexes
     *          - Terms
     *          - Hosts list and their status
     *
     * > Heartbeat reception
     *      - as result of new P/HS
     *          > update hosts list state
     *          > trigger election of new HS if old one took over
     *      - new host special flag
     *          > update hosts list & state
     *          > ack back with log data and full hosts list
     *      - if from other node operating in the same mode
     *          > Compare terms and step down from role if necessary
     *      > compare terms and indexes
     *          - if indexes wrong
     *              >heartbeat ack with missing log entry message
     *      > compare hosts lists
     *          > adjust if necessary
     *      > send indexes
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
    */
    return 0;
}
