/// @file rcguserCommon.c
/// @brief File contains routines for user app startup

// **********************************************************************************************
// Capture SIGHALT from ControlC
void
intHandler( int dummy )
{
    pLocalEpics->epicsInput.vmeReset = 1;
}

// **********************************************************************************************

void
attach_shared_memory( char* sysname )
{

    char         shm_name[ 64 ];
    extern char* addr;

    // epics shm used to communicate with model's EPICS process
    sprintf( shm_name, "%s", sysname );
    findSharedMemory( sysname );
    _epics_shm = (char*)addr;
    if ( _epics_shm < 0 )
    {
        printf( "mbuf_allocate_area() failed; ret = %d\n", _epics_shm );
        return -1;
    }
    printf( "EPICSM at 0x%lx\n", (long)_epics_shm );

    // ipc_shm used to communicate with IOP
    findSharedMemory( "ipc" );
    _ipc_shm = (char*)addr;
    if ( _ipc_shm < 0 )
    {
        printf( "mbuf_allocate_area(ipc) failed; ret = %d\n", _ipc_shm );
        return -1;
    }
    printf( "IPC    at 0x%lx\n", (long)_ipc_shm );
    ioMemData = (volatile IO_MEM_DATA*)( ( (char*)_ipc_shm ) + 0x4000 );
    printf(
        "IOMEM  at 0x%lx size 0x%x\n", (long)ioMemData, sizeof( IO_MEM_DATA ) );
    printf( "%d PCI cards found\n", ioMemData->totalCards );

    // DAQ is via shared memory 
    sprintf( shm_name, "%s_daq", sysname );
    findSharedMemory( shm_name );
    _daq_shm = (char*)addr;
    if ( _daq_shm < 0 )
    {
        printf( "mbuf_allocate_area() failed; ret = %d\n", _daq_shm );
        return -1;
    }
    printf( "DAQSM at 0x%lx\n", _daq_shm );
    daqPtr = (struct rmIpcStr*)_daq_shm;

    // shmipc is used to send SHMEM IPC data between processes on same computer
    findSharedMemory( "shmipc" );
    _shmipc_shm = (char*)addr;
    if ( _shmipc_shm < 0 )
    {
        printf( "mbuf_allocate_area() failed; ret = %d\n", _shmipc_shm );
        return -1;
    }

    // Open new IO shared memory in support of no hardware I/O
    findSharedMemory( "virtual_io_space" );
    _io_shm = (char*)addr;
    if ( _io_shm < 0 )
    {
        printf( "mbuf_allocate_area() failed; ret = %d\n", _io_shm );
        return -1;
    }
    ioMemDataIop = (volatile IO_MEM_DATA_IOP*)( ( (char*)_io_shm ) );
}

