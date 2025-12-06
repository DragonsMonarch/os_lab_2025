#ifndef PARALLEL_SUM_H
#define PARALLEL_SUM_H

#include <stdint.h>

// Structure for thread arguments
struct SumArgs {
    int *array;
    int begin;
    int end;
};

// Function to calculate sum of array segment
int Sum(const struct SumArgs *args);

// Thread function for parallel sum calculation
void *ThreadSum(void *args);

// Main parallel sum function
// Returns: total sum, or -1 on error
// time_taken: output parameter for execution time in seconds
int ParallelSum(int *array, int array_size, int threads_num, double *time_taken);

#endif