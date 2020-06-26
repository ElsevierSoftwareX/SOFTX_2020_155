/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gpsclk							*/
/*                                                         		*/
/* Module Description: implements API for VME-SYNCCLOK32 GPS board	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

/* Header File List: */
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <semaphore.h>

#ifdef OS_VXWORKS
#include <vxWorks.h>
#include <taskLib.h>
#include <intLib.h>
#include <sysLib.h>
#include <logLib.h>
#include <timers.h>
#include <iv.h>
#include <fppLib.h>
#include <vxLib.h>
#if 0 && defined(PROCESSOR_BAJA47)
#include <heurikon.h> 
#endif
#endif

#include "dtt/gdsutil.h"
#include "dtt/hardware.h"
#include "dtt/gpsclkdef.h"
#include "dtt/gpsclk.h"
#include "dtt/gpstime.h"
#include "dtt/ntp.h"


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Constants: _GPSCLK_TEST	  define for enabling tests		*/
/* 	      GPS_HEARTBEAT_RATE  gps heartbeat rate: 64Hz  		*/
/* 	      GPS_HEARTBEAT_DIV	  gps counter divisor	  		*/
/* 	      GPS_HEARTBEAT_LEN	  gps heartbeat interval length (us)	*/
/* 	      TRUE_HEARTBEAT_RATE nominal heartbeat rate		*/
/* 	      GPS_TRUE_DIV	  divisor gps / nominal rate 		*/
/* 	      TRUE_HEARTBEAT_LEN  nominal heartbeat interval lengt (us)	*/
/* 	      JAM_INTERVAL	  jam interval in us (prior to 1pps)	*/
/*            _ONESEC		  one second (in nsec)			*/
/*            _DP_DELAY_PER_TICK  delay for dual port RAM (delays/tick)	*/
/*            _HEALTH_UNIT	  units of the heartbeath health value	*/
/*            _MAX_ATTEMPT	  maximum attempts to read GPS info	*/
/*            syncDelay		  jam interval in ns			*/
/*            								*/
/*----------------------------------------------------------------------*/
#define IGNORE_YEAR		1
#define _GPSCLK_TEST
#define GPS_HEARTBEAT_RATE	64
#if (SYNCCLOCK32_0_TYPE == 0)
#define GPS_HEARTBEAT_DIV	(3000000 / GPS_HEARTBEAT_RATE)
#else
#define GPS_HEARTBEAT_DIV	( 131072 / GPS_HEARTBEAT_RATE)
#endif
#define GPS_HEARTBEAT_LEN	(1000000 / GPS_HEARTBEAT_RATE)
#define TRUE_HEARTBEAT_RATE	16
#define GPS_TRUE_DIV		(GPS_HEARTBEAT_RATE / TRUE_HEARTBEAT_RATE)
#define TRUE_HEARTBEAT_LEN	(1000000 / TRUE_HEARTBEAT_RATE)
#define JAM_INTERVAL		10000
#define _DP_DELAY_PER_TICK	25
#define _HEALTH_UNIT		100
#define _MAX_ATTEMPT		10

   static const struct timespec	syncDelay = {1, 1000 * JAM_INTERVAL};
   static const struct timespec	smallDelay = {0, _ONESEC / 100};


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Globals: gpsISR0		callback routine for ISR		*/
/*          hbcount		heartbeat count			 	*/
/*          currentYear		current year information		*/
/*          DPaccess		keeps track of DP RAM accesses		*/
/*          hbHealth		interrupt delay averaged over last 1s	*/
/*                              (in us/_HEALTH_UNIT)			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static gpsISR_func 	gpsISR0 = NULL;
   static volatile int 	hbcount = -2;
   static int		currentYear = 0;
   static volatile int	DPaccess = 0;
   static volatile int	hbHealth = 0;
   static int 		initDelay = 0;
   static char*		gps[] = {NULL};
   int 			gpsdebug = 0;


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Forward declarations: 						*/
/*	Set_Dual_Port_RAM	set value in dual port RAM		*/
/*	Read_Dual_Port_RAM	read value from dual port RAM		*/
/*	setYearInformation	set year information in dual port RAM	*/
/*	setHeartbeatRate	set heartbeat rate in dual port RAM	*/
/*	bcd2bin			bcd to binary conversion		*/
/*	bin2bcd			binary to bcd conversion		*/
/*	beatISR0		ISR for gps heartbeat (gps board)	*/
/*	beatISR1		ISR for gps heartbeat (other Baja)	*/
/*      gpsYDS			converts year/day/sec into GPS seconds	*/
/*      								*/
/*----------------------------------------------------------------------*/
   static int Set_Dual_Port_RAM (short ID, byte DPaddr, byte value);
   static int Read_Dual_Port_RAM (short ID, char DPaddr);
   static int setYearInformation (short ID, int year);
#if defined (OS_VXWORKS) && (SYNCCLOCK32_0_INT_SOURCE==1)
   static void setHeartbeatRate (short ID, unsigned short div);
