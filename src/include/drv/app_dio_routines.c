inline int app_dio_init( void );
inline int app_dio_read_write( void );

inline int
app_dio_init( )
{

    int ii, kk;

    /// \> Read Dio card initial values
    /// - ---- Control units read/write their own DIO \n
    /// - ---- IOP units ignore DIO for speed reasons \n
    for ( kk = 0; kk < cdsPciModules.doCount; kk++ )
    {
        ii = cdsPciModules.doInstance[ kk ];
        if ( cdsPciModules.doType[ kk ] == ACS_8DIO )
        {
            rioInputInput[ ii ] =
                accesIiro8ReadInputRegister( &cdsPciModules, kk ) & 0xff;
            rioInputOutput[ ii ] =
                accesIiro8ReadOutputRegister( &cdsPciModules, kk ) & 0xff;
            rioOutputHold[ ii ] = -1;
        }
        else if ( cdsPciModules.doType[ kk ] == ACS_16DIO )
        {
            rioInput1[ ii ] =
                accesIiro16ReadInputRegister( &cdsPciModules, kk ) & 0xffff;
            rioOutputHold1[ ii ] = -1;
        }
        else if ( cdsPciModules.doType[ kk ] == CON_32DO )
        {
            CDO32Input[ ii ] = contec32ReadOutputRegister( &cdsPciModules, kk );
        }
        else if ( cdsPciModules.doType[ kk ] == CON_6464DIO )
        {
            CDIO6464LastOutState[ ii ] =
                contec6464ReadOutputRegister( &cdsPciModules, kk );
            // printf ("Initial state of contec6464 number %d = 0x%x
            // \n",ii,CDIO6464LastOutState[ii]);
        }
        else if ( cdsPciModules.doType[ kk ] == CDI64 )
        {
            CDIO6464LastOutState[ ii ] =
                contec6464ReadOutputRegister( &cdsPciModules, kk );
            // printf ("Initial state of contec6464 number %d = 0x%x
            // \n",ii,CDIO6464LastOutState[ii]);
        }
        else if ( cdsPciModules.doType[ kk ] == ACS_24DIO )
        {
            dioInput[ ii ] = accesDio24ReadInputRegister( &cdsPciModules, kk );
        }
    }
    return 0;
}
inline int
app_dio_read_write( )
{

    int ii, kk;

    // Read DIO cards, one card per cycle
    if ( cdsPciModules.doCount )
    {
        kk = cycleNum % cdsPciModules.doCount;
        ii = cdsPciModules.doInstance[ kk ];
        if ( cdsPciModules.doType[ kk ] == ACS_8DIO )
        {
            rioInputInput[ ii ] =
                accesIiro8ReadInputRegister( &cdsPciModules, kk ) & 0xff;
            rioInputOutput[ ii ] =
                accesIiro8ReadOutputRegister( &cdsPciModules, kk ) & 0xff;
        }
        if ( cdsPciModules.doType[ kk ] == ACS_16DIO )
        {
            rioInput1[ ii ] =
                accesIiro16ReadInputRegister( &cdsPciModules, kk ) & 0xffff;
        }
        if ( cdsPciModules.doType[ kk ] == ACS_24DIO )
        {
            dioInput[ ii ] = accesDio24ReadInputRegister( &cdsPciModules, kk );
        }
        if ( cdsPciModules.doType[ kk ] == CON_6464DIO )
        {
            CDIO6464InputInput[ ii ] =
                contec6464ReadInputRegister( &cdsPciModules, kk );
        }
        if ( cdsPciModules.doType[ kk ] == CDI64 )
        {
            CDIO6464InputInput[ ii ] =
                contec6464ReadInputRegister( &cdsPciModules, kk );
        }
    }
    /// \> Write Dio cards only on change
    for ( kk = 0; kk < cdsPciModules.doCount; kk++ )
    {
        ii = cdsPciModules.doInstance[ kk ];
        if ( ( cdsPciModules.doType[ kk ] == ACS_8DIO ) &&
             ( rioOutput[ ii ] != rioOutputHold[ ii ] ) )
        {
            accesIiro8WriteOutputRegister(
                &cdsPciModules, kk, rioOutput[ ii ] );
            rioOutputHold[ ii ] = rioOutput[ ii ];
        }
        else if ( ( cdsPciModules.doType[ kk ] == ACS_16DIO ) &&
                  ( rioOutput1[ ii ] != rioOutputHold1[ ii ] ) )
        {
            accesIiro16WriteOutputRegister(
                &cdsPciModules, kk, rioOutput1[ ii ] );
            rioOutputHold1[ ii ] = rioOutput1[ ii ];
        }
        else if ( cdsPciModules.doType[ kk ] == CON_32DO )
        {
            if ( CDO32Input[ ii ] != CDO32Output[ ii ] )
            {
                CDO32Input[ ii ] = contec32WriteOutputRegister(
                    &cdsPciModules, kk, CDO32Output[ ii ] );
            }
        }
        else if ( cdsPciModules.doType[ kk ] == CON_6464DIO )
        {
            if ( CDIO6464LastOutState[ ii ] != CDIO6464Output[ ii ] )
            {
                CDIO6464LastOutState[ ii ] = contec6464WriteOutputRegister(
                    &cdsPciModules, kk, CDIO6464Output[ ii ] );
            }
        }
        else if ( cdsPciModules.doType[ kk ] == CDO64 )
        {
            if ( CDIO6464LastOutState[ ii ] != CDIO6464Output[ ii ] )
            {
                CDIO6464LastOutState[ ii ] = contec6464WriteOutputRegister(
                    &cdsPciModules, kk, CDIO6464Output[ ii ] );
            }
        }
        else if ( ( cdsPciModules.doType[ kk ] == ACS_24DIO ) &&
                  ( dioOutputHold[ ii ] != dioOutput[ ii ] ) )
        {
            accesDio24WriteOutputRegister(
                &cdsPciModules, kk, dioOutput[ ii ] );
            dioOutputHold[ ii ] = dioOutput[ ii ];
        }
    }
    return 0;
}
