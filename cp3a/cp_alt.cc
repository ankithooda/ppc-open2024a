#include <iostream>
#include <cmath>
#include <vector>
#include <immintrin.h>
#include <cmath>
#include <x86intrin.h>

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

  //unsigned long before, after;
  unsigned int ny = (unsigned int)orig_y;
  unsigned int nx = (unsigned int)orig_x;
  unsigned int pad_nx = (nx + capacity - 1) / capacity;

  // std::cout << "Vector Bounds " <<  ny << " " << pad_nx << "\n";
  // std::cout << "Original Bounds " <<  ny << " " << nx << "\n";

  double *row_sq_sums = (double *)malloc(sizeof(double) * ny);
  double *row_means = (double *)malloc(sizeof(double) * ny);
  double *DT = (double *)malloc(sizeof(double) * nx * ny);

  /////////////////////////////// DOUBLE FLOAT COPY ////////////////////////////
  ///
  ///                             PARALLEL OK       ////////////////////////////
  ///
  //////////////////////////////////////////////////////////////////////////////
  // COPY ORIGINAL DATA TO A DOUBLE FLOAT MATRIX
  // WHICH WILL ACT AS SOURCE FOR CREATING __m256d MATRIX.
  // This needs to be done because __m256d gets loaded with
  // continuous memory therefore the source memory should be
  // doubles (8-byte) not floats (4-byte).
  //before = __rdtsc();
  #pragma omp parallel for
  for (unsigned int r = 0; r < ny; r++) {
    for (unsigned int c = 0; c < nx; c++) {
      DT[c + r * nx] = (double)data[c + r * nx];
    }
  }
  //after = __rdtsc();
  //std::cout << after - before << " DT Copy \n";

  /////////////////////////////// DOUBLE FLOAT COPY END ////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////


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

  /////////////////////////////// VECTOR COPY //////////////////////////////////
  ///
  ///                             PARALLEL NOT OK  ////////////////////////////
  ///
  /////////////////////////////////////////////////////////////////////////////

  // Run the loop for pad_nx - 1
  // because pad_nx - 1 vectors will alway be completely filled.
  // Only the last vector can be partially filled.
  //before = __rdtsc();
  //#pragma omp parallel for
  for (unsigned int r = 0; r < ny; r++) {
    pad_mask = _cvtu32_mask8(15);
    for (unsigned int c = 0; c < pad_nx-1; c++) {
      IT[c + r * pad_nx] = _mm256_maskz_loadu_pd(pad_mask, DT + c * capacity + r * nx);
    }
    // Fill in the the last vector which can be partially filled.
    mask_bits = nx - ((pad_nx - 1) * capacity);
    pad_mask = _cvtu32_mask8(pow(2, mask_bits) - 1);
    //std::cout << mask_bits << " " << nx - ((pad_nx - 1) * capacity) << " " << nx << " " << pad_nx << "\n";
    IT[pad_nx - 1 + r * pad_nx] = _mm256_maskz_expandloadu_pd(pad_mask, DT + ((pad_nx - 1) * capacity) + r * nx);
  }
  //after = __rdtsc();
  //std::cout << after - before << " Vector Copy \n";

  /////////////////////////////// VECTOR COPY END ///////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////


  /////////////////////////////// 1st NORMAL //////////////////////////////////
  ///
  ///                             PARALLEL NOT OK  ////////////////////////////
  ///
  /////////////////////////////////////////////////////////////////////////////

  // Vector operations.
  // Calculate Mean for each row using Vector operations.
  //before = __rdtsc();

  //#pragma omp parallel for
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
  //#pragma omp parallel for
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
  //after = __rdtsc();
  //std::cout << after - before << " 1st norm \n";

  /////////////////////////////// 1st NORMAL END   //////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////


  /////////////////////////////// 2nd NORMAL //////////////////////////////////
  ///
  ///                             PARALLEL OK      ////////////////////////////
  ///
  /////////////////////////////////////////////////////////////////////////////

  // VECTOR OPS
  // Calculate Squared Sum of this new matrix.
  //before = __rdtsc();
  #pragma omp parallel for
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
  #pragma omp parallel for
  for (unsigned int r = 0; r < ny; r++) {
    __m256d root = _mm256_set_pd(row_sq_sums[r], row_sq_sums[r], row_sq_sums[r], row_sq_sums[r]);

    for (unsigned int c = 0; c < pad_nx; c++) {
      IT[c + r * pad_nx] = IT[c + r * pad_nx] / root;
    }
    // We do not need to zero the padded doubles in this normalization.
    // because they were already zero and getting divided by root will also produce zero.
  }
  //after = __rdtsc();
  //std::cout << after - before << " 2nd norm \n";

  /////////////////////////////// 2nd NORMAL END //////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////


  /////////////////////////////// MATRIX MULT //////////////////////////////////
  ///
  ///                             PARALLEL OK      ////////////////////////////
  ///
  /////////////////////////////////////////////////////////////////////////////

  //before = __rdtsc();
  unsigned factor = 3;
  //unsigned bound_nx = pad_nx < factor ? factor : pad_nx;
  unsigned bound_r, bound_c;
  bound_r = bound_c = ny < factor ? factor : ny;

  #pragma omp parallel for
  for (unsigned int r = 0; r < bound_r; r = r + 3) {
    //#pragma omp parallel for
    for (unsigned int c = 0; c < bound_c; c = c + 3) {
      //if (r > c) {continue;}       // Go to next r, c pair

      // Iterate on all 9 results simlultaneously
      // Get memory for 9 accumulators & 9 double sums
      __m256d acc4[9];
      double sum4[9];
      // if (posix_memalign((void**)&acc4, align_boundary, factor * factor * capacity * sizeof(double)) != 0) {
      //   ;
      // }
      // double *sum4 = (double*)malloc(factor * factor * sizeof(double));

      // Set all four accumulators and double sums to zero, initialized above.
      for (unsigned int a = 0; a < factor * factor; a++) {
        acc4[a] = _mm256_setzero_pd();
        sum4[a] = 0;
      }

      // 6 values for r + 0, r + 1, r + 2, c + 0, c + 1, c + 2
      unsigned int r0 = r;
      unsigned int r1 = r + 1;
      unsigned int r2 = r + 2;
      unsigned int c0 = c;
      unsigned int c1 = c + 1;
      unsigned int c2 = c + 2;

      // We have 9 result cells
      // [r0, c0] [r0, c1] [r0, c2]
      // [r1, c0] [r1, c1] [r1, c2]
      // [r2, c0] [r2, c1] [r2, c2]

      // Calculate all 9 accs in one single loop

      for (unsigned k = 0; k < pad_nx; k++) {
        //[r0, c0] [r0, c1] [r0, c2]
        if (r0 <= c0 && r0 < ny && c0 < ny) {
          acc4[0] = _mm256_fmadd_pd(IT[k + r0 * pad_nx], IT[k + c0 * pad_nx], acc4[0]);
        }
        if (r0 <= c1 && r0 < ny && c1 < ny) {
          acc4[1] = _mm256_fmadd_pd(IT[k + r0 * pad_nx], IT[k + c1 * pad_nx], acc4[1]);
        }
        if (r0 <= c2 && r0 < ny && c2 < ny) {
          acc4[2] = _mm256_fmadd_pd(IT[k + r0 * pad_nx], IT[k + c2 * pad_nx], acc4[2]);
        }

        // [r1, c0] [r1, c1] [r1, c2]
        if (r1 <= c0 && r1 < ny && c0 < ny) {
          acc4[3] = _mm256_fmadd_pd(IT[k + r1 * pad_nx], IT[k + c0 * pad_nx], acc4[3]);
        }
        if (r1 <= c1 && r1 < ny && c1 < ny) {
          acc4[4] = _mm256_fmadd_pd(IT[k + r1 * pad_nx], IT[k + c1 * pad_nx], acc4[4]);
        }
        if (r1 <= c2 && r1 < ny && c2 < ny) {
          acc4[5] = _mm256_fmadd_pd(IT[k + r1 * pad_nx], IT[k + c2 * pad_nx], acc4[5]);
        }

        // [r2, c0] [r2, c1] [r2, c2]
        if (r2 <= c0 && r2 < ny && c0 < ny) {
          acc4[6] = _mm256_fmadd_pd(IT[k + r2 * pad_nx], IT[k + c0 * pad_nx], acc4[6]);
        }
        if (r2 <= c1 && r2 < ny && c1 < ny) {
          acc4[7] = _mm256_fmadd_pd(IT[k + r2 * pad_nx], IT[k + c1 * pad_nx], acc4[7]);
        }
        if (r2 <= c2 && r2 < ny && c2 < ny) {
          acc4[8] = _mm256_fmadd_pd(IT[k + r2 * pad_nx], IT[k + c2 * pad_nx], acc4[8]);
        }
      }

      // Take horizontal sums for all 4 accs

      for (unsigned f = 0; f < factor * factor; f++) {
        __m128d vlow  = _mm256_castpd256_pd128(acc4[f]);
        __m128d vhigh = _mm256_extractf128_pd(acc4[f], 1);    // high 128
        vlow  = _mm_add_pd(vlow, vhigh);                     // reduce down to 128

        __m128d high64 = _mm_unpackhi_pd(vlow, vlow);
        sum4[f] =  _mm_cvtsd_f64(_mm_add_sd(vlow, high64));
      }

      if (r0 <= c0 && r0 < ny && c0 < ny) {
        result[c0 + r0 * ny] = (float)sum4[0];
      }

      if (r0 <= c1 && r0 < ny && c1 < ny) {
        result[c1 + r0 * ny] = (float)sum4[1];
      }

      if (r0 <= c2 && r0 < ny && c2 < ny) {
        result[c2 + r0 * ny] = (float)sum4[2];
      }
      ///////////////////////////////////////////////////////////
      if (r1 <= c0 && r1 < ny && c0 < ny) {
        result[c0 + r1 * ny] = (float)sum4[3];
      }

      if (r1 <= c1 && r1 < ny && c1 < ny) {
        result[c1 + r1 * ny] = (float)sum4[4];
      }

      if (r1 <= c2 && r1 < ny && c2 < ny) {
        result[c2 + r1 * ny] = (float)sum4[5];
      }
      ///////////////////////////////////////////////////////////
      if (r2 <= c0 && r2 < ny && c0 < ny) {
        result[c0 + r2 * ny] = (float)sum4[6];
      }
      if (r2 <= c1 && r2 < ny && c1 < ny) {
        result[c1 + r2 * ny] = (float)sum4[7];
      }
      if (r2 <= c2 && r2 < ny && c2 < ny) {
        result[c2 + r2 * ny] = (float)sum4[8];
      }
    }
  }
  //after = __rdtsc();
  //std::cout << after - before << " Matrix calc \n";

  /////////////////////////////// MATRIX MULT END //////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  // Free all allocated memory
  free(IT);
  //free(temp);
  free(DT);
  free(row_means);
  free(row_sq_sums);
}
