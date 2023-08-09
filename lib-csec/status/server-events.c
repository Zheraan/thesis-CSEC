//
// Created by zheraan on 22/04/23.
//

#include "server-events.h"

int server_random_ops_init(overseer_s *overseer) {
    debug_log(3, stdout, "- Initializing next random op generation event ... ");
    // Create a non-persistent event only triggered by timeout
    struct event *nevent = evtimer_new(overseer->eb,
                                       server_random_ops_cb,
                                       (void *) overseer);
    if (nevent == NULL) {
        fprintf(stderr, "Failed to create the data op event\n");
        if (INSTANT_FFLUSH) fflush(stderr);
        return EXIT_FAILURE;
    }

    // Random operation generator has low priority
    event_priority_set(nevent, 1);

    if (overseer->special_event != NULL) // Freeing the past random op generator event if any
        event_free(overseer->special_event);
    overseer->special_event = nevent;

    // Add the event in the loop
    errno = 0;
    struct timeval ops_timeout = timeout_gen(TIMEOUT_TYPE_RANDOM_OPS);
    if (errno == EUNKNOWN_TIMEOUT_TYPE || event_add(nevent, &ops_timeout) != 0) {
        fprintf(stderr, "Failed to add the data op event\n");
        if (INSTANT_FFLUSH) fflush(stderr);
        return EXIT_FAILURE;
    }

    debug_log(3, stdout, "Done.\n");
    return EXIT_SUCCESS;
}

void server_random_ops_cb(evutil_socket_t fd, short event, void *arg) {
    debug_log(4, stdout,
              "Start of random op callback --------------------------------------------------------------------\n");

    overseer_s *overseer = (overseer_s *) arg;

    if (log_replay_ongoing(overseer) == true || log_repair_ongoing(overseer) == true) {
        debug_log(2, stdout, "Log repair or replay ongoing, random op generation cancelled.");
    } else {

        // Check if queue is empty
        int queue_was_empty = 1;
        if (overseer->mfs->queue != NULL)
            queue_was_empty = 0;

        // Create random op
        data_op_s *nop = op_new();
        if (nop == NULL) {
            fprintf(stderr, "Failed to generate a new data op\n");
            if (INSTANT_FFLUSH) fflush(stderr);
            debug_log(4, stdout,
                      "End of random op callback ----------------------------------------------------------------------\n\n");

            if (DEBUG_LEVEL == 3)
                printf("\n");
            return; // Abort in case of failure
        }

        if (DEBUG_LEVEL >= 3) {
            printf("Generated a new op:\n");
            op_print(nop, stdout);
        }

        // Add it to the queue
        ops_queue_s *nqelem = ops_queue_add(nop, overseer->mfs);
        if (nqelem == NULL) {
            free(nop); // Cleanup and abort in case of failure
            fprintf(stderr, "Failed to add a new op in the queue\n");
            if (INSTANT_FFLUSH) fflush(stderr);
            debug_log(4, stdout,
                      "End of random op callback ----------------------------------------------------------------------\n\n");

            if (DEBUG_LEVEL == 3)
                printf("\n");
            return;
        }

        // Set timer for deletion
        if (server_queue_element_deletion_init(overseer, nqelem) != EXIT_SUCCESS) {
            // In case of failure and if the queue wasn't empty, we must clear possible dangling pointers before
            // cleaning up the list:
            if (!queue_was_empty) {
                for (ops_queue_s *ptr = overseer->mfs->queue; ptr == NULL; ptr = ptr->next) {
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
                    ops_queue_free_all(overseer, nqelem));
            if (INSTANT_FFLUSH) fflush(stderr);
            debug_log(4, stdout,
                      "End of random op callback ----------------------------------------------------------------------\n\n");

            if (DEBUG_LEVEL == 3)
                printf("\n");
            return;
        }

        int p_available = is_p_available(overseer->hl);

        if (!queue_was_empty) {
            debug_log(3, stdout, "Queue is not empty: holding new proposition.\n");
        } else if (!p_available) {
            debug_log(3, stdout, "Queue is empty, however P is not available: holding new proposition.\n");
        } else { // If queue was empty when checked before adding the new element and if there is an available P
            // Then transmit the proposition
            server_send_first_prop(overseer, 0);
        }
    }

    // Set next generator event
    if (server_random_ops_init(overseer) != EXIT_SUCCESS) {
        fprintf(stderr, "Fatal Error: random ops generator event couldn't be set\n");
        if (INSTANT_FFLUSH) fflush(stderr);
        exit(EXIT_FAILURE);
    }

    debug_log(4, stdout,
              "End of random op callback ----------------------------------------------------------------------\n\n");

    if (DEBUG_LEVEL == 3)
        printf("\n");
    return;
}

void server_proposition_dequeue_timeout_cb(evutil_socket_t fd, short event, void *arg) {
    // Since ops are queued in order, and since we can't guarantee that subsequent ops aren't dependent on the first
    // one, we clear the full queue to avoid incoherences caused by partial applications.
    // Since ops_queue_free_all() also removes dequeuing events tied to queue elements, there is no risk of events
    // eventually added being accidentally removed by old dequeuing events whose ops have already been removed.
    int nb_freed = ops_queue_free_all((overseer_s *) arg, ((overseer_s *) arg)->mfs->queue);
    if (DEBUG_LEVEL >= 1) {
        printf("Proposition queue element timed out: %d element(s) freed.\n", nb_freed);
        if (INSTANT_FFLUSH) fflush(stdout);
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
    errno = 0;
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

// FIXME Extension when new entry arrives, entries in the queue may relate to outdated data so invalidating them might
//  be necessary
//  attention : ops_queue_element_free doesn't free the data op but frees and removes the event