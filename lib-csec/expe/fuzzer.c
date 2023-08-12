//
// Created by zheraan on 24/07/23.
//

#include "fuzzer.h"

int fuzzer_entry_init(overseer_s *overseer, enum packet_type t, union packet p, struct sockaddr_in6 addr,
                      socklen_t socklen) {
    debug_log(3, stdout, "Fuzzer start ... ");
    if (MESSAGE_DROPPING_ENABLED == 1) {
        uint8_t buf;
        evutil_secure_rng_get_bytes(&buf, sizeof(uint8_t));
        buf = MODULO(buf, 100); // Set the value inside the range

        if (buf < MESSAGE_DROP_RATE) {
            if (DEBUG_LEVEL >= 3) {
                printf("Message dropped by the fuzzer (result %d / %d%% chances).\n", buf, MESSAGE_DROP_RATE);
                if (INSTANT_FFLUSH) fflush(stdout);
            }
            return EXIT_SUCCESS;
        }
    }

    fuzzer_cache_s *nfc = malloc(sizeof(fuzzer_cache_s));
    if (nfc == NULL) {
        perror("Malloc fuzzer entry");
        return EXIT_FAILURE;
    }

    nfc->type = t;
    nfc->p = p;
    nfc->next = NULL;
    nfc->overseer = overseer;
    nfc->socklen = socklen;
    nfc->addr = addr;

    struct event *nevent = evtimer_new(overseer->eb,
                                       fuzzer_transmission_cb,
                                       (void *) nfc);
    if (nevent == NULL) {
        fprintf(stderr, "Failed to create a fuzzer timeout event\n");
        free(nfc);
        return EXIT_FAILURE;
    }

// Higher priority to prevent queue stacking
    event_priority_set(nevent, 0);
    nfc->ev = nevent;

    // Add the transmission event with the randomized timer
    struct timeval timeout = timeout_gen(TIMEOUT_TYPE_FUZZER);
    if (errno == EUNKNOWN_TIMEOUT_TYPE || event_add(nevent, &timeout) != 0) {
        fprintf(stderr, "Failed to add a fuzzer timeout event\n");
        event_free(nevent);
        free(nfc);
        return EXIT_FAILURE;
    }

    if (overseer->fc == NULL) { // If the current list is empty just add it at the start with ID 0
        nfc->id = 0;
        overseer->fc = nfc;
    } else {
        // Otherwise add it at the end
        fuzzer_cache_s *target = overseer->fc;
        do {
            if (target->next == NULL) {
                target->next = nfc;
                nfc->id = target->id + 1;
                break;
            }
            target = target->next;
        } while (target != NULL);
    }

    if (DEBUG_LEVEL >= 3) {
        printf("Done: added a new fuzzer cache entry with id %d.\n", nfc->id);
        if (INSTANT_FFLUSH) fflush(stdout);
    }

    return EXIT_SUCCESS;
}

void fuzzer_cache_free_all_aux(fuzzer_cache_s *fc) {
    if (fc == NULL)
        return;
    fuzzer_cache_free_all_aux(fc->next);
    event_free(fc->ev);
    free(fc);
    return;
}

void fuzzer_cache_free_all(overseer_s *overseer) {
    fuzzer_cache_free_all_aux(overseer->fc);
    overseer->fc = NULL;
    return;
}

int fuzzer_entry_free(overseer_s *overseer, uint32_t id) {
    fuzzer_cache_s *ptr = overseer->fc;
    if (ptr == NULL) {
        fprintf(stderr, "Error freeing cache entry: no entry with id %d in the fuzzer's cache.\n", id);
        if (INSTANT_FFLUSH) fflush(stderr);
        return EXIT_FAILURE;
    }
    if (ptr->id == id) {
        overseer->fc = ptr->next;
        event_free(ptr->ev);
        free(ptr);
        return EXIT_SUCCESS;
    }

    for (fuzzer_cache_s *tmp; ptr != NULL; ptr = tmp) {
        tmp = ptr->next;
        if (tmp->id == id) {
            ptr->next = tmp->next;
            event_free(tmp->ev);
            free(tmp);
            return EXIT_SUCCESS;
        }
    }

    fprintf(stderr, "Error freeing cache entry: no entry with id %d in the fuzzer's cache.\n", id);
    if (INSTANT_FFLUSH) fflush(stderr);
    return EXIT_FAILURE;
}

void fuzzer_transmission_cb(evutil_socket_t fd, short event, void *arg) {
    debug_log(4, stdout,
              "Start of Fuzzer transmission callback --------------------------------------------------------------------\n");
    fuzzer_cache_s *fc = arg;

    if (DEBUG_LEVEL >= 3) {
        char buf[32];
        cm_type_string(buf, fc->type == PACKET_TYPE_CM ? fc->p.cm.type : fc->p.etr.cm.type);
        printf("Fuzzer timeout: transmitting entry %d, a %s of type %d (%s) ... ",
               fc->id,
               fc->type == PACKET_TYPE_CM ? "CM" : "ETR",
               fc->type == PACKET_TYPE_CM ? fc->p.cm.type : fc->p.etr.cm.type,
               buf);
        if (INSTANT_FFLUSH) fflush(stdout);
    }

    if (fc->type == PACKET_TYPE_ETR) {
        fc->addr.sin6_port = htons(PORT_ETR);
        do {
            errno = 0;
            if (sendto(fc->overseer->socket_etr,
                       &fc->p.etr,
                       sizeof(entry_transmission_s),
                       0,
                       (const struct sockaddr *) &fc->addr,
                       fc->socklen) == -1) {
                perror("Fuzzer ETR sendto");
                if (errno != EAGAIN)
                    break;
            }
        } while (errno == EAGAIN);
    } else if (fc->type == PACKET_TYPE_CM) {
        fc->addr.sin6_port = htons(PORT_CM);
        do {
            errno = 0;
            if (sendto(fc->overseer->socket_cm,
                       &fc->p.cm,
                       sizeof(control_message_s),
                       0,
                       (const struct sockaddr *) &fc->addr,
                       fc->socklen) == -1) {
                perror("Fuzzer CM sendto");
                if (errno != EAGAIN)
                    break;
            }
        } while (errno == EAGAIN);
    }

    // Cleaning up the memory
    fuzzer_entry_free(fc->overseer, fc->id);

    debug_log(3, stdout, "Done.\n");
    debug_log(4, stdout,
              "End of Fuzzer transmission callback ----------------------------------------------------------------------\n\n");
    if (DEBUG_LEVEL == 3)
        printf("\n");
    return;
}