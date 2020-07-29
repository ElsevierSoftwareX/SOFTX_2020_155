/// @file rcguserCommon.c
/// @brief File contains routines for user app startup

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
print_io_info( CDS_HARDWARE* cdsp )
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
