static char *versionId = "Version $Id$" ;
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: tconv							*/
/*                                                         		*/
/* Procedure Description: prvides functions to convert TAI and UTC;	*/
/* and functions to handle TAI (international atomic time)		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

#ifndef OS_VXWORKS
#define _LEAP_DYNAMIC
#endif

/* Header File List: */
#ifdef OS_VXWORKS
#include <vxWorks.h>
#include <inetLib.h>
#include <timers.h>

#if defined(PROCESSOR_BAJA47) && defined(_USE_POSIX_TIMER)
#include <heurikon.h>
#define IOBASE        		 0xb0000000
#define DS1286_BASE              (IOBASE+0x0f010600)
#define DS1286_INTERVAL          8
#include <drv/rtc/ds1286.h>
#endif /* defined(PROCESSOR_BAJA47) && defined(_USE_POSIX_TIMER) */

#if !defined(_USE_POSIX_TIMER)
#include "dtt/gpsclk.h"
#define _GPS_BOARD_ID		0
#endif /* !defined(_USE_POSIX_TIMER) */
#define HAVE_CLOCK_GETTIME

#else /* OS_VXWORKS */
#include "PConfig.h"
#if defined (P__SOLARIS)
#include <sys/types.h>
#include <sys/byteorder.h>
#else 
#include <netinet/in.h>
#endif /* defined (P__SOLARIS) */
#endif /* OS_VXWORKS */

#include <string.h>
#include <time.h>
#if !defined(HAVE_CLOCK_GETTIME)
#include <sys/time.h>
#endif
#include "tconv.h"
#include <stdio.h>


#if (defined (_CONFIG_DYNAMIC) || defined (OS_VXWORKS)) &&  \
    defined (_LEAP_DYNAMIC)
#include "gdsmain.h"
#include "gdserr.h"
#include "gdstask.h"
#include "gdsstring.h"
#define _LEAP_PRIORITY		150
#define _LEAP_TASKNAME		"tLeap"
#define _LEAPREAD_INTERVAL	3600	/* 1h */

#if defined (_CONFIG_DYNAMIC)
#include "confinfo.h"
#include "rleap.h"
#define _NETID			"tcp"
#else
#define _FILE_LEAP		gdsPathFile ("/param", "leap.conf")
typedef int confinfo_t;
#endif
#endif

/* define some time constants */
#define SECS_PER_HOUR 	(60 * 60)
#define SECS_PER_DAY 	(SECS_PER_HOUR * 24)

/* define leap year macros */
#define ISLEAP(year) \
  ((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0))
#define LEAPS_THRU_END_OF(y) ((y) / 4 - (y) / 100 + (y) / 400)

/* defines conversion coefficients between TAI and UTC */
#define OFFS_YEAR 1972
#define OFFS_LEAP 10
#define OFFS_TAI (((72-58) * 365 + 3) * SECS_PER_DAY + OFFS_LEAP)
#define OFFS_WDAY 6  /*  January 1, 1972 was a Saturday */

