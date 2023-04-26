
#include <event2/event.h>
#include <unistd.h>
#include "hosts-list/hosts-list.h"
#include "raft-comms/heartbeat.h"

#define ECFG_MAX_DISPATCH_USEC 2000
#define ECFG_MAX_CALLBACKS 5

#define LISTEN_MAX_CONNEXIONS 16


void do_receive(evutil_socket_t listener, short event, void *arg) {
    heartbeat_s hb;
    struct sockaddr_in6 sender;
    socklen_t sender_len;

    if (recvfrom(listener, &hb, sizeof(heartbeat_s), 0, &sender, &sender_len) == -1)
        perror("recvfrom");

    print_hb(&hb, stdout);
    return;
}

int main() {
    hosts_list_s hl;
    //log_s log;

    // Init the hosts list
    if (init_hosts("hostfile.txt", &hl) < 1) {
        fprintf(stderr, "Failed to parse any hosts");
        exit(EXIT_FAILURE);
    }

    struct timeval max_dispatch_interval = {
            .tv_sec = 0,
            .tv_usec = ECFG_MAX_DISPATCH_USEC
    };

    // Config for the event base
    struct event_config *ecfg = event_config_new();
    if (ecfg == NULL ||
        event_config_set_flag(ecfg, EVENT_BASE_FLAG_NOLOCK) != 0 ||
        event_config_set_max_dispatch_interval(ecfg,
                                               &max_dispatch_interval,
                                               ECFG_MAX_CALLBACKS,
                                               0) != 0) {
        perror("Event config init error");
        event_config_free(ecfg);
        exit(EXIT_FAILURE);
    }

    // Init the event base
    struct event_base *eb = event_base_new_with_config(ecfg);
    if (eb == NULL || event_base_priority_init(eb, 2) != 0) {
        perror("Event base init error");
        event_config_free(ecfg);
        event_base_free(eb);
        exit(EXIT_FAILURE);
    }

    // Create the socket
    evutil_socket_t listener = socket(AF_INET6, SOCK_DGRAM, 0);
    if (listener == -1 || evutil_make_socket_nonblocking(listener) != 0) {
        perror("Socket init error");
        event_config_free(ecfg);
        event_base_free(eb);
        exit(EXIT_FAILURE);
    }

    // Bind the socket
    if (bind(listener, (struct sockaddr_in6 *) &(hl.hosts[0].addr), sizeof(hl.hosts[0].addr)) != 0) {
        perror("Socket bind");
        event_config_free(ecfg);
        event_base_free(eb);
        exit(EXIT_FAILURE);
    }

    // Listen to the socket
    if (listen(listener, LISTEN_MAX_CONNEXIONS) != 0) {
        perror("Socket listen");
        event_config_free(ecfg);
        event_base_free(eb);
        exit(EXIT_FAILURE);
    }

    struct event *listener_event = event_new(eb, listener, EV_READ | EV_PERSIST, do_receive, (void *) eb);
    if (listener_event == NULL) {
        perror("Socket init error");
        event_config_free(ecfg);
        event_base_free(eb);
        close(listener);
        exit(EXIT_FAILURE);
    }

    // Free Any pointers
    // Close sockets
    // Free any running events first
    event_free(listener_event);
    event_config_free(ecfg);
    event_base_free(eb);
    close(listener);

    // Initialize log
    // Initialize term with special start value
    // Parse hostfile
    // Init active hosts list and state
    // Create corresponding sockets
    // Create heartbeat channels
    // Create heartbeat cluster-monitor channels
    // Create manual input channel

    /* Event loop:
     *
     * ------------- Heartbeats ---------------------------------------------------
     *
     * > P heartbeat reception
     *      - as result of new P (check term)
     *          > update hosts list state
     *              - prevent possibility of DOS
     *              - maybe through checking if P is previous HS ?
     *          > send queued element if any
     *      > Check term
     *          > Ack with correct term and who is P if wrong
     *      > check match index
     *          > update if out of date
     *      > check next index
     *          > if it doesn't match, log entries are missing
     *          > ask for next missing log entry (include replication index)
     *      > update log entry state if any (one by one ?)
     *      > Check, hosts list if there is, flags, status
     *          > update hosts if need be
     *      > ack back with log entry replication notice if any
     *      > Normal ack back if nothing was wrong
     *      > Reset heartbeat timeout
     *
     * > Receive special message from outdated masters on who is P
     *      - May happen in case local node sent a message to a node that is no longer P
     *      > Update hosts list
     *
     * > Missing log entry received
     *      > Update log accordingly
     *      > Check if next index matches
     *          > If not, ask for next missing log entry (include pending index)
     *
     *
     *
     * ------------- Log entries --------------------------------------------------
     *
     * > Data Op reception
     *      > Create entry with created status
     *      - Entry queue not empty
     *          > queue it up
     *      - Entry queue empty
     *          > Keep it cached
     *          - There is an active P
     *              > Send proposition to P
     *          - There is no P
     *              > Queue it up with no P flag
     *              > Set timeout for deletion but not for retransmission
     *
     * > Replicate entry order
     *      - Term invalid
     *          > Ack with right term
     *      - Indexes invalid
     *          > Ask for needed entries if any
     *          > Fix the incoherences otherwise
     *      - All valid
     *          > create corresponding pending entry
     *          > Ack back
     *
     * > Log entry proposal ack received
     *      - Special flag for invalid indexes and/or terms
     *          > Abandon created entry
     *          > Apply fix, ask for next one if any
     *      - Rejected due to "proposed" state
     *          > Queue the cached proposition
     *      - Validated
     *          > Make entry pending
     *
     * > Commit entry order
     *      - Term invalid
     *          > Ack with right term
     *      - Indexes invalid
     *          > Ask for needed entries if any
     *          > Fix the incoherences otherwise
     *      - All valid
     *          > commit corresponding pending entry
     *          > Ack back
     *          > Apply entry through data op module
     *
     *
     *
     * ------------- History snapshotting and message counter reset ---------------
     *
     *
     *
     * ------------- Manual input channel -----------------------------------------
     *
     *
     *
     * ------------- Entry queue --------------------------------------------------
     *
     * > Deletion timeout
     *
     * > Retransmission timeout
     *
     *
     *
     * ------------- Network topology ---------------------------------------------
     *
     *
     *      
     *
    */
    return EXIT_SUCCESS;
}
