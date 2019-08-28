///	@file inputFilterModule.h
///	@brief File contains routines for running single pole/single zero filter
///modules 		with settings from EPICS.

/// Control ramping of filter parameters
static double
inputFilterModuleRamp(double *v,           /* current value in/out */
                      double nv,           /* new value from Epics */
                      double tramp,        /* Ramping time in seconds */
                      unsigned long *step) /* current ramping step in/out */
{
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
  return *v;
}

/// Perform single pole / single zero filter calcs.
///	@param[in] in		input IN0
///	@param[in,out] *old_out		input IN1_PREV, output IN1
///	@param[in,out] *old_val		input VAL_PREV, output VAL
///	@param[in] offset		filter DC offset
///	@param[in,out] *pk		input current gain value, output new value
///(after ramping was done)
///	@param[in,out] *pp		input current pole value, output new value
///(after ramping was done)
///	@param[in,out] *pz		input current zero value, output new value
///(after ramping was done)
///	@param[in] epics_k		EPICS gain value
///	@param[in] epics_p		EPICS pole value
///	@param[in] epics_z		EPICS zero value
///	@param[in] k_tramp		EPICS gain ramp time in seconds
///	@param[in] p_tramp		EPICS pole ramp time in seconds
///	@param[in] z_tramp		EPICS zero ramp time in seconds
///	@param[in,out] *ks		Gain ramping steps
///	@param[in,out] *ps		Pole ramping steps
///	@param[in,out] *zs		Zero ramping steps
void inputFilterModule(
    double in,       /* input IN0 */
    double *old_out, /* input IN1_PREV, output IN1 */
    double *old_val, /* input VAL_PREV, output VAL */
    double offset, double *pk, double *pp,
    double *
        pz, /* input current value, output new values (after ramping was done */
    double epics_k, double epics_p,
    double epics_z, /* EPICS record values, their change initiates ramping */
    double k_tramp, double p_tramp,
    double z_tramp, /* EPICS records, ramp times in seconds */
    unsigned long *ks, unsigned long *ps,
    unsigned long *zs) /* ramping steps, in and out */
{
  double p = inputFilterModuleRamp(pp, epics_p, p_tramp, ps) *
             (double)(M_PI / (double)CYCLE_PER_SECOND);
  double a = (1.0 - p) / (1.0 + p);
  double z = inputFilterModuleRamp(pz, epics_z, z_tramp, zs) *
             (double)(M_PI / (double)CYCLE_PER_SECOND);
  double b = (1.0 - z) / (1.0 + z);
  double newval =
      inputFilterModuleRamp(pk, epics_k, k_tramp, ks) * (in + offset);
  double out = newval - b * *old_val + a * *old_out;
  *old_out = out;
  *old_val = newval;
}
