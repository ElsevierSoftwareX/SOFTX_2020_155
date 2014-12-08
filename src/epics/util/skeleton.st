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
%% #include <errno.h>
%% #include <tsDefs.h>
%% #include <cadef.h>
%% #include <ezca.h>
%% #include <alarm.h>                                                    /* MA */
%% #include <sys/types.h>                                                /* MA */
%% #include <sys/stat.h>                                                 /* MA */
%% #include <ctype.h>

#if defined(RFM_EPICS)
%% #define SERVO16K
#else
%% #define SERVO5HZ
#endif

%% #define MAX_MODULES %FMNUM%
#define MAX_MODULES %FMNUM%
#define MAX_MODULES_TIMES_TWO %FMNUMx2%
#define MAX_MODULES_TIMES_TEN %FMNUMx10%


%% #include "fm10Gen.h"
%% #define FM_SUBSYS_NUM  1
%% #include "fmReadCoeff.h"
%% #include "feComms.h"
%% #include "findSharedMemory.h"
%% #define FE_ERROR_CFC 0x400

/* Cat string and make upper case */                                     /* MA */
%% static char *strcat_upper(char *dest, char *src) {                    /* MA */
%%   char *d = dest;                                                     /* MA */
%%   for( ; *d; d++);                                                    /* MA */
%%   for( ; (*d++ = toupper(*src)); src++);                              /* MA */
%%   return dest;                                                        /* MA */
%% }                                                                     /* MA */
                                                                         /* MA */
/* Cat string and make lower case */                                     /* MA */
%% static char *strcat_lower(char *dest, char *src) {                    /* MA */
%%   char *d = dest;                                                     /* MA */
%%   for( ; *d; d++);                                                    /* MA */
%%   for( ; (*d++ = tolower(*src)); src++);                              /* MA */
%%   return dest;                                                        /* MA */
%% }                                                                     /* MA */

#if defined(RFM_EPICS)
%% #define rfm_assign(a,b) if ((a) != (b)) { a = (b); }
#else
%% #define rfm_assign(a,b) ((a)=(b))
#endif

#if defined(SEQUENCER_NAME_tcsh1) || defined(SEQUENCER_NAME_tcsh2)
%% #include "tcs.h"
#endif

%DECL1%

#if defined(RFM_EPICS)
%% static volatile RFM_FE_COMMS *pRfm;
%% static volatile VME_COEF *pVmeCoeff;
%% static volatile FILT_MOD *pFilt;
%% static %EPICS_TYPE% *pEpics;
%% static unsigned int sysnum;
%% extern void *findRfmCard(int);
#else
%% static VME_COEF vmeCoeffSpace;
%% static COEF coeffSpace;
%% static FILT_MOD filtSpace;
%% static VME_COEF *pVmeCoeff = &vmeCoeffSpace;
%% static FILT_MOD *pFilt = &filtSpace;
%% static %EPICS_TYPE% pEpicsSpace;
%% static %EPICS_TYPE% *pEpics = &pEpicsSpace;
#endif

string msg;
assign msg to "{ifo}:FEC-{sysnum}_MSG";

string msgDAQ;
assign msgDAQ to "{ifo}:FEC-{sysnum}_MSGDAQ";

string svnString;
assign svnString to "{ifo}:FEC-{sysnum}_BUILD_SVN";

int stateWord;
assign stateWord to "{ifo}:FEC-{sysnum}_STATE_WORD";

double epicsSyncTime;
assign epicsSyncTime to "{ifo}:FEC-{sysnum}_EPICS_SYNC_TIME";

int epicsWait;
assign epicsWait to "{ifo}:FEC-{sysnum}_EPICS_WAIT";

int allNewCoeff;
assign allNewCoeff to "{ifo}:FEC-{sysnum}_LOAD_NEW_COEFF";

string localtimestring;
assign localtimestring to "{ifo}:FEC-{sysnum}_TIME_STRING";

#ifndef NO_DAQ_IN_SKELETON

int reload;
assign reload to "{ifo}:DAQ-FEC_{sysnum}_LOAD_CONFIG";

int dcmsg;
assign dcmsg to "{ifo}:DAQ-FEC_{sysnum}_MSG";

int chnCnt;
assign chnCnt to "{ifo}:DAQ-FEC_{sysnum}_CHAN_CNT";

int epicschnCnt;
assign epicschnCnt to "{ifo}:DAQ-FEC_{sysnum}_EPICS_CHAN_CNT";

int dataSize;
assign dataSize to "{ifo}:DAQ-FEC_{sysnum}_TOTAL";

int dcuId;
assign dcuId to "{ifo}:DAQ-FEC_{sysnum}_DCU_ID";

#endif


unsigned int prev_gps = 0;
unsigned int prev_cycle = 0;
%% /* ezCaWrites we want to update slower */
unsigned int pvUpdateCycle = 0;
int prev_seqTrig = 0;


%DECL2%

%% static fmReadCoeff fmc = {
%%  "lho", "h1", "xxx", 0, {{"", "", MAX_MODULES, fmmap0, 0}},
%% };

