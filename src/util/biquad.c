
#include <stdio.h>
#include <stdlib.h>


/// Recalculate second order section itno a biquad form
int
main(int argc, char *argv[]) {
unsigned i;
double a[4];
double b[4];
for (i = 0; i < 4; i++) {
	a[i] = atof(argv[i+1]);
///	printf("%.16f\n", a[i]);
}
b[0] = - a[0] - 1.0;
b[1] = - a[1] - a[0] - 1.0;
b[2] = a[2] - a[0];
b[3] = a[3] - a[1] + a[2] - a[0];

printf ("%.16f, %.16f, %.16f, %.16f\n", b[0], b[1], b[2], b[3]);
}
