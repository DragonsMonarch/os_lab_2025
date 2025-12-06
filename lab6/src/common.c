#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod) {
    uint64_t result = 0;
    a = a % mod;
    while (b > 0) {
        if (b % 2 == 1)
            result = (result + a) % mod;
        a = (a * 2) % mod;
        b /= 2;
    }
    return result % mod;
}

bool ConvertStringToUI64(const char *str, uint64_t *val) {
    char *end = NULL;
    unsigned long long i = strtoull(str, &end, 10);
    if (errno == ERANGE) {
        fprintf(stderr, "Out of uint64_t range: %s\n", str);
        return false;
    }
    if (errno != 0)
        return false;
    *val = i;
    return true;
}

int CreateAndConnectSocket(const char *ip, int port) {
    struct hostent *hostname = gethostbyname(ip);
    if (hostname == NULL) {
        fprintf(stderr, "gethostbyname failed with %s\n", ip);
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = *((unsigned long *)hostname->h_addr);

    int sck = socket(AF_INET, SOCK_STREAM, 0);
    if (sck < 0) {
        fprintf(stderr, "Socket creation failed!\n");
        return -1;
    }

    if (connect(sck, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "Connection failed to %s:%d\n", ip, port);
        close(sck);
        return -1;
    }

    return sck;
}

int CreateAndBindSocket(int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        fprintf(stderr, "Cannot create server socket on port %d\n", port);
        return -1;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons((uint16_t)port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    int opt_val = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

    if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        fprintf(stderr, "Cannot bind to socket on port %d\n", port);
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, 128) < 0) {
        fprintf(stderr, "Cannot listen on socket on port %d\n", port);
        close(server_fd);
        return -1;
    }

    return server_fd;
}

struct Server* ReadServersFromFile(const char *filename, int *count) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Cannot open servers file: %s\n", filename);
        return NULL;
    }

    struct Server *servers = NULL;
    *count = 0;
    char line[256];

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        
        if (strlen(line) == 0)
            continue;
        
        (*count)++;
        servers = realloc(servers, sizeof(struct Server) * (*count));
        
        char *colon = strchr(line, ':');
        if (colon == NULL) {
            fprintf(stderr, "Invalid server format: %s\n", line);
            fclose(file);
            free(servers);
            return NULL;
        }
        
        *colon = '\0';
        strncpy(servers[(*count) - 1].ip, line, sizeof(servers[(*count) - 1].ip) - 1);
        servers[(*count) - 1].ip[sizeof(servers[(*count) - 1].ip) - 1] = '\0';
        servers[(*count) - 1].port = atoi(colon + 1);
    }
    
    fclose(file);
    return servers;
}

int ParseTaskFromBuffer(const char *buffer, struct TaskData *task) {
    memcpy(&task->begin, buffer, sizeof(uint64_t));
    memcpy(&task->end, buffer + sizeof(uint64_t), sizeof(uint64_t));
    memcpy(&task->mod, buffer + 2 * sizeof(uint64_t), sizeof(uint64_t));
    return 0;
}

void SerializeTaskToBuffer(const struct TaskData *task, char *buffer) {
    memcpy(buffer, &task->begin, sizeof(uint64_t));
    memcpy(buffer + sizeof(uint64_t), &task->end, sizeof(uint64_t));
    memcpy(buffer + 2 * sizeof(uint64_t), &task->mod, sizeof(uint64_t));
}