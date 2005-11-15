/* Read EPICS data from RFM */
inline void getEpicsData(int *pRfmData, int *pLocalData, int count,int reset)
{
  static UINT32 *rfmPtr;
  static UINT32 *localPtr;
  int ii;

  if(reset==0)
  {
    rfmPtr = (UINT32 *)pRfmData;
    localPtr = (UINT32 *)pLocalData;
  }

  for(ii=0;ii<count;ii++)
  {
    *localPtr = *rfmPtr;
    localPtr ++;
    rfmPtr ++;
  }
}

/* Write EPICS data to RFM */
inline void putEpicsData(int *pRfmData, int *pLocalData, int count,int reset)
{
  static int *rfmPtr;
  static int *localPtr;
  int ii;

  if(reset==0)
  {
    rfmPtr = (int *)pRfmData;
    localPtr = (int *)pLocalData;
  }

  for(ii=0;ii<count;ii++)
  {
    *rfmPtr = *localPtr;
    localPtr ++;
    rfmPtr ++;
  }
}

/************************************************************************/
/* TASK: updateEpics()                                          */
/*      Initiates/performs functions required on particular clock cycle */
/*      counts. Due to performace reqs of LSC, do not want to do more   */
/*      than one housekeeping I/O function per LSC cycle. Resulting     */
/*      update rate to/from EPICS is 16Hz.                              */
/************************************************************************/
inline int updateEpics(int subcycle,
			int *epicsInAddShm,
			int *epicsOutAddShm,
			int *epicsInAddLoc,
			int *epicsOutAddLoc,
			FILT_MOD *dsp,
			FILT_MOD *pDsp,
			COEF *dspCoeff,
			VME_COEF *pCoeff,
			CDS_EPICS *plocalEpics){

int ii;

  ii = subcycle;
  /* Get EPICS input variables */
  if((ii >= 0) && (ii < (EPICS_IN_SIZE)))
          getEpicsData(epicsInAddShm, epicsInAddLoc,1,ii);
  ii -= EPICS_IN_SIZE;

  /* Send EPICS output variables */
  if((ii >= 0) && (ii < (EPICS_OUT_SIZE)))
          putEpicsData(epicsOutAddShm, epicsOutAddLoc,1,ii);
  ii -= EPICS_OUT_SIZE;

  /* Check for new filter coeffs or history resets */
  if((ii >= 0) && (ii < MAX_MODULES))
        checkFiltReset(ii, dsp, pDsp, dspCoeff, MAX_MODULES, pCoeff);
  ii -= MAX_MODULES;

  /* Get filter switch settings from EPICS */
  if((ii >= 0) && (ii < MAX_MODULES))
        dsp->inputs[ii].opSwitchE = pDsp->inputs[ii].opSwitchE;
  ii -= MAX_MODULES;

  /* Send filter module input data to EPICS */
  if((ii >= 0) && (ii < MAX_MODULES))
{
        pDsp->data[ii].filterInput = dsp->data[ii].filterInput;
        pDsp->data[ii].exciteInput = dsp->data[ii].exciteInput;
}
  ii -= MAX_MODULES;

  /* Send filter module 16Hz filtered output to EPICS */
  if((ii >= 0) && (ii < MAX_MODULES))
{
        pDsp->data[ii].output16Hz = dsp->data[ii].output16Hz;
        pDsp->data[ii].output = dsp->data[ii].output;
}
  ii -= MAX_MODULES;

  /* Send filter module test point output data to EPICS */
  if((ii >= 0) && (ii < MAX_MODULES))
{
        pDsp->data[ii].testpoint = dsp->data[ii].testpoint;
        dsp->inputs[ii].gain_ramp_time = pDsp->inputs[ii].gain_ramp_time;
}
  ii -= MAX_MODULES;

  /* Send filter module switch settings to EPICS */
  if((ii >= 0) && (ii < MAX_MODULES))
{
        pDsp->inputs[ii].opSwitchP = dsp->inputs[ii].opSwitchP;
        dsp->inputs[ii].offset = pDsp->inputs[ii].offset;
}
  ii -= MAX_MODULES;

  /* Get filter module gain settings from EPICS */
  if((ii >= 0) && (ii < MAX_MODULES))
{
        dsp->inputs[ii].outgain = pDsp->inputs[ii].outgain;
        dsp->inputs[ii].limiter = pDsp->inputs[ii].limiter;
}
  ii -= MAX_MODULES;

     if (plocalEpics->epicsInput.vmeReset) {
        printf("VME_RESET PUSHED !!! \n");
        gsaAdcStop();
        return(1);
     }
  ii -= 1;

#ifdef TODO
  /* Check for overflow counter reset command */
  if(ii==0) {
          if(pLocal_epics->overflowReset == 1)
          {
                pLocal_epics->ovAccum = 0;
                pLocalEpicsRfm->overflowReset = 0;
          }
        if(pLocal_epics->ovAccum > 10000000) pLocal_epics->ovAccum = 0;
  }
#endif

  return(0);

}


