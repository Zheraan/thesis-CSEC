//
// Created by zheraan on 06/08/23.
//

#include "expe/state-monitoring.h"
#include "expe/killer.h"
#include "lib-csec.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        debug_log(0, stdout, "Usage: ./raft-server <hostfile>\n");
        exit(EXIT_FAILURE);
    }

    debug_log(1, stdout, "Starting program state initialization ...\n");
    // Initialize program state
    overseer_s overseer;
    if (overseer_init(&overseer, argv[1]) != EXIT_SUCCESS) {
        fprintf(stderr, "Failed to initialize the program state\n");
        if (INSTANT_FFLUSH) fflush(stderr);
        exit(EXIT_FAILURE);
    }
    debug_log(1, stdout, "Done.\nStarting event loop initialization ...\n");

    // Initialize event loop
    if (pstr_reception_init(&overseer) != EXIT_SUCCESS || // Initialize the PSTR reception handler event
                                                          etr_reception_init(&overseer) != EXIT_SUCCESS ||
                                                          killer_init(
                                                                  &overseer)) { // Initialize ETR reception handler event
        fprintf(stderr, "Failed to initialize the event loop\n");
        overseer_wipe(&overseer);
        exit(EXIT_FAILURE);
    }
    debug_log(1, stdout, "Done.\nLaunching cluster-monitor.\n");

    // Run the loop
    int rv = event_base_dispatch(overseer.eb);
    if (rv == 1)
        fprintf(stdout, "No events pending or active in the base\n");
    else if (rv == -1)
        fprintf(stderr, "An error occurred while running the loop\n");

    // Clean program state and close socket
    fprintf(stdout, "Cleaning up and finishing...\n");
    overseer_wipe(&overseer);

    return rv;
}