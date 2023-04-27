
#include <event2/event.h>
#include <unistd.h>
#include "hosts-list/hosts-list.h"
#include "raft-comms/heartbeat.h"
#include "overseer.h"

#define ECFG_MAX_DISPATCH_USEC 2000
#define ECFG_MAX_CALLBACKS 5

#define LISTEN_MAX_CONNEXIONS 16

static int message_counter = 0;
overseer_s overseer;

void do_send(evutil_socket_t sender, short event, void *arg) {
    heartbeat_s hb = {
            .host_id = 0,
            .status = HOST_STATUS_P,
            .flags = 0,
            .next_index = 0,
            .rep_index = 0,
            .match_index = 0,
            .commit_index = 0,
            .term = 0
    };

    struct sockaddr_in6 receiver = (((overseer_s *) arg)->hl->hosts[1].addr);
    socklen_t receiver_len= (((overseer_s *) arg)->hl->hosts[1].addr_len);

    char buf[256];
    evutil_inet_ntop(AF_INET6,&(receiver.sin6_addr),buf, 256);
    printf("Sending to %s the following heartbeat:\n", buf);
    print_hb(&hb, stdout);

    if (sendto(sender, &hb, sizeof(heartbeat_s), 0, &receiver, receiver_len) == -1)
        perror("sendto");

    if (++message_counter >= 3) {
        event_base_loopbreak(((overseer_s *) arg)->eb);
    }

    return;
}

int main() {
    hosts_list_s hl;
    log_s log;
    overseer.hl = &hl;
    overseer.log = &log;

    // Init the hosts list
    if (init_hosts("hostfile.txt", overseer.hl) < 1) {
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
        fprintf(stderr, "Event config init error");
        event_config_free(ecfg);
        exit(EXIT_FAILURE);
    }

    // Init the event base
    struct event_base *eb = event_base_new_with_config(ecfg);
    if (eb == NULL || event_base_priority_init(eb, 2) != 0) {
        fprintf(stderr, "Event base init error");
        event_config_free(ecfg);
        event_base_free(eb);
        exit(EXIT_FAILURE);
    }
    overseer.eb = eb;

    // Create the socket
    evutil_socket_t sender = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sender == -1 || evutil_make_socket_nonblocking(sender) != 0) {
        perror("Socket init error");
        event_config_free(ecfg);
        event_base_free(eb);
        exit(EXIT_FAILURE);
    }
