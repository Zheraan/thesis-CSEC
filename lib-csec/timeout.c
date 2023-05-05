//
// Created by zheraan on 05/05/23.
//

#include "timeout.h"

struct timeval *timeout_random(enum timeout_type type){
    struct timeval *ntv;

    // Randomly pick a new character
    int buf;
    evutil_secure_rng_get_bytes(&buf, sizeof(int));
    buf %= 95; // Ensure value is no bigger than the alphanumeric range
    ntv->tv_usec = buf + 32; // Ensure value is within the alphanumeric range

    return ntv;
}