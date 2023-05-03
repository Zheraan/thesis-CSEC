//
// Created by zheraan on 22/04/23.
//

#include "server-events.h"

int server_reception_init(overseer_s *overseer){
    // Create the event related to the socket
    struct event *reception_event = event_new(overseer->eb,
                                              overseer->udp_socket,
                                             EV_READ | EV_PERSIST,
                                              heartbeat_receive,
                                              (void *) &overseer);
    if (reception_event == NULL) {
        fprintf(stderr, "Failed to create the reception event\n");
        return EXIT_FAILURE;
    }

    if (event_list_add(overseer, reception_event) == EXIT_FAILURE) {
        fprintf(stderr, "Failed to allocate the event list struct for reception event\n");
        return(EXIT_FAILURE);
    }

    // Add the event in the loop
    if (event_add(reception_event, NULL) != 0) {
        fprintf(stderr, "Failed to add the reception event\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}