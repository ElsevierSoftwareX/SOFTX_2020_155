/* PREDEFINEDFILTER.c
 * 
 * This function implements a hard coded filter. band-limited rms filter
 * Authors:  JCB
 * March 8 2012
 */


/*  Two filters: 100 mHz high pass and 10 mHz low pass.
 *  
 * 
 */

#define LOCAL_TAPS 3

// 100mHz 6th order Butterworth High Pass
static double coeff100mHzHP[13] = {0.9999814787757688,
-1.9999814786838519, 0.9999814787757688, -2.0000000000000000, 1.0000000000000000,
-1.9999864413972834, 0.9999864414892007, -2.0000000000000000, 1.0000000000000000,
-1.9999950371273625, 0.9999950372192801, -2.0000000000000000, 1.0000000000000000};

// 10mHz 6th order Butterworth Low Pass
static double coeff10mHzLP[13] = {1.213441511757052e-38,
-1.9999981478612208, 0.9999981478621400, 2.0000000000000000, 1.0000000000000000,
-1.9999986441397286, 0.9999986441406479, 2.0000000000000000, 1.0000000000000000,
-1.9999995037199008, 0.9999995037208200, 2.0000000000000000, 1.0000000000000000};

// Variable to store history
static double fixedFilterHistory[MAX_HISTRY];


/* This function implements a hard coded 100 mHz High pass filter.
 */
void FILTER100MHZHP(double *argin, int nargin, double *argout, int nargout){
	static double reset_history = 0;
	int ii = 0;

	reset_history = argin[1];
	if (reset_history == 1) {
		for (ii = 0; ii < MAX_HISTRY; ii++) {
			fixedFilterHistory[ii] = 0.0;
		}
	}

	argout[0] = iir_filter(argin[0], &coeff100mHzHP[0],LOCAL_TAPS, &fixedFilterHistory[0]);
	

}

void FILTER10MHZLP(double *argin, int nargin, double *argout, int nargout){
	static double reset_history = 0;
        int ii = 0;

	reset_history = argin[1];
        if (reset_history == 1) {
                for (ii = 0; ii < MAX_HISTRY; ii++) {
                        fixedFilterHistory[ii] = 0.0;
                }
        }

	argout[0] = iir_filter(argin[0], &coeff10mHzLP[0],LOCAL_TAPS, &fixedFilterHistory[0]);
}
