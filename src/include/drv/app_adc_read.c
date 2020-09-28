inline int app_adc_read( int, int, adcInfo_t*, int[] );
inline int app_adc_status_update( adcInfo_t* );

inline int
app_adc_read( int ioMemCtr, int ioClk, adcInfo_t* adcinfo, int cpuClk[] )
{
    int limit = OVERFLOW_LIMIT_16BIT;
    int mm;
    int num_chans = 32;
    int card = 0;
    int chan = 0;

    /// \> Control model gets its adc data from MASTER via ipc shared memory\n
    /// \> For each ADC defined:
    for ( card = 0; card < cdsPciModules.adcCount; card++ )
    {
        mm = cdsPciModules.adcConfig[ card ];
        if( mm >= 0 ) 
        {
        cpuClk[ CPU_TIME_RDY_ADC ] = rdtsc_ordered( );
        /// - ---- Wait for proper timestamp in shared memory, indicating data
        /// ready.
        do
        {
            cpuClk[ CPU_TIME_ADC_WAIT ] = rdtsc_ordered( );
            adcinfo->adcWait =
                ( cpuClk[ CPU_TIME_ADC_WAIT ] - cpuClk[ CPU_TIME_RDY_ADC ] ) /
                CPURATE;
        } while ( ( ioMemData->iodata[ mm ][ ioMemCtr ].cycle != ioClk ) &&
                  ( adcinfo->adcWait < MAX_ADC_WAIT_CONTROL ) );
        timeSec = ioMemData->iodata[ mm ][ ioMemCtr ].timeSec;
        if ( cycle_gps_time == 0 )
        {
            startGpsTime = timeSec;
            pLocalEpics->epicsOutput.startgpstime = startGpsTime;
        }
        cycle_gps_time = timeSec;

        /// - --------- If data not ready in time, set error, release DAC
        /// channel reservation and exit the code.
        if ( adcinfo->adcWait >= MAX_ADC_WAIT_CONTROL )
            return 1;
        if(cdsPciModules.adcType[ card ] == GSC_18AI32SSC1M) num_chans = 8;
        else num_chans = MAX_ADC_CHN_PER_MOD;
        for ( chan = 0; chan < num_chans; chan++ )
        {
            /// - ---- Read data from shared memory.
            adcinfo->adcData[ card ][ chan ] =
                ioMemData->iodata[ mm ][ ioMemCtr ].data[ chan ];
#ifdef FLIP_SIGNALS
            adcinfo->adcData[ card ][ chan ] *= -1;
#endif
            dWord[ card ][ chan ] = adcinfo->adcData[ card ][ chan ];
#ifdef OVERSAMPLE
            /// - ---- Downsample ADC data from 64K to rate of user application
            if ( dWordUsed[ card ][ chan ] )
            {
                dWord[ card ][ chan ] =
                    iir_filter_biquad( dWord[ card ][ chan ],
                                       FE_OVERSAMPLE_COEFF,
                                       2,
                                       &dHistory[ chan + card * num_chans ][ 0 ] );
            }
            /// - ---- Check for ADC data over/under range
            if ( ( adcinfo->adcData[ card ][ chan ] > limit ) ||
                 ( adcinfo->adcData[ card ][ chan ] < -limit ) )
            {
                adcinfo->overflowAdc[ card ][ chan ]++;
                pLocalEpics->epicsOutput.overflowAdcAcc[ card ][ chan ]++;
                overflowAcc++;
                adcinfo->adcOF[ card ] = 1;
                odcStateWord |= ODC_ADC_OVF;
            }
#endif
        }
      }
    }
    return 0;
}

inline int
app_adc_status_update( adcInfo_t* adcinfo )
{
    int status = 0;
    int card = 0;
    int chan = 0;
    int num_chans = 0;

    for ( card = 0; card < cdsPciModules.adcCount; card++ )
    {
        // SET/CLR Channel Hopping Error
        if ( adcinfo->adcChanErr[ card ] )
        {
            pLocalEpics->epicsOutput.statAdc[ card ] &= ~( ADC_CHAN_HOP );
            status |= FE_ERROR_ADC;
        }
        else
            pLocalEpics->epicsOutput.statAdc[ card ] |= ADC_CHAN_HOP;
        adcinfo->adcChanErr[ card ] = 0;
        // SET/CLR Overflow Error
        if ( adcinfo->adcOF[ card ] )
        {
            pLocalEpics->epicsOutput.statAdc[ card ] &= ~( ADC_OVERFLOW );
            status |= FE_ERROR_OVERFLOW;
        }
        else
            pLocalEpics->epicsOutput.statAdc[ card ] |= ADC_OVERFLOW;
        adcinfo->adcOF[ card ] = 0;
        if(cdsPciModules.adcType[ card ] == GSC_18AI32SSC1M) num_chans = 8;
        else num_chans = MAX_ADC_CHN_PER_MOD;
        for ( chan = 0; chan < num_chans; chan++ )
        {
            if ( pLocalEpics->epicsOutput.overflowAdcAcc[ card ][ chan ] >
                 OVERFLOW_CNTR_LIMIT )
            {
                pLocalEpics->epicsOutput.overflowAdcAcc[ card ][ chan ] = 0;
            }
            pLocalEpics->epicsOutput.overflowAdc[ card ][ chan ] =
                adcinfo->overflowAdc[ card ][ chan ];
            adcinfo->overflowAdc[ card ][ chan ] = 0;
        }
    }
    return status;
}
