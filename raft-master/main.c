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
    // Create consensus lead channels
    // Create elections channels
    // Create close sync channels
    // Create CS-side backup channels

    // Launch HS elections

    /* Event loop:
     *
     * > Consensus
     *
     * > Log replication
     *
     * > Heartbeat
     *      - as result of new P/HS
     *          > update hosts list state
     *          > trigger election of new HS if old one took over
     *      > send update for log entry state if any (one by one ?)
     *      >
     *
     * > Heartbeat ack reception
     *      > from server : update log entry replication if any
     *
     * > P takeover
     *      > transitions to P, sends heartbeat as new P
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
     * > P to CS downgrade
     *
     * > Partition mode
     *
     * > Partition detection
    */
    return 0;
}
