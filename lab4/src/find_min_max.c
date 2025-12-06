#include "find_min_max.h"

#include <limits.h>

struct MinMax GetMinMax(int *array, unsigned int begin, unsigned int end) {
  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  min_max.max = array[0];
  min_max.min = array[0];

  for (int i = 1; i < end; i++) {

      // Update max if arr[i] is larger
      if (array[i] > min_max.max)
          min_max.max = array[i];

      // Update min if arr[i] is smaller
      if (array[i] < min_max.min)
          min_max.min = array[i];
  }
  return min_max;
}
