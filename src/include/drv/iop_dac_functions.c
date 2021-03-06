inline int iop_dac_init( int[] );
inline int iop_dac_preload( volatile GSC_DAC_REG*[] );
inline int iop_dac_write( int );

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
            // to be used to check on control apps' channel allocation
            ioMemData->dacOutUsed[ ii ][ jj ] = 0;
        }
    }

    for ( jj = 0; jj < cdsPciModules.dacCount; jj++ )
    {
        pLocalEpics->epicsOutput.statDac[ jj ] = DAC_FOUND_BIT;
        status = cdsPciModules.dacAcr[ jj ] & DAC_CAL_PASS;
        if(status)  pLocalEpics->epicsOutput.statDac[ jj ] |= DAC_AUTOCAL_BIT;
        // Arm DAC DMA for full data size
        if ( cdsPciModules.dacType[ jj ] == GSC_16AO16 )
        {
            status = gsc16ao16DmaSetup( jj );
        }
        else if ( cdsPciModules.dacType[ jj ] == GSC_20AO8 )
        {
            status = gsc20ao8DmaSetup( jj );
        }
        else
        {
            status = gsc18ao8DmaSetup( jj );
        }
    }

    return 0;
}

inline int
iop_dac_preload( volatile GSC_DAC_REG* dacReg[] )
{
    volatile GSA_18BIT_DAC_REG* dac18Ptr;
    volatile GSA_20BIT_DAC_REG* dac20Ptr;
    volatile GSC_DAC_REG*       dac16Ptr;
    int                         ii, jj;

    for ( jj = 0; jj < cdsPciModules.dacCount; jj++ )
    {
        if ( cdsPciModules.dacType[ jj ] == GSC_18AO8 )
        {
            dac18Ptr = (volatile GSA_18BIT_DAC_REG*)( dacReg[ jj ] );
            for ( ii = 0; ii < GSAO_18BIT_PRELOAD; ii++ )
                dac18Ptr->OUTPUT_BUF = 0;
        }
        else if ( cdsPciModules.dacType[ jj ] == GSC_20AO8 )
        {
            dac20Ptr = (volatile GSA_20BIT_DAC_REG*)( dacReg[ jj ] );
            for ( ii = 0; ii < GSAO_20BIT_PRELOAD; ii++ )
                dac20Ptr->OUTPUT_BUF = 0;
        }
        else
        {
            dac16Ptr = dacReg[ jj ];
            for ( ii = 0; ii < GSAO_16BIT_PRELOAD; ii++ )
                dac16Ptr->ODB = 0;
        }
    }
    return 0;
}

inline int
iop_dac_recover( int samples, volatile GSC_DAC_REG* dacReg[] )
{
    volatile GSA_18BIT_DAC_REG* dac18Ptr;
    volatile GSA_20BIT_DAC_REG* dac20Ptr;
    volatile GSC_DAC_REG*       dac16Ptr;
    int                         kk;
    int                         card;
    int                         chan;
    for ( card = 0; card < cdsPciModules.dacCount; card++ )
    {
        if ( cdsPciModules.dacType[ card ] == GSC_18AO8 )
        {
            dac18Ptr = (volatile GSA_18BIT_DAC_REG*)( dacReg[ card ] );
            for ( chan = 0; chan < ( GSAO_18BIT_CHAN_COUNT * samples ); chan++ )
                dac18Ptr->OUTPUT_BUF = dacOutEpics[ card ][ chan ];
        }
        else if ( cdsPciModules.dacType[ card ] == GSC_20AO8 )
        {
            dac20Ptr = (volatile GSA_20BIT_DAC_REG*)( dacReg[ card ] );
            for ( chan = 0; chan < ( GSAO_20BIT_CHAN_COUNT * samples ); chan++ )
                dac20Ptr->OUTPUT_BUF = dacOutEpics[ card ][ chan ];
        }
        else
        {
            dac16Ptr = dacReg[ card ];
            for ( chan = 0; chan < ( GSAO_16BIT_CHAN_COUNT * samples * 2 );
                  chan++ )
                dac16Ptr->ODB = dacOutEpics[ card ][ chan ];
        }
    }

    return 0;
}

