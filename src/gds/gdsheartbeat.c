static char *versionId = "Version $Id$" ;
#if !defined(OS_VXWORKS) && !defined(GDS_ONLINE)
#ifndef AVOID_SIGNALS
#define AVOID_SIGNALS
#endif
#endif

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdsheartbeat						*/
/*                                                         		*/
/* Procedure Description: provides functions to utilize the system	*/
/* heartbeat; in LIGO 16Hz						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

/* Header File List: */
#include "gdsutil.h"
#include <time.h>

/* VxWorks */
#ifdef OS_VXWORKS
#include <vxWorks.h>
#include <semLib.h>
#include <iv.h>
#include <sigLib.h>
#include <taskLib.h>
#include <timers.h>

#ifdef _USE_POSIX_TIMER
#include <sysLib.h>
#define SYS_CLK_RATE_MIN	38
#define SYS_CLK_RATE_MAX	10000
#include <private/timerLibP.h>
#endif

#if !defined(_USE_POSIX_TIMER)
#if GDS_SITE == GDS_SITE_MIT
/* reflective memory interrupts for PNI test */
#inclide "dtt/rmapi.h"
#else
/* GPS board interrupt otherwise */
#include "dtt/gpsclk.h"
#define _GPS_BOARD_ID		0
#endif
#endif

/* others */
#else
#include <errno.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>
#if !defined(AVOID_SIGNALS)
#include <signal.h>
#endif
#include <pthread.h>
#include "dtt/gdstask.h"
#endif

/* all */
#include "dtt/gdsheartbeat.h"

#include "rmapi.h"

/* The total number of DAQ bytes the front-end is sending per second */
/* This is the current number of bytes and it is updated 16 times per second */
unsigned int curDaqBlockSize;

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Signals:  signal used for heartbeat timer (POSIX only)		*/
/*           SIGUSR2                                           		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#if !defined (OS_VXWORKS) || defined (_USE_POSIX_TIMER)
#define SIGheartbeat SIGUSR2
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Types: semhb_t	semaphore used for heartbeat			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#ifdef OS_VXWORKS
   typedef SEM_ID semhb_t;
#else
   typedef pthread_cond_t* semhb_t;
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Globals: semHeartbeat: heartbeat semaphore;				*/
/*          		  gets release every heartbeat	              	*/
/*          heartbeatCount: counts the number of heartbeats received	*/
/*                          since the heartbeat was installed		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static semhb_t semHeartbeat = NULL;
   static unsigned long heartbeatCount = 0;


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Forward declarations: 						*/
/*	initHeartbeat		init of hearbeat interface		*/
/*	finitHearbeat		stops the hearbeat 			*/
/*      								*/
/*----------------------------------------------------------------------*/
   __init__(initHeartbeat);

#ifndef __GNUC__
#pragma init(initHeartbeat)
#endif
   __fini__(finiHeartbeat);
#ifndef __GNUC__
#pragma fini(finiHeartbeat)
#endif

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: doHeartbeat					*/
/*                                                         		*/
/* Procedure Description: releases the heartbeat semaphore		*/
/* 									*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: 0 when successful or error number when failed	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#ifdef OS_VXWORKS	/* VxWorks version */

   int doHeartbeat (void) 
   {
      /* wait for semaphore */
      if ((semHeartbeat == NULL) || 
         (semFlush (semHeartbeat) == ERROR)) {
         return -1;
      } 
      else {
         /* increment interrupt count */
         heartbeatCount++;
         return 0;
      }
   }

#else			/* UNIX Posix version */

   int doHeartbeat (void) 
   {
      /* wait for semaphore */
      if (semHeartbeat == 0) return -1;
      if (pthread_cond_broadcast (semHeartbeat) != 0) return -1;

      /* increment interrupt count */
      heartbeatCount++;
      return 0;
   }

#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: setupHeartbeat				*/
/*                                                         		*/
/* Procedure Description: releases the heartbeat semaphore		*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: 0 when successful or error number when failed	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#ifdef OS_VXWORKS	/* VxWorks version */

   int setupHeartbeat (void)
   {
      /* test if heartbeat semaphore is already created */
      if (semHeartbeat != NULL) {
         return -2;
      }
      /* create a new heartbet semaphore */
      semHeartbeat = semBCreate (SEM_Q_FIFO, SEM_EMPTY);
      if (semHeartbeat == NULL) {
         return -1;
      }
#if !defined(_USE_POSIX_TIMER) && (GDS_SITE != GDS_SITE_MIT)
      /* init VMESYNCCLOCK board */
      return gpsInit (_GPS_BOARD_ID, NULL);
#else
      return 0;
#endif
   }