// Function to print out I/O configuration on startup
void
print_io_info_1( CDS_HARDWARE* cdsp )
{
    int ii, jj, kk;
    int channels = 0;
    jj = 0;

#ifndef USER_SPACE
    printf( "" SYSTEM_NAME_STRING_LOWER ":startup time is %ld\n",
            current_time_fe( ) );
    printf( "" SYSTEM_NAME_STRING_LOWER ":cpu clock %u\n", cpu_khz );
#endif
    printf( "" SYSTEM_NAME_STRING_LOWER ":EPICSM at 0x%lx\n",
            (unsigned long)_epics_shm );
    printf( "" SYSTEM_NAME_STRING_LOWER ":IPC    at 0x%lx\n",
            (unsigned long)_ipc_shm );
    printf( "" SYSTEM_NAME_STRING_LOWER ":IOMEM  at 0x%lx size 0x%lx\n",
            ( (unsigned long)_ipc_shm + 0x4000 ),
            sizeof( IO_MEM_DATA ) );
    printf( "" SYSTEM_NAME_STRING_LOWER ":DAQSM at 0x%lx\n",
            (unsigned long)_daq_shm );
    printf( "" SYSTEM_NAME_STRING_LOWER ":configured to use %d cards\n",
            cdsp->cards );
    kk = 0;
    printf( "******************************************************************"
            "*********\n" );
    printf( "" SYSTEM_NAME_STRING_LOWER ":%d ADC cards found\n",
            cdsp->adcCount );
    for ( ii = 0; ii < cdsp->adcCount; ii++ )
    {
        kk++;
        if ( cdsp->adcType[ ii ] == GSC_16AI64SSA )
        {
            printf( "\tADC %d is a GSC_16AI64SSA module\n", ii );
            jj = 32;
            printf( "\t\tChannels = %d \n", jj );
            printf( "\t\tFirmware Rev = %d \n\n",
                    ( cdsp->adcConfig[ ii ] & 0xfff ) );
        }
    }
    printf( "******************************************************************"
            "*********\n" );
    printf( "" SYSTEM_NAME_STRING_LOWER ":%d DAC cards found\n",
            cdsp->dacCount );
    for ( ii = 0; ii < cdsp->dacCount; ii++ )
    {
        kk++;
        if ( cdsp->dacType[ ii ] == GSC_18AO8 )
        {
            printf( "\tDAC %d is a GSC_18AO8 module\n", ii );
        }
        if ( cdsPciModules.dacType[ ii ] == GSC_20AO8 )
        {
            printf( "\tDAC %d is a GSC_20AO8 module\n", ii );
        }
        if ( cdsp->dacType[ ii ] == GSC_16AO16 )
        {
            printf( "\tDAC %d is a GSC_16AO16 module\n", ii );
            jj = 16;
            printf( "\t\tChannels = %d \n", jj );
            printf( "\t\tOutput Type = Differential\n" );
        }
    }
    kk += cdsp->dioCount;
    printf( "******************************************************************"
            "*********\n" );
    printf( "" SYSTEM_NAME_STRING_LOWER ":%d DIO cards found\n",
            cdsp->dioCount );
    printf( "******************************************************************"
            "*********\n" );
    printf( "" SYSTEM_NAME_STRING_LOWER ":%d IIRO-8 Isolated DIO cards found\n",
            cdsp->iiroDioCount );
    printf( "******************************************************************"
            "*********\n" );
    printf( "" SYSTEM_NAME_STRING_LOWER
            ":%d IIRO-16 Isolated DIO cards found\n",
            cdsp->iiroDio1Count );
    pLocalEpics->epicsOutput.bioMon[ 0 ] = cdsp->iiroDio1Count;
    printf( "******************************************************************"
            "*********\n" );
    printf( "" SYSTEM_NAME_STRING_LOWER ":%d Contec 32ch PCIe DO cards found\n",
            cdsp->cDo32lCount );
    pLocalEpics->epicsOutput.bioMon[ 1 ] = cdsp->cDo32lCount;
    printf( "" SYSTEM_NAME_STRING_LOWER ":%d Contec PCIe DIO1616 cards found\n",
            cdsp->cDio1616lCount );
    pLocalEpics->epicsOutput.bioMon[ 2 ] = cdsp->cDio1616lCount;
    printf( "" SYSTEM_NAME_STRING_LOWER ":%d Contec PCIe DIO6464 cards found\n",
            cdsp->cDio6464lCount );
    pLocalEpics->epicsOutput.bioMon[ 3 ] = cdsp->cDio6464lCount;
    printf( "" SYSTEM_NAME_STRING_LOWER ":%d DO cards found\n", cdsp->doCount );
    printf( "" SYSTEM_NAME_STRING_LOWER
            ":Total of %d I/O modules found and mapped\n",
            kk );
    printf( "******************************************************************"
            "*********\n" );
    printf( "" SYSTEM_NAME_STRING_LOWER ":%d RFM cards found\n",
            cdsp->rfmCount );
    printf( "******************************************************************"
            "*********\n" );
    for ( ii = 0; ii < cdsp->dolphinCount; ii++ )
    {
        printf( "\tDolphin found %d\n", ii );
        printf( "Read address is 0x%lx\n", cdsp->dolphinRead[ ii ] );
        printf( "Write address is 0x%lx\n", cdsp->dolphinWrite[ ii ] );
    }
}
// ENDOF PRINTIO


