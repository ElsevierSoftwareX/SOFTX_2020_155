program hepiWatchdogHMY

%% #define UINT32 unsigned int
%% #define UINT16 unsigned short
%% #define OK 1
%% #define ERROR 0
%% #define FALSE 0
%% #define TRUE 1
%%
%% #include <errno.h>
%% #include <unistd.h>

%% #define MAX_MODULES 1
#define MAX_MODULES 1

%% #include "fm10Gen.h"
%% #include <feComms.h>
%% #include <hepi.h>
%% #include <stdio.h>
%% #include <stdlib.h>
%% #include <string.h>
%% #include <math.h>
%% #include <pthread.h>
%% #include <fpvalidate.h>

%% extern pthread_mutex_t seq_lock;

%% #define rfm_assign(a,b) if ((a) != (b)) { a = (b); }
%% #define HEPI_OK		0
%% #define HEPI_TRIP_RAMP	1
%% #define HEPI_TRIP_STS_OFF	2
%% #define HEPI_TRIP_LOOP_OFF	3
%% #define HEPI_TRIP_LOOP_RESET	4
%% #define HEPI_TRIP_MASTER_OFF	5
%% #define HEPI_TRIP_RESET	6
%% #define HEPI_TRIP_CHM	7

/* WD Reset requests from operator. */
int wdReset;
assign wdReset to "{ifo}:{sys}-{subsys}_WD_M_RESET";

/* WD status to operator screens. */
int wdState;
assign wdState to "{ifo}:{sys}-{subsys}_WD_M_STATUS";

/* STS_X Output filter ramp times. */
float trampX;
assign trampX to "{ifo}:{sys}-{subsys}_FIR_X_OUT_TRAMP";

/* STS_Y Output filter ramp times. */
float trampY;
assign trampY to "{ifo}:{sys}-{subsys}_FIR_Y_OUT_TRAMP";

/* STS_X Output filter gains. */
float gainX;
assign gainX "{ifo}:{sys}-{subsys}_FIR_X_OUT_GAIN";

/* STS_Y Output filter gains. */
float gainY;
assign gainY to "{ifo}:{sys}-{subsys}_FIR_Y_OUT_GAIN";

/* STS_X Output filter on/off switches. */
int swX;
assign swX to "{ifo}:{sys}-{subsys}_FIR_X_OUT_SW2";

/* STS_Y Output filter on/off switches. */
int swY;
assign swY to "{ifo}:{sys}-{subsys}_FIR_Y_OUT_SW2";

/* STS_X Output filter on/off switch readbacks. */
int swXr;
assign swXr to "{ifo}:{sys}-{subsys}_FIR_X_OUT_SW2R";

/* STS_Y Output filter on/off switch readbacks. */
int swYr;
assign swYr to "{ifo}:{sys}-{subsys}_FIR_Y_OUT_SW2R";

/* Loop output on/off switch settings. */
int modalSw;
assign modalSw to "{ifo}:{sys}-{subsys}_LOOP_SW";

/* Master output on/off switch settings. */
int masterSw;
assign masterSw to "{ifo}:{sys}-{subsys}_ACT_SW";

/* Master output on/off switch settings. */
int tiltSw;
assign tiltSw to "{ifo}:{sys}-{subsys}_TILTCORR_SW";

%% static volatile CDS_EPICS *pEpics;		/* Pointer to HEPI epics in SHM. */
%% static volatile RFM_FE_COMMS *pRfm;		/* Pointer to epicsShm 		*/
%% static int ii, jj;
%% static int tripState;			/* Holds WD trip status.	*/
%% static int tripMonitor;			/* WD status to operator display.*/
%% static int tripCounter;			/* Counter for timing transision between
						   WD fault states.			*/
%% static float gainXhold,gainYhold;	/* Saved state of STS out gains. 	*/
%% static float trampHoldX,trampHoldY;	/* Saved state of STS out ramp times. 	*/
%% static int tripped;
%% static int susMonitor;
%%      extern void *findRfmCard(unsigned int);

