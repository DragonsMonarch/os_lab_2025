#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "parallel_sum.h"

int main(int argc, char **argv) {
  uint32_t threads_num = 0;
  uint32_t array_size = 0;
  uint32_t seed = 0;
  
  // Parse command line arguments
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--threads_num") == 0 && i + 1 < argc) {
      threads_num = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--array_size") == 0 && i + 1 < argc) {
      array_size = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
      seed = atoi(argv[++i]);
    }
  }
  
  if (threads_num == 0 || array_size == 0) {
    printf("Usage: %s --threads_num <num> --seed <num> --array_size <num>\n", argv[0]);
    return 1;
  }
  
  int *array = malloc(sizeof(int) * array_size);
  
  // Generate array (not included in time measurement)
  GenerateArray(array, array_size, seed);
  
  // Use library function for parallel sum calculation
  double time_taken;
  int total_sum = ParallelSum(array, array_size, threads_num, &time_taken);
  
  if (total_sum == -1) {
    printf("Error: Parallel sum calculation failed!\n");
    free(array);
    return 1;
  }
  
  free(array);
  printf("Total: %d\n", total_sum);
  printf("Time: %.6f seconds\n", time_taken);
  return 0;
}