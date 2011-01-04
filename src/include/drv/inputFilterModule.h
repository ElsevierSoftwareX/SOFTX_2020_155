

inline void inputFilterModule(double in, double *old_out, double *old_val, double offset, double k, double p, double z)
{
	double a = (1.0 - p) / (1.0 + p);
	double b = (1.0 - z) / (1.0 + z);
	double newval = k * (in + offset);
	double out = newval - b * *old_val + a * *old_out;
	*old_out = out;
	*old_val = newval;
}
