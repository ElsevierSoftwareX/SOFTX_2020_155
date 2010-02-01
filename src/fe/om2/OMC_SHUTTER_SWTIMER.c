/* 
* OMC_SHUTTER_TIMER
*
* This block counts cycles that input 0 is high (greater than 0).
* The counter is active when input 1 is high, and resets when it is low.
* The increment per cycle can be set with input 2.  The default increment
* per cycle is 1 (e.g., cycle counter).
*
* For example, to make a timer that outputs milliseconds since a
* trigger time, connect the trigger to inputs 0 and 1 and connect
* a constant millisec/cycle to input 2 (e.g., 0.030518 for 32kHz).
*
* datIn[0]  = timer start/stop
* datIn[1]  = timer active
* datIn[2]  = time per cycle (optional)
* 
* datOut[0] = timer output
*
*/

/*
* FE Function
*/

void OMC_SHUTTER_SWTIMER(double* datIn, int nIn, double* datOut, int nOut)
{
  // check for valid number of inputs and outputs...
  if( nOut > 0 && nIn > 1 )
  {
    // start/stop timer
    if( datIn[0] > 0.0 )
    {
      if( nIn > 2 )
        datOut[0] += datIn[2];
      else
        datOut[0]++;
    }

    // reset when not active
    if( datIn[1] <= 0.0 )
      datOut[0] = 0.0;
  }
}
