inline int app_dac_init( void );
inline int app_dac_write( int, int, dacInfo_t* );

inline int
app_dac_init( )
{
    int ii, jj;
    int pd;
    /// \> Verify DAC channels defined for this app are not already in use. \n
    /// - ---- User apps are allowed to share DAC modules but not DAC channels.
    // See if my DAC channel map overlaps with already running models
    for ( ii = 0; ii < cdsPciModules.dacCount; ii++ )
    {
        pd = cdsPciModules.dacConfig[ ii ] -
            ioMemData->adcCount; // physical DAC number
        for ( jj = 0; jj < 16; jj++ )
        {
            if ( dacOutUsed[ ii ][ jj ] )
            {
                if ( ioMemData->dacOutUsed[ pd ][ jj ] )
                {
                    // printf("Failed to allocate DAC channel.\n");
                    // printf("DAC local %d global %d channel %d is already
                    // allocated.\n", ii, pd, jj);
                    return 1;
                }
            }
        }
    }
    for ( ii = 0; ii < cdsPciModules.dacCount; ii++ )
    {
        pLocalEpics->epicsOutput.statDac[ jj ] = DAC_FOUND_BIT;
        pd = cdsPciModules.dacConfig[ ii ] -
            ioMemData->adcCount; // physical DAC number
        for ( jj = 0; jj < MAX_DAC_CHN_PER_MOD; jj++ )
        {
            if ( dacOutUsed[ ii ][ jj ] )
            {
                ioMemData->dacOutUsed[ pd ][ jj ] = 1;
            }
        }
    }
    return 0;
}

inline int
app_dac_write( int ioMemCtrDac, int ioClkDac, dacInfo_t* dacinfo )
{
    int    ii, jj, mm, kk;
    int    limit, mask, num_outs;
    double dac_in = 0.0;
    int    dac_out = 0;
    int    memCtr = 0;

    /// \> Loop thru all DAC modules
    for ( jj = 0; jj < cdsPciModules.dacCount; jj++ )
    {
        /// - -- locate the proper DAC memory block
        mm = cdsPciModules.dacConfig[ jj ];
        /// - -- Set overflow limits, data mask, and chan count based on DAC
        /// type
        limit = OVERFLOW_LIMIT_16BIT;
        mask = GSAO_16BIT_MASK;
        num_outs = GSAO_16BIT_CHAN_COUNT;
        if ( cdsPciModules.dacType[ jj ] == GSC_18AO8 )
        {
            limit = OVERFLOW_LIMIT_18BIT; // 18 bit limit
            mask = GSAO_18BIT_MASK;
            num_outs = GSAO_18BIT_CHAN_COUNT;
        }
        if ( cdsPciModules.dacType[ jj ] == GSC_20AO8 )
        {
            limit = OVERFLOW_LIMIT_20BIT; // 20 bit limit
            mask = GSAO_20BIT_MASK;
            num_outs = GSAO_20BIT_CHAN_COUNT;
        }
        /// - -- If user app < 64k rate (typical), need to upsample from code
        /// rate to IOP rate
        for ( kk = 0; kk < OVERSAMPLE_TIMES; kk++ )
        {
            /// - -- For each DAC channel
            for ( ii = 0; ii < num_outs; ii++ )
            {
#ifdef FLIP_SIGNALS
                dacOut[ jj ][ ii ] *= -1;
#endif
                /// - ---- Read in DAC value, if channel is used by the
                /// application,  and run thru upsample filter\n
                if ( dacOutUsed[ jj ][ ii ] )
                {
#ifdef OVERSAMPLE_DAC
#ifdef NO_ZERO_PAD
                    /// - --------- If set to NO_ZERO_PAD (not standard
                    /// setting), then DAC output value is repeatedly run thru
                    /// the filter OVERSAMPE_TIMES.\n
                    dac_in = dacOut[ jj ][ ii ];
                    dac_in = iir_filter_biquad(
                        dac_in,
                        FE_OVERSAMPLE_COEFF,
                        2,
                        &dDacHistory[ ii + jj * MAX_DAC_CHN_PER_MOD ][ 0 ] );
#else
                    /// - --------- Otherwise zero padding is used (standard),
                    /// ie DAC output value is applied to filter on first
                    /// interation, thereafter zeros are applied.
                    dac_in = kk == 0 ? (double)dacOut[ jj ][ ii ] : 0.0;
                    dac_in = iir_filter_biquad(
                        dac_in,
                        FE_OVERSAMPLE_COEFF,
                        2,
                        &dDacHistory[ ii + jj * MAX_DAC_CHN_PER_MOD ][ 0 ] );
                    dac_in *= OVERSAMPLE_TIMES;
#endif
#else
                    dac_in = dacOut[ jj ][ ii ];
#endif
                }
                /// - ---- If channel is not used, then set value to zero.
                else
                    dac_in = 0.0;
                /// - ---- Smooth out some of the double > short roundoff errors
                if ( dac_in > 0.0 )
                    dac_in += 0.5;
                else
                    dac_in -= 0.5;
                dac_out = dac_in;

                /// - ---- Check output values are within range of DAC \n
                /// - --------- If overflow, clip at DAC limits and report
                /// errors
                if ( dac_out > limit || dac_out < -limit )
                {
                    dacinfo->overflowDac[ jj ][ ii ]++;
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
                dacinfo->dacOutEpics[ jj ][ ii ] = dac_out;

                /// - ---- Load DAC testpoints
                floatDacOut[ 16 * jj + ii ] = dac_out;
                /// - ---- Determine shared memory location for new DAC output
                /// data
                memCtr = ( ioMemCtrDac + kk ) % IO_MEMORY_SLOTS;
                /// - ---- Write DAC output to shared memory. \n
                /// - --------- Only write to DAC channels being used to allow
                /// two or more control models to write to same DAC module.
                if ( dacOutUsed[ jj ][ ii ] )
                    ioMemData->iodata[ mm ][ memCtr ].data[ ii ] = dac_out;
            }
            /// - ---- Write cycle count to make DAC data complete
            if ( iopDacEnable )
                ioMemData->iodata[ mm ][ memCtr ].cycle =
                    ( ioClkDac + kk ) % IOP_IO_RATE;
        }
    }
    return 0;
}
