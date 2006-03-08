program %SEQUENCER_NAME%

#define SEQUENCER_NAME_%SEQUENCER_NAME%

%% #include <stdio.h>
%% #include <stdlib.h>
%% #include <string.h>
%% #include <math.h>
%% #include <unistd.h>
%% #include <fpvalidate.h>
%% #include <daqmap.h>
%% #include <param.h>
%% #define MAX_MODULES 1
%% #include "fm10Gen.h"
%% #include "feComms.h"

#if defined(RFM_EPICS)
%% #define rfm_assign(a,b) if ((a) != (b)) { a = (b); }
#else
%% #define rfm_assign(a,b) ((a)=(b))
#endif

%DECL1%

%% static const float conv = 0.000152588;

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

%DECL4%

  }state monScreen
}

}
