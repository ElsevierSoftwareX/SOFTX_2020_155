void LSC_prc_inv(double *in, int inSize, double *out, int outSize)
{
if(in[2] > in[0])
	out[0] = in[1]*-1;
else
	out[0] = in[1];

}
