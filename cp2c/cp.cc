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

void print_m(unsigned int ny, unsigned int nx, const float *T) {

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
  double *DT = (double *)malloc(sizeof(double) * nx * ny);

  // COPY ORIGINAL DATA TO A DOUBLE FLOAT MATRIX
  // WHICH WILL ACT AS SOURCE FOR CREATING __m256d MATRIX.
  // This needs to be done because __m256d gets loaded with
  // continuous memory therefore the source memory should be
  // doubles (8-byte) not floats (4-byte).
  for (unsigned int r = 0; r < ny; r++) {
    for (unsigned int c = 0; c < nx; c++) {
      DT[c + r * nx] = (double)data[c + r * nx];
    }
  }

  // Vector Matrix
  __mmask8 pad_mask;
  __m256d *IT;
  int mask_bits;

  if (posix_memalign((void**)&IT, align_boundary, ny * pad_nx * capacity * sizeof(double)) != 0) {
    // Return from function, this will cause
    // address sanitizer issuer in the testing framework
    // as other memory has not been freed.
    // We dont care if we are not able to allocate memory for the __m256d
    // all is lost.
    return;
  }

  // Run the loop for pad_nx - 1
  // because pad_nx - 1 vectors will alway be completely filled.
  // Only the last vector can be partially filled.
  for (unsigned int r = 0; r < ny; r++) {
    pad_mask = _cvtu32_mask8(15);
    for (unsigned int c = 0; c < pad_nx-1; c++) {
      IT[c + r * pad_nx] = _mm256_maskz_expandloadu_pd(pad_mask, DT + c * capacity + r * nx);
    }
    // Fill in the the last vector which can be partially filled.
    mask_bits = nx - ((pad_nx - 1) * capacity);
    pad_mask = _cvtu32_mask8(pow(2, mask_bits) - 1);
    //std::cout << mask_bits << " " << nx - ((pad_nx - 1) * capacity) << " " << nx << " " << pad_nx << "\n";
    IT[pad_nx - 1 + r * pad_nx] = _mm256_maskz_expandloadu_pd(pad_mask, DT + ((pad_nx - 1) * capacity) + r * nx);
  }

  //std::cout << "Printing original data \n";
  //print_m(ny, nx, data);

  //std::cout << "Priniting vector matrix \n";
  //print_matrix_vector_double(ny, pad_nx, IT);

  // Vector operations.
  // Calculate Mean for each row using Vector operations.
  for (unsigned int r = 0; r < ny; r++) {
    double sum = 0;
    __m256d acc = _mm256_setzero_pd();

    for (unsigned int c = 0; c < pad_nx; c++) {
      acc = acc + IT[c + r * pad_nx];
    }

    // Take horizontal sum
    __m128d vlow  = _mm256_castpd256_pd128(acc);
    __m128d vhigh = _mm256_extractf128_pd(acc, 1); // high 128
    vlow  = _mm_add_pd(vlow, vhigh);               // reduce down to 128

    __m128d high64 = _mm_unpackhi_pd(vlow, vlow);
    sum =  _mm_cvtsd_f64(_mm_add_sd(vlow, high64));
    row_means[r] = sum / nx;       // Sum is divided by original nx not pad_nx
  }

  // Normalize the matrix so that
  // for each row arithmetic mean is zero.
  // This can be done by subtracting
  // each element of the row by the arithmetic mean of the row.

  for (unsigned int r = 0; r < ny; r++) {
    __m256d mean = _mm256_set_pd(row_means[r], row_means[r], row_means[r], row_means[r]);
    __m256d zeros = _mm256_setzero_pd();
    for (unsigned int c = 0; c < pad_nx; c++) {
      IT[c + r * pad_nx] = IT[c + r * pad_nx] - mean;
    }
    // Zero out the padded doubles in the last vector.
    // This is required because next stage is to take squared sum of all elements
    // if padded doubles has non-zero value it will produce
    // wrong results.
    mask_bits = nx - ((pad_nx - 1) * capacity);
    pad_mask = _cvtu32_mask8(pow(2, mask_bits) - 1);
    pad_mask = _knot_mask8(pad_mask);
    IT[pad_nx - 1 + r * pad_nx] = _mm256_mask_and_pd(IT[pad_nx - 1 + r * pad_nx], pad_mask, IT[pad_nx - 1 + r * pad_nx], zeros);
  }

  // std::cout << "Priniting vector matrix after mean normalization\n";
  // print_matrix_vector_double(ny, pad_nx, IT);
  // free(IT);

  // VECTOR OPS
  // Calculate Squared Sum of this new matrix.
  for (unsigned int r = 0; r < ny; r++) {
    double sq_sum = 0;
    __m256d acc = _mm256_setzero_pd();

    for (unsigned int c = 0; c < pad_nx; c++) {
      acc = acc + IT[c + r * pad_nx] * IT[c + r * pad_nx];
    }
    // Take horizontal sum
    __m128d vlow  = _mm256_castpd256_pd128(acc);
    __m128d vhigh = _mm256_extractf128_pd(acc, 1); // high 128
    vlow  = _mm_add_pd(vlow, vhigh);               // reduce down to 128

    __m128d high64 = _mm_unpackhi_pd(vlow, vlow);
    sq_sum =  _mm_cvtsd_f64(_mm_add_sd(vlow, high64));

    row_sq_sums[r] = sqrt(sq_sum);
  }

  // Normalize T matrix so that sum of squared each is zero.
  for (unsigned int r = 0; r < ny; r++) {
    __m256d root = _mm256_set_pd(row_sq_sums[r], row_sq_sums[r], row_sq_sums[r], row_sq_sums[r]);

    for (unsigned int c = 0; c < pad_nx; c++) {
      IT[c + r * pad_nx] = IT[c + r * pad_nx] / root;
    }
    // We do not need to zero the padded doubles in this normalization.
    // because they were already zero and getting divided by root will also produce zero.
  }


  // std::cout << "Priniting vector matrix after sq sum normalization\n";
  // print_matrix_vector_double(ny, pad_nx, IT);
  //free(IT);

  // VECTOR IMPLEMENTATION
  __m256d *temp;
  if (posix_memalign((void**)&temp, align_boundary,  pad_nx * capacity * sizeof(double)) != 0) {
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
  free(IT);
  free(temp);
  free(DT);
}
