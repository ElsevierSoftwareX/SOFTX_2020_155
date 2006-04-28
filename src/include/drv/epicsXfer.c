extern int gsaAdcStop();                        /* Stops ADC acquisition.               */

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
        dsp->inputs[ii].gain_ramp_time = pDsp->inputs[ii].gain_ramp_time;
        pDsp->inputs[ii].opSwitchP = dsp->inputs[ii].opSwitchP;
        dsp->inputs[ii].offset = pDsp->inputs[ii].offset;
        dsp->inputs[ii].outgain = pDsp->inputs[ii].outgain;
        dsp->inputs[ii].limiter = pDsp->inputs[ii].limiter;
  }
}


inline int checkEpicsReset(int subcycle, CDS_EPICS *plocalEpics){
  int ii;

  ii = subcycle;

  if ((ii==MAX_MODULES) && (plocalEpics->epicsInput.vmeReset)) {
        printf("VME_RESET PUSHED !!! \n");
        gsaAdcStop();
        return(1);
  }

  return(0);

}

int gainRamp(float gainReq, int rampTime, int id, float *gain)
{

static int dir[40];
static float inc[40];
static float gainFinal[40];
static float gainOut[40];

        if(gainFinal[id] != gainReq)
        {
                inc[id] = rampTime * 2048;
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



