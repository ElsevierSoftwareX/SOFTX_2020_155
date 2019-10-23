inline int
waitPcieTimingSignal( TIMING_SIGNAL* timePtr, int cycle )
{
    int loop = 0;

    do
    {
        udelay( 1 );
        loop++;
    } while ( timePtr->cycle != cycle && loop < 18 );
    if ( loop >= 18 )
        return ( 1 );
    else
        return ( 0 );
}
inline unsigned int
sync2master( TIMING_SIGNAL* timePtr )
{
    int loop = 0;
    int cycle = 0;

    do
    {
        udelay( 5 );
        loop++;
    } while ( timePtr->cycle != cycle && loop < 1000000 );
    if ( loop >= 1000000 )
    {
        return ( -1 );
    } else {
        return ( timePtr->gps_time );
    }
}

inline int
iop_adc_init( adcInfo_t* adcinfo )
{
    volatile int* adcDummyData;
    int           status;
    int           ii, jj;

    /// \> If IOP,  Initialize the ADC modules
    for ( jj = 0; jj < cdsPciModules.adcCount; jj++ )
    {
        pLocalEpics->epicsOutput.statAdc[ jj ] = 1;
        // Reset Diag Info
        adcinfo->adcRdTimeErr[ jj ] = 0;
        adcinfo->adcChanErr[ jj ] = 0;
        adcinfo->adcOF[ jj ] = 0;
        for ( ii = 0; ii < MAX_ADC_CHN_PER_MOD; ii++ )
            adcinfo->overflowAdc[ jj ][ ii ] = 0;
    }
    adcinfo->adcHoldTime = 0;
    adcinfo->adcHoldTimeMax = 0;
    adcinfo->adcHoldTimeEverMax = 0;
    adcinfo->adcHoldTimeEverMaxWhen = 0;
    adcinfo->adcHoldTimeMin = 0xffff;
    adcinfo->adcHoldTimeAvg = 0;
    adcinfo->adcWait = 0;
    adcinfo->chanHop = 0;

    return 0;
}

