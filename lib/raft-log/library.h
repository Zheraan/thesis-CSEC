#ifndef RAFT_LOG_LIBRARY_H
#define RAFT_LOG_LIBRARY_H

/* Log Data structure
 * - next index (index of next log entry that will be added)
 * - match index (index of last log entry replicated by all nodes)
 * - pending index (index of last log entry in pending state locally)
 * - committed index (index of last log committed locally)
 * - election ongoing ? ("proposed" state)
 */

/* Log Entry Data structure
 * - term number
 * - state (created, queued, pending, committed, applied)
 * - replicated by
 * - server replication count
 * - safe?
 */

void hello(void);

#endif //RAFT_LOG_LIBRARY_H