#endif
   static unsigned long bcd2bin (unsigned long bcd);
   static unsigned long bin2bcd (unsigned long bin);
   static int64_t YDHMS2gps (int year, int day, int hour, 
			     int min, int sec);


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: gpsBaseAddress				*/
/*                                                         		*/
/* Procedure Description: get SYNCCLOCK VME base address		*/
/*                                                         		*/
/* Procedure Arguments: board ID					*/
/*                                                         		*/
/* Procedure Returns: base address or NULL if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   char* gpsBaseAddress (short ID)
   {
      switch (ID) 
      {
         case 0:
            {
               return gps[0];
            }
         default:
            {
               return NULL;
            }
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: gpsMaster					*/
/*                                                         		*/
/* Procedure Description: True if GPS master, false if secondary board	*/
/*                                                         		*/
/* Procedure Arguments: board ID					*/
/*                                                         		*/
/* Procedure Returns: true if master					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int gpsMaster (short ID)
   {
      switch (ID) {
         case 0:
            {
               return (SYNCCLOCK32_0_MASTER != 0);
            }
         default:
            {
               return 0;
            }
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: gpsTimeNow					*/
/*                                                         		*/
/* Procedure Description: returns the current gps time			*/
/*                                                         		*/
/* Procedure Arguments: board ID					*/
/*                                                         		*/
/* Procedure Returns: gps time in nsec					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int64_t gpsTimeNow (short ID)
   {
      unsigned int		timelo;		/* low time word */
      unsigned int		timehi;		/* high time word */
   
      gpsNativeTime (ID, &timehi, &timelo);
      return gpsTime (ID, timehi, timelo);
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: ntpRtcSet 					*/
/*                                                         		*/
/* Procedure Description: sets the real time clock from an NTP server	*/
/*                                                         		*/
/* Procedure Arguments: server name					*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#ifdef OS_VXWORKS
   int ntpRtcSet (char* server)
   {   
      int 	i;
      int 	year;

      /* copy ntp server name */
      if (server == NULL) {
         server = SYNCCLOCK32_0_NTPSERVER;
      }
      if (server == NULL) {
         return -1;
      }

      /* call the NTP srever */
      year = -1;
      if (sntp_sync_year (server, &year) !=0) {
  	return -1;
      }

   #if 0 && defined(OS_VXWORKS) && defined(PROCESSOR_BAJA47)
      {
         struct timespec	t1;
         sysRtcShow();
         clock_gettime (CLOCK_REALTIME, &t1);
         sysRtcSet (gmtime (&t1.tv_sec));
         sysRtcShow();
      }
   #endif
      /* set the year if successful */
      if (year >= 0) {
         printf ("Current year from NTP server: %i\n", year);
         currentYear = year;
      }

      return (year >= 0) ? 0 : -1;
   }
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: gpsTime					*/
/*                                                         		*/
/* Procedure Description: returns the gps time in nsec			*/
/*                                                         		*/
/* Procedure Arguments: board ID, timehi and timelo			*/
/*                                                         		*/
/* Procedure Returns: gps time						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int64_t gpsTime (short ID, unsigned int timehi, 
                     unsigned int timelo)
   {
      int64_t		t;		/* return time */
      int		year;		/* year */
      int		day;		/* days */
      int		hour;		/* hour */
      int		min;		/* minutes */
      int		sec;		/* seconds */
   
      if (!IGNORE_YEAR && gpsMaster (ID)) {
         if (currentYear == 0) {
            volatile char* volatile addr;	/* base address */
            short		statusEx;	/* extended status */
         
         /* get base address */
            addr = gpsBaseAddress (ID);
            if (addr == NULL) {
               return 0;
            }
         /* try to read year from board */
            year = 0;
            gpsSyncInfo (ID, &statusEx);
            if ((statusEx & DP_Extd_Sts_NoYear) == 0) {
               year = Read_Dual_Port_RAM (ID, DP_Year10Year1) & 0xff;
               year += 
                  (Read_Dual_Port_RAM (ID, DP_Year1000Year100) & 0xff) << 8;
            }
         /* quick-and-dirty Y2K fix */
            year = ((year & 0x00FF) | 0x2000);
            printf ("GPS master: year %x\n", year); 
            if ((year > 0x1990) && (year < 0x3000)) {
               currentYear = bcd2bin (year);
            }
            else {
               return 0;
            }
         }
      }
      else { /* gps secondary board or ignore year */
         if (currentYear == 0) {
            struct tm	utc;		/* utc time from clock */
         #if 0 && defined(OS_VXWORKS) && defined(PROCESSOR_BAJA47)
         /* initialize year from real-time clock:
            :TODO: this method may fail during a couple of seconds
            around year's end. It also requires the real-time clock to 
            be initialized to utc during startup! */
         if (sysRtcGet (&utc) != OK) {
            return 0;
         }
         #else 
            struct timespec 	now;
            if (clock_gettime (CLOCK_REALTIME, &now) != 0) {
               return 0;
            }
         /* gmtime_r returns int on VxWorks and 
            a ptr to the 2nd arg on UNIX */
         #ifdef OS_VXWORKS
         if (gmtime_r (&now.tv_sec, &utc) != 0) {
            return 0;
         }
         #else
            if (gmtime_r (&now.tv_sec, &utc) == NULL) {
               return 0;
            }
         #endif
         #endif
            if (utc.tm_year < 96) {
               utc.tm_year += 100;
            }
            currentYear = 1900 + utc.tm_year;
            /*currentYear = 2004;*/
         /* write year info to gps board if secondary */
            if (!gpsMaster (ID) &&
               setYearInformation (0, currentYear) != 0) {
               currentYear = 0;
               return 0;
            }
            timehi = (timehi & 0x0FFFFFFF) + ((currentYear % 10) << 28);
         }
      }
   
      /* extract day of year and seconds */
      day = bcd2bin ((timehi & 0x0FFF0000) >> 16);
      hour = bcd2bin ((timehi & 0x0000FF00) >> 8);
      min = bcd2bin (timehi & 0x000000FF);
      sec = bcd2bin ((timelo & 0xFF000000) >> 24);
      /* extract year infromation */
      year = timehi >> 28;
      /* check whether year was initialized */
      if ((year > 9) || (currentYear == 0)) {
         return 0;
      }
      /* check whether year has changed 
   	 ignore the first 100 calls because of a strange 
         initialization problem */
      if (!IGNORE_YEAR && (year != (currentYear % 10)) && (++initDelay > 1000)) {
         printf ("current year is %i; new readout says %i\n", 
                currentYear, year);
         year += 10 * (currentYear / 10);
         if (year < currentYear - 1) {
            year += 10;
         }
         if (year > currentYear + 1) {
            year -= 10;
         }
         {
            char	buf[256];
            sprintf (buf, "new year = %i", currentYear);
            gdsConsoleMessage (buf);
         }
      
      }
      else {
         year = currentYear;
      }
      /* check whether days of year make sense */
      if (day <= 0) {
         return 0;
      }
      t = YDHMS2gps (year, day, hour, min, sec) * _ONESEC + 
	(int64_t)bcd2bin (timelo & 0x00FFFFFF) * 1000LL;

