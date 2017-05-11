program suswd

%% #define UINT32 unsigned int
%% #define UINT16 unsigned short
%% #define OK 1
%% #define ERROR 0
%% #define FALSE 0
%% #define TRUE 1
%%
%% #include <errno.h>
%% #include <unistd.h>
%% #include <epicsTime.h>
%% #include <cadef.h>
%% #include <ezca.h>

%% #define MAX_MODULES 1
#define MAX_MODULES 1

%% #include "fm10Gen.h"
%% #include <feComms.h>
%% #include <sus.h>
%% #include <stdio.h>
%% #include <stdlib.h>
%% #include <string.h>
%% #include <math.h>
%% #include <pthread.h>
%% #include <fpvalidate.h>

%% #define rfm_assign(a,b) if ((a) != (b)) { a = (b); }

/* WD Reset requests from operator. */
int wdSwitch;
assign wdSwitch to "{ifo}:{sys}-Q1_WD_SW";


%% static volatile CDS_EPICS *pEpics;		/* Pointer to HEPI epics in SHM. */
%% static volatile RFM_FE_COMMS *pRfm;		/* Pointer to epicsShm 		*/
%% static int ii, jj;
%%      extern void *findRfmCard(unsigned int);

ss susWD{

state init
{
  when() {
	/* Init pointer to HEPI EPICS in RFM. */
	%%  pRfm = (RFM_FE_COMMS *)(findRfmCard(0));
	%%  pEpics = (CDS_EPICS *)(&pRfm->epicsSpace);

    /* Clear WD variables on startup. */

  }state monWD
}
	/* end state init */

/* Perform WD monitoring */
state monWD{

  when(delay(.2)){

    /* Process operator requests for WD reset, Loop on/off, master on/off. */
%%      ii = pEpics->sus.Q1_WD;
%%	if(ii == 0) {
		wdSwitch = 0;	
		pvPut(wdSwitch);
%%		{
%%			short s[1];
%%			s[0] = 0;
%%  			ezcaPut("M1:SEI-BSC_ACT_SW", ezcaShort, 1, s);
%%		}
%%	}

  }state monWD
}	/* end state monWD */
}	/* end monitor WD  */

