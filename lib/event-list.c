//
// Created by zheraan on 03/05/23.
//

#include "event-list.h"

void event_list_free(event_list_s *el) {
    if (el != NULL) {
        event_list_s *next;
        for (event_list_s *cur = el; cur != NULL; cur = next) {
            next = cur->next;
            event_free(cur->e);
            free(cur);
        }
    }
    return;
}

int event_list_add(overseer_s *o, struct event *ev) {
    if (o->el == NULL) {
        o->el = malloc(sizeof(event_list_s));
        if (o->el == NULL) {
            perror("Malloc");
            return EXIT_FAILURE;
        }
        o->el->e = ev;
        return EXIT_SUCCESS;
    }

    event_list_s *next = o->el;
    do {
        if (next->next == NULL) {
            next->next = malloc(sizeof(event_list_s));
            next->next->e = ev;
            break;
        }
        next = next->next;
    } while (next != NULL);
    return EXIT_SUCCESS;
}