#if TARGET == (TARGET_L1_GDS_AWG1 + 20) ||  TARGET == (TARGET_L1_GDS_AWG1 + 21)
#warning Building in fixed 12 second leapsecond GPS time correction
      t += 13 * _ONESEC; /* Add leapseconds; 40M has old GPS card */
#endif

      return t;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: gpsNativeTime				*/
/*                                                         		*/
/* Procedure Description: returns the gps time in native format		*/
/*                                                         		*/
/* Procedure Arguments: board ID, pointer to timehi and timelo		*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   void gpsNativeTime (short ID, unsigned int* timehi, 
                     unsigned int* timelo)
   {
      volatile char* volatile	addr;	/* base address */
   
      /* get base address */
      addr = gpsBaseAddress (ID);
      if (addr == NULL) {
         *timehi = 0;
         if (timelo != NULL) {
            *timelo = 0;
         }
         return;
      }
   
      /* get time */
      *timelo = ((GPSREGS*) addr)->Sec40_Usec1;
      if (timehi != NULL) {
         *timehi = ((GPSREGS*) addr)->Year8_Min1;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: gpsMicroSec					*/
/*                                                         		*/
/* Procedure Description: returns the micro seconds of the gps time	*/
/*                                                         		*/
/* Procedure Arguments: board ID					*/
/*                                                         		*/
/* Procedure Returns: usec						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   unsigned int gpsMicroSec (short ID) 
   {
      unsigned int 	timelo;
   
      gpsNativeTime (ID, NULL, &timelo);
      return bcd2bin (timelo & 0x00FFFFFF);
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: gpsInit					*/
/*                                                         		*/
/* Procedure Description: initializes the GPS board and sets the year	*/
/*                                                         		*/
/* Procedure Arguments: board ID, pointer to year information		*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int gpsInit (short ID, int* year)
   {
   #ifndef OS_VXWORKS
      return -10;
   #else
      volatile char* volatile	addr;	/* base address */
      char			test;	/* test byte */
      int			status; /* return value */
   
      /* test ID */
      if (ID != 0) {
         return -1;
      }
   
      /* Obtain board's base address. */
      gps[0] = NULL;
      status = sysBusToLocalAdrs(SYNCCLOCK32_0_ADRMOD,
                                (char*)(SYNCCLOCK32_0_BASE_ADDRESS & 0xFFFFFF),
                                (char**)&addr);
      printf ("gpsInit: sysBusToLocalAdrs = 0x%x\n", (int) addr);
      if(status != OK) {
         printf("gpsInit: Error, sysBusToLocalAdrs\n");
         return -1;
      }
      gps[0] = (char*) addr;
   
      /* get base address */
      addr = gpsBaseAddress (ID);
      if (addr == NULL) {
         return -1;
      }
   
      /* check if board is there */
      if (vxMemProbe ((char*) addr+21, VX_READ, 1, &test) == ERROR) {
         char		buf[100];
         sprintf (buf, "GPS%i not accessable in VME", ID);
         printf ("ERROR: %s\n", buf);
         gdsError (GDS_ERR_MEM, buf);
         gps[ID] = NULL;
         return -1;
      }
      else {
         printf ("Brandywine syncclock32 (GPS %s) installed at %x\n",
                gpsMaster(ID) ? "master" : "secondary", (int) addr);
      }
   
      /* set real-time clock to UTC */
      if (ntpRtcSet (NULL) < 0) return -1;
      printf ("Real-time clock set to new value\n");
      taskDelay (60);
      /* wait until synchronized to input */
   
      /* set the year information if a secondary */
      if (!gpsMaster (ID)) {
      /* set year */
         if (year != NULL) {
            currentYear = *year;
         }
         if (currentYear != 0) {
            setYearInformation (ID, currentYear);
         }
         else {
            /* try to read year from rtc */
            (void) gpsTimeNow (ID);
         }
      }
      else {
         /* try to read year from gps module */
         (void) gpsTimeNow (ID);
      }
   
   /*   printf ("GPS init done\n");
      taskDelay (60);*/
      return 0;
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: gpsSyncInfo					*/
/*                                                         		*/
/* Procedure Description: returns synchronization information		*/
/*                                                         		*/
/* Procedure Arguments: board ID, pointer to extended sync status	*/
/*                                                         		*/
/* Procedure Returns: 1 if synchronized, 0 otherwise, -1 on error	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int gpsSyncInfo (short ID, short* statusEx)
   {
      volatile char* volatile	addr;	/* base address */
   
      /* get base address */
      addr = gpsBaseAddress (ID);
      if (addr == NULL) {
         return -1;
      }
   
      /* get extended information */
      if (statusEx != NULL) {
         *statusEx = Read_Dual_Port_RAM (ID, DP_Extd_Sts) & 0x1F;
      }
   
      return ((((GPSREGS*) addr)->Status & 0x02) >> 1);
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: gpsInfo					*/
/*                                                         		*/
/* Procedure Description: returns GPS information			*/
/*                                                         		*/
/* Procedure Arguments: board ID, pointer GPS information		*/
/*                                                         		*/
/* Procedure Returns: # satellites tracked if successful, <0 otherwise	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int gpsInfo (short ID, gpsInfo_t* info)
   {
   #ifndef OS_VXWORKS
      return -10;
   #else 
      if (!gpsMaster (ID)) {
         return -9;
      }
      else {
         char*		addr;	/* base address */
         short		stat;	/* GPS status */
         int		sem;	/* GPS semaphore */
         int		i;	/* index */
         long		l; 	/* temporary result */
      
      /* get base address */
         addr = gpsBaseAddress (ID);
         if (addr == NULL) {
            return -1;
         }
      
         if (info == NULL) {
         /* read GPS status */
            stat = Read_Dual_Port_RAM (ID, DP_GPS_Status);
         }
         else {
         /* read time first */
            info->time = (unsigned long) (gpsTimeNow (ID) / _ONESEC);
         /* read extended synchronization status */
            (void) gpsSyncInfo (ID, &info->syncStatus);
         /* try to get GPS semaphore */
            for (i = 0; i < _MAX_ATTEMPT; i++) {
               sem = Set_Dual_Port_RAM (ID, DP_Command, GPS_Readlock);
               if (sem > 0) {
                  break;
               }
               taskDelay (1);
            }
            if (sem <= 0) {
            /* failed to obtain semaphore */
               return -3;
            }
         
          /* read GPS status */
            stat = Read_Dual_Port_RAM (ID, DP_GPS_Status);
            info->numSatellite = stat & DP_GPS_Status_Sats;
            info->status = ((stat & DP_GPS_Status_Nav) != 0) ? 1 : 0;
         
         /* read latitude */
            l = Read_Dual_Port_RAM (ID, DP_GPS_Latbin0700) & 0x00ff;
            l += (Read_Dual_Port_RAM (ID, DP_GPS_Latbin1508) & 0x00ff) << 8;
            l += (Read_Dual_Port_RAM (ID, DP_GPS_Latbin2316) & 0x00ff) << 16;
            l += (Read_Dual_Port_RAM (ID, DP_GPS_Latbin3124) & 0x00ff) << 24;
            info->latitude = (double) l / 3600.0E3;
         
         /* read longitude */
            l = Read_Dual_Port_RAM (ID, DP_GPS_Lonbin0700) & 0x00ff;
            l += (Read_Dual_Port_RAM (ID, DP_GPS_Lonbin1508) & 0x00ff) << 8;
            l += (Read_Dual_Port_RAM (ID, DP_GPS_Lonbin2316) & 0x00ff) << 16;
            l += (Read_Dual_Port_RAM (ID, DP_GPS_Lonbin3124) & 0x00ff) << 24;
            info->longitude = (double) l / 3600.0E3;
         
         /* read altitude */
            l = Read_Dual_Port_RAM (ID, DP_GPS_Altbin0700) & 0x00ff;
            l += (Read_Dual_Port_RAM (ID, DP_GPS_Altbin1508) & 0x00ff) << 8;
            l += (Read_Dual_Port_RAM (ID, DP_GPS_Altbin2316) & 0x00ff) << 16;
            l += (Read_Dual_Port_RAM (ID, DP_GPS_Altbin3124) & 0x00ff) << 24;
            info->altitude =  (double) l / 100.0;
         
         /* read speed */
            l = Read_Dual_Port_RAM (ID, DP_GPS_SOG0M1) & 0x00ff;
            l += (Read_Dual_Port_RAM (ID, DP_GPS_SOG21) & 0x00ff) << 8;
            info->speed = (double) bcd2bin (l) / 10.0;
         
         /* read direction */
            l = Read_Dual_Port_RAM (ID, DP_GPS_HOG0M1) & 0x00ff;
            l += (Read_Dual_Port_RAM (ID, DP_GPS_HOG21) & 0x00ff) << 8;
            info->direction = (double) bcd2bin (l) / 10.0;
         
          /* unlock GPS semaphore */
            Set_Dual_Port_RAM (ID, DP_Command, GPS_Readunlock);
         }
      
      /* return number of satellites */
         return (stat & DP_GPS_Status_Sats);
      }
   #endif
   }