/* How many days come before each month (0-12) */
   static const unsigned short int mon_yday[2][13] =
   {
    /* Normal years.  */
   { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
    /* Leap years.  */
   { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
   };

/* This table contains every TAI second which is a leap second
   the second argument is the correction to apply, relative to
   Jan. 1, 1972. The 10sec difference at Jan. 1, 1972 is already
   included in OFFS_TAI. If a leap second has to be removed its
   TAI has to be the second just prior to the occurance of the leap.
   There must one entry for every added leap second.
   This list has to be in ascending order of time. */

   static int num_leaps = 26;
   static leap_t leaps[100] = 
   {{OFFS_TAI + 182*SECS_PER_DAY, +1}, 			/* Jul. 1, 1972 */
    {OFFS_TAI + (365+1)*SECS_PER_DAY+ 1, +2},		/* Jan. 1, 1973 */
    {OFFS_TAI + (2*365+1)*SECS_PER_DAY+ 2, +3},		/* Jan. 1, 1974 */
    {OFFS_TAI + (3*365+1)*SECS_PER_DAY+ 3, +4},		/* Jan. 1, 1975 */
    {OFFS_TAI + (4*365+1)*SECS_PER_DAY+ 4, +5},		/* Jan. 1, 1976 */
    {OFFS_TAI + (5*365+2)*SECS_PER_DAY+ 5, +6},		/* Jan. 1, 1977 */
    {OFFS_TAI + (6*365+2)*SECS_PER_DAY+ 6, +7},		/* Jan. 1, 1978 */
    {OFFS_TAI + (7*365+2)*SECS_PER_DAY+ 7, +8},		/* Jan. 1, 1979 */
    {OFFS_TAI + (8*365+2)*SECS_PER_DAY+ 8, +9},		/* Jan. 1, 1980 */
    {OFFS_TAI + (9*365+3+181)*SECS_PER_DAY+ 9, +10},	/* Jul. 1, 1981 */
    {OFFS_TAI + (10*365+3+181)*SECS_PER_DAY+10,+11},	/* Jul. 1, 1982 */
    {OFFS_TAI + (11*365+3+181)*SECS_PER_DAY+11,+12},	/* Jul. 1, 1983 */
    {OFFS_TAI + (13*365+4+181)*SECS_PER_DAY+12,+13},	/* Jul. 1, 1985 */
    {OFFS_TAI + (16*365+4)*SECS_PER_DAY+13, +14},	/* Jan. 1, 1988 */
    {OFFS_TAI + (18*365+5)*SECS_PER_DAY+14, +15},	/* Jan. 1, 1990 */
    {OFFS_TAI + (19*365+5)*SECS_PER_DAY+15, +16},	/* Jan. 1, 1991 */
    {OFFS_TAI + (20*365+5+182)*SECS_PER_DAY+16,+17},	/* Jul. 1, 1992 */
    {OFFS_TAI + (21*365+6+181)*SECS_PER_DAY+17,+18},	/* Jul. 1, 1993 */ 
    {OFFS_TAI + (22*365+6+181)*SECS_PER_DAY+18,+19},	/* Jul. 1, 1994 */
    {OFFS_TAI + (24*365+6)*SECS_PER_DAY+19, +20},	/* Jan. 1, 1996 */
    {OFFS_TAI + (25*365+7+181)*SECS_PER_DAY+20, +21},	/* Jul. 1, 1997 */
    {OFFS_TAI + (27*365+7)*SECS_PER_DAY+21, +22},	/* Jan. 1, 1999 */
    {OFFS_TAI + (34*365+9)*SECS_PER_DAY+22, +23}, 	/* Jan. 1, 2006 */
    {OFFS_TAI + (37*365+10)*SECS_PER_DAY+23,+24},       /* Jan. 1, 2009 */
    {OFFS_TAI + (40*365+10+182)*SECS_PER_DAY+24,+25},   /* Jul. 1, 2012 */
    {OFFS_TAI + (43*365+11+181)*SECS_PER_DAY+25,+26}};  /* Jul. 1, 2015 */

#if (defined (_CONFIG_DYNAMIC) || defined (OS_VXWORKS)) &&  \
    defined (_LEAP_DYNAMIC)
   static mutexID_t	leapmux;	/* mutex for leap list */
   static taskID_t	leapTID = 0;	/* leap config task */
   static int		leapinit = 0;	/* leap init */

   __init__ (initLeapConfig);
#pragma init(initLeapConfig)
   __fini__(finiLeapConfig);
#pragma fini(finiLeapConfig)
   static int leapConfig (void);

#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: TAItoUTC					*/
/*                                                         		*/
/* Procedure Description: converts TAI to UTC				*/
/* 									*/
/*                                                         		*/
/* Procedure Arguments: taisec_t t, utc_t* utc_ptr			*/
/*                                                         		*/
/* Procedure Returns: utc_t* or NULL when failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   utc_t* TAItoUTC (taisec_t t, utc_t* utc_ptr)
   {
      long	tt;  /* seconds since Jan. 1, OFFS_YEAR */
      long int	leap_correction;
      int 	leap_extra_secs;
      long int	days;
      long int	rem;
      long int	y;
      register	int i;
      const unsigned short int *ip;
   
   #if (defined (_CONFIG_DYNAMIC) || defined (OS_VXWORKS)) && \
    defined (_LEAP_DYNAMIC)
      if (leapinit == 0) {
         leapinit = 1;
         leapConfig ();
      }
   #endif
   
      t += TAIatGPSzero;
      if ((utc_ptr == NULL) || (t < OFFS_TAI)) {
         return NULL;
      }
   
      /* find the last leap second correction transition time before t */
   #if (defined (_CONFIG_DYNAMIC) || defined (OS_VXWORKS)) &&  \
    defined (_LEAP_DYNAMIC)
      MUTEX_GET (leapmux);
   #endif
      i = num_leaps;
      do {
         i--;
      } while ((i >= 0) && (t < leaps[i].transition));
   
      /* apply leap correction.  */
      if (i < 0) {
         /* before 1st leap second */
         leap_correction = 0;
         leap_extra_secs = 0;
      }
      else {
         leap_correction = leaps[i].change;
      
         /* exactly at transition time ?  */
         if ((t == leaps[i].transition) && 
            (((i == 0) && (leaps[i].change > 0)) ||
            (leaps[i].change > leaps[i - 1].change))) {
            leap_extra_secs = 1; /* sec = 60 */
            /* more than one leap second ? */
            while ((i > 0) &&
                  (leaps[i].transition == leaps[i - 1].transition + 1) &&
                  (leaps[i].change == leaps[i - 1].change + 1)) {
               leap_extra_secs++;
               i--;
            }
         }
         else {
            leap_extra_secs = 0;
         }
      }
   #if (defined (_CONFIG_DYNAMIC) || defined (OS_VXWORKS)) &&  \
    defined (_LEAP_DYNAMIC)
      MUTEX_RELEASE (leapmux);
   #endif
   
   /* calculate utc from TAI */
      tt = t - OFFS_TAI - leap_correction;
      /* first separate days from the rest */
      days = tt / SECS_PER_DAY;
      rem = tt % SECS_PER_DAY;
      while (rem < 0) {
         rem += SECS_PER_DAY;
         days--;
      }
      while (rem >= SECS_PER_DAY) {
         rem -= SECS_PER_DAY;
         days++;
      }
   
      /* fill in hours, minutes, sec */
      utc_ptr->tm_hour = rem / SECS_PER_HOUR;
      rem %= SECS_PER_HOUR;
      utc_ptr->tm_min = rem / 60;
      utc_ptr->tm_sec = rem % 60;
   
      /* fill in week day */
      utc_ptr->tm_wday = (OFFS_WDAY + days) % 7;
      if (utc_ptr->tm_wday < 0) {
         utc_ptr->tm_wday += 7;
      }
   
      /* calculate year */ 
      y = OFFS_YEAR;
      while ((days < 0) || (days >= (ISLEAP (y) ? 366 : 365))) {
         /* Guess a corrected year, assuming 365 days per year.  */
         long int yg = y + days / 365 - (days % 365 < 0);
      
         /* Adjust DAYS and Y to match the guessed year.  */
         days -= ((yg - y) * 365
                 + LEAPS_THRU_END_OF (yg - 1)
                 - LEAPS_THRU_END_OF (y - 1));
         y = yg;
      }
   
      /* fill in year, days of year */
      utc_ptr->tm_year = y - 1900;
      utc_ptr->tm_yday = days;
   
      /* fill in month and day of month */
      ip = mon_yday[ISLEAP(y)];
      for (y = 11; days < ip[y]; --y) {
         continue;
      }
      days -= ip[y];
      utc_ptr->tm_mon = y;
      utc_ptr->tm_mday = days + 1;
   
      /* adjust extra seconds when leap occures */
      utc_ptr->tm_sec += leap_extra_secs;      
   
      return utc_ptr;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: TAIntoUTC					*/
/*                                                         		*/
/* Procedure Description: converts TAI to UTC				*/
/* 									*/
/*                                                         		*/
/* Procedure Arguments: tainsec_t t, utc_t* utc_ptr			*/
/*                                                         		*/
/* Procedure Returns: utc_t* or NULL when failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   utc_t* TAIntoUTC (tainsec_t t, utc_t* utc_ptr)
   {
      taisec_t tai;
   
      if ((tai = TAIsec (t, NULL)) == 0) {
         return NULL;
      }
      return TAItoUTC (tai, utc_ptr);
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: UTCtoTAI					*/
/*                                                         		*/
/* Procedure Description: converts UTC to TAI				*/
/* 									*/
/*                                                         		*/
/* Procedure Arguments: const utc_t* utc_ptr				*/
/*                                                         		*/
/* Procedure Returns: taisec_t or 0 when failed				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   taisec_t UTCtoTAI (const utc_t* utc_ptr)
   {
      int	mon_remainder;
      int 	mon_years;
      int 	year;
      int 	ydays;
      int 	days;
      taisec_t	tai;
      int 	leap_correction;
      int 	leap_extra_secs;
      int 	i;
   
   #if (defined (_CONFIG_DYNAMIC) || defined (OS_VXWORKS)) && \
    defined (_LEAP_DYNAMIC)
      if (leapinit == 0) {
         leapinit = 1;
         leapConfig ();
      }
   #endif
   
      if (utc_ptr == NULL) {
         return 0;
      }
   
      /* Ensure that month is in range */
      mon_remainder = utc_ptr->tm_mon % 12;
      mon_years = utc_ptr->tm_mon / 12;
      if (mon_remainder < 0) {
         mon_remainder += 12;
         mon_years--;
      }
      /* caluclate year and days in year */
      year = (utc_ptr->tm_year + mon_years) + 1900;
      ydays = mon_yday[ISLEAP(year)][mon_remainder] + 
              utc_ptr->tm_mday - 1;
      /* calculate days since OFFS_YEAR */
      days = ydays + 365 * (year - OFFS_YEAR) + 
             LEAPS_THRU_END_OF (year - 1) -
             LEAPS_THRU_END_OF (OFFS_YEAR - 1);
      if (days < 0) {
         return 0;
      }
   
      /* calculate TAI neglecting leap seconds */
      tai = ((taisec_t) days) * SECS_PER_DAY + OFFS_TAI +
            utc_ptr->tm_hour * SECS_PER_HOUR + 
            utc_ptr->tm_min * 60 + utc_ptr->tm_sec;
      /* correct for leap seconds */
      leap_extra_secs = (utc_ptr->tm_sec > 59) ? 
                        (utc_ptr->tm_sec - 59) : 0;
      leap_correction = 0;
      i = 0;
   #if (defined (_CONFIG_DYNAMIC) || defined (OS_VXWORKS)) &&  \
    defined (_LEAP_DYNAMIC)
      MUTEX_GET (leapmux);
   #endif
      while ((i < num_leaps) && 
            (((leap_extra_secs > 0) &&	/* handles leap seconds */
            (tai + leap_correction - leap_extra_secs > 
            leaps[i].transition)) ||
            ((leap_extra_secs == 0) && 	/* handles non-leap seconds */
            (tai + leap_correction >= leaps[i].transition)))) {
         leap_correction = leaps[i].change; 
         i++;
      }
   #if (defined (_CONFIG_DYNAMIC) || defined (OS_VXWORKS)) &&  \
    defined (_LEAP_DYNAMIC)
      MUTEX_RELEASE (leapmux);
   #endif
      return (tai + leap_correction) - TAIatGPSzero;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: UTCtoTAIn					*/
/*                                                         		*/
/* Procedure Description: converts UTC to TAI				*/
/* 									*/
/*                                                         		*/
/* Procedure Arguments: const utc_t* utc_ptr				*/
/*                                                         		*/
/* Procedure Returns: tainsec_t or 0 when failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   tainsec_t UTCtoTAIn (const utc_t* utc_ptr)
   {
      tai_t 	t;
   
      t.tai = UTCtoTAI (utc_ptr);
      if (t.tai == 0) {
         return 0;
      }
      t.nsec = 0;
      return TAInsec (&t);
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: TAInow					*/
/*                                                         		*/
/* Procedure Description: returns the current TAI			*/
/* 									*/
/*                                                         		*/
/* Procedure Arguments:	void						*/
/*                                                         		*/
/* Procedure Returns: tainsec_t 					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   tainsec_t TAInow (void)
   {

#ifndef _TP_DAQD
	// These are pointers into the shared memory
	extern volatile int *ioMemDataGPS;
	extern volatile int *ioMemDataCycle;


	// Cycle is at 64Khz
	tainsec_t t =  ((tainsec_t)(*ioMemDataGPS)) * 1000000000
		+ ((tainsec_t)*ioMemDataCycle) * (1000000000/(64*1024));
	//printf("%ld\n", t);
	return t;
#else
      /* can be implementation dependent */
   
      /* use current time returned by time.h, uses POSIX */
      struct tm 	utc;
      struct timespec 	now;
      tai_t		tai;
   
      /* for Bajas initialize the posix clock with the on-board
         real-time clock (DS1286 chip) */
   #if defined(OS_VXWORKS) && defined(PROCESSOR_BAJA47) && \
    defined(_USE_POSIX_TIMER)
   #define sysBcdToBin(bcd) ((((bcd) >> 4) * 10) + ((bcd) & 0xf))
      {
         static _initRTC = 0;
         /* sysRtcGet (&utc); */
         if (_initRTC == 0) {
            /* latch the time via the transfer enable bit */
            /* this does not stop the clock */
            *DS1286_CR &= ~DS1286_CR_TE;
            now.tv_nsec = 10000000 * sysBcdToBin(*DS1286_MSEC);
            utc.tm_sec = sysBcdToBin(*DS1286_SEC);
            utc.tm_min = sysBcdToBin(*DS1286_MIN);
            utc.tm_hour = sysBcdToBin(*DS1286_HOURS);
            utc.tm_mday = sysBcdToBin(*DS1286_DATE);
            utc.tm_mon = sysBcdToBin(*DS1286_MONTH & 0x3f) - 1;
            utc.tm_year = sysBcdToBin(*DS1286_YEAR);
            if (utc.tm_year < 96) {
               utc.tm_year += 100;
            }
            /* unlatch the time */
            *DS1286_CR |= DS1286_CR_TE;
            /* initialze POSIX timer */
            now.tv_sec = mktime (&utc);
            clock_settime (CLOCK_REALTIME, &now);
            _initRTC = 1;
         }
      }
   #endif
   #if defined (OS_VXWORKS) && !defined (_USE_POSIX_TIMER)
      /* VMESYNCCLOCK stuff here */
      return (tainsec_t) gpsTimeNow (_GPS_BOARD_ID);
   #elif !defined(HAVE_CLOCK_GETTIME)
      {
         struct timeval tp;
         if (gettimeofday (&tp, NULL) != 0) {
            return 0;
         }
         now.tv_sec = tp.tv_sec;
         now.tv_nsec = 1000 * tp.tv_usec;
      }
   #else
      if (clock_gettime (CLOCK_REALTIME, &now) != 0) {
         return 0;
      }
   #endif
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
      tai.tai = UTCtoTAI (&utc);
      tai.nsec = now.tv_nsec;
      return TAInsec (&tai);
#endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: TAInsec					*/
/*                                                         		*/
/* Procedure Description: converts between tai_t and TAInsec 		*/
/* 									*/
/*                                                         		*/
/* Procedure Arguments:	tai_t* t					*/
/*                                                         		*/
/* Procedure Returns: tainsec_t						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   tainsec_t TAInsec (const tai_t* t)
   {
      if (t == NULL) {
         return 0;
      }
      return _ONESEC * (tainsec_t) t->tai + (tainsec_t) t->nsec;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: TAIsec					*/
/*                                                         		*/
/* Procedure Description: converts from TAInsec to TAIsec and		*/
/* tai_t (if tai != NULL)						*/
/*                                                         		*/
/* Procedure Arguments:	const tainsec_t t, tai_t* tai			*/
/*                                                         		*/
/* Procedure Returns: taisec_t						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   taisec_t TAIsec (tainsec_t t, tai_t* tai)
   {
      tai_t 	t0;
   
      t0.tai = (taisec_t) (t / _ONESEC);
      t0.nsec = (taisec_t) (t % _ONESEC);
      if (tai != NULL) {
         *tai = t0;
      }
      /* round to next second */
      if (t0.nsec < 500000000L) { 
         return t0.tai;
      }
      else {
         return t0.tai + 1;
      }
   }



/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: htonTAI					*/
/*                                                         		*/
/* Procedure Description: converts TAI to network byte order		*/
/* 									*/
/*                                                         		*/
/* Procedure Arguments:	tainsec_t t, char* buf (needs 8 bytes)		*/
/*                                                         		*/
/* Procedure Returns: char* to buf or NULL if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   char* htonTAI (tainsec_t t, char* buf)
   {
      tai_t 	tai;
   
      if ((buf == NULL) || (TAIsec (t, &tai) == 0)) {
         return NULL;
      }
      tai.tai = htonl (tai.tai);
      tai.nsec = htonl (tai.nsec);
      memcpy (buf, &tai, sizeof (tai_t));
      return buf;
   }



/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: ntohTAI					*/
/*                                                         		*/
/* Procedure Description: converts TAI from network byte order		*/
/* 									*/
/*                                                         		*/
/* Procedure Arguments:	const char* buf (reads 8 bytes)			*/
/*                                                         		*/
/* Procedure Returns: tainsec_t	or 0 if failed				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   tainsec_t ntohTAI (const char* buf)
   {
      tai_t 	tai;
   
      if (buf == NULL) {
         return 0;
      }
      memcpy (&tai, buf, sizeof (tai_t));
      tai.tai = ntohl (tai.tai);
      tai.nsec = ntohl (tai.nsec);
      return TAInsec (&tai);
   }



/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: getNextLeap					*/
/*                                                         		*/
/* Procedure Description: returns the next leap second			*/
/* only looks for leaps > t						*/
/*                                                         		*/
/* Procedure Arguments:	taisec_t t, struct leap* nextleap		*/
/*                                                         		*/
/* Procedure Returns: struct leap* (or NULL when no leap left 		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   leap_t* getNextLeap (taisec_t t, leap_t* nextleap)
   {
      int 	i;
   
   #if (defined (_CONFIG_DYNAMIC) || defined (OS_VXWORKS)) && \
    defined (_LEAP_DYNAMIC)
      if (leapinit == 0) {
         leapinit = 1;
         leapConfig ();
      }
   #endif
   
      if (nextleap == NULL) {
         return NULL;
      }
   
   #if (defined (_CONFIG_DYNAMIC) || defined (OS_VXWORKS)) &&  \
    defined (_LEAP_DYNAMIC)
      MUTEX_GET (leapmux);
   #endif
   
      /* determine last index in leap table with transition <= t */
      i = 0;
      while ((i < num_leaps) && 
            (((tainsec_t) leaps[i].transition - TAIatGPSzero) <= t)) {
         i++; 
      }
   
      /* return next leap if exists */
      if (i < num_leaps) {
         nextleap->transition = leaps[i].transition - TAIatGPSzero;
         nextleap->change = leaps[i].change + OFFS_LEAP;
      #if (defined (_CONFIG_DYNAMIC) || defined (OS_VXWORKS)) &&  \
    defined (_LEAP_DYNAMIC)
         MUTEX_RELEASE (leapmux);
      #endif
         return nextleap;
      }
      else {
         nextleap->transition = 0;
         nextleap->change = 0;
      #if (defined (_CONFIG_DYNAMIC) || defined (OS_VXWORKS)) &&  \
    defined (_LEAP_DYNAMIC)
         MUTEX_RELEASE (leapmux);
      #endif
         return NULL;
      }
   }


