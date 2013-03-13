///	@file epicsXfer.c
///	@brief File contains routines for:
///<		- Exchanging filter module data with EPICS shared memory
///<		- Checking for process kill command from EPICS
///<		- Ramping algorithm, used by Product.pm part.
/************************************************************************/
///	Function to exchange filter module data with EPICS via shared memory.
///	@param[in] subcycle		Filter Module ID for data transfer.
///	@param[in] *dsp			Pointer to process memory fm data.
///	@param[in] *pDsp		Pointer to shared memory fm data.
///	@param[in] *dspCoeff		Pointer to process memory fm coeff data.
///	@param[in] *pCoeff		Pointer to shared memory fm coeff data.
/************************************************************************/
inline void updateEpics(int subcycle,
			FILT_MOD *dsp,
			FILT_MOD *pDsp,
			COEF *dspCoeff,
			VME_COEF *pCoeff)
{
int ii;

  ii = subcycle;
  if((ii >= 0) && (ii < MAX_MODULES))
  {
        checkFiltReset(ii, dsp, pDsp, dspCoeff, MAX_MODULES, pCoeff);
        // dsp->inputs[ii].opSwitchE = pDsp->inputs[ii].opSwitchE;
        pDsp->data[ii].filterInput = dsp->data[ii].filterInput;
        pDsp->data[ii].exciteInput = dsp->data[ii].exciteInput;
        pDsp->data[ii].output16Hz = dsp->data[ii].output16Hz;
        pDsp->data[ii].output = dsp->data[ii].output;
        pDsp->data[ii].testpoint = dsp->data[ii].testpoint;
        pDsp->inputs[ii].opSwitchP = dsp->inputs[ii].opSwitchP;
        // dsp->inputs[ii].limiter = pDsp->inputs[ii].limiter;
        pDsp->inputs[ii].mask = dsp->inputs[ii].mask;
        pDsp->inputs[ii].control = dsp->inputs[ii].control;
#if 0
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
#endif
  }
}
inline void updateFmSetpoints(
			FILT_MOD *dsp,
			FILT_MOD *pDsp,
			COEF *dspCoeff,
			VME_COEF *pCoeff)
{
int ii;

	for(ii=0;ii<MAX_MODULES;ii++)
	{
		dsp->inputs[ii].opSwitchE = pDsp->inputs[ii].opSwitchE;
		dsp->inputs[ii].limiter = pDsp->inputs[ii].limiter;
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


/// Check for process stop command from EPICS
///	@param[in] subcycle	Present code cycle
///	@param[in] *plocalEpics	Pointer to EPICS data in shared memory
inline int checkEpicsReset(int subcycle, CDS_EPICS *plocalEpics){
  int ii;

  ii = subcycle;

  if ((ii==MAX_MODULES) && (plocalEpics->epicsInput.vmeReset)) {
#ifdef ADC_MASTER
	if (cdsPciModules.adcCount > 0)  gsc16ai64AdcStop();
#endif
        return(1);
  }

  return(0);

}

///	Perform gain ramping
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