#if defined (OS_VXWORKS) && (SYNCCLOCK32_0_INT_SOURCE==1)
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Interrupt Service Routine: beatISR0					*/
/*                                                         		*/
/* Procedure Description: called by heartbeat interrupt			*/
/*                                                         		*/
/* Procedure Arguments: board ID					*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void beatISR0 (int ID)
   {
      unsigned int 	timehi;		/* gps time high word */
      unsigned int 	timelo;		/* gps time low word */
      int		usec;		/* gps micro seconds */
      int		epoch;		/* epoch of interrupt */
      int		rem;		/* time difference */
      volatile int 	i;		/* index */
      volatile char* volatile addr;	/* GPS base address */
      int		year;		/* year */
      int		day;		/* days */
      int		hour;		/* hour */
      int		min;		/* minutes */
      int		sec;		/* seconds */
      int64_t           isrtime;	/* gps time of ISR */
      static int64_t	oldisrtime = 0;	/* gps time of prev. ISR */
   
   #ifdef OS_VXWORKS
      int 		lock;		/* interrupt lock */
      lock = intLock();
   #endif
      /* :DO NOT CHANGE: reset heartbeat flag 
   	  there seems to be a timing problem! */
      addr = gpsBaseAddress (0);
      if (addr == NULL) {
         return;
      }
      for (i = 0; i < 100; i++) {
         ((GPSREGS*) addr)->Resets = Reset_Heartbeat;
      }
   
      switch (hbcount)
      {
         case -2:
            {
               /* ignore */
               break;
            }
         case -1:
            {
               if (gpsMicroSec (0) % 62500 >= 15625) {
                  /* break if not synchronized */
                  break;
               }
               /* continue if synchronized */
               hbcount = 3;
            }
         default:
            {
               gpsNativeTime (0, &timehi, &timelo);
	       day = bcd2bin ((timehi & 0x0FFF0000) >> 16);
	       hour = bcd2bin ((timehi & 0x0000FF00) >> 8);
	       min = bcd2bin (timehi & 0x000000FF);
	       sec = bcd2bin ((timelo & 0xFF000000) >> 24);
               year = currentYear;
               isrtime = YDHMS2gps (year, day, hour, min, sec) * _ONESEC + 
                  (int64_t) bcd2bin (timelo & 0x00FFFFFF) * 1000LL;
	       /* ignore double interrupts */
	       if (isrtime - oldisrtime < 1000000LL) {
	          if (gpsdebug) 
  	             logMsg ("ISR at %i.%09i (ignored)\n", (int)(isrtime/1000000000LL),
	                     (int)(isrtime%1000000000LL), 0, 0, 0, 0);
	          break;
	       }
	       if (gpsdebug)
	          logMsg ("ISR at %i.%09i (0x%06x)\n", (int)(isrtime/1000000000LL),
	                  (int)(isrtime%1000000000LL), timelo&0x00FFFFFF, 0, 0, 0);
	       oldisrtime = isrtime;
               hbcount++;
               /* prevent overflow */
               if (hbcount >= GPS_HEARTBEAT_RATE) {
                  hbcount -= GPS_HEARTBEAT_RATE;
               }
               /* if in between: check synchronization */
               if ((hbcount % GPS_TRUE_DIV) != 0) {
                  usec = bcd2bin (timelo & 0x00FFFFFF);
               	  /* printf ("t = %d   ", timelo & 0x00FFFFFF);*/
                  epoch = (usec + GPS_HEARTBEAT_LEN / 10) / 
                          GPS_HEARTBEAT_LEN;
                  rem = usec - epoch * GPS_HEARTBEAT_LEN;
                  hbHealth = (((GPS_HEARTBEAT_RATE - 1) * hbHealth) + 
                             _HEALTH_UNIT * rem) / GPS_HEARTBEAT_RATE;
                  if (epoch % GPS_HEARTBEAT_RATE == hbcount) {
                     /* break if synchronized! */
                     break;
                  }
               	  /* else resycnchronize */
               #if defined(_GPSCLK_TEST) && defined(OS_VXWORKS) && defined(DEBUG)
                  logMsg ("resync (%d) at epoch = %d, heartbeat = %d\n",
                         usec, epoch, hbcount, 0, 0, 0);
               #endif
                  hbcount = epoch;
                  if ((hbcount % GPS_TRUE_DIV) != 0) {
                     /* break if in between */
                     break;
                  }
               }
               /* this is a 16Hz interrupt! */
               /* call GPS interrupt service routine */
               if (gpsISR0 != NULL) {
                  (void) gpsISR0 (timehi, timelo);
               }
               break;
            }
      }
   #ifdef OS_VXWORKS
      intUnlock(lock);
   #endif
   }