#else			/* UNIX Posix version */

   int setupHeartbeat (void)
   {
      static pthread_cond_t semHB;
   
      /* test if heartbeat semaphore is already created */
      if (semHeartbeat != NULL) {
         return -2;
      }
      /* create a new heartbet semaphore */ 
      if (pthread_cond_init (&semHB, NULL) != 0) {
         return -1;
      }
      semHeartbeat = &semHB;
      return 0;
   }

#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: getTimeAndEpoch				*/
/*                                                         		*/
/* Procedure Description: obtains the current time and divides it	*/
/*                        TAI in sec and an epoch count			*/
/*                                                         		*/
/* Procedure Arguments: _tai : pointer to TAI time variable		*/
/*                      _epoch: pointer to epoch variable 		*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void getTimeAndEpoch (taisec_t* _tai, int* _epoch)
   {
      tainsec_t	tain;	/* tai in nsec */
      tai_t	taibd;	/* broken down time */
      taisec_t	tai;	/* tai of epoch in sec */
      int	epoch;	/* epoch */
   
      /* obtain current time and calculate epoch */
      tain = TAInow();
      TAIsec (tain, &taibd);
      tai = taibd.tai;
      epoch = (taibd.nsec + _EPOCH / 10) / _EPOCH;
      if (epoch >= NUMBER_OF_EPOCHS) {
         epoch -= NUMBER_OF_EPOCHS;
         tai++;
      }
      if (_tai != NULL) {
         *_tai = tai;
      }
      if (_epoch != NULL) {
         *_epoch = epoch;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: synchWithHeartbeat				*/
/*                                                         		*/
/* Procedure Description: blocks until the next heartbeat		*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: 0 when successful or error number when failed	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#ifdef OS_VXWORKS	/* Vx Works version */
   int syncWithHeartbeat (void)
   {
      static struct timespec	tick = 
      {_EPOCH / _ONESEC, _EPOCH % _ONESEC};
   
      /* test if heartbeat semaphore is already created */
      if (semHeartbeat == NULL) {
         nanosleep (&tick, NULL);
         return -2;
      }
   
      /* wait for the next heartbeat */
      if (semTake (semHeartbeat, WAIT_FOREVER) != 0) {
         return -1;
      }
      else {
         return 0;
      }
   }

#else

   static pthread_mutex_t  dummy = PTHREAD_MUTEX_INITIALIZER;

   int syncWithHeartbeat (void)
   {
      static struct timespec	tick = 
      {_EPOCH / _ONESEC, _EPOCH % _ONESEC};
   
      /* test if heartbeat semaphore is already created */
      if (semHeartbeat == NULL) {
         nanosleep (&tick, NULL);
         return -2;
      }

      /* wait for the next heartbeat */
      pthread_mutex_lock (&dummy);
      pthread_cond_wait (semHeartbeat, &dummy);
      pthread_mutex_unlock (&dummy);
      return 0;
   }

#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: synchWithHeartbeatEx			*/
/*                                                         		*/
/* Procedure Description: blocks until the next heartbeat		*/
/*                                                         		*/
/* Procedure Arguments: tai: variable to store TAI of heartbeat		*/
/*                      epoch: variable to store epoch count 		*/
/*                                                         		*/
/* Procedure Returns: 0 when successful or error number when failed	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int syncWithHeartbeatEx (taisec_t* tai, int* epoch)
   {
      int	status;
   
      status = syncWithHeartbeat ();
      if (status == 0) {
         getTimeAndEpoch (tai, epoch);
      }
      return status;
   }


/* Forward declarations */
#if !defined(AVOID_SIGNALS)
   static void defaultISR ();
#endif
   static int  connectHeartbeatISR (void ISR ());


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: installHeartbeat				*/
/*                                                         		*/
/* Procedure Description: installs the interrupt service and reset	*/
/* the heartbeat semaphore.						*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: 0 when successful or error number when failed	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#if defined(OS_VXWORKS) && defined (_USE_POSIX_TIMER)
   static void heartbeatProc (void hbISR (void)) 
   {
      /* semFlush and signals don't work within the same task context */
      connectHeartbeatISR (hbISR);
      /* never to return */
      for (;;) {
         taskDelay (1000000);
      }
   }
#endif

