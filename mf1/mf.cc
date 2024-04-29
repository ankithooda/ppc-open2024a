#include <iostream>
#include <limits>
#include <random>
#include <array>
#include <algorithm>

/*
This is the function you need to implement. Quick reference:
- input rows: 0 <= y < ny
- input columns: 0 <= x < nx
- element at row y and column x is stored in in[x + y*nx]
- for each pixel (x, y), store the median of the pixels (a, b) which satisfy
  max(x-hx, 0) <= a < min(x+hx+1, nx), max(y-hy, 0) <= b < min(y+hy+1, ny)
  in out[x + y*nx].
*/

void mf(int ny, int nx, int hy, int hx, const float *in, float *out) {
  // nt is the maximum number of median window size.
  int nt = (2*hx+1) * (2*hy+1);

  double *t = (double *)malloc(sizeof(double) * nt);

  // For each pixel, calculate the median.
  // Iterate on row first then column.
  for (int y = 0; y < ny; y++) {
    for (int x = 0; x < nx; x++) {

      int x1 = std::max(x-hx, 0);
      int x2 = std::min(x+hx+1, nx);

      int y1 = std::max(y-hy, 0);
      int y2 = std::min(y+hy+1, ny);

      // Fill with zeros
      for (int t1 = 0; t1 < nt; t1++) {
        t[t1] = 0.0;
      }


      // Copy the pixel range to the temp array.
      // because we need nth_element modifies the original array.
      // cant use std::copy() as the source array
      // as defined by (x1->x2) and (y1->y2) bounds is not continuous.
      int t_len = 0;
      // iterate first on row then col.
      for (int ty = y1; ty < y2; ty++) {
        for (int tx = x1; tx < x2; tx++) {
          t[t_len] = in[tx + nx*ty];
          t_len++;
        }
      }

      double m1 = 0.0;
      double m2 = 0.0;

      std::vector<double> tarray(t, t + t_len);

      if ((t_len % 2) != 0) {
        std::nth_element(tarray.begin(), tarray.begin() + t_len / 2, tarray.end());
        m1 = tarray[t_len / 2];
        out[x + nx*y] = (float)m1;
      } else {

        std::nth_element(tarray.begin(), tarray.begin() + t_len / 2, tarray.end());
        m1 = tarray[t_len / 2];

        std::nth_element(tarray.begin(), tarray.begin() + (t_len / 2) - 1, tarray.end());
        m2 = tarray[(t_len / 2) - 1];

        out[x + nx*y] = (float)((m1 + m2) / 2.0);
      }
    }
  }
  free(t);
}
