

#define M_PI 	3.14159265358979323846

inline double inputFilterModuleRamp(
	double *v, 	/* current value in/out */
	double nv,	/* new value from Epics */
	double tramp,	/* Ramping time in seconds */
	double *step) 	/* current ramping step in/out */
{
	if (*step) { // currently ramping
		// Finish ramping right away if user changed ramp time to 0
		if (tramp == 0.0) {
			*step = 0;
			*v = nv;
		} else {
			return nv - (((nv - *v) / (double)( CYCLE_PER_SECOND * tramp)) * *step--);
		}
	} else { // not ramping
		if (*v != nv) {
			// Initiate ramping
			*step = CYCLE_PER_SECOND * tramp;
		}
	}
	return *v;
}

inline void inputFilterModule(
	double in,						/* input IN0 */
	double *old_out,					/* input IN1_PREV, output IN1 */
	double *old_val,					/* input VAL_PREV, output VAL */
	double offset,						
	double *pk, double *pp, double *pz,			/* input current value, output new values (after ramping was done */
	double epics_k, double epics_p, double epics_z,		/* EPICS record values, their change initiates ramping */
	double k_tramp, double p_tramp, double z_tramp,		/* EPICS records, ramp times in seconds */
	unsigned long *ks, unsigned long *ps, unsigned *zs)	/* ramping steps, in and out */
{
	double p = inputFilterModuleRamp(pp, epics_p, p_tramp, ps) * M_PI/CYCLE_PER_SECOND;
	double a = (1.0 - p) / (1.0 + p);
	double z = inputFilterModuleRamp(pz, epics_z, z_tramp, zs) * M_PI/CYCLE_PER_SECOND;
	double b = (1.0 - z) / (1.0 + z);
	double newval = inputFilterModuleRamp(pk, epics_k, k_tramp, ks) * (in + offset);
	double out = newval - b * *old_val + a * *old_out;
	*old_out = out;
	*old_val = newval;
}