void initmap(CDS_HARDWARE* pCds)
{
int i;
int dac_cnt = 0;
int adc_cnt = 0;
    pCds->adcCount = 0;
    pCds->dacCount = 0;
    pCds->dioCount = 0;
    pCds->doCount = 0;

 for ( i = 0; i < pCds->cards; i++ )
    {
     pCds->adcMap[ i ] = 0;
     pCds->dacMap[ i ] = 0;
    }

 for ( i = 0; i < pCds->cards; i++ )
    {
        if ( pCds->cards_used[ i ].type == GSC_18AO8 )
        {
            pCds->dacType[ dac_cnt ] = GSC_18AO8;
            pCds->dacInstance[ dac_cnt ] =  pCds->cards_used[ i ].instance;
            pCds->dacConfig[ dac_cnt ] = 0;
            pCds->dacMap[ i ] = dac_cnt;
            pCds->dacCount++;
            pCds->dac18Count++;
            dac_cnt++;
        }
        if ( pCds->cards_used[ i ].type == GSC_16AO16 )
        {
            pCds->dacType[ dac_cnt ] = GSC_16AO16;
            pCds->dacInstance[ dac_cnt ] =  pCds->cards_used[ i ].instance;
            pCds->dacConfig[ dac_cnt ] = 0;
            pCds->dacMap[ i ] = dac_cnt;
            pCds->dacCount++;
            pCds->dac16Count++;
            dac_cnt++;
        }
        if ( pCds->cards_used[ i ].type == GSC_20AO8 )
        {
            pCds->dacType[ dac_cnt ] = GSC_20AO8;
            pCds->dacInstance[ dac_cnt ] =  pCds->cards_used[ i ].instance;
            pCds->dacConfig[ dac_cnt ] = 0;
            pCds->dacMap[ i ] = dac_cnt;
            pCds->dacCount++;
            pCds->dac20Count++;
            dac_cnt++;
        }
        if ( pCds->cards_used[ i ].type == GSC_16AI64SSA )
        {
            pCds->adcType[ adc_cnt ] = GSC_16AI64SSA;
            pCds->adcInstance[ adc_cnt ] =  pCds->cards_used[ i ].instance;
            pCds->adcConfig[ adc_cnt ] = -1;
            pCds->adcMap[ i ] = adc_cnt;
            pCds->adcCount++;
            pCds->adc16Count++;
            adc_cnt++;
        }
        if ( pCds->cards_used[ i ].type == GSC_18AI32SSC1M )
        {
            pCds->adcType[ adc_cnt ] = GSC_18AI32SSC1M;
            pCds->adcInstance[ adc_cnt ] =  pCds->cards_used[ i ].instance;
            pCds->adcConfig[ adc_cnt ] = -1;
            pCds->adcMap[ i ] = adc_cnt;
            pCds->adcCount++;
            pCds->adc18Count++;
            adc_cnt++;
        }
    }

}


