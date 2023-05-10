//
// Created by zheraan on 03/05/23.
//

#ifndef THESIS_CSEC_EVENT_LIST_H
#define THESIS_CSEC_EVENT_LIST_H

#include <event2/event.h>
#include <stdlib.h>

#include "datatypes.h"
#include "overseer.h"

// Frees the linked list of events
void event_list_free(event_list_s * el);

// Adds an event to the list in the overseer to keep track of the persistent events that need to be freed later
// Returns EXIT_FAILURE in case allocation fails, EXIT_SUCCESS otherwise
int event_list_add(overseer_s *o, struct event *ev);

#endif //THESIS_CSEC_EVENT_LIST_H
