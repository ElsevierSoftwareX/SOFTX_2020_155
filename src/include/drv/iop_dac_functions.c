inline int iop_dac_init(int []);
inline int iop_dac_preload(volatile GSC_DAC_REG *[]);
inline int iop_dac_write(void);

inline int iop_dac_init(int errorPend[])
{
  int ii,jj;
  int status;

  /// \> Zero out DAC outputs
  for (ii = 0; ii < MAX_DAC_MODULES; ii++)
  {
    errorPend[ii] = 0;
    for (jj = 0; jj < 16; jj++) {
 		dacOut[ii][jj] = 0.0;
 		dacOutUsed[ii][jj] = 0;
		dacOutBufSize[ii] = 0;
		// Zero out DAC channel map in the shared memory
		// to be used to check on slaves' channel allocation
		ioMemData->dacOutUsed[ii][jj] = 0;
    }
  }

  for(jj = 0; jj < cdsPciModules.dacCount; jj++) {
  	pLocalEpics->epicsOutput.statDac[jj] = DAC_FOUND_BIT;
	// Arm DAC DMA for full data size
	if(cdsPciModules.dacType[jj] == GSC_16AO16) {
		status = gsc16ao16DmaSetup(jj);
	} else if (cdsPciModules.dacType[jj] == GSC_20AO8){
	         status = gsc20ao8DmaSetup(jj);
	} else {
		status = gsc18ao8DmaSetup(jj);
	}
  }

  return 0;
}


inline int iop_dac_preload(volatile GSC_DAC_REG *dacReg[])
{
	volatile GSA_18BIT_DAC_REG *dac18Ptr;
	volatile GSA_20BIT_DAC_REG *dac20Ptr;
	volatile GSC_DAC_REG *dac16Ptr;
	int ii,jj;

		for(jj=0;jj<cdsPciModules.dacCount;jj++)
		{       
			if(cdsPciModules.dacType[jj] == GSC_18AO8)
			{       
				dac18Ptr = (volatile GSA_18BIT_DAC_REG *)(dacReg[jj]);
				for(ii=0;ii<GSAO_18BIT_PRELOAD;ii++) dac18Ptr->OUTPUT_BUF = 0;
			} else if(cdsPciModules.dacType[jj] == GSC_20AO8) {
		        dac20Ptr = (volatile GSA_20BIT_DAC_REG *)(dacReg[jj]);
		        for(ii=0;ii<GSAO_20BIT_PRELOAD;ii++) dac20Ptr->OUTPUT_BUF = 0;
			}else{  
				dac16Ptr = dacReg[jj];
				for(ii=0;ii<GSAO_16BIT_PRELOAD;ii++) dac16Ptr->ODB = 0;
			}       
		}       
		return 0;
}


