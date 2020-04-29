inline int app_adc_read( int, int, adcInfo_t*, int[] );
inline int app_adc_status_update( adcInfo_t* );

inline int
app_adc_read( int ioMemCtr, int ioClk, adcInfo_t* adcinfo, int cpuClk[] )
{
    int ii, jj;
    int limit = OVERFLOW_LIMIT_16BIT;
    int mm;

    /// \> SLAVE gets its adc data from MASTER via ipc shared memory\n
    /// \> For each ADC defined:
    for ( jj = 0; jj < cdsPciModules.adcCount; jj++ )
    {
        mm = cdsPciModules.adcConfig[ jj ];
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
                  ( adcinfo->adcWait < MAX_ADC_WAIT_SLAVE ) );
        timeSec = ioMemData->iodata[ mm ][ ioMemCtr ].timeSec;
        if ( cycle_gps_time == 0 )
        {
            startGpsTime = timeSec;
            pLocalEpics->epicsOutput.startgpstime = startGpsTime;
        }
        cycle_gps_time = timeSec;

        /// - --------- If data not ready in time, set error, release DAC
        /// channel reservation and exit the code.
        if ( adcinfo->adcWait >= MAX_ADC_WAIT_SLAVE )
            return 1;
        for ( ii = 0; ii < MAX_ADC_CHN_PER_MOD; ii++ )
        {
            /// - ---- Read data from shared memory.
            adcinfo->adcData[ jj ][ ii ] =
                ioMemData->iodata[ mm ][ ioMemCtr ].data[ ii ];
#ifdef FLIP_SIGNALS
            adcinfo->adcData[ jj ][ ii ] *= -1;
#endif
            dWord[ jj ][ ii ] = adcinfo->adcData[ jj ][ ii ];
#ifdef OVERSAMPLE
            /// - ---- Downsample ADC data from 64K to rate of user application
            if ( dWordUsed[ jj ][ ii ] )
            {
                dWord[ jj ][ ii ] =
                    iir_filter_biquad( dWord[ jj ][ ii ],
                                       FE_OVERSAMPLE_COEFF,
                                       2,
                                       &dHistory[ ii + jj * 32 ][ 0 ] );
            }
            /// - ---- Check for ADC data over/under range
            if ( ( adcinfo->adcData[ jj ][ ii ] > limit ) ||
                 ( adcinfo->adcData[ jj ][ ii ] < -limit ) )
            {
                adcinfo->overflowAdc[ jj ][ ii ]++;
                pLocalEpics->epicsOutput.overflowAdcAcc[ jj ][ ii ]++;
                overflowAcc++;
                adcinfo->adcOF[ jj ] = 1;
                odcStateWord |= ODC_ADC_OVF;
            }
#endif
        }
    }
    return 0;
}

inline int
app_adc_status_update( adcInfo_t* adcinfo )
{
    int ii, jj;
    int status = 0;

    for ( jj = 0; jj < cdsPciModules.adcCount; jj++ )
    {
        // SET/CLR Channel Hopping Error
        if ( adcinfo->adcChanErr[ jj ] )
        {
            pLocalEpics->epicsOutput.statAdc[ jj ] &= ~( ADC_CHAN_HOP );
            status |= FE_ERROR_ADC;
            ;
        }
        else
            pLocalEpics->epicsOutput.statAdc[ jj ] |= ADC_CHAN_HOP;
        adcinfo->adcChanErr[ jj ] = 0;
        // SET/CLR Overflow Error
        if ( adcinfo->adcOF[ jj ] )
        {
            pLocalEpics->epicsOutput.statAdc[ jj ] &= ~( ADC_OVERFLOW );
            status |= FE_ERROR_OVERFLOW;
            ;
        }
        else
            pLocalEpics->epicsOutput.statAdc[ jj ] |= ADC_OVERFLOW;
        adcinfo->adcOF[ jj ] = 0;
        for ( ii = 0; ii < 32; ii++ )
        {
            if ( pLocalEpics->epicsOutput.overflowAdcAcc[ jj ][ ii ] >
                 OVERFLOW_CNTR_LIMIT )
            {
                pLocalEpics->epicsOutput.overflowAdcAcc[ jj ][ ii ] = 0;
            }
            pLocalEpics->epicsOutput.overflowAdc[ jj ][ ii ] =
                adcinfo->overflowAdc[ jj ][ ii ];
            adcinfo->overflowAdc[ jj ][ ii ] = 0;
        }
    }
    return status;
}
