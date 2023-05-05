//
// Created by zheraan on 05/05/23.
//

#ifndef THESIS_CSEC_TIMEOUT_H
#define THESIS_CSEC_TIMEOUT_H

#include <stdio.h>
#include "errno.h"
#include <event2/util.h>

#define EUNKOWN_TIMEOUT_TYPE (-1)

#ifndef TIMEOUT_OFFSET
#define TIMEOUT_OFFSET 100 // minimum value of timeouts, maximum is offset + 
#endif

// Defines for the usec value of timeouts' timeval
#ifndef TIMEOUT_RANGE_P_HB_USEC
#define TIMEOUT_RANGE_P_HB_USEC 500
#endif

#ifndef TIMEOUT_RANGE_HS_HB_USEC
#define TIMEOUT_RANGE_HS_HB_USEC 500
#endif

#ifndef TIMEOUT_RANGE_PROPOSITION_USEC
#define TIMEOUT_RANGE_PROPOSITION_USEC 500
#endif

#ifndef TIMEOUT_RANGE_ACK_USEC
#define TIMEOUT_RANGE_ACK_USEC 500
#endif

#ifndef TIMEOUT_RANGE_ELECTION_USEC
#define TIMEOUT_RANGE_ELECTION_USEC 500
#endif

#ifndef TIMEOUT_RANGE_FUZZER_USEC
#define TIMEOUT_RANGE_FUZZER_USEC 0
#endif

// Defines for the sec value of timeouts' timeval
#ifndef TIMEOUT_RANGE_P_HB_SEC
#define TIMEOUT_RANGE_P_HB_SEC 0
#endif

#ifndef TIMEOUT_RANGE_HS_HB_SEC
#define TIMEOUT_RANGE_HS_HB_SEC 0
#endif

#ifndef TIMEOUT_RANGE_PROPOSITION_SEC
#define TIMEOUT_RANGE_PROPOSITION_SEC 0
#endif

#ifndef TIMEOUT_RANGE_ACK_SEC
#define TIMEOUT_RANGE_ACK_SEC 0
#endif

#ifndef TIMEOUT_RANGE_ELECTION_SEC
#define TIMEOUT_RANGE_ELECTION_SEC 0
#endif

#ifndef TIMEOUT_RANGE_FUZZER_SEC
#define TIMEOUT_RANGE_FUZZER_SEC 1
#endif

enum timeout_type{
    TIMEOUT_TYPE_P_HB = 0,
    TIMEOUT_TYPE_HS_HB = 1,
    TIMEOUT_TYPE_PROPOSITION = 2,
    TIMEOUT_TYPE_ACK = 3,
    TIMEOUT_TYPE_ELECTION = 4,
    TIMEOUT_TYPE_FUZZER = 5,
};

// Returns a timeval containing a random value within the defined parameters
// of the range corresponding to the timeout type.
// Defined parameters may be redefined at compile time
struct timeval timeout_random(enum timeout_type type);

#endif //THESIS_CSEC_TIMEOUT_H
