inline void lockGpsTime()
{
  SYMCOM_REGISTER *timeRead;
          timeRead = (SYMCOM_REGISTER *)cdsPciModules.gps;
	  timeRead->TIMEREQ = 1;  // Trigger module to capture time
}

//***********************************************************************
// Function to read time from Symmetricom IRIG-B Module ***********************
//***********************************************************************
inline int  getGpsTime(unsigned int *tsyncSec, unsigned int *tsyncUsec)
{
  SYMCOM_REGISTER *timeRead;
    unsigned int timeSec,timeNsec,sync;

      if (cdsPciModules.gps) {
		timeRead = (SYMCOM_REGISTER *)cdsPciModules.gps;
	        timeSec = timeRead->TIME1;
	        timeNsec = timeRead->TIME0;
	        *tsyncSec = timeSec - 315964800;
	        *tsyncUsec = (timeNsec & 0xfffff);
	        // Read seconds, microseconds, nanoseconds
		sync = !(timeNsec & (1<<24));
		return sync;
	        }
	        return (0);
}

//***********************************************************************
// Get current GPS time from TSYNC IRIG-B Rcvr
//***********************************************************************
inline int  getGpsTimeTsync(unsigned int *tsyncSec, unsigned int *tsyncUsec) {
  TSYNC_REGISTER *timeRead;
    unsigned int timeSec,timeNsec,sync;

      if (cdsPciModules.gps) {
              timeRead = (TSYNC_REGISTER *)cdsPciModules.gps;
	      timeSec = timeRead->BCD_SEC;
	      timeSec += cdsPciModules.gpsOffset;
	      *tsyncSec = timeSec;
	      timeNsec = timeRead->SUB_SEC;
	      *tsyncUsec = ((timeNsec & 0xfffffff) * 5) / 1000;
	      sync = ((timeNsec >> 31) & 0x1) + 1;
	      return(sync);
      }
      return(0);
}

//***********************************************************************
//// Get current GPS seconds from TSYNC IRIG-B Rcvr
//***********************************************************************
inline unsigned int  getGpsSecTsync() {
TSYNC_REGISTER *timeRead;
    unsigned int timeSec,timeNsec,sync;

        if (cdsPciModules.gps) {
            timeRead = (TSYNC_REGISTER *)cdsPciModules.gps;
            timeSec = timeRead->BCD_SEC;
            timeSec += cdsPciModules.gpsOffset;
            return(timeSec);
        }
        return(0);
}

//***********************************************************************
// Get current GPS useconds from TSYNC IRIG-B Rcvr
//***********************************************************************
inline int  getGpsuSecTsync(unsigned int *tsyncUsec) {
    TSYNC_REGISTER *timeRead;
    unsigned int timeNsec,sync;

    	if (cdsPciModules.gps) {
             timeRead = (TSYNC_REGISTER *)cdsPciModules.gps;
             timeNsec = timeRead->SUB_SEC;
             *tsyncUsec = ((timeNsec & 0xfffffff) * 5) / 1000;
             sync = ((timeNsec >> 31) & 0x1) + 1;
             return(sync);
        }
        return(0);
}

//***********************************************************************
// Get current kernel time (in GPS)
//***********************************************************************
inline unsigned long current_time() {
    struct timespec t;
    extern struct timespec current_kernel_time(void);
    	t = current_kernel_time();
        t.tv_sec += - 315964819 + 33 + 3;
        return t.tv_sec;
}

#ifdef TIME_SLAVE
//***********************************************************************
// Test Mode - allows computer w/o IOC to run on timer from MASTER on
// 		Dolphin RFM network.
//***********************************************************************
inline void waitDolphinTime()
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
// Calculate ADC/DAC duotone offset for diagnostics
// Code should only run on IOP
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