#endif

#if defined (OS_VXWORKS) && (SYNCCLOCK32_0_INT_SOURCE==0)
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Interrupt Service Routine: beatISR1					*/
/*                                                         		*/
/* Procedure Description: called by other baja CPU			*/
/*                                                         		*/
/* Procedure Arguments: board ID					*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void beatISR1 (int ID)
   {
      unsigned int 	timehi;		/* gps time high word */
      unsigned int 	timelo;		/* gps time low word */
   
   #ifdef OS_VXWORKS
      int 		lock;		/* interrupt lock */
      lock = intLock();
   #endif
   
      gpsNativeTime (0, &timehi, &timelo);
      hbcount++;
   
      /* prevent overflow */
      if (hbcount >= GPS_HEARTBEAT_RATE) {
         hbcount -= GPS_HEARTBEAT_RATE;
      }
      /* call GPS interrupt service routine */
      if (gpsISR0 != NULL) {
         (void) gpsISR0 (timehi, timelo);
      }
   
   #ifdef OS_VXWORKS
      intUnlock(lock);
   #endif
   }
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: gpsHeartbeatInstall				*/
/*                                                         		*/
/* Procedure Description: installs the heartbeat interrupt		*/
/*                                                         		*/
/* Procedure Arguments: board ID, callback ISR 				*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int gpsHeartbeatInstall (short ID, gpsISR_func ISR)
   {
   #ifndef OS_VXWORKS
      return -10;
   
      /* interrupt source is an other Baja */
   #elif (SYNCCLOCK32_0_INT_SOURCE==0)
      /*return 0;*/
      if (ISR == NULL) {
         /* remove interrupt handler */
         sysIntDisable (SYNCCLOCK32_0_INT_LEVEL & 0x07);
         gpsISR0 = NULL;
         return 0;
      }
   
      /* set ISR */
      gpsISR0 = ISR;
   
      /* connect ISR */
      if (intConnect ((VOIDFUNCPTR*) INUM_TO_IVEC(SYNCCLOCK32_0_INT_VEC),
         (VOIDFUNCPTR) beatISR1, ID) != OK) {
         return -2;
      }	
      /* enable heartbeat interrupt */
      hbcount = 0;
      if (sysIntEnable (SYNCCLOCK32_0_INT_LEVEL & 0x07) != OK) {
         return -3;
      }
   
   #ifdef _GPSCLK_TEST
      printf ("GPS heart beat installed ");
      printf ("(int vec = %x, lev = %x)\n",
             SYNCCLOCK32_0_INT_VEC, SYNCCLOCK32_0_INT_LEVEL & 0x07);
   #endif
      return 0;
   
   #else
      /* interrupt source is GPS board */
      volatile char* volatile	addr;	/* base address */
   
      /* get base address */
      addr = gpsBaseAddress (ID);
      if ((addr == NULL) || (ID != 0)) {
         return -1;
      }
   
      if (ISR == NULL) {
         /* remove interrupt handler */
         sysIntDisable (SYNCCLOCK32_0_INT_LEVEL & 0x07);
         ((GPSREGS*) addr)->Heartbeat_Intr_Ctl = SYNCCLOCK32_0_INT_LEVEL & 0x07;
         ((GPSREGS*) addr)->Resets = Reset_Heartbeat;
         gpsISR0 = NULL;
         return 0;
      }
   
      /* set ISR */
      gpsISR0 = ISR;
   
      /* connect ISR */
      if (intConnect ((VOIDFUNCPTR*) INUM_TO_IVEC(SYNCCLOCK32_0_INT_VEC),
         (VOIDFUNCPTR) beatISR0, ID) != OK) {
         return -2;
      }	
      /* enable heartbeat interrupt */
      hbcount = -2;
      if (sysIntEnable (SYNCCLOCK32_0_INT_LEVEL & 0x07) != OK) {
         return -3;
      }
   
      /* set interrupt vector and level */
      ((GPSREGS*) addr)->Heartbeat_Intr_Vec = SYNCCLOCK32_0_INT_VEC;
      ((GPSREGS*) addr)->Heartbeat_Intr_Ctl = 
         Int_Enb | (SYNCCLOCK32_0_INT_LEVEL & 0x07);
   
      /* set interrupt rate, wait unitl the last moment */
      /* this seems to be necessary, otherwise the jam doesn't 
         work right and the interrupts are not aligned with the 1PPS */
      while (gpsMicroSec (ID) < (1000000 - JAM_INTERVAL)) {;}
      setHeartbeatRate (ID, GPS_HEARTBEAT_DIV);
      nanosleep (&syncDelay, NULL);
      /* while (gpsMicroSec (ID) > (1000000 - JAM_INTERVAL)) {;} */
      hbcount = -1;
   
   #ifdef _GPSCLK_TEST
      printf ("GPS heart beat installed ");
      printf ("(int vec = %x, lev = %x / %x)\n",
             ((GPSREGS*) addr)->Heartbeat_Intr_Vec,
             ((GPSREGS*) addr)->Heartbeat_Intr_Ctl,
             Int_Enb | (SYNCCLOCK32_0_INT_LEVEL & 0x07));
   #endif
      return 0;
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: gpsHeartbeatHealth				*/
/*                                                         		*/
/* Procedure Description: monitors the heartbeat health			*/
/*                                                         		*/
/* Procedure Arguments: board ID	 				*/
/*                                                         		*/
/* Procedure Returns: averaged heartbeat delay in us			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int gpsHeartbeatHealth (short ID)
   {
      if (ID != 0) {
         return 0;
      }
      else {
         return (hbHealth + _HEALTH_UNIT/2) / _HEALTH_UNIT;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: Set_Dual_Port_RAM				*/
