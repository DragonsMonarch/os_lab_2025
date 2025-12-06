#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include "common.h"

struct ThreadArgs {
    struct Server server;
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
    uint64_t result;
};

void *ProcessServer(void *args) {
    struct ThreadArgs *thread_args = (struct ThreadArgs *)args;
    
    int sck = CreateAndConnectSocket(thread_args->server.ip, thread_args->server.port);
    if (sck < 0) {
        pthread_exit(NULL);
    }

    struct TaskData task = {
        .begin = thread_args->begin,
        .end = thread_args->end,
        .mod = thread_args->mod
    };
    
    char task_buffer[sizeof(uint64_t) * 3];
    SerializeTaskToBuffer(&task, task_buffer);

    if (send(sck, task_buffer, sizeof(task_buffer), 0) < 0) {
        fprintf(stderr, "Send failed to %s:%d\n", 
                thread_args->server.ip, thread_args->server.port);
        close(sck);
        pthread_exit(NULL);
    }

    char response[sizeof(uint64_t)];
    if (recv(sck, response, sizeof(response), 0) < 0) {
        fprintf(stderr, "Receive failed from %s:%d\n", 
                thread_args->server.ip, thread_args->server.port);
        close(sck);
        pthread_exit(NULL);
    }

    memcpy(&thread_args->result, response, sizeof(uint64_t));
    close(sck);
    
    pthread_exit(NULL);
}

int main(int argc, char **argv) {
    uint64_t k = -1;
    uint64_t mod = -1;
    char servers_file[255] = {'\0'};

    while (true) {
        int current_optind = optind ? optind : 1;

        static struct option options[] = {
            {"k", required_argument, 0, 0},
            {"mod", required_argument, 0, 0},
            {"servers", required_argument, 0, 0},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);

        if (c == -1)
            break;

        switch (c) {
        case 0: {
            switch (option_index) {
            case 0:
                ConvertStringToUI64(optarg, &k);
                break;
            case 1:
                ConvertStringToUI64(optarg, &mod);
                break;
            case 2:
                memcpy(servers_file, optarg, strlen(optarg));
                break;
            default:
                printf("Index %d is out of options\n", option_index);
            }
        } break;

        case '?':
            printf("Arguments error\n");
            break;
        default:
            fprintf(stderr, "getopt returned character code 0%o?\n", c);
        }
    }

    if (k == -1 || mod == -1 || !strlen(servers_file)) {
        fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
                argv[0]);
        return 1;
    }

    // Чтение серверов из файла
    FILE *file = fopen(servers_file, "r");
    if (file == NULL) {
        fprintf(stderr, "Cannot open servers file: %s\n", servers_file);
        return 1;
    }

    struct Server *servers = NULL;
    unsigned int servers_num = 0;
    char line[256];

    while (fgets(line, sizeof(line), file)) {
        // Удаляем символ новой строки
        line[strcspn(line, "\n")] = 0;
        
        // Игнорируем пустые строки
        if (strlen(line) == 0)
            continue;
        
        servers_num++;
        servers = realloc(servers, sizeof(struct Server) * servers_num);
        
        char *colon = strchr(line, ':');
        if (colon == NULL) {
            fprintf(stderr, "Invalid server format: %s\n", line);
            fclose(file);
            free(servers);
            return 1;
        }
        
        *colon = '\0';
        strncpy(servers[servers_num - 1].ip, line, sizeof(servers[servers_num - 1].ip) - 1);
        servers[servers_num - 1].ip[sizeof(servers[servers_num - 1].ip) - 1] = '\0';
        servers[servers_num - 1].port = atoi(colon + 1);
    }
    
    fclose(file);

    if (servers_num == 0) {
        fprintf(stderr, "No servers found in file: %s\n", servers_file);
        free(servers);
        return 1;
    }

    // Распределение работы между серверами
    uint64_t chunk_size = k / servers_num;
    pthread_t threads[servers_num];
    struct ThreadArgs thread_args[servers_num];
    
    for (int i = 0; i < servers_num; i++) {
        thread_args[i].server = servers[i];
        thread_args[i].mod = mod;
        
        // Определяем диапазон для каждого сервера
        thread_args[i].begin = i * chunk_size + 1;
        thread_args[i].end = (i == servers_num - 1) ? k : (i + 1) * chunk_size;
        
        // Создаем поток для каждого сервера
        if (pthread_create(&threads[i], NULL, ProcessServer, (void *)&thread_args[i])) {
            fprintf(stderr, "Error creating thread for server %s:%d\n", 
                    servers[i].ip, servers[i].port);
            // Продолжаем с другими серверами
        }
    }

    // Ожидаем завершения всех потоков
    uint64_t total = 1;
    for (int i = 0; i < servers_num; i++) {
        pthread_join(threads[i], NULL);
        if (thread_args[i].result != 0) {
            total = MultModulo(total, thread_args[i].result, mod);
        }
    }

    printf("Total result: %llu\n", (unsigned long long)total);
    
    free(servers);
    return 0;
}
// ./client --k 100000 --mod 123456789 --servers servers.txt