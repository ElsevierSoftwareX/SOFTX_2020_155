///	\file timing.c
///	\brief File contains some timing diagnostics previously imbedded
///<		into the controller.c code.
//***********************************************************************
/// \brief Get current kernel time (in GPS)
///	@return Current time in form of GPS Seconds.
//***********************************************************************
inline unsigned long current_time(void) {
    struct timespec t;
    extern struct timespec current_kernel_time(void);
    	t = current_kernel_time();
	// Added leap second for July 1, 2015
        t.tv_sec += - 315964819 + 33 + 3 + 1;
        return t.tv_sec;
}

#ifdef TIME_SLAVE
//***********************************************************************
/// \brief Test Mode - allows computer w/o IOC to run on timer from MASTER on
///< 		Dolphin RFM network.
//***********************************************************************
inline void waitDolphinTime(void)
{
unsigned long d = cdsPciModules.dolphin[0][1];

     if (boot_cpu_has(X86_FEATURE_MWAIT)) {
	     for (;;) {
             	if (cdsPciModules.dolphin[0][1] != d) break;
	        __monitor((void *)&cdsPciModules.dolphin[0][1], 0, 0);
	        if (cdsPciModules.dolphin[0][1] != d) break;
	        __mwait(0, 0);
		}
	} else {
	     do {
	        udelay(1);
	     } while(cdsPciModules.dolphin[0][1] != d);
	}

}
#endif

//***********************************************************************
/// \brief Calculate ADC/DAC duotone offset for diagnostics. \n
///< Code should only run on IOP
//***********************************************************************
#ifdef ADC_MASTER
inline float duotime(int count, float meanVal, float data[])
{
  float x,y,sumX,sumY,sumXX,sumXY,msumX;
  int ii;
  float xInc;
  float offset,slope,answer;
  float den;

  x = 0;
  sumX = 0;
  sumY = 0;
  sumXX = 0;
  sumXY= 0;
  xInc = 1000000/CYCLE_PER_SECOND;


  for(ii=0;ii<count;ii++)
  {
          y = data[ii];
          sumX += x;
          sumY += y;
          sumXX += x * x;
          sumXY += x * y;
          x += xInc;
  }
  msumX = sumX * -1;
  den = (count*sumXX-sumX*sumX);
  if(den == 0.0)
  {
          return(-1000);
  }
  offset = (msumX*sumXY+sumXX*sumY)/den;
  slope = (msumX*sumY+count*sumXY)/den;
  if(slope == 0.0)
  {
          return(-1000);
  }
  meanVal -= offset;
  answer = meanVal/slope - 91.552;
  return(answer);

}
#endif

