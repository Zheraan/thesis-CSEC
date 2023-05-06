//
// Created by zheraan on 22/04/23.
//

#include "hosts-list.h"

uint32_t hosts_init(char const *hostfile, hosts_list_s *list) {
    uint32_t parsed = 0;
    FILE *file = fopen(hostfile, "r");

    // Initializing
    list->localhost_id = -1;

    if (file == NULL) {
        perror("hosts_init fopen");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_storage result;
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
            exit(EXIT_FAILURE);
        }

        // Strip newline characters in case there are some
        line[strcspn(line, "\r\n")] = 0;

        // Parsing type
        const char delim[] = ",";
        char *token = strtok(line, delim);
        if (DEBUG_LEVEL >= 2) {
            fprintf(stdout, "Parsing host no.%d at line %d:\n"
                            "   -type: %s\n", parsed, nb_lines, token);
        }
        switch (token[0]) {
            case 'M':
            case 'm':
                list->hosts[parsed].type = HOST_TYPE_M;
                break;
            case 'S':
            case 's':
                list->hosts[parsed].type = HOST_TYPE_S;
                break;
            case 'C':
            case 'c':
                list->hosts[parsed].type = HOST_TYPE_CM;
                break;
            default:
                fprintf(stderr, "Failure to parse host: invalid host type \"%s\"", token);
                freeaddrinfo(res);
                fclose(file);
                exit(EXIT_FAILURE);
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
            if (list->localhost_id != -1) {
                fprintf(stderr, "Failure to parse hostfile: several local hosts defined");
                freeaddrinfo(res);
                fclose(file);
                exit(EXIT_FAILURE);
            }
            list->localhost_id = parsed;
        } else {
            fprintf(stderr, "Failure to parse host: invalid host locality \"%s\"", token);
            freeaddrinfo(res);
            fclose(file);
            exit(EXIT_FAILURE);
        }

        // Parsing IPv6 address
        token = strtok(NULL, delim);
        if (DEBUG_LEVEL >= 2) {
            fprintf(stdout, "   -address string: %s", token);
        }
        int rc = getaddrinfo(token, "35007", &hints, &res);
        if (rc != 0) {
            fprintf(stderr, "Failure to parse host with address '%s': %s (%d)", token, gai_strerror(rc), rc);
            freeaddrinfo(res);
            fclose(file);
            exit(EXIT_FAILURE);
        }

        if (res == NULL) {
            // In case getaddrinfo failed to resolve the address contained in the row
            fprintf(stderr, "No host resolved for '%s'", token);
            list->hosts[parsed].status = HOST_STATUS_UNRESOLVED;
            if (DEBUG_LEVEL >= 2) {
                fprintf(stdout, " (unresolved)\n\n");
            }
        } else {
            list->hosts[parsed].status = HOST_STATUS_UNKNOWN; // Host status will only be determined later
            // Only the first returned entry is used
            memcpy(&(list->hosts[parsed].addr), res->ai_addr, res->ai_addrlen); // Only the first returned entry is used
            list->hosts[parsed].addr_len = res->ai_addrlen;
            if (DEBUG_LEVEL >= 2) {
                char buf[256];
                evutil_inet_ntop(AF_INET6, &(list->hosts[parsed].addr.sin6_addr), buf, 256);
                fprintf(stdout,
                        " (resolved to %s)\n\n",
                        buf);
            }
        }

        // Copy the address row in the host entry struct
        // Can be used to try again later in case the host can't be resolved now
        snprintf(list->hosts[parsed].addr_string, 256, "%s", token);

        parsed++;
        freeaddrinfo(res);
        res = NULL;
    }

    fclose(file);
    list->nb_hosts = parsed;
    return parsed;
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
                "Failure to parse host '%s': %s (%d)",
                list->hosts[host_id].addr_string,
                gai_strerror(rc), rc);
        exit(EXIT_FAILURE);
    }

    if (res == NULL) {
        // In case getaddrinfo failed to resolve the address contained in the row
        fprintf(stderr, "No host found for '%s'", list->hosts[host_id].addr_string);
        list->hosts[host_id].status = HOST_STATUS_UNRESOLVED;
        freeaddrinfo(res);
        return EXIT_FAILURE;
    }

    list->hosts[host_id].status = HOST_STATUS_UNKNOWN; // Host status will only be determined later
    // Only the first returned entry is used
    memcpy(&(list->hosts[host_id].addr), res->ai_addr, res->ai_addrlen);
    list->hosts[host_id].addr_len = res->ai_addrlen;

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
    for (int i = 0; i < list->nb_hosts; ++i) {
        if(list->hosts[i].status == HOST_STATUS_P)
            return 1;
    }
    return 0;
}