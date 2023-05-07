//
// Created by zheraan on 22/04/23.
//

#include "server-events.h"

int server_reception_init(overseer_s *overseer) {
    struct event *reception_event = event_new(overseer->eb,
                                              overseer->socket_cm,
                                              EV_READ | EV_PERSIST,
                                              cm_receive_cb,
                                              (void *) overseer);
    if (reception_event == NULL) {
        fprintf(stderr, "Failed to create the reception event\n");
        return EXIT_FAILURE;
    }

    // Message reception has low priority
    event_priority_set(reception_event, 1);

    if (event_list_add(overseer, reception_event) == EXIT_FAILURE) {
        fprintf(stderr, "Failed to allocate the event list struct for reception event\n");
        return (EXIT_FAILURE);
    }

    // Add the event in the loop
    if (event_add(reception_event, NULL) != 0) {
        fprintf(stderr, "Failed to add the reception event\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

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
    if(nqelem == NULL) {
        free(nop); // Cleanup and abort in case of failure
        return;
    }

    // Set timer for deletion
    if(server_queue_element_deletion_init((overseer_s *) arg, nqelem) == EXIT_FAILURE){
        // In case of failure and if the queue wasn't empty, we must clear possible dangling pointers before
        // cleaning up the list:
        if(!queue_was_empty) {
            for (ops_queue_s *ptr = ((overseer_s *) arg)->mfs->queue; ptr == NULL; ptr = ptr->next) {
                if(ptr->next == nqelem){
                    ptr->next = NULL; // Clear pointer
                    break;
                }
            }
        }
        ops_queue_free_all(nqelem);// Cleanup and abort in case of failure, including subsequent elements
        return;
    }

    // If queue was empty when checked before adding the new element and there is an available P
    if (queue_was_empty && is_p_available(((overseer_s *) arg)->hl)) {
        if(server_proposition_send_init((overseer_s *) arg, nqelem) == EXIT_FAILURE){
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

int server_queue_element_deletion_init(overseer_s *overseer, ops_queue_s *element){
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

int server_proposition_send_init(overseer_s *overseer, ops_queue_s *element){
    // Copy the data op in the queue element into the cache in case it is removed before retransmission
    data_op_s *nop = malloc(sizeof(data_op_s));
    if (nop == NULL) {
        perror("malloc data op for caching");
        return EXIT_FAILURE; // Abort in case of failure
    }

    // send proposition to P
    // TODO finish this
    // set timer for retransmission in case of no ack
    // TODO when ack comes, remove retransmission event
    return EXIT_SUCCESS;
}

void server_proposition_retransmission_cb(evutil_socket_t fd, short event, void *arg){
    // Retransmit cached
    // Empty cache
    return;
}

// TODO Retransmit cached element

// TODO when data op commit order arrives, create pending entry, and fire the queue element deletion
// Then free the event
// attention : ops_queue_element_free doesn't free the data op but frees and removes the event