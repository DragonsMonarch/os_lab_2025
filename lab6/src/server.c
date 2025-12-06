#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <getopt.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>

struct ServerConfig {
    int port;
    int tnum;
};

struct FactorialArgs {
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
};

struct ClientThreadArgs {
    int client_fd;
    int tnum;
};

uint64_t Factorial(const struct FactorialArgs *args) {
    uint64_t ans = 1;
    
    if (args->begin == 0 || args->end == 0 || args->begin > args->end) {
        return 1;
    }
    
    for (uint64_t i = args->begin; i <= args->end; i++) {
        ans = MultModulo(ans, i, args->mod);
    }
    
    return ans;
}

void *ThreadFactorial(void *args) {
    struct FactorialArgs *fargs = (struct FactorialArgs *)args;
    uint64_t *result = malloc(sizeof(uint64_t));
    if (result == NULL) {
        return NULL;
    }
    *result = Factorial(fargs);
    return (void *)result;
}

void *HandleClient(void *args) {
    struct ClientThreadArgs *client_args = (struct ClientThreadArgs *)args;
    int client_fd = client_args->client_fd;
    int tnum = client_args->tnum;
    
    while (true) {
        unsigned int buffer_size = sizeof(uint64_t) * 3;
        char from_client[buffer_size];
        int read_bytes = recv(client_fd, from_client, buffer_size, 0);

        if (!read_bytes)
            break;
        if (read_bytes < 0) {
            fprintf(stderr, "Client read failed\n");
            break;
        }
        if (read_bytes < (int)buffer_size) {
            fprintf(stderr, "Client send wrong data format\n");
            break;
        }

        pthread_t threads[tnum];
        struct TaskData task;
        ParseTaskFromBuffer(from_client, &task);
        
        uint64_t begin = task.begin;
        uint64_t end = task.end;
        uint64_t mod = task.mod;

        printf("Server: Received task: %llu %llu %llu\n", 
               (unsigned long long)begin, 
               (unsigned long long)end, 
               (unsigned long long)mod);

        struct FactorialArgs *thread_args = malloc(sizeof(struct FactorialArgs) * tnum);
        if (thread_args == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            break;
        }
        
        // Распределяем работу между потоками на сервере
        if (begin > end) {
            // Если begin > end, меняем местами
            uint64_t temp = begin;
            begin = end;
            end = temp;
        }
        
        uint64_t range_size = end - begin + 1;
        uint64_t chunk_size = range_size / tnum;
        uint64_t remainder = range_size % tnum;
        
        uint64_t current_begin = begin;
        for (int i = 0; i < tnum; i++) {
            thread_args[i].begin = current_begin;
            thread_args[i].end = current_begin + chunk_size - 1 + (i < remainder ? 1 : 0);
            thread_args[i].mod = mod;
            
            current_begin = thread_args[i].end + 1;
            
            if (pthread_create(&threads[i], NULL, ThreadFactorial, (void *)&thread_args[i])) {
                fprintf(stderr, "Error: pthread_create failed!\n");
                free(thread_args);
                close(client_fd);
                free(client_args);
                return NULL;
            }
        }

        uint64_t total = 1;
        for (int i = 0; i < tnum; i++) {
            uint64_t *result = NULL;
            pthread_join(threads[i], (void **)&result);
            if (result) {
                total = MultModulo(total, *result, mod);
                free(result);
            }
        }

        free(thread_args);

        printf("Server: Total result: %llu\n", (unsigned long long)total);

        char buffer[sizeof(total)];
        memcpy(buffer, &total, sizeof(total));
        int err = send(client_fd, buffer, sizeof(total), 0);
        if (err < 0) {
            fprintf(stderr, "Can't send data to client\n");
            break;
        }
    }

    shutdown(client_fd, SHUT_RDWR);
    close(client_fd);
    free(client_args);
    return NULL;
}

void StartServer(int port, int tnum) {
    int server_fd = CreateAndBindSocket(port);
    if (server_fd < 0) {
        fprintf(stderr, "Failed to create server on port %d\n", port);
        exit(1);
    }

    printf("Server started on port %d with %d threads\n", port, tnum);

    while (true) {
        struct sockaddr_in client;
        socklen_t client_len = sizeof(client);
        int client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len);

        if (client_fd < 0) {
            fprintf(stderr, "Server on port %d: Could not establish new connection\n", port);
            continue;
        }

        // Обрабатываем соединение в отдельном потоке для параллельной обработки клиентов
        pthread_t client_thread;
        struct ClientThreadArgs *thread_args = malloc(sizeof(struct ClientThreadArgs));
        if (thread_args == NULL) {
            fprintf(stderr, "Memory allocation failed for client thread args\n");
            close(client_fd);
            continue;
        }
        
        thread_args->client_fd = client_fd;
        thread_args->tnum = tnum;
        
        if (pthread_create(&client_thread, NULL, HandleClient, (void *)thread_args)) {
            fprintf(stderr, "Server on port %d: Failed to create client thread\n", port);
            free(thread_args);
            close(client_fd);
            continue;
        }
        
        pthread_detach(client_thread); // Не ждем завершения потока
    }
    
    close(server_fd);
}

