#include <iostream>
#include <limits>
#include <random>
#include <x86intrin.h>

void step(float* r, const float* d, int n);

int main() {
  constexpr int n = 1500;
  float *d = (float*)malloc(sizeof(float) * n * n);

  // d[0] = 0;
  // d[1] = 8;
  // d[2] = 2;

  // d[3] = 1;
  // d[4] = 0;
  // d[5] = 9;

  // d[6] = 4;
  // d[7] = 5;
  // d[8] = 0;

  //Fill with random numbers
  std::random_device seed;
  std::mt19937 gen{seed()}; // seed the generator
  std::uniform_int_distribution<> dist{1, 10000}; // set min and max
  for (int j = 0; j < n; j++) {
    for (int i = 0; i < n; i++) {
      d[j*n+i] = (float)dist(gen);
    }
  }

  unsigned long after, before;

  float *r = (float*)malloc(sizeof(float) * n * n);
  std::cout << "Start\n";

  before = __rdtsc();
  step(r, d, n);
  after = __rdtsc();
  std::cout << after - before << " " << "Done\n";
  // for (int i = 0; i < n; ++i) {
  //   for (int j = 0; j < n; ++j) {
  //     std::cout << r[i*n + j] << " ";
  //   }
  //   std::cout << "\n";
  // }
}

void step(float *r, const float* d, int n) {
  for (unsigned int j = 0; j < n; j++) {
    for (unsigned int i = 0; i < n; i++) {
      float v = std::numeric_limits<float>::infinity();

      for (unsigned int k = 0; k < n; k++) {
        float x = d[k*n+i];
        float y = d[j*n+k];
        float z = x + y;
        v = std::min(v, z);
      }
      r[j*n+i] = v;
    }
  }
}