%% static const double conv = 0.000152588;
%% static const int mask[2] = {0xAAA0,0x30AA};
%% extern pthread_mutex_t seq_lock;

#if defined(SEQUENCER_NAME_hepi) || defined(SEQUENCER_NAME_hepia)
%% static unsigned int cpunum;
#endif

#ifdef SEQUENCER_NAME_asc
#define FILT_QUAD_SIZE 10
int filt[FILT_QUAD_SIZE];
assign filt to
{"{ifo}:{sys}-WFS1_I_QUAD","{ifo}:{sys}-WFS1_Q_QUAD","{ifo}:{sys}-WFS2_I_QUAD",
 "{ifo}:{sys}-WFS2_Q_QUAD","{ifo}:{sys}-WFS3_I_QUAD","{ifo}:{sys}-WFS3_Q_QUAD",
 "{ifo}:{sys}-WFS4_I_QUAD","{ifo}:{sys}-WFS4_Q_QUAD","{ifo}:{sys}-WFS5_I_QUAD",
 "{ifo}:{sys}-WFS5_Q_QUAD"};
#endif

#ifdef SEQUENCER_NAME_asc40m
#define FILT_QUAD_SIZE 12
int filt[FILT_QUAD_SIZE];
assign filt to
{"{ifo}:{sys}-WFS1_I_QUAD","{ifo}:{sys}-WFS1_Q_QUAD","{ifo}:{sys}-WFS2_I_QUAD",
 "{ifo}:{sys}-WFS2_Q_QUAD","{ifo}:{sys}-WFS3_I_QUAD","{ifo}:{sys}-WFS3_Q_QUAD",
 "{ifo}:{sys}-WFS4_I_QUAD","{ifo}:{sys}-WFS4_Q_QUAD","{ifo}:{sys}-WFS5_I_QUAD",
 "{ifo}:{sys}-WFS5_Q_QUAD","{ifo}:{sys}-WFS6_I_QUAD","{ifo}:{sys}-WFS6_Q_QUAD"};
#endif

#ifdef SEQUENCER_NAME_ascmc
#define FILT_QUAD_SIZE 4
int filt[FILT_QUAD_SIZE];
assign filt to
{"{ifo}:{sys}-WFS1_I_QUAD","{ifo}:{sys}-WFS1_Q_QUAD","{ifo}:{sys}-WFS2_I_QUAD",
 "{ifo}:{sys}-WFS2_Q_QUAD"};
#endif

#ifdef SEQUENCER_NAME_lsc_removed
int lscModeSwitch;
assign lscModeSwitch to "{ifo}:LSC-FE_MODE";

int laState;
assign laState to "{ifo}:LSC-LA_State_Bits_Read";

%% static int lastLscModeSwitch;

%% static int coeffReset = 0;

#endif

#if defined(SEQUENCER_NAME_tcsh1) || defined(SEQUENCER_NAME_tcsh2)

double tcs1Input;
assign tcs1Input to "{ifo}:ASC-WFS5_PIT_OUT16"; 
/*assign tcs1Input to "{ifo}:LSC-LA_SPOB_NORM";*/
double tcs2Input;
assign tcs2Input to "{ifo}:LSC-ASI_CORR_OUT16";
double  inpt;
double  outpts[4];

double tcsXOutput;
double tcsYOutput;


#if defined(SEQUENCER_NAME_tcsh2)
assign tcsXOutput to "{ifo}:TCS-ITMX_AOMDRV";
assign tcsYOutput to "{ifo}:TCS-ITMY_AOMDRV";
#else
assign tcsXOutput to "{ifo}:TCS-ITMX_PWRSET";
assign tcsYOutput to "{ifo}:TCS-ITMY_PWRSET";
#endif

int tcsEnable;
assign tcsEnable to "{ifo}:TCS-SERVO_EN";
#endif

#define copy_labels(ii)\
       strcpy(labels0[ii],(const char *)(pVmeCoeff->vmeCoeffs[ii].filtName[0]));\
       strcpy(labels1[ii],(const char *)(pVmeCoeff->vmeCoeffs[ii].filtName[1]));\
       strcpy(labels2[ii],(const char *)(pVmeCoeff->vmeCoeffs[ii].filtName[2]));\
       strcpy(labels3[ii],(const char *)(pVmeCoeff->vmeCoeffs[ii].filtName[3]));\
       strcpy(labels4[ii],(const char *)(pVmeCoeff->vmeCoeffs[ii].filtName[4]));\
       strcpy(labels5[ii],(const char *)(pVmeCoeff->vmeCoeffs[ii].filtName[5]));\
       strcpy(labels6[ii],(const char *)(pVmeCoeff->vmeCoeffs[ii].filtName[6]));\
       strcpy(labels7[ii],(const char *)(pVmeCoeff->vmeCoeffs[ii].filtName[7]));\
       strcpy(labels8[ii],(const char *)(pVmeCoeff->vmeCoeffs[ii].filtName[8]));\
       strcpy(labels9[ii],(const char *)(pVmeCoeff->vmeCoeffs[ii].filtName[9]));
