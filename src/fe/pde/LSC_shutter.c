int curcycle;
void LSC_shutter(double *in, int inSize, double *out, int outSize)
{
if(in[2] == 0 || in[0] > in[1])
	out[0] = 1; // closed
else
	out[0] = 0; // open
}
