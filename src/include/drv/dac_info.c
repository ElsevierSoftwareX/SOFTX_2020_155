inline int dac_status_update( dacInfo_t* );

inline int
dac_status_update( dacInfo_t* dacinfo )
{
    int ii, jj;
    int status = 0;

    for ( jj = 0; jj < cdsPciModules.dacCount; jj++ )
    {
        if ( dacOF[ jj ] )
        {
            pLocalEpics->epicsOutput.statDac[ jj ] &= ~( DAC_OVERFLOW_BIT );
            status |= FE_ERROR_OVERFLOW;
            ;
        }
        else
            pLocalEpics->epicsOutput.statDac[ jj ] |= DAC_OVERFLOW_BIT;
        dacOF[ jj ] = 0;
        if ( dacChanErr[ jj ] )
        {
            pLocalEpics->epicsOutput.statDac[ jj ] &= ~( DAC_TIMING_BIT );
        }
        else
            pLocalEpics->epicsOutput.statDac[ jj ] |= DAC_TIMING_BIT;
        dacChanErr[ jj ] = 0;
        for ( ii = 0; ii < MAX_DAC_CHN_PER_MOD; ii++ )
        {

            if ( pLocalEpics->epicsOutput.overflowDacAcc[ jj ][ ii ] >
                 OVERFLOW_CNTR_LIMIT )
            {
                pLocalEpics->epicsOutput.overflowDacAcc[ jj ][ ii ] = 0;
            }
            pLocalEpics->epicsOutput.overflowDac[ jj ][ ii ] =
                dacinfo->overflowDac[ jj ][ ii ];
            dacinfo->overflowDac[ jj ][ ii ] = 0;
        }
    }
    return status;
}