#if (defined (_CONFIG_DYNAMIC) || defined (OS_VXWORKS)) &&  \
    defined (_LEAP_DYNAMIC)
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: readLeap					*/
/*                                                         		*/
/* Procedure Description: reads leap second info from server		*/
/*                                                         		*/
/* Procedure Arguments:	conf info					*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwise	 		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#if defined (_CONFIG_DYNAMIC)
   static int readLeap (confinfo_t* crec)
   {
      resultLeapQuery_r		res;		/* rpc return */
      int			i;		/* leap index */
      static CLIENT*		clnt;		/* rpc client handle */
      struct timeval 		timeout;	/* connect timeout */
   
      timeout.tv_sec = RPC_PROBE_WAIT;
      timeout.tv_usec = 0;
   
      memset (&res, 0, sizeof (res));
      if (!rpcProbe (crec->host, crec->port_prognum, crec->progver,
         _NETID, &timeout, &clnt) ||
         (leapquery_1 (&res, clnt) != RPC_SUCCESS) ||
         (res.status != 0)) {
         return -1;
      }
      clnt_destroy (clnt);
   
      if (res.leaplist.leaplist_r_len + 9 != num_leaps) {
         MUTEX_GET (leapmux);
         for (i = 0; i < res.leaplist.leaplist_r_len; i++) {
            leaps[i+9].transition = 
               res.leaplist.leaplist_r_val[i].transition + TAIatGPSzero;
            leaps[i+9].change = 
               res.leaplist.leaplist_r_val[i].change - OFFS_LEAP;
         }
         num_leaps = 9 + res.leaplist.leaplist_r_len;
         MUTEX_RELEASE (leapmux);
         gdsConsoleMessage ("leap second information changed");
      }
   
      xdr_free (xdr_resultLeapQuery_r, (char*) &res);   
      return 0;
   }

