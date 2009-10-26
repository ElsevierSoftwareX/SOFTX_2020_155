/* adaptive filter algorithm, based on NLMS (normalized least-mean-square)
* and filtered X-LMS algorithms.
*
* This code is intended to be used in the LIGO Bork-space environment,
* initially as an arbitrary C code block.  As such its interface to the
* world is through an input array and an output array.  The expected data
* order in these arrays are:
*
* datIn[0] = reset flag
*   change from previous value to any value other than zero to reset
* datIn[1] = number of FIR coeffients (on reset)
* datIn[2] = downsample ratio (on reset)
* datIn[3] = corr to error extra delay (on reset)
* datIn[4] = number of aux channels (on reset)
* datIn[5] = adaptation gain
* datIn[6] = decay rate (fraction per downsampled cycle)
* datIn[7] = not used
* datIn[8] = diagnostic request 1
* datIn[9] = diagnostic request 2
* datIn[10] = error signal
* datIn[11] = auxiliary channel 1, signal path
* datIn[12] = auxiliary channel 1, adaptation path
* datIn[13] = auxiliary channel 2, signal path
* datIn[14] = auxiliary channel 2, adaptation path
* ... more auxiliary channel pairs ...
* 
* datOut[0] = correction signal
* datOut[1] = diagnostic signal 1
* datOut[2] = diagnostic signal 2
*/

#define XF_MAX_FIR 16383
#define XF_MAX_DELAY 1024
#define XF_MAX_AUX 32

/*
* Structs
*/
typedef struct adaptive_filter_aux
{
  double prevNorm;		// previous signal norm

  double bufDelay[XF_MAX_DELAY];		// delay buffer
  double bufFIR[XF_MAX_FIR + 1];		// FIR buffer
  double bufAdapt[XF_MAX_FIR + 1];		// adaptation buffer
  double coefFIR[XF_MAX_FIR + 1];		// FIR coefficients
} adaptive_filter_aux;

typedef struct adaptive_filter_state
{
  double resetFlag;		// reset flag

  int nDown;			// downsampling factor
  int iDown;			// downsample counter

  int nDelay;			// aux delay
  int iDelay;			// delay counter

  int nFIR;			// number of FIR coeffs
  int iWait;			// reset wait counter (wait for nFIR)

  double prevCorr;		// previous correction signal

  int nAux;			// number of auxiliary channels
  adaptive_filter_aux aux[XF_MAX_AUX];	// auxiliary channel state
} adaptive_filter_state;

// Coefficient storage
double *cstor = (double *)(((char *)pLocalEpics) + 0x3f00000);


/*
* FE Function
* Save the coefficients to (((char *)pLocalEpics) + 0x3f00000)
*/

