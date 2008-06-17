extern int npfd[];
double TMP_PDE;

void HMY_PDE_frompde(double *in, int inSize, double *out, int outSize) {
  static double PDE_LAST[8] = {0,0,0,0,0,0,0,0};
  int n;
  int numread;
  for(n = 0; n < 8; n++) {
    numread = read(npfd[n], &TMP_PDE, sizeof(double));
    if (numread == sizeof(double)) {
      out[n] = TMP_PDE;
      PDE_LAST[n] = TMP_PDE;
    }
    else out[n] = PDE_LAST[n];
  }
}