#ifndef _TP_DAQD
   int installHeartbeat (void ISR (void))
   {
      void (*hbISR) (void);

      /* do not install more than one hearbeat clock */
      switch (setupHeartbeat ()) 
      {
         case 0: 
            {
               /* init successful */
               break;
            }
         case -2: 
            {
               /* already initialized, return immeadiately */
               return 0;
            }
         default:
            {
               /* init failed */
               return -1;
            }
      }
   
      /* assign default ISR if argument is NULL */
   #if !defined(AVOID_SIGNALS)
      if (ISR == NULL) {
         hbISR = defaultISR;
      }
      else {
   #endif
      hbISR = ISR;
   #if !defined(AVOID_SIGNALS)
      }
   #endif
      /* connect target specific ISR */
   #if defined(OS_VXWORKS) && defined (_USE_POSIX_TIMER)
      taskSpawn ("tHB", 102, 0, 10000, (FUNCPTR) heartbeatProc, 
                (int) hbISR, 0, 0, 0, 0, 0, 0, 0, 0, 0);
      taskDelay (1);
      return 0;
   #else
      return connectHeartbeatISR (hbISR);
   #endif
   }
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: defaultISR					*/
/*                                                         		*/
/* Procedure Description: declares the default ISR			*/
/* 									*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#if defined(OS_VXWORKS) && !defined(_USE_POSIX_TIMER)

#if GDS_SITE == GDS_SITE_MIT

   /* reflective memory */
   static void defaultISR (void)
   {
      int	sid;
      int	inum;
   	
      /* clear interrupt registers */
      sid = pRM->sid1;
      inum = 0x0007 & pRM->irs;

      /* do a heartbeat */
      doHeartbeat ();
   }

#else 

   /* GPS board interrupt */
   static void defaultISR (void)
   {
      /* do a heartbeat */
      doHeartbeat ();
   }

#endif

#else

/* POSIX: not really an ISR, but a signal handler */

   static int		signalHandlerStatus = 0;
#ifndef OS_VXWORKS
   static taskID_t	hearbeatTID = 0;
#endif

#if !defined(AVOID_SIGNALS)
   static void defaultISR (int sig)
   {
      /* do a heartbeat */
      doHeartbeat();
      /* reinstall itself */
      signal (SIGheartbeat, defaultISR);
   }
#endif
#endif



/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: connectHeartbeatISR				*/
/*                                                         		*/
/* Procedure Description: connects the ISR				*/
/*                                                         		*/
/* Procedure Arguments: hbISR interrup service routine			*/
/*                                                         		*/
/* Procedure Returns: 0 when successful or error number when failed	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#if defined(OS_VXWORKS) && !defined(_USE_POSIX_TIMER)

#if GDS_SITE == GDS_SITE_MIT

   /* reflective memory */
   static int connectHeartbeatISR (void ISR (void))
   {
      /* connect ISR */
      if (intConnect((VOIDFUNCPTR*) INUM_TO_IVEC(RMAPI_INT_VEC),
         ISR, 0) != OK) {
         return -2;
      }	
      /* enable heartbeat interrupt */
      if (sysIntEnable(RMAPI_INT_LEVEL) != OK) {
         return -3;
      }
   
      /* Setup Interrupts for */
      /* Flag Auto Clear */
      /* Interrupt Enable */
      pRM->cr1 = 0x50 | RMAPI_INT_LEVEL;
      pRM->cr2 = 0x50 | RMAPI_INT_LEVEL;
      pRM->cr3 = 0x50 | RMAPI_INT_LEVEL;
   
      /* Set interrupt Vectors */
      pRM->vr1 = RMAPI_INT_VEC;
      pRM->vr2 = RMAPI_INT_VEC;
      pRM->vr3 = RMAPI_INT_VEC;
   
      /* extinguish the Fail LED */
      rmLED(0);
   
      /* reset latch sync loss bit */
      pRM->irs &= ~(0x40);
   
      return 0;
   }

#else

   /* GPS board interrupt */
   static int connectHeartbeatISR (void ISR (void))
   {
      
      return gpsHeartbeatInstall (_GPS_BOARD_ID, 
                                  (gpsISR_func) defaultISR);
   }

#endif

#else

   /* POSIX: creates a realtime timer and attaches it to a signal */ 

