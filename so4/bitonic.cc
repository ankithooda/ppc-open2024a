#include <iostream>
#include <algorithm>
#include <random>
#include <x86intrin.h>
#include <omp.h>
#include <limits>
#include <cstring>

#define long_v __m256i


typedef unsigned long long data_t;
const int capacity = 4;
const int align_boundary  = 32;
const long long infinity = std::numeric_limits<long long>::max();


void print_l(int n, data_t *data) {
  std::cout << "\n";
  for (int i = 0; i < n; i++) {
    std::cout << data[i] << " ";
  }
  std::cout << "\n";
}

void print_vector_long(int n, __m256i *v) {
  data_t *a;
  // std::cout << v << " Address of vector\n";
  if (posix_memalign((void**)&a, align_boundary, capacity * sizeof(data_t)) == 0) {

    for (unsigned int j = 0; j < n; j++) {
      _mm256_store_epi64(a, *(v+j));
      for (unsigned int i = 0; i < capacity; i++) {
        std::cout << a[i] << " ";
      }
      //std::cout << "\n";
    }
    std::cout << "\n";
    free(a);
  }
}

void bitonic_merge(int p1_start, int p1_end, int p2_start, int p2_end, long_v *data, long_v *scratch) {

}

// order : 0 -> ascending
// order : 1 -> descending
void bitonic_sort(int order, int start, int end, long_v *data, long_v *scratch) {

}

void psort(int n, data_t *data) {
    // FIXME: Implement a more efficient parallel sorting algorithm for the CPU,
    // using the basic idea of quicksort.

  int pad_n = (n + capacity - 1) / capacity;

  __mmask8 all1 = _cvtu32_mask8(15);

  long_v *padded_data;
  long_v *scratch_data;
  long_v max_vector = _mm256_set1_epi64x(infinity);

  if (posix_memalign((void**)&padded_data, align_boundary, pad_n * capacity * sizeof(data_t)) != 0) {
    return;
  }

  if (posix_memalign((void**)&scratch_data, align_boundary, pad_n * capacity * sizeof(data_t)) != 0) {
    return;
  }

  print_l(n, data);

  for (int i = 0; i < pad_n-1; i++) {
    std::cout << i << " " << i * capacity << " " << (i * capacity) + capacity << "\n";
    //print_l(4, data + (i * capacity));
    if ((i % 2) == 0) {
      std::sort(data + (i * capacity), data + (i * capacity) + capacity,  std::less<long long>());
    } else {
      std::sort(data + (i * capacity), data + (i * capacity) + capacity,  std::greater<long long>());
    }
    padded_data[i] = _mm256_mask_expandloadu_epi64(max_vector, all1, data + (i * capacity));
  }

  // Fill the last vector which might be partially filled.
  int gap = (pad_n * capacity) - n;
  //std::cout << pad_n << " " << capacity << " " << n << " " << gap << "\n";
  __mmask8 gap_mask = _cvtu32_mask8(pow(2, (capacity - gap)) -1);

  data_t *last_vector;

  if (posix_memalign((void**)&last_vector, align_boundary, capacity * sizeof(data_t)) != 0) {
    return;
  }

  for (int vector_index = 0; vector_index < capacity - gap; vector_index++) {
    last_vector[vector_index] = data[((pad_n - 1) * capacity) + vector_index];
  }

  for (int vector_index = capacity - gap; vector_index < capacity; vector_index++) {
    last_vector[vector_index] = infinity;
  }

  if (((pad_n - 1) % 2) == 0) {
    std::sort(last_vector, last_vector + capacity, std::less<long long>());
  } else {
    std::sort(last_vector, last_vector + capacity, std::greater<long long>());
  }

  padded_data[pad_n - 1] = _mm256_mask_expandloadu_epi64(max_vector, all1, last_vector);

  print_vector_long(pad_n, padded_data);

  /////////////////// MERGE SORT IMPL /////////////////////////////
  // int procs = 8;
  // int partition_len = pad_n / procs;

  // for (int p = 0; p < procs; p++) {
  //   int partition_start = p * partition_len;
  //   int partition_end = std::min((partition_start + partition_len), pad_n);

  //   bitonic_sort(partition_start, partition_end, padded_data, scratch_data);
  // }


  //std::sort(data, data + n);
}

/////////////////////////////////////////////////////////////////////////////////////////

int test_monotonicity(int count, data_t *data) {
  if (count < 2) {
    return 0;
  }
  data_t prev_value = data[0];
  for (int i = 1; i < count; i++) {
    if (prev_value > data[i]) {
      return 1;
    }
    prev_value = data[i];
  }
  return 0;
}

int main() {

  const int count = 38;
  data_t *data = (data_t *)malloc(count * sizeof(data_t));

  std::random_device seed;
  std::mt19937 gen{seed()}; // seed the generator
  std::uniform_int_distribution<> dist{1, 10000}; // set min and max
  for (int i = 0; i < count; i++) {
    data[i] = (data_t)dist(gen);
  }
  unsigned long before, after;
  // data[0] = 8;
  // data[1] = 4;
  // data[2] = 9;
  // data[3] = 5;
  // data[4] = 9;
  // data[5] = 3;
  // data[6] = 1;
  // data[7] = 7;
  // data[8] = 0;
  // data[9] = 6;
  // print_l(count, data);

  before = __rdtsc();
  psort(count, data);
  after = __rdtsc();

  // print_l(count, data);

  std::cout << "Cycles for sorting - " << after - before << "\n";

  before = __rdtsc();
  int test_code = test_monotonicity(count, data);
  after = __rdtsc();

  std::cout << "Cycles for testing - " << after - before << "\n";

  free(data);

  if (test_code == 0)
    std::cout << "Success\n";
  else
    std::cout << "Failure\n";
}
