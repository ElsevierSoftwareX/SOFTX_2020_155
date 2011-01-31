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

%% #define INLINE
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

string msg;
assign msg to "{ifo}:FEC-{sysnum}_MSG";

int statErrCnt;
assign statErrCnt to "{ifo}:{sys}-{subsys}STAT_ERR_CNT";

string staterr[6];
assign staterr to {
"{ifo}:{sys}-{subsys}FESTAT_1","{ifo}:{sys}-{subsys}FESTAT_2",
"{ifo}:{sys}-{subsys}FESTAT_3","{ifo}:{sys}-{subsys}FESTAT_4",
"{ifo}:{sys}-{subsys}FESTAT_5","{ifo}:{sys}-{subsys}FESTAT_6"};

int matrixThreshCount;                                                   /* MA */
assign matrixThreshCount to "{ifo}:{sys}-{subsys}MTRX_STAT_ERR_CNT";     /* MA */

string matrixErr[10];                                                    /* MA */
assign matrixErr to {                                                    /* MA */
"{ifo}:{sys}-{subsys}MTRXSTAT_1","{ifo}:{sys}-{subsys}MTRXSTAT_2",       /* MA */
"{ifo}:{sys}-{subsys}MTRXSTAT_3","{ifo}:{sys}-{subsys}MTRXSTAT_4",       /* MA */
"{ifo}:{sys}-{subsys}MTRXSTAT_5","{ifo}:{sys}-{subsys}MTRXSTAT_6",       /* MA */
"{ifo}:{sys}-{subsys}MTRXSTAT_7","{ifo}:{sys}-{subsys}MTRXSTAT_8",       /* MA */
"{ifo}:{sys}-{subsys}MTRXSTAT_9","{ifo}:{sys}-{subsys}MTRXSTAT_10"};     /* MA */

int filterStatusCount;                                                   /* MA */
assign filterStatusCount to  "{ifo}:{sys}-{subsys}FILTER_STATUS_COUNT";  /* MA */

string filterErrId[10];                                                  /* MA */
assign filterErrId to {                                                  /* MA */
"{ifo}:{sys}-{subsys}FILTER_ID_1","{ifo}:{sys}-{subsys}FILTER_ID_2",     /* MA */
"{ifo}:{sys}-{subsys}FILTER_ID_3","{ifo}:{sys}-{subsys}FILTER_ID_4",     /* MA */
"{ifo}:{sys}-{subsys}FILTER_ID_5","{ifo}:{sys}-{subsys}FILTER_ID_6",     /* MA */
"{ifo}:{sys}-{subsys}FILTER_ID_7","{ifo}:{sys}-{subsys}FILTER_ID_8",     /* MA */
"{ifo}:{sys}-{subsys}FILTER_ID_9","{ifo}:{sys}-{subsys}FILTER_ID_10"};   /* MA */

string filterParam[10];                                                  /* MA */
assign filterParam to {                                                  /* MA */
"{ifo}:{sys}-{subsys}FILTER_PARAM_1",                                    /* MA */
"{ifo}:{sys}-{subsys}FILTER_PARAM_2",                                    /* MA */
"{ifo}:{sys}-{subsys}FILTER_PARAM_3",                                    /* MA */
"{ifo}:{sys}-{subsys}FILTER_PARAM_4",                                    /* MA */
"{ifo}:{sys}-{subsys}FILTER_PARAM_5",                                    /* MA */
"{ifo}:{sys}-{subsys}FILTER_PARAM_6",                                    /* MA */
"{ifo}:{sys}-{subsys}FILTER_PARAM_7",                                    /* MA */
"{ifo}:{sys}-{subsys}FILTER_PARAM_8",                                    /* MA */
"{ifo}:{sys}-{subsys}FILTER_PARAM_9",                                    /* MA */
"{ifo}:{sys}-{subsys}FILTER_PARAM_10"};                                  /* MA */

