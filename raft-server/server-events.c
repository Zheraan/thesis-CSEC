//
// Created by zheraan on 22/04/23.
//

#include "server-events.h"

int server_random_ops_init(overseer_s *overseer) {
    // Create a persistent event only triggered by timeout
    struct event *nevent = event_new(overseer->eb,
                                     -1,
                                     EV_TIMEOUT | EV_PERSIST,
                                     server_random_ops_cb,
                                     (void *) overseer);
    if (nevent == NULL) {
        fprintf(stderr, "Failed to create the data op event\n");
        return EXIT_FAILURE;
    }

    // Random operation generator has low priority
    event_priority_set(nevent, 1);

    if (event_list_add(overseer, nevent) == EXIT_FAILURE) {
        fprintf(stderr, "Failed to allocate the event list struct for the data op event\n");
        return (EXIT_FAILURE);
    }

    // Add the event in the loop
    struct timeval ops_timeout = timeout_gen(TIMEOUT_TYPE_RANDOM_OPS);
    if (errno == EUNKOWN_TIMEOUT_TYPE || event_add(nevent, &ops_timeout) != 0) {
        fprintf(stderr, "Failed to add the data op event\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void server_random_ops_cb(evutil_socket_t fd, short event, void *arg) {
    // Check if queue is empty
    int queue_was_empty = 1;
    if (((overseer_s *) arg)->mfs->queue != NULL)
        queue_was_empty = 0;

    // Create random op
    data_op_s *nop = op_new();
    if (nop == NULL)
        return; // Abort in case of failure

    // Add it to the queue
    ops_queue_s *nqelem = ops_queue_add(nop, ((overseer_s *) arg)->mfs);
    if (nqelem == NULL) {
        free(nop); // Cleanup and abort in case of failure
        return;
    }

    // Set timer for deletion
    if (server_queue_element_deletion_init((overseer_s *) arg, nqelem) == EXIT_FAILURE) {
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
        ops_queue_free_all(nqelem);// Cleanup and abort in case of failure, including subsequent elements
        return;
    }

    // If queue was empty when checked before adding the new element, if there is an available P and the cache is empty
    if (queue_was_empty && is_p_available(((overseer_s *) arg)->hl) && ((overseer_s *) arg)->mfs->op_cache == NULL) {
        if (server_proposition_send_init((overseer_s *) arg, nqelem) == EXIT_FAILURE) {
            // Cleanup and abort in case of failure, including subsequent elements
            // Note: no risk of dangling pointer since queue was empty
            ops_queue_free_all(nqelem);
            return;
        }
    }

    return;
}

void server_proposition_dequeue_timeout_cb(evutil_socket_t fd, short event, void *arg) {
    // Since ops are queued in order and we can't guarantee that subsequent ops aren't dependent on the first one,
    // we clear the full queue to avoid incoherences caused by partial applications.
    // Since ops_queue_free_all() also removes dequeuing events tied to queue elements, there is not risk of events
    // eventually added being accidentally removed.
    ops_queue_free_all(((overseer_s *) arg)->mfs->queue);
    return;
}

int server_queue_element_deletion_init(overseer_s *overseer, ops_queue_s *element) {
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
    if (errno == EUNKOWN_TIMEOUT_TYPE || event_add(nevent, &ops_timeout) != 0) {
        fprintf(stderr, "Failed to add a queue element deletion event\n");
        event_free(nevent);
        element->timeout_event = NULL; // Avoid dangling pointer
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int server_proposition_send_init(overseer_s *overseer, ops_queue_s *element) {
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
    transmission_s *ntr = tr_new(overseer,
                                 MSG_TYPE_TR_PROPOSITION,
                                 nop,
                                 overseer->log->next_index,
                                 overseer->log->P_term,
                                 ENTRY_STATE_PENDING);

    if (ntr == NULL) {
        fprintf(stderr, "Failed creating new transmission for transmitting proposition\n");
    } else {
        // Send proposition to P
        uint32_t p_id = whois_p(overseer->hl);
        if (p_id == 1 && errno == ENO_P) {
            fprintf(stderr, "Trying to send new proposition but no host has P status\n");
            return EXIT_FAILURE;
        }
        if (tr_sendto(overseer,
                      overseer->hl->hosts[p_id].addr,
                      overseer->hl->hosts[p_id].addr_len,
                      ntr)) {
            fprintf(stderr, "Failed transmitting the proposition\n");
            free(ntr);
            return EXIT_FAILURE;
        }
        free(ntr);
    }

    if (PROPOSITION_RETRANSMISSION_MAX_ATTEMPTS > 0) {
        // set timer for retransmission in case server doesn't receive any ack
        server_proposition_retransmission_init(overseer);
    }

    // TODO when ack comes, mfs_free_cache
    return EXIT_SUCCESS;
}

int server_proposition_retransmission_init(overseer_s *overseer) {
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
    if (errno == EUNKOWN_TIMEOUT_TYPE || event_add(nevent, &retransmission_timeout) != 0) {
        fprintf(stderr, "Failed to add a proposition retransmission event\n");
        event_free(nevent);
        free(overseer->mfs->op_cache);
        overseer->mfs->op_cache = NULL; // Avoid dangling pointer when checking if cache is empty
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void server_proposition_retransmission_cb(evutil_socket_t fd, short event, void *arg) {
    // Create new proposition with cached data
    transmission_s *ntr = tr_new((overseer_s *) arg,
                                 MSG_TYPE_TR_PROPOSITION,
                                 ((overseer_s *) arg)->mfs->op_cache,
                                 ((overseer_s *) arg)->log->next_index,
                                 ((overseer_s *) arg)->log->P_term,
                                 ENTRY_STATE_PENDING);
    if (ntr == NULL) {
        fprintf(stderr, "Failed creating new transmission for retransmitting proposition\n");
    } else {
        // Send proposition to P
        uint32_t p_id = whois_p(((overseer_s *) arg)->hl);
        if (p_id == 1 && errno == ENO_P) {
            fprintf(stderr, "Trying to retransmit proposition but no host has P status\n");
        } else if (tr_sendto((overseer_s *) arg,
                             ((overseer_s *) arg)->hl->hosts[p_id].addr,
                             ((overseer_s *) arg)->hl->hosts[p_id].addr_len,
                             ntr)) {
            fprintf(stderr, "Failed transmitting the proposition\n");
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
    return;
}

// TODO when data op commit order arrives, create pending entry, and fire the queue element deletion
// Then free the event
// attention : ops_queue_element_free doesn't free the data op but frees and removes the event