/*                                                         		*/
/* Procedure Description: writes to the dual ported RAM			*/
/*                                                         		*/
/* Procedure Arguments: board ID, address, value			*/
/*                                                        		*/
/* Procedure Returns: 0 if successful, <0 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int Set_Dual_Port_RAM (short ID, byte DPaddr, byte value)
   {
   #ifndef OS_VXWORKS
      return -10;
   #else
      volatile char* volatile	addr;	/* base address */
      byte 		response;
   
      /* get base address */
      addr = gpsBaseAddress (ID);
      if (addr == NULL) {
         return -1;
      }
      /* clear response flag and write DP address */
      response = ((GPSREGS*) addr)->DPData;
      ((GPSREGS*) addr)->DPAddr = DPaddr;
      /* check status bit */
      while ((((GPSREGS*) addr)->Status & Response_Ready) ==0) {}
      /* clear response flag and set DP value */
      response = ((GPSREGS*) addr)->DPData;
      ((GPSREGS*) addr)->DPData = value;
      /* check status bit */
      while ((((GPSREGS*) addr)->Status & Response_Ready) ==0) {}
      /* read data and return after wait of 400us(avrg.) */
      response = ((GPSREGS*) addr)->DPData;
      DPaccess++;
      if ((DPaccess % _DP_DELAY_PER_TICK) == 0) {
         taskDelay (1);
         DPaccess = 0;
      }
      return response;
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: Read_Dual_Port_RAM				*/
/*                                                         		*/
/* Procedure Description: reads from the dual ported RAM		*/
/*                                                         		*/
/* Procedure Arguments: board ID, address				*/
/*                                                         		*/
/* Procedure Returns: value if successful, <0 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int Read_Dual_Port_RAM (short ID, char DPaddr)
   {
   #ifndef OS_VXWORKS
      return -10;
   #else
      volatile char* volatile	addr;	/* base address */
      byte 		response;
   
      /* get base address */
      addr = gpsBaseAddress (ID);
      if (addr == NULL) {
         return -1;
      }
      /* clear response flag and write address */
      response = ((GPSREGS*) addr)->DPData;
      ((GPSREGS*) addr)->DPAddr = DPaddr;
      /* check status bit */
      while ((((GPSREGS*) addr)->Status & Response_Ready) ==0) {}
      /* read data and return after wait of 400us(avrg.) */
      response = ((GPSREGS*) addr)->DPData;
      DPaccess++;
      if ((DPaccess % _DP_DELAY_PER_TICK) == 0) {
         taskDelay (1);
         DPaccess = 0;
      }
      return response;
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: setYearInformation				*/
/*                                                         		*/
/* Procedure Description: sets the year information in the DP RAM	*/
/*                                                         		*/
/* Procedure Arguments: board ID, year					*/
/*                                                         		*/
/* Procedure Returns: 0 if seuucessful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int getYearInformation (short ID)
   {   
      short statusEx;
      int year = 0;
      /* try to read year from board */
      gpsSyncInfo (ID, &statusEx);
      if ((statusEx & DP_Extd_Sts_NoYear) != 0) {
         return -1;
      }
      year = bcd2bin (Read_Dual_Port_RAM (ID, DP_Year10Year1) & 0xff);
      year += 100 * bcd2bin 
              (Read_Dual_Port_RAM (ID, DP_Year1000Year100) & 0xff);
      return year;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: setYearInformation				*/
