//
// Created by zheraan on 24/07/23.
//

#ifndef THESIS_CSEC_EXPE_DATATYPES_H
#define THESIS_CSEC_EXPE_DATATYPES_H

#ifndef PSTR_NB_ENTRIES
// Number of log entries sent in LSTRs
#define PSTR_NB_ENTRIES 10
#endif

#ifndef MONITORING_LEVEL
// Verbosity of the cluster monitor's output
#define MONITORING_LEVEL 3
#endif

#ifndef RANDOM_KILL_ENABLE
// Enables the random killings of server and master nodes by the cluster monitor
#define RANDOM_KILL_ENABLE 1
#endif

#include "event2/event.h"
#include "../datatypes.h"
#include "../overseer.h"

// Contains either an entry_transmission_s in the etr field or a control_message_s in the cm field
union packet {
    entry_transmission_s etr;
    control_message_s cm;
};

enum packet_type {
    PACKET_TYPE_ETR = 0, // Signifies the packet contains an ETR
    PACKET_TYPE_CM = 1, // Signifies the packet contains a CM
};

// Linked list of entries for the fuzzer. Whenever a message is to be sent, it is instead added to this
// list to only be sent after a randomized delay to simulate the inconsistent nature of real-world
// communications.
typedef struct fuzzer_cache_s {
    // Number of the cache entry in the queue
    uint32_t id;
    // Type of the packet, either ETR or CM
    enum packet_type type;
    // Packet that is to be sent
    union packet p;
    // Event tied to the transmission of this packet
    struct event *ev;
    // Address of the packet's receiver
    struct sockaddr_in6 addr;
    // Socklen of the packet's receiver
    socklen_t socklen;
    // Next entry in the list
    struct fuzzer_cache_s *next;
    // Pointer back to the program state
    overseer_s *overseer;
} fuzzer_cache_s;

typedef struct program_state_transmission_s {
    // Sender's hosts-list ID
    uint32_t id;
    // Status of the sender
    enum host_status status;
    // Sender's P-term
    uint32_t P_term;
    // Index of next log entry that will be added by the sender
    uint64_t next_index;
    // Index of latest log entry that is committed by the sender
    uint64_t commit_index;
    // Number of ops applied since initialization or last patch by the sender
    uint64_t nb_ops;
    // Latest PSTR_NB_ENTRIES entries in the sender's log
    log_entry_s last_entries[PSTR_NB_ENTRIES];
    // Sender's current MFS state
    uint8_t mfs_array[MOCKED_FS_ARRAY_ROWS][MOCKED_FS_ARRAY_COLUMNS];
} program_state_transmission_s;

#endif //THESIS_CSEC_EXPE_DATATYPES_H