inline int
iop_adc_read( adcInfo_t* adcinfo, int cpuClk[] )
{
    int           kk;
    volatile int* packedData;
    int           limit;
    int           mask;
    unsigned int  offset;
    int           num_outs;
    int           ioMemCntr = 0;
    int           adcStat = 0;
    int           card;
    int           chan;
    int           missedCycle;

    // Read ADC data
    missedCycle = waitPcieTimingSignal( pcieTimer, cycleNum );
    timeSec = pcieTimer->gps_time;

    for ( card = 0; card < cdsPciModules.adcCount; card++ )
    {
        /// - ---- ADC DATA RDY is detected when last channel in memory no
        /// longer contains the dummy variable written during initialization and
        /// reset after the read.
        packedData = (int*)cdsPciModules.pci_adc[ card ];

        cpuClk[ CPU_TIME_RDY_ADC ] = rdtsc_ordered( );
        cpuClk[ CPU_TIME_ADC_WAIT ] = rdtsc_ordered( );
        adcinfo->adcWait =
            ( cpuClk[ CPU_TIME_ADC_WAIT ] - cpuClk[ CPU_TIME_RDY_ADC ] ) /
            CPURATE;

        /// - ---- Added ADC timing diagnostics to verify timing consistent and
        /// all rdy together.
        if ( card == 0 )
        {
            adcinfo->adcRdTime[ card ] = ( cpuClk[ CPU_TIME_ADC_WAIT ] -
                                           cpuClk[ CPU_TIME_CYCLE_START ] ) /
                CPURATE;
            if ( adcinfo->adcRdTime[ card ] > 1000 )
                adcStat = ADC_BUS_DELAY;
            if ( adcinfo->adcRdTime[ card ] < 13 )
                adcStat = ADC_SHORT_CYCLE;
        }
        else
        {
            adcinfo->adcRdTime[ card ] = adcinfo->adcWait;
        }

        if ( adcinfo->adcRdTime[ card ] > adcinfo->adcRdTimeMax[ card ] )
            adcinfo->adcRdTimeMax[ card ] = adcinfo->adcRdTime[ card ];

        // if((card==0) && (adcinfo->adcRdTimeMax[card] > MAX_ADC_WAIT_CARD_0))
        if ( ( card == 0 ) && ( adcinfo->adcRdTime[ card ] > 20 ) )
            adcinfo->adcRdTimeErr[ card ]++;

        if ( ( card != 0 ) &&
             ( adcinfo->adcRdTimeMax[ card ] > MAX_ADC_WAIT_CARD_S ) )
            adcinfo->adcRdTimeErr[ card ]++;

        /// - --------- If data not ready in time, abort.
        /// Either the clock is missing or code is running too slow and ADC FIFO
        /// is overflowing.
        if ( adcinfo->adcWait >= MAX_ADC_WAIT )
        {
            // printk("timeout %d %d \n",jj,adcinfo->adcWait);
            pLocalEpics->epicsOutput.stateWord = FE_ERROR_ADC;
            pLocalEpics->epicsOutput.diagWord |= ADC_TIMEOUT_ERR;
            pLocalEpics->epicsOutput.fe_status = ADC_TO_ERROR;
            stop_working_threads = 1;
            vmeDone = 1;
            continue;
            // return 0;
        }

        if ( card == 0 )
        {
            // Capture cpu clock for cpu meter diagnostics
            cpuClk[ CPU_TIME_CYCLE_START ] = rdtsc_ordered( );
            /// \> If first cycle of a new second, capture IRIG-B time. Standard
            /// for aLIGO is TSYNC_RCVR.
            if ( cycleNum == 0 )
            {
                // if SymCom type, just do write to lock current time and read
                // later This save a couple three microseconds here
                if ( cdsPciModules.gpsType == SYMCOM_RCVR )
                    lockGpsTime( );
                if ( cdsPciModules.gpsType == TSYNC_RCVR )
                {
                    /// - ---- Reading second info will lock the time register,
                    /// allowing nanoseconds to be read later (on next cycle).
                    /// Two step process used to save CPU time here, as each
                    /// read can take 2usec or more.
                    timeSec = getGpsSecTsync( );
                }
            }
        }

        /// \> Read adc data
        packedData = (int*)cdsPciModules.pci_adc[ card ];

        limit = OVERFLOW_LIMIT_16BIT;
        // Various ADC models have different number of channels/data bits
        offset = GSAI_DATA_CODE_OFFSET;
        mask = GSAI_DATA_MASK;
        num_outs = GSAI_CHAN_COUNT;
        /// - ---- Determine next ipc memory location to load ADC data
        ioMemCntr = ( cycleNum % IO_MEMORY_SLOTS );

        /// - ----  Read adc data from PCI mapped memory into local variables
        for ( chan = 0; chan < num_outs; chan++ )
        {
            // adcData is the integer representation of the ADC data
            adcinfo->adcData[ card ][ chan ] = *packedData;
            // dWord is the double representation of the ADC data
            // This is the value used by the rest of the code calculations.
            dWord[ card ][ chan ][ kk ] = adcinfo->adcData[ card ][ chan ];
            /// - ----  Load ADC value into ipc memory buffer
            ioMemData->iodata[ card ][ ioMemCntr ].data[ chan ] =
                adcinfo->adcData[ card ][ chan ];
            /// - ---- Check for ADC overflows
            if ( ( adcinfo->adcData[ card ][ chan ] > limit ) ||
                 ( adcinfo->adcData[ card ][ chan ] < -limit ) )
            {
                adcinfo->overflowAdc[ card ][ chan ]++;
                pLocalEpics->epicsOutput.overflowAdcAcc[ card ][ chan ]++;
                overflowAcc++;
                adcinfo->adcOF[ card ] = 1;
                odcStateWord |= ODC_ADC_OVF;
            }
            packedData++;
        }

        /// - ---- Write GPS time and cycle count as indicator to slave that adc
        /// data is ready
        ioMemData->gpsSecond = timeSec;
        ioMemData->iodata[ card ][ ioMemCntr ].timeSec = timeSec;
        ioMemData->iodata[ card ][ ioMemCntr ].cycle = cycleNum;
    }
    return adcStat;
}

inline int
iop_dac_init( int errorPend[] )
{
    int ii, jj;
    int status;

    /// \> Zero out DAC outputs
    for ( ii = 0; ii < MAX_DAC_MODULES; ii++ )
    {
        errorPend[ ii ] = 0;
        for ( jj = 0; jj < 16; jj++ )
        {
            dacOut[ ii ][ jj ] = 0.0;
            dacOutUsed[ ii ][ jj ] = 0;
            dacOutBufSize[ ii ] = 0;
            // Zero out DAC channel map in the shared memory
            // to be used to check on slaves' channel allocation
            ioMemData->dacOutUsed[ ii ][ jj ] = 0;
        }
    }

    for ( jj = 0; jj < cdsPciModules.dacCount; jj++ )
    {
        pLocalEpics->epicsOutput.statDac[ jj ] = DAC_FOUND_BIT;
    }

    return 0;
}

