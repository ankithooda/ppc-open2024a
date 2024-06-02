#include <iostream>
#include <random>

void correlate(int, int, const float *, float *);

int main() {
  const int size      = 9000;
  float *data         = (float *)malloc(size * size * sizeof(float));
  float *result       = (float *)malloc(size * size * sizeof(float));

  std::random_device seed;
  std::mt19937 gen{seed()};                       // seed the generator
  std::uniform_int_distribution<> dist{1, 10000}; // set min and max

  for (int r = 0; r < size; r++) {
    for (int c = 0; c < size; c++) {
      data[c + r * size] = (float)dist(gen);
    }
  }

  correlate(size, size, (const float*)data, result);
}
