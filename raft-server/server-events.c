//
// Created by zheraan on 22/04/23.
//

#include "server-events.h"

int server_random_ops_init(overseer_s *overseer) {
    if (DEBUG_LEVEL >= 3) {
        printf("- Initializing random ops generation events ... ");
        fflush(stdout);
    }
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

    if (DEBUG_LEVEL >= 3) {
        printf("Done.\n");
        fflush(stdout);
    }
    return EXIT_SUCCESS;
}

void server_random_ops_cb(evutil_socket_t fd, short event, void *arg) {
    if (DEBUG_LEVEL >= 4)
        printf("Start of random op callback ---------\n");

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

    // If queue was empty when checked before adding the new element, if there is an available P and the cache is empty
    if (queue_was_empty && is_p_available(((overseer_s *) arg)->hl) && ((overseer_s *) arg)->mfs->op_cache == NULL) {
        // Then transmit the proposition
        if (server_proposition_transmit((overseer_s *) arg, nqelem) != EXIT_SUCCESS) {
            // Cleanup and abort in case of failure, including subsequent elements
            // Note: no risk of dangling pointer since queue was empty
            fprintf(stderr,
                    "Failed to transmit proposition.\n"
                    "Reset the queue: %d elements freed.\n",
                    ops_queue_free_all((overseer_s *) arg, nqelem));
            fflush(stderr);
            return;
        }
    }

    if (server_random_ops_init((overseer_s *) arg) != EXIT_SUCCESS) {
        fprintf(stderr, "Fatal Error: random ops generator event couldn't be set\n");
        fflush(stderr);
        exit(EXIT_FAILURE); // TODO Crash handler
    }

    if (DEBUG_LEVEL >= 4) {
        printf("End of random op callback ---------\n");
        fflush(stdout);
    }
    return;
}

void server_proposition_dequeue_timeout_cb(evutil_socket_t fd, short event, void *arg) {
    // Since ops are queued in order and we can't guarantee that subsequent ops aren't dependent on the first one,
    // we clear the full queue to avoid incoherences caused by partial applications.
    // Since ops_queue_free_all() also removes dequeuing events tied to queue elements, there is not risk of events
    // eventually added being accidentally removed.
    int nb_freed = ops_queue_free_all((overseer_s *) arg, ((overseer_s *) arg)->mfs->queue);
    if (DEBUG_LEVEL >= 1) {
        printf("Proposition queue element timed out: %d element(s) freed.\n", nb_freed);
        fflush(stdout);
    }
    return;
}

int server_queue_element_deletion_init(overseer_s *overseer, ops_queue_s *element) {
    if (DEBUG_LEVEL >= 4) {
        printf("- Initializing queue element deletion event ... ");
        fflush(stdout);
    }
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

    if (DEBUG_LEVEL >= 4) {
        printf("Done.\n");
        fflush(stdout);
    }
    return EXIT_SUCCESS;
}

int server_proposition_transmit(overseer_s *overseer, ops_queue_s *element) {
    if (DEBUG_LEVEL >= 4) {
        printf("Starting proposition transmission ... ");
        fflush(stdout);
    }
    // Copy the data op in the queue element into the op_cache in case it is removed before retransmission
    data_op_s *nop = malloc(sizeof(data_op_s));
    if (nop == NULL) {
        perror("malloc data op for caching");
        return EXIT_FAILURE; // Abort in case of failure
    }
    nop->column = element->op->column;
    nop->row = element->op->row;
    nop->newval = element->op->newval;
    free(overseer->mfs->op_cache); // Free memory space of last cached value
    overseer->mfs->op_cache = nop;

    // Create new proposition
    entry_transmission_s *ntr = etr_new(overseer,
                                        MSG_TYPE_ETR_PROPOSITION,
                                        nop,
                                        overseer->log->next_index,
                                        overseer->log->P_term,
                                        ENTRY_STATE_PENDING);
    free(nop);

    if (ntr == NULL) {
        fprintf(stderr, "Failed creating new transmission for transmitting proposition\n");
    } else {
        // Send proposition to P
        uint32_t p_id = whois_p(overseer->hl);
        if (p_id == 1 && errno == ENO_P) {
            fprintf(stderr, "Trying to send new proposition but no host has P status\n");
            return EXIT_FAILURE;
        }
        if (etr_sendto(overseer,
                       overseer->hl->hosts[p_id].addr,
                       overseer->hl->hosts[p_id].socklen,
                       ntr) != EXIT_SUCCESS) {
            fprintf(stderr, "Failed transmitting the proposition\n");
            free(ntr);
            return EXIT_FAILURE;
        }
        free(ntr);
    }

    if (PROPOSITION_RETRANSMISSION_MAX_ATTEMPTS > 0) {
        // set timer for retransmission in case server doesn't receive any ack
        if (server_proposition_retransmission_init(overseer) != EXIT_SUCCESS) {
            fprintf(stderr, "Failed initializing retransmission event.\n");
            return EXIT_FAILURE;
        }
    }

    // TODO when ack comes, mfs_free_cache && send next prop
    if (DEBUG_LEVEL >= 4) {
        printf("Done.\n");
        fflush(stdout);
    }
    return EXIT_SUCCESS;
}

