#include <iostream>
#include <algorithm>
#include <random>
#include <cstring>
#include <x86intrin.h>
#include <omp.h>


typedef unsigned long long data_t;

void psort(unsigned int n, data_t *data) {
    // FIXME: Implement a more efficient parallel sorting algorithm for the CPU,
    // using the basic idea of merge sort.
    std::sort(data, data + n);
}

void my_merge(unsigned int start,unsigned int mid, unsigned int end, data_t *data) {
  data_t *temp = (data_t *)malloc((end - start + 1) * sizeof(data_t));
  unsigned int i1 = start;
  unsigned int i2 = mid;
  unsigned t = 0;

  while(i1 <= mid-1 && i2 <=end ) {
    if (data[i1] < data[i2]) {
      temp[t] = data[i1];
      i1++;
    } else {
      temp[t] = data[i2];
      i2++;
    }
    t++;
  }

  while (i1 <= mid-1) {
    temp[t] = data[i1];
    i1++;
    t++;
  }

  while (i2 <= end) {
    temp[t] = data[i2];
    i2++;
    t++;
  }

  // Final copy back to the main memory
  std::memcpy(data + start, temp, (end-start+1) * sizeof(data_t));

  free(temp);
}

void my_sort_partial(unsigned int low, unsigned int high, data_t *data) {
  if (low == high) {
    return;
  }
  // if (high - low == 1) {
  //   if (data[high] < data[low]) {
  //     data[high] = data[low] ^ data[high];
  //     data[low] = data[high] ^ data[low];
  //     data[high] = data[low] ^ data[high];
  //   }
  //   return;
  // }
  unsigned mid = (high + low + 1) / 2;

  #pragma omp task
  my_sort_partial(low, mid-1, data);

  #pragma omp task
  my_sort_partial(mid, high, data);


  my_merge(low, mid, high, data);

}

void my_sort(unsigned int n, data_t *data) {
  #pragma omp parallel
  #pragma omp single
  {
    my_sort_partial(0, n-1, data);
  }
}

void print_l(unsigned int n, data_t *data) {
  std::cout << "\n";
  for (unsigned int i = 0; i < n; i++) {
    std::cout << data[i] << " ";
  }
  std::cout << "\n";
}

int main() {

  const unsigned int count = 8;
  data_t *data = (data_t *)malloc(count * sizeof(data));

  std::random_device seed;
  std::mt19937 gen{seed()}; // seed the generator
  std::uniform_int_distribution<> dist{1, 10000}; // set min and max
  for (int i = 0; i < count; i++) {
    data[i] = (data_t)dist(gen);
  }
  unsigned long before, after;
  print_l(count, data);

  before = __rdtsc();
  my_sort(count, data);
  after = __rdtsc();

  print_l(count, data);

  std::cout << "Cycles - " << after - before;
  free(data);



  // generate random
}
