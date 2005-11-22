extern int gsaAdcStop();                        /* Stops ADC acquisition.               */

/************************************************************************/
/* TASK: updateEpics()                                          */
/*      Initiates/performs functions required on particular clock cycle */
/*      counts. Due to performace reqs of LSC, do not want to do more   */
/*      than one housekeeping I/O function per LSC cycle. Resulting     */
/*      update rate to/from EPICS is 16Hz.                              */
/************************************************************************/
inline int updateEpics(int subcycle,
			FILT_MOD *dsp,
			FILT_MOD *pDsp,
			COEF *dspCoeff,
			VME_COEF *pCoeff,
			CDS_EPICS *plocalEpics){

int ii;

  ii = subcycle;
  /* Check for new filter coeffs or history resets */
  if((ii >= 0) && (ii < MAX_MODULES))
  {
        checkFiltReset(ii, dsp, pDsp, dspCoeff, MAX_MODULES, pCoeff);
        dsp->inputs[ii].opSwitchE = pDsp->inputs[ii].opSwitchE;
        pDsp->data[ii].filterInput = dsp->data[ii].filterInput;
        pDsp->data[ii].exciteInput = dsp->data[ii].exciteInput;
        pDsp->data[ii].output16Hz = dsp->data[ii].output16Hz;
        pDsp->data[ii].output = dsp->data[ii].output;
        pDsp->data[ii].testpoint = dsp->data[ii].testpoint;
        dsp->inputs[ii].gain_ramp_time = pDsp->inputs[ii].gain_ramp_time;
        pDsp->inputs[ii].opSwitchP = dsp->inputs[ii].opSwitchP;
        dsp->inputs[ii].offset = pDsp->inputs[ii].offset;
        dsp->inputs[ii].outgain = pDsp->inputs[ii].outgain;
        dsp->inputs[ii].limiter = pDsp->inputs[ii].limiter;
  }



  if ((ii==MAX_MODULES) && (plocalEpics->epicsInput.vmeReset)) {
        printf("VME_RESET PUSHED !!! \n");
        gsaAdcStop();
        return(1);
  }

  return(0);

}


