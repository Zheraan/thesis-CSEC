//
// Created by zheraan on 05/05/23.
//

#include "timeout.h"

struct timeval timeout_gen(enum timeout_type type) {
    struct timeval ntv;

    uint32_t buf, modulo = 0, offset = 0, randomize = 1;

    // Define the timeout range depending on the defined parameters
    switch (type) {
        case TIMEOUT_TYPE_P_HB:
            ntv.tv_usec = MODULO(TIMEOUT_VALUE_P_HB, 1000000);
            ntv.tv_sec = DIVIDE(TIMEOUT_VALUE_P_HB, 1000000);
            randomize = 0;
            break;

        case TIMEOUT_TYPE_HS_HB:
            ntv.tv_usec = MODULO(TIMEOUT_VALUE_HS_HB, 1000000);
            ntv.tv_sec = DIVIDE(TIMEOUT_VALUE_HS_HB, 1000000);
            randomize = 0;
            break;

        case TIMEOUT_TYPE_PROPOSITION:
            ntv.tv_usec = MODULO(TIMEOUT_VALUE_PROPOSITION, 1000000);
            ntv.tv_sec = DIVIDE(TIMEOUT_VALUE_PROPOSITION, 1000000);
            randomize = 0;
            break;

        case TIMEOUT_TYPE_ACK:
            ntv.tv_usec = MODULO(TIMEOUT_VALUE_ACK, 1000000);
            ntv.tv_sec = DIVIDE(TIMEOUT_VALUE_ACK, 1000000);
            randomize = 0;
            break;

        case TIMEOUT_TYPE_PROP_RETRANSMISSION:
            ntv.tv_usec = MODULO(TIMEOUT_VALUE_PROP_RETRANSMISSION, 1000000);
            ntv.tv_sec = DIVIDE(TIMEOUT_VALUE_PROP_RETRANSMISSION, 1000000);
            randomize = 0;
            break;

        case TIMEOUT_TYPE_P_LIVENESS:
            ntv.tv_usec = MODULO(TIMEOUT_VALUE_P_LIVENESS, 1000000);
            ntv.tv_sec = DIVIDE(TIMEOUT_VALUE_P_LIVENESS, 1000000);
            randomize = 0;
            break;

        case TIMEOUT_TYPE_HS_ELECTION:
            if (TIMEOUT_RANGE_ELECTION > 0)
                modulo = TIMEOUT_RANGE_ELECTION + 1;
            offset = TIMEOUT_OFFSET_ELECTION;
            break;

        case TIMEOUT_TYPE_FUZZER:
            if (FUZZER_LATENCY_DISTRIBUTION_ENABLE) {
                // To gain performance and clarity, here we emulate fixed-point arithmetics using 32 bits unsigned
                // integers. As we are using timevals that have a 6 digits integers for the microsecond parameter, the
                // fixed point is set at 1000000.
                evutil_secure_rng_get_bytes(&buf, sizeof(uint32_t));
                uint32_t a = FUZZER_LATENCY_DISTRIBUTION_MINIMUM,
                        b = 100000000, // Maximum of the definition interval (minimum is 0)
                c = FUZZER_LATENCY_DISTRIBUTION_PROPORTION * 1000000;
                buf = MODULO(buf, b);
                buf = a * (1 + (buf * (b - c) * (b - c)) / (c * (b - buf) * (b - buf))); // Applying function
                if (buf > FUZZER_LATENCY_DISTRIBUTION_MAXIMUM)
                    buf = FUZZER_LATENCY_DISTRIBUTION_MAXIMUM;
                ntv.tv_usec = MODULO(buf, 1000000);
                ntv.tv_sec = DIVIDE(buf, 1000000);
            }
            if (TIMEOUT_RANGE_FUZZER > 0)
                modulo = TIMEOUT_RANGE_FUZZER + 1;
            offset = TIMEOUT_OFFSET_FUZZER;
            break;

        case TIMEOUT_TYPE_RANDOM_OPS:
            if (TIMEOUT_RANGE_RANDOM_OPS > 0)
                modulo = TIMEOUT_RANGE_RANDOM_OPS + 1;
            offset = TIMEOUT_OFFSET_RANDOM_OPS;
            break;

        default:
            debug_log(0, stderr, "Unknown timeout type\n");
            errno = EUNKNOWN_TIMEOUT_TYPE;
            return ntv;
    }

    if (randomize) {
        if (modulo > 0) {
            evutil_secure_rng_get_bytes(&buf, sizeof(uint32_t));
        } else {
            buf = 0;
        }
        buf = MODULO(buf, modulo); // Set the value inside the range
        ntv.tv_usec = MODULO(buf + offset + TIMEOUT_GLOBAL_SLOWDOWN_OFFSET, 1000000);
        ntv.tv_sec = DIVIDE(buf + offset + TIMEOUT_GLOBAL_SLOWDOWN_OFFSET, 1000000);
    } else {
        ntv.tv_sec += DIVIDE(TIMEOUT_GLOBAL_SLOWDOWN_OFFSET, 1000000);
        ntv.tv_usec += MODULO(TIMEOUT_GLOBAL_SLOWDOWN_OFFSET, 1000000);
    }


    if (DEBUG_LEVEL >= 4)
        printf("[Created a timeout of type %d and a duration of %ld.%lds] ",
               type,
               ntv.tv_sec,
               ntv.tv_usec);
    return ntv;
}