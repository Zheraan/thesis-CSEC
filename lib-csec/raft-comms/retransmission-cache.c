//
// Created by zheraan on 03/05/23.
//

#include "retransmission-cache.h"

void rt_cache_free(retransmission_cache_s *rtc) {
    free(rtc->etr);
    if (rtc->ev != NULL) {
        event_del(rtc->ev);
        event_free(rtc->ev);
    }
    free(rtc);
    return;
}

void rt_cache_free_all(overseer_s *overseer) {
    if (overseer->rt_cache != NULL) {
        retransmission_cache_s *next;
        for (retransmission_cache_s *cur = overseer->rt_cache; cur != NULL; cur = next) {
            next = cur->next;
            rt_cache_free(cur);
        }
        overseer->rt_cache = NULL; // Avoid dangling pointer
    }
    return;
}

uint32_t rt_cache_add_new(overseer_s *overseer,
                          uint8_t attempts,
                          struct sockaddr_in6 addr,
                          socklen_t socklen,
                          enum message_type type,
                          entry_transmission_s *etr) {
    if (DEBUG_LEVEL >= 4) {
        printf("\n    - Creating a new retransmission cache element ... ");
        fflush(stdout);
    }

    retransmission_cache_s *nrtc = malloc(sizeof(retransmission_cache_s));
    if (overseer->rt_cache == NULL) {
        perror("Malloc retransmission cache");
        fflush(stderr);
        return 0;
    }

    // Initialize cache
    nrtc->overseer = overseer;
    nrtc->socklen = socklen;
    nrtc->addr = addr;
    nrtc->max_attempts = attempts;
    nrtc->cur_attempts = 0;
    nrtc->type = type;
    nrtc->next = NULL;
    nrtc->etr = etr;
    nrtc->ev = NULL;

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
        rt_cache_free(nrtc);
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
        rt_cache_free(nrtc);
        return 0;
    }

    // If there are no other cached elements, just insert it in first place with ID 1
    if (overseer->rt_cache == NULL) {
        overseer->rt_cache = nrtc;
        nrtc->id = 1;
        if (DEBUG_LEVEL >= 4) {
            printf("Done.\n");
        }
        return nrtc->id;
    }

    // Otherwise insert the cache in the list with id equal to previous + 1, or resets to 1 when hitting the max value
    // 0 is the value for CMs without retransmission
    retransmission_cache_s *iterator = overseer->rt_cache;
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

    if (DEBUG_LEVEL >= 4) {
        printf("Done.\n");
    }
    return nrtc->id;
}

retransmission_cache_s *rt_cache_find_by_id(overseer_s *o, uint32_t id) {
    if (o->rt_cache == NULL)
        return NULL;
    retransmission_cache_s *target = o->rt_cache;
    for (; target != NULL && target->id != id; target = target->next);
    return target;
}

int rt_cache_remove_by_id(overseer_s *o, uint32_t id) {
    if (o->rt_cache == NULL)
        return EXIT_FAILURE;

    retransmission_cache_s *ptr = o->rt_cache;
    if (o->rt_cache->id == id) {
        o->rt_cache = ptr->next;
        rt_cache_free(ptr);
        return EXIT_SUCCESS;
    }
    for (; ptr->next != NULL && ptr->next->id != id; ptr = ptr->next);
    if (ptr->next != NULL && ptr->next->id == id) {
        retransmission_cache_s *tmp = ptr->next;
        ptr->next = tmp->next;
        rt_cache_free(tmp);
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}