/*
    // Bind the socket
    if (bind(sender,
             (struct sockaddr_in6 *) &(overseer.hl->hosts[0].addr),
             sizeof(overseer.hl->hosts[0].addr)) != 0) {
        perror("Socket bind");
        event_config_free(ecfg);
        event_base_free(eb);
        exit(EXIT_FAILURE);
    }*/

    // Create the event related to the socket
    struct event *sender_event = event_new(eb, sender, EV_PERSIST | EV_TIMEOUT, do_send, (void *) &overseer);
    if (sender_event == NULL) {
        fprintf(stderr, "Failed to create an event");
        event_config_free(ecfg);
        event_base_free(eb);
        close(sender);
        exit(EXIT_FAILURE);
    }

    struct timeval sender_timeout = {
            .tv_sec = 5,
            .tv_usec = 0
    };
    // Add the event in the loop
    if (event_add(sender_event, &sender_timeout) != 0) {
        fprintf(stderr, "Failed to add an event");
        event_config_free(ecfg);
        event_base_free(eb);
        close(sender);
        exit(EXIT_FAILURE);
    }

    // Run the loop
    int rv = event_base_dispatch(eb);
    if (rv == 1)
        fprintf(stdout, "No events pending or active in the base, cleaning up and finishing...");
    else if (rv == -1)
        fprintf(stderr, "An error occurred while running the loop, cleaning up and finishing...");
    else fprintf(stdout, "Cleaning up and finishing...");

    // Free any running events first
    event_free(sender_event);
    // Free Any pointers
    event_config_free(ecfg);
    event_base_free(eb);
    // Close sockets
    if (close(sender) != 0)
        perror("Error closing socket file descriptor");

    // Initialize log
    // Initialize term with special start value
    // Parse hostfile
    // Init active hosts list and state
    // Create corresponding sockets
    // Create heartbeat channels
    // Create heartbeat cluster-monitor channels
    // Create manual input channel
    // Create consensus lead channels
    // Create elections channels
    // Create control channel
    // Create close sync channels
    // Create CS-side backup channels

    /* Events:
     *
     * ------------- P Takeover ---------------------------------------------------
     *
     * > P heartbeat timeout as HS
     *      > Transition to P candidate
     *      > Queries other masters for reachability and for who is master with term number
     *      > Set query timeout
     *
     * > Reachability query timeout (as P candidate mode)
     *      > Check if queries indicate right term and right master
     *          > Step down if local terms are outdated compared to the majority of answers
     *          > Update hosts and indexes accordingly
     *      ? transition to partition mode if received less than half queries back
     *      - Majority of negative reachability
     *          ?! transitions to P status
     *      - Majority of positive reachability
     *          > rollback to HS
     *
     * > Reachability query reception as CS
     *      - Term is outdated
     *          > Ack back with hosts list and terms
     *      > Ack back
     *      > Query master for reachability
     *
     * > Reachability query reception as P
     *      > Ack back with special flag
     *
     * > Reachability query answer reception as HS
     *      > Update with result
     *          - Majority attained: unreachable
     *              ! Transition to P
     *          - Majority attained: reachable
     *              > Rollback to HS, query P for reachability
     *                  ? Partition mode if still unreachable after several attempts
     *
     * > Transition to P
     *      > transitions to P status, sends heartbeat as new P with special flag
     *          > remove any HS or CS related events
     *          > create P related events
     *          > Set non-proposed state
     *          > remove HS-side backup channels
     *
     *
     *
     * ------------- HS Election --------------------------------------------------
     *
     * > HS election bid reception
     *      > check term, ack back with hosts status and terms if lower than current
     *      > otherwise respond favorably
     *          X find solution to unlimited bid requests with increasing term numbers, possibility of DOS
     *      > set term timeout
     *
     * > HS election
     *      - as result of old HS takeover as P (trigger through callback of heartbeat)
     *      - because of deployment phase, trigger through setup
     *      - as result of HS heartbeat timeout
     *          > update hosts state
     *      > enter candidate state, increase term and vote for self
     *      > send bid to all, set term timeout
     *          > otherwise starts new term at timeout
     *
     * > Election bid answer
     *      > Check term, ignore if wrong
     *      - positive
     *          > increase positive vote count
     *          - If majority reached
     *              > transitions to HS, sends heartbeat as new HS
     *                  > remove any CS related events
     *                  > create HS related events
     *                  > remove CS-side backup channels
     *                  > create HS-side backup channels
     *      - negative
     *          > increase negative vote count
     *
     * > HS term timeout
     *      - first time
     *          > try reaching again unreachable nodes
     *          > reset timeout
     *      - second time
     *          ! HS election
     *      X Find a way to identify when partitioned (maybe when receiving no ack back?)
     *
     *
     *
     * ------------- P Handling of log entry proposition --------------------------
     *
     * > Receive log entry proposal
     *      - indexes and term are invalid
     *          > heartbeat back with correct indexes and terms and special flag
     *          > send the oldest log entry to be repaired if any
     *      - indexes and term are valid
     *          - if under proposed state
     *              > ack back with negative answer
     *          - otherwise
     *              > ack proposition
     *              > enter proposed state and set timeout
     *              > create pending proposition, and broadcast to servers for voting (replicate_entry)
     *
     * > Timeout of ongoing election ("proposed state")
     *      > Check majority of votes
     *          - Reached
     *              > Check if entry is marked as safe
     *                  - Yes
     *                      > Commit locally
     *                      > send commit order
     *                      > Set timer for commit ack
     *          - Not reached
     *              > Reiterate update to servers not responding
     *      > Exit "proposed" state
     *
     * > Receive replication Ack
     *      > Verify indexes
     *      > Update replication status
     *      > Check majority of votes
     *          - Reached
     *              > Check if entry is marked as safe
     *                  - Yes
     *                      > Commit locally
     *                      > send commit order
     *                      > Set timer for commit ack
     *      > Exit "proposed" state
     *
     * > Log entry commit ack
     *      > update commit data
     *      > remove retransmission event
     *
     * > Log entry safe message from HS
     *      > mark entry as safe
     *      > check majority status on entry
     *          - Reached
     *              > Broadcast commit order
     *
     *
     *
     * ------------- HS and CS entry replication ----------------------------------
     *
     * > Replicate entry order
     *      - Term invalid
     *          > Ack with right term
     *      - Indexes invalid
     *          > Ask for needed entries if any
     *              - from P if HS
     *              - from HS if CS
     *          > Fix the incoherences otherwise
     *      - All valid
     *          > create corresponding pending entry
     *              - If local is HS
     *                  > Set entry's master pool replication count to 1
     *          > Ack back
     *
     * > Commit entry order
     *      - Term invalid
     *          > Ack with right term
     *      - Indexes invalid
     *          > Ask for needed entries if any
     *              - from P if HS
     *              - from HS if CS
     *          > Fix the incoherences otherwise
     *      - All valid
     *          > commit corresponding pending entry
     *          > Ack back
     *
     * > Replicate entry ack (as HS from CS)
     *      > Increase entry's master pool replication count
     *          - Majority attained
     *              > Mark as safe
     *              > Signify entry is safe to P
     *
     *
     *
     * ------------- Heartbeats and log repair ------------------------------------
     *
     * > Periodic standard heartbeat (P to servers and HS, HS to CS)
     *      > Include:
     *          - Provenance
     *          - Flags (status, special flags)
     *          - Indexes
     *          - Terms
     *          ? Hosts list and their status
     *
     * > Heartbeat reception
     *      - as result of new P (special flag)
     *          > update hosts list state
     *          ! HS election
     *      - as result of new HS (special flag)
     *          > update hosts list state
     *      - new host special flag (recovery from crash or at startup)
     *          > update hosts list & state
     *          > ack back with log data and full hosts list
     *      - if from other node operating in the same mode (P or HS)
     *          ? Compare terms and step down from role if necessary
     *          - Shouldn't happen with partition mode, though important for security
     *      - If in P candidate mode
     *          >rollback to HS
     *      > compare terms and indexes
     *          - if indexes wrong
     *              > heartbeat ack with missing log entry message
     *                  - possibility of erroneous log entries, check term from entries past match index
     *              > update accordingly
     *      > compare hosts lists
     *          > adjust if necessary
     *      > ack back with eventual log status changes
     *
     * > Heartbeat Ack reception
     *      > Update log entry replication if any
     *          > check if commit status makes an entry safe or reach majority
     *              - trigger related events if so
     *      - Contains missing log entry message
     *          ? Check for coherence with match index
     *          > Send corresponding entry with current indexes
     *
     * > Heartbeat reception from P as HS
     *      > Reset heartbeat timeout
     *      > Ack back
     *
     * > HS and CS: if receive message with wrong term addressed to P with update on who is P
     *      > Heartbeat back with update on who is P
     *
     * > Heartbeat reception from HS as CS
     *      > Check
     *      > Reset heartbeat timeout
     *
     * > Heartbeat reception from P as CS
     *      > Update hosts list
     *      > Special flag if queried from reachability
     *          > Answer HS query with positive reachability
     *
     *
     *
     * ------------- P-HS close sync ----------------------------------------------
     *
     *
     *
     *
     * ------------- HS-CS backup sync --------------------------------------------
     *
     *
     *
     * ------------- History snapshotting and message counter reset ---------------
     *
     *
     *
     * ------------- Partition mode -----------------------------------------------
     *
     * ? Transition back to CS
     * ? Reset term to special partition value (>init value, <any online value)
     *
     *
     *
     * ------------- Manual input channel -----------------------------------------
     *
     *
     *
     * ------------- Network topology ---------------------------------------------
     *
     *
     *
    */
    return EXIT_SUCCESS;
}
