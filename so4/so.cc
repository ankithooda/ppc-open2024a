#include <iostream>
#include <algorithm>
#include <random>
#include <cstring>
#include <x86intrin.h>
#include <omp.h>

typedef unsigned long long data_t;

void my_merge_serial(unsigned int start,unsigned int mid, unsigned int end, data_t *data) {

  data_t *temp = (data_t *)malloc((end - start) * sizeof(data_t));
  unsigned int i1 = start;
  unsigned int i2 = mid;
  unsigned t = 0;

  while(i1 < mid && i2 < end ) {
    if (data[i1] < data[i2]) {
      temp[t] = data[i1];
      i1++;
    } else {
      temp[t] = data[i2];
      i2++;
    }
    t++;
  }

  while (i1 < mid) {
    temp[t] = data[i1];
    i1++;
    t++;
  }

  while (i2 < end) {
    temp[t] = data[i2];
    i2++;
    t++;
  }

  // Final copy back to the main memory
  std::memcpy(data + start, temp, (end-start) * sizeof(data_t));

  free(temp);
}

// data[low, high)
void my_sort_partial(unsigned int low, unsigned int high, data_t *data) {
  //std::cout << "SORT " << low << " " << high << "\n";
  // If data range contains only 1 element.
  if (high - low == 1) {
    return;
  }
  // If data range contains only 2 elements.
  if (high - low == 2) {
    if (data[high-1] < data[low]) {
      data[high-1]   = data[low]    ^ data[high-1];
      data[low]      = data[high-1] ^ data[low];
      data[high-1]   = data[low]    ^ data[high-1];
    }
    return;
  }
  unsigned mid = (high + low) / 2;

#pragma omp task
  {
    my_sort_partial(low, mid, data);
  }
#pragma omp task
  {
    my_sort_partial(mid, high, data);
  }
#pragma omp taskwait

  my_merge_serial(low, mid, high, data);
}

void psort(int n, data_t *data) {
    // FIXME: Implement a more efficient parallel sorting algorithm for the CPU,
    // using the basic idea of merge sort.
    // std::sort(data, data + n);
  if (n > 0) {
    //std::cout << "PSORT " << n << "\n";
#pragma omp parallel
#pragma omp single
    {
      my_sort_partial(0, n, data);
    }
#pragma omp taskwait

  }
}