#define pvput_labels(ii)\
       pvPut(labels0[ii]);\
       pvPut(labels1[ii]);\
       pvPut(labels2[ii]);\
       pvPut(labels3[ii]);\
       pvPut(labels4[ii]);\
       pvPut(labels5[ii]);\
       pvPut(labels6[ii]);\
       pvPut(labels7[ii]);\
       pvPut(labels8[ii]);\
       pvPut(labels9[ii]);


%% static DAQ_INFO_BLOCK *info;
%% static volatile void *base;
%% static DAQ_INFO_BLOCK infoTmp;
%% static char site[160];
%% static char ifo[160];
%% static char daqsys[160];

%% char fName[2][256];                                                   /* MA */
%% char fType[3][10] = {"IIR", "FIR", "IIR & FIR"};                      /* MA */
%% char fNameDAQ[256];                                                   /* ma */

#ifdef FIR_FILTERS                                                       /* MA */
%% int fltrTypes = 2;                                                    /* MA */
#else                                                                    /* MA */
%% int fltrTypes = 1;                                                    /* MA */
#endif                                                                   /* MA */

/* %% struct filterThresh thresholds[MAX_MODULES];                          MA */


%% static void copy_info() {
%%   int i;
%%   for (i = 0; i < infoTmp.numChans; i++) {
%%     info -> tp [i].tpnum = infoTmp.tp [i].tpnum;
%%     info -> tp [i].dataType = infoTmp.tp [i].dataType;
%%     info -> tp [i].dataRate = infoTmp.tp [i].dataRate;
%%     info -> tp [i].dataGain = infoTmp.tp [i].dataGain;
%%   }
%%   info -> configFileCRC = infoTmp.configFileCRC;
%%   info->numChans = infoTmp.numChans;
%%   info->numEpicsInts = infoTmp.numEpicsInts;
%%   info->numEpicsFloats = infoTmp.numEpicsFloats;
%%   info->numEpicsFilts = infoTmp.numEpicsFilts;
%%   info->numEpicsTotal = infoTmp.numEpicsTotal;
%% }


