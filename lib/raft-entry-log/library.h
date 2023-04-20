#ifndef RAFT_ENTRY_LOG_LIBRARY_H
#define RAFT_ENTRY_LOG_LIBRARY_H

/* Log Data structure
 * - next index (index of next log entry that will be added)
 * - replication index (index of latest log entry that is either pending, committed or applied)
 * - match index (index of last log entry replicated by all nodes)
 * - committed index (index of last log committed locally)
 * - election ongoing ? ("proposed" state for master node)
 * - current P term
 * - current HS term
 */

/* Log Entry Data structure
 * - term number
 * - state (created, queued, pending, committed, applied)
 * - replicated by
 * - server replication count
 * - safe?
 */

void hello(void);

#endif //RAFT_ENTRY_LOG_LIBRARY_H
