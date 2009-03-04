/* 
* DC ERROR SIGNAL - compute DARM error signal from DC readout signal
* see T0900023
*
* This code is intended to be used in the LIGO Bork-space environment,
* as an arbitrary C code block.  As such its interface to the world is
* through an input array and an output array.  The expected data
* order in these arrays are:
*
* datIn[0] = Pas   = DC readout signal
* datIn[1] = Parm  = average arm power signal (forced greater than 1.0)
* datIn[2] = Pref  = reference arm power
* datIn[3] = Pdfct = contrast defect offset (a.k.a. kappa)
* datIn[4] = x0    = fringe offset (in pm)
* datIn[5] = xf    = offset cofficient
* datIn[6] = C2    = non-linear coefficient (for dx^2) 
* datIn[7] = Tramp = ramp time (in seconds)
* datIn[8] = load  = load bit (start ramp when this changes from 0 to 1)
* datIn[9] = TP1   = test-point selector
* 
* datOut[0] = output signal (dx + C2 * dx^2)
* datOut[1] = state flag (0 = ok, 1 = need load, 2 = ramping, 3 = 1 & 2)
* datOut[2] = test-point output
*
* Test-Point Selection Table:
* 0-5: post-ramp values for Pref, Pdfct, x0, xf, C2
* 8,9: Pas, Parm
* 10: Pas * Pref / Parm - Pdfct
* 11: xf^2 * Pas * Pref / Parm - Pdfct
* 12: xf^2 * Pas * Pref / Parm - Pdfct - x0^2
*
* TP 10-12 are defined to assist in calibration.  Recal that
*   x = x0 + dx
* then
*   TP[10] = (x / xf)^2
*   TP[11] = x^2
*   TP[12] = dx * (2 * x0 + dx)
*
* In particular, TP[12] should be small and mean-zero when locked
* on RF if the DCERR parameters match the RF offset (e.g., x0 and
* xf are set properly).
*/

#define OMC_DCERR_FS FE_RATE  /* sample rate */
//#define OMC_DCERR_FS 2048   /* sample rate */
#define OMC_DCERR_N_SIG 2   /* input signals (before ramping parameters) */
#define OMC_DCERR_N_RAMP 5  /* ramping parameters (before non-ramping) */

//#define abs fabs            /* whatever the FE likes for absolute-value */

inline double fabs(double x) {
  return (x>0 ? x : -x);
}

#define OMC_DCERR_OMC_DCERR OMC_DCERR

enum OMC_DCERR_Params
{
  OMC_DCERR_P_REF = 0,
  OMC_DCERR_P_DFCT = 1,
  OMC_DCERR_X_0 = 2,
  OMC_DCERR_X_F = 3,
  OMC_DCERR_C2 = 4,
  OMC_DCERR_T_RAMP = 5,
  OMC_DCERR_LOAD = 6,
  OMC_DCERR_TP1 = 7
};

/*
* Structs
*/
typedef struct OMC_DCERR_ParamState
{
  // running variables (updated each cycle while ramping)
  int isRamping;        // state
  double val;           // current value
  double dxPrev;        // previous change

  // ramp parameters (updated on load)
  double req;           // requested value
  double dxMax;         // maximum change in x in one step
  double ddxMax;        // maximum change in dx in one step

  // ramp construction parameters (user set, or fixed)
  double softRatio;     // ratio of time at constant velocity to accelleration
  double softVel;       // maximum velocity relative to linear ramp
  double minAccCycles;  // accelleration cycles (when ramping is interrupted)

} OMC_DCERR_ParamState;

/*
* Ramping Functions
* (static so they are only visible in to OMC_DCERR)
*/
void ParamInit(OMC_DCERR_ParamState* state, double xInit)
{
  state->isRamping = 0;
  state->val = xInit;
  state->dxPrev = 0.0;

  state->req = xInit;
  state->dxMax = 0.0;
  state->ddxMax = 0.0;

  state->softRatio = 4.0;
  state->softVel = 1.36;  // how to compute this from softRatio?
  state->minAccCycles = 0.05 * OMC_DCERR_FS;
}

void ParamLoad(OMC_DCERR_ParamState* state, double req, double tRamp)
{
  double inv_nRamp; // 1 / number of ramp cycles

  if( tRamp <= 0.0 )
  {
    // no ramp
    state->isRamping = 0;
    state->val = req;
    state->dxPrev = 0.0;

    state->req = req;
    state->dxMax = 0.0;
    state->ddxMax = 0.0;
  }
  else if( state->isRamping || req != state->val )
  {
    // setup ramp parameters
    inv_nRamp = 1.0 / (OMC_DCERR_FS * tRamp);
    state->req = req;
    state->dxMax = state->softVel * abs(req - state->val) * inv_nRamp;
    state->ddxMax = state->softRatio * state->dxMax * inv_nRamp;
        
    // if currently ramping, allow high accelleration
    if( state->isRamping )
      state->ddxMax = abs(state->dxMax + state->dxPrev) / state->minAccCycles;
      
    // start the ramp
    state->isRamping = 1;
  }
}