#else 
   static int readLeap (confinfo_t* x)
   {
      leap_t		newleaps[100];	/* new leaps */
      int		num = 0;	/* new leaps index */
      int		i = 0;		/* new leaps index */
      FILE*		fp;		/* file descr. */
      char		line[128];	/* line from file */
      char*		p;		/* pointer into line */
   
      /* open file */
      fp = fopen (_FILE_LEAP, "r");
      if (fp == NULL) {
         return -1;
      }
   
      /* read in leap config. file */
      while ((p = fgets (line, sizeof (line), fp)) != NULL) {
         while ((*p == ' ') || (*p == '\t')) {
            p++;
         }
         if (*p == '#') {
            continue;
         }
         if (sscanf (p, "%li %i", &newleaps[num].transition, 
            &newleaps[num].change) != 2) {
            fclose (fp);
            return -1;
         }
         if (++num >= 90) {
            break;
         }
      }
      fclose (fp);
   
      /* set new leap information */
      if (num + 9 != num_leaps) {
         MUTEX_GET (leapmux);
         for (i = 0; i < num; i++) {
            leaps[i+9].transition = 
               newleaps[i].transition + TAIatGPSzero;
            leaps[i+9].change = 
               newleaps[i].change - OFFS_LEAP;
         }
         num_leaps = 9 + num;
         MUTEX_RELEASE (leapmux);
         gdsConsoleMessage ("leap second information changed");
      }
      return 0;
   }