#if !defined(AVOID_SIGNALS)
   void* installSignal (void* noreturn)
   {
      static timer_t	hbtimer;
      struct sigevent	hbsig;
      struct itimerspec	tval;
      struct timespec	now;
   
      /* create a timer */
      hbsig.sigev_notify = SIGEV_SIGNAL;
      hbsig.sigev_signo = SIGheartbeat;
      hbsig.sigev_value.sival_int = 0;
      if (timer_create (CLOCK_REALTIME, &hbsig, &hbtimer) != 0) {
         signalHandlerStatus = -1;
         return NULL;
      }
   
      /* get current time */
      if (clock_gettime (CLOCK_REALTIME, &now) != 0) {
         signalHandlerStatus = -2;
         return NULL;
      }
   
      /* start timer synchronized with the next second */
      tval.it_value = now;
      if (now.tv_nsec > 700000000L) {
         /* too close to the next sec, go one further */
         tval.it_value.tv_sec += 2;
      }
      else {
         tval.it_value.tv_sec += 1;
      }
      tval.it_value.tv_nsec = 0; /* exactly on sec */ 
   
      /* set timer to heartbeat rate */
      tval.it_interval.tv_sec = 1 / NUMBER_OF_EPOCHS;
      tval.it_interval.tv_nsec = (1000000000UL / NUMBER_OF_EPOCHS) % 1000000000UL;
   
       /* connect signal handler */
      if (signal (SIGheartbeat, defaultISR) == SIG_ERR) {
         signalHandlerStatus = -3;
         return NULL;
      }
   
      /* arm timer */
      if (timer_settime (hbtimer, TIMER_ABSTIME, &tval, NULL) != 0) {
         signalHandlerStatus = -4;
         return NULL;
      }
   
      if ((int) noreturn) {
         sigset_t	set;
         int		sig;
      
         /* signal mask */
         if ((sigemptyset (&set) != 0) ||
            (sigaddset (&set, SIGheartbeat) != 0)) {
            signalHandlerStatus = -5;
            return NULL;
         }
      
         signalHandlerStatus = 1;
         while (1) {
            /* wait for heartbeat signal */
         #ifndef OS_VXWORKS
            sigwait (&set, &sig);
         #else
            sigwait (&set);
         #endif
            /* check if finished */
            if (signalHandlerStatus == 2) {
               signal (SIGheartbeat, SIG_IGN);
               return NULL;
            }
            /* do a heartbeat */
            doHeartbeat();
         }
      }
      return NULL;
   }
#else

#ifndef _TP_DAQD
   void* installSignal (void* noreturn)
   {

      if (0 == noreturn) {
         return NULL;
      }
      signalHandlerStatus = 1;
   #ifndef OS_VXWORKS
      pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
   #endif

      extern volatile unsigned int *ioMemDataCycle;
      extern volatile unsigned int *ioMemDataGPS;

      // Sync up to the master clock
      // Find memory buffer of first ADC to be used in secondary application.
      printf("waiting to sync %d\n", *ioMemDataCycle);
      //rdtscl(cpuClock[0]);
      // Spin until cycle 0 detected in first ADC buffer location.
      int spin_cnt = 0;
      do {
	spin_cnt++;
	if (spin_cnt >= 1000000000) {
	  //fprintf(stderr, "Timed out waiting for the IOP cycle\n");
	  //_exit(1);
	  sleep(10);
	  spin_cnt = 0;
	}
      } while (*ioMemDataCycle != 0);
      //rdtscl(cpuClock[1]);
      //cycleTime = (cpuClock[1] - cpuClock[0])/CPURATE;
      //printf("Synched %d\n",cycleTime);
      // Get GPS seconds from MASTER
      int timeSec = *ioMemDataGPS;
      int timeCycle = 0;
      printf("TimeSec=%d;  cycle=%d\n", timeSec, *ioMemDataCycle);

      while (1) {

         /* check if finished */
         if (signalHandlerStatus == 2) {
            return NULL;
         }
         /* do a heartbeat */
         doHeartbeat();

	 /*printf("AWG installSignal loop iteration\n"); */

	timeCycle += 4096; // Master cycle is at 65536 Hz
	timeCycle %= 65536;
	
	int sleep_cnt = 0;
        do {
           struct timespec wait = {0, 10000000UL }; // 10 milliseconds
           nanosleep (&wait, NULL);
	   //printf("nanosleeping...\n"); 
	   sleep_cnt++;
	   if (sleep_cnt >= 100) {
		fprintf(stderr, "IOP cycle timeout\n");
		_exit(1);
	   }
        } while(timeCycle?
		*ioMemDataCycle < timeCycle:
		*ioMemDataCycle > (65536 - 4096));

      //int timeSec = ioMemData->gpsSecond;
      //printf("TimeSec=%d; timeCycle=%d,  cycle=%d\n", timeSec, timeCycle, ioMemData->iodata[ll][0].cycle);
       // Send gps seconds to the front-end
       rmWrite (0, (char *)ioMemDataGPS, 0, 4, 0);

       // Read the current DAQ block size from the front-end
       rmRead (0, (char *)&curDaqBlockSize, 4, 4, 0);
       //printf("curDaqBlockSize=%d\n", curDaqBlockSize);
      }
   }