void ReadServerConfigs(const char *filename, struct ServerConfig **configs, int *count) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Cannot open config file: %s\n", filename);
        exit(1);
    }

    *configs = NULL;
    *count = 0;
    char line[256];

    while (fgets(line, sizeof(line), file)) {
        // Удаляем символ новой строки и пробелы
        line[strcspn(line, "\n")] = 0;
        char *trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
        
        // Игнорируем пустые строки и комментарии
        if (strlen(trimmed) == 0 || trimmed[0] == '#')
            continue;
        
        // Парсим строку: порт количество_потоков
        int port, tnum;
        if (sscanf(trimmed, "%d %d", &port, &tnum) == 2) {
            (*count)++;
            *configs = realloc(*configs, sizeof(struct ServerConfig) * (*count));
            
            (*configs)[(*count) - 1].port = port;
            (*configs)[(*count) - 1].tnum = tnum;
        } else {
            fprintf(stderr, "Invalid config line: %s\n", trimmed);
        }
    }
    
    fclose(file);
}

int main(int argc, char **argv) {
    char config_file[256] = {'\0'};
    int single_port = -1;
    int single_tnum = -1;

    // Парсим аргументы командной строки
    while (true) {
        int current_optind = optind ? optind : 1;

        static struct option options[] = {
            {"port", required_argument, 0, 0},
            {"tnum", required_argument, 0, 0},
            {"config", required_argument, 0, 0},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);

        if (c == -1)
            break;

        switch (c) {
        case 0: {
            switch (option_index) {
            case 0: // --port
                single_port = atoi(optarg);
                if (single_port <= 0) {
                    fprintf(stderr, "Invalid port number: %s\n", optarg);
                    return 1;
                }
                break;
            case 1: // --tnum
                single_tnum = atoi(optarg);
                if (single_tnum <= 0) {
                    fprintf(stderr, "Invalid thread number: %s\n", optarg);
                    return 1;
                }
                break;
            case 2: // --config
                strncpy(config_file, optarg, sizeof(config_file) - 1);
                config_file[sizeof(config_file) - 1] = '\0';
                break;
            default:
                printf("Index %d is out of options\n", option_index);
            }
        } break;

        case '?':
            printf("Unknown argument\n");
            break;
        default:
            fprintf(stderr, "getopt returned character code 0%o?\n", c);
        }
    }

    // Проверяем режим запуска
    if (strlen(config_file) > 0) {
        // Режим запуска через конфигурационный файл
        struct ServerConfig *configs = NULL;
        int config_count = 0;
        
        ReadServerConfigs(config_file, &configs, &config_count);
        
        if (config_count == 0) {
            fprintf(stderr, "No valid server configurations found in %s\n", config_file);
            free(configs);
            return 1;
        }

        printf("Starting %d servers from config file: %s\n", config_count, config_file);
        
        // Запускаем каждый сервер в отдельном процессе
        pid_t pids[config_count];
        
        for (int i = 0; i < config_count; i++) {
            pid_t pid = fork();
            
            if (pid == 0) {
                // Дочерний процесс - запускаем сервер
                printf("Starting server %d: port=%d, threads=%d\n", 
                       i + 1, configs[i].port, configs[i].tnum);
                StartServer(configs[i].port, configs[i].tnum);
                exit(0);
            } else if (pid > 0) {
                // Родительский процесс - сохраняем PID
                pids[i] = pid;
                printf("Server %d started with PID %d\n", i + 1, pid);
            } else {
                fprintf(stderr, "Failed to fork for server on port %d\n", configs[i].port);
            }
            
            // Небольшая задержка между запусками серверов
            sleep(1);
        }
        
        free(configs);
        
        // Родительский процесс ждет завершения всех дочерних
        printf("\nAll servers started. Press Ctrl+C to stop all servers.\n");
        printf("Waiting for servers to finish...\n");
        
        for (int i = 0; i < config_count; i++) {
            int status;
            waitpid(pids[i], &status, 0);
            if (WIFEXITED(status)) {
                printf("Server %d (PID %d) finished with status %d\n", 
                       i + 1, pids[i], WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("Server %d (PID %d) killed by signal %d\n", 
                       i + 1, pids[i], WTERMSIG(status));
            }
        }
        
    } else if (single_port != -1 && single_tnum != -1) {
        // Режим запуска одиночного сервера
        printf("Starting single server on port %d with %d threads\n", 
               single_port, single_tnum);
        StartServer(single_port, single_tnum);
    } else {
        // Неверные аргументы
        fprintf(stderr, "Using:\n");
        fprintf(stderr, "  Single server: %s --port PORT --tnum THREADS\n", argv[0]);
        fprintf(stderr, "  Multiple servers: %s --config CONFIG_FILE\n", argv[0]);
        fprintf(stderr, "\nConfig file format (one server per line):\n");
        fprintf(stderr, "  PORT THREADS\n");
        fprintf(stderr, "Example config file:\n");
        fprintf(stderr, "  20001 4\n");
        fprintf(stderr, "  20002 8\n");
        fprintf(stderr, "  20003 2\n");
        return 1;
    }

    return 0;
}
// # В первом терминале - серверы
// ./server --config servers_config.txt
// ./server --port 20001 --tnum 4