ss monitorFilt{
 
state init
{
  when() {
%% {
%% int ii, jj;
%% int ij;
%% time_t lt = time(0);
%% strcpy(localtimestring, ctime(&lt));
%% localtimestring[strlen(localtimestring) - 1] = 0; /* Delete newline character */
   pvPut(localtimestring);

#if defined(RFM_EPICS)
    sysnum = atoi(macValueGet("sysnum"));

#if defined(SEQUENCER_NAME_hepi) || defined(SEQUENCER_NAME_hepia)
    cpunum = 0;
%%  if(sysnum > 3) cpunum = 1;
%%  if(sysnum == 7) cpunum = 2;
%%  if(sysnum == 8) cpunum = 3;
    strncpy(fmc.system, macValueGet("sys"), sizeof(fmc.system)-1);
#endif

#endif
	strcpy(svnString,BUILD_SVN_VERSION_NO);
	pvPut(svnString);
%%  printf("svn is %s\n", svnString);

#if defined(SEQUENCER_NAME_tcsh1) || defined(SEQUENCER_NAME_tcsh2)
    tcsEnable = 0;
#endif

    strncpy(fmc.site, macValueGet("site"), sizeof(fmc.site)-1);
    strncpy(fmc.ifo, macValueGet("ifo"), sizeof(fmc.ifo)-1);
    strncpy(fmc.system, macValueGet("sysfile"), sizeof(fmc.system)-1);
    if (macValueGet("sysfxxx") != 0) {
      strncpy(fmc.subSys[0].name, macValueGet("sysfile"), sizeof(fmc.subSys[0].name)-1);
    } else {
      fmc.subSys[0].name[0] = 0;
    }
%%  {
%%    int l = strlen(fmc.system);
%%    if (l) {
%%	if (fmc.system[l-1] == '-' || fmc.system[l-1] == '_') {
%%	  fmc.system[l-1] = 0;
%%	}
%%    }
%%  }
%%  {
%%    int l = strlen(fmc.subSys[0].name);
%%    if (l) {
%%	if (fmc.subSys[0].name[l-1] == '-' || fmc.subSys[0].name[l-1] == '_') {
%%	  fmc.subSys[0].name[l-1] = 0;
%%	}
%%    }
%%  }

#if defined(RFM_EPICS)
%%  printf("Init Sequencer %s; system %d\n", fmc.subSys[0].name, sysnum);
%%  findSharedMemory(fmc.system);
%%  pRfm = (RFM_FE_COMMS *)(findRfmCard(0));
%%  pFilt = (FILT_MOD *)(&pRfm->%EPICS_FILT_VAR%);
%%  pVmeCoeff = (VME_COEF*)(&pRfm->%EPICS_COEFF_VAR%);
%%  pEpics = (%EPICS_TYPE%*)(&pRfm->%EPICS_EPICS_VAR%);
%%  /* Zero the Epics structure out */
%%  memset(pEpics, 0, sizeof(%EPICS_TYPE%));
%%  printf("%s pFilt is at 0x%lx\n", fmc.subSys[0].name, ((unsigned long)pFilt-(unsigned long)pRfm));
%%  printf("%s pVmeCoeff is at 0x%lx\n", fmc.subSys[0].name, ((unsigned long)pVmeCoeff-(unsigned long)pRfm));
%%  printf("%s pVmeCoeff size is 0x%lx\n", fmc.subSys[0].name, sizeof(*pVmeCoeff));
%%  printf("%s pEpics is at 0x%lx\n", fmc.subSys[0].name, ((unsigned long)pEpics-(unsigned long)pRfm));
%%  printf("%s pEpics is at 0x%lx\n", fmc.subSys[0].name, ((unsigned long)pRfm));
#endif

%% strcpy(fName[0], "/opt/rtcds/");                                        /* MA */
%% strcat_lower(fName[0], fmc.site);                                     /* MA */
%% strcat(fName[0], "/");                                          /* MA */
%% strcat_lower(fName[0], fmc.ifo);                                     /* MA */
%% strcat(fName[0], "/chans/");                                          /* MA */
%% strcat_upper(fName[0], fmc.system);                                   /* MA */
%% if (strlen(fmc.subSys[0].name) > 0) {                                 /* MA */
%%    strcat(fName[0], "_");                                             /* MA */
%%    strcat_upper(fName[0], fmc.subSys[0].name);                        /* MA */
%% }                                                                     /* MA */
%% strcat(fName[0], ".");                                                /* MA */
%% strcpy(fName[1], fName[0]);                                           /* MA */
%% strcat(fName[0], "txt");                                              /* MA */
%% strcat(fName[1], "fir");                                              /* MA */

%DECL3%

%%  fmc.pVmeCoeff = (VME_COEF *)pVmeCoeff; /* Initialize pointer for fmReadCoeffFile() */
#if MAX_MODULES > 0
%%  for(ii=0;ii<MAX_MODULES;ii++)
%%    for(jj=0;jj<FILTERS;jj++) {
%%        pVmeCoeff->vmeCoeffs[ii].filtSections[jj] = 0;
%%        pVmeCoeff->vmeCoeffs[ii].filterType[jj] = 0;
%%        strcpy((char *)(pVmeCoeff->vmeCoeffs[ii].filtName[jj]),"NA");
%%    }
#endif

%%  /* printf("About to read file \n"); */
    strcpy(msg, "Reading coeff file...");
    pvPut(msg);

%%  if (fmReadCoeffFile(&fmc, 0, pEpics->epicsOutput.timeDiag) != 0) {
%%  	fprintf(stderr, "Error: %s\n", fmReadErrMsg());
	strncpy(msg, fmReadShortErrMsg(), MAX_STRING_SIZE-1);
%%  } else {
	strcpy(msg, localtimestring);
#if !defined(RFM_EPICS)
	/* Initialize variables for use in a filter module code */
	initVars(pFilt, pFilt,&coeffSpace, MAX_MODULES, pVmeCoeff);
#endif
%%  }
    pvPut(msg);

#if MAX_MODULES > 0
%%  for(ii=0;ii<MAX_MODULES;ii++) {
%%	copy_labels(ii);
	pvput_labels(ii);
%%  }
#endif
%%  printf("Should have put the values on the screen by now. ;)\n");
%%  printf("Setting initial screen variables to zero.\n");
#if MAX_MODULES > 0
%%  for(ii=0;ii<MAX_MODULES;ii++){
      input[ii] = 0.0;
      pvPut(input[ii]);
      excite[ii] = 0.0;
      pvPut(excite[ii]);
      offset[ii] = 0.0;
      pvPut(offset[ii]);
%%    pFilt->inputs[ii].offset = 0.0;

      outgain[ii] = 1.0;
      pvPut(outgain[ii]);
%%    pFilt->inputs[ii].outgain = 1.0;
      gain_ramp_time[ii] = 0.0;
      pvPut(gain_ramp_time[ii]);
%%    pFilt->inputs[ii].gain_ramp_time = 0.0;

      limit[ii] = 0;
      pvPut(limit[ii]);
      pFilt->inputs[ii].limiter = limit[ii];
      pFilt->inputs[ii].opSwitchE = 0;
      pFilt->inputs[ii].rset = 0;
      pFilt->inputs[ii].mask = 0;
      pFilt->inputs[ii].control = 0;

      sw1[ii] = 0x4;
      sw2[ii] = 0x600;
      switchR1[ii] = 0;
      switchR2[ii] = 0;
      saveSwitch1[ii] = 0;
      saveSwitch2[ii] = 0;
      swMask[ii] = 0;
      swReq[ii] = 0;
      swstat_alarm_level[ii] = 0x7fff;
      pvPut(sw1[ii]);
      pvPut(switchR1[ii]);
      pvPut(saveSwitch1[ii]);
      pvPut(sw2[ii]);
      pvPut(switchR2[ii]);
      pvPut(saveSwitch2[ii]);
      pvPut(swMask[ii]);
      pvPut(swReq[ii]);
      pvPut(swstat_alarm_level[ii]);
%%  }
#endif

    allNewCoeff = 0;
    pvPut(allNewCoeff);

%% }
#ifdef NO_DAQ_IN_SKELETON
  } state monScreen
#else
  } state daqInit
#endif
}

