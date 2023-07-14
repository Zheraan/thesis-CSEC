//
// Created by zheraan on 03/05/23.
//

#include "retransmission-cache.h"

void rtc_free(retransmission_cache_s *rtc) {
    if (rtc == NULL)
        return;
    free(rtc->etr);
    if (rtc->ev != NULL) {
        event_free(rtc->ev);
    } else {
        debug_log(2,
                  stderr,
                  "Attempting to free retransmission cache but retransmission event pointer is not set.\n");
    }
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
    return;
}

uint32_t rtc_add_new(overseer_s *overseer,
                     uint8_t rt_attempts,
                     struct sockaddr_in6 sockaddr,
                     socklen_t socklen,
                     enum message_type type,
                     entry_transmission_s *etr,
                     uint32_t ack_back) {
    debug_log(4, stdout, "  - Creating a new retransmission cache element ... ");

    retransmission_cache_s *nrtc = malloc(sizeof(retransmission_cache_s));
    if (nrtc == NULL) {
        perror("Malloc retransmission cache");
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

    event_callback_fn callback;
    if (etr == NULL)
        callback = cm_retransmission_cb;
    else
        callback = etr_retransmission_cb;

    // Create a non-persistent event triggered only by timeout
    struct event *nevent = event_new(overseer->eb,
                                     -1,
                                     EV_TIMEOUT,
                                     callback,
                                     (void *) nrtc);
    if (nevent == NULL) {
        fprintf(stderr, "Failed to create a retransmission event\n");
        rtc_free(nrtc);
        return 0;
    }

    // CM retransmission has low priority
    event_priority_set(nevent, 1);

    // We add the retransmission event to the event op_cache to keep track of it for eventual deletion and free
    nrtc->ev = nevent;

    // Add the event in the loop with ACK timeout (if local host doesn't receive an ack with the ID of the message
    // before the timeout, retransmission will happen)
    struct timeval retransmission_timeout = timeout_gen(TIMEOUT_TYPE_ACK);
    if (errno == EUNKNOWN_TIMEOUT_TYPE || event_add(nevent, &retransmission_timeout) != 0) {
        fprintf(stderr, "Failed to add a retransmission event\n");
        rtc_free(nrtc);
        return 0;
    }

    // If there are no other cached elements, just insert it in first place with ID 1
    if (overseer->rtc == NULL) {
        overseer->rtc = nrtc;
        nrtc->id = 1;
        debug_log(4, stdout, "Done.\n");
        return nrtc->id;
    }

    // Otherwise insert the cache in the list with id equal to previous + 1, or resets to 1 when hitting the max value
    // 0 is the value for CMs without retransmission
    retransmission_cache_s *iterator = overseer->rtc;
    do {
        if (iterator->next == NULL) {
            iterator->next = nrtc;
            nrtc->id = iterator->id + 1;
            if (nrtc->id == 0xFFFFFFFF)
                nrtc->id = 1;
            break;
        }
        iterator = iterator->next;
    } while (iterator != NULL);

    debug_log(4, stdout, "Done.\n");
    return nrtc->id;
}

retransmission_cache_s *rtc_find_by_id(overseer_s *overseer, uint32_t id) {
    if (overseer->rtc == NULL)
        return NULL;
    retransmission_cache_s *target = overseer->rtc;
    for (; target != NULL && target->id != id; target = target->next);
    return target;
}

int rtc_remove_by_id(overseer_s *overseer, uint32_t id, char flag) {
    if (overseer->rtc == NULL) {
        if (flag && FLAG_SILENT == FLAG_SILENT)
            return EXIT_SUCCESS;
        fprintf(stderr, "Attempting to remove cache element %d but cache is empty.\n", id);
        if (INSTANT_FFLUSH) fflush(stderr);
        return EXIT_FAILURE;
    }

    retransmission_cache_s *ptr = overseer->rtc;
    if (overseer->rtc->id == id) {
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
        rtc_free(tmp);
        return EXIT_SUCCESS;
    }
    if (flag && FLAG_SILENT == FLAG_SILENT)
        return EXIT_SUCCESS;
    fprintf(stderr, "Attempting to remove cache element %d but it does not exist.\n", id);
    if (INSTANT_FFLUSH) fflush(stderr);
    return EXIT_FAILURE;
}