string filterSeverity[10];                                               /* MA */
assign filterSeverity to {                                               /* MA */
"{ifo}:{sys}-{subsys}FILTER_SEVERITY_1",                                 /* MA */
"{ifo}:{sys}-{subsys}FILTER_SEVERITY_2",                                 /* MA */
"{ifo}:{sys}-{subsys}FILTER_SEVERITY_3",                                 /* MA */
"{ifo}:{sys}-{subsys}FILTER_SEVERITY_4",                                 /* MA */
"{ifo}:{sys}-{subsys}FILTER_SEVERITY_5",                                 /* MA */
"{ifo}:{sys}-{subsys}FILTER_SEVERITY_6",                                 /* MA */
"{ifo}:{sys}-{subsys}FILTER_SEVERITY_7",                                 /* MA */
"{ifo}:{sys}-{subsys}FILTER_SEVERITY_8",                                 /* MA */
"{ifo}:{sys}-{subsys}FILTER_SEVERITY_9",                                 /* MA */
"{ifo}:{sys}-{subsys}FILTER_SEVERITY_10"};                               /* MA */

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

int dataSize;
assign dataSize to "{ifo}:DAQ-FEC_{sysnum}_TOTAL";

int dcuId;
assign dcuId to "{ifo}:DAQ-FEC_{sysnum}_DCU_ID";
#endif


%DECL2%

%% static fmReadCoeff fmc = {
%%  "lho", "h1", "xxx", 0, {{"", "", MAX_MODULES, fmmap0, 0}},
%% };

%% static const float conv = 0.000152588;
%% static const int mask[2] = {0xAAA0,0x10AA};
%% extern pthread_mutex_t seq_lock;

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

#define update_labels(ii)\
       strcpy(labels0[ii],pVmeCoeff->vmeCoeffs[ii].filtName[0]);\
       pvPut(labels0[ii]);\
       strcpy(labels1[ii],pVmeCoeff->vmeCoeffs[ii].filtName[1]);\
       pvPut(labels1[ii]);\
       strcpy(labels2[ii],pVmeCoeff->vmeCoeffs[ii].filtName[2]);\
       pvPut(labels2[ii]);\
       strcpy(labels3[ii],pVmeCoeff->vmeCoeffs[ii].filtName[3]);\
       pvPut(labels3[ii]);\
       strcpy(labels4[ii],pVmeCoeff->vmeCoeffs[ii].filtName[4]);\
       pvPut(labels4[ii]);\
       strcpy(labels5[ii],pVmeCoeff->vmeCoeffs[ii].filtName[5]);\
       pvPut(labels5[ii]);\
       strcpy(labels6[ii],pVmeCoeff->vmeCoeffs[ii].filtName[6]);\
       pvPut(labels6[ii]);\
       strcpy(labels7[ii],pVmeCoeff->vmeCoeffs[ii].filtName[7]);\
       pvPut(labels7[ii]);\
       strcpy(labels8[ii],pVmeCoeff->vmeCoeffs[ii].filtName[8]);\
       pvPut(labels8[ii]);\
       strcpy(labels9[ii],pVmeCoeff->vmeCoeffs[ii].filtName[9]);\
       pvPut(labels9[ii]);

%% static DAQ_INFO_BLOCK *info;
%% static void *base;
%% static DAQ_INFO_BLOCK infoTmp;
%% static char site[160];
%% static char ifo[160];
%% static char daqsys[160];

%% char buffer[264];                                                     /* MA */
%% char fName[2][256];                                                   /* MA */
%% char fType[3][10] = {"IIR", "FIR", "IIR & FIR"};                      /* MA */

%% FILE *pipePtr;                                                        /* MA */

#ifdef FIR_FILTERS                                                       /* MA */
%% int fltrTypes = 2;                                                    /* MA */
#else                                                                    /* MA */
%% int fltrTypes = 1;                                                    /* MA */
#endif                                                                   /* MA */

%% long chkSum[2] = {-99999, -99999};                                    /* MA */
%% long crc;                                                             /* MA */

%% struct stat statBuf;                                                  /* MA */

%% int statusFlag = 0;                                                   /* MA */
%% int matrixStatusFlag = 0;                                             /* MA */

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
%% }


