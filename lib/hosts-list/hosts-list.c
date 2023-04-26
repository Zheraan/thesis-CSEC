//
// Created by zheraan on 22/04/23.
//

#include "hosts-list.h"
#ifdef NO_DNS_LOOKUP // May be defined to skip DNS lookup
#define __NO_DNS_LOOKUP 1
#else
#define __NO_DNS_LOOKUP 0
#endif

uint32_t init_hosts(char const *hostfile, hosts_list_s *list) {
    uint32_t parsed = 0;
    FILE *file = fopen(hostfile, "r");

    if (file == NULL) {
        perror("init_hosts error");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_storage result;
    struct addrinfo *res = NULL;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;

if(__NO_DNS_LOOKUP)
    hints.ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG | AI_NUMERICHOST;
else
    hints.ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG;


    char line[256];

    // Read each line in the hostfile
    while (fgets(line, sizeof(line), file)) {

        // Skip blank lines
        if (is_blank(line) == 1)
            continue;

        // Strip newline in case there is one
        line[strcspn(line, "\r\n")] = 0;

        int rc = getaddrinfo(line, "35007", &hints, &res);
        if (rc != 0) {
            fprintf(stderr, "Failure to parse host '%s': %s (%d)", line, gai_strerror(rc), rc);
            exit(EXIT_FAILURE);
        }

        if (res == NULL) {
            // In case getaddrinfo failed to resolve the address contained in the line
            fprintf(stderr, "No host found for '%s'", line);
            list->hosts[parsed].status = HOST_STATUS_UNRESOLVED;
        } else {
            list->hosts[parsed].status = HOST_STATUS_UNKNOWN; // Host status will only be determined later
            memcpy(&(list->hosts[parsed].addr), res->ai_addr, res->ai_addrlen); // Only the first returned entry is used
        }

        // Copy the address line in the host entry struct
        // Can be used to try again later in case the host can't be resolved now
        snprintf(list->hosts[parsed].addr_string, 256, "%s", line);
        parsed++;
        freeaddrinfo(res);
        res = NULL;
    }

    fclose(file);
    list->nb_hosts = parsed;
    return parsed;
}

int re_resolve_host(hosts_list_s *list, uint32_t host_id) {
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
        // In case getaddrinfo failed to resolve the address contained in the line
        fprintf(stderr, "No host found for '%s'", list->hosts[host_id].addr_string);
        list->hosts[host_id].status = HOST_STATUS_UNRESOLVED;
        freeaddrinfo(res);
        return EXIT_FAILURE;
    }

    list->hosts[host_id].status = HOST_STATUS_UNKNOWN; // Host status will only be determined later
    memcpy(&(list->hosts[host_id].addr), res->ai_addr, res->ai_addrlen); // Only the first returned entry is used

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