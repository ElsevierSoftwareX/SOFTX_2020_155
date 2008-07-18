program %SEQUENCER_NAME%

#define SEQUENCER_NAME_%SEQUENCER_NAME%

%% #include <stdio.h>
%% #include <stdlib.h>
%% #include <string.h>
%% #include <math.h>
%% #include <unistd.h>
%% #include <pthread.h>
%% #include <fpvalidate.h>
%% #include <daqmap.h>
%% #include <param.h>
%% #include <alarm.h>                                                    /* MA */

%% #define MAX_MODULES 1
%% #include "fm10Gen.h"
%% #include "feComms.h"

#if defined(RFM_EPICS)
%% #define rfm_assign(a,b) if ((a) != (b)) { a = (b); }
#else
%% #define rfm_assign(a,b) ((a)=(b))
#endif

%DECL1%

int setpointStatusCount;                                                 /* MA */
assign setpointStatusCount to                                            /* MA */
    "{ifo}:{sys}-{subsys}SETPOINT_STATUS_COUNT";                         /* MA */
 
string setpointErrId[10];                                                /* MA */
assign setpointErrId to {                                                /* MA */
   "{ifo}:{sys}-{subsys}SETPOINT_ID_1",                                  /* MA */
   "{ifo}:{sys}-{subsys}SETPOINT_ID_2",                                  /* MA */
   "{ifo}:{sys}-{subsys}SETPOINT_ID_3",                                  /* MA */
   "{ifo}:{sys}-{subsys}SETPOINT_ID_4",                                  /* MA */
   "{ifo}:{sys}-{subsys}SETPOINT_ID_5",                                  /* MA */
   "{ifo}:{sys}-{subsys}SETPOINT_ID_6",                                  /* MA */
   "{ifo}:{sys}-{subsys}SETPOINT_ID_7",                                  /* MA */
   "{ifo}:{sys}-{subsys}SETPOINT_ID_8",                                  /* MA */
   "{ifo}:{sys}-{subsys}SETPOINT_ID_9",                                  /* MA */
   "{ifo}:{sys}-{subsys}SETPOINT_ID_10"};                                /* MA */

string setpointMajorCount[10];                                           /* MA */
assign setpointMajorCount to {                                           /* MA */
   "{ifo}:{sys}-{subsys}SETPOINT_MAJOR_COUNT_1",                         /* MA */
   "{ifo}:{sys}-{subsys}SETPOINT_MAJOR_COUNT_2",                         /* MA */
   "{ifo}:{sys}-{subsys}SETPOINT_MAJOR_COUNT_3",                         /* MA */
   "{ifo}:{sys}-{subsys}SETPOINT_MAJOR_COUNT_4",                         /* MA */
   "{ifo}:{sys}-{subsys}SETPOINT_MAJOR_COUNT_5",                         /* MA */
   "{ifo}:{sys}-{subsys}SETPOINT_MAJOR_COUNT_6",                         /* MA */
   "{ifo}:{sys}-{subsys}SETPOINT_MAJOR_COUNT_7",                         /* MA */
   "{ifo}:{sys}-{subsys}SETPOINT_MAJOR_COUNT_8",                         /* MA */
   "{ifo}:{sys}-{subsys}SETPOINT_MAJOR_COUNT_9",                         /* MA */
   "{ifo}:{sys}-{subsys}SETPOINT_MAJOR_COUNT_10"};                       /* MA */
 
string setpointMinorCount[10];                                           /* MA */
assign setpointMinorCount to {                                           /* MA */
   "{ifo}:{sys}-{subsys}SETPOINT_MINOR_COUNT_1",                         /* MA */
   "{ifo}:{sys}-{subsys}SETPOINT_MINOR_COUNT_2",                         /* MA */
   "{ifo}:{sys}-{subsys}SETPOINT_MINOR_COUNT_3",                         /* MA */
   "{ifo}:{sys}-{subsys}SETPOINT_MINOR_COUNT_4",                         /* MA */
   "{ifo}:{sys}-{subsys}SETPOINT_MINOR_COUNT_5",                         /* MA */
   "{ifo}:{sys}-{subsys}SETPOINT_MINOR_COUNT_6",                         /* MA */
   "{ifo}:{sys}-{subsys}SETPOINT_MINOR_COUNT_7",                         /* MA */
   "{ifo}:{sys}-{subsys}SETPOINT_MINOR_COUNT_8",                         /* MA */
   "{ifo}:{sys}-{subsys}SETPOINT_MINOR_COUNT_9",                         /* MA */
   "{ifo}:{sys}-{subsys}SETPOINT_MINOR_COUNT_10"};                       /* MA */

%% static const float conv = 0.000152588;
%% extern pthread_mutex_t seq_lock;

#if defined(RFM_EPICS)
%% static RFM_FE_COMMS *pRfm;
%% static %EPICS_TYPE% *pEpics;
%% extern void *findRfmCard(int);
#else
%% static %EPICS_TYPE% pEpicsSpace;
%% static %EPICS_TYPE% *pEpics = &pEpicsSpace;
#endif

%% static void *base;
%% static char site[160];
%% static char ifo[160];

ss monitorFilt{
 
state init
{
  when() {
%% {
%% int ii, jj;

#if defined(RFM_EPICS)
%%  pRfm = (RFM_FE_COMMS *)(findRfmCard(0));
%%  pEpics = (%EPICS_TYPE%*)(&pRfm->%EPICS_EPICS_VAR%);
%%  printf("pEpics is at 0x%x\n", ((int)pEpics-(int)pRfm));
#endif

%DECL3%

%% }
  } state monScreen
}

state monScreen{
  when(delay(.2)){
%% int ii, jj;	
%% pthread_mutex_lock (&seq_lock);

%%  setpointStatusCount = 0;                                             /* MA */
%%  int setpointDisplayCount = 0;                                        /* MA */

%%  for (ii = 0; ii < 10; ii++) {                                        /* MA */
%%     strcpy(setpointErrId[ii], "CLR");                                 /* MA */
%%     strcpy(setpointMajorCount[ii], "CLR");                            /* MA */
%%     strcpy(setpointMinorCount[ii], "CLR");                            /* MA */
%%  }                                                                    /* MA */

%DECL4%

      pvPut(setpointStatusCount);                                        /* MA */
      for (ii = 0; ii < 10; ii++) {                                      /* MA */
         pvPut(setpointErrId[ii]);                                       /* MA */
         pvPut(setpointMajorCount[ii]);                                  /* MA */
         pvPut(setpointMinorCount[ii]);                                  /* MA */
      }                                                                  /* MA */

%% pthread_mutex_unlock (&seq_lock);

  }state monScreen
}

}
