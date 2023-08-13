//
// Created by zheraan on 13/08/23.
//

#include "killer.h"

int killer_init(overseer_s *overseer) {
    if (RANDOM_KILL_ENABLE == false)
        return EXIT_SUCCESS;

    debug_log(3, stdout, "- Initializing next random kill event ... ");
    // Create a non-persistent event only triggered by timeout
    struct event *nevent = evtimer_new(overseer->eb,
                                       killer_cb,
                                       (void *) overseer);
    if (nevent == NULL) {
        debug_log(0, stderr, "Failed to create the kill event\n");
        return EXIT_FAILURE;
    }

    // Random operation generator has low priority
    event_priority_set(nevent, 1);

    if (overseer->special_event != NULL) // Freeing the past killer event if any
        event_free(overseer->special_event);
    overseer->special_event = nevent;

    // Add the event in the loop
    errno = 0;
    struct timeval kill_timeout = timeout_gen(TIMEOUT_TYPE_KILL);
    if (errno == EUNKNOWN_TIMEOUT_TYPE || event_add(nevent, &kill_timeout) != 0) {
        debug_log(0, stderr, "Failed to add the killer event\n");
        return EXIT_FAILURE;
    }

    debug_log(3, stdout, "Done.\n");
    return EXIT_SUCCESS;
}

void killer_cb(evutil_socket_t fd, short event, void *arg) {
    debug_log(4,
              stdout,
              "Start of random kill callback -----------------------------------------------------------------------------\n");

    overseer_s *overseer = (overseer_s *) arg;

    uint32_t buf, modulo = 0, active_masters = 0, active_servers = 0;

    // Determine how many active servers and masters there are
    for (uint32_t i = 0; i < overseer->hl->nb_hosts; ++i) {
        if (overseer->hl->hosts[i].status == HOST_STATUS_HS ||
            overseer->hl->hosts[i].status == HOST_STATUS_P ||
            overseer->hl->hosts[i].status == HOST_STATUS_CS)
            active_masters++;
        else if (overseer->hl->hosts[i].status == HOST_STATUS_S)
            active_servers++;
    }

    // Set modulo to include server and master count based on if there are enough active masters or servers to allow
    // for a killing without breaking the invariant of having at least the majority alive
    if (active_servers > overseer->log->server_majority)
        modulo += active_servers;
    else active_servers = 0;
    if (active_masters > overseer->log->master_majority)
        modulo += active_masters;
    else active_servers = 0;

    if (modulo != 0) {
        evutil_secure_rng_get_bytes(&buf, sizeof(uint32_t));
        if (modulo == 1)
            buf = 0;
        else buf = MODULO(buf, modulo); // Set the value inside the range
    }

    if (DEBUG_LEVEL >= 3) {
        if (active_servers != 0 && active_masters != 0)
            printf("There are currently %d active masters and %d active servers; target value is %d.\n",
                   active_masters,
                   active_servers,
                   buf);
        else if (active_servers != 0)
            printf("There are currently %d active servers but not enough active masters to allow for a kill; "
                   "target value is %d.\n",
                   active_servers,
                   buf);
        else if (active_masters != 0)
            printf("There are currently %d active masters but not enough active servers to allow for a kill; "
                   "target value is %d.\n",
                   active_masters,
                   buf);
        else printf("There are currently neither enough servers nor enough masters to allow for a kill.\n");

        if (INSTANT_FFLUSH) fflush(stdout);
    }

    if (modulo != 0) {
        host_s *target;
        for (uint32_t i = 0, j = 0; i < overseer->hl->nb_hosts; i++) {
            // Select host in the list
            target = &overseer->hl->hosts[i];

            // If we determine that there are enough master nodes for one to be killed off and if the target is an active
            // master
            if (active_masters != 0 &&
                (target->status == HOST_STATUS_HS || target->status == HOST_STATUS_CS ||
                 target->status == HOST_STATUS_P)) {
                // Kill it if its number is equal to the random value we generated
                if (j == buf) {
                    if (DEBUG_LEVEL >= 1) {
                        printf("Killing off %s.\n", target->name);
                        if (INSTANT_FFLUSH) fflush(stdout);
                    }
                    kill_sendto(overseer, target->addr, target->socklen);
                    target->status = HOST_STATUS_UNREACHABLE; // Reset its status
                    break;
                } else j++; // Otherwise increase number (j only counts eligible targets, while i refers to host id)
            }

            // If we determine that there are enough server nodes for one to be killed off and if the target is an active
            // server
            if (active_servers != 0 && target->status == HOST_STATUS_S) {
                // Kill it if its number is equal to the random value we generated
                if (j == buf) {
                    if (DEBUG_LEVEL >= 1) {
                        printf("Killing off %s.\n", target->name);
                        if (INSTANT_FFLUSH) fflush(stdout);
                    }
                    kill_sendto(overseer, target->addr, target->socklen);
                    target->status = HOST_STATUS_UNREACHABLE; // Reset its status
                    break;
                } else j++; // Otherwise increase number (j only counts eligible targets, while i refers to host id)
            }
        }
    }

    if (killer_init(overseer) != EXIT_SUCCESS) {
        debug_log(0, stderr, "Failed initializing next killer event.\n");
        exit(EXIT_FAILURE);
    }

    debug_log(4,
              stdout,
              "End of random kill callback -------------------------------------------------------------------------------\n\n");
    if (DEBUG_LEVEL == 3)
        printf("\n");
    return;
}

int kill_sendto(overseer_s *overseer, struct sockaddr_in6 sockaddr, socklen_t socklen) {
    return cm_sendto(overseer, sockaddr, socklen, MSG_TYPE_KILL, FLAG_BYPASS_FUZZER);
}