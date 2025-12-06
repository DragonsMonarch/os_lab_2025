#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include <pthread.h>

#include "parallel_sum.h"

int Sum(const struct SumArgs *args) {
    int sum = 0;
    for (int i = args->begin; i < args->end; i++) {
        sum += args->array[i];
    }
    return sum;
}

void *ThreadSum(void *args) {
    struct SumArgs *sum_args = (struct SumArgs *)args;
    return (void *)(size_t)Sum(sum_args);
}

int ParallelSum(int *array, int array_size, int threads_num, double *time_taken) {
    pthread_t threads[threads_num];
    struct SumArgs args[threads_num];
    
    // Prepare arguments for threads
    int segment_size = array_size / threads_num;
    for (int i = 0; i < threads_num; i++) {
        args[i].array = array;
        args[i].begin = i * segment_size;
        args[i].end = (i == threads_num - 1) ? array_size : (i + 1) * segment_size;
    }
    
    // Start time measurement
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Create threads
    for (int i = 0; i < threads_num; i++) {
        if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&args[i])) {
            return -1; // Error
        }
    }
    
    // Collect results
    int total_sum = 0;
    for (int i = 0; i < threads_num; i++) {
        int sum = 0;
        pthread_join(threads[i], (void **)&sum);
        total_sum += sum;
    }
    
    // End time measurement
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    *time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    
    return total_sum;
}