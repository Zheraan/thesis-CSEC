//
// Created by zheraan on 22/04/23.
//

#include "hosts-list.h"

uint32_t hosts_init(char const *hostfile, hosts_list_s *list) {
    errno = 0;
    uint32_t parsed = 0, resolved = 0;
    FILE *file = fopen(hostfile, "r");

    // Initializing
    int localhost_init = 0;
    list->nb_masters = 0;
    list->nb_servers = 0;

    if (file == NULL) {
        perror("hosts_init fopen");
        errno = EPARSINGFAILURE;
        fflush(stdout);
        fflush(stderr);
        return 0;
    }

    struct addrinfo *res = NULL;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;

    if (NO_DNS_LOOKUP_)
        hints.ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG | AI_NUMERICHOST;
    else
        hints.ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG;


    char line[256];
    uint32_t nb_lines = 0;

    // Read each row in the hostfile
    while (fgets(line, sizeof(line), file)) {
        nb_lines++; // Counts the number of line for accurate error display

        // Skip blank lines
        if (is_blank(line) == 1)
            continue;

        // Skip comment lines
        if (is_comment(line) == 1)
            continue;

        // Stop if maximum number of hosts is reached and quit
        if (parsed >= HOSTS_LIST_SIZE) {
            fprintf(stderr,
                    "Error: hosts list exceeding set max hosts list size.\n"
                    "Stopped at line %d.\n"
                    "Consider redefining HOSTS_LIST_SIZE preprocessor macro at compile time.\n",
                    nb_lines);
            freeaddrinfo(res);
            fclose(file);
            errno = EPARSINGFAILURE;
            fflush(stdout);
            fflush(stderr);
            return 0;
        }

        // Strip newline characters in case there are some
        line[strcspn(line, "\r\n")] = 0;

        // Parsing type
        const char delim[] = ",";
        char *token = strtok(line, delim);
        if (DEBUG_LEVEL >= 2) {
            fprintf(stdout, "Parsing host %d at line %d:\n"
                            "   -type: %s\n", parsed, nb_lines, token);
        }
        switch (token[0]) {
            case 'M':
            case 'm':
                list->hosts[parsed].type = NODE_TYPE_M;
                list->nb_masters++;
                break;
            case 'S':
            case 's':
                list->hosts[parsed].type = NODE_TYPE_S;
                list->nb_servers++;
                break;
            case 'C':
            case 'c':
                list->hosts[parsed].type = NODE_TYPE_CM;
                break;
            default:
                fprintf(stderr, "Failure to parse host: invalid host type \"%s\"\n", token);
                freeaddrinfo(res);
                fclose(file);
                errno = EPARSINGFAILURE;
                fflush(stdout);
                fflush(stderr);
                return 0;
        }

        // Parsing name
        token = strtok(NULL, delim);
        if (DEBUG_LEVEL >= 2) {
            fprintf(stdout, "   -name: %s\n", token);
        }
        snprintf(list->hosts[parsed].name, 256, "%s", token);

        // Parsing locality
        token = strtok(NULL, delim);
        if (DEBUG_LEVEL >= 2) {
            fprintf(stdout, "   -locality: %s\n", token);
        }
        if (strncmp(token, "d", 1) == 0 ||
            strncmp(token, "D", 1) == 0) {
            list->hosts[parsed].locality = HOST_LOCALITY_DISTANT;
        } else if (strncmp(token, "l", 1) == 0 ||
                   strncmp(token, "L", 1) == 0) {
            list->hosts[parsed].locality = HOST_LOCALITY_LOCAL;
            if (localhost_init) { // Checking we did not already set the value
                fprintf(stderr, "Failure to parse hostfile: several local hosts defined\n");
                freeaddrinfo(res);
                fclose(file);
                errno = EPARSINGFAILURE;
                fflush(stdout);
                fflush(stderr);
                return 0;
            }
            localhost_init = 1;
            list->hosts[parsed].status = list->hosts[parsed].type == NODE_TYPE_M ? HOST_STATUS_CS : HOST_STATUS_S;
            list->localhost_id = parsed;
        } else {
            fprintf(stderr, "Failure to parse host: invalid host locality \"%s\"", token);
            freeaddrinfo(res);
            fclose(file);
            errno = EPARSINGFAILURE;
            fflush(stdout);
            fflush(stderr);
            return 0;
        }

        // Parsing IPv6 address
        token = strtok(NULL, delim);
        if (DEBUG_LEVEL >= 2) {
            fprintf(stdout, "   -address string: %s", token);
        }
        char port_number[10];
        sprintf(port_number, "%d", PORT_CM);
        int rc = getaddrinfo(token, port_number, &hints, &res);
        if (rc != 0) {
            fprintf(stderr,
                    "Failure to parse host with address '%s': %s (%d)\n",
                    token,
                    gai_strerror(rc),
                    rc);
            freeaddrinfo(res);
            fclose(file);
            errno = EPARSINGFAILURE;
            fflush(stdout);
            fflush(stderr);
            return 0;
        }

        if (res == NULL) {
            // In case getaddrinfo failed to resolve the address contained in the row
            fprintf(stderr, "No host resolved for '%s'", token);
            list->hosts[parsed].status = HOST_STATUS_UNRESOLVED;
            debug_log(2, stdout, " (unresolved)\n");
        } else {
            if (list->hosts[parsed].locality != HOST_LOCALITY_LOCAL)
                list->hosts[parsed].status = HOST_STATUS_UNKNOWN; // Host status will only be determined later
            // Only the first returned entry is used
            memcpy(&(list->hosts[parsed].addr), res->ai_addr, res->ai_addrlen);
            list->hosts[parsed].socklen = res->ai_addrlen;
            if (DEBUG_LEVEL >= 2) {
                char buf[256];
                evutil_inet_ntop(AF_INET6, &(list->hosts[parsed].addr.sin6_addr), buf, 256);
                fprintf(stdout,
                        " (resolved to %s)\n",
                        buf);
            }
            resolved++;
        }

        // Copy the address row in the host entry struct
        // Can be used to try again later in case the host can't be resolved now
        snprintf(list->hosts[parsed].addr_string, 256, "%s", token);

        list->hosts[parsed].next_index = 0;
        list->hosts[parsed].commit_index = 0;
        parsed++;
        freeaddrinfo(res);
        res = NULL;
        fflush(stdout);
    }

    fclose(file);
    list->nb_hosts = resolved;
    if (DEBUG_LEVEL >= 1) {
        printf("%d hosts parsed from file \"%s\" (%d/%d resolved)\n", parsed, hostfile, resolved, parsed);
    }
    if (localhost_init == 0) {
        debug_log(0, stderr, "Fatal error: no local host parsed.\n");
        errno = ENOLOCALHOST;
        fflush(stdout);
        fflush(stderr);
        return 0;
    }
    if (list->nb_servers == 0) {
        debug_log(0, stderr, "Fatal error: no server hosts parsed.\n");
        errno = ENOSERVER;
        fflush(stdout);
        fflush(stderr);
        return 0;
    }
    if (list->nb_masters == 0) {
        debug_log(0, stderr, "Fatal error: no master hosts parsed.\n");
        errno = ENOMASTER;
        fflush(stdout);
        fflush(stderr);
        return 0;
    }
    fflush(stdout);
    return parsed;
}

