#include <iostream>
#include <limits>
#include <random>
#include <x86intrin.h>

int main() {
  constexpr int n = 4000;
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
  unsigned long after, before, mid1;



    float *r = (float*)malloc(sizeof(float) * n * n);
    float *t = (float*)malloc(sizeof(float) * n * n);

    std::cout << "Start\n";

    before = __rdtsc();
    #pragma omp parallel for
    for (unsigned int j = 0; j < n; j++) {
      for (unsigned int i = 0; i < n; i++) {
        t[i * n + j] = d[j * n + i];
      }
    }
    mid1 = __rdtsc();

    // for (int i = 0; i < n; ++i) {
    //   for (int j = 0; j < n; ++j) {
    //     std::cout << d[i*n + j] << " ";
    //   }
    //   std::cout << "\n";
    // }
    // std::cout << "\n#################\n";
    // for (int i = 0; i < n; ++i) {
    //   for (int j = 0; j < n; ++j) {
    //     std::cout << t[i*n + j] << " ";
    //   }
    //   std::cout << "\n";
    // }
    //#pragma omp parallel for
    for (unsigned int j = 0; j < n; j++) {
      for (unsigned int i = 0; i < n; i++) {
        float v = std::numeric_limits<float>::infinity();

        for (unsigned int k = 0; k < n; k++) {
          float x = d[j*n+k];
          float y = t[i*n+k];
          float z = x + y;
          v = std::min(v, z);
        }
        r[j*n+i] = v;
      }
    }
    after = __rdtsc();
    std::cout << mid1 - before << " " << after - mid1 << " " << "Done\n";
    // for (int i = 0; i < n; ++i) {
    //     for (int j = 0; j < n; ++j) {
    //         std::cout << r[i*n + j] << " ";
    //     }
    //     std::cout << "\n";
    // }
}
