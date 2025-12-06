#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <signal.h>  // Добавляем для работы с сигналами
#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

// Глобальные переменные
static pid_t *child_pids = NULL;
static int timeout = 0;  // Таймаут в секундах, 0 - отключен
static volatile sig_atomic_t timeout_occurred = 0;
static int pnum_global = 0;  // Глобальная переменная для pnum

// Обработчик сигнала SIGALRM
void timeout_handler(int sig) {
    timeout_occurred = 1;
    printf("Timeout occurred! Sending SIGKILL to child processes...\n");
    
    if (child_pids != NULL) {
        for (int i = 0; i < pnum_global; i++) {
            if (child_pids[i] > 0) {
                kill(child_pids[i], SIGKILL);
            }
        }
    }
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;
  timeout = 0;  // По умолчанию таймаут отключен

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {"timeout", required_argument, 0, 't'},  // Добавляем опцию timeout
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "ft:", options, &option_index);  // Добавляем 't:' для timeout

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
                printf("seed must be a positive number\n");
                return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
                printf("array_size must be a positive number\n");
                return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
                printf("pnum must be a positive number\n");
                return 1;
            }
            break;
          case 3:
            with_files = true;
            break;

          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;
      case 't':  // Обработка таймаута
        timeout = atoi(optarg);
        if (timeout <= 0) {
            printf("timeout must be a positive number\n");
            return 1;
        }
        break;

      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"seconds\"]\n",
           argv[0]);
    return 1;
  }

  // Сохраняем pnum в глобальную переменную для использования в обработчике сигнала
  pnum_global = pnum;

  // Выделяем память для хранения PID дочерних процессов
  child_pids = malloc(pnum * sizeof(pid_t));
  for (int i = 0; i < pnum; i++) {
    child_pids[i] = 0;
  }

  // Устанавливаем обработчик сигнала SIGALRM
  signal(SIGALRM, timeout_handler);

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;

  // Создаем pipes для каждого процесса, если не используем файлы
  int **pipes = NULL;
  if (!with_files) {
    pipes = malloc(pnum * sizeof(int*));
    for (int i = 0; i < pnum; i++) {
      pipes[i] = malloc(2 * sizeof(int));
      if (pipe(pipes[i]) == -1) {
        printf("Pipe creation failed!\n");
        return 1;
      }
    }
  }

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  // Запускаем таймер, если задан таймаут
  if (timeout > 0) {
    alarm(timeout);
  }

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      // successful fork
      active_child_processes += 1;
      child_pids[i] = child_pid;  // Сохраняем PID дочернего процесса
      
      if (child_pid == 0) {
        // child process
        
        // Вычисляем границы для текущего дочернего процесса
        int chunk_size = array_size / pnum;
        int start = i * chunk_size;
        int end = (i == pnum - 1) ? array_size : (i + 1) * chunk_size;
        
        struct MinMax local_min_max = GetMinMax(array, start, end);
        
        if (with_files) {
          // use files here
          char filename[32];
          snprintf(filename, sizeof(filename), "min_max_%d.txt", i);
          FILE *file = fopen(filename, "w");
          if (file != NULL) {
            fprintf(file, "%d %d", local_min_max.min, local_min_max.max);
            fclose(file);
          }
        } else {
          // use pipe here
          close(pipes[i][0]); // закрываем чтение в дочернем процессе
          write(pipes[i][1], &local_min_max.min, sizeof(int));
          write(pipes[i][1], &local_min_max.max, sizeof(int));
          close(pipes[i][1]);
        }
        
        // В дочернем процессе НЕ освобождаем array и pipes - это делает родитель
        exit(0);
      }

    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  // В родительском процессе закрываем ненужные концы pipe
  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      close(pipes[i][1]); // закрываем запись в родительском процессе
    }
  }

  // Ожидаем завершения дочерних процессов с неблокирующим wait
  while (active_child_processes > 0) {
    int status;
    pid_t finished_pid = waitpid(-1, &status, WNOHANG);
    
    if (finished_pid > 0) {
      active_child_processes -= 1;
      
      // Обновляем массив child_pids
      for (int i = 0; i < pnum; i++) {
        if (child_pids[i] == finished_pid) {
          child_pids[i] = 0;
          break;
        }
      }
    } else if (finished_pid == 0) {
      // Есть еще работающие процессы, проверяем таймаут
      if (timeout_occurred) {
        // Таймаут уже обработан в обработчике сигнала
        // Продолжаем ожидать завершения процессов после SIGKILL
        usleep(100000); // Небольшая задержка перед следующей проверкой
      } else {
        // Таймаут не наступил, продолжаем ожидание
        usleep(100000); // Небольшая задержка перед следующей проверкой
      }
    } else {
      // Ошибка в waitpid
      perror("waitpid failed");
      break;
    }
  }

  // Отменяем таймер, если он еще не сработал
  if (timeout > 0 && !timeout_occurred) {
    alarm(0);
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      // read from files
      char filename[32];
      snprintf(filename, sizeof(filename), "min_max_%d.txt", i);
      FILE *file = fopen(filename, "r");
      if (file != NULL) {
        fscanf(file, "%d %d", &min, &max);
        fclose(file);
        remove(filename); // удаляем временный файл
      }
    } else {
      // read from pipes
      if (!timeout_occurred || child_pids[i] == 0) {
        // Читаем из pipe только если процесс завершился нормально или был убит, но данные успели записаться
        read(pipes[i][0], &min, sizeof(int));
        read(pipes[i][0], &max, sizeof(int));
      }
      close(pipes[i][0]);
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  // Освобождаем ресурсы только в родительском процессе
  free(array);
  free(child_pids);
  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      free(pipes[i]);
    }
    free(pipes);
  }

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  if (timeout_occurred) {
    printf("Execution was terminated by timeout after %d seconds\n", timeout);
  }
  fflush(NULL);
  return 0;
}