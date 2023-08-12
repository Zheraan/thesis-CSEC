#ifndef RAFT_ENTRY_LOG_LIBRARY_H
#define RAFT_ENTRY_LOG_LIBRARY_H

#include "../datatypes.h"
#include "../raft-comms/entry-transmission.h"
#include "../expe/state-monitoring.h"

// Initializes the log's state
// Returns a pointer to the initialized log (same as argument)
log_s *log_init(log_s *log);

// Prints the log state to the specified output
void log_print(log_s *log, FILE *stream);

// Sets buf to be a string containing the name of the given state number. Attention: buffer space must be at least 12
// characters long
void log_entry_state_string(char *buf, enum entry_state state);

// Adds the entry contained in the transmission to the log as new entry with the given state.
// State parameter is voluntarily redundant with transmission's state field to allow for caching new entries
// in a log that has missing entries, to not require them again when repairing the missing ones.
// If the state parameter is ENTRY_STATE_CACHED, log indexes are not modified when adding the new entry.
// If the state parameter is ENTRY_STATE_COMMITTED, first commits all entries up to the given entry.
// If the local host is a master node, will update the replication array values for it.
// Returns EXIT_FAILURE in case of failure (if log is full or if it was not possible to commit all entries
// up to the new one) or EXIT_SUCCESS otherwise.
int log_add_entry(overseer_s *overseer, const entry_transmission_s *etr, enum entry_state state);

// Returns a pointer to the entry with the given id, or NULL if there is none
log_entry_s *log_get_entry_by_id(log_s *log, uint64_t id);

// Invalidates (through log_invalidate_from) entries above dist's next index and adjusts local index if
// necessary, then does nothing if another repair conversation is ongoing. This is checked by comparing the
// input CM's ack_back value with the stored fix_conversation cache ID. Otherwise, any ongoing replay process
// is ended, local next index is decremented, and a Log Repair request is sent to P. The state variables to
// keep track of ongoing fix (fix_type and fix_conversation) are set to keep track of it.
// In case no action is taken because another fix is ongoing, acks back the communication in the proper way.
// Returns EXIT_FAILURE if no P was found in the local hosts-list, if the local Fix-Type was invalid, if the
// retransmission initialization failed, or if the message failed to be sent (note that the message will still
// be retransmitted as per the RTC). EXIT_SUCCESS is returned otherwise.
int log_repair(overseer_s *overseer, control_message_s *cm);

// Does nothing if another replay conversation is ongoing. This is checked by comparing the input CM's
// ack_back value with the stored fix_conversation cache ID. Otherwise, any ongoing repair process is ended,
// and a Log Replay request is sent to P. The state variables to keep track of ongoing fix (fix_type and
// fix_conversation) are set to keep track of it.
// In case no action is taken because another fix is ongoing, acks back the communication in the proper way.
// Returns EXIT_FAILURE if no P was found in the local hosts-list, if the local Fix-Type was invalid, if the
// retransmission initialization failed, or if the message failed to be sent (note that the message will still
// be retransmitted as per the RTC). EXIT_SUCCESS is returned otherwise.
int log_replay(overseer_s *overseer, control_message_s *cm);

// Removes the RTC (through rtc_remove_by_id) whose ID is stored in the local fix_conversation value, then
// sets this value to 0 and the fix_type to FIX_TYPE_NONE.
void log_fix_end(overseer_s *overseer);

// Returns a boolean value based on if the local fix_type is FIX_TYPE_REPAIR
int log_repair_ongoing(overseer_s *overseer);

// Returns a boolean value based on if the local fix_type is FIX_TYPE_REPLAY
int log_replay_ongoing(overseer_s *overseer);

// Not implemented yet, only logs an error message and returns EXIT_SUCCESS
int log_repair_override(overseer_s *overseer, control_message_s *cm); // TODO Extension implement log_repair_override

// Marks all non-empty entries from given index included as invalid, and sets the log's next index as the
// given index if it was greater than it.
void log_invalidate_from(log_s *log, uint64_t index);

// Commits the log entry with the given index and applies it in the MFS, may fail if entry is marked
// as empty or invalid, or if the commit index is not equal to given index minus 1
int log_entry_commit(overseer_s *overseer, uint64_t index);

// Commit log entries from the local commit index up to the given index (included) and applies them in the MFS.
int log_commit_upto(overseer_s *overseer, uint64_t index);

#endif //RAFT_ENTRY_LOG_LIBRARY_H