inline int
iop_dac_write( )
{
    unsigned int* pDacData;
    int           ii, jj, mm;
    int           limit;
    int           mask;
    int           num_outs;
    int           status = 0;

    /// WRITE DAC OUTPUTS ***************************************** \n

    /// Writing of DAC outputs is dependent on code compile option: \n
    /// - -- IOP (ADC_MASTER) reads DAC output values from memory shared with
    /// user apps and writes to DAC hardware. \n
    /// - -- USER APP (ADC_SLAVE) sends output values to memory shared with IOP.
    /// \n

    /// START OF IOP DAC WRITE ***************************************** \n
    /// \> If DAC FIFO error, always output zero to DAC modules. \n
    /// - -- Code will require restart to clear.
    // COMMENT OUT NEX LINE FOR TEST STAND w/bad DAC cards.
    if ( dacTimingError )
        iopDacEnable = 0;
    // Write out data to DAC modules
    /// \> Loop thru all DAC modules
    for ( jj = 0; jj < cdsPciModules.dacCount; jj++ )
    {
        /// - -- locate the proper DAC memory block
        mm = cdsPciModules.dacConfig[ jj ];
        /// - -- Determine if memory block has been set with the correct cycle
        /// count by Slave app.
        if ( ioMemData->iodata[ mm ][ ioMemCntrDac ].cycle == ioClockDac )
        {
            dacEnable |= pBits[ jj ];
        }
        else
        {
            dacEnable &= ~( pBits[ jj ] );
            dacChanErr[ jj ] += 1;
        }
        /// - -- Set overflow limits, data mask, and chan count based on DAC
        /// type
        limit = OVERFLOW_LIMIT_16BIT;
        num_outs = GSAO_16BIT_CHAN_COUNT;
        if ( cdsPciModules.dacType[ jj ] == GSC_18AO8 )
        {
            limit = OVERFLOW_LIMIT_18BIT; // 18 bit limit
            num_outs = GSAO_18BIT_CHAN_COUNT;
        }
        if ( cdsPciModules.dacType[ jj ] == GSC_20AO8 )
        {
            limit = OVERFLOW_LIMIT_20BIT; // 20 bit limit
            num_outs = GSAO_20BIT_CHAN_COUNT;
        }
        /// - -- Point to DAC memory buffer
        pDacData = (unsigned int*)( cdsPciModules.pci_dac[ jj ] );
        // Advance to the correct point in the one second memory buffer
        pDacData += num_outs * cycleNum;
        /// - -- For each DAC channel
        for ( ii = 0; ii < num_outs; ii++ )
        {
            /// - ---- Read DAC output value from shared memory and reset memory
            /// to zero
            if ( ( !dacChanErr[ jj ] ) && ( iopDacEnable ) )
            {
                dac_out = ioMemData->iodata[ mm ][ ioMemCntrDac ].data[ ii ];
                /// - --------- Zero out data in case user app dies by next
                /// cycle when two or more apps share same DAC module.
                ioMemData->iodata[ mm ][ ioMemCntrDac ].data[ ii ] = 0;
            }
            else
            {
                dac_out = 0;
                status = 1;
            }
            /// - ---- Check output values are within range of DAC \n
            /// - --------- If overflow, clip at DAC limits and report errors
            if ( dac_out > limit || dac_out < -limit )
            {
                overflowDac[ jj ][ ii ]++;
                pLocalEpics->epicsOutput.overflowDacAcc[ jj ][ ii ]++;
                overflowAcc++;
                dacOF[ jj ] = 1;
                odcStateWord |= ODC_DAC_OVF;
                ;
                if ( dac_out > limit )
                    dac_out = limit;
                else
                    dac_out = -limit;
            }
            /// - ---- If DAQKILL tripped, set output to zero.
            if ( !iopDacEnable )
                dac_out = 0;
            /// - ---- Load last values to EPICS channels for monitoring on
            /// GDS_TP screen.
            dacOutEpics[ jj ][ ii ] = dac_out;

            /// - ---- Load DAC testpoints
            floatDacOut[ 16 * jj + ii ] = dac_out;

            /// - ---- Write to DAC local memory area, for later xmit to DAC
            /// module
            *pDacData = (unsigned int)( dac_out );
            pDacData++;
        }
        /// - -- Mark cycle count as having been used -1 \n
        /// - --------- Forces slaves to mark this cycle or will not be used
        /// again by Master
        ioMemData->iodata[ mm ][ ioMemCntrDac ].cycle = -1;
        /// - -- DMA Write data to DAC module
    }
    /// \> Increment DAC memory block pointers for next cycle
    ioClockDac = ( ioClockDac + 1 ) % IOP_IO_RATE;
    ioMemCntrDac = ( ioMemCntrDac + 1 ) % IO_MEMORY_SLOTS;
    if ( dacWriteEnable < 10 )
        dacWriteEnable++;

    return status;
}
