#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

// Структура для передачи данных в потоки
typedef struct {
    long long start;
    long long end;
    long long mod;
    long long partial_result;
} thread_data_t;

// Глобальные переменные
long long global_result = 1;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Функция для вычисления произведения чисел от start до end по модулю mod
void* compute_partial_factorial(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    data->partial_result = 1;
    for (long long i = data->start; i <= data->end; i++) {
        data->partial_result = data->partial_result * i;
    }
    
    // Синхронизированное обновление глобального результата
    pthread_mutex_lock(&mutex);
    global_result = global_result * data->partial_result;
    pthread_mutex_unlock(&mutex);
    
    return NULL;
}

// Функция для парсинга аргументов командной строки
int parse_arguments(int argc, char* argv[], long long* k, int* pnum, long long* mod) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) {
            *k = atoll(argv[++i]);
        } else if (strncmp(argv[i], "--pnum=", 7) == 0) {
            *pnum = atoi(argv[i] + 7);
        } else if (strncmp(argv[i], "--mod=", 6) == 0) {
            *mod = atoll(argv[i] + 6);
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s -k <number> --pnum=<threads> --mod=<modulus>\n", argv[0]);
            printf("Example: %s -k 10 --pnum=4 --mod=10\n", argv[0]);
            return 0;
        }
    }
    
    if (*k <= 0 || *pnum <= 0 || *mod <= 0) {
        fprintf(stderr, "Error: All parameters must be positive integers\n");
        return -1;
    }
    
    return 1;
}

int main(int argc, char* argv[]) {
    long long k = 0;
    int pnum = 0;
    long long mod = 0;
    
    // Парсинг аргументов командной строки
    if (parse_arguments(argc, argv, &k, &pnum, &mod) <= 0) {
        return 1;
    }
    
    printf("Computing %lld! mod %lld using %d threads\n", k, mod, pnum);
    
    // Если количество потоков больше k, уменьшаем количество потоков
    if (pnum > k) {
        pnum = k;
        printf("Adjusted number of threads to %d (cannot exceed k)\n", pnum);
    }
    
    // Выделение памяти для потоков и их данных
    pthread_t* threads = malloc(pnum * sizeof(pthread_t));
    thread_data_t* thread_data = malloc(pnum * sizeof(thread_data_t));
    
    if (!threads || !thread_data) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    
    // Распределение работы между потоками
    long long numbers_per_thread = k / pnum;
    long long remainder = k % pnum;
    long long current_start = 1;
    
    for (int i = 0; i < pnum; i++) {
        thread_data[i].start = current_start;
        thread_data[i].end = current_start + numbers_per_thread - 1;
        
        // Распределяем остаток по первым потокам
        if (remainder > 0) {
            thread_data[i].end++;
            remainder--;
        }
        
        thread_data[i].mod = mod;
        current_start = thread_data[i].end + 1;
        
        printf("Thread %d: numbers from %lld to %lld\n", 
               i, thread_data[i].start, thread_data[i].end);
    }
    
    // Создание потоков
    for (int i = 0; i < pnum; i++) {
        if (pthread_create(&threads[i], NULL, compute_partial_factorial, &thread_data[i]) != 0) {
            fprintf(stderr, "Error creating thread %d\n", i);
            return 1;
        }
    }
    
    // Ожидание завершения всех потоков
    for (int i = 0; i < pnum; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Вывод результата
    printf("\nResult: %lld! = %lld\n", k, global_result);
    printf("\nResult: %lld! mod %lld = %lld\n", k, mod, (global_result % mod));
    
    
    // Освобождение ресурсов
    free(threads);
    free(thread_data);
    pthread_mutex_destroy(&mutex);
    
    return 0;
}