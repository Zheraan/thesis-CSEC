//
// Created by zheraan on 06/05/23.
//

#include "ops-queue.h"

ops_queue_s *ops_queue_pop(mocked_fs_s *mfs){
    if (mfs->queue == NULL)
        return NULL;
    ops_queue_s *res = mfs->queue;
    mfs->queue = res->next;
    return res;
}

ops_queue_s *ops_queue_add(const data_op_s *op, mocked_fs_s *mfs) {
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
    for (; i->next == NULL; i = i->next); // Go to end of queue
    i->next = nqueue; // Append new element
    return nqueue;
}

void ops_queue_element_free(ops_queue_s *element){
    event_del(element->timeout_event);
    event_free(element->timeout_event);
    free(element);
    return;
}

int ops_queue_free_all(overseer_s *overseer, ops_queue_s *queue) {
    // TODO inspect: potential memory leak because deletion events don't seem to be deleted properly
    if (queue == NULL)
        return 0;
    int nb_elem = 0;
    for (ops_queue_s *ite = overseer->mfs->queue; ite != queue; ite = ite->next) {
        if (ite == NULL) {
            fprintf(stderr, "Queue element in parameter was not part of the queue\n");
            return -1;
        }
        if (ite == queue)
            ite->next = NULL; // Clearing dangling pointer in case the queue element was not the first
    }
    ops_queue_s *ptr = queue;
    ops_queue_s *next;
    do {
        next = queue->next;
        free(ptr->op);
        ops_queue_element_free(ptr);
        ptr = next;
        nb_elem++;
    } while (ptr != NULL);

    if (overseer->mfs->queue == queue)
        overseer->mfs->queue = NULL; // Clearing dangling pointer in case the queue element was the first
    return nb_elem;
}