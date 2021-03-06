inline int adc_status_update( adcInfo_t* );

inline int
adc_status_update( adcInfo_t* adcinfo )
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
        }
        else
            pLocalEpics->epicsOutput.statAdc[ jj ] |= ADC_CHAN_HOP;
        adcinfo->adcChanErr[ jj ] = 0;

        // Check ADC Timing
        if ( adcinfo->adcRdTimeErr[ jj ] )
            pLocalEpics->epicsOutput.statAdc[ jj ] &= ~( ADC_RD_TIME );
        else
            pLocalEpics->epicsOutput.statAdc[ jj ] |= ADC_RD_TIME;
        if ( adcinfo->adcRdTimeErr[ jj ] > MAX_ADC_WAIT_ERR_SEC )
            pLocalEpics->epicsOutput.stateWord |= FE_ERROR_ADC;
        adcinfo->adcRdTimeErr[ jj ] = 0;

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