state monScreen{

  when(1){
%% int ii, jj;	
%% int ij;
%% int rtruns = 0;
%% int myTimeout;

%% rtruns = 1;
%%    stateWord = pEpics->epicsOutput.stateWord
%%	| (prev_gps == pEpics->epicsOutput.timeDiag
%%	   && prev_cycle == pEpics->epicsOutput.cycle);
%%    prev_gps = pEpics->epicsOutput.timeDiag;
%%    prev_cycle = pEpics->epicsOutput.cycle;
      pvGet(msg);
%%    if(strncmp(msg,"Modified",8) == 0) stateWord |= FE_ERROR_CFC;
      pvGet(msgDAQ);
%%    if(strncmp(msgDAQ,"Modified",8) == 0) stateWord |= FE_ERROR_CFC;
      pvPut(stateWord);
// If RT Code runs, sync up to it.
// else just wait 50msec.
%% if(stateWord & 1) rtruns = 0;
%% usleep(10000);
%% if(rtruns)
%% {
%%	myTimeout = 0;
%% 	do{
%%		usleep(2000);
%%		myTimeout ++;
%% 	}while((pEpics->epicsOutput.epicsSync == prev_seqTrig) && (myTimeout < 250));
%% } else {
%% 	usleep(10000);
%% }
%% pvUpdateCycle++;
%% prev_seqTrig = pEpics->epicsOutput.epicsSync;
   epicsSyncTime = prev_seqTrig * 0.0625  + pEpics->epicsOutput.timeDiag;
%% if(prev_seqTrig == 0) epicsSyncTime += 1;
   epicsWait = myTimeout;
   pvPut(epicsWait);

%% pthread_mutex_lock (&seq_lock);

#if MAX_MODULES > 0
%%  for(ii=0;ii<MAX_MODULES;ii++){
%%    double t;
      pvGet(swRset[ii]);
      if(swRset[ii]&1)
%% 	{
	  strcpy(msg, "Reading coeff file...");
	  pvPut(msg);

%%	  /* printf("About to read file \n"); */
%%	  if (fmReadCoeffFile(&fmc, 0, pEpics->epicsOutput.timeDiag) != 0) {
%%	     fprintf(stderr, "Error: %s\n", fmReadErrMsg());
	     strncpy(msg, fmReadShortErrMsg(), MAX_STRING_SIZE-1);
             pvPut(msg);
%%	  } else {
#if !defined(RFM_EPICS)
%%	    /* Successful coeff reload */
	    sprintf(msg, "%s Complete", fmmap0[ii].name);
            /* Load filter names to EPICS */
%%          copy_labels(ii);
            pvput_labels(ii);
	    initVars(pFilt, pFilt,&coeffSpace, MAX_MODULES, pVmeCoeff);
#else
%%	    int kk;

            /* Send new coeff flags to Pentium */
%%	    pFilt->coef_load_error = 0;
%%          /* Send new coeff flags to Pentium */
%%          pFilt->inputs[ii].rset = 1;
%%	    usleep(100);
%%	    for (kk = 0; kk < 2000; kk++) {
%%		int vme_ack;
%%		vme_ack = pFilt->coef_load_error;
%%		if (vme_ack == -1) {
%%		   /* RFM network error */
		   sprintf(msg, "%s Transmission Error", fmmap0[ii].name);
%%		   goto bailout2;
%%		} else if (vme_ack == ii + 1) {
%%		   /* Successful coeff reload */
		   sprintf(msg, "%s Complete", fmmap0[ii].name);
                   /* Load filter names to EPICS */
%%            	   copy_labels(ii);
                   pvput_labels(ii);
%%		   break;
%%		}
%%		usleep(100);
%%	    }
%%	    if (kk == 2000) {
		strcpy(msg, "Error: Front-end timeout");
%%	    }
#endif
%%  	  }
%%   bailout2:
	  pvPut(msg);
	  /* set switch back to zero */
	  swRset[ii] = 0;
	  pvPut(swRset[ii]);
%%      }

	swStatus[ii] = pFilt->data[ii].swStatus;
	pvGet(swMask[ii]);
	pvGet(swReq[ii]);
%%      if(swMask[ii])
%%      {
%%	int smask = swMask[ii];
%%	int sreq = swReq[ii];
%%	int sstat = swStatus[ii];
%%	for(jj = 0; jj<17;jj++)
%%	{
%%		if(smask & 1)
%%		{
%%			if((sreq & 1) != (sstat & 1))
%%			{
%%				swStatus[ii] |= 0x40000;
%%			}
%%		}
%%		smask >>= 1;
%%		sreq >>= 1;
%%		sstat >>= 1;
%%	}
%%      }
	
      /* Check for history reset */
      if(swRset[ii]&2)
%%      {
	  /* set switch back to zero */
%%	  /*printf("resetting history for module %d\n",ii);*/
%%	  pFilt->inputs[ii].rset = 2;
	  swRset[ii] = 0;
	  pvPut(swRset[ii]);
	  swStatus[ii] |= 0x20000;
%%      }
	pvPut(swStatus[ii]);


      /* Get saved switch settings, used for burt purposes */
      pvGet(saveSwitch1[ii]);  
      pvGet(saveSwitch2[ii]);  

      /* Get filter switch selections */
      pvGet(sw1[ii]);
      pvGet(sw2[ii]);	

%%    unsigned int msk = pFilt->inputs[ii].mask;
%%    unsigned int control = pFilt->inputs[ii].control;
%% 	/*printf("mask=%d control=%d\n", msk, control);*/
      if ((saveSwitch1[ii] & (msk & 0xffff)) != (control & 0xffff))
%%    {
	saveSwitch1[ii] = (control & msk) | (saveSwitch1[ii] & ~msk);
      	pvPut(saveSwitch1[ii]);  
%%    }
      if(sw1[ii]) {
        saveSwitch1[ii] ^= (sw1[ii] & ~msk);
      	sw1[ii] = 0;
      	pvPut(sw1[ii]);  
      	pvPut(saveSwitch1[ii]);  
      }
      msk >>=16; control >>= 16;
      if ((saveSwitch2[ii] & (msk & 0xffff)) != (control & 0xffff))
%%    {
	saveSwitch2[ii] = (control & msk) | (saveSwitch2[ii] & ~msk);
      	pvPut(saveSwitch2[ii]);  
%%    }
      if(sw2[ii])
%%    {
        saveSwitch2[ii] ^= (sw2[ii] & ~msk);
      	sw2[ii] = 0;
      	pvPut(sw2[ii]);  
      	pvPut(saveSwitch2[ii]);  
%%    }




%%    {
%%         int getSwitch1, getSwitch2;

      /* Get switch readbacks from Pentium */
%%	getSwitch1 = ((pFilt->inputs[ii].opSwitchP)&(mask[0]));
%%    	getSwitch2 = (((pFilt->inputs[ii].opSwitchP) >> 16)&(mask[1]));
      	switchR1[ii] = ((getSwitch1)|(0x555f&saveSwitch1[ii]));
	switchR2[ii] = ((getSwitch2)|(0xf55&saveSwitch2[ii]));
%%    }
%%    rfm_assign(pFilt->inputs[ii].opSwitchE, (switchR2[ii] << 16) + switchR1[ii]);
      pvPut(switchR1[ii]);  
      pvPut(switchR2[ii]);

%%      /*get/set everything else*/
%%	t = fpvalidate(pFilt->data[ii].filterInput);
        input[ii] = t;
%%	t = fpvalidate(pFilt->data[ii].exciteInput);
	excite[ii] = t;
/*
	input[ii] *= conv;
	excite[ii] *= conv;
*/
        pvPut(input[ii]);
	pvPut(excite[ii]);
	if (msk & 0x2000) { /* Offset is controlled locally by the FE */
%%		offset[ii] = pFilt->inputs[ii].offset;
		pvPut(offset[ii]);
	} else {
		pvGet(offset[ii]);
%%		rfm_assign(pFilt->inputs[ii].offset, offset[ii]);
	}

	if (msk & 0x4000) { /* Gain is controlled locally by the FE */
%%		outgain[ii] = pFilt->inputs[ii].outgain;
		pvPut(outgain[ii]);
	} else {
		pvGet(outgain[ii]);
%%		rfm_assign(pFilt->inputs[ii].outgain, outgain[ii]);
	}

	if (msk & 0x8000) { /* Ramp time is controlled locally by the FE */
%%		gain_ramp_time[ii] = pFilt->inputs[ii].gain_ramp_time;
		pvPut(gain_ramp_time[ii]);
	} else {
		pvGet(gain_ramp_time[ii]);
%%		rfm_assign(pFilt->inputs[ii].gain_ramp_time, gain_ramp_time[ii]);
	}

	pvGet(limit[ii]);
%%	rfm_assign(pFilt->inputs[ii].limiter, limit[ii]);
%%	t = fpvalidate(pFilt->data[ii].testpoint);
	tp[ii] = t;
	/*tp[ii] = pFilt->data[ii].testpoint;*/
%%	t = fpvalidate(pFilt->data[ii].output16Hz)/* * conv*/;
	out16Hz[ii] = t;
	pvPut(out16Hz[ii]);
%%	t = fpvalidate(pFilt->data[ii].output);
	output[ii] = t;
	pvPut(tp[ii]);
	pvPut(output[ii]);


#if !defined(RFM_EPICS)
    	pvGet(epics_exc[ii]);
#if defined(SEQUENCER_NAME_tcsh1) || defined(SEQUENCER_NAME_tcsh2)
%%	switch(ii) {
%%		case 0:
    			pvGet(tcs1Input);
    			pvGet(tcs2Input);
%%			inpt = tcs1Input * pEpicsSpace.inmtrx[0][0]
%%			   + tcs2Input * pEpicsSpace.inmtrx[1][0];
%%			break;
%%		case 1:
%%			inpt = tcs1Input * pEpicsSpace.inmtrx[0][1]
%%			   + tcs2Input * pEpicsSpace.inmtrx[1][1];
%%			break;
%%		case 2:
%%			inpt = outpts[0] * pEpicsSpace.outmtrx[0][0]
%%			   + outpts[1] * pEpicsSpace.outmtrx[1][0];
%%			break;
%%		case 3:
%%			inpt = outpts[0] * pEpicsSpace.outmtrx[0][1]
%%			   + outpts[1] * pEpicsSpace.outmtrx[1][1];
    			pvGet(tcsEnable);
%%			break;
%%	}
#else
    	pvGet(epics_input[ii]);
	inpt = epics_input[ii];
#endif
%%  	pFilt->data[ii].exciteInput = epics_exc[ii];
%%	checkFiltReset(ii, pFilt, pFilt, &coeffSpace, MAX_MODULES, pVmeCoeff);
%%  	outpts[ii] =
%%	   filterModuleD(pFilt,     /* Filter module data  */
%%             &coeffSpace,            /* Filter coefficients */
%%             ii,          /* Filter module number */
%%	       inpt,
%%             0); /* system id */
#if defined(SEQUENCER_NAME_tcsh1) || defined(SEQUENCER_NAME_tcsh2)
%%	if (ii == 3) {
%%	   if (tcsEnable) {
%%	     tcsXOutput = outpts[2];
%%	     tcsYOutput = outpts[3];
	     pvPut(tcsXOutput, SYNC);
	     pvPut(tcsYOutput, SYNC);
%%	   }
%% 	}
#endif
#endif

%% }
#endif


%DECL4%

   pvPut(epicsSyncTime);

%% time_t lt = time(0);
%% strcpy(localtimestring, ctime(&lt));
%% localtimestring[strlen(localtimestring) - 1] = 0; /* Delete newline character */
   pvPut(localtimestring);


%% pthread_mutex_unlock (&seq_lock);
#ifndef NO_DAQ_IN_SKELETON
  }state daqProcess
#else
  }state filtCoefChk
