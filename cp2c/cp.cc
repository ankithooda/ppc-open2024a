#include <iostream>
#include <cmath>
#include <vector>
#include <immintrin.h>
#include <cmath>

constexpr unsigned int capacity = 4; // Vector registers can hold 4 double precision float.
constexpr int align_boundary = 32;

void print_m(unsigned int ny, unsigned int nx, double *T) {

  for (unsigned int r = 0; r < ny; r++) {
    for (unsigned int c = 0; c < nx; c++) {
      std::cout << " " << T[c + r * nx] << " ";
    }
    std::cout << "\n";
  }
}

void print_vector_double(__m256d *v) {
  double *a;
  // std::cout << v << " Address of vector\n";
  if (posix_memalign((void**)&a, align_boundary, capacity * sizeof(double)) == 0) {
    _mm256_store_pd(a, *v);
    for (unsigned int i = 0; i < capacity; i++) {
      std::cout << " " << a[i] << " ";
    }
    std::cout << "\n";
    free(a);
  }
}

void print_matrix_vector_double(unsigned int ny, unsigned int nx, __m256d *matrix) {
  for (unsigned int r = 0; r < ny; r++) {
    for (unsigned int c = 0; c < nx; c++) {
      // std::cout << matrix + c + r * nx << " Address in loop\n";
      print_vector_double(matrix + c + r * nx);
    }
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
void correlate(int orig_y, int orig_x, const float *data, float *result) {

  unsigned int ny = (unsigned int)orig_y;
  unsigned int nx = (unsigned int)orig_x;
  unsigned int pad_nx = (nx + capacity - 1) / capacity;

  // std::cout << "Vector Bounds " <<  ny << " " << pad_nx << "\n";
  // std::cout << "Original Bounds " <<  ny << " " << nx << "\n";

  double *row_sq_sums = (double *)malloc(sizeof(double) * ny);
  double *row_means = (double *)malloc(sizeof(double) * ny);
  double *T = (double *)malloc(sizeof(double) * nx * ny);

  // Calculate sums and means.
  for (unsigned int r = 0; r < ny; r++) {
    double sum = 0;
    for (unsigned int c = 0; c < nx; c++) {
      sum = sum + data[c + r * nx];
    }
    //row_sums[r] = sum;
    row_means[r] = sum / nx;
  }

  // Normalize the matrix so that
  // for each row arithmetic mean is zero.
  // This can be done by subtracting
  // each element of the row by the arithmetic mean of the row.

  for (unsigned int r = 0; r < ny; r++) {
    for (unsigned int c = 0; c < nx; c++) {
      T[c + r * nx] = data[c + r * nx] - row_means[r];
    }
  }

  // Calculate Squared Sum of this new matrix.
  for (unsigned int r = 0; r < ny; r++) {
    double sq_sum = 0;

    for (unsigned int c = 0; c < nx; c++) {
      sq_sum = sq_sum + T[c + r * nx] * T[c + r * nx];
    }
    row_sq_sums[r] = sqrt(sq_sum);
  }

  // Normalize T matrix so that sum of squared each is zero.
  for (unsigned int r = 0; r < ny; r++) {
    for (unsigned int c = 0; c < nx; c++) {
      T[c + r * nx] = T[c + r * nx] / row_sq_sums[r];
    }
  }

  // std::cout << "Printing original T \n";
  // print_m(ny, nx, T);

  // Convert T -> VT vectorized form
  // using intrinsics
  __mmask8 pad_mask;
  __m256d *IT;
  int mask_bits;

  if (posix_memalign((void**)&IT, 32, ny * pad_nx * capacity * sizeof(double)) != 0) {
    // Return from function, this will cause
    // address sanitizer issuer in the testing framework
    // as other memory has not been freed.
    return;
  }

  // Run the loop for pad_nx - 1
  // because pad_nx - 1 vectors will alway be completely filled.
  // Only the last vector can be partially filled.
  for (unsigned int r = 0; r < ny; r++) {
    pad_mask = _cvtu32_mask8(15);
    for (unsigned int c = 0; c < pad_nx-1; c++) {
      IT[c + r * pad_nx] = _mm256_maskz_expandloadu_pd(pad_mask, T + c * capacity + r * nx);
    }
    // Fill in the the last vector which can be partially filled.
    mask_bits = nx - ((pad_nx - 1) * capacity);
    pad_mask = _cvtu32_mask8(pow(2, mask_bits) - 1);
    //std::cout << mask_bits << " " << nx - ((pad_nx - 1) * capacity) << " " << nx << " " << pad_nx << "\n";
    IT[pad_nx - 1 + r * pad_nx] = _mm256_maskz_expandloadu_pd(pad_mask, T + ((pad_nx - 1) * capacity) + r * nx);
  }

  //std::cout << "Priniting vector matrix \n";
  //print_matrix_vector_double(ny, pad_nx, IT);

  // VECTOR IMPLEMENTATION
  __m256d *temp;
  if (posix_memalign((void**)&temp, 32,  pad_nx * capacity * sizeof(double)) != 0) {
    return;
  }

  for (unsigned int r = 0; r < ny; r++) {
    for (unsigned int c = 0; c < ny; c++) {
      if ( r <= c) {
        double sum = 0;
        __m256d acc = _mm256_setzero_pd();
        for (unsigned k = 0; k < pad_nx; k++) {
          temp[k] = IT[k + r * pad_nx] * IT[k + c * pad_nx];
          acc = _mm256_add_pd(acc, temp[k]);
        }
        // Take horizontal sum
        __m128d vlow  = _mm256_castpd256_pd128(acc);
        __m128d vhigh = _mm256_extractf128_pd(acc, 1); // high 128
        vlow  = _mm_add_pd(vlow, vhigh);               // reduce down to 128

        __m128d high64 = _mm_unpackhi_pd(vlow, vlow);
        sum =  _mm_cvtsd_f64(_mm_add_sd(vlow, high64));
        result[c + r * ny] = (float)sum;
      }
    }
  }
  // Free all allocated memory
  free(row_means);
  free(row_sq_sums);
  free(T);
  free(IT);
  free(temp);
}