int server_proposition_retransmission_init(overseer_s *overseer) {
    if (DEBUG_LEVEL >= 4) {
        printf("- Initializing proposition retransmission event ... ");
        fflush(stdout);
    }
    // Create a non-persistent event triggered only by timeout
    struct event *nevent = event_new(overseer->eb,
                                     -1,
                                     EV_TIMEOUT,
                                     server_proposition_retransmission_cb,
                                     (void *) overseer);
    if (nevent == NULL) {
        fprintf(stderr, "Failed to create a proposition retransmission event\n");
        return EXIT_FAILURE;
    }

    // Proposition retransmission has low priority
    event_priority_set(nevent, 1);

    // We add the retransmission event to the event op_cache to keep track of it for eventual deletion and free
    overseer->mfs->event_cache = nevent;

    // Add the event in the loop
    struct timeval retransmission_timeout = timeout_gen(TIMEOUT_TYPE_PROP_RETRANSMISSION);
    if (errno == EUNKNOWN_TIMEOUT_TYPE || event_add(nevent, &retransmission_timeout) != 0) {
        fprintf(stderr, "Failed to add a proposition retransmission event\n");
        event_free(nevent);
        free(overseer->mfs->op_cache);
        overseer->mfs->op_cache = NULL; // Avoid dangling pointer when checking if cache is empty
        return EXIT_FAILURE;
    }

    if (DEBUG_LEVEL >= 4) {
        printf("Done.\n");
        fflush(stdout);
    }
    return EXIT_SUCCESS;
}

void server_proposition_retransmission_cb(evutil_socket_t fd, short event, void *arg) {
    if (DEBUG_LEVEL >= 3) {
        printf("Proposition retransmission timed out, reattempting transmission (attempt %d) ... ",
               ((overseer_s *) arg)->mfs->retransmission_attempts + 1);
        fflush(stdout);
    }
    int success = 1;
    // Create new proposition with cached data
    entry_transmission_s *ntr = etr_new((overseer_s *) arg,
                                        MSG_TYPE_ETR_PROPOSITION,
                                        ((overseer_s *) arg)->mfs->op_cache,
                                        ((overseer_s *) arg)->log->next_index,
                                        ((overseer_s *) arg)->log->P_term,
                                        ENTRY_STATE_PENDING);
    if (ntr == NULL) {
        fprintf(stderr, "Failed creating new transmission for retransmitting proposition\n");
        success = 0;
    } else {
        // Send proposition to P
        uint32_t p_id = whois_p(((overseer_s *) arg)->hl);
        if (p_id == 1 && errno == ENO_P) {
            fprintf(stderr, "Trying to retransmit proposition but no host has P status\n");
            success = 0;
        } else if (etr_sendto((overseer_s *) arg,
                              ((overseer_s *) arg)->hl->hosts[p_id].addr,
                              ((overseer_s *) arg)->hl->hosts[p_id].socklen,
                              ntr)) {
            fprintf(stderr, "Failed transmitting the proposition\n");
            success = 0;
        }
        free(ntr);
    }

    // Increase attempts number
    ((overseer_s *) arg)->mfs->retransmission_attempts++;

    // If attempts max reached, reset cache
    if (((overseer_s *) arg)->mfs->retransmission_attempts >= PROPOSITION_RETRANSMISSION_MAX_ATTEMPTS) {
        mfs_free_cache((overseer_s *) arg);
    } else { // Otherwise add retransmission event
        server_proposition_retransmission_init((overseer_s *) arg);
    }

    if (DEBUG_LEVEL >= 3 && success == 1) {
        printf("Done.\n");
        fflush(stdout);
    }
    return;
}

// TODO when data op commit order arrives, create pending entry, and fire the queue element deletion
// Then free the event
// attention : ops_queue_element_free doesn't free the data op but frees and removes the event