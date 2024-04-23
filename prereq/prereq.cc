struct Result {
    float avg[3];
};

/*
This is the function you need to implement. Quick reference:
- x coordinates: 0 <= x < nx
- y coordinates: 0 <= y < ny
- horizontal position: 0 <= x0 < x1 <= nx
- vertical position: 0 <= y0 < y1 <= ny
- color components: 0 <= c < 3
- input: data[c + 3 * x + 3 * nx * y]
- output: avg[c]
*/
Result calculate(int ny, int nx, const float *data, int y0, int x0, int y1, int x1) {
  double c0 = 0.0f;
  double c1 = 0.0f;
  double c2 = 0.0f;

  double t0 = 0.0f;
  double t1 = 0.0f;
  double t2 = 0.0f;

  double l = (y1 - y0) * (x1 - x0);

  double r0 = 0.0f;
  double r1 = 0.0f;
  double r2 = 0.0f;

  for (int j = y0; j < y1; j++) {
    for (int i = x0; i < x1; i++) {
      t0 = (double)data[0 + 3 * i + 3 * nx * j];
      t1 = (double)data[1 + 3 * i + 3 * nx * j];
      t2 = (double)data[2 + 3 * i + 3 * nx * j];

      c0 = c0 + t0;
      c1 = c1 + t1;
      c2 = c2 + t2;
    }
  }
  r0 = c0 / l;
  r1 = c1 / l;
  r2 = c2 / l;

  Result result{{(float)r0, float(r1), float(r2)}};
  return result;
}
