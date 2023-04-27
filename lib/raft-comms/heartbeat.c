//
// Created by zheraan on 26/04/23.
//

#include "heartbeat.h"

void print_hb(heartbeat_s *hb, FILE *stream) {
    fprintf(stream, "host_id:       %d\n"
                    "status:        %d\n"
                    "flags:         %d\n"
                    "next_index:    %ld\n"
                    "rep_index:     %ld\n"
                    "match_index:   %ld\n"
                    "commit_index:  %ld\n"
                    "term:          %d\n\n",
            hb->host_id,
            hb->status,
            hb->flags,
            hb->next_index,
            hb->rep_index,
            hb->match_index,
            hb->commit_index,
            hb->term);
    return;
}