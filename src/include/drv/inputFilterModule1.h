///	@file inputFilterModule1.h
///	@brief File contains routines to support single pole/ single zero filter
///modules
///<		with EPICS inputs.

static double
inputFilterModuleRamp1(double *v,           /* current value in/out */
                       double nv,           /* new value from Epics */
                       double tramp,        /* Ramping time in seconds */
                       unsigned long *step, /* current ramping step in/out */
                       int doramp)          /* ramp go flag */
{
  if (doramp) {
    if (*step) { // currently ramping
      // Finish ramping right away if user changed ramp time to 0
      if (tramp == 0.0) {
        *step = 0;
        *v = nv;
      } else {
        (*step)--;
        if (!*step)
          *v = nv;
        return nv -
               (((nv - *v) / (double)(CYCLE_PER_SECOND * tramp)) * (1 + *step));
      }
    } else { // not ramping
      if (*v != nv) {
        // Ramp is not set or set incorrectly
        if (tramp <= 0.) {
          *v = nv;
        } else {
          // Initiate ramping
          *step = CYCLE_PER_SECOND * tramp;
        }
      }
    }
  }
  return *v;
}

void inputFilterModule1(
    double in,       /* input IN0 */
    double *old_out, /* input IN1_PREV, output IN1 */
    double *old_val, /* input VAL_PREV, output VAL */
    double offset, double *pk, double *pp,
    double *
        pz, /* input current value, output new values (after ramping was done */
    double epics_k, double epics_p,
    double epics_z, /* EPICS record values, their change initiates ramping */
    double tramp,
    int *doramp, /* EPICS records, ramp time in seconds, rapm go flag*/
    unsigned long *ks, unsigned long *ps,
    unsigned long *zs) /* ramping steps, in and out */
{
  double p = inputFilterModuleRamp1(pp, epics_p, tramp, ps, *doramp) *
             (double)(M_PI / (double)CYCLE_PER_SECOND);
  double a = (1.0 - p) / (1.0 + p);
  double z = inputFilterModuleRamp1(pz, epics_z, tramp, zs, *doramp) *
             (double)(M_PI / (double)CYCLE_PER_SECOND);
  double b = (1.0 - z) / (1.0 + z);
  double newval =
      inputFilterModuleRamp1(pk, epics_k, tramp, ks, *doramp) * (in + offset);
  double out = newval - b * *old_val + a * *old_out;
  *old_out = out;
  *old_val = newval;
  // See if we ough to reset RAMP GO variable
  if (*doramp && !*ks && !*ps && !*zs)
    *doramp = 0;
}
