/************************************************************************/
/* TASK: updateEpics()                                          */
/*      Initiates/performs functions required on particular clock cycle */
/*      counts. Due to performace reqs of LSC, do not want to do more   */
/*      than one housekeeping I/O function per LSC cycle. Resulting     */
/*      update rate to/from EPICS is 16Hz.                              */
/************************************************************************/
inline void updateEpics(int subcycle,
			FILT_MOD *dsp,
			FILT_MOD *pDsp,
			COEF *dspCoeff,
			VME_COEF *pCoeff)
{
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
        pDsp->inputs[ii].opSwitchP = dsp->inputs[ii].opSwitchP;
        dsp->inputs[ii].limiter = pDsp->inputs[ii].limiter;
        pDsp->inputs[ii].mask = dsp->inputs[ii].mask;
        pDsp->inputs[ii].control = dsp->inputs[ii].control;
	if (dsp->inputs[ii].mask & 0x20000000) { /* Offset controlled by the FE */
        	pDsp->inputs[ii].offset = dsp->inputs[ii].offset;
	} else {
        	dsp->inputs[ii].offset = pDsp->inputs[ii].offset;
	}
	if (dsp->inputs[ii].mask & 0x40000000) { /* Gain controlled by the FE */
        	pDsp->inputs[ii].outgain = dsp->inputs[ii].outgain;
	} else {
        	dsp->inputs[ii].outgain = pDsp->inputs[ii].outgain;
	}
	if (dsp->inputs[ii].mask & 0x80000000) { /* Ramp time controlled by the FE */
        	pDsp->inputs[ii].gain_ramp_time = dsp->inputs[ii].gain_ramp_time;
	} else {
        	dsp->inputs[ii].gain_ramp_time = pDsp->inputs[ii].gain_ramp_time;
	}
  }
}


inline int checkEpicsReset(int subcycle, CDS_EPICS *plocalEpics){
  int ii;

  ii = subcycle;

  if ((ii==MAX_MODULES) && (plocalEpics->epicsInput.vmeReset)) {
#ifndef NO_RTL
        //printf("VME_RESET PUSHED !!! \n");
#endif
#ifdef ADC_MASTER
	if (cdsPciModules.adcCount > 0) gsaAdcStop();
#endif
        return(1);
  }

  return(0);

}

int gainRamp(float gainReq, int rampTime, int id, float *gain, int gainRate)
{

static int dir[40];
static float inc[40];
static float gainFinal[40];
static float gainOut[40];

	if (rampTime <= 0) return 0;

        if(gainFinal[id] != gainReq)
        {
                inc[id] = rampTime * gainRate;
                inc[id] = (gainReq - gainOut[id]) / inc[id];
                if(inc[id] <= 0.0) dir[id] = 0;
                else dir[id] = 1;
                gainFinal[id] = gainReq;
        }
        if(gainFinal[id] == gainOut[id])
        {
		*gain = gainOut[id];
                return(0);
        }
        gainOut[id] += inc[id];
        if((dir[id] == 1) && (gainOut[id] >= gainFinal[id]))
                gainOut[id] = gainFinal[id];
        if((dir[id] == 0) && (gainOut[id] <= gainFinal[id]))
                gainOut[id] = gainFinal[id];
	*gain = gainOut[id];
        return(1);
}



