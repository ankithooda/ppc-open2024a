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

// s1 <= e1 <= s2 <= e2                     (All indices on data)
// scratch_start <= scratch_end             (All indices on scratch)
void my_merge_parallel(
                       unsigned int s1,
                       unsigned int e1,
                       unsigned int s2,
                       unsigned int e2,
                       data_t *data,
                       unsigned int scratch_start,
                       unsigned int scratch_end,
                       data_t *scratch
                       ) {

  unsigned d1 = e1 - s1;
  unsigned d2 = e2 - s2;

  unsigned s3, e3;           // Index bounds for the larger list from the two.
  unsigned s4, e4;           // Index bounds for the smaller list from the two.

  unsigned large_mid;
  data_t m_value;
  if (d1 < d2) {
    s3 = s2;
    e3 = e2;
    s4 = s1;
    e4 = e1;
    large_mid = (e2 + s2) / 2;
  } else {
    s3 = s1;
    e3 = e1;
    s4 = s2;
    e4 = e2;
    large_mid = (e1 + s1) / 2;
  }
  m_value = data[large_mid];

  // mid_l is an index value
  // Binary search on the smaller list.
  unsigned found = custom_bs(s4, e4, m_value, data);

  // mid point of scratch would be first half elements of largest array
  // + first half element of smaller array)

  unsigned mid_scratch = (large_mid - s3) + (found - s4) + 1;

  // Set the min value of the scratch buffer
  scratch[mid_scratch] = m_value;

  // First Half
  unsigned int half1_1 = s3;
  unsigned int half1_2 = large_mid;

  // Second Half
  unsigned int half2_1 = ;
  unsigned int half2_2 = end;

  // Setup the recursion

  my_merge_parallel(s3, large_mid, s4, found + 1, data, scratch_start, mid_scratch, scratch);
  my_merge_parallel(large_mid, e3, found + 1, e4, data, mid_scratch + 1, scratch_end, scratch);

  // Final copy back to the main memory
  std::memcpy(data + s3, scratch, (mid_scratch - scratch_start) * sizeof(data_t));
  std::memcpy(data + large_mid, scratch, (scratch_end - mid_scratch + 1) * sizeof(data_t));

}

void my_sort_partial(unsigned int low, unsigned int high, data_t *data) {
  std::cout << "PARTIAL SORT " << low << " " << high << "\n";
  if (low == high) {
    return;
  }
  if (high - low == 1) {
    if (data[high] < data[low]) {
      data[high] = data[low] ^ data[high];
      data[low] = data[high] ^ data[low];
      data[high] = data[low] ^ data[high];
    }
    return;
  }
  unsigned mid = (high + low + 1) / 2;

  #pragma omp task
  my_sort_partial(low, mid-1, data);

  #pragma omp task
  my_sort_partial(mid, high, data);

  #pragma omp taskwait
  my_merge(low, mid, high, data);

}

void my_sort(unsigned int n, data_t *data) {
  #pragma omp parallel
  #pragma omp single
  {
    my_sort_partial(0, n-1, data);
  }
  #pragma omp taskwait
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
