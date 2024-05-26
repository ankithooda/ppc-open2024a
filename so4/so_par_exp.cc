#include <iostream>
#include <algorithm>
#include <random>
#include <cstring>
#include <x86intrin.h>
#include <omp.h>


typedef unsigned long long data_t;

void print_l(int n, data_t *data) {
  std::cout << "\n";
  for (int i = 0; i < n; i++) {
    std::cout << data[i] << " ";
  }
  std::cout << "\n";
}

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

  my_sort_partial(low, mid, data, scratch);
  my_sort_partial(mid, high, data, scratch);


  bottom_up_merge(low, mid, mid, high, data, low, high, scratch);

  // Copy back scratch buffer to main memory.
  std::memcpy(data+low, scratch+low, (high-low) * sizeof(data_t));

}

void bottom_up_merge_sort(int start, int end, data_t *data, data_t *scratch) {

  int n = end - start;

  // 1 interation for each width.
  for (int width = 1; width < n; width = 2 * width) {

    // Two runs of size width are processed at a time.
    for (int i = start; i < n; i = i + 2 * width) {
      int run1_start = i;
      int run1_end = std::min(run1_start + width, n);

      int run2_start = run1_end;
      int run2_end = std::min(run2_start + width, n);

      // std::cout << run1_start << " " << run1_end << " " << run2_start << " " << run2_end << "\n";

      bottom_up_merge(run1_start, run1_end, run2_start, run2_end, data, run1_start, run2_end, scratch);

    }

    //print_l(n, scratch);

    // Once one iteration of fixed width is done copy scratch back to data.
    std::memcpy(data + start, scratch + start, (n * sizeof(data_t)));
  }
}

void psort(int n, data_t *data) {
  data_t *scratch = (data_t *)malloc(n * sizeof(data_t));

  if (n > 0) {
    my_sort_partial(0, n, data, scratch);
  }
  free(scratch);

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

  const int count = 100000000;
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
  //print_l(count, data);

  before = __rdtsc();
  psort(count, data);
  after = __rdtsc();

  //print_l(count, data);

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