/*                                                         		*/
/* Procedure Description: sets the year information in the DP RAM	*/
/*                                                         		*/
/* Procedure Arguments: board ID, year					*/
/*                                                         		*/
/* Procedure Returns: 0 if seuucessful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int setYearInformation (short ID, int year)
   {
      int		ret;
      int		told;		/* for sync with 1pps */
   
      ret = Set_Dual_Port_RAM (0, DP_Year10Year1, bin2bcd (year % 100));
      if (ret >= 0) {
         Set_Dual_Port_RAM (0, DP_Year1000Year100, bin2bcd (year / 100));
      }
      if (ret >= 0) {
         Set_Dual_Port_RAM (0, DP_Command, Command_Set_Years);
      }
      if (ret < 0) {
         return -1;
      }
      /* wait for next 1pps */
      told = gpsMicroSec (ID);
      while (gpsMicroSec (ID) > told) {
         nanosleep (&smallDelay, NULL);
      }
   
      return 0;
   }


#if defined (OS_VXWORKS) && (SYNCCLOCK32_0_INT_SOURCE==1)
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: setHeartbeatRate				*/
/*                                                         		*/
/* Procedure Description: sets the heartbeat rate in the DP RAM		*/
/*                                                         		*/
/* Procedure Arguments: board ID, counter divisor			*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void setHeartbeatRate (short ID, unsigned short div)
   {
      Set_Dual_Port_RAM (0, DP_Ctr1_ctl, DP_ctl_bin | DP_ctl_mode2 |
                        DP_ctl_rw | DP_Ctr1_ctl_sel);
      Set_Dual_Port_RAM (0, DP_Ctr1_msb, div / 256);
      Set_Dual_Port_RAM (0, DP_Ctr1_lsb, div % 256);
      Set_Dual_Port_RAM (0, DP_Command, Command_Rejam);
   }
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: bcd2bin					*/
/*                                                         		*/
/* Procedure Description: converts from bcd to binary			*/
/*                                                         		*/
/* Procedure Arguments: bcd value					*/
/*                                                         		*/
/* Procedure Returns: binary value					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#define bcdmask(x) ((x) <= 10 ? (x) : 0)

   static unsigned long bcd2bin (unsigned long bcd)
   {
      unsigned long	b;
      unsigned long	bin;
      int		i;
   
      for (i = 0, bin = 0, b = bcd; i < 2 * sizeof(bcd); i++) {
         bin = 10 * bin + bcdmask (b >> (8 * sizeof(bcd) - 4));
         b = b << 4;
      }
      return bin;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: bin2bcd					*/
/*                                                         		*/
/* Procedure Description: converts from binary to bcd			*/
/*                                                         		*/
/* Procedure Arguments: binary value					*/
/*                                                         		*/
/* Procedure Returns: bcd value						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static unsigned long bin2bcd (unsigned long bin) {
      unsigned long	b;
      unsigned long	bcd;
      int		i;
   
      for (i = 0, bcd = 0, b = bin; i < 2 * sizeof(bin); i++) {
         bcd = (bcd >> 4) + ((b % 10) << (8 * sizeof(bcd) - 4));
         b = b / 10;
      }
      return bcd;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: YDHMS2gps					*/
/*                                                         		*/
/* Procedure Description: converts from year/day/sec into GPS seconds	*/
/*                                                         		*/
/* Procedure Arguments: year, day, hour, min and seconds		*/
/*                                                         		*/
/* Procedure Returns: GPS seconds					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int64_t YDHMS2gps (int year, int day, int hour, 
			     int min, int sec)
   {
      gpstime_t 	gt;	/* gps time */
      tais_t		t;	/* tai & gps seconds */
   
      gt.year = year;
      gt.yearday = day;
      gt.hour = hour;
      gt.minute = min;
      gt.second = sec;
      gpstime_to_gpssec (&gt, &t);
      return t.s /* + 694656019LL*/;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Test Section: gpsTest and gpsTest2					*/
