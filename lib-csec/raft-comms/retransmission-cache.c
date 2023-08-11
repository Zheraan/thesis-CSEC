//
// Created by zheraan on 03/05/23.
//

#include "retransmission-cache.h"

void rtc_free(retransmission_cache_s *rtc) {
    if (rtc == NULL)
        return;
    if (rtc->ev != NULL) {
        event_free(rtc->ev);
    } else {
        debug_log(2,
                  stderr,
                  "Attempting to free retransmission cache but retransmission event pointer is not set.\n");
    }
    if (rtc->etr != NULL)
        free(rtc->etr);
    if (rtc->id != 0)
        rtc->overseer->rtc_number--;
    free(rtc);
    return;
}

void rtc_free_all(overseer_s *overseer) {
    if (overseer->rtc != NULL) {
        retransmission_cache_s *next;
        for (retransmission_cache_s *cur = overseer->rtc; cur != NULL; cur = next) {
            next = cur->next;
            rtc_free(cur);
        }
        overseer->rtc = NULL; // Avoid dangling pointer
    }
    overseer->log->fix_conversation = 0;
    overseer->log->fix_type = FIX_TYPE_NONE;
    overseer->rtc_number = 0;
    return;
}

uint32_t rtc_add_new(overseer_s *overseer,
                     uint8_t rt_attempts,
                     struct sockaddr_in6 sockaddr,
                     socklen_t socklen,
                     enum message_type type,
                     entry_transmission_s *etr,
                     uint32_t ack_back) {
    debug_log(4, stdout, "- Creating a new retransmission cache element ... ");

    retransmission_cache_s *nrtc = malloc(sizeof(retransmission_cache_s));
    if (nrtc == NULL) {
        perror("Malloc retransmission cache entry");
        if (INSTANT_FFLUSH) fflush(stderr);
        return 0;
    }

    // Initialize cache
    nrtc->overseer = overseer;
    nrtc->socklen = socklen;
    nrtc->addr = sockaddr;
    nrtc->max_attempts = rt_attempts;
    nrtc->cur_attempts = 0;
    nrtc->type = type;
    nrtc->next = NULL;
    nrtc->etr = etr;
    nrtc->ev = NULL;
    nrtc->ack_back = ack_back;
    nrtc->id = 0; // To check if it's been added in the list when the free function is used

    event_callback_fn callback;
    if (etr == NULL)
        callback = cm_retransmission_cb;
    else
        callback = etr_retransmission_cb;

    // Create a non-persistent event triggered only by timeout
    struct event *nevent = evtimer_new(overseer->eb,
                                       callback,
                                       (void *) nrtc);
    if (nevent == NULL) {
        fprintf(stderr, "Failed to create a retransmission event\n");
        fflush(stderr);
        rtc_free(nrtc);
        return 0;
    }

    // CM retransmission has low priority
    event_priority_set(nevent, 1);

    // We add the retransmission event to the event op_cache to keep track of it for eventual deletion and free
    nrtc->ev = nevent;

    // Add the event in the loop with ACK timeout (if local host doesn't receive an ack with the ID of the message
    // before the timeout, retransmission will happen)
    errno = 0;
    struct timeval retransmission_timeout = timeout_gen(TIMEOUT_TYPE_ACK);
    if (errno == EUNKNOWN_TIMEOUT_TYPE || event_add(nevent, &retransmission_timeout) != 0) {
        fprintf(stderr, "Failed to add a retransmission event\n");
        fflush(stderr);
        rtc_free(nrtc);
        return 0;
    }

    // Use strictly increasing cache entry indexes, apart from a reset to 1 after the maximum value has been hit. That
    // way, chances of collision are low.
    // ID 0 is forbidden as a value of 0 is used in the ack_reference field of messages to signify that it does not
    // require acknowledgement.
    if (overseer->rtc == NULL) {
        overseer->rtc = nrtc;
        nrtc->id = overseer->rtc_index;
        overseer->rtc_index += 1;
        overseer->rtc_number += 1;
        if (overseer->rtc_index >= 0xFFFFFFFF)
            overseer->rtc_index = 1;
    } else {
        // Otherwise insert the cache at the end of the list
        retransmission_cache_s *iterator = overseer->rtc;
        do {
            if (iterator->next == NULL) {
                iterator->next = nrtc;
                nrtc->id = overseer->rtc_index;
                overseer->rtc_index += 1;
                overseer->rtc_number += 1;
                if (overseer->rtc_index >= 0xFFFFFFFF)
                    overseer->rtc_index = 1;
                break;
            }
            iterator = iterator->next;
        } while (iterator != NULL);
    }

    if (DEBUG_LEVEL >= 4) {
        printf("Done (%d entr%s currently in the cache).\n",
               overseer->rtc_number,
               overseer->rtc_number == 1 ? "y" : "ies");
        if (INSTANT_FFLUSH) fflush(stdout);
    }
    return nrtc->id;
}

