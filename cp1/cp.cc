/*
This is the function you need to implement. Quick reference:
- input rows: 0 <= y < ny
- input columns: 0 <= x < nx
- element at row y and column x is stored in data[x + y*nx]
- correlation between rows i and row j has to be stored in result[i + j*ny]
- only parts with 0 <= j <= i < ny need to be filled
*/
void correlate(int ny, int nx, const float *data, float *result) {

  double *row_sums = malloc(sizeof(double) * nx);
  double *row_means = malloc(sizeof(double) * nx);

  double *T = malloc(sizeof(double) * nx * ny);

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

  // Calculate Sum of this new matrix.
  for (int r = 0; r < ny; r++) {
    double sum = 0;

    for (int c = 0; c < nx; c++) {
      sum = sum + T[c + r * nx];
    }
    row_sums[r] = sum;
  }

  // Normalize T matrix so that sum of each is zero.
  for (int r = 0; r < ny; r++) {
    for (int c = 0; c < nx; c++) {
      T[c + r * nx] = T[c + r * nx] / row_sums[r];
    }
  }

}
