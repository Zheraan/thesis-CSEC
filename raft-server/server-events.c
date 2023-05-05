//
// Created by zheraan on 22/04/23.
//

#include "server-events.h"

int server_reception_init(overseer_s *overseer) {
    struct event *reception_event = event_new(overseer->eb,
                                              overseer->udp_socket,
                                              EV_READ | EV_PERSIST,
                                              heartbeat_receive_cb,
                                              (void *) &overseer);
    if (reception_event == NULL) {
        fprintf(stderr, "Failed to create the reception event\n");
        return EXIT_FAILURE;
    }

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
    // Create the event related to the socket
    struct event *nevent = event_new(overseer->eb,
                                     overseer->udp_socket,
                                     EV_TIMEOUT | EV_PERSIST,
                                     server_random_ops_cb,
                                     (void *) &overseer);
    if (nevent == NULL) {
        fprintf(stderr, "Failed to create the reception event\n");
        return EXIT_FAILURE;
    }

    if (event_list_add(overseer, nevent) == EXIT_FAILURE) {
        fprintf(stderr, "Failed to allocate the event list struct for reception event\n");
        return (EXIT_FAILURE);
    }

    // Add the event in the loop
    struct timeval *ops_timeout = timeout_random();
    if (event_add(nevent, ops_timeout) != 0) {
        fprintf(stderr, "Failed to add the reception event\n");
        return EXIT_FAILURE;
    }

    free(ops_timeout);

    return EXIT_SUCCESS;
}

void server_random_ops_cb(evutil_socket_t fd, short event, void *arg){
    // Create random op

    // Check for ops in the queue
    // - if not empty, queue it and set timer for de-queuing

    // Send proposition to P

    // Set timer for proposition Ack

    // Cache the op in queue

    return;
}

void server_proposition_dequeue_timeout_cb(evutil_socket_t fd, short event, void *arg){
    // TODO server_proposition_dequeue_timeout_cb
    // Dequeue proposition if it's still there

    return;
}