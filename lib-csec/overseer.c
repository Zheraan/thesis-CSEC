//
// Created by zheraan on 30/04/23.
//

#include "overseer.h"

int overseer_init(overseer_s *overseer, const char *hostfile) {
    // To simplify cleanup in case of initialization failure
    overseer->mfs = NULL;
    overseer->log = NULL;
    overseer->eb = NULL;
    overseer->rtc = NULL;
    overseer->special_event = NULL;
    overseer->cm_reception_event = NULL;
    overseer->etr_reception_event = NULL;
    overseer->p_liveness_event = NULL;
    overseer->es = NULL;
    overseer->fc = NULL;
    overseer->socket_cm = 0;
    overseer->socket_etr = 0;
    overseer->rtc_index = 1;
    overseer->rtc_number = 0;

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
    log->master_majority = hl->nb_masters / 2 + 1;
    log->server_majority = hl->nb_servers / 2 + 1;

    // Init the hosts list
    if (hosts_init(hostfile, overseer->hl) < 1) {
        if (errno == 0)
            fprintf(stderr, "Fatal error: Failed to parse any hosts.\n");
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
    overseer->socket_etr = socket(AF_INET6, SOCK_DGRAM, 0);
    if (overseer->socket_etr == -1 || evutil_make_socket_nonblocking(overseer->socket_etr) != 0) {
        perror("Socket etr init error");
        overseer_wipe(overseer);
        return EXIT_FAILURE;
    }

    // Create a second address struct with same address but different port to be used for the second socket
    struct sockaddr_in6 naddr = overseer->hl->hosts[overseer->hl->localhost_id].addr;
    naddr.sin6_port = htons(PORT_ETR);

    // Set options
    if (setsockopt(overseer->socket_etr,
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   &option,
                   sizeof(option)) == -1) {
        perror("Socket etr set options");
        overseer_wipe(overseer);
        return EXIT_FAILURE;
    }

    // Bind the socket to the local address for receiving messages
    if (bind(overseer->socket_etr,
             (struct sockaddr *) &naddr,
             sizeof(overseer->hl->hosts[overseer->hl->localhost_id].addr)) != 0) {
        perror("Socket etr bind");
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
    nmfs->queue = NULL;
    overseer->mfs = nmfs;

    // If local host is a master node
    if (overseer->hl->hosts[overseer->hl->localhost_id].type == NODE_TYPE_M) {
        // Allocate and initialize the election state struct
        election_state_s *nes = malloc(sizeof(election_state_s));
        if (nes == NULL) {
            perror("malloc election state");
            overseer_wipe(overseer);
            return EXIT_FAILURE;
        }
        nes->election_round_event = NULL;
        nes->candidacy = CANDIDACY_NONE;
        nes->vote_count = 0;
        nes->last_voted_bid = 0;
        nes->bid_number = 0;
        overseer->es = nes;
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
    if (INSTANT_FFLUSH) fflush(stderr);

    debug_log(3, stdout, "Beginning cleanup ...\n- Log ... ");
    free(overseer->log);

    debug_log(3, stdout, "Done.\n- Hosts-list ... ");
    if (overseer->hl != NULL)
        free(overseer->hl);

    debug_log(3, stdout, "Done.\n- Mocked filesystem ... ");
    if (overseer->mfs != NULL) {
        ops_queue_free_all(overseer, overseer->mfs->queue);
        free(overseer->mfs);
    }

    debug_log(3, stdout, "Done.\n- Event-list ... ");
    rtc_free_all(overseer);

    debug_log(3, stdout, "Done.\n- Event-base ... ");
    if (overseer->eb != NULL)
        event_base_free(overseer->eb);
    if (overseer->special_event != NULL)
        event_free(overseer->special_event);
    if (overseer->cm_reception_event != NULL)
        event_free(overseer->cm_reception_event);
    if (overseer->etr_reception_event != NULL)
        event_free(overseer->etr_reception_event);
    if (overseer->p_liveness_event != NULL)
        event_free(overseer->p_liveness_event);

    if (overseer->es != NULL) {
        debug_log(3, stdout, "Done.\n- Election state ... ");
        if (overseer->es->election_round_event != NULL)
            event_free(overseer->es->election_round_event);
        free(overseer->es);
    }

    if (overseer->fc != NULL) {
        debug_log(3, stdout, "Done.\n- Fuzzer cache ... ");
        fuzzer_cache_free_all(overseer);
    }

    debug_log(3, stdout, "Done.\n- Closing sockets ... ");
    if (overseer->socket_cm != 0 && close(overseer->socket_cm) != 0)
        perror("Error closing communication socket file descriptor");
    if (overseer->socket_etr != 0 && close(overseer->socket_etr) != 0)
        perror("Error closing communication socket file descriptor");

    debug_log(3, stdout, "Done.\nCleanup finished.\n");
    return;
}

void debug_log(uint8_t level, FILE *stream, const char *string) {
    if (DEBUG_LEVEL >= level) {
        fprintf(stream, "%s", string);
        if (INSTANT_FFLUSH) fflush(stream);
    }
    return;
}