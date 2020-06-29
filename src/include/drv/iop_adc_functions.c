inline int iop_adc_init( adcInfo_t* );
inline int iop_adc_read( adcInfo_t*, int[] );
inline int sync_adc_2_1pps( int );

// Routine for initializing ADC modules on startup
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
        if ( ( cdsPciModules.adcConfig[ jj ] & GSAI_AUTO_CAL_PASS ) != 0 )
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

// Routine to startup and synchronize ADC modules when 1PPS
// signal is used for synch.
inline int
sync_adc_2_1pps( int sync21pps)
{
    int           ii, jj, kk;
    int           status;
    volatile int* adcDummyData;
    int           search_cycles;
    // Arm ADC modules
    // This has to be done sequentially, one at a time.
    kk = 0;
    for ( jj = 0; jj < cdsPciModules.adcCount; jj++ )
    {
        adcDummyData = (int*)cdsPciModules.pci_adc[ jj ];
        adcDummyData += ADC_DUOTONE_CHAN;
        // Enable next ADC card
        gsc16ai64Enable1PPS( jj );
        // Wait for ADC read to complete
        status = gsc16ai64WaitDmaDone( jj, adcDummyData );
        // if(status == 44444) return -1;
        kk++;
        udelay( 2 );
        // Rearm ADC boards that had been previously enables
        for ( ii = 0; ii < kk; ii++ )
        {
            gsc16ai64DmaEnable( ii );
        }
    }

    if(sync21pps)
    {
        search_cycles = ADC_MEMCPY_RATE + 10000;
    } else {
        search_cycles = 2;
    }
    // Look for 1PPS signal for 1+ seconds
    for ( jj = 0; jj < search_cycles; jj++ )
    {
        // Wait until all ADC reads are complete
        for ( ii = 0; ii < cdsPciModules.adcCount; ii++ )
        {
            adcDummyData = (int*)cdsPciModules.pci_adc[ ii ];
            adcDummyData += 31;
            // Want to verify ADC FIFOs are empty to ensure they are in sync.
            status = gsc16ai64WaitDmaDone( ii, adcDummyData );
            status = gsc16ai64CheckAdcBuffer( ii );
        }
        adcDummyData = (int*)cdsPciModules.pci_adc[ ADC_DUOTONE_BRD ];
        adcDummyData += ADC_DUOTONE_CHAN;
        // Check value of 1PPS channel
        if ( *adcDummyData > ONE_PPS_THRESH && search_cycles > 10)
        {
            // Found 1PPS sync signal
            return 1;
        }

        // Rearm ADC boards
        for ( ii = 0; ii < cdsPciModules.adcCount; ii++ )
            gsc16ai64DmaEnable( ii );
    }
    // Did not find 1PPS signal
    return 0;
}

// ADC Read *****************************************************************
// Routine for reading data from ADC modules.
inline int
iop_adc_read( adcInfo_t* adcinfo, int cpuClk[] )
{
    int           ii, kk;
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
#ifdef DIAG_TEST
        // For DIAGS ONLY !!!!!!!!
        // This will change ADC DMA BYTE count
        // -- Greater than normal will result in channel hopping.
        // -- Less than normal will result in ADC timeout.
        // In both cases, real-time kernel code should exit with errors to dmesg
        int mydelay = 0;
        if ( pLocalEpics->epicsInput.longAdcRd != 0 )
        {
            mydelay = pLocalEpics->epicsInput.longAdcRd;
            udelay( mydelay );
            pLocalEpics->epicsInput.longAdcRd = 0;
        }
#endif
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
            if ( adcinfo->adcRdTime[ card ] > 500 )
                // Return a guestimate of how many cycles were delayed
                adcStat = adcinfo->adcRdTime[ card ] / 14;
            if ( adcinfo->adcRdTime[ card ] < 10 )
                // Indicate short cycle during recovery from long cycle
                adcStat = -1;
#ifdef XMIT_DOLPHIN_TIME
            // Send time on Dolphin net if this is the time xmitter.
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
            pLocalEpics->epicsOutput.stateWord = FE_ERROR_ADC;
            pLocalEpics->epicsOutput.diagWord |= ADC_TIMEOUT_ERR;
            pLocalEpics->epicsOutput.fe_status = ADC_TO_ERROR;
            fe_status_return = ADC_TO_ERROR;
            fe_status_return_subcode = card;
            stop_working_threads = 1;
            vmeDone = 1;
            continue;
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
        if ( cdsPciModules.adcType[ card ] == GSC_16AI64SSA && UNDERSAMPLE > 4 )
            loops = 1;
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
    // This only used on test system for checking decimation filters
    // by providing an ADC signal via a GDS EXC signal
                if ( chan == 0 )
                {
                    adcinfo->adcData[ card ][ chan ] =
                        dspPtr[ 0 ]->data[ 0 ].exciteInput;
                }
#endif
                // dWord is the double representation of the ADC data
                // This is the value used by the rest of the code calculations.
                dWord[ card ][ chan ][ kk ] = adcinfo->adcData[ card ][ chan ];
                // If running with fast ADC at 512K and gsc16ai64 at 64K, then:
                // fill in the missing data by doing data copy
                if ( cdsPciModules.adcType[ card ] == GSC_16AI64SSA &&
                     UNDERSAMPLE > 4 )
                {
                    for ( ii = 1; ii < UNDERSAMPLE; ii++ )
                        dWord[ card ][ chan ][ ii ] =
                            adcinfo->adcData[ card ][ chan ];
                }
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

            // For normal IOP ADC undersampling ie not a mixed fast 512K adc and
            // normal 64K adc
            if ( UNDERSAMPLE < 5 )
            {
                /// - ---- Write GPS time and cycle count as indicator to
                /// control app that adc data is ready
                ioMemData->gpsSecond = timeSec;
                ioMemData->iodata[ card ][ ioMemCntr ].timeSec = timeSec;
                ioMemData->iodata[ card ][ ioMemCntr ].cycle = iocycle;
                ioMemCntr++;
                iocycle++;
                iocycle %= IOP_IO_RATE;
            }
        }

        // For ADC undersampling when running with a fast ADC at 512K
        // to limit rate to user apps at 64K
        if ( UNDERSAMPLE > 4 )
        {
            /// - ---- Write GPS time and cycle count as indicator to control
            /// app that adc data is ready
            ioMemData->gpsSecond = timeSec;
            ioMemData->iodata[ card ][ ioMemCntr ].timeSec = timeSec;
            ioMemData->iodata[ card ][ ioMemCntr ].cycle = iocycle;
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
