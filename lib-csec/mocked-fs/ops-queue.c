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

ops_queue_s *ops_queue_add(data_op_s *op, mocked_fs_s *mfs){
    ops_queue_s *nqueue = malloc(sizeof(ops_queue_s));
    if (nqueue == NULL) {
        perror("malloc new ops queue element");
        return NULL;
    }
    nqueue->next = NULL;
    nqueue->op = op;

    if (mfs->queue == NULL){ // If queue empty
        mfs->queue = nqueue; // Append new one
        return nqueue;
    }

    ops_queue_s *i;
    for (; i->next == NULL; i = i->next); // Go to end of queue
    i->next = nqueue; // Append new element
    return nqueue;
}

void ops_queue_element_free(ops_queue_s *element){
    free(element->op);
    free(element);
    return;
}