#endif
}

#ifndef NO_DAQ_IN_SKELETON
state daqInit
{
  when()
    {
%%      strcpy(fNameDAQ, "/opt/rtcds/");                                 /* ma */
%%      strcat_lower(fNameDAQ, fmc.site);                                /* ma */
%%      strcat(fNameDAQ, "/");                                           /* ma */
%%      strcat_lower(fNameDAQ, fmc.ifo);                                 /* ma */
%%      strcat(fNameDAQ, "/chans/daq/");                                 /* ma */
%%      strcat_upper(fNameDAQ, fmc.system);                              /* ma */
%%      if (strlen(fmc.subSys[0].name) > 0) {                            /* ma */
%%         strcat(fNameDAQ, "_");                                        /* ma */
%%         strcat_upper(fNameDAQ, fmc.subSys[0].name);                   /* ma */
%%      }                                                                /* ma */
%%      strcat(fNameDAQ, ".ini");                                        /* ma */

        dcuId = atoi(macValueGet("sysnum"));
%%      if (dcuId < 0 || dcuId >= DCU_COUNT) {
%%        fprintf(stderr, "Invalid system number %d\n", dcuId);
%%        exit(1);
%%      }
        strcpy(site, macValueGet("site"));
        strcpy(ifo, macValueGet("ifo"));
	strcpy(daqsys, macValueGet("sysfile"));
%%      info = ((DAQ_INFO_BLOCK *)((base = (volatile void *)findRfmCard(0)) + DAQ_INFO_ADDRESS));
	pvPut(dcuId);
%%    rfm_assign(pEpics->epicsInput.dcuId, dcuId);
%%      printf("daq info at 0x%lx\n",(unsigned long)info);
%%      memset(&infoTmp, 0, sizeof(infoTmp));
%%      info->reconfig = 0;
%%      if (loadDaqConfigFile(&infoTmp, site, ifo, daqsys)) {
           strcpy(msgDAQ, localtimestring);
%%         copy_info();
%%         printf("%s%s daqConfig info is at rfm offset 0x%lx\n", ifo, daqsys, ((char *)info) - ((char *)base));
%%         info -> reconfig = 1;
%%         chnCnt = infoTmp.numChans;
%%         epicschnCnt = infoTmp.numEpicsInts + infoTmp.numEpicsFloats + infoTmp.numEpicsFilts;
%%         dataSize = 0;
%%         { int i;
%%              for(i=0; i < infoTmp.numChans; i++) {
%%                      dataSize += infoTmp.tp[i].dataRate * DAQ_DATA_TYPE_SIZE(infoTmp.tp[i].dataType);
%%              }
%%         }
%%		dataSize += 64 * (infoTmp.numEpicsInts + infoTmp.numEpicsFloats + infoTmp.numEpicsFilts);
%%         dataSize /= 1000;
           strcpy(msgDAQ, localtimestring);
%%      } else {
           strcpy(msgDAQ, "ERROR reading DAQ file!");
%%         /*chnCnt = 0;*/
%%         /*dataSize = 0;*/
%%      }

        pvPut(dcmsg);
        pvPut(chnCnt);
        pvPut(epicschnCnt);
        pvPut(dataSize);
        pvPut(msgDAQ);
    } state monScreen
}

