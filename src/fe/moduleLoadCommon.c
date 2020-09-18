///     @file moduleLoadCommon.c
///     @brief File contains common routines for moduleLoad.c
///     for both IOP and User apps.`

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

void
print_exit_messages( int error_type, int error_sub )
{
    printf( "" SYSTEM_NAME_STRING_LOWER " : Brought the CPU back up\n" );
    switch ( error_type )
    {
    case FILT_INIT_ERROR:
        printf( "" SYSTEM_NAME_STRING_LOWER " FE error: %s\n",
                "exited on filter initiialization error" );
        break;
    case DAQ_INIT_ERROR:
        printf( "" SYSTEM_NAME_STRING_LOWER " FE error: %s\n",
                "exited on DAQ initiialization error" );
        break;
    case CHAN_HOP_ERROR:
        printf( "" SYSTEM_NAME_STRING_LOWER " FE error: %s\n",
                "exited on ADC Channel Hopping error" );
        printf( "" SYSTEM_NAME_STRING_LOWER " : Error detected on ADC %d\n",
                error_sub );
        break;
    case BURT_RESTORE_ERROR:
        printf( "" SYSTEM_NAME_STRING_LOWER " FE error: %s\n",
                "exited on BURT restore error" );
        break;
    case DAC_INIT_ERROR:
        printf( "" SYSTEM_NAME_STRING_LOWER " FE error: %s\n",
                "exited on DAC module initialization error" );
        break;
    case ADC_TO_ERROR:
        printf( "" SYSTEM_NAME_STRING_LOWER " FE error: %s\n",
                "exited on ADC module timeout error" );
        printf( "" SYSTEM_NAME_STRING_LOWER " : Error detected on ADC %d\n",
                error_sub );
        break;
    default:
        printf( "Returning from cleanup_module "
                "for " SYSTEM_NAME_STRING_LOWER "\n" );
        break;
    }
}

#ifndef USER_SPACE
int
detach_shared_memory( )
{
    int  ret;
    char fname[ 128 ];

    ret = mbuf_release_area( SYSTEM_NAME_STRING_LOWER, _epics_shm );
    ret = mbuf_release_area( "ipc", _ipc_shm );
    ret = mbuf_release_area( "shmipc", _shmipc_shm );
    sprintf( fname, "%s_daq", SYSTEM_NAME_STRING_LOWER );
    ret = mbuf_release_area( fname, _daq_shm );
    return ret;
}
int
attach_shared_memory( )
{
    int  ret;
    char fname[ 128 ];

    /// Allocate EPICS memory area
    ret = mbuf_allocate_area( SYSTEM_NAME_STRING_LOWER, 64 * 1024 * 1024, 0 );
    if ( ret < 0 )
    {
        printk( "" SYSTEM_NAME_STRING_LOWER
                ": ERROR: mbuf_allocate_area(epics) failed; ret = %d\n",
                ret );
        return -12;
    }
    _epics_shm = (unsigned char*)( kmalloc_area[ ret ] );
    // Set pointer to EPICS area
    pLocalEpics = (CDS_EPICS*)&( (RFM_FE_COMMS*)_epics_shm )->epicsSpace;
    pLocalEpics->epicsOutput.fe_status = 0;

    /// Allocate IPC memory area
    ret = mbuf_allocate_area( "ipc", 32 * 1024 * 1024, 0 );
    if ( ret < 0 )
    {
        printk( "" SYSTEM_NAME_STRING_LOWER
                ": ERROR: mbuf_allocate_area(ipc) failed; ret = %d\n",
                ret );
        return -12;
    }
    _ipc_shm = (unsigned char*)( kmalloc_area[ ret ] );

    // Assign pointer to IOP/USER app comms space
    ioMemData = (IO_MEM_DATA*)( _ipc_shm + 0x4000 );

    /// Allocate Shared memory IPC comms memory area
    ret = mbuf_allocate_area( "shmipc", 16 * 1024 * 1024, 0 );
    if ( ret < 0 )
    {
        printk( "" SYSTEM_NAME_STRING_LOWER
                ": ERROR: mbuf_allocate_area(shmipc) failed; ret = %d\n",
                ret );
        return -12;
    }
    _shmipc_shm = (unsigned char*)( kmalloc_area[ ret ] );

    /// Allocate DAQ memory area
    sprintf( fname, "%s_daq", SYSTEM_NAME_STRING_LOWER );
    ret = mbuf_allocate_area( fname, 64 * 1024 * 1024, 0 );
    if ( ret < 0 )
    {
        printk( "" SYSTEM_NAME_STRING_LOWER
                ": ERROR:mbuf_allocate_area(daq) failed; ret = %d\n",
                ret );
        return -12;
    }
    _daq_shm = (unsigned char*)( kmalloc_area[ ret ] );
    daqPtr = (struct rmIpcStr*)_daq_shm;

    return 0;
}

