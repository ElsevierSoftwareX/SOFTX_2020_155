/* 
* OMC_CD_SQUAREROOT - compute the square root
*
* This code simply calculates a square root
*
* datIn[0]  = Input signal
* 
* datOut[0] = output signal = sqrt ( Input signal )
*
*/

/*
* FE Function
*/

void OMC_CD_SQUAREROOT(double* datIn, int nIn, double* datOut, int nOut)
{
  // ================ Assign Outputs
  if( nOut > 0 && nIn > 0 && datIn[0] > 0)
    datOut[0] = lsqrt(datIn[0]);
}
