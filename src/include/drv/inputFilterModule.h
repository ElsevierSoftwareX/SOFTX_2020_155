
// TODO: write ramping code
//
#define M_PI 	3.14159265358979323846

inline void inputFilterModule(
	double in,						/* input IN0 */
	double *old_out,					/* input IN1_PREV, output IN1 */
	double *old_val,					/* input VAL_PREV, output VAL */
	double offset,						
	double *pk, double *pp, double *pz,			/* input current value, output new values (after ramping was done */
	double epics_k, double epics_p, double epics_z,		/* EPICS record values, their change initiates ramping */
	double k_tramp, double p_tramp, double z_tramp)		/* EPICS records, ramp times in seconds */
{
	double p = *pp * M_PI/CYCLE_PER_SECOND;
	double a = (1.0 - p) / (1.0 + p);
	double z = *pz * M_PI/CYCLE_PER_SECOND;
	double b = (1.0 - z) / (1.0 + z);
	double newval = *pk * (in + offset);
	double out = newval - b * *old_val + a * *old_out;
	*old_out = out;
	*old_val = newval;
}
