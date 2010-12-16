static char *versionId = "Version $Id$" ;
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

const int max_points = 1000000;


void integrand(double* out, const double* freq, const double* asp, int N)
{
	//----- Loop to invert PSDed data.  Also divide by f^(7/3).
	for (int i = 0; i < N; ++i) {
		out[i] = pow(asp[i],-2);
		out[i] *= (pow(freq[i],(-7.0/3.0)));
	}
}

double integrate(const double* freq, const double* data, int N)
{
	double total = 0.0;

	//----- Apply Trapezoidal Rule
	for (int i = 1; i < N-1; i++) {
		total += (data[i+1] + data[i]) * (freq[i+1] - freq[i]) / 2.;
	}
	return total;
}


double range(double f_7_3, double arm_length)
{
	//----- Set physical contants and parameters
        /* Range formula in kiloparsecs is (Finn & Chernoff 1993)  
             range = (3 * 1.84)^{1/3} * [
                       (5 M_chirp^{5/3} c^{1/3} f_{7/3})/(96 \pi^{4/3}\rho_0^2)
                     ]^{1/2} * ARM_LENGTH(m) / METERS_PER_KILOPARSEC
           where f_{7/3} is given by Calibrate, Integrand, Integrate in 
	   (nm)^{-2}*Hz^{-1/3} so ARM_LENGTH is measured in m, and where 
           the chirp mass for two 1.4 solar mass objects is 
             M_chirp = 1.219*SOLARMASS*NEWTONS_G/C^2
	*/
	const double SOLARMASS = 1.989E30;
	const double NEWTONS_G = 6.67E-11; 
	const double C = 299792458;
	const double METER_PER_KILOPARSEC = 3.086E19;
	const double RHO_o = 8;
	const double PI_CONST = 3.141592654;

  	double M = (1.219 * SOLARMASS * NEWTONS_G) / pow(C,2);
	double r_o = pow(M,5.0/6.0);
	r_o *= pow(5,0.5) * pow(96.0*pow(PI_CONST,4.0/3.0)*pow(RHO_o,2),-0.5);
	r_o *= pow(C,1.0/6.0) * pow(f_7_3,0.5);
	r_o *= pow((3*1.84),1.0/3.0);
        r_o *= 1.0/METER_PER_KILOPARSEC*arm_length;
	
	return(r_o);  
}

double compute_range (const double* freq, const double* asp, int N,
		      double arm_length)
{
	double* data = new double [N];
	integrand(data, freq, asp, N);
	double f_7_3 = integrate (freq, data, N);
	return range (f_7_3, arm_length);
}

int main (int argc, char* argv[])
{
	if (argc != 2) {
	   printf ("usage: InspiralRange arm_length < disp_file\n");
	   printf ("       arm_length in meters\n");
	   printf ("       disp_file in m/rtHz (two column format: freq/ampl)\n");
	   return 1;
	}
	double arm_length = 4000;
	sscanf (argv[1], "%lf", &arm_length);
	arm_length *= 1E9;
	
	double* freq = new double [max_points];
	double* asp = new double [max_points];
	int N = 0;
	
	while (!feof (stdin) && (N < max_points)) {
	   scanf ("%lf%lf", freq+N, asp+N);
	   asp[N] *= 1E9;
	   if (freq[N] > 1E-6) ++N;
	}
	double rkpc = compute_range (freq, asp, N, arm_length);
	
	printf ("Inspiral range is %g kpc\n", rkpc);
	
   	return 0;
}