#endif
#endif

#ifndef _TP_DAQD
   static int connectHeartbeatISR (void ISR (void))
   {
      /* for Bajas make sure the realtime clock is intialized.
         :PATHETIC: the timer resolution is determiend by the
         system clock interrupt rate! To make it work properly,
         it has to be a multiple of the heartbeat rate. */
   #if defined (OS_VXWORKS)
      {
         int			clkRate;/* system clock rate */
         struct timespec	res;	/* posix resolution */	
      
         /* make sure the realtime clock is initialized */
         (void) TAInow();
   
         /* get current clock rate */
         clkRate = sysClkRateGet();
      	 /* round to the next multiple of heartbeat */
         clkRate = NUMBER_OF_EPOCHS * 
                   ((clkRate + NUMBER_OF_EPOCHS/2) / NUMBER_OF_EPOCHS);
         while (clkRate < SYS_CLK_RATE_MIN) {
            clkRate += NUMBER_OF_EPOCHS;
         }
         while (clkRate > SYS_CLK_RATE_MAX) {
            clkRate -= NUMBER_OF_EPOCHS;
         }
      	/* now set new rate; don't forget to inform the POSIX library */
         if (sysClkRateSet (clkRate) != 0) {
            return -100;
         }
         res.tv_sec = 0;
         res.tv_nsec = 1000000000UL / clkRate;
         clock_setres (CLOCK_REALTIME, &res);
      }
   #endif
   
      signalHandlerStatus = 0;
   
   #ifndef OS_VXWORKS
      {
         static int	once = 0;
         int		attr;
         struct timespec tick = {0, 1000000};	/* tick */	
      
      #if !defined(AVOID_SIGNALS)
         sigset_t	set;
         /* mask heartbeat signal */
         if ((sigemptyset (&set) != 0) ||
            (sigaddset (&set, SIGheartbeat) != 0) ||
            (pthread_sigmask (SIG_BLOCK, &set, NULL) != 0)) {
            return -101;
         }
      #endif
         /* install signal handler in separate task context */
         attr = PTHREAD_CREATE_DETACHED;
         if (taskCreate (attr, 90, &hearbeatTID, "tHB", installSignal, 
            (taskarg_t) 1) != 0) {
            return -102;
         }
         /* wait for signal installation */
         while (signalHandlerStatus == 0) {
            nanosleep (&tick, NULL);
         }
         if (signalHandlerStatus < 0) {
            return signalHandlerStatus;
         }
      
         /* support forking (timers are process specific), so make a new 
            one for each fork'ed child; but only the first time around! */
         if (once == 0) {
            pthread_atfork (NULL, NULL, 
                           (void (*) (void)) connectHeartbeatISR);
         }
         once++;
         return 0;
      }
   #else
      /* install signal handler */
      installSignal (0);
      return signalHandlerStatus;
   #endif
   }
#endif

#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: getHeartbeatCount				*/
/*                                                         		*/
/* Procedure Description: returns the number of heartbeats		*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: number of heartbeats since heartbeat was installed*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   unsigned long getHeartbeatCount (void)
   {
      return heartbeatCount;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: initHeartbeat				*/
/*                                                         		*/
/* Procedure Description: initilializes heartbeats			*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void initHeartbeat (void) 
   {
   #if !defined(OS_VXWORKS) && !defined(AVOID_SIGNALS)
      sigset_t	set;
      if ((sigemptyset (&set) != 0) ||
         (sigaddset (&set, SIGheartbeat) != 0) ||
         (pthread_sigmask (SIG_BLOCK, &set, NULL) != 0)) {
         return;
      }
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: finiHeartbeat				*/
/*                                                         		*/
/* Procedure Description: terminates the heartbeat			*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void finiHeartbeat (void)
   {
   #if !defined(OS_VXWORKS) || defined(_USE_POSIX_TIMER)
      signalHandlerStatus = 2;
   #if !defined(AVOID_SIGNALS)
      signal (SIGheartbeat, SIG_IGN);
   #endif
   #ifndef OS_VXWORKS
      taskCancel (&hearbeatTID);
   #endif
   #endif
   }
