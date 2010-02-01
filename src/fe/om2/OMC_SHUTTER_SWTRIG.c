/* 
* OMC_SHUTTER_SWTRIG
*
* This block generates the software shutter time series.  The idea is to
* produce a bang-bang signal which steps TT1 as fast as possible, and
* then stops it.  This is done by accelerating for a fixed time (the first
* bang) and then decellerating for the same time (the second bang).
*
* Complications:
* The TT coils have dewhitening filters.  These filters have a response
* which is greater than 1 above 2kHz, so a fast step will cause them
* to saturate.  To prevent this, the bang-bang signal should be filtered
* with a 2kHz 2nd order low-pass (in the RESP filter bank).
*
* datIn[0]  = SW trigger, 0 or 1
* datIn[1]  = HW trigger, 0 or 1
* datIn[2]  = HW SW force ratio, (HW force / SW force)
* datIn[3]  = first BANG duration "up", milliseconds
* datIn[4]  = second BANG duration "down", milliseconds
* datIn[5]  = hold value (until SW trigger is reset)
* datIn[6]  = milliseconds per clock cycle
* 
* datOut[0] = time series of bang-bang signal
*
*/

/*
* FE Function
*/

void OMC_SHUTTER_SWTRIG(double* datIn, int nIn, double* datOut, int nOut)
{
  // keep the SW trigger from doing bad things if it goes nuts
  const double MAX_TIME = 200.0;

  // this is the "bang" output value
  const int bangCounts = 32000;

  // timers for the hardware and software shutters
  static double swTime = 0.0;
  static double hwTime = 0.0;

  // input values are coppied in here
  double forceRatio;
  double bangUpTime;
  double bangDownTime;
  double holdValue;
  double msPerCycle;

  // check for valid number of inputs and outputs...
  if( nOut > 0 && nIn > 5 )
  {
    forceRatio = datIn[2];
    bangUpTime = datIn[3];
    bangDownTime = datIn[4] + bangUpTime;
    holdValue = datIn[5];
    msPerCycle = datIn[6];

    // make sure force ratio is greater than 1
    if( forceRatio < 1.0 )
      forceRatio = 1.0;

    // if SW trigger is off, just rest
    if( datIn[0] <= 0.0 )
    {
      // ========= SW trigger is off
      swTime = 0.0;
      hwTime = 0.0;
      datOut[0] = 0.0;
    }
    else
    {
      // HW trigger timer
      if( datIn[1] > 0.0 )
        hwTime += msPerCycle;

      // SW trigger timer
      swTime += msPerCycle;

      // SW shutter output (bang-bang-hold)
      if( swTime > MAX_TIME )
        datOut[0] = holdValue;   // limit bang time to MAX_TIME
      else if( swTime < bangUpTime )
        datOut[0] = bangCounts;  // first bang
      else if( swTime < bangDownTime + hwTime * forceRatio )
        datOut[0] = -bangCounts; // second bang
      else
        datOut[0] = holdValue;   // go to parking place
    }
  }
}
