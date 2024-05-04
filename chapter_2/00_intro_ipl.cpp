#include <iostream>
#include <limits>
#include <random>
#include <x86intrin.h>

int main() {
  constexpr int n = 3000;

  int factor = 4;

  const float infty = std::numeric_limits<float>::infinity();

  std::vector<float> d(n * n, infty);
  std::vector<float> r(n * n);

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


  int pn = ((n - (factor - 1)) / factor) * factor;

  std::vector<float> _d(n * pn, infty);
  std::vector<float> _t(n * pn, infty);

  std::cout << "Start\n";

  before = __rdtsc();
#pragma omp parallel for
  for (unsigned int j = 0; j < n; j++) {
    for (unsigned int i = 0; i < n; i++) {
      // Transpose
      _t[i * n + j] = d[j * n + i];

      // Data
      _d[j * n + i] = d[j * n + i];
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
      std::vector<float> v(factor, infty);
      //float v = std::numeric_limits<float>::infinity();

      for (unsigned int k = 0; k < pn; k=k+4) {
        for (unsigned int ki = 0; ki < factor; ki++) {
          v[ki] = std::min(v[ki], _d[j * n + k] + _t[i * n + k]);
        }
        //float x = d[j*n+k];
        //float y = t[i*n+k];
        //float z = x + y;
        //v = std::min(v, z);
      }
      //Calculate final min;
      float vv = infty;
      for (unsigned int ki = 0; ki < factor; ki++) {
        vv = std::min(vv, v[ki]);
      }
      r[j*n+i] = vv;
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
