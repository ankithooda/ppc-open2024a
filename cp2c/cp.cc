#include <iostream>
#include <cmath>
#include <vector>

constexpr int capacity = 4; // Vector registers can hold 4 double precision float.
typedef double double_vt __attribute__ ((vector_size (capacity * sizeof(double))));
constexpr double_vt zero_vt {
  0, 0, 0, 0
};

double l_sum(double_vt a) {
  double sum = 0;
  for (int c = 0; c < capacity; c++) {
    sum = sum + a[c];
  }
  return sum;
}

void print_m(int ny, int nx, double *T) {

  for (int r = 0; r < ny; r++) {
    for (int c = 0; c < nx; c++) {
      std::cout << " " << T[c + r * nx] << " ";
    }
    std::cout << "\n";
  }
}

/*
This is the function you need to implement. Quick reference:
- input rows: 0 <= y < ny
- input columns: 0 <= x < nx
- element at row y and column x is stored in data[x + y*nx]
- correlation between rows i and row j has to be stored in result[i + j*ny]
- only parts with 0 <= j <= i < ny need to be filled
*/
void correlate(int ny, int nx, const float *data, float *result) {

  int pad_nx = (nx + capacity - 1) / nx;

  double *row_sq_sums = (double *)malloc(sizeof(double) * ny);
  double *row_means = (double *)malloc(sizeof(double) * ny);
  double *T = (double *)malloc(sizeof(double) * nx * ny);

  std::vector<double_vt> VT(ny * pad_nx);

  // Calculate sums and means.
  for (int r = 0; r < ny; r++) {
    double sum = 0;
    for (int c = 0; c < nx; c++) {
      sum = sum + data[c + r * nx];
    }
    //row_sums[r] = sum;
    row_means[r] = sum / nx;
  }

  // Normalize the matrix so that
  // for each row arithmetic mean is zero.
  // This can be done by subtracting
  // each element of the row by the arithmetic mean of the row.

  for (int r = 0; r < ny; r++) {
    for (int c = 0; c < nx; c++) {
      T[c + r * nx] = data[c + r * nx] - row_means[r];
    }
  }

  // Calculate Squared Sum of this new matrix.
  for (int r = 0; r < ny; r++) {
    double sq_sum = 0;

    for (int c = 0; c < nx; c++) {
      sq_sum = sq_sum + T[c + r * nx] * T[c + r * nx];
    }
    row_sq_sums[r] = sqrt(sq_sum);
  }

  // Normalize T matrix so that sum of squared each is zero.
  for (int r = 0; r < ny; r++) {
    for (int c = 0; c < nx; c++) {
      T[c + r * nx] = T[c + r * nx] / row_sq_sums[r];
    }
  }

  // Zero'd VT
  for (int r = 0; r < ny; r++) {
    for (int c = 0; c < pad_nx; c++) {
      VT[c + r * pad_nx] = zero_vt;
    }
  }
  // Convert T -> VT vectorized form
  for (int r = 0; r < ny; r++) {
    for (int c = 0; c < pad_nx; c=c+capacity) {
      for (int vi = 0; vi < capacity; vi++) {
        VT[c + r * pad_nx][vi] = T[vi + c + r * pad_nx];
      }
    }
  }

  // Multiply T with it's transpose; only the upper half.
  // Y = T*T`
  // The result matrix will have ny*ny
  // because T is ny * nx
  // T` is nx * ny
  //
  for (int r = 0; r < ny; r++) {
    for (int c = 0; c < ny; c++) {
      // Only the upper half.
      if (r <= c) {
        double rc_sum = 0;
        for (int k = 0; k < nx; k++) {
          // T[k + c * nx] = T`[c + k * nx]

          // sum = T[i, k] + T`[k, j]
          // or
          // sum = T[i, k] + T`[j, k]
          rc_sum = rc_sum + T[k + r * nx] * T[k + c * nx];
        }
        result[c + r * ny] = (float)rc_sum;
      }
    }
  }
  // Free all allocated memory
  free(row_means);
  free(row_sq_sums);
  free(T);
}