// call only if state->isRamping
// return is new parameter value
double ParamUpdate(OMC_DCERR_ParamState* state)
{
  double dxReq;  // distance to requested value
  double dxNow;  // current step size
  double dxNowAbs;  // abs(dxNow)
  double ddxNow; // current change in step size
  double nStop;  // steps required to stop (may be non-integer)
  double dxStop; // shortest distance required to stop
  double dxLand; // current step size to achieve "soft landing"

  // if not ramping, just return
  if( !state->isRamping )
    return state->val;

  // requested change
  dxReq = state->req - state->val;
        
  // apply slew limit
  if( dxReq > state->dxMax )
    dxNow = state->dxMax;
  else if( dxReq < -state->dxMax )
    dxNow = -state->dxMax;
  else
    dxNow = dxReq;
      
  // apply accelleration limit
  ddxNow = dxNow - state->dxPrev;
  if( ddxNow > state->ddxMax )
    dxNow = state->dxPrev + state->ddxMax;
  else if( ddxNow < -state->ddxMax )
    dxNow = state->dxPrev - state->ddxMax;

  // enforce "soft landing"
  dxNowAbs = abs(dxNow);
  if( dxNowAbs > state->ddxMax && dxNow * dxReq > 0.0 )
  {
    // number of decellerations required to stop
    nStop = dxNowAbs / state->ddxMax;

    // distance traveled before stopping
    // assuming max decelleration in all subsequent steps
    dxStop = dxNow * (1.5 + 0.5 * nStop);

    // dxNow required to make stopping distance = dxReq
    // (approximation for dxReq ~ dxStop)
    dxLand = dxNow + (dxReq - dxStop) / (1.5 + nStop);

    // if dxLand is slower than dxNow, use it
    if( abs(dxLand) < dxNowAbs )
      dxNow = dxLand;
  }

  // update state
  state->isRamping = !(dxNow == 0.0 && state->dxPrev == 0.0);
  if( dxNow == dxReq )
    state->val = state->req;
  else
    state->val += dxNow;
  state->dxPrev = dxNow;

  return state->val;
}

/*
* FE Function
*/

void OMC_DCERR(double* datIn, int nIn, double* datOut, int nOut)
{
  static int isFirst = 1;
  static OMC_DCERR_ParamState paramState[OMC_DCERR_N_RAMP];
  static double prevLoad = 0.0;

  double* param = datIn + OMC_DCERR_N_SIG;  // short-cut

  int i;           // counter
  int flag = 0;    // state flag

  double xf2;      // OMC_DCERR_X_F^2
  double x0;       // OMC_DCERR_X_0
  double pGain;    // linear gain for Pas, not including contrast defect
  double dx;       // the DC error signal

  // ================ Initialize
  if( isFirst )
  {
    isFirst = 0;
    for( i = 0; i < OMC_DCERR_N_RAMP; i++ )
      ParamInit(paramState + i, param[i]);
  }
  
  // ================ Load New Parameters
  if( param[OMC_DCERR_LOAD] != 0.0 && prevLoad == 0.0 )
    for( i = 0; i < OMC_DCERR_N_RAMP; i++ )
      ParamLoad(paramState + i, param[i], param[OMC_DCERR_T_RAMP]);
  prevLoad = param[OMC_DCERR_LOAD];

  // ================ Update Ramping Parameters
  for( i = 0; i < OMC_DCERR_N_RAMP; i++ )
  {
    if( paramState[i].isRamping )
    {
      // continue ramp
      ParamUpdate(paramState + i);
      flag |= 2;
    }

    // check for unloaded parameters
    flag |= (param[i] != paramState[i].req);
  }

  // ================ Compute Outputs
  // x0 and xf^2
  x0 = paramState[OMC_DCERR_X_0].val;
  xf2 = paramState[OMC_DCERR_X_F].val;
  xf2 *= xf2;

  // normalized Pas, less contrast defect
  pGain = paramState[OMC_DCERR_P_REF].val;
  if( datIn[1] > 1.0 )
    pGain /= datIn[1];
  pGain -= paramState[OMC_DCERR_P_DFCT].val;

  // error signal
  if( x0 == 0.0 )
    dx = 0.0;
  else
    //    dx = 0.5 * (pNorm * xf2 / x0 - x0); // Modified 
    dx = pGain * datIn[0] - 0.5 * x0;

  // non-linear correction
  dx += paramState[OMC_DCERR_C2].val * dx * dx;

  // ================ Assign Outputs
  // DC error signal
  if( nOut > 0 )
    datOut[0] = dx;

  // state flag
  if( nOut > 1 )
    datOut[1] = flag;

  // pGain 
  if( nOut > 2) 
    datOut[2] = pGain;

  // offset 
  if( nOut > 3)
    datOut[3] = 0.5 * x0;
 
  // test-point 1
  if( nOut > 4 )
  {
    i = (int)(param[OMC_DCERR_TP1] + 0.5);
    switch( i )
    {
      // ramping parameters
      case OMC_DCERR_P_REF:
      case OMC_DCERR_P_DFCT:
      case OMC_DCERR_X_0:
      case OMC_DCERR_X_F:
      case OMC_DCERR_C2:
      datOut[2] = paramState[i].val;
      break;

      // input signals
      case 8:
      datOut[2] = datIn[0];
      break;
      case 9:
      datOut[2] = datIn[1];
      break;

      // normalized Pas
      case 10:
	datOut[2] = pGain *datIn[0];
      break;

      // x^2 = (x0 + dx)^2
      case 11:
      datOut[2] = pGain * datIn[0] * xf2;
      break;

      // x^2 - x0^2 = dx * (2 * x0 + dx)
      // when locked on RF with an offset
      // this should be zero (since dx ~ 0)
      case 12:
      datOut[2] = pGain * datIn[0] * xf2 - x0 * x0;
      break;
      
      // set output to invalid request value
      default:
      datOut[2] = i;
    }
  }
}

#undef abs
