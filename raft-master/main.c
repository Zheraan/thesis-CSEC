#include <stdio.h>

int main() {
    // Initialize log
    // Initialize message counter
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

    /* Event loop:
     *
     * > HS and CS: if receive message with wrong term addressed to P, reply with update on who is P
     *
     * > Receive log entry proposal
     *      > check if message counter and term are valid, heartbeat back if invalid (with correct indexes)
     *          > check if no entry is under vote ("proposed" state)
     *              > reject if so
     *      > otherwise broadcast to servers for voting, and make it pending
     *
     * > Consensus
     *      >
     *
     * > Heartbeat reception
     *      - as result of new P/HS
     *          > update hosts list state
     *          > trigger election of new HS if old one took over
     *      > send current term and message counter (if from P)
     *      > Send who is P (and who is HS ?)
     *      > send indexes
     *
     * > Heartbeat Ack reception
     *      > from server : update log entry replication if any
     *      - Missing log entry message reception
     *          ? Check for coherence with match index
     *          > Send corresponding entry with current indexes
     *
     * > Heartbeat reception from P as HS
     *      > If in P candidate mode, rollback to HS
     *      > Reset heartbeat timeout
     *
     * > Heartbeat reception from HS as CS
     *      > Reset heartbeat timeout
     *
     * > Heartbeat reception from P as CS
     *      > Update hosts list
     *      > Special flag if queried from reachability
     *          > Answer HS query with positive reachability
     *
     * > P heartbeat timeout as HS
     *      > Transition to P candidate
     *      > Queries other masters for reachability
     *
     * > Reachability query reception as CS
     *      > Ack back
     *      > Query master for reachability
     *
     * > Reachability query reception as P
     *      > Heartbeat back with special flag
     *
     * > Reachability query answer reception as HS
     *      > Update with result
     *          - Majority attained: unreachable
     *              > Transition to P and send heartbeat as P
     *          - Majority attained: reachable
     *              > Rollback to HS, query P for reachability
     *
     * > Timeout of
     *
     * > P takeover
     *      > transitions to P status, sends heartbeat as new P
     *          > remove any HS or CS related events
     *          > create P related events
     *          > remove HS-side backup channels
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
     * > P-HS close sync
     *
     * > HS-CS backup sync
     *
     * > History snapshotting and message counter reset
     *
     * > Log operation commit
     *
     * > Log operation pending
     *
     * > Log operation voting timeout ("proposed" state)
     *
     * > P to CS downgrade
     *
     * > Partition mode
     *
     * > Partition detection
    */
    return 0;
}
