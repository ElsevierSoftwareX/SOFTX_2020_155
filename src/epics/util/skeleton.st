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
assign msg to "{ifo}:{sys}-{subsys}MSG";

int allNewCoeff;
assign allNewCoeff to "{ifo}:{sys}-{subsys}LOAD_NEW_COEFF";

#ifndef NO_DAQ_IN_SKELETON

int reload;
assign reload to "{ifo}:DAQ-{sys}_{subsys}LOAD_CONFIG";

int dcmsg;
assign dcmsg to "{ifo}:DAQ-{sys}_{subsys}MSG";

int chnCnt;
assign chnCnt to "{ifo}:DAQ-{sys}_{subsys}CHAN_CNT";

int dataSize;
assign dataSize to "{ifo}:DAQ-{sys}_{subsys}TOTAL";

int dcuId;
assign dcuId to "{ifo}:DAQ-{sys}_{subsys}DCU_ID";
#endif


%DECL2%

%% static fmReadCoeff fmc = {
%%  "lho", "h1", "xxx", 0, {{"", "", MAX_MODULES, fmmap0, 0}},
%% };

%% static const float conv = 0.000152588;
%% static const int mask[2] = {0xAAA0,0x10AA};
%% extern pthread_mutex_t seq_lock;

#if defined(RFM_EPICS)
%% static RFM_FE_COMMS *pRfm;
%% static VME_COEF *pVmeCoeff;
%% static FILT_MOD *pFilt;
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

#ifdef SEQUENCER_NAME_lsc
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

#if defined(RFM_EPICS)
    sysnum = atoi(macValueGet("sysnum"));

#if defined(SEQUENCER_NAME_hepi) || defined(SEQUENCER_NAME_hepia)
    cpunum = 0;
%%  if(sysnum > 3) cpunum = 1;
%%  if(sysnum == 7) cpunum = 2;
%%  if(sysnum == 8) cpunum = 3;
#endif
#endif

#if defined(SEQUENCER_NAME_tcsh1) || defined(SEQUENCER_NAME_tcsh2)
    tcsEnable = 0;
#endif

    strncpy(fmc.site, macValueGet("site"), sizeof(fmc.site)-1);
    strncpy(fmc.ifo, macValueGet("ifo"), sizeof(fmc.ifo)-1);
    strncpy(fmc.system, macValueGet("sys"), sizeof(fmc.system)-1);
    if (macValueGet("subsys") != 0) {
      strncpy(fmc.subSys[0].name, macValueGet("subsys"), sizeof(fmc.subSys[0].name)-1);
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
%%  pRfm = (RFM_FE_COMMS *)(findRfmCard(0));
%%  pFilt = (FILT_MOD *)(&pRfm->%EPICS_FILT_VAR%);
%%  pVmeCoeff = (VME_COEF*)(&pRfm->%EPICS_COEFF_VAR%);
%%  pEpics = (%EPICS_TYPE%*)(&pRfm->%EPICS_EPICS_VAR%);
%%  printf("%s pFilt is at 0x%x\n", fmc.subSys[0].name, ((int)pFilt-(int)pRfm));
%%  printf("%s pVmeCoeff is at 0x%x\n", fmc.subSys[0].name, ((int)pVmeCoeff-(int)pRfm));
%%  printf("%s pVmeCoeff size is 0x%x\n", fmc.subSys[0].name, sizeof(*pVmeCoeff));
%%  printf("%s pEpics is at 0x%x\n", fmc.subSys[0].name, ((int)pEpics-(int)pRfm));
#endif

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

  when(delay(.2)){
%% int ii, jj;	

%% pthread_mutex_lock (&seq_lock);

   /* Coefficient update request */	
    pvGet(allNewCoeff);
    if(allNewCoeff)
%%    {
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
%%	       for (kk = 0; kk < 1000; kk++) {
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
%%		 usleep(100);
%%	       }
%%	       if (kk == 1000) {
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

#ifdef SEQUENCER_NAME_lsc
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
%%	    for (kk = 0; kk < 1000; kk++) {
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
%%	    if (kk == 1000) {
		strcpy(msg, "Error: front-end timeout");
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

%%	rfm_assign(pFilt->inputs[ii].offset, offset[ii]);
	pvGet(outgain[ii]);
%%	rfm_assign(pFilt->inputs[ii].outgain, outgain[ii]);
	pvGet(gain_ramp_time[ii]);
%%	rfm_assign(pFilt->inputs[ii].gain_ramp_time, gain_ramp_time[ii]);
	pvGet(limit[ii]);
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

%DECL4%

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
	strcpy(daqsys, fmc.system);
	if (fmc.subSys[0].name[0]) strcat(strcat(daqsys, "_"), fmc.subSys[0].name);
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
