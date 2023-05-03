//
// Created by zheraan on 30/04/23.
//

#include "overseer.h"

int overseer_init(overseer_s *overseer) {
    // Malloc the hosts list
    hosts_list_s *hl = malloc(sizeof(hosts_list_s));
    if (hl == NULL) {
        perror("Malloc hosts list");
        return (EXIT_FAILURE);
    }
    overseer->hl = hl;

    // Malloc the log
    log_s *log = malloc(sizeof(log_s));
    if (hl == NULL) {
        perror("Malloc log");
        free(hl);
        return (EXIT_FAILURE);
    }
    overseer->log = log_init(log);

    // Init the hosts list
    if (hosts_init("hostfile.txt", overseer->hl) < 1) {
        fprintf(stderr, "Failed to parse any hosts\n");
        free(hl);
        free(log);
        return (EXIT_FAILURE);
    }

    // Create a configured event base
    struct event_base *eb = eb_new();
    if (eb == NULL) {
        fprintf(stderr, "Failed to create the event base\n");
        free(hl);
        free(log);
        return (EXIT_FAILURE);
    }
    overseer->eb = eb;

    // Create the socket (since sockets are int aliases it's not necessary to allocate on the stack)
    overseer->udp_socket = socket(AF_INET6, SOCK_DGRAM, 0);
    if (overseer->udp_socket == -1 || evutil_make_socket_nonblocking(overseer->udp_socket) != 0) {
        perror("Socket init error");
        free(hl);
        free(log);
        event_base_free(eb);
        return (EXIT_FAILURE);
    }

    // Bind the socket to the local address for receiving messages
    if (bind(overseer->udp_socket,
             (struct sockaddr *) &(overseer->hl->hosts[overseer->hl->localhost_id].addr),
             sizeof(overseer->hl->hosts[overseer->hl->localhost_id].addr)) != 0) {
        perror("Socket bind");
        overseer_wipe(overseer);
        return (EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}

struct event_base *eb_new() {
    struct event_config *ecfg = event_config_new();
    if (ecfg == NULL) {
        fprintf(stderr, "Failed to create the event base config\n");
        return (NULL);
    }

    struct timeval max_dispatch_interval = {
            .tv_sec = 0,
            .tv_usec = ECFG_MAX_DISPATCH_USEC
    };

    // Init the event base config
    if (ecfg == NULL ||
        event_config_set_flag(ecfg, EVENT_BASE_FLAG_NOLOCK) != 0 ||
        event_config_set_max_dispatch_interval(ecfg,
                                               &max_dispatch_interval,
                                               ECFG_MAX_CALLBACKS,
                                               0) != 0) {
        fprintf(stderr, "Event config init error\n");
        event_config_free(ecfg);
        return NULL;
    }

    // Init the event base
    struct event_base *eb = event_base_new_with_config(ecfg);
    if (eb == NULL || event_base_priority_init(eb, 2) != 0) {
        fprintf(stderr, "Event base init error\n");
        event_config_free(ecfg);
        return NULL;
    }
    event_config_free(ecfg);
    return eb;
}

void overseer_wipe(overseer_s *overseer) {
    free(overseer->hl);
    free(overseer->log);
    event_list_free(overseer->el);
    event_base_free(overseer->eb);
    if (close(overseer->udp_socket) != 0)
        perror("Error closing communication socket file descriptor");
    return;
}