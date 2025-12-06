#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s seed arraysize\n", argv[0]);
        return 1;
    }

    int seed = atoi(argv[1]);
    if (seed <= 0) {
        printf("seed must be a positive number\n");
        return 1;
    }

    int array_size = atoi(argv[2]);
    if (array_size <= 0) {
        printf("array_size must be a positive number\n");
        return 1;
    }

    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork failed");
        return 1;
    }
    else if (pid == 0) {
        // Дочерний процесс
        char seed_str[20];
        char array_size_str[20];
        
        snprintf(seed_str, sizeof(seed_str), "%d", seed);
        snprintf(array_size_str, sizeof(array_size_str), "%d", array_size);
        
        // Запускаем sequential_min_max
        execl("./sequential_min_max", "sequential_min_max", seed_str, array_size_str, NULL);
        
        // Если execl вернул управление, значит произошла ошибка
        perror("execl failed");
        exit(1);
    }
    else {
        // Родительский процесс
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            printf("Child process exited with status: %d\n", WEXITSTATUS(status));
        } else {
            printf("Child process terminated abnormally\n");
        }
    }

    return 0;
}