retransmission_cache_s *rtc_find_by_id(overseer_s *overseer, uint32_t id) {
    if (overseer->rtc == NULL)
        return NULL;
    retransmission_cache_s *target = overseer->rtc;
    for (; target != NULL && target->id != id; target = target->next);
    return target;
}

//FIXME Apparently there's some segfaulting here
uint32_t rtc_remove_by_type(overseer_s *overseer, enum message_type type) {
    if (overseer->rtc == NULL)
        return 0;

    uint32_t nb_removed = 0;

    retransmission_cache_s *ite = overseer->rtc, *tmp;
    // Clearing all entries of said type that are at the beginning of the linked list
    while (ite != NULL && ite->type == type) {
        tmp = ite->next;
        rtc_free(ite);
        ite = tmp;
        nb_removed++;
    }
    overseer->rtc = ite;
    if (ite != NULL) {
        while (ite->next != NULL) {
            // Remove any entry with said type without breaking the chain
            if (ite->next->type == type) {
                tmp = ite->next->next;
                rtc_free(ite->next);
                ite->next = tmp;
                nb_removed++;
            }
            ite = ite->next;
        }
    }

    return nb_removed;
}

int rtc_remove_by_id(overseer_s *overseer, uint32_t id, char flags) {
    if (overseer->rtc == NULL) {
        if ((flags & FLAG_SILENT) == FLAG_SILENT)
            return EXIT_SUCCESS;
        fprintf(stderr, "Attempting to remove cache element %d but cache is empty.\n", id);
        if (INSTANT_FFLUSH) fflush(stderr);
        return EXIT_FAILURE;
    }

    retransmission_cache_s *ptr = overseer->rtc;
    if (overseer->rtc->id == id) {
        if (DEBUG_LEVEL >= 4) {
            char type_string[32];
            cm_type_string(type_string, ptr->type);
            printf("[Entry is of type %s] ", type_string);
            if (INSTANT_FFLUSH) fflush(stdout);
        }
        overseer->rtc = ptr->next;
        rtc_free(ptr);
        return EXIT_SUCCESS;
    }
    for (; ptr->next != NULL && ptr->next->id != id; ptr = ptr->next);
    if (ptr->next != NULL && ptr->next->id == id) {
        retransmission_cache_s *tmp = ptr->next;
        ptr->next = tmp->next;
        if (tmp->id == overseer->log->fix_conversation) {
            overseer->log->fix_conversation = 0;
            overseer->log->fix_type = FIX_TYPE_NONE;
        }
        if (DEBUG_LEVEL >= 4) {
            char type_string[32];
            cm_type_string(type_string, tmp->type);
            printf("[Entry is of type %s] ", type_string);
            if (INSTANT_FFLUSH) fflush(stdout);
        }
        rtc_free(tmp);
        return EXIT_SUCCESS;
    }
    if ((flags & FLAG_SILENT) == FLAG_SILENT)
        return EXIT_SUCCESS;
    fprintf(stderr, "Attempting to remove cache element %d but it does not exist.\n", id);
    if (INSTANT_FFLUSH) fflush(stderr);
    return EXIT_FAILURE;
}