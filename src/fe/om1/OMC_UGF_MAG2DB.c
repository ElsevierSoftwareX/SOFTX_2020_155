/* 
* OMC_UGF_MAG2DB - convert magnitude into dB, incl limits
*
* This code converts the magnitude into dB
*
* datIn[0]  = Input signal = magnitude
* datIn[1]  = lower limit switch: =0: OFF, !=0: ON
* datIn[2]  = upper limit switch: =0: OFF, !=0: ON
* datIn[3]  = lower limit (dB)
* datIn[4]  = upper limit (dB)
* 
* datOut[0] = output signal = in dB
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

void OMC_UGF_MAG2DB(double* datIn, int nIn, double* datOut, int nOut)
{
  double mag;
  double dB;
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
    mag = fabs(datIn[0]);
    if( mag > 0.0 ) {
      dB=20*llog10(mag);
      if( limLowSW != 0 && dB < limLow ) {
        dB = limLow;
      }
      if( limHighSW != 0 && dB > limHigh ) {
        dB = limHigh;
      }
    } else {
      dB = limLow;
    }
  } else {
    dB= 0.0;
  }
  
  // ================ Assign Outputs
  if( nOut > 0 )
    datOut[0] = dB;
}