state daqProcess
{
  when()
    {
%%      int modDAQ;                                                      /* ma */
%%	static unsigned int cksum_cnt = 0;

%% pthread_mutex_lock (&seq_lock);

        pvGet(reload);
        if (reload)
%%        {
%%          info -> reconfig = 0;
            strcpy(msgDAQ, "Reading DAQ (.ini) file...");
            pvPut(msgDAQ);
%%          if (loadDaqConfigFile(&infoTmp, site, ifo, daqsys)) {
%%            copy_info();
%%            printf("%s%s daqConfig info is at rfm offset 0x%lx\n", ifo, daqsys, ((char *)info) - ((char *)base))
;
%%            info -> reconfig = 1;
%%            chnCnt = infoTmp.numChans;
%%         epicschnCnt = infoTmp.numEpicsInts + infoTmp.numEpicsFloats + infoTmp.numEpicsFilts;
%%            dataSize = 0;
%%            { int i;
%%              for(i=0; i < infoTmp.numChans; i++) {
%%                      dataSize += infoTmp.tp[i].dataRate * DAQ_DATA_TYPE_SIZE(infoTmp.tp[i].dataType);
%%              }
%%            }
%%		dataSize += 64 * (infoTmp.numEpicsInts + infoTmp.numEpicsFloats + infoTmp.numEpicsFilts);
%%            dataSize /= 1000;
%%            sleep(1);
              strcpy(msgDAQ, localtimestring);
%%          } else {
		pvGet(chnCnt);
		pvGet(dataSize);
              strcpy(msgDAQ, "ERROR reading DAQ file!");
%%          }
            pvPut(chnCnt);
            pvPut(epicschnCnt);
            pvPut(dataSize);
            pvPut(msgDAQ);
%%        }
        reload = 0;
        pvPut(reload);
%% pthread_mutex_unlock (&seq_lock);

    } state filtCoefChk
}
#endif

