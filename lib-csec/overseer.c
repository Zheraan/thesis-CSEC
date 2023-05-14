//
// Created by zheraan on 30/04/23.
//

#include "overseer.h"

int overseer_init(overseer_s *overseer) {
    // To simplify cleanup in case of initialization failure
    overseer->mfs = NULL;
    overseer->log = NULL;
    overseer->eb = NULL;
    overseer->rt_cache = NULL;
    overseer->special_event = NULL;
    overseer->cm_reception_event = NULL;

    // Malloc the hosts list
    hosts_list_s *hl = malloc(sizeof(hosts_list_s));
    if (hl == NULL) {
        perror("Malloc hosts list");
        return EXIT_FAILURE;
    }
    overseer->hl = hl;

    // Malloc the log
    log_s *log = malloc(sizeof(log_s));
    if (hl == NULL) {
        perror("Malloc log");
        overseer_wipe(overseer);
        return EXIT_FAILURE;
    }
    overseer->log = log_init(log);

    // Init the hosts list
    if (hosts_init("hostfile.txt", overseer->hl) < 1) {
        fprintf(stderr, "Failed to parse any hosts\n");
        overseer_wipe(overseer);
        return EXIT_FAILURE;
    }

    // Create a configured event base
    struct event_base *eb = eb_new();
    if (eb == NULL) {
        fprintf(stderr, "Failed to create the event base\n");
        overseer_wipe(overseer);
        return EXIT_FAILURE;
    }
    overseer->eb = eb;

    // Create the socket
    overseer->socket_cm = socket(AF_INET6, SOCK_DGRAM, 0);
    if (overseer->socket_cm == -1 || evutil_make_socket_nonblocking(overseer->socket_cm) != 0) {
        perror("Socket cm init error");
        overseer_wipe(overseer);
        return EXIT_FAILURE;
    }

    // Set options
    int option = 1;
    if (setsockopt(overseer->socket_cm,
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   &option,
                   sizeof(option)) == -1) {
        perror("Socket cm set options");
        overseer_wipe(overseer);
        return EXIT_FAILURE;
    }

    // Bind the socket to the local address for receiving messages
    if (bind(overseer->socket_cm,
             (struct sockaddr *) &(overseer->hl->hosts[overseer->hl->localhost_id].addr),
             sizeof(overseer->hl->hosts[overseer->hl->localhost_id].addr)) != 0) {
        perror("Socket cm bind");
        overseer_wipe(overseer);
        return EXIT_FAILURE;
    }

    // Create the socket
    overseer->socket_tr = socket(AF_INET6, SOCK_DGRAM, 0);
    if (overseer->socket_tr == -1 || evutil_make_socket_nonblocking(overseer->socket_tr) != 0) {
        perror("Socket tr init error");
        overseer_wipe(overseer);
        return EXIT_FAILURE;
    }

    // Create a second address struct with same address but different port to be used for the second socket
    struct sockaddr_in6 naddr = overseer->hl->hosts[overseer->hl->localhost_id].addr;
    naddr.sin6_port = htons(35008);

    // Set options
    if (setsockopt(overseer->socket_tr,
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   &option,
                   sizeof(option)) == -1) {
        perror("Socket tr set options");
        overseer_wipe(overseer);
        return EXIT_FAILURE;
    }

    // Bind the socket to the local address for receiving messages
    if (bind(overseer->socket_tr,
             (struct sockaddr *) &naddr,
             sizeof(overseer->hl->hosts[overseer->hl->localhost_id].addr)) != 0) {
        perror("Socket tr bind");
        overseer_wipe(overseer);
        return EXIT_FAILURE;
    }

    // Allocate and initialize the mocked filesystem
    mocked_fs_s *nmfs = malloc(sizeof(mocked_fs_s));
    if (nmfs == NULL) {
        perror("malloc mfs");
        overseer_wipe(overseer);
        return EXIT_FAILURE;
    }
    memset(nmfs->array, 0, sizeof(int[MOCKED_FS_ARRAY_ROWS][MOCKED_FS_ARRAY_COLUMNS]));
    nmfs->nb_ops = 0;
    nmfs->retransmission_attempts = 0;
    nmfs->op_cache = NULL;
    nmfs->event_cache = NULL;
    nmfs->queue = NULL;
    overseer->mfs = nmfs;

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
    fflush(stderr);
    if (DEBUG_LEVEL >= 3) {
        printf("Beginning cleanup ...\n"
               "- Log ... ");
        fflush(stdout);
    }
    // If the local node is a Master node, it is necessary to free the replication arrays in the entries as well
    if (overseer->hl != NULL && overseer->hl->hosts[overseer->hl->localhost_id].type == NODE_TYPE_M) {
        if (overseer->log != NULL)
            log_free(overseer->log);
    } else {
        free(overseer->log);
    }
    if (DEBUG_LEVEL >= 3) {
        printf("Done.\n"
               "- Hosts-list ... ");
        fflush(stdout);
    }

    if (overseer->hl != NULL)
        free(overseer->hl);
    if (DEBUG_LEVEL >= 3) {
        printf("Done.\n"
               "- Mocked filesystem ... ");
        fflush(stdout);
    }

    if (overseer->mfs != NULL) {
        ops_queue_free_all(overseer, overseer->mfs->queue);
        mfs_free_cache(overseer);
        printf("bruh");
        fflush(stdout);
        free(overseer->mfs);
    }
    if (DEBUG_LEVEL >= 3) {
        printf("Done.\n"
               "- Event-list ... ");
        fflush(stdout);
    }

    rt_cache_free_all(overseer);
    if (DEBUG_LEVEL >= 3) {
        printf("Done.\n"
               "- Event-base ... ");
        fflush(stdout);
    }

    if (overseer->eb != NULL)
        event_base_free(overseer->eb);
    if (DEBUG_LEVEL >= 3) {
        printf("Done.\n"
               "- Closing sockets ... ");
        fflush(stdout);
    }

    if (overseer->special_event != NULL)
        event_free(overseer->special_event);
    if (overseer->cm_reception_event != NULL)
        event_free(overseer->cm_reception_event);

    if (overseer->socket_cm != 0 && close(overseer->socket_cm) != 0)
        perror("Error closing communication socket file descriptor");
    if (overseer->socket_tr != 0 && close(overseer->socket_tr) != 0)
        perror("Error closing communication socket file descriptor");
    if (DEBUG_LEVEL >= 3) {
        printf("Done.\n"
               "Cleanup finished.\n");
        fflush(stdout);
    }
    return;
}