/*                                                         		*/
/* Description: test the gps board					*/
/*              compile with #define _GPSCLK_TEST	  		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#if defined (_GPSCLK_TEST) && defined (OS_VXWORKS)

   static sem_t		tsem;
   static int 		count = -1;
   static double 	n = 0.0;
   static double 	m = 0.0;
   static double 	d = 0.0;
   static double 	r = 0.0;
   static double 	told = 0.0;

   void testISR2 (void) 
   {
      sem_post (&tsem);
   }

   void testISR (unsigned int thi, unsigned int tlo)
   {
      /* unsigned int	thi;
      unsigned int	tlo; */
      double		t;
   
      count++;
      /* gpsNativeTime (0, &thi, &tlo);*/
      t = (double) bcd2bin (tlo) / 1E6;
      if ((t - told > 0) && (t - told < 0.1)) {
         n++;
         m += t - told;
         d += (t - told) * (t - told);
         r += fmod (t, 1.0/16.0);
      }
      told = t;
      /* printf ("time hi = %x, time lo = %x\n", thi, tlo); */
   }


   void gpsTest (void)
   
   {
      unsigned int	thi;
      unsigned int	tlo;
      int		i;
      int		oldhb;
   
      count = oldhb = -1;
      n = m = d = r = 0.0;
      sem_init (&tsem, 0, 0);
   
      printf ("sizeof (GPSREGS) = %d\n", (int) sizeof (GPSREGS));
      gpsNativeTime (0, &thi, &tlo);
      printf ("time hi = %x, time lo = %x\n", thi, tlo);
      /* set year 1998 365 23:59:55 */
      /* setYearInformation (0, 2000);
      Set_Dual_Port_RAM (0, DP_Major_Time_d1000d10, 0x01);
      Set_Dual_Port_RAM (0, DP_Major_Time_d10d1,0x65);
      Set_Dual_Port_RAM (0, DP_Major_Time_h10h1,0x23);
      Set_Dual_Port_RAM (0, DP_Major_Time_m10m1,0x59);
      Set_Dual_Port_RAM (0, DP_Major_Time_s10s1,0x20);
      Set_Dual_Port_RAM (0, DP_Command, Command_Set_Major);
      taskDelay (30); */
      gpsNativeTime (0, &thi, &tlo);
      printf ("time hi = %x, time lo = %x\n", thi, tlo);
   
      printf ("ISR vec = %x\n", (int) 
         intVecGet ((FUNCPTR*) INUM_TO_IVEC(SYNCCLOCK32_0_INT_VEC)));
   
      printf ("hb count = %d\n", hbcount);
      i = gpsHeartbeatInstall (0, testISR);
      if (i != 0) {
         printf ("heartbeat installation failed\n");
         return;
      }
      printf ("ISR vec = %x\n", (int) 
         intVecGet ((FUNCPTR*) INUM_TO_IVEC(SYNCCLOCK32_0_INT_VEC)));
   
      while (count < 50 * 16) {
         if ((count != oldhb) && (count % 16 == 0)) {
            gpsNativeTime (0, &thi, &tlo);
            printf ("time hi = %x, time lo = %x at ", thi, tlo);
            printf ("beat = %d (health = %d us)\n", count, 
               gpsHeartbeatHealth (0));
            oldhb = count;
         }
      }
      gpsHeartbeatInstall (0, NULL);
      printf ("number of interrupts = %d\n", count);
      printf ("number of counts = %-9f\n", n);
      printf ("interrupt rate = %-9f Hz (rms = %-9f ms) \n", 
         n / m, 1E3 * sqrt ((d - m*m/n) / (n - 1)));
      printf ("interrupt delay = %-9f ms\n", 1E3 * r / n);
      sem_destroy (&tsem);
   }

   void gpsTest2 (void)
   {
      int		i;
   
      for (i = 0; i < 20; i++) {
         gpsTest();
         taskDelay (i);
      }
   }
#endif

#if defined (OS_VXWORKS)
   void gpsRead (void)
   {
      gpsInfo_t		info;
      int		ret;
      unsigned int	t1, t2, t3;
   
      ret = 2000;
      gpsInit (0, NULL);
      gpsNativeTime (0, &t3, &t1);
      ret = gpsInfo (0, &info);
      gpsNativeTime (0, NULL, &t2);
      printf ("timehi = %x, timelo = %x\n", t3, t2);
      printf ("Year is %i\n", getYearInformation(0));
      printf ("time to read info = %i us\n", 
             (int) t2 - (int) t1);
      printf ("number of satellites = %d\n", ret);
      if (ret >= 0) {
         printf ("time = %ld sec\n", info.time);
         printf ("synchronization status = 0x%02x\n", info.syncStatus);
         if (info.status == 0) {
            printf ("acquiring\n");
         }
         else {
            printf ("navigate\n");
         }
         printf ("latitude = %-9f deg\n", info.latitude);
         printf ("longitude = %-9f deg\n", info.longitude);
         printf ("altitude = %-9f m\n", info.altitude);
         printf ("speed = %-9f m/s\n", info.speed);
         printf ("direction = %-9f deg\n", info.direction);
      }
      printf ("COMPARE TIMES\n");
      for (ret = 0; ret < 30; ret++) {
      taskDelay (4);
      gpsNativeTime (0, NULL, &t2);
      printf ("timehi = %x, timelo = %x\n", t3, t2);
#if 0 && defined(PROCESSOR_BAJA47)
      sysRtcShow();
#endif
      gpsNativeTime (0, NULL, &t2);
      printf ("timehi = %x, timelo = %x\n\n", t3, t2);
      }
   }

#endif