inline int
iop_dac_write( int in_delay )
{
    unsigned int* pDacData;
    int           mm;
    int           limit;
    int           mask;
    int           num_outs;
    int           status = 0;
    int           card = 0;
    int           chan = 0;

    /// START OF IOP DAC WRITE ***************************************** \n
    /// \> If DAC FIFO error, always output zero to DAC modules. \n
    /// - -- Code will require restart to clear.
    // COMMENT OUT NEX LINE FOR TEST STAND w/bad DAC cards.
    /// \> Loop thru all DAC modules
    if ( ( dacWriteEnable > DAC_START_CYCLE ) && ( in_delay < 2 ) )
    {
        for ( card = 0; card < cdsPciModules.dacCount; card++ )
        {
            /// - -- Point to DAC memory buffer
            pDacData = (unsigned int*)( cdsPciModules.pci_dac[ card ] );
            /// - -- locate the proper DAC memory block
            mm = cdsPciModules.dacConfig[ card ];
            /// - -- Determine if memory block has been set with the correct
            /// cycle count by control app.
            if ( ioMemData->iodata[ mm ][ ioMemCntrDac ].cycle == ioClockDac ||
                 ( in_delay == 1 ) )
            {
                dacEnable |= pBits[ card ];
            }
            else
            {
                dacEnable &= ~( pBits[ card ] );
                dacChanErr[ card ] += 1;
            }
            /// - -- Set overflow limits, data mask, and chan count based on DAC
            /// type
            limit = OVERFLOW_LIMIT_16BIT;
            mask = GSAO_16BIT_MASK;
            num_outs = GSAO_16BIT_CHAN_COUNT;
            if ( cdsPciModules.dacType[ card ] == GSC_18AO8 )
            {
                limit = OVERFLOW_LIMIT_18BIT; // 18 bit limit
                mask = GSAO_18BIT_MASK;
                num_outs = GSAO_18BIT_CHAN_COUNT;
            }
            if ( cdsPciModules.dacType[ card ] == GSC_20AO8 )
            {
                limit = OVERFLOW_LIMIT_20BIT; // 20 bit limit
                mask = GSAO_20BIT_MASK;
                num_outs = GSAO_20BIT_CHAN_COUNT;
            }
            /// - -- For each DAC channel
            for ( chan = 0; chan < num_outs; chan++ )
            {
                /// - ---- Read DAC output value from shared memory and reset
                /// memory to zero
                if ( ( !dacChanErr[ card ] ) && ( iopDacEnable ) )
                {
                    if ( !in_delay )
                    {
                        dac_out = ioMemData->iodata[ mm ][ ioMemCntrDac ]
                                      .data[ chan ];
                    }
                    else
                    {
                        dac_out = dac_out_last[ card ][ chan ];
                    }
                    /// - --------- Zero out data in case user app dies by next
                    /// cycle when two or more apps share same DAC module.
                    ioMemData->iodata[ mm ][ ioMemCntrDac ].data[ chan ] = 0;
                }
                else
                {
                    dac_out = 0;
                    status = 1;
                }
                /// - ----  Write out ADC duotone signal to first DAC, last
                /// channel, if DAC duotone is enabled.
                if ( ( dt_diag.dacDuoEnable ) && ( chan == ( num_outs - 1 ) ) &&
                     ( card == 0 ) )
                {
                    dac_out = adcinfo.adcData[ 0 ][ ADC_DUOTONE_CHAN ];
                }
// Code below is only for use in DAQ test system.
#ifdef DIAG_TEST
                if ( ( chan == 0 ) && ( card < 3 ) )
                {
                    if ( cycleNum < 100 )
                        dac_out = limit / 20;
                    else
                        dac_out = 0;
                }
#endif
                /// - ---- Check output values are within range of DAC \n
                /// - --------- If overflow, clip at DAC limits and report
                /// errors
                if ( dac_out > limit || dac_out < -limit )
                {
                    dacinfo.overflowDac[ card ][ chan ]++;
                    pLocalEpics->epicsOutput.overflowDacAcc[ card ][ chan ]++;
                    overflowAcc++;
                    dacinfo.dacOF[ card ] = 1;
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
                dacOutEpics[ card ][ chan ] = dac_out;
                dac_out_last[ card ][ chan ] = dac_out;

                /// - ---- Load DAC testpoints
                floatDacOut[ 16 * card + chan ] = dac_out;

                /// - ---- Write to DAC local memory area, for later xmit to DAC
                /// module
                *pDacData = (unsigned int)( dac_out & mask );
                pDacData++;
            }
            /// - -- Mark cycle count as having been used -1 \n
            /// - --------- Forces control apps to mark this cycle or will not
            /// be used again by Master
            ioMemData->iodata[ mm ][ ioMemCntrDac ].cycle = -1;
            /// - -- DMA Write data to DAC module
            if ( dacWriteEnable > DAC_START_CYCLE )
            {
                if ( cdsPciModules.dacType[ card ] == GSC_16AO16 )
                {
                    gsc16ao16DmaStart( card );
                }
                else if ( cdsPciModules.dacType[ card ] == GSC_20AO8 )
                {
                    gsc20ao8DmaStart( card );
                }
                else
                {
                    gsc18ao8DmaStart( card );
                }
            }
        }
    }
    /// \> Increment DAC memory block pointers for next cycle
    ioClockDac = ( ioClockDac + 1 ) % IOP_IO_RATE;
    ioMemCntrDac = ( ioMemCntrDac + 1 ) % IO_MEMORY_SLOTS;
    if ( dacWriteEnable < 10 )
        dacWriteEnable++;
    /// END OF IOP DAC WRITE *************************************************
    return status;
}

inline int
check_dac_buffers( int cycleNum, int report_all_faults )
{
    // if report_all_faults not set, then will only set
    // dacTimingError on FIFO full.
    volatile GSA_18BIT_DAC_REG* dac18bitPtr;
    volatile GSA_20BIT_DAC_REG* dac20bitPtr;
    int                         out_buf_size = 0;
    int                         jj = 0;
    int                         status = 0;

    jj = cycleNum - HKP_DAC_FIFO_CHK;
    if ( cdsPciModules.dacType[ jj ] == GSC_18AO8 )
    {
        if ( report_all_faults )
        {
            dac18bitPtr = (volatile GSA_18BIT_DAC_REG*)( dacPtr[ jj ] );
            out_buf_size = dac18bitPtr->OUT_BUF_SIZE;
            dacOutBufSize[ jj ] = out_buf_size;
            pLocalEpics->epicsOutput.buffDac[ jj ] = out_buf_size;
            if ( !dacTimingError )
            {
                if ( ( out_buf_size < 8 ) || ( out_buf_size > 24 ) )
                {
                    pLocalEpics->epicsOutput.statDac[ jj ] &= ~( DAC_FIFO_BIT );
                    if ( dacTimingErrorPending[ jj ] > DAC_WD_TRIP_SET)
                        dacTimingError = 1;
                    dacTimingErrorPending[ jj ] ++;
                }
                else
                {
                    pLocalEpics->epicsOutput.statDac[ jj ] |= DAC_FIFO_BIT;
                    dacTimingErrorPending[ jj ] = 0;
                }
            }

            // Set/unset FIFO empty,hi qtr, full diags
            if ( out_buf_size < 4 )
                pLocalEpics->epicsOutput.statDac[ jj ] |= DAC_FIFO_EMPTY;
            else
                pLocalEpics->epicsOutput.statDac[ jj ] &= ~( DAC_FIFO_EMPTY );
            if ( out_buf_size > 24 )
                pLocalEpics->epicsOutput.statDac[ jj ] |= DAC_FIFO_HI_QTR;
            else
                pLocalEpics->epicsOutput.statDac[ jj ] &= ~( DAC_FIFO_HI_QTR );
            if ( out_buf_size > 320 )
            {
                pLocalEpics->epicsOutput.statDac[ jj ] |= DAC_FIFO_FULL;
                dacTimingError = 1;
            }
            else
            {
                pLocalEpics->epicsOutput.statDac[ jj ] &= ~( DAC_FIFO_FULL );
            }
        }
        else
        {
            pLocalEpics->epicsOutput.statDac[ jj ] &= ~( DAC_FIFO_EMPTY );
            pLocalEpics->epicsOutput.statDac[ jj ] &= ~( DAC_FIFO_HI_QTR );
            // Check only for FIFO FULL
            if ( out_buf_size > 320 )
            {
                pLocalEpics->epicsOutput.statDac[ jj ] |= DAC_FIFO_FULL;
                pLocalEpics->epicsOutput.statDac[ jj ] &= ~( DAC_FIFO_BIT );
                dacTimingError = 1;
            }
            else
            {
                pLocalEpics->epicsOutput.statDac[ jj ] |= DAC_FIFO_BIT;
            }
        }
    }

    if ( cdsPciModules.dacType[ jj ] == GSC_20AO8 )
    {
        dac20bitPtr = (volatile GSA_20BIT_DAC_REG*)( dacPtr[ jj ] );
        out_buf_size = dac20bitPtr->OUT_BUF_SIZE;
        dacOutBufSize[ jj ] = out_buf_size;
        pLocalEpics->epicsOutput.buffDac[ jj ] = out_buf_size;
        if ( ( out_buf_size > 24 ) )
        {
            pLocalEpics->epicsOutput.statDac[ jj ] &= ~( DAC_FIFO_BIT );
            if ( (dacTimingErrorPending[ jj ] > DAC_WD_TRIP_SET) && report_all_faults )
                dacTimingError = 1;
            else dacTimingErrorPending[ jj ] ++;
        }
        else
        {
            pLocalEpics->epicsOutput.statDac[ jj ] |= DAC_FIFO_BIT;
            dacTimingErrorPending[ jj ] = 0;
        }

        // Set/unset FIFO empty,hi qtr, full diags
        if ( out_buf_size > 24 )
            pLocalEpics->epicsOutput.statDac[ jj ] |= DAC_FIFO_HI_QTR;
        else
            pLocalEpics->epicsOutput.statDac[ jj ] &= ~( DAC_FIFO_HI_QTR );
        if ( out_buf_size > 500 )
        {
            pLocalEpics->epicsOutput.statDac[ jj ] |= DAC_FIFO_FULL;
            dacTimingError = 1;
        }
        else
        {
            pLocalEpics->epicsOutput.statDac[ jj ] &= ~( DAC_FIFO_FULL );
        }
    }
    if ( cdsPciModules.dacType[ jj ] == GSC_16AO16 )
    {
        status = gsc16ao16CheckDacBuffer( jj );
        dacOutBufSize[ jj ] = status;
        if ( report_all_faults )
        {
            if ( !dacTimingError )
            {
                if ( status != 2 )
                {
                    pLocalEpics->epicsOutput.statDac[ jj ] &= ~( DAC_FIFO_BIT );
                    if ( dacTimingErrorPending[ jj ] > DAC_WD_TRIP_SET)
                        dacTimingError = 1;
                    else dacTimingErrorPending[ jj ] ++;
                }
                else
                {
                    pLocalEpics->epicsOutput.statDac[ jj ] |= DAC_FIFO_BIT;
                    dacTimingErrorPending[ jj ] = 0;
                }
            }

            // Set/unset FIFO empty,hi qtr, full diags
            if ( status & 1 )
                pLocalEpics->epicsOutput.statDac[ jj ] |= DAC_FIFO_EMPTY;
            else
                pLocalEpics->epicsOutput.statDac[ jj ] &= ~( DAC_FIFO_EMPTY );
            if ( status & 8 )
            {
                pLocalEpics->epicsOutput.statDac[ jj ] |= DAC_FIFO_FULL;
                dacTimingError = 1;
            }
            else
            {
                pLocalEpics->epicsOutput.statDac[ jj ] &= ~( DAC_FIFO_FULL );
            }
            if ( status & 4 )
                pLocalEpics->epicsOutput.statDac[ jj ] |= DAC_FIFO_HI_QTR;
            else
                pLocalEpics->epicsOutput.statDac[ jj ] &= ~( DAC_FIFO_HI_QTR );
        }
        else
        {
            pLocalEpics->epicsOutput.statDac[ jj ] &= ~( DAC_FIFO_EMPTY );
            pLocalEpics->epicsOutput.statDac[ jj ] &= ~( DAC_FIFO_HI_QTR );
            // Check only for FIFO FULL
            if ( status & 8 )
            {
                pLocalEpics->epicsOutput.statDac[ jj ] &= ~( DAC_FIFO_BIT );
                pLocalEpics->epicsOutput.statDac[ jj ] |= DAC_FIFO_FULL;
                pLocalEpics->epicsOutput.statDac[ jj ] &= ~( DAC_FIFO_BIT );
                dacTimingError = 1;
            }
            else
            {
                pLocalEpics->epicsOutput.statDac[ jj ] |= DAC_FIFO_BIT;
                pLocalEpics->epicsOutput.statDac[ jj ] |= DAC_FIFO_BIT;
                pLocalEpics->epicsOutput.statDac[ jj ] &= ~( DAC_FIFO_FULL );
            }
        }
    }
    return 0;
}