#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: leapTask					*/
/*                                                         		*/
/* Procedure Description: calls readLeap once 1 hour			*/
/*                                                         		*/
/* Procedure Arguments:	conf info					*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwise	 		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void leapTask (confinfo_t* crec)
   {
      struct timespec wait;
   
      while (1) {
	 wait.tv_sec = _LEAPREAD_INTERVAL;
	 wait.tv_nsec = 0;
         nanosleep (&wait, NULL);
         readLeap (crec);
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: leapConfig					*/
/*                                                         		*/
/* Procedure Description: starts the dynamic leap second configuration	*/
/*                                                         		*/
/* Procedure Arguments:	void						*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwise	 		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#if defined (_CONFIG_DYNAMIC)
   static int leapConfig (void)
   {
      const char* const* 	cinfo;		/* configuration info */
      static confinfo_t		crec;		/* conf. info record */   
      int			i;		/* leap list index */
      leap_r*			info;		/* infor pointer */
      int			attr;		/* task create attrib. */
   
      /* get leap second server info */
      for (cinfo = getConfInfo (0, 0); *cinfo != NULL; cinfo++) {
         if ((parseConfInfo (*cinfo, &crec) == 0) &&
            (gds_strcasecmp (crec.interface, CONFIG_SERVICE_LEAP) == 0) &&
            (crec.ifo == -1) && (crec.port_prognum > 0) &&
            (crec.progver > 0) && (strlen (crec.host) > 0)) {
            if (readLeap (&crec) < 0) {
               return -1;
            }
         #ifdef OS_VXWORKS
            attr = VX_FP_TASK;
         #else
            attr = PTHREAD_CREATE_DETACHED | PTHREAD_SCOPE_SYSTEM;
         #endif
            taskCreate (attr, _LEAP_PRIORITY, &leapTID, 
                       _LEAP_TASKNAME, (taskfunc_t) leapTask, &crec);
            return 0;
         }
      }
      return -1;
   }