void TOP_XFCODE(double* datIn, int nIn, double* datOut, int nOut)
{
  static int save_coeffs_state = 0;
  static int restore_coeffs_state = 0;
  static int isFirst = 1;
  static adaptive_filter_state state;

  // misc vars
  int i, j;		// loop counters
  int isSamp;	  	// is this a new sample for the downsampled data?
  int iDelay;		// current delay buffer index
  double adaptGain;	// adaptation gain (mu)
  double decayRate;	// FIR coeff decay rate

  // temporary signals
  double sigErr;	// filtered error signal
  double sigCorr;	// auxiliar signal (correction path)
  double sigAdapt;	// auxiliar signal (adaptation path)

  double delta;		// coef change in this cycle
  double corr, norm;	// signal temp's

  // pointers into FIR and adaptation buffers
  adaptive_filter_aux* thisAux;
  double* pBufFIR;
  double* pBufAdapt;
  double* pCoefFIR;

  volatile int save_coeffs = pLocalEpics->ass.COEFF_SAVE;
  volatile int restore_coeffs = pLocalEpics->ass.COEFF_RESTORE;

  // make sure there are enough signals
  if( nIn < 12 )
  {
    printf("Not enough input signals %d < 12", nIn);
    return;
  }

  // ================ RESET
  if( isFirst || state.resetFlag < datIn[0] )
  {
    // reset isFirst static
    isFirst = 0;

    // reset with new parameters
    state.nFIR = (int) datIn[1];	// number of FIR coeffs
    state.nDown = (int) datIn[2];	// downsampling factor
    state.nDelay = (int) datIn[3];	// extra aux delay
    state.nAux = (int) datIn[4];	// number of auxiliary channels

    state.iDown = 0;			// downsample counter
    state.iDelay = 0;			// delay counter
    state.iWait = 0;			// reset wait counter

    state.prevCorr = 0.0;		// no previous correction

    // add sample-and-hold delay, and one sample turn around delay
    state.nDelay += state.nDown / 2;

    // ======== Check for Sanity
    printf("XFCODE: nFIR %d, nDelay %d, nAux %d\n",
            state.nFIR, state.nDelay, state.nAux);
    if( state.nFIR > XF_MAX_FIR || state.nFIR < 1 )
    {
      printf("Invalid nFIR = %d (must be >= 1 and <= %d)\n",
	     state.nFIR, XF_MAX_FIR);
      state.nFIR = 500;
    }
    if( state.nDelay > XF_MAX_DELAY || state.nDelay < 1 )
    {
      printf("Invalid nDelay %d (must be >= 1 and <= %d)\n",
	     state.nDelay, XF_MAX_DELAY);
      state.nDelay = 5 + state.nDown / 2;
    }
    if( state.nAux > XF_MAX_AUX || state.nAux < 1 )
    {
      printf("Invalid nAux %d (must be >= 1 and <= %d)\n",
	     state.nAux, XF_MAX_AUX);
      state.nAux = 1;
    }
    if( state.nAux > 10 + 2 * nIn )
    {
      printf("Invalid nAux %d > 10 + 2 * %d\n", state.nAux, nIn);
      state.nAux = 1;
    }

    // clear the delay buffer
    for( i = 0; i < state.nAux; i++ )
      for( j = 0; j < XF_MAX_DELAY; j++ )
        state.aux[i].bufDelay[j] = 0.0;

  }

  // Load coeffs from storage
  if (restore_coeffs_state == 0 && restore_coeffs == 1) {
    // Only if the number of coeffs didn't change
    if (cstor[0] == (double) state.nFIR 
        && cstor[1] == (double) state.nAux) {
      		printf("Restoring %d coefficients for %d Aux inputs\n", state.nFIR, state.nAux);
      		for (i = 0; i < state.nAux; i++) {
			printf("Aux input %d\n", i);
      			for (j = 0; j < state.nFIR; j++) {
    				thisAux = state.aux + i;
        			thisAux->coefFIR[j] = cstor[2 + i * state.nFIR + j];
				if (j % 100 == 0)
        				printf("\tcoeff #%d = %f\n", j, thisAux->coefFIR[j]);
			}
		}
      } else {
      		printf("Not restoring the coeffs nAux=%d; nFIR=%d; (old %d %d)\n",  state.nFIR, state.nAux, (int)cstor[0], (int)cstor[1]);
      }
  }
  restore_coeffs_state = restore_coeffs;

  // Save coefficients
  if (save_coeffs_state == 0 && save_coeffs == 1) {
    int i, j;
    
    printf("Saving coeffs; nAux=%d nFIR=%d\n", state.nAux, state.nFIR);
    // Save the number of coeffs
    cstor[0] = (double)state.nFIR;

    // Save the number of Aux inputs
    cstor[1] = (double)state.nAux; 

    for (i = 0; i < state.nAux; i++ ) {
      adaptive_filter_aux* thisAux = state.aux + i;
      for (j = 0; j < state.nFIR; j++ ) {
	// Store
        pCoefFIR = thisAux->coefFIR;
	cstor[2 + i * state.nFIR + j] = *pCoefFIR;
	pCoefFIR++;
      }
    }
  }
  save_coeffs_state = save_coeffs;

  // update reset flag, nDelay, etc.
  state.resetFlag = datIn[0];
  state.nDelay = (int) datIn[3];	// extra aux delay
  adaptGain = datIn[5];
  decayRate = datIn[6];

  // add sample-and-hold delay, and one sample turn around delay
  state.nDelay += state.nDown / 2;

  if( state.nDelay > XF_MAX_DELAY || state.nDelay < 1 )
    state.nDelay = 5 + state.nDown / 2;

  // ================ Update State
  // downsample counter
  state.iDown++;
  isSamp = (state.iDown >= state.nDown);
  if( isSamp )
    state.iDown = 0;

  // delay counter
  iDelay = state.iDelay++;
  if( state.iDelay >= state.nDelay )
    state.iDelay = 0;

  // reset wait counter
  if( isSamp )
  {
    // increment wait counter
    if( state.iWait < 10 * state.nFIR )
      state.iWait++;

    // modify adaptation gain for soft start
    if( state.iWait < state.nFIR )
    {
      adaptGain = 0.0;
      decayRate = 1.0;  // clear FIR coeffs after reset
    }
    else if( state.iWait < 2 * state.nFIR )
    {
      adaptGain *= state.iWait - state.nFIR;
      adaptGain /= state.nFIR;
    }
  }

  // ================ Process Signals

  // filter error signal (or just copy)
  sigErr = datIn[10];
  
  // process auxiliar signals to make correction signal
  if( isSamp )
    corr = 0.0;
  else
    corr = state.prevCorr;

  // Save the number of coeffs
  //if (st) cstor[0] = (double)state.nFIR;

  // Save the number of Aux inputs
  //if (st) cstor[1] = (double)state.nAux; 

  for( i = 0; i < state.nAux; i++ )
  {
    // pointer for easy access
    thisAux = state.aux + i;

    // correction path input
    sigCorr = datIn[11 + 2 * i];
    
    // adaptation path input, with circular delay buffer
    if( state.nDelay <= 0 )
      sigAdapt = datIn[12 + 2 * i];
    else
    {
      sigAdapt = thisAux->bufDelay[iDelay];
      thisAux->bufDelay[iDelay] = datIn[12 + 2 * i];
    }

    // ======== Downsampled Operations
    if( isSamp )
    {
      // compute the adaptation change
      if( thisAux->prevNorm != 0.0 )
	delta = adaptGain * sigErr / thisAux->prevNorm;
      else
	delta = 0.0;

      // put sigCorr and sigAdapt at end of buffers, after HP filtering
      pBufFIR = thisAux->bufFIR;
      pBufAdapt = thisAux->bufAdapt;
      pCoefFIR = thisAux->coefFIR;

      pBufFIR[state.nFIR] = sigCorr;
      pBufAdapt[state.nFIR] = sigAdapt;

      // computer FIR output and perform adaptation
      norm = 0.0;


      for( j = 0; j < state.nFIR; j++ )
      {
	// FIR
	*pBufFIR = *(pBufFIR + 1);		// shift the buffer
	corr += (*pCoefFIR) * (*pBufFIR);	// add to the output

	// Adaptation
	*pBufAdapt = *(pBufAdapt + 1);		// shift the buffer
	*pCoefFIR *= (1.0 - decayRate);		// decay FIR coef
	*pCoefFIR += delta * (*pBufAdapt);	// update FIR coef
	norm += (*pBufAdapt) * (*pBufAdapt);	// add to the norm

	// Store
//	if (st) cstor[2 + i * state.nFIR + j] = *pCoefFIR;

	// move pointers
	pBufFIR++;
	pBufAdapt++;
	pCoefFIR++;
      }

      // keep the norm for next time
      thisAux->prevNorm = norm;
    }
  }
  
  // limit and buffer the correction signal
  if( !(corr < 0.0 || corr > 0.0) )
    corr = 0.0;          // catch NaN
  else if( corr > 1e9 )
    corr = 1e9;          // catch big positive numbers
  else if( corr < -1e9 )
    corr = -1e9;         // catch big negative numbers

  state.prevCorr = corr;

  // ================ Assign Outputs

  // main correction signal
  if( nOut > 0 )
    datOut[0] = corr;

  // filtered error signal
  if( nOut > 1 )
    datOut[1] = sigCorr;

  // unfiltered corr signal
  if( nOut > 2 )
    datOut[2] = 0.0;

}