ss monitorWD{

state init
{
  when() {
	/* Init pointer to HEPI EPICS in RFM. */
	%%  pRfm = (RFM_FE_COMMS *)(findRfmCard(0));
	%%  pEpics = (CDS_EPICS *)(&pRfm->epicsSpace);

    /* Clear WD variables on startup. */
        wdReset = 0;
        pvPut(wdReset);
        wdState = 0;
        pvPut(wdState);
%%	tripState = 0;
%%	tripMonitor = 0;
%%	tripCounter = 0;
%%	tripped = 0;

  }state monWD
}
	/* end state init */

/* Perform WD monitoring */
state monWD{

  when(delay(.2)){

    /* Process operator requests for WD reset, Loop on/off, master on/off. */
%%      pthread_mutex_lock (&seq_lock);
        pvGet(wdReset);
%%      pthread_mutex_unlock (&seq_lock);
%%      if(wdReset == 1) 
%%	{
		/* Send WD reset to front end. */
%%              pEpics->sei.HMY_WD_M.trip = 0;
		/* Indicate WD being reset */
%%		tripMonitor = 32;
%%		wdState = tripMonitor;
		pvPut(wdState);
		/* Clear the reset command. */
                wdReset = 0;
                pvPut(wdReset);

		/* Put back the sts output filter gains and ramp times. */
%%		if(tripped == 1)
%%		{
%%			gainX = gainXhold;
%%			gainY = gainYhold;
%%			trampX = trampHoldX;
%%			trampY = trampHoldY;
			pvPut(gainX);
			pvPut(gainY);
			pvPut(trampX);
			pvPut(trampY);
%%			tripped = 0;
%%      	}

		/* Allow time for front end to reset its fault counters. */
%%		usleep(2000000);

		/* Reset trip counters and WD state. */
%%		tripState = HEPI_OK;
%%		tripMonitor = 0;
%%		tripCounter = 0;
%%      }

	/* Read in Loop On/Off Request */
%%      pthread_mutex_lock (&seq_lock);
	pvGet(modalSw);
%%      pthread_mutex_unlock (&seq_lock);
	/* If WD at or past LOOP_OFF state, clear the request */
%%	if(modalSw && (tripState >= HEPI_TRIP_LOOP_OFF))
%%	{
		modalSw = 0;
		pvPut(modalSw);
%%	}

	/* Read master on/off switch request. */
%%      pthread_mutex_lock (&seq_lock);
	pvGet(masterSw);
%%      pthread_mutex_unlock (&seq_lock);

	/* If WD has tripped off master switch, clear any ON requests. */
%%	if(masterSw && (tripState == HEPI_TRIP_MASTER_OFF))
%%	{
		masterSw = 0;
		pvPut(masterSw);
%%	}

    /* Process WD monitoring and actions. *************************************	*/
	/* If this is first detected fault, not due to CHM, ...  */
%%	if((pEpics->sei.HMY_WD_M.trip) && (tripState == HEPI_OK))
%%	{

%%		tripState = HEPI_TRIP_RAMP;
%%		tripMonitor |= 1;
%%		tripped = 1;
		/* Get present sts output filter ramp time and gain settings.
		   These will be put back on WD reset.				*/
%%      pthread_mutex_lock (&seq_lock);
		pvGet(gainX);
		pvGet(gainY);
		pvGet(trampX);
		pvGet(trampY);
%%      pthread_mutex_unlock (&seq_lock);
%%		gainXhold = gainX;
%%		gainYhold = gainY;
%%		trampHoldX = trampX;
%%		trampHoldY = trampY;

		/* Set sts output filter ramp times to 10 seconds. */
		trampX = 10;
		trampY = 10;
		pvPut(trampX);
		pvPut(trampY);
		/* Allow time for ramp times to get to front end.  */
%%		usleep(1000000);
		/* Load gain settings to start sts output filter ramping. */
		gainX = 0.0;
		gainY = 0.0;
		pvPut(gainX);
		pvPut(gainY);
%%	}
%%	if(tripState != 0) tripCounter ++;

	/* STS output filters should have ramped down by now, so turn
	   off the output filters. */
%%	if((tripState == HEPI_TRIP_RAMP) && (tripCounter > 50))
%%	{
%%      pthread_mutex_lock (&seq_lock);
		pvGet(swXr);
		pvGet(swYr);
%%      pthread_mutex_unlock (&seq_lock);
%%		swX = (swXr & 0x400);
		pvPut(swX);
%%		swY = (swYr & 0x400);
		pvPut(swY);
%%		tripState = HEPI_TRIP_STS_OFF;
%%		tripMonitor |= 2;
%%		tripCounter = 0;
%%              pEpics->sei.HMY_WD_M.trip = 0;
%%	}

	/* Next fault will turn off loop output */
%%	if((tripState == HEPI_TRIP_STS_OFF) && (tripCounter > 50))
%%	{
		/* Fault detected */
%%		if(pEpics->sei.HMY_WD_M.trip)
%%		{
%%			tripState = HEPI_TRIP_LOOP_OFF;
%%			tripMonitor |= 4;
			modalSw = 0;
			pvPut(modalSw);
%%			tripCounter = 0;
%%		}
		/* No fault. Check again next cycle thru loop. */
%%		else tripCounter = 50;
%%	}

	/* After shutting off loop output, wait for a time period, then
	   reset front end faults. */
%%	if((tripState == HEPI_TRIP_LOOP_OFF) && (tripCounter > 50))
%%	{
%%              pEpics->sei.HMY_WD_M.trip = 0;
%%		tripCounter = 40;
%%		tripState = HEPI_TRIP_LOOP_RESET;
%%	}

	/* Next fault condition detected will trip master output switch. */
%%	if((tripState == HEPI_TRIP_LOOP_RESET) && (tripCounter > 50))
%%	{
		/* Fault detected, trip off master switch */
%%		if(pEpics->sei.HMY_WD_M.trip)
%%		{
%%			tripState = HEPI_TRIP_MASTER_OFF;
%%			tripMonitor |= 8;
			masterSw = 0;
			pvPut(masterSw);
%%			tripCounter = 0;
%%		}
		/* No fault detected, check again next time thru loop. */
%%		else tripCounter = 50;
%%	}

	/* If WD has tripped master or due to channel hop, stay in
	   present state.						*/
%%	if(tripState == HEPI_TRIP_MASTER_OFF) tripCounter = 0;


	/* Write WD status to EPICS records */
%%	wdState = tripMonitor;
	pvPut(wdState);

/* ****************************************************************************	*/
/* Check for Suspension watchdog trips.						*/
%%      pthread_mutex_lock (&seq_lock);
	pvGet(masterSw);
%%      pthread_mutex_unlock (&seq_lock);
%%	if(masterSw == 0)
%%	{
		tiltSw = 0;
		pvPut(tiltSw);
%%	}

  }state monWD
}	/* end state monWD */
}	/* end monitor WD  */

