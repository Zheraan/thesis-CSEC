//
// Created by zheraan on 22/04/23.
//

#include "server-events.h"

int server_random_ops_init(overseer_s *overseer) {
    debug_log(3, stdout, "- Initializing random ops generation events ... ");
    // Create a non-persistent event only triggered by timeout
    struct event *nevent = event_new(overseer->eb,
                                     -1,
                                     EV_TIMEOUT,
                                     server_random_ops_cb,
                                     (void *) overseer);
    if (nevent == NULL) {
        fprintf(stderr, "Failed to create the data op event\n");
        fflush(stderr);
        return EXIT_FAILURE;
    }

    // Random operation generator has low priority
    event_priority_set(nevent, 1);

    if (overseer->special_event != NULL) // Freeing the past heartbeat if any
        event_free(overseer->special_event);
    overseer->special_event = nevent;

    // Add the event in the loop
    struct timeval ops_timeout = timeout_gen(TIMEOUT_TYPE_RANDOM_OPS);
    if (errno == EUNKNOWN_TIMEOUT_TYPE || event_add(nevent, &ops_timeout) != 0) {
        fprintf(stderr, "Failed to add the data op event\n");
        fflush(stderr);
        return EXIT_FAILURE;
    }

    debug_log(3, stdout, "Done.\n");
    return EXIT_SUCCESS;
}

void server_random_ops_cb(evutil_socket_t fd, short event, void *arg) {
    debug_log(4, stdout, "Start of random op callback -------------------------------------------------------\n");

    // Check if queue is empty
    int queue_was_empty = 1;
    if (((overseer_s *) arg)->mfs->queue != NULL)
        queue_was_empty = 0;

    // Create random op
    data_op_s *nop = op_new();
    if (nop == NULL) {
        fprintf(stderr, "Failed to generate a new data op\n");
        fflush(stderr);
        return; // Abort in case of failure
    }

    if (DEBUG_LEVEL >= 3) {
        printf("Generated a new op:\n");
        op_print(nop, stdout);
    }

    // Add it to the queue
    ops_queue_s *nqelem = ops_queue_add(nop, ((overseer_s *) arg)->mfs);
    if (nqelem == NULL) {
        free(nop); // Cleanup and abort in case of failure
        fprintf(stderr, "Failed to add a new op in the queue\n");
        fflush(stderr);
        return;
    }

    // Set timer for deletion
    if (server_queue_element_deletion_init((overseer_s *) arg, nqelem) != EXIT_SUCCESS) {
        // In case of failure and if the queue wasn't empty, we must clear possible dangling pointers before
        // cleaning up the list:
        if (!queue_was_empty) {
            for (ops_queue_s *ptr = ((overseer_s *) arg)->mfs->queue; ptr == NULL; ptr = ptr->next) {
                if (ptr->next == nqelem) {
                    ptr->next = NULL; // Clear pointer
                    break;
                }
            }
        }
        // Cleanup and abort in case of failure, including subsequent elements
        fprintf(stderr,
                "Failed to initialize queue element deletion timeout.\n"
                "Clear queue from element: %d element(s) freed.\n",
                ops_queue_free_all((overseer_s *) arg, nqelem));
        fflush(stderr);
        return;
    }

    int p_available = is_p_available(((overseer_s *) arg)->hl);

    if (!queue_was_empty) {
        debug_log(3, stdout, "Queue is not empty: holding new proposition.\n");
    } else if (!p_available) {
        debug_log(3, stdout, "Queue is empty, however P is not available: holding new proposition.\n");
    } else { // If queue was empty when checked before adding the new element and if there is an available P
        // Then transmit the proposition
        server_send_first_prop((overseer_s *) arg);
    }

    if (server_random_ops_init((overseer_s *) arg) != EXIT_SUCCESS) {
        fprintf(stderr, "Fatal Error: random ops generator event couldn't be set\n");
        fflush(stderr);
        exit(EXIT_FAILURE); // TODO Crash handler
    }

    debug_log(4, stdout, "End of random op callback -------------------------------------------------------\n\n");
    return;
}

int server_send_first_prop(overseer_s *overseer) {
    entry_transmission_s *netr = etr_new(overseer,
                                         MSG_TYPE_ETR_PROPOSITION,
                                         overseer->mfs->queue->op,
                                         overseer->log->next_index,
                                         overseer->log->P_term,
                                         ENTRY_STATE_PROPOSAL,
                                         0);

    if (etr_sendto_with_rt_init(overseer,
                                netr,
                                overseer->hl->hosts[whois_p(overseer->hl)].addr,
                                overseer->hl->hosts[whois_p(overseer->hl)].socklen,
                                MSG_TYPE_ETR_PROPOSITION,
                                PROPOSITION_RETRANSMISSION_MAX_ATTEMPTS) != EXIT_SUCCESS) {
        // Cleanup and abort in case of failure, including subsequent elements
        // Note: no risk of dangling pointer since queue was empty
        fprintf(stderr,
                "Failed to transmit proposition.\n"
                "Resetting the queue: %d elements freed.\n",
                ops_queue_free_all(overseer, overseer->mfs->queue));
        fflush(stderr);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void server_proposition_dequeue_timeout_cb(evutil_socket_t fd, short event, void *arg) {
    // Since ops are queued in order, and since we can't guarantee that subsequent ops aren't dependent on the first
    // one, we clear the full queue to avoid incoherences caused by partial applications.
    // Since ops_queue_free_all() also removes dequeuing events tied to queue elements, there is no risk of events
    // eventually added being accidentally removed by old dequeuing events whose ops have already been removed.
    int nb_freed = ops_queue_free_all((overseer_s *) arg, ((overseer_s *) arg)->mfs->queue);
    if (DEBUG_LEVEL >= 1) {
        printf("Proposition queue element timed out: %d element(s) freed.\n", nb_freed);
        fflush(stdout);
    }
    return;
}

int server_queue_element_deletion_init(overseer_s *overseer, ops_queue_s *element) {
    debug_log(4, stdout, "- Initializing queue element deletion event ... ");
    // Create a non-persistent event triggered only by timeout
    struct event *nevent = event_new(overseer->eb,
                                     -1,
                                     EV_TIMEOUT,
                                     server_proposition_dequeue_timeout_cb,
                                     (void *) overseer);
    if (nevent == NULL) {
        fprintf(stderr, "Failed to create a queue element deletion event\n");
        return EXIT_FAILURE;
    }

    // Element dequeuing has low priority
    event_priority_set(nevent, 1);

    // We add the related deletion event to the queue element to keep track of it for eventual deletion and free
    element->timeout_event = nevent;

    // Add the event in the loop
    struct timeval ops_timeout = timeout_gen(TIMEOUT_TYPE_PROPOSITION);
    if (errno == EUNKNOWN_TIMEOUT_TYPE || event_add(nevent, &ops_timeout) != 0) {
        fprintf(stderr, "Failed to add a queue element deletion event\n");
        event_free(nevent);
        element->timeout_event = NULL; // Avoid dangling pointer
        return EXIT_FAILURE;
    }

    debug_log(4, stdout, "Done.\n");
    return EXIT_SUCCESS;
}

// TODO when data op commit order arrives, free cache and event, create pending entry, and send the next one
//  in the queue.
//  attention : ops_queue_element_free doesn't free the data op but frees and removes the event