ss monitorFilt{
 
state init
{
  when() {
%% {
%% int ii, jj;
%% int ij;                                                               /* MA */

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
%%  printf("%s pFilt is at 0x%x\n", fmc.subSys[0].name, ((int)pFilt-(int)pRfm));
%%  printf("%s pVmeCoeff is at 0x%x\n", fmc.subSys[0].name, ((int)pVmeCoeff-(int)pRfm));
%%  printf("%s pVmeCoeff size is 0x%x\n", fmc.subSys[0].name, sizeof(*pVmeCoeff));
%%  printf("%s pEpics is at 0x%x\n", fmc.subSys[0].name, ((int)pEpics-(int)pRfm));
%%  printf("%s pEpics is at 0x%x\n", fmc.subSys[0].name, ((int)pRfm));
#endif

%% strcpy(fName[0], "/opt/rtcds/");                                        /* MA */
%% strcat_lower(fName[0], fmc.site);                                     /* MA */
%% strcat(fName[0], "/chans/");                                          /* MA */
%% strcat_upper(fName[0], fmc.ifo);                                      /* MA */
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
%%  for(ii=0;ii<MAX_MODULES;ii++)
%%    for(jj=0;jj<FILTERS;jj++) {
%%        pVmeCoeff->vmeCoeffs[ii].filtSections[jj] = 0;
%%        pVmeCoeff->vmeCoeffs[ii].filterType[jj] = 0;
%%        strcpy((char *)(pVmeCoeff->vmeCoeffs[ii].filtName[jj]),"NA");
%%    }

%%  printf("About to read file \n");
    strcpy(msg, "Reading coeff file...");
    pvPut(msg);
%%  for(ii=0;ii<6;ii++) {
%%    strcpy(staterr[ii], "Clr");
      pvPut(staterr[ii]);
%%  }
%%  for (ii = 0; ii < 10; ii++) {                                        /* MA */
%%     strcpy(matrixErr[ii], "Clr");                                     /* MA */
       pvPut(matrixErr[ii]);                                             /* MA */
%%  }                                                                    /* MA */

%%  if (fmReadCoeffFile(&fmc, 0) != 0) {
%%  	fprintf(stderr, "Error: %s\n", fmReadErrMsg());
	strncpy(msg, fmReadShortErrMsg(), MAX_STRING_SIZE-1);
%%  } else {
	strcpy(msg, "Coeff file load complete");
#if !defined(RFM_EPICS)
	/* Initialize variables for use in a filter module code */
	initVars(pFilt, pFilt,&coeffSpace, MAX_MODULES, pVmeCoeff);
#endif
%%  }
    pvPut(msg);

%%  for (ii = 0; ii < fltrTypes; ii++) {                                 /* MA */
%%     strcpy(buffer, "cksum ");                                         /* MA */
%%     strcat(buffer, fName[ii]);                                        /* MA */
%%     if (!stat(fName[ii], &statBuf)) {                                 /* MA */
%%        if ((pipePtr = popen(buffer, "r")) != NULL) {                  /* MA */
%%           fgets(buffer, 256, pipePtr);                                /* MA */
%%           pclose(pipePtr);                                            /* MA */
%%           sscanf(buffer, "%ld", &chkSum[ii]);                         /* MA */
%%        }                                                              /* MA */
%%     }                                                                 /* MA */
%%  }                                                                    /* MA */

%%  for(ii=0;ii<MAX_MODULES;ii++) {
	update_labels(ii);
%%  }
%%  printf("should have put the values on the screen by now. ;)\n");
%%  printf("setting initial screen variables to zero.\n");
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

      sw1[ii] = 0x4;
      sw2[ii] = 0x600;
      switchR1[ii] = 0;
      switchR2[ii] = 0;
      saveSwitch1[ii] = 0;
      saveSwitch2[ii] = 0;
      pvPut(sw1[ii]);
      pvPut(switchR1[ii]);
      pvPut(saveSwitch1[ii]);
      pvPut(sw2[ii]);
      pvPut(switchR2[ii]);
      pvPut(saveSwitch2[ii]);
%%  }

#if 0
    swRset[0] = 0;
    swRset[1] = 0;
    pvPut(swRset[0]);
    pvPut(swRset[1]);

    runAcquireWait[0] = 2;
    pvPut(runAcquireWait[0]);
    runAcquireWait[1] = 2;
    pvPut(runAcquireWait[1]);
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
%% int ij;                                                               /* MA */
%% int mod;                                                              /* MA */
%% int rcode = 0;                                                        /* MA */
%% int size;                                                             /* MA */
%% char matrixID[32];                                                    /* MA */
%% usleep(200000);
/* %% char thresholdErrMsg[6][32];                                          MA */

/* %% strcpy(thresholdErrMsg[0], "Threshold file open failed");             MA */
/* %% strcpy(thresholdErrMsg[1], "Thresholds not found");                   MA */
/* %% strcpy(thresholdErrMsg[2], "Thresh. parse err. - cf. stderr");        MA */
/* %% strcpy(thresholdErrMsg[3], "Matrix thresh. file open failed");        MA */
/* %% strcpy(thresholdErrMsg[4], "Matrix thresholds not found");            MA */
/* %% strcpy(thresholdErrMsg[5], "MThresh. parse err - cf. stderr");        MA */

%% int thisFilterStatusCount;                                            /* MA */
%% char fltrParamHelp[10];                                               /* MA */
%% char fltrSevHelp[10];                                                 /* MA */

%% pthread_mutex_lock (&seq_lock);

%%  mod = 0;                                                             /* MA */
%%  for (ii = 0; ii < fltrTypes; ii++) {                                 /* MA */
%%     strcpy(buffer, "cksum ");                                         /* MA */
%%     strcat(buffer, fName[ii]);                                        /* MA */
%%     if (!stat(fName[ii], &statBuf)) {                                 /* MA */
%%        if ((pipePtr = popen(buffer, "r")) != NULL) {                  /* MA */
%%           fgets(buffer, 256, pipePtr);                                /* MA */
%%           pclose(pipePtr);                                            /* MA */
%%           sscanf(buffer, "%d", &crc);                                 /* MA */
%%           if (crc != chkSum[ii]) {                                    /* MA */
%%              chkSum[ii] = crc;                                        /* MA */
%%              mod += ii + 1;                                           /* MA */
%%           }                                                           /* MA */
%%        }                                                              /* MA */
%%     }                                                                 /* MA */
%%  }                                                                    /* MA */
%%  if (mod) {                                                           /* MA */
       strcpy(msg, "Modified ");                                         /* MA */
       strcat(msg, fType[mod - 1]);                                      /* MA */
       strcat(msg, " file");                                             /* MA */
%%     if (mod == 3) {                                                   /* MA */
          strcat(msg, "s");                                              /* MA */
%%     }                                                                 /* MA */
       pvPut(msg);                                                       /* MA */
%%  }                                                                    /* MA */

   /* Coefficient update request */	
    pvGet(allNewCoeff);
    if(allNewCoeff)
%%    {

/* %%        rcode = readThresholdsFile(&fmc,                               MA */
/* %%                                   &fmmap0,                            MA */
/* %%                                   &thresholds,                        MA */
/* %%                                   MAX_MODULES,                        MA */
/* %%                                   0);                                 MA */

%%        statusFlag = 1;                                                /* MA */
/* %%        if (rcode) {                                                   MA */
/* %%           strcpy(msg, thresholdErrMsg[rcode - 1]);                    MA */
/*              pvPut(msg);                                                 MA */

/* %%           if (rcode != TF_PARSE_ERROR) {                              MA */
/* %%              strcat(msg, "\n");                                       MA */
/* %%              fprintf(stderr, "%s", msg);                              MA */
/* %%              statusFlag = 0;                                          MA */
/* %%           }                                                           MA */
/* %%        }                                                              MA */

%%        matrixStatusFlag = 1;                                          /* MA */

/* %DECL3_5%                                                                MA */

	  strcpy(msg, "Reading coeff file...");
	  pvPut(msg);


%%        printf("About to read file \n");
%%        if (fmReadCoeffFile(&fmc, 0) != 0) {
%%              fprintf(stderr, "Error: %s\n", fmReadErrMsg());
                strncpy(msg, fmReadShortErrMsg(), MAX_STRING_SIZE-1);
                pvPut(msg);
%%	  } else {
#if !defined(RFM_EPICS)
%%	    /* Successful coeff reload */
	    sprintf(msg, "Coeff load complete");
            /* Load filter names to EPICS */
%%          for (ii=0;ii<MAX_MODULES;ii++) {
            	update_labels(ii);
%%	    }
	    initVars(pFilt, pFilt,&coeffSpace, MAX_MODULES, pVmeCoeff);
#else
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
		    update_labels(ii);
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
%%	bailout1:
%%	     if (ii == MAX_MODULES) {
               strcpy(msg, "Coeff file load complete");
               pvPut(msg);
%%	     }
%%	  }
          allNewCoeff = 0;
          pvPut(allNewCoeff);
%%    }

#if defined(SEQUENCER_NAME_ascmc) || defined(SEQUENCER_NAME_asc) || defined(SEQUENCER_NAME_asc40m)
%%  for(ii=0;ii<FILT_QUAD_SIZE;ii++){
      pvGet(filt[ii]);
%%  }
#endif

#ifdef SEQUENCER_NAME_lsc_removed
    pvGet(lscModeSwitch);
%%  if(lscModeSwitch != lastLscModeSwitch)
%%  {
      lastLscModeSwitch = lscModeSwitch;
%%    if(lscModeSwitch == 0)
%%    {
%%	saveSwitch1[0] &= 0xffef;
%%	saveSwitch1[3] &= 0xffef;
	pvPut(saveSwitch1[0]);
	pvPut(saveSwitch1[3]);
%%	saveSwitch1[30] &= 0xffef;
%%	saveSwitch1[31] &= 0xffef;
%%	saveSwitch1[32] &= 0xffef;
	pvPut(saveSwitch1[30]);
	pvPut(saveSwitch1[31]);
	pvPut(saveSwitch1[32]);
%%    }
%%    if(lscModeSwitch == 1)
%%    {
%%	saveSwitch1[0] |= 0x10;
%%	saveSwitch1[3] |= 0x10;
	pvPut(saveSwitch1[0]);
	pvPut(saveSwitch1[3]);
%%	saveSwitch1[30] &= 0xffef;
%%	saveSwitch1[31] &= 0xffef;
%%	saveSwitch1[32] &= 0xffef;
	pvPut(saveSwitch1[30]);
	pvPut(saveSwitch1[31]);
	pvPut(saveSwitch1[32]);
%%    }
%%    if(lscModeSwitch == 2)
%%    {
%%	saveSwitch1[30] |= 0x10;
%%	saveSwitch1[31] |= 0x10;
%%	saveSwitch1[32] |= 0x10;
	pvPut(saveSwitch1[30]);
	pvPut(saveSwitch1[31]);
	pvPut(saveSwitch1[32]);
%%    }
	pvGet(laState);
%%    if((lscModeSwitch == 2) && (laState & 0x20))
%%    {
%%	saveSwitch1[0] |= 0x10;
%%	saveSwitch1[3] |= 0x10;
	pvPut(saveSwitch1[0]);
	pvPut(saveSwitch1[3]);
%%    }
%%    if((lscModeSwitch == 2) && ((laState & 0x20) == 0))
%%    {
%%	saveSwitch1[0] &= ~0x10;
%%	saveSwitch1[3] &= ~0x10;
	pvPut(saveSwitch1[0]);
	pvPut(saveSwitch1[3]);
%%    }
%%  }

#endif

%%  statErrCnt = 0;                                                      /* MA */
%%  int displayCount;                                                    /* MA */
%%  displayCount = /* 0; */ 1;                                                    /* MA */
%%  int matrixDisplayCount;                                              /* MA */
%%  matrixDisplayCount = 0;                                              /* MA */
%%  int thresholdFlag;                                                   /* MA */
%%  filterStatusCount = 0;                                               /* MA */
%%  int filterDisplayCount = 0;                                          /* MA */

%%  setpointStatusCount = 0;                                             /* MA */
%%  int setpointDisplayCount = 0;                                        /* MA */
%%  int matrixStatusMajCount = 0;                                        /* MA */
%%  int matrixStatusMinCount = 0;                                        /* MA */

%%  for(ii = 0; ii < 6; ii++) {                                          /* MA */
%%     strcpy(staterr[ii], "CLR");                                       /* MA */
%%  }                                                                    /* MA */

%%  for (ii = 0; ii < 10; ii++) {                                        /* MA */
%%     strcpy(matrixErr[ii], "CLR");                                     /* MA */
%%  }                                                                    /* MA */

%%  for (ii = 0; ii < 10; ii++) {                                        /* MA */
%%     strcpy(filterErrId[ii], "CLR");                                   /* MA */
%%     strcpy(filterParam[ii], "CLR");                                   /* MA */
%%     strcpy(filterSeverity[ii], "CLR");                                /* MA */
%%                                                                       /* MA */
%%     strcpy(setpointErrId[ii], "CLR");                                 /* MA */
%%     strcpy(setpointMajorCount[ii], "CLR");                            /* MA */
%%     strcpy(setpointMinorCount[ii], "CLR");                            /* MA */
%%  }                                                                    /* MA */

%%  for(ii=0;ii<MAX_MODULES;ii++){
%%    float t;
      pvGet(swRset[ii]);
      if(swRset[ii]&1)
%% 	{
	  strcpy(msg, "Reading coeff file...");
	  pvPut(msg);

%%	  printf("About to read file \n");
%%	  if (fmReadCoeffFile(&fmc, 0) != 0) {
%%	     fprintf(stderr, "Error: %s\n", fmReadErrMsg());
	     strncpy(msg, fmReadShortErrMsg(), MAX_STRING_SIZE-1);
             pvPut(msg);
%%	  } else {
#if !defined(RFM_EPICS)
%%	    /* Successful coeff reload */
	    sprintf(msg, "%s Complete", fmmap0[ii].name);
            /* Load filter names to EPICS */
            update_labels(ii);
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
                   update_labels(ii);
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

      /* Check for history reset */
      if(swRset[ii]&2)
%%      {
	  /* set switch back to zero */
%%	  /*printf("resetting history for module %d\n",ii);*/
%%	  pFilt->inputs[ii].rset = 2;
	  swRset[ii] = 0;
	  pvPut(swRset[ii]);
%%      }

%%    thresholdFlag = 0;                                                 /* MA */
%%    matrixThreshCount = 0;                                             /* MA */

      /* Get filter switch selections */
      pvGet(sw1[ii]);
      pvGet(sw2[ii]);	

      /* Get saved switch settings, used for burt purposes */
      pvGet(saveSwitch1[ii]);  
      pvGet(saveSwitch2[ii]);  
      if(sw1[ii]) {
        saveSwitch1[ii] ^= sw1[ii];
      	sw1[ii] = 0;
      	pvPut(sw1[ii]);  
      	pvPut(saveSwitch1[ii]);  
      }
      if(sw2[ii])
%%    {
        saveSwitch2[ii] ^= sw2[ii];
      	sw2[ii] = 0;
      	pvPut(sw2[ii]);  
      	pvPut(saveSwitch2[ii]);  
%%    }

/* %%    if (saveSwitch1[ii] != thresholds[ii].switch1) {                   MA */
/* %%       thresholdFlag += 1;                                             MA */
/* %%    }                                                                  MA */
/* %%    if (saveSwitch2[ii] != thresholds[ii].switch2) {                   MA */
/* %%       thresholdFlag += 2;                                             MA */
/* %%    }                                                                  MA */

#if defined(SEQUENCER_NAME_ascmc) || defined(SEQUENCER_NAME_asc) || defined(SEQUENCER_NAME_asc40m)
      /* check for WFS input group change selections */
%%    if(ii< 4*FILT_QUAD_SIZE){
%% 	static const int filton1 = 0x10;
%% 	static const int filton2 = 0x40;
%%      if(filt[ii/4]&1){
%%        saveSwitch1[ii] ^= filton1;
          pvPut(saveSwitch1[ii]);
%%      }
%%      if((filt[ii/4]&2)==2){
%%        saveSwitch1[ii] ^= filton2;
          pvPut(saveSwitch1[ii]);
%%      }
%%      if((ii%4)==3){
%%        filt[ii/4]=0;
          pvPut(filt[ii/4]);
%%      }
%%    }
#endif


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
	pvGet(offset[ii]);

/* %%    if ( (offset[ii] < thresholds[ii].offset[LOW]) ||                  MA */
/* %%         (offset[ii] > thresholds[ii].offset[HI]) ) {                  MA */
/* %%       thresholdFlag += 4;                                             MA */
/* %%    }                                                                  MA */

%%	rfm_assign(pFilt->inputs[ii].offset, offset[ii]);
	pvGet(outgain[ii]);

/* %%    if ( (outgain[ii] < thresholds[ii].gain[LOW]) ||                   MA */
/* %%         (outgain[ii] > thresholds[ii].gain[HI]) ) {                   MA */
/* %%       thresholdFlag += 8;                                             MA */
/* %%    }                                                                  MA */

%%	rfm_assign(pFilt->inputs[ii].outgain, outgain[ii]);
	pvGet(gain_ramp_time[ii]);

/* %%    if ( (gain_ramp_time[ii] < thresholds[ii].rampTime[LOW]) ||        MA */
/* %%         (gain_ramp_time[ii] > thresholds[ii].rampTime[HI]) ) {        MA */
/* %%       thresholdFlag += 16;                                            MA */
/* %%    }                                                                  MA */

%%	rfm_assign(pFilt->inputs[ii].gain_ramp_time, gain_ramp_time[ii]);
	pvGet(limit[ii]);

/* %%    if ( (limit[ii] < thresholds[ii].limit[LOW]) ||                    MA */
/* %%         (limit[ii] > thresholds[ii].limit[HI]) ) {                    MA */
/* %%       thresholdFlag += 32;                                            MA */
/* %%    }                                                                  MA */

%%	rfm_assign(pFilt->inputs[ii].limiter, limit[ii]);
%%	t = fpvalidate(pFilt->data[ii].testpoint);
	tp[ii] = t;
	/*tp[ii] = pFilt->data[ii].testpoint;*/
%%	t = fpvalidate(pFilt->data[ii].output16Hz)/* * conv*/;
	out16Hz[ii] = t;
%%	t = fpvalidate(pFilt->data[ii].output);
	output[ii] = t;
	pvPut(tp[ii]);
	pvPut(out16Hz[ii]);
	pvPut(output[ii]);

%%      if (thresholdFlag) {                                             /* MA */
%%         statErrCnt++;                                                 /* MA */
%%         if (displayCount < 6) {                                       /* MA */
%%            sprintf(staterr[displayCount], "%2d: ", thresholdFlag);    /* MA */
%%            strcat(staterr[displayCount], fmmap0[ii].name);            /* MA */
%%            displayCount++;                                            /* MA */
%%         }                                                             /* MA */
%%      }                                                                /* MA */

%%      thisFilterStatusCount = 0;                                       /* MA */
%%      strcpy(fltrParamHelp, "------");                                 /* MA */
%%      strcpy(fltrSevHelp, "------");                                   /* MA */

%AUPDT%                                                                  /* MA */

%%      if (thisFilterStatusCount) {                                     /* MA */
%%         filterStatusCount++;                                          /* MA */
 
%%         if (filterDisplayCount < 10) {                                /* MA */
%%            strcpy(filterErrId[filterDisplayCount], fmmap0[ii].name);  /* MA */
%%            strcpy(filterParam[filterDisplayCount], fltrParamHelp);    /* MA */
%%            strcpy(filterSeverity[filterDisplayCount], fltrSevHelp);   /* MA */
%%            filterDisplayCount++;                                      /* MA */
%%         }                                                             /* MA */
%%      }                                                                /* MA */

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

%% if (statusFlag) {                                                     /* MA */
      pvPut(statErrCnt);                                                 /* MA */
      for (ii = 0; ii < 6; ii++) {                                       /* MA */
         pvPut(staterr[ii]);                                             /* MA */
      }                                                                  /* MA */
%% }                                                                     /* MA */

%DECL4%

      if (matrixStatusFlag) {                                            /* MA */
         pvPut(matrixThreshCount);                                       /* MA */
      }                                                                  /* MA */
      for (ii = 0; ii < 10; ii++) {                                      /* MA */
         pvPut(matrixErr[ii]);                                           /* MA */
      }                                                                  /* MA */

      pvPut(filterStatusCount);                                          /* MA */
      for (ii = 0; ii < 10; ii++) {                                      /* MA */
         pvPut(filterErrId[ii]);                                         /* MA */
         pvPut(filterParam[ii]);                                         /* MA */
         pvPut(filterSeverity[ii]);                                      /* MA */
      }                                                                  /* MA */

      pvPut(setpointStatusCount);                                        /* MA */
      for (ii = 0; ii < 10; ii++) {                                      /* MA */
         pvPut(setpointErrId[ii]);                                       /* MA */
         pvPut(setpointMajorCount[ii]);                                  /* MA */
         pvPut(setpointMinorCount[ii]);                                  /* MA */
      }                                                                  /* MA */
%% time_t lt = time(0);
%% strcpy(localtimestring, ctime(&lt));
%% localtimestring[strlen(localtimestring) - 1] = 0; /* Delete newline character */
   pvPut(localtimestring);


%% pthread_mutex_unlock (&seq_lock);
#ifndef NO_DAQ_IN_SKELETON
  }state daqProcess
#else
  }state monScreen
#endif
}

#ifndef NO_DAQ_IN_SKELETON
state daqInit
{
  when()
    {
        dcuId = atoi(macValueGet("sysnum"));
%%      if (dcuId < 0 || dcuId >= DCU_COUNT) {
%%        fprintf(stderr, "Invalid system number %d\n", dcuId);
%%        exit(1);
%%      }
        strcpy(site, macValueGet("site"));
        strcpy(ifo, macValueGet("ifo"));
	strcpy(daqsys, macValueGet("sysfile"));
%%      info = ((DAQ_INFO_BLOCK *)((base = (volatile void *)findRfmCard(0)) + DAQ_INFO_ADDRESS)) + dcuId;
	pvPut(dcuId);
%%    rfm_assign(pEpics->epicsInput.dcuId, dcuId);
%%      printf("daq info at 0x%lx\n",(long)info);
%%      memset(&infoTmp, 0, sizeof(infoTmp));
%%      info->reconfig = 0;
%%      if (loadDaqConfigFile(&infoTmp, site, ifo, daqsys)) {
%%         copy_info();
%%         printf("%s%s daqConfig info is at rfm offset 0x%x\n", ifo, daqsys, ((char *)info) - ((char *)base));
%%         info -> reconfig = 1;
%%         dcmsg = infoTmp.configFileCRC;
%%         chnCnt = infoTmp.numChans;
%%         dataSize = 0;
%%         { int i;
%%              for(i=0; i < infoTmp.numChans; i++) {
%%                      dataSize += infoTmp.tp[i].dataRate * DAQ_DATA_TYPE_SIZE(infoTmp.tp[i].dataType);
%%              }
%%         }
%%         dataSize /= 1000;
%%      } else {
%%         dcmsg = 0xffffffff;
%%         chnCnt = 0;
%%         dataSize = 0;
%%      }
        pvPut(dcmsg);
        pvPut(chnCnt);
        pvPut(dataSize);
    } state monScreen
}

state daqProcess
{
  when()
    {

%% pthread_mutex_lock (&seq_lock);

        pvGet(reload);
        if (reload)
%%        {
%%          info -> reconfig = 0;
%%          if (loadDaqConfigFile(&infoTmp, site, ifo, daqsys)) {
%%            copy_info();
%%            printf("%s%s daqConfig info is at rfm offset 0x%x\n", ifo, daqsys, ((char *)info) - ((char *)base))
;
%%            info -> reconfig = 1;
%%            dcmsg = infoTmp.configFileCRC;
%%            chnCnt = infoTmp.numChans;
%%            dataSize = 0;
%%            { int i;
%%              for(i=0; i < infoTmp.numChans; i++) {
%%                      dataSize += infoTmp.tp[i].dataRate * DAQ_DATA_TYPE_SIZE(infoTmp.tp[i].dataType);
%%              }
%%            }
%%            dataSize /= 1000;
%%          } else {
%%            dcmsg = 0xffffffff;
%%            chnCnt = 0;
%%            dataSize = 0;
%%          }
            pvPut(dcmsg);
            pvPut(chnCnt);
            pvPut(dataSize);
%%        }
        reload = 0;
        pvPut(reload);
%% pthread_mutex_unlock (&seq_lock);

    } state monScreen
}
#endif

}
