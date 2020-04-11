/// \file mapApp.c
/// \brief This file contains the software to find PCIe card mapping from IOP in
/// mbuf..

int
mapPciModules( CDS_HARDWARE* pCds )
{
    int status = 0;
    int ii, jj, kk; /// @param ii,jj,kk default loop counters
    int cards; /// @param cards Number of PCIe cards found on bus
    int adcCnt = 0; /// @param adcCnt Number of ADC cards found by slave model.
    int dacCnt = 0; /// @param dacCnt Number of 16bit DAC cards found by slave
                    /// model.
    int dac18Cnt = 0; /// @param dac18Cnt Number of 18bit DAC cards found by
                      /// slave model.
    int dac20Cnt = 0; /// @param dac20Cnt Number of 20bit DAC cards found by
                      /// slave model.
    int doCnt = 0; /// @param doCnt Total number of digital I/O cards found by
                   /// slave model.
    int do32Cnt = 0; /// @param do32Cnt Total number of Contec 32 bit DIO cards
                     /// found by slave model.
    int doIIRO16Cnt = 0; /// @param doIIRO16Cnt Total number of Acces I/O 16 bit
                         /// relay cards found by slave model.
    int doIIRO8Cnt = 0; /// @param doIIRO8Cnt Total number of Acces I/O 8 bit
                        /// relay cards found by slave model.
    int cdo64Cnt = 0; /// @param cdo64Cnt Total number of Contec 6464 DIO card
                      /// 32bit output sections mapped by slave model.
    int cdi64Cnt = 0; /// @param cdo64Cnt Total number of Contec 6464 DIO card
                      /// 32bit input sections mapped by slave model.

    // Have to search thru all cards and find desired instance for application
    // Master will map ADC cards first, then DAC and finally DIO
    for ( ii = 0; ii < ioMemData->totalCards; ii++ )
    {
        for ( jj = 0; jj < pCds->cards; jj++ )
        {
            switch ( ioMemData->model[ ii ] )
            {
            case GSC_16AI64SSA:
                if ( ( pCds->cards_used[ jj ].type == GSC_16AI64SSA ) &&
                     ( pCds->cards_used[ jj ].instance == adcCnt ) )
                {
                    kk = pCds->adcCount;
                    pCds->adcType[ kk ] = GSC_16AI64SSA;
                    pCds->adcConfig[ kk ] = ioMemData->ipc[ ii ];
                    pCds->adcCount++;
                    status++;
                }
                break;
            case GSC_16AO16:
                if ( ( pCds->cards_used[ jj ].type == GSC_16AO16 ) &&
                     ( pCds->cards_used[ jj ].instance == dacCnt ) )
                {
                    kk = pCds->dacCount;
                    pCds->dacType[ kk ] = GSC_16AO16;
                    pCds->dacConfig[ kk ] = ioMemData->ipc[ ii ];
                    pCds->pci_dac[ kk ] = (long)( ioMemData->iodata[ ii ] );
                    pCds->dacCount++;
                    status++;
                }
                break;
            case GSC_18AO8:
                if ( ( pCds->cards_used[ jj ].type == GSC_18AO8 ) &&
                     ( pCds->cards_used[ jj ].instance == dac18Cnt ) )
                {
                    kk = pCds->dacCount;
                    pCds->dacType[ kk ] = GSC_18AO8;
                    pCds->dacConfig[ kk ] = ioMemData->ipc[ ii ];
                    pCds->pci_dac[ kk ] = (long)( ioMemData->iodata[ ii ] );
                    pCds->dacCount++;
                    status++;
                }
                break;
            case GSC_20AO8:
                if ( pCds->cards_used[ jj ].type == GSC_20AO8 &&
                     ( pCds->cards_used[ jj ].instance == dac20Cnt ) )
                {
                    kk = pCds->dacCount;
                    pCds->dacType[ kk ] = GSC_20AO8;
                    pCds->dacConfig[ kk ] = ioMemData->ipc[ ii ];
                    pCds->pci_dac[ kk ] = (long)( ioMemData->iodata[ ii ] );
                    pCds->dacCount++;
                    status++;
                }
                break;

            case CON_6464DIO:
                if ( ( pCds->cards_used[ jj ].type == CON_6464DIO ) &&
                     ( pCds->cards_used[ jj ].instance == doCnt ) )
                {
                    kk = pCds->doCount;
                    pCds->doType[ kk ] = ioMemData->model[ ii ];
                    pCds->pci_do[ kk ] = ioMemData->ipc[ ii ];
                    pCds->doCount++;
                    pCds->cDio6464lCount++;
                    pCds->pci_do[ kk ] = ioMemData->ipc[ ii ];
                    pCds->doInstance[ kk ] = doCnt;
                    status += 2;
                }
                if ( ( pCds->cards_used[ jj ].type == CDO64 ) &&
                     ( pCds->cards_used[ jj ].instance == doCnt ) )
                {
                    kk = pCds->doCount;
                    pCds->doType[ kk ] = CDO64;
                    pCds->pci_do[ kk ] = ioMemData->ipc[ ii ];
                    pCds->doCount++;
                    pCds->cDio6464lCount++;
                    pCds->doInstance[ kk ] = doCnt;
                    cdo64Cnt++;
                    status++;
                }
                if ( ( pCds->cards_used[ jj ].type == CDI64 ) &&
                     ( pCds->cards_used[ jj ].instance == doCnt ) )
                {
                    kk = pCds->doCount;
                    pCds->doType[ kk ] = CDI64;
                    pCds->pci_do[ kk ] = ioMemData->ipc[ ii ];
                    pCds->doInstance[ kk ] = doCnt;
                    pCds->doCount++;
                    pCds->cDio6464lCount++;
                    cdi64Cnt++;
                    status++;
                }
                break;
            case CON_32DO:
                if ( ( pCds->cards_used[ jj ].type == CON_32DO ) &&
                     ( pCds->cards_used[ jj ].instance == do32Cnt ) )
                {
                    kk = pCds->doCount;
                    pCds->doType[ kk ] = ioMemData->model[ ii ];
                    pCds->pci_do[ kk ] = ioMemData->ipc[ ii ];
                    pCds->doCount++;
                    pCds->cDo32lCount++;
                    pCds->doInstance[ kk ] = do32Cnt;
                    status++;
                }
                break;
            case ACS_16DIO:
                if ( ( pCds->cards_used[ jj ].type == ACS_16DIO ) &&
                     ( pCds->cards_used[ jj ].instance == doIIRO16Cnt ) )
                {
                    kk = pCds->doCount;
                    pCds->doType[ kk ] = ioMemData->model[ ii ];
                    pCds->pci_do[ kk ] = ioMemData->ipc[ ii ];
                    pCds->doCount++;
                    pCds->iiroDio1Count++;
                    pCds->doInstance[ kk ] = doIIRO16Cnt;
                    status++;
                }
                break;
            case ACS_8DIO:
                if ( ( pCds->cards_used[ jj ].type == ACS_8DIO ) &&
                     ( pCds->cards_used[ jj ].instance == doIIRO8Cnt ) )
                {
                    kk = pCds->doCount;
                    pCds->doType[ kk ] = ioMemData->model[ ii ];
                    pCds->pci_do[ kk ] = ioMemData->ipc[ ii ];
                    pCds->doCount++;
                    pCds->iiroDioCount++;
                    pCds->doInstance[ kk ] = doIIRO8Cnt;
                    status++;
                }
                break;
            default:
                break;
            }
        }
        if ( ioMemData->model[ ii ] == GSC_16AI64SSA )
            adcCnt++;
        if ( ioMemData->model[ ii ] == GSC_16AO16 )
            dacCnt++;
        if ( ioMemData->model[ ii ] == GSC_18AO8 )
            dac18Cnt++;
        if ( ioMemData->model[ ii ] == GSC_20AO8 )
            dac20Cnt++;
        if ( ioMemData->model[ ii ] == CON_6464DIO )
            doCnt++;
        if ( ioMemData->model[ ii ] == CON_32DO )
            do32Cnt++;
        if ( ioMemData->model[ ii ] == ACS_16DIO )
            doIIRO16Cnt++;
        if ( ioMemData->model[ ii ] == ACS_8DIO )
            doIIRO8Cnt++;
    }

    // Dolphin PCIe network style. Slave units will perform I/O transactions
    // with RFM directly ie MASTER does not do RFM I/O. Master unit only maps
    // the RFM I/O space and passes pointers to SLAVES.

    // Slave gets RFM module count from MASTER.
    cdsPciModules.rfmCount = ioMemData->rfmCount;
    // dolphinCount is number of segments
    cdsPciModules.dolphinCount = ioMemData->dolphinCount;
    // dolphin read/write 0 is for local PCIe network traffic
    cdsPciModules.dolphinRead[ 0 ] = ioMemData->dolphinRead[ 0 ];
    cdsPciModules.dolphinWrite[ 0 ] = ioMemData->dolphinWrite[ 0 ];
    // dolphin read/write 1 is for long range PCIe (RFM) traffic
    cdsPciModules.dolphinRead[ 1 ] = ioMemData->dolphinRead[ 1 ];
    cdsPciModules.dolphinWrite[ 1 ] = ioMemData->dolphinWrite[ 1 ];
    for ( ii = 0; ii < cdsPciModules.rfmCount; ii++ )
    {
        cdsPciModules.pci_rfm[ ii ] = ioMemData->pci_rfm[ ii ];
        cdsPciModules.pci_rfm_dma[ ii ] = ioMemData->pci_rfm_dma[ ii ];
    }
    // User APP does not access IRIG-B cards
    cdsPciModules.gps = 0;
    cdsPciModules.gpsType = 0;

    return status;
}
