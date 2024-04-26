#include <iostream>
#include <limits>
#include <random>

void step(float* r, const float* d, int n);

int main() {
    constexpr int n = 3000;
    float *d = (float*)malloc(sizeof(float) * n * n);

    // Fill with random numbers
    std::random_device seed;
    std::mt19937 gen{seed()}; // seed the generator
    std::uniform_int_distribution<> dist{1, 10000}; // set min and max
    for (int j = 0; j < n; j++) {
      for (int i = 0; i < n; i++) {
        d[j*n+i] = (float)dist(gen);
      }
    }

    float *r = (float*)malloc(sizeof(float) * n * n);
    std::cout << "Start\n";
    step(r, d, n);
    /*
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            std::cout << r[i*n + j] << " ";
        }
        std::cout << "\n";
    }
    */
    std::cout << "Done\n";

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