void host_status_string(char *buf, enum host_status status) {
    switch (status) {
        case HOST_STATUS_UNKNOWN:
            sprintf(buf, "UNKNOWN");
            break;
        case HOST_STATUS_P:
            sprintf(buf, "PRINCIPAL MASTER");
            break;
        case HOST_STATUS_HS:
            sprintf(buf, "HOT-STANDBY MASTER");
            break;
        case HOST_STATUS_CS:
            sprintf(buf, "COLD-STANDBY MASTER");
            break;
        case HOST_STATUS_S:
            sprintf(buf, "SERVER");
            break;
        case HOST_STATUS_UNREACHABLE:
            sprintf(buf, "UNREACHABLE");
            break;
        case HOST_STATUS_UNRESOLVED:
            sprintf(buf, "UNRESOLVED");
            break;
    }
    return;
}

int host_re_resolve(hosts_list_s *list, uint32_t host_id) {
    struct addrinfo *res = NULL;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG;

    int rc = getaddrinfo(list->hosts[host_id].addr_string, "35007", &hints, &res);
    if (rc != 0) {
        fprintf(stderr,
                "Failure to parse host '%s': %s (%d)\n",
                list->hosts[host_id].addr_string,
                gai_strerror(rc), rc);
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    if (res == NULL) {
        // In case getaddrinfo failed to resolve the address contained in the row
        fprintf(stderr, "No host found for '%s'\n", list->hosts[host_id].addr_string);
        list->hosts[host_id].status = HOST_STATUS_UNRESOLVED;
        freeaddrinfo(res);
        fflush(stderr);
        return EXIT_FAILURE;
    }

    list->hosts[host_id].status = HOST_STATUS_UNKNOWN; // Host status will only be determined later
    // Only the first returned entry is used
    memcpy(&(list->hosts[host_id].addr), res->ai_addr, res->ai_addrlen);
    list->hosts[host_id].socklen = res->ai_addrlen;

    freeaddrinfo(res);
    return EXIT_SUCCESS;
}

int is_blank(char const *line) {
    int res = 1;

    // Iterate through each character.
    for (int i = 0; line[i] != '\0' && line[i] != '\n'; i++) {
        if (!isblank(line[i])) {
            // Found a non-whitespace character.
            res = 0;
            break;
        }
    }
    return res;
}

int is_comment(char const *line) {
    if (line[0] == '#')
        return 1;
    return 0;
}

int is_p_available(hosts_list_s *list) {
    for (uint32_t i = 0; i < list->nb_hosts; ++i) {
        if (list->hosts[i].status == HOST_STATUS_P)
            return 1;
    }
    return 0;
}

uint32_t hl_whois(hosts_list_s *list, enum host_status status) {
    for (uint32_t i = 0; i < list->nb_hosts; ++i) {
        if (list->hosts[i].status == status)
            return i;
    }
    errno = ENONE;
    fprintf(stdout, "HL_whois: no host of status %d was found in the hosts-list.\n", status);
    fflush(stderr);
    return EXIT_FAILURE;
}

int hl_change_master(overseer_s *overseer, enum host_status status, uint32_t id) {
    if (id >= overseer->hl->nb_hosts) {
        fprintf(stderr, "HL change master error: host ID out of hosts list range\n");
        return EXIT_FAILURE;
    }
    if (status != HOST_STATUS_P && status != HOST_STATUS_HS) {
        fprintf(stderr,
                "HL change master error: invalid parameter status %d, only 1 (P) and 2 (HS) are valid)\n",
                status);
        return EXIT_FAILURE;
    }

    for (uint32_t i = 0; i < overseer->hl->nb_hosts; ++i) {
        // If current id is of the given status (HS or P), resets it to CS
        if (overseer->hl->hosts[i].status == status && overseer->hl->hosts[i].type == NODE_TYPE_M) {
            if (i == overseer->hl->localhost_id)
                stepdown_to_cs(overseer); // Stepdown to CS if local host is changed from P or HS
            else overseer->hl->hosts[i].status = HOST_STATUS_CS; // Or simply set the node's status as CS in the HL
        }
        // Sets the status for the node with the given ID
        if (i == id)
            overseer->hl->hosts[i].status = status;
    }
    return EXIT_SUCCESS;
}

int hl_update_status(overseer_s *overseer, enum host_status status, uint32_t id) {
    if (id >= overseer->hl->nb_hosts) {
        fprintf(stderr, "HL change master error: host ID out of hosts list range\n");
        return EXIT_FAILURE;
    }

    int rv = EXIT_SUCCESS;
    if (DEBUG_LEVEL >= 4)
        printf("Updating host %d status in the Hosts-list ... ", id);
    if (DEBUG_LEVEL >= 3) {
        if (status != overseer->hl->hosts[id].status) {
            printf("[Updated status of %s from %d to %d] ",
                   overseer->hl->hosts[id].name,
                   overseer->hl->hosts[id].status,
                   status);
        } else {
            printf("[Status of %s (%d) did not change] ",
                   overseer->hl->hosts[id].name,
                   overseer->hl->hosts[id].status);
        }
        if (INSTANT_FFLUSH) fflush(stdout);
    }

    if (status == HOST_STATUS_P || status == HOST_STATUS_HS)
        rv = hl_change_master(overseer, status, id);
    else
        overseer->hl->hosts[id].status = status;

    if (rv == EXIT_SUCCESS) debug_log(4, stdout, "Done.\n");
    else debug_log(0, stderr, "Failure updating host status.\n");

    return rv;
}

int hl_replication_index_change(overseer_s *overseer, uint32_t host_id, uint64_t next_index, uint64_t commit_index) {
    if (DEBUG_LEVEL >= 4) {
        printf("Checking for replication index change for %s %s... ",
               overseer->hl->hosts[host_id].name,
               overseer->hl->localhost_id == host_id ? "(local host) " : "");
        if (INSTANT_FFLUSH) fflush(stdout);
    }

    if (host_id >= overseer->hl->nb_hosts) {
        fprintf(stderr,
                "Given host id (%d) beyond host list boundary (%d)\n",
                host_id,
                overseer->hl->nb_hosts - 1);
        if (INSTANT_FFLUSH) fflush(stderr);
        return EXIT_FAILURE;
    }
    host_s *target_host = &overseer->hl->hosts[host_id];
    log_s *log = overseer->log;
    int local_status = overseer->hl->hosts[overseer->hl->localhost_id].status;
    // Stores the highest entry number that needs committing so as to only send one order in case several entries
    // require committing.
    uint64_t commit_order = 0;

    int change = 0;
    if (DEBUG_LEVEL >= 3 && target_host->next_index < next_index) {
        printf("[New replication index (%ld) is higher than its local replication array counterpart (%ld), "
               "updating array] ",
               next_index,
               target_host->next_index);
        change = 1;
        if (INSTANT_FFLUSH) fflush(stdout);
    } else if (DEBUG_LEVEL >= 3 && target_host->next_index > next_index) {
        printf("[New replication index (%ld) is lower than its local replication array counterpart (%ld), "
               "updating array] ",
               next_index,
               target_host->next_index);
        change = 1;
        if (INSTANT_FFLUSH) fflush(stdout);
    }

    if (DEBUG_LEVEL >= 3 && target_host->commit_index < commit_index) {
        printf("[New commit index (%ld) is higher than its local replication array counterpart (%ld), updating "
               "array] ",
               commit_index,
               target_host->commit_index);
        change = 1;
        if (INSTANT_FFLUSH) fflush(stdout);
    } else if (DEBUG_LEVEL >= 3 && target_host->commit_index > commit_index) {
        printf("[New commit index (%ld) is lower than its local replication array counterpart (%ld), updating "
               "array] ",
               commit_index,
               target_host->commit_index);
        change = 1;
        if (INSTANT_FFLUSH) fflush(stdout);
    }

    if (change == 0)
        debug_log(3, stdout, "[No changes] ");

    // If new next index is higher, increment replication index on each of the concerned log entries
    for (uint64_t i = target_host->next_index; i < next_index; ++i) {
        target_host->status == NODE_TYPE_M ? log->entries[i].master_rep++ : log->entries[i].server_rep++;
        printf("debug rep : %d ",
               target_host->status == NODE_TYPE_M ?
               log->entries[i].master_rep : log->entries[i].server_rep); // TODO Remove debug print
        // If local is P, both majorities are reached, and the entry is not already committed,
        // send the commit order
        if (local_status == HOST_STATUS_P &&
            log->entries[i].state != ENTRY_STATE_COMMITTED &&
            log->entries[i].master_rep >= log->master_majority &&
            log->entries[i].server_rep >= log->server_majority) {
            commit_order = i;
            printf("debug commit order : %d ", commit_order); // TODO Remove debug print
        }
    }
    // Otherwise decrement it for concerned entries
    for (uint64_t i = target_host->next_index; i > next_index; --i) {
        target_host->status == NODE_TYPE_M ? log->entries[i - 1].master_rep-- : log->entries[i - 1].server_rep--;
    }

    // If the commit index changed, broadcast the commit order
    if (commit_order != 0) {
        if (DEBUG_LEVEL >= 2) {
            printf("Consensus reached for log entries up to entry #%ld, ", commit_order);
            if (INSTANT_FFLUSH) fflush(stdout);
        }
        etr_broadcast_commit_order(overseer, commit_order);
    }
    target_host->next_index = next_index;
    target_host->commit_index = commit_index;
    debug_log(4, stdout, "Done.\n");
    return EXIT_SUCCESS;
}