inline int iop_dac_write()
{
	unsigned int *pDacData; 
	int ii,jj,mm;
	int limit;     
	int mask;          
	int num_outs;
	int status = 0;

/// START OF IOP DAC WRITE ***************************************** \n
        /// \> If DAC FIFO error, always output zero to DAC modules. \n
        /// - -- Code will require restart to clear.
        // COMMENT OUT NEX LINE FOR TEST STAND w/bad DAC cards. 
#ifndef DAC_WD_OVERRIDE
	/// \> Loop thru all DAC modules
	for(jj=0;jj<cdsPciModules.dacCount;jj++)
    {
		/// - -- Point to DAC memory buffer
       	pDacData = (unsigned int *)(cdsPciModules.pci_dac[jj]);
       	/// - -- locate the proper DAC memory block
       	mm = cdsPciModules.dacConfig[jj];
       	/// - -- Determine if memory block has been set with the correct cycle count by Slave app.
		if(ioMemData->iodata[mm][ioMemCntrDac].cycle == ioClockDac)
		{
			dacEnable |= pBits[jj];
		}else {
			dacEnable &= ~(pBits[jj]);
			dacChanErr[jj] += 1;
		}
		/// - -- Set overflow limits, data mask, and chan count based on DAC type
        limit = OVERFLOW_LIMIT_16BIT;
        mask = GSAO_16BIT_MASK;
        num_outs = GSAO_16BIT_CHAN_COUNT;
        if (cdsPciModules.dacType[jj] == GSC_18AO8) {
       		limit = OVERFLOW_LIMIT_18BIT; // 18 bit limit
            mask = GSAO_18BIT_MASK;
            num_outs = GSAO_18BIT_CHAN_COUNT;
		}
		if (cdsPciModules.dacType[jj] == GSC_20AO8) {
       		limit = OVERFLOW_LIMIT_20BIT; // 20 bit limit
            mask = GSAO_20BIT_MASK;
            num_outs = GSAO_20BIT_CHAN_COUNT;
		}
		/// - -- For each DAC channel
        for (ii=0; ii < num_outs; ii++)
        {
#ifdef FLIP_SIGNALS
       		dacOut[jj][ii] *= -1;
#endif
			/// - ---- Read DAC output value from shared memory and reset memory to zero
            if((!dacChanErr[jj]) && (iopDacEnable)) {
           		dac_out = ioMemData->iodata[mm][ioMemCntrDac].data[ii];
                /// - --------- Zero out data in case user app dies by next cycle
                /// when two or more apps share same DAC module.
                ioMemData->iodata[mm][ioMemCntrDac].data[ii] = 0;
    		} else {
				dac_out = 0;
				status = 1;
			}
            /// - ----  Write out ADC duotone signal to first DAC, last channel, 
			/// if DAC duotone is enabled.
			if((dt_diag.dacDuoEnable) && (ii==(num_outs-1)) && (jj == 0))
            {
           		dac_out = adcinfo.adcData[0][ADC_DUOTONE_CHAN];
      		}
// Code below is only for use in DAQ test system.
#ifdef DIAG_TEST
                        if((ii==0) && (jj == 0))
                        {
                                if(cycleNum < 100) dac_out = limit / 20;
                                else dac_out = 0;
                        }
                        if((ii==0) && (jj == 2))
                        {
                                if(cycleNum < 100) dac_out = limit / 20;
                                else dac_out = 0;
                        }
#endif
                        /// - ---- Check output values are within range of DAC \n
                        /// - --------- If overflow, clip at DAC limits and report errors
			if(dac_out > limit || dac_out < -limit)
            {
           		dacinfo.overflowDac[jj][ii] ++;
				pLocalEpics->epicsOutput.overflowDacAcc[jj][ii] ++;
                overflowAcc ++;
                dacOF[jj] = 1;
				odcStateWord |= ODC_DAC_OVF;;
				if(dac_out > limit) dac_out = limit;
				else dac_out = -limit;
        	}
           	/// - ---- If DAQKILL tripped, set output to zero.
            if(!iopDacEnable) dac_out = 0;
       		/// - ---- Load last values to EPICS channels for monitoring on GDS_TP screen.
           		dacOutEpics[jj][ii] = dac_out;

                /// - ---- Load DAC testpoints
                floatDacOut[16*jj + ii] = dac_out;

                /// - ---- Write to DAC local memory area, for later xmit to DAC module
                *pDacData =  (unsigned int)(dac_out & mask);
                pDacData ++;
			}
            /// - -- Mark cycle count as having been used -1 \n
            /// - --------- Forces slaves to mark this cycle or will not be used again by Master
            ioMemData->iodata[mm][ioMemCntrDac].cycle = -1;
            /// - -- DMA Write data to DAC module
            if(dacWriteEnable > 4) {
           		if(cdsPciModules.dacType[jj] == GSC_16AO16) {
                                gsc16ao16DmaStart(jj);
			 	} else if(cdsPciModules.dacType[jj] == GSC_20AO8) {
					            gsc20ao8DmaStart(jj);
                } else {
                                gsc18ao8DmaStart(jj);
                }
        	}
	}
    /// \> Increment DAC memory block pointers for next cycle
    ioClockDac = (ioClockDac + 1) % IOP_IO_RATE;
    ioMemCntrDac = (ioMemCntrDac + 1) % IO_MEMORY_SLOTS;
    if(dacWriteEnable < 10) dacWriteEnable ++;
/// END OF IOP DAC WRITE *************************************************
#endif
	return status;

}


inline int check_dac_buffers (int cycleNum)
{
	volatile GSA_18BIT_DAC_REG *dac18bitPtr;
	volatile GSA_20BIT_DAC_REG *dac20bitPtr;
	int out_buf_size = 0;
	int jj = 0;
	int status = 0;

		jj = cycleNum - HKP_DAC_FIFO_CHK;
		if(cdsPciModules.dacType[jj] == GSC_18AO8)
		{
			dac18bitPtr = (volatile GSA_18BIT_DAC_REG *)(dacPtr[jj]);
			out_buf_size = dac18bitPtr->OUT_BUF_SIZE;
			dacOutBufSize[jj] = out_buf_size;
			if(!dacTimingError) {
				if((out_buf_size < 8) || (out_buf_size > 24))
				{
				    pLocalEpics->epicsOutput.statDac[jj] &= ~(DAC_FIFO_BIT);
				    if(dacTimingErrorPending[jj]) dacTimingError = 1;
				    dacTimingErrorPending[jj] = 1;
				} else {
				    pLocalEpics->epicsOutput.statDac[jj] |= DAC_FIFO_BIT;
				    dacTimingErrorPending[jj] = 0;
				}
			}
			if(out_buf_size < 4) {
			    pLocalEpics->epicsOutput.statDac[jj] |= DAC_FIFO_EMPTY;
			} else { 
			    pLocalEpics->epicsOutput.statDac[jj] &= ~(DAC_FIFO_EMPTY);
			}
			if(out_buf_size > 32) {
			    pLocalEpics->epicsOutput.statDac[jj] |= DAC_FIFO_FULL;
			} else { 
			    pLocalEpics->epicsOutput.statDac[jj] &= ~(DAC_FIFO_FULL);
			}

		}
		if(cdsPciModules.dacType[jj] == GSC_20AO8)
        {
            dac20bitPtr = (volatile GSA_20BIT_DAC_REG *)(dacPtr[jj]);
            out_buf_size = dac20bitPtr->OUT_BUF_SIZE;
            dacOutBufSize[jj] = out_buf_size;
            if((out_buf_size > 24))
            {
                pLocalEpics->epicsOutput.statDac[jj] &= ~(DAC_FIFO_BIT);
                if(dacTimingErrorPending[jj]) dacTimingError = 1;
                dacTimingErrorPending[jj] = 1;
            } else {
                pLocalEpics->epicsOutput.statDac[jj] |= DAC_FIFO_BIT;
                dacTimingErrorPending[jj] = 0;
            }
		}
		if(cdsPciModules.dacType[jj] == GSC_16AO16)
		{
			status = gsc16ao16CheckDacBuffer(jj);
			dacOutBufSize[jj] = status;
			if(!dacTimingError) {
				if(status != 2)
				{
				    pLocalEpics->epicsOutput.statDac[jj] &= ~(DAC_FIFO_BIT);
				    if(dacTimingErrorPending[jj]) dacTimingError = 1;
				    dacTimingErrorPending[jj] = 1;
				} else {
				    pLocalEpics->epicsOutput.statDac[jj] |= DAC_FIFO_BIT;
				    dacTimingErrorPending[jj] = 0;
				}
			}
			if(status & 1) {
			    pLocalEpics->epicsOutput.statDac[jj] |= DAC_FIFO_EMPTY;
			} else { 
			    pLocalEpics->epicsOutput.statDac[jj] &= ~(DAC_FIFO_EMPTY);
			}
			if(status & 8) {
			    pLocalEpics->epicsOutput.statDac[jj] |= DAC_FIFO_FULL;
			} else { 
			    pLocalEpics->epicsOutput.statDac[jj] &= ~(DAC_FIFO_FULL);
			}
			if(status & 4) {
			    pLocalEpics->epicsOutput.statDac[jj] |= DAC_FIFO_HI_QTR;
			} else { 
			    pLocalEpics->epicsOutput.statDac[jj] &= ~(DAC_FIFO_HI_QTR);
			}
		}
		return 0;
}
