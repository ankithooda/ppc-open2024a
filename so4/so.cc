#include <iostream>
#include <algorithm>
#include <random>
#include <cstring>
#include <x86intrin.h>
#include <omp.h>

typedef unsigned long long data_t;

void bottom_up_merge(
                     int run1_start,
                     int run1_end,
                     int run2_start,
                     int run2_end,
                     data_t *data,
                     int scratch_start,
                     int scratch_end,
                     data_t *scratch
                     ) {

  int scratch_index = scratch_start;

  int run1_index = run1_start;
  int run2_index = run2_start;

  // Merge the common run length.
  while (run1_index < run1_end && run2_index < run2_end) {

    if (data[run1_index] < data[run2_index]) {
      scratch[scratch_index] = data[run1_index];
      run1_index++;
    } else {
      scratch[scratch_index] = data[run2_index];
      run2_index++;
    }
    scratch_index++;
  }

  // Copy the rest of the elements from whichever run is left.
  while (run1_index < run1_end) {

    scratch[scratch_index] = data[run1_index];

    scratch_index++;
    run1_index++;
  }

  while (run2_index < run2_end) {

    scratch[scratch_index] = data[run2_index];

    scratch_index++;
    run2_index++;
  }
}

void bottom_up_merge_sort(int start, int end, data_t *data, data_t *scratch) {

  int n = end - start;
  int swap_count = 0;

  // 1 interation for each width.
  for (int width = 1; width < n; width = 2 * width) {

    // Two runs of size width are processed at a time.
    for (int i = start; i < end; i = i + 2 * width) {
      int run1_start = i;
      int run1_end = std::min(run1_start + width, end);

      int run2_start = run1_end;
      int run2_end = std::min(run2_start + width, end);

      bottom_up_merge(run1_start, run1_end, run2_start, run2_end, data, run1_start, run2_end, scratch);
    }

    // Swap data and scratch

    data_t *temp;
    temp     = data;
    data     = scratch;
    scratch  = temp;
    swap_count++;
  }

  // TODO : The logic below is too complex, simplify it.
  // After odd number of swaps scratch pointer is the original data pointer
  // and data pointer is the original scratch pointer.
  // But data pointer has the sorted data.
  // So we copy from data pointer to scratch pointer
  if ((swap_count % 2) == 1) {
    std::memcpy(scratch + start, data + start, (n * sizeof(data_t)));
  }
}

void psort(int n, data_t *data) {
  data_t *scratch = (data_t *)malloc(n * sizeof(data_t));
  struct range {
    int start;
    int end;
  };

  int procs = 8;//omp_get_num_procs();
  struct range *base_ranges = (struct range*)malloc(procs * sizeof(struct range));

  //std::cout << "Procs " << omp_get_num_procs() << "\n";

  int len = n / procs;

  if (n > 0) {
    #pragma omp parallel for
    for (int p = 0; p < procs; p++) {

      int range_start = p * len;
      int range_end = (p + 1) * len;

      if ((p + 1) >= procs) {
        range_end = n;
      }

      base_ranges[p].start = range_start;
      base_ranges[p].end = range_end;

      bottom_up_merge_sort(range_start, range_end, data, scratch);
    }
  }

  int partition_count = 1;
  int total_partitions = procs;

  int swap_count = 0;

  while (partition_count < total_partitions) {
    // We process two set of partitions at a time.
    // Each partition set contains partition_count partitions.

    // int i = 0;
    #pragma omp parallel for
    for (int i = 0; i < total_partitions; i = i + (partition_count * 2)) {
    // while (i < total_partitions) {
      bottom_up_merge(
                      base_ranges[i].start,
                      base_ranges[i + partition_count - 1].end,
                      base_ranges[i + partition_count].start,
                      base_ranges[i + (partition_count * 2) - 1].end,
                      data,
                      base_ranges[i].start,
                      base_ranges[i + (partition_count * 2) - 1].end,
                      scratch
                      );
    }

    partition_count = partition_count * 2;

    // Swap data and scratch

    data_t *temp;
    temp     = data;
    data     = scratch;
    scratch  = temp;
    swap_count++;


    // Once all merges are done, copy back scratch buffer.
    // std::memcpy(data, scratch, (n * sizeof(data_t)));
  }
  // TODO : The logic below is too complex, simplify it.
  // After odd number of swaps scratch pointer is the original data pointer
  // and data pointer is the original scratch pointer.
  // But data pointer has the sorted data.
  // So we swap first
  // Copy scratch to data and free scratch afterword (avoids unnecessary branch).
  if ((swap_count % 2) == 1) {
    // Swap back
    data_t *temp;
    temp     = data;
    data     = scratch;
    scratch  = temp;

    std::memcpy(data, scratch, (n * sizeof(data_t)));
  }

  free(scratch);
  free(base_ranges);
}
