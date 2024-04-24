#include <iostream>
#include <limits>

void step(float* r, const float* d, int n);

int main() {
    constexpr int n = 3;
    const float d[n*n] = {
        0, 8, 2,
        1, 0, 9,
        4, 5, 0,
    };
    float r[n*n];
    step(r, d, n);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            std::cout << r[i*n + j] << " ";
        }
        std::cout << "\n";
    }
}

void step(float *r, const float* d, int n) {
  float t;

  for (unsigned int j = 0; j < n; j++) {
    for (unsigned int i = 0; i < n; i++) {
      r[j*n+i] = std::numeric_limits<float>::infinity();;
    }
  }

  for (unsigned int j = 0; j < n; j++) {
    for (unsigned int i = 0; i < n; i++) {
      for (unsigned int k = 0; k < n; k++) {
        t = d[k*n+i] + d[j*n+k];
        if (t < r[j*n+i]) {
          r[j*n+i] = t;
        }
      }
    }
  }
}