int
mapPciModules( CDS_HARDWARE* pCds )
{
    int status = 0;
    int ii, jj, kk; /// @param ii,jj,kk default loop counters
    int cards; /// @param cards Number of PCIe cards found on bus
    int adcCnt =
        0; /// @param adcCnt Number of ADC cards found by control model.
    int fastadcCnt =
        0; /// @param adcCnt Number of ADC cards found by control model.
    int dacCnt = 0; /// @param dacCnt Number of 16bit DAC cards found by control
                    /// model.
    int dac18Cnt = 0; /// @param dac18Cnt Number of 18bit DAC cards found by
                      /// control model.
    int dac20Cnt = 0; /// @param dac20Cnt Number of 20bit DAC cards found by
                      /// control model.
    int doCnt = 0; /// @param doCnt Total number of digital I/O cards found by
                   /// control model.
    int do32Cnt = 0; /// @param do32Cnt Total number of Contec 32 bit DIO cards
                     /// found by control model.
    int doIIRO16Cnt = 0; /// @param doIIRO16Cnt Total number of Acces I/O 16 bit
                         /// relay cards found by control model.
    int doIIRO8Cnt = 0; /// @param doIIRO8Cnt Total number of Acces I/O 8 bit
                        /// relay cards found by control model.
    int cdo64Cnt = 0; /// @param cdo64Cnt Total number of Contec 6464 DIO card
                      /// 32bit output sections mapped by control model.
    int cdi64Cnt = 0; /// @param cdo64Cnt Total number of Contec 6464 DIO card
                      /// 32bit input sections mapped by control model.

    // Have to search thru all cards and find desired instance for application
    // IOP will map ADC cards first, then DAC and finally DIO
    for ( jj = 0; jj < pCds->cards; jj++ )
    {
        for ( ii = 0; ii < ioMemData->totalCards; ii++ )
        {
            switch ( ioMemData->model[ ii ] )
            {
            case GSC_16AI64SSA:
                if ( ( pCds->cards_used[ jj ].type == GSC_16AI64SSA ) &&
                     ( pCds->cards_used[ jj ].instance ==
                       ioMemData->card[ ii ] ) )
                {
                    kk = pCds->adcMap[ jj ];
                    // pCds->adcType[ kk ] = GSC_16AI64SSA;
                    // pCds->adcInstance[ kk ] = ioMemData->card[ ii ];
                    pCds->adcConfig[ kk ] = ioMemData->ipc[ ii ];
                    // pCds->adcCount++;
                    status++;
                }
                break;
            case GSC_18AI32SSC1M:
                if ( ( pCds->cards_used[ jj ].type == GSC_18AI32SSC1M ) &&
                     ( pCds->cards_used[ jj ].instance ==
                       ioMemData->card[ ii ] ) )
                {
                    // kk = pCds->adcCount;
                    kk = pCds->adcMap[ jj ];
                    // pCds->adcType[ kk ] = GSC_18AI32SSC1M;
                    // pCds->adcInstance[ kk ] = ioMemData->card[ ii ];
                    pCds->adcConfig[ kk ] = ioMemData->ipc[ ii ];
                    // pCds->adcCount++;
                    status++;
                }
                break;
            case GSC_16AO16:
                if ( ( pCds->cards_used[ jj ].type == GSC_16AO16 ) &&
                     ( pCds->cards_used[ jj ].instance ==
                       ioMemData->card[ ii ] ) )
                {
                    kk = pCds->dacMap[ jj ];
                    // kk = pCds->dacCount;
                    // pCds->dacType[ kk ] = GSC_16AO16;
                    // pCds->dacInstance[ kk ] = ioMemData->card[ ii ];
                    pCds->dacConfig[ kk ] = ioMemData->ipc[ ii ];
                    pCds->pci_dac[ kk ] = (long)( ioMemData->iodata[ ii ] );
                    // pCds->dacCount++;
                    status++;
                }
                break;
            case GSC_18AO8:
                if ( ( pCds->cards_used[ jj ].type == GSC_18AO8 ) &&
                     ( pCds->cards_used[ jj ].instance ==
                       ioMemData->card[ ii ] ) )
                {
                    kk = pCds->dacMap[ jj ];
                    // kk = pCds->dacCount;
                    // pCds->dacType[ kk ] = GSC_18AO8;
                    // pCds->dacInstance[ kk ] = ioMemData->card[ ii ];
                    pCds->dacConfig[ kk ] = ioMemData->ipc[ ii ];
                    pCds->pci_dac[ kk ] = (long)( ioMemData->iodata[ ii ] );
                    // pCds->dacCount++;
                    status++;
                }
                break;
            case GSC_20AO8:
                if ( pCds->cards_used[ jj ].type == GSC_20AO8 &&
                     ( pCds->cards_used[ jj ].instance ==
                       ioMemData->card[ ii ] ) )
                {
                    kk = pCds->dacMap[ jj ];
                    // kk = pCds->dacCount;
                    // pCds->dacType[ kk ] = GSC_20AO8;
                    // pCds->dacInstance[ kk ] = ioMemData->card[ ii ];
                    pCds->dacConfig[ kk ] = ioMemData->ipc[ ii ];
                    pCds->pci_dac[ kk ] = (long)( ioMemData->iodata[ ii ] );
                    // pCds->dacCount++;
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
        if ( ioMemData->model[ ii ] == GSC_18AI32SSC1M )
            fastadcCnt++;
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

    // Dolphin PCIe network style. Control units will perform I/O transactions
    // with RFM directly ie MASTER does not do RFM I/O. Master unit only maps
    // the RFM I/O space and passes pointers to control models.

    // Control app gets RFM module count from MASTER.
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


void
print_io_info( CDS_HARDWARE* cdsp , int iopmodel)
{
    int ii, jj, kk;
    int channels = 0;
    jj = 0;

#ifndef USER_SPACE
    printf( "" SYSTEM_NAME_STRING_LOWER ":startup time is %ld\n",
            current_time_fe( ) );
    printf( "" SYSTEM_NAME_STRING_LOWER ":cpu clock %u\n", cpu_khz );
#endif
    printf( "" SYSTEM_NAME_STRING_LOWER ":EPICSM at 0x%lx\n",
            (unsigned long)_epics_shm );
    printf( "" SYSTEM_NAME_STRING_LOWER ":IPC    at 0x%lx\n",
            (unsigned long)_ipc_shm );
    printf( "" SYSTEM_NAME_STRING_LOWER ":IOMEM  at 0x%lx size 0x%lx\n",
            ( (unsigned long)_ipc_shm + 0x4000 ),
            sizeof( IO_MEM_DATA ) );
    printf( "" SYSTEM_NAME_STRING_LOWER ":DAQSM at 0x%lx\n",
            (unsigned long)_daq_shm );
    printf( "" SYSTEM_NAME_STRING_LOWER ":configured to use %d cards\n",
            cdsp->cards );
    kk = 0;
    printf( "******************************************************************"
            "*********\n" );
    printf( "" SYSTEM_NAME_STRING_LOWER ":%d ADC cards found\n",
            cdsp->adcCount );
    for ( ii = 0; ii < cdsp->adcCount; ii++ )
    {
        kk++;
        if ( cdsp->adcType[ ii ] == GSC_18AI32SSC1M )
        {
            printf( "" SYSTEM_NAME_STRING_LOWER ":\tADC %d is a GSC_18AI32SSC1M module\n", ii );
            jj = ( cdsp->adcConfig[ ii ] >> 16 ) & 0x3;
            switch ( jj )
            {
            case 0:
                channels = 32;
                break;
            case 1:
                channels = 16;
                break;
            case 2:
                channels = 8;
                break;
            case 3:
                channels = 4;
                break;
            default:
                channels = 0;
                break;
            }
            if(iopmodel) {
                printf( "" SYSTEM_NAME_STRING_LOWER ":\t\tChannels = %d \n", channels );
                printf( "" SYSTEM_NAME_STRING_LOWER ":\t\tFirmware Rev = %d \n\n",
                        ( cdsp->adcConfig[ ii ] & 0xfff ) );
            }
        }
        if ( cdsp->adcType[ ii ] == GSC_16AI64SSA )
        {
            printf( "" SYSTEM_NAME_STRING_LOWER ":\tADC %d is a GSC_16AI64SSA module\n", ii );
            printf( "" SYSTEM_NAME_STRING_LOWER ":\tCard number is %d\n", cdsp->adcInstance[ ii ] );
            if(! iopmodel) {
            printf( "" SYSTEM_NAME_STRING_LOWER ":\tMemory at block %d\n", cdsp->adcConfig[ ii ] );
            }
            if(iopmodel) {
                if ( ( cdsp->adcConfig[ ii ] & 0x10000 ) > 0 )
                    jj = 32;
                else
                    jj = 64;
                printf( "" SYSTEM_NAME_STRING_LOWER ":\t\tChannels = %d \n", channels );
                printf( "" SYSTEM_NAME_STRING_LOWER ":\t\tFirmware Rev = %d \n\n",
                        ( cdsp->adcConfig[ ii ] & 0xfff ) );
            }
        }
    }
    printf( "******************************************************************"
            "*********\n" );
    printf( "" SYSTEM_NAME_STRING_LOWER ":%d DAC cards found\n",
            cdsp->dacCount );
    for ( ii = 0; ii < cdsp->dacCount; ii++ )
    {
        kk++;
        if ( cdsp->dacType[ ii ] == GSC_18AO8 )
        {
            printf( "" SYSTEM_NAME_STRING_LOWER ":\tDAC %d is a GSC_18AO8 module\n", ii );
            printf( "" SYSTEM_NAME_STRING_LOWER ":\tCard number is %d\n", cdsp->dacInstance[ ii ] );
            printf( "" SYSTEM_NAME_STRING_LOWER ":\tMemory at block %d\n", cdsp->dacConfig[ ii ] );
            if(iopmodel) {
                channels = 8;
                if(cdsp->dacAcr[ii] & 0x10000) channels = 4;
                printf( "" SYSTEM_NAME_STRING_LOWER ":\tChannels = %d \n", channels );
                printf( "" SYSTEM_NAME_STRING_LOWER ":\tFirmware Rev = %d \n\n",
                        ( cdsp->dacAcr[ ii ] & 0xfff ) );
            }
        }
        if ( cdsPciModules.dacType[ ii ] == GSC_20AO8 )
        {
            printf( "" SYSTEM_NAME_STRING_LOWER ":\tDAC %d is a GSC_20AO8 module\n", ii );
            printf( "" SYSTEM_NAME_STRING_LOWER ":\tCard number is %d\n", cdsp->dacInstance[ ii ] );
            printf( "" SYSTEM_NAME_STRING_LOWER ":\tMemory at block %d\n", cdsp->dacConfig[ ii ] );
            if(iopmodel) {
                channels = 8;
                if(cdsp->dacAcr[ii] & 0x10000) channels = 4;
                printf( "" SYSTEM_NAME_STRING_LOWER ":\tChannels = %d \n", channels );
                printf( "" SYSTEM_NAME_STRING_LOWER ":\tFirmware Rev = %d \n\n",
                        ( cdsp->dacAcr[ ii ] & 0xfff ) );
            }
        }
        if ( cdsp->dacType[ ii ] == GSC_16AO16 )
        {
            printf( "" SYSTEM_NAME_STRING_LOWER ":\tDAC %d is a GSC_16AO16 module\n", ii );
            printf( "" SYSTEM_NAME_STRING_LOWER ":\tCard number is %d\n", cdsp->dacInstance[ ii ] );
            printf( "" SYSTEM_NAME_STRING_LOWER ":\tMemory at block %d\n", cdsp->dacConfig[ ii ] );
            if(iopmodel) {
                channels = 16;
                printf( "" SYSTEM_NAME_STRING_LOWER ":\tChannels = %d \n", channels );
                printf( "" SYSTEM_NAME_STRING_LOWER ":\tFirmware Rev = %d \n\n",
                        ( cdsp->dacAcr[ ii ] & 0xfff ) );
            }
        }
    }
    kk += cdsp->dioCount;
    printf( "******************************************************************"
            "*********\n" );
    printf( "" SYSTEM_NAME_STRING_LOWER ":%d DIO cards found\n",
            cdsp->dioCount );
    printf( "******************************************************************"
            "*********\n" );
    printf( "" SYSTEM_NAME_STRING_LOWER ":%d IIRO-8 Isolated DIO cards found\n",
            cdsp->iiroDioCount );
    printf( "******************************************************************"
            "*********\n" );
    printf( "" SYSTEM_NAME_STRING_LOWER
            ":%d IIRO-16 Isolated DIO cards found\n",
            cdsp->iiroDio1Count );
    pLocalEpics->epicsOutput.bioMon[ 0 ] = cdsp->iiroDio1Count;
    printf( "******************************************************************"
            "*********\n" );
    printf( "" SYSTEM_NAME_STRING_LOWER ":%d Contec 32ch PCIe DO cards found\n",
            cdsp->cDo32lCount );
    pLocalEpics->epicsOutput.bioMon[ 1 ] = cdsp->cDo32lCount;
    printf( "" SYSTEM_NAME_STRING_LOWER ":%d Contec PCIe DIO1616 cards found\n",
            cdsp->cDio1616lCount );
    pLocalEpics->epicsOutput.bioMon[ 2 ] = cdsp->cDio1616lCount;
    printf( "" SYSTEM_NAME_STRING_LOWER ":%d Contec PCIe DIO6464 cards found\n",
            cdsp->cDio6464lCount );
    pLocalEpics->epicsOutput.bioMon[ 3 ] = cdsp->cDio6464lCount;
    printf( "" SYSTEM_NAME_STRING_LOWER ":%d DO cards found\n", cdsp->doCount );
    printf( "" SYSTEM_NAME_STRING_LOWER
            ":Total of %d I/O modules found and mapped\n",
            kk );
    printf( "******************************************************************"
            "*********\n" );
    printf( "" SYSTEM_NAME_STRING_LOWER ":%d RFM cards found\n",
            cdsp->rfmCount );
    for ( ii = 0; ii < cdsp->rfmCount; ii++ )
    {
        printf( "\tRFM %d is a VMIC_%x module with Node ID %d\n",
                ii,
                cdsp->rfmType[ ii ],
                cdsp->rfmConfig[ ii ] );
        printf( "address is 0x%lx\n", cdsp->pci_rfm[ ii ] );
    }
    printf( "******************************************************************"
            "*********\n" );
    if ( cdsp->gps )
    {
        printf( "" SYSTEM_NAME_STRING_LOWER ":IRIG-B card found %d\n",
                cdsp->gpsType );
        printf( "**************************************************************"
                "*************\n" );
    }
    for ( ii = 0; ii < cdsp->dolphinCount; ii++ )
    {
        printf( "\tDolphin found %d\n", ii );
        printf( "Read address is 0x%lx\n", cdsp->dolphinRead[ ii ] );
        printf( "Write address is 0x%lx\n", cdsp->dolphinWrite[ ii ] );
    }
}