#else
   static int leapConfig (void)
   {
      int			attr;		/* task create attrib. */
   
      /* get leap second server info */
      if (readLeap (NULL) < 0) {
         return -1;
      }
   #ifdef OS_VXWORKS
      attr = VX_FP_TASK;
   #else
      attr = PTHREAD_CREATE_DETACHED | PTHREAD_SCOPE_SYSTEM;
   #endif
      taskCreate (attr, _LEAP_PRIORITY, &leapTID, 
                 _LEAP_TASKNAME, (taskfunc_t) leapTask, NULL);
      return 0;
   
   }
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: initLeapConfig				*/
/*                                                         		*/
/* Procedure Description: initialization				*/
/*                                                         		*/
/* Procedure Arguments:	void						*/
/*                                                         		*/
/* Procedure Returns: void				 		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void initLeapConfig (void) 
   {
      if (MUTEX_CREATE (leapmux) != 0) {
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: finiLeapConfig				*/
/*                                                         		*/
/* Procedure Description: cleanup					*/
/*                                                         		*/
/* Procedure Arguments:	void						*/
/*                                                         		*/
/* Procedure Returns: void				 		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void finiLeapConfig (void) 
   {
      taskCancel (&leapTID);
      MUTEX_DESTROY (leapmux);
   }
#endif
