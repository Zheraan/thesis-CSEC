//
// Created by zheraan on 05/05/23.
//

#ifndef THESIS_CSEC_TIMEOUT_H
#define THESIS_CSEC_TIMEOUT_H

#include <stdio.h>
#include <errno.h>
#include <event2/util.h>
#include "rc-datatypes.h"

// In case of success, returns a timeval containing a value within the defined parameters, either a random value
// in a range or a fixed time, depending on the timeout type specified in the function parameter.
// In case of unknown timeval range type, returns an uninitialized timeval and errno is set to EUNKOWN_TIMEOUT_TYPE.
// Defined parameters (see above) may be redefined at compile time.
struct timeval timeout_gen(enum timeout_type type);

#endif //THESIS_CSEC_TIMEOUT_H