state filtCoefChk
{
  when()
    {
%% 	int mod,ii;
%% pthread_mutex_lock (&seq_lock);
%%  
   /* Coefficient update request */	
    pvGet(allNewCoeff);
    if(allNewCoeff)
%%    {
   pvGet(localtimestring);
%%              fprintf(stderr, "%s READING COEFF FILE\n",localtimestring);
	  strcpy(msg, "Reading coeff File...");
	  pvPut(msg);


%%        /* printf("About to read file \n"); */
%%        if (fmReadCoeffFile(&fmc, 0, pEpics->epicsOutput.timeDiag) != 0) {
%%              fprintf(stderr, "Error: %s\n", fmReadErrMsg());
                strncpy(msg, fmReadShortErrMsg(), MAX_STRING_SIZE-1);
                pvPut(msg);
%%	  } else {
#if !defined(RFM_EPICS)
%%	    /* Successful coeff reload */
	    sprintf(msg, "Coeff load complete");
#if MAX_MODULES > 0
            /* Load filter names to EPICS */
%%          for (ii=0;ii<MAX_MODULES;ii++) {
%%            	copy_labels(ii);
                pvput_labels(ii);
%%	    }
#endif
	    initVars(pFilt, pFilt,&coeffSpace, MAX_MODULES, pVmeCoeff);
#else
#if MAX_MODULES > 0
%%           for (ii=0;ii<MAX_MODULES;ii++) {
%%	       int kk;
%%	       pFilt->coef_load_error = 0;
%%
%%             /* Send new coeff flags to Pentium */
%%             pFilt->inputs[ii].rset = 1;
%%	       usleep(100);
%%
%%	       for (kk = 0; kk < 2000; kk++) {
%%		 int vme_ack;
%%		 vme_ack = pFilt->coef_load_error;
%%		 if (vme_ack == -1) {
%%		    /* RFM network error */
		    sprintf(msg, "%s Transmission Error", fmmap0[ii].name);
		    pvPut(msg);
%%		    goto bailout1;
%%		 } else if (vme_ack == ii + 1) {
%%		    /* Successful coeff reload */
		    sprintf(msg, "%s Complete", fmmap0[ii].name);
		    pvPut(msg);
                    /* Load filter names to EPICS */
%%		    copy_labels(ii);
		    pvput_labels(ii);
%%		    break;
%%		 }
%%		 usleep(500);
%%	       }
%%	       if (kk == 2000) {
		 strcpy(msg, "Error: front-end timeout");
		 pvPut(msg);
%%		 break;
%%	       }
%%           }
#endif
#endif
%%	bailout1:
   pvGet(localtimestring);
%%	     if (ii == MAX_MODULES) {
		strcpy(msg, localtimestring);
               pvPut(msg);
%%	     }
%%	  }
          allNewCoeff = 0;
          pvPut(allNewCoeff);
%%    }
%% pthread_mutex_unlock (&seq_lock);
    } state monScreen
}

}
