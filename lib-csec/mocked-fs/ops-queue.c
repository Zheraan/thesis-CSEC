//
// Created by zheraan on 06/05/23.
//

#include "ops-queue.h"

ops_queue_s *ops_queue_pop(mocked_fs_s *mfs) {
    if (mfs->queue == NULL)
        return NULL;
    ops_queue_s *res = mfs->queue;
    mfs->queue = res->next;
    return res;
}

ops_queue_s *ops_queue_add(data_op_s *op, mocked_fs_s *mfs) {
    ops_queue_s *nqueue = malloc(sizeof(ops_queue_s));
    if (nqueue == NULL) {
        perror("malloc new ops queue element");
        return NULL;
    }
    nqueue->next = NULL;
    nqueue->op = op;

    if (mfs->queue == NULL) { // If queue empty
        mfs->queue = nqueue; // Append new one
        return nqueue;
    }

    ops_queue_s *i = mfs->queue;
    for (; i->next != NULL; i = i->next); // Go to end of queue
    i->next = nqueue; // Append new element
    return nqueue;
}

void ops_queue_element_free(ops_queue_s *element) {
    if (element == NULL)
        return;
    free(element->op);
    event_del(element->timeout_event);
    event_free(element->timeout_event);
    free(element);
    return;
}

void ops_queue_element_free_first(mocked_fs_s *mfs) {
    if (mfs->queue == NULL)
        return;
    ops_queue_s *ptr = mfs->queue;
    mfs->queue = mfs->queue->next;
    ops_queue_element_free(ptr);
    return;
}

int ops_queue_free_all(overseer_s *overseer, ops_queue_s *element) {
    if (element == NULL)
        return 0;

    int nb_elem = 0;

    ops_queue_s *ite = overseer->mfs->queue;

    if (ite == NULL || ite == element) {
        if (ite == element) // Clearing dangling pointer in case the given element was the first of the queue
            overseer->mfs->queue = NULL;
        ite = element->next;
        ops_queue_element_free(element);
        return 1 + ops_queue_free_all(overseer, ite);
    }

    // Getting to where the element is in the queue to clear the dangling pointer on the previous element if any, skip
    // if it's the first one
    while (true) {
        if (ite == NULL) {
            fprintf(stderr,
                    "Error resetting queue: given element was not part of it\n");
            return -1;
        }

        if (ite->next == element) {
            ite->next = NULL; // Clearing dangling pointer in case the queue element was not the first
            break;
        }
        ite = ite->next;
    }

    ops_queue_s *ptr = element;
    ops_queue_s *next;
    do {
        next = element->next;
        ops_queue_element_free(ptr);
        ptr = next;
        nb_elem++;
    } while (ptr != NULL);

    return nb_elem;
}