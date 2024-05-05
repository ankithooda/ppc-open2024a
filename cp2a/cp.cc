#include <iostream>
#include <cmath>
#include <vector>
/*
This is the function you need to implement. Quick reference:
- input rows: 0 <= y < ny
- input columns: 0 <= x < nx
- element at row y and column x is stored in data[x + y*nx]
- correlation between rows i and row j has to be stored in result[i + j*ny]
- only parts with 0 <= j <= i < ny need to be filled
*/
void correlate(int ny, int nx, const float *data, float *result) {

  double *row_sq_sums = (double *)malloc(sizeof(double) * ny);
  double *row_means = (double *)malloc(sizeof(double) * ny);
  double *T = (double *)malloc(sizeof(double) * nx * ny);
  //const float infty = std::numeric_limits<float>::infinity();

  // Create padded data.
  int factor = 4; // How many instruction in pipeline we want.

  // Calculate sums and means. [INSTRUCTION PIPELINED CODE]
  for (int r = 0; r < ny; r++) {
    double sum = 0;
    std::vector<double> sum4(factor, 0);

    // Calculate 4 sums at a time.
    for (int c = 0; c < nx; c=c+factor) {
      for (int f = 0; f < factor; f++) {

        // Check we are within in nx bound.
        if (f + c < nx) {
          sum4[f] = sum4[f] + data[f + c + r * nx];
        }
      }
      //sum = sum + data[c +  r * nx];
    }
    // Get all 4 sums
    for (int f = 0; f < factor; f++) {
      sum = sum + sum4[f];
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
    std::vector<double> sq_sum4(factor, 0);

    for (int c = 0; c < nx; c=c+factor) {
      for (int f = 0; f < factor; f++) {
        if (f + c < nx) {
          sq_sum4[f] = sq_sum4[f] + T[f + c + r * nx] * T[f + c + r * nx];
        }
      }
      //sq_sum = sq_sum + T[c + r * nx] * T[c + r * nx];
    }
    for (int f = 0; f < factor; f++) {
      sq_sum = sq_sum + sq_sum4[f];
    }

    row_sq_sums[r] = sqrt(sq_sum);
  }

  // Normalize T matrix so that sum of squared each is zero.

  for (int r = 0; r < ny; r++) {
    for (int c = 0; c < nx; c++) {
      T[c + r * nx] = T[c + r * nx] / row_sq_sums[r];
    }
  }

  // Multiply T with it's transpose; only the upper half.
  // Y = T*T`
  // The result matrix will have ny*ny
  // because T is ny * nx
  // T` is nx * ny
  //
  // [INSTRUCTION PIPELINED CODE]

  for (int r = 0; r < ny; r++) {
    for (int c = 0; c < ny; c++) {
      // Only the upper half.
      if (r <= c) {
        double rc_sum = 0;
        std::vector<double> rc_sum4(factor, 0);

        for (int k = 0; k < nx; k=k+factor) {
          // T[k + c * nx] = T`[c + k * nx]

          // sum = T[i, k] + T`[k, j]
          // or
          // sum = T[i, k] + T`[j, k]

          for (int f = 0; f < factor; f++) {
            if (f + k < nx) {
              rc_sum4[f] = rc_sum4[f] + T[f + k + r * nx] * T[f + k + c * nx];
            }
          }
          //rc_sum = rc_sum + T[k + r * nx] * T[k + c * nx];
        }
        // Get all 4 sums
        for (int f = 0; f < factor; f++) {
          rc_sum = rc_sum + rc_sum4[f];
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
