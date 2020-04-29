inline int iop_adc_init( adcInfo_t* );
inline int iop_adc_read( adcInfo_t*, int[] );
inline int sync_adc_2_1pps( void );

inline int
iop_adc_init( adcInfo_t* adcinfo )
{
    volatile int* adcDummyData;
    int           status;
    int           ii, jj;

    /// \> If IOP,  Initialize the ADC modules
    for ( jj = 0; jj < cdsPciModules.adcCount; jj++ )
    {
        // Setup the DMA registers
        if ( cdsPciModules.adcType[ jj ] == GSC_18AI32SSC1M )
        {
            status = gsc18ai32DmaSetup( jj );
        }
        else
        {
            status = gsc16ai64DmaSetup( jj );
        }
        // Preload input memory with dummy variables to test that new ADC data
        // has arrived.
        adcDummyData = (int*)cdsPciModules.pci_adc[ jj ];
        // Write a dummy 0 to first ADC channel location
        // This location should never be zero when the ADC writes data as it
        // should always have an upper bit set indicating channel 0.
        *adcDummyData = 0x0;
        // if (cdsPciModules.adcType[jj] == GSC_18AI32SSC1M)  adcDummyData +=
        // 63;
        if ( cdsPciModules.adcType[ jj ] == GSC_18AI32SSC1M )
            adcDummyData += GSAF_8_OFFSET;
        else
            adcDummyData += GSAI_64_OFFSET;
        // Write a number into the last channel which the ADC should never write
        // ie no upper bits should be set in channel 31.
        *adcDummyData = DUMMY_ADC_VAL;
        // Set ADC Present Flag
        pLocalEpics->epicsOutput.statAdc[ jj ] = ADC_MAPPED;
        // Set ADC AutoCal Pass/Fail Flag
        if((cdsPciModules.adcConfig [ jj ] & GSAI_AUTO_CAL_PASS) != 0) 
            pLocalEpics->epicsOutput.statAdc[ jj ] |= ADC_CAL_PASS;
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
sync_adc_2_1pps( )
{
    int           ii, jj, kk;
    int           status;
    volatile int* adcDummyData;
    // Arm ADC modules
    // This has to be done sequentially, one at a time.
    kk = 0;
    for ( jj = 0; jj < cdsPciModules.adcCount; jj++ )
    {
        adcDummyData = (int*)cdsPciModules.pci_adc[ 0 ];
        adcDummyData += 31;
        gsc16ai64Enable1PPS( jj );
        status = gsc16ai64WaitDmaDone( 0, adcDummyData );
        kk++;
        udelay( 2 );
        for ( ii = 0; ii < kk; ii++ )
        {
            gsc16ai64DmaEnable( ii );
        }
    }
    // Need to do some dummy reads here to allow time for last ADC to arm
    // as it takes two clock cycles past arm to actually deliver data.
    for ( ii = 0; ii < cdsPciModules.adcCount; ii++ )
    {
        // Want to verify ADC FIFOs are empty to ensure they are in sync.
        status = gsc16ai64WaitDmaDone( 0, adcDummyData );
        status = gsc16ai64CheckAdcBuffer( ii );
        for ( jj = 0; jj < cdsPciModules.adcCount; jj++ )
        {
            gsc16ai64DmaEnable( jj );
        }
    }
    return 0;
}

// ADC Read *****************************************************************
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
    int           loops = 1;
    int           iocycle = 0;

    // Read ADC data
    for ( card = 0; card < cdsPciModules.adcCount; card++ )
    {
        /// - ---- ADC DATA RDY is detected when last channel in memory no
        /// longer contains the dummy variable written during initialization and
        /// reset after the read.
        packedData = (int*)cdsPciModules.pci_adc[ card ];
        if ( cdsPciModules.adcType[ card ] == GSC_18AI32SSC1M )
            packedData += GSAF_8_OFFSET;
        else
            packedData += GSAI_64_OFFSET;

        cpuClk[ CPU_TIME_RDY_ADC ] = rdtsc_ordered( );
        do
        {
            /// - ---- Need to delay if not ready as constant banging of the
            /// input register will slow down the ADC DMA.
            cpuClk[ CPU_TIME_ADC_WAIT ] = rdtsc_ordered( );
            adcinfo->adcWait =
                ( cpuClk[ CPU_TIME_ADC_WAIT ] - cpuClk[ CPU_TIME_RDY_ADC ] ) /
                CPURATE;
            /// - ---- Allow 1sec for data to be ready (should never take that
            /// long).
        } while ( ( *packedData == DUMMY_ADC_VAL ) &&
                  ( adcinfo->adcWait < MAX_ADC_WAIT ) );

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
#ifdef TIME_MASTER
            pcieTimer->gps_time = timeSec;
            pcieTimer->cycle = cycleNum;
            clflush_cache_range( (void*)&pcieTimer->gps_time, 16 );
#endif
        }
        else
        {
            adcinfo->adcRdTime[ card ] = adcinfo->adcWait;
        }

        if ( adcinfo->adcRdTime[ card ] > adcinfo->adcRdTimeMax[ card ] )
            adcinfo->adcRdTimeMax[ card ] = adcinfo->adcRdTime[ card ];

        if ( ( card == 0 ) &&
             ( adcinfo->adcRdTimeMax[ card ] >
               ( MAX_ADC_WAIT_CARD_0 * UNDERSAMPLE ) ) )
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
            fe_status_return = ADC_TO_ERROR;
            fe_status_return_subcode = card;
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
        /// - ---- First, and only first, channel should have upper bit marker
        /// set. If not, have a channel hopping error.
        if ( (unsigned int)*packedData < 65535 )
        // if(!((unsigned int)*packedData & ADC_1ST_CHAN_MARKER))
        {
            adcinfo->chanHop = 1;
            fe_status_return_subcode = card;
        }

        limit = OVERFLOW_LIMIT_16BIT;
        // Various ADC models have different number of channels/data bits
        offset = GSAI_DATA_CODE_OFFSET;
        mask = GSAI_DATA_MASK;
        num_outs = cdsPciModules.adcChannels[ card ];
        loops = UNDERSAMPLE;
        /// - ---- Determine next ipc memory location to load ADC data
        ioMemCntr = ( ( cycleNum / ADC_MEMCPY_RATE ) % IO_MEMORY_SLOTS );
        iocycle = ( cycleNum / ADC_MEMCPY_RATE );

        /// - ----  Read adc data from PCI mapped memory into local variables
        for ( kk = 0; kk < loops; kk++ )
        {
            for ( chan = 0; chan < num_outs; chan++ )
            {
                // adcData is the integer representation of the ADC data
                adcinfo->adcData[ card ][ chan ] = ( *packedData & mask );
                adcinfo->adcData[ card ][ chan ] -= offset;
#ifdef DEC_TEST
                if ( chan == 0 )
                {
                    adcinfo->adcData[ card ][ chan ] =
                        dspPtr[ 0 ]->data[ 0 ].exciteInput;
                }
#endif
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

            /// - ---- Write GPS time and cycle count as indicator to slave that
            /// adc data is ready
            ioMemData->gpsSecond = timeSec;
            ioMemData->iodata[ card ][ ioMemCntr ].timeSec = timeSec;
            ioMemData->iodata[ card ][ ioMemCntr ].cycle = iocycle;
            ioMemCntr++;
            iocycle++;
            iocycle %= 65536;
        }

        /// - ---- Clear out last ADC data read for test on next cycle
        packedData = (int*)cdsPciModules.pci_adc[ card ];
        *packedData = 0x0;

        if ( cdsPciModules.adcType[ card ] == GSC_18AI32SSC1M )
            packedData += GSAF_8_OFFSET;
        else
            packedData += GSAI_64_OFFSET;
        // Set dummy value to last channel
        *packedData = DUMMY_ADC_VAL;
        // Enable the ADC Demand DMA for next read
        if ( cdsPciModules.adcType[ card ] == GSC_18AI32SSC1M )
            gsc18ai32DmaEnable( card );
        else
            gsc16ai64DmaEnable( card );

#ifdef DIAG_TEST
        // For DIAGS ONLY !!!!!!!!
        // This will change ADC DMA BYTE count
        // -- Greater than normal will result in channel hopping.
        // -- Less than normal will result in ADC timeout.
        // In both cases, real-time kernel code should exit with errors to dmesg
        if ( pLocalEpics->epicsInput.bumpAdcRd != 0 )
        {
            gsc16ai64DmaBump( card, pLocalEpics->epicsInput.bumpAdcRd );
            pLocalEpics->epicsInput.bumpAdcRd = 0;
        }
#endif
    }
    return adcStat;
}
