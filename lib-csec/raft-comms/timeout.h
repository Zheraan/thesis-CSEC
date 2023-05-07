//
// Created by zheraan on 05/05/23.
//

#ifndef THESIS_CSEC_TIMEOUT_H
#define THESIS_CSEC_TIMEOUT_H

#include <stdio.h>
#include <errno.h>
#include <event2/util.h>

#define EUNKOWN_TIMEOUT_TYPE (-1)

#ifndef TIMEOUT_OFFSET_USEC
// minimum value of timeouts' timeval's usec value for randomized timers, maximum is offset + range
#define TIMEOUT_OFFSET_USEC 100
#endif

#ifndef TIMEOUT_OFFSET_SEC
// minimum value of timeouts' timeval's sec value for randomized timers, maximum is offset + range
#define TIMEOUT_OFFSET_SEC 0
#endif

// Defines for the range of the usec value in timeouts' timeval
#ifndef TIMEOUT_RANGE_P_HB_USEC
#define TIMEOUT_RANGE_P_HB_USEC 0
#endif

#ifndef TIMEOUT_RANGE_HS_HB_USEC
#define TIMEOUT_RANGE_HS_HB_USEC 0
#endif

#ifndef TIMEOUT_RANGE_ELECTION_USEC
#define TIMEOUT_RANGE_ELECTION_USEC 150000 // Default: 150ms
#endif

#ifndef TIMEOUT_RANGE_FUZZER_USEC
#define TIMEOUT_RANGE_FUZZER_USEC 0
#endif

#ifndef TIMEOUT_RANGE_RANDOM_OPS_USEC
#define TIMEOUT_RANGE_RANDOM_OPS_USEC 0
#endif

// Defines for the range of the sec value in timeouts' timeval
#ifndef TIMEOUT_RANGE_P_HB_SEC
#define TIMEOUT_RANGE_P_HB_SEC 1
#endif

#ifndef TIMEOUT_RANGE_HS_HB_SEC
#define TIMEOUT_RANGE_HS_HB_SEC 1
#endif

#ifndef TIMEOUT_RANGE_ELECTION_SEC
#define TIMEOUT_RANGE_ELECTION_SEC 0
#endif

#ifndef TIMEOUT_RANGE_FUZZER_SEC
#define TIMEOUT_RANGE_FUZZER_SEC 1
#endif

#ifndef TIMEOUT_RANGE_RANDOM_OPS_SEC
#define TIMEOUT_RANGE_RANDOM_OPS_SEC 8
#endif

// Defines for the fixed timeouts values
#ifndef TIMEOUT_VALUE_PROPOSITION_USEC
#define TIMEOUT_VALUE_PROPOSITION_USEC 500000 // Default: 500ms
#endif

#ifndef TIMEOUT_VALUE_PROPOSITION_SEC
#define TIMEOUT_VALUE_PROPOSITION_SEC 0
#endif

#ifndef TIMEOUT_VALUE_ACK_SEC
#define TIMEOUT_VALUE_ACK_SEC 0
#endif

#ifndef TIMEOUT_VALUE_ACK_USEC
#define TIMEOUT_VALUE_ACK_USEC 100000 // Default: 100ms
#endif

#ifndef TIMEOUT_VALUE_PROP_RETRANSMISSION_SEC
#define TIMEOUT_VALUE_PROP_RETRANSMISSION_SEC 0
#endif

#ifndef TIMEOUT_VALUE_PROP_RETRANSMISSION_USEC
#define TIMEOUT_VALUE_PROP_RETRANSMISSION_USEC 300000 // Default: 300ms
#endif

enum timeout_type{
    TIMEOUT_TYPE_P_HB = 0, // P master heartbeat timeout type
    TIMEOUT_TYPE_HS_HB = 1, // HS master heartbeat timeout
    TIMEOUT_TYPE_PROPOSITION = 2, // Data op proposition timeout type, after which the proposition is deleted from queue
    TIMEOUT_TYPE_ACK = 3, // Message ack timeout type
    TIMEOUT_TYPE_ELECTION = 4, // Master election timeout type
    TIMEOUT_TYPE_FUZZER = 5, // Fuzzer delay introduction timeout type
    TIMEOUT_TYPE_RANDOM_OPS = 6, // Random data op generator timeout type
    TIMEOUT_TYPE_PROP_RETRANSMISSION = 7, // Timeout before retransmitting cached ops proposition
};

// In case of success, returns a timeval containing a value within the defined parameters, either a random value
// in a range or a fixed time, depending on the timeout type specified in the function parameter.
// In case of unknown timeval range type, returns an uninitialized timeval and errno is set to EUNKOWN_TIMEOUT_TYPE.
// Defined parameters (see above) may be redefined at compile time.
struct timeval timeout_gen(enum timeout_type type);

#endif //THESIS_CSEC_TIMEOUT_H