void
send_io_info_to_mbuf( int totalcards, CDS_HARDWARE* pCds )
{
    int ii, jj, kk;

    /// Wirte PCIe card info to mbuf for use by userapp models
    // Clear out card model info in IO_MEM
    for ( ii = 0; ii < MAX_IO_MODULES; ii++ )
    {
        ioMemData->model[ ii ] = -1;
    }

    /// Master send module counts to control model via ipc shm
    ioMemData->totalCards = totalcards;
    ioMemData->adcCount = pCds->adcCount;
    ioMemData->dacCount = pCds->dacCount;
    ioMemData->bioCount = pCds->doCount;
    // kk will act as ioMem location counter for mapping modules
    kk = pCds->adcCount;
    for ( ii = 0; ii < pCds->adcCount; ii++ )
    {
        // MASTER maps ADC modules first in ipc shm for control models
        ioMemData->model[ ii ] = pCds->adcType[ ii ];
        ioMemData->card[ ii ] = pCds->adcInstance[ ii ];
        ioMemData->ipc[ ii ] =
            ii; // ioData memory buffer location for control model to use
    }
    for ( ii = 0; ii < pCds->dacCount; ii++ )
    {
        // Pass DAC info to control processes
        ioMemData->model[ kk ] = pCds->dacType[ ii ];
        ioMemData->card[ kk ] = pCds->dacInstance[ ii ];
        ioMemData->ipc[ kk ] = kk;
        // Following used by MASTER to point to ipc memory for inputting DAC
        // data from control models
        pCds->dacConfig[ ii ] = kk;
        kk++;
    }
    // MASTER sends DIO module information to control models
    // Note that for DIO, control modules will perform the I/O directly and
    // therefore need to know the PCIe address of these modules.
    ioMemData->bioCount = pCds->doCount;
    for ( ii = 0; ii < pCds->doCount; ii++ )
    {
        // MASTER needs to find Contec 1616 I/O card to control timing card.
        if ( pCds->doType[ ii ] == CON_1616DIO )
        {
            tdsControl[ tdsCount ] = ii;
            tdsCount++;
        }
        ioMemData->model[ kk ] = pCds->doType[ ii ];
        // Unlike ADC and DAC, where a memory buffer number is passed, a PCIe
        // address is passed for DIO cards.
        ioMemData->ipc[ kk ] = pCds->pci_do[ ii ];
        kk++;
    }
    // Following section maps Reflected Memory, both VMIC hardware style and
    // Dolphin PCIe network style. Control units will perform I/O transactions
    // with RFM directly ie MASTER does not do RFM I/O. Master unit only maps
    // the RFM I/O space and passes pointers to control models.

    /// Map VMIC RFM cards, if any
    ioMemData->rfmCount = pCds->rfmCount;
    for ( ii = 0; ii < pCds->rfmCount; ii++ )
    {
        // Master sends RFM memory pointers to control models
        ioMemData->pci_rfm[ ii ] = pCds->pci_rfm[ ii ];
        ioMemData->pci_rfm_dma[ ii ] = pCds->pci_rfm_dma[ ii ];
    }
#ifdef DOLPHIN_TEST
    /// Send Dolphin addresses to user app processes
    // dolphinCount is number of segments
    ioMemData->dolphinCount = pCds->dolphinCount;
    // dolphin read/write 0 is for local PCIe network traffic
    ioMemData->dolphinRead[ 0 ] = pCds->dolphinRead[ 0 ];
    ioMemData->dolphinWrite[ 0 ] = pCds->dolphinWrite[ 0 ];
    // dolphin read/write 1 is for long range PCIe (RFM) traffic
    ioMemData->dolphinRead[ 1 ] = pCds->dolphinRead[ 1 ];
    ioMemData->dolphinWrite[ 1 ] = pCds->dolphinWrite[ 1 ];

#else
    // Clear Dolphin pointers so the controller process sees NULLs
    ioMemData->dolphinCount = 0;
    ioMemData->dolphinRead[ 0 ] = 0;
    ioMemData->dolphinWrite[ 0 ] = 0;
    ioMemData->dolphinRead[ 1 ] = 0;
    ioMemData->dolphinWrite[ 1 ] = 0;
#endif
}
#endif
