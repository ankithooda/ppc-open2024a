#include <iostream>
#include <algorithm>
#include <random>
#include <cstring>
#include <x86intrin.h>
#include <omp.h>

typedef unsigned long long data_t;

void my_merge_serial(
                     int s1,
                     int e1,
                     int s2,
                     int e2,
                     data_t *data,
                     int scratch_start,
                     int scratch_end,
                     data_t *scratch
                     ) {

  // #pragma omp critical
  // std::cout << "SER MERGE " << s1 << " " << e1 << " | " << s2 << " " << e2 << " | " << scratch_start << " " << scratch_end << "\n";
  //data_t *temp = (data_t *)malloc((end - start) * sizeof(data_t));
  data_t *temp = scratch + scratch_start;
  int i1 = s1;
  int i2 = s2;
  int t = 0;

  while(i1 < e1 && i2 < e2 ) {
    if (data[i1] < data[i2]) {
      temp[t] = data[i1];
      i1++;
    } else {
      temp[t] = data[i2];
      i2++;
    }
    t++;
  }

  while (i1 < e1) {
    temp[t] = data[i1];
    i1++;
    t++;
  }

  while (i2 < e2) {
    temp[t] = data[i2];
    i2++;
    t++;
  }

  // Final copy back to the main memory
  //std::memcpy(data + start, temp, (end-start) * sizeof(data_t));

  //free(temp);
}

int bs_leftmost(int start, int end, data_t value, data_t *data) {
  while (start < end) {
    int mid = (end + start) / 2;

    if (data[mid] < value) {
      start = mid + 1;
    } else {
      end = mid;
    }
  }
  return start;
}

// s1 <= e1 <= s2 <= e2                     (All indices on data)
// scratch_start <= scratch_end             (All indices on scratch)
void my_merge_parallel(
                       int s1,
                       int e1,
                       int s2,
                       int e2,
                       data_t *data,
                       int scratch_start,
                       int scratch_end,
                       data_t *scratch
                       ) {

  // #pragma omp critical
  // std::cout << "PAR MERGE " << s1 << " " << e1 << " | " << s2 << " " << e2 << " | " << scratch_start << " " << scratch_end << "\n";
  int d1 = e1 - s1;
  int d2 = e2 - s2;

  if (d1 < 128 || d2 < 128) {
    my_merge_serial(s1, e1, s2, e2, data, scratch_start, scratch_end, scratch);
    return;
  }

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
  // If largest array's len is zero or less, we return cause that is our basecase.
  if ((e3 - s3) <= 0) {
    return;
  }
  m_value = data[large_mid];

  // mid_l is an index value
  // Binary search on the smaller list.
  unsigned found = bs_leftmost(s4, e4, m_value, data);

  // mid point of scratch would be first half elements of largest array
  // + first half element of smaller array)

  unsigned mid_scratch = scratch_start + (large_mid - s3) + (found - s4);

  // Set the min value of the scratch buffer
  scratch[mid_scratch] = m_value;

  #pragma omp task
  my_merge_parallel(s3, large_mid, s4, found, data, scratch_start, mid_scratch, scratch);

  #pragma omp task
  my_merge_parallel(large_mid+1, e3, found, e4, data, mid_scratch + 1, scratch_end, scratch);

  #pragma omp taskwait
  return;
}

void my_sort_partial(int low, int high, data_t *data, data_t *scratch) {
  // #pragma omp critical
  // std::cout << "SORT " << low << " " << high << "\n";

  // If data range contains only 1 element.
  if (high - low == 1) {
    return;
  }
  // If data range contains only 2 elements.
  // Swap'em if they are out of order.
  if (high - low == 2) {
    if (data[high-1] < data[low]) {
      data[high-1]   = data[low]    ^ data[high-1];
      data[low]      = data[high-1] ^ data[low];
      data[high-1]   = data[low]    ^ data[high-1];
    }
    return;
  }
  unsigned mid = (high + low) / 2;

#pragma omp task shared(data, scratch)
  my_sort_partial(low, mid, data, scratch);

#pragma omp task shared(data, scratch)
  my_sort_partial(mid, high, data, scratch);

  #pragma omp taskwait

  #pragma omp parallel
  #pragma omp single
  {
  my_merge_parallel(low, mid, mid, high, data, low, high, scratch);
   }
  //  Copy back scratch buffer to main memory.


  std::memcpy(data+low, scratch+low, (high-low) * sizeof(data_t));

}

void psort(int n, data_t *data) {
  data_t *scratch = (data_t *)malloc(n * sizeof(data_t));

  //omp_set_max_active_levels(4);
  #pragma omp parallel
  #pragma omp single
  {
    if (n > 0) {
      my_sort_partial(0, n, data, scratch);
    }
  free(scratch);
  }
}
