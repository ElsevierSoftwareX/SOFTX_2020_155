/* 
* OMC_UGF_DB2MAG - convert dB into magnitude, incl limits
*
* This code converts the magnitude into dB
*
* datIn[0]  = Input signal = dB
* datIn[1]  = lower limit switch: =0: OFF, !=0: ON
* datIn[2]  = upper limit switch: =0: OFF, !=0: ON
* datIn[3]  = lower limit (dB)
* datIn[4]  = upper limit (dB)
* 
* datOut[0] = output signal = in magnitude
*
*/

/*
* FE Function
*/

#ifndef __FABS__
#define __FABS__
static inline double fabs(double x) {
  return (x>0 ? x : -x);
}
#endif

double calc2powN(double N)
{
  double res;
  int    iN;

  if( N>=0.0 ) {
    if( N>30 ) {
      res = 1073741824.0 * calc2powN(N-30);
    } else {
      iN=N;
      iN=1<<iN;
      res=iN;
    }
  } else {
    res=1.0/calc2powN(-N);
  }
  return res;
}

double calc2powX(double x)
{
  double res;
  res = lrndint(x);                     //find nearest integer
  res = l2xr(x-res)*calc2powN(res);     // calculate 2^dB  
  return res;
}

double calc10powX(double x)
{
  double res;
  double y;
  y   = lmullog210(x);
  res = lrndint(y);                     //find nearest integer
  res = l2xr(y-res)*calc2powN(res);     // calculate 2^dB  
  return res;
}


void OMC_UGF_DB2MAG(double* datIn, int nIn, double* datOut, int nOut)
{
  double dB;
  double dBn;
  double mag;
  double limLowSW;
  double limHighSW;
  double limLow;
  double limHigh;
  

  if( nIn > 4) {
    limLowSW  = datIn[1];
    limHighSW = datIn[2];
    limLow    = datIn[3];
    limHigh   = datIn[4];    
  } else if( nIn > 3) {
    limLowSW  = datIn[1];
    limHighSW = datIn[2];
    limHigh =  fabs(datIn[3]);  // only one limit specified
    limLow  = -fabs(datIn[3]);  // only one limit specified
  } else {
    limLowSW  = 0.0;  // limits not used
    limHighSW = 0.0;  // limits not used
    limLow    = -320; //
    limHigh   =  320;
  }

  if( nIn > 0) {
    dB = datIn[0];
    if( limLowSW != 0 && dB < limLow ) {
      dB = limLow;
    }
    if( limHighSW != 0 && dB > limHigh ) {
      dB = limHigh;
    }
    mag = calc10powX(dB/20);      // calculate 10^(dB/20)
  } else {
    mag= 1.0;
  }
  
  // ================ Assign Outputs
  if( nOut > 0 )
    datOut[0] = mag;
}
