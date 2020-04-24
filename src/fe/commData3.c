/// 	@file commData3.c
///	@brief This is the generic software for communicating realtime data
/// between CDS applications.
///	@detail This software supports data communication via: \n
///		1) Shared memory, between two processes on the same computer \n
///		2) GE Fanuc 5565 Reflected Memory PCIe hardware \n
///		3) Dolphinics Reflected Memory over a PCIe network.
///     @author R.Bork, A.Ivanov
///     @copyright Copyright (C) 2014 LIGO Project 	\n
///<	California Institute of Technology 		\n
///<	Massachusetts Institute of Technology		\n\n
///     @license This program is free software: you can redistribute it and/or
///     modify
///<    it under the terms of the GNU General Public License as published by
///<    the Free Software Foundation, version 3 of the License. \n This program
///<    is distributed in the hope that it will be useful, but WITHOUT ANY
///<    WARRANTY; without even the implied warranty of MERCHANTABILITY or
///<    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
///<    for more details.

#include "commData3.h"
// #include "isnan.h"
#include <asm/cacheflush.h>

#ifdef COMMDATA_INLINE
#define INLINE static inline
#else
#define INLINE
#endif

///	This function is called from the user application to initialize
/// communications structures 	and pointers.
///	@param[in] connects = total number of IPC connections in the application
///	@param[in] rate = Sample rate of the calling application eg 2048
///	@param[in,out] ipcInfo[] = Stucture to hold information about each IPC
INLINE void
commData3Init(
    int connects, // total number of IPC connections in the application
    int rate, // Sample rate of the calling application eg 2048, 16384, etc.
    CDS_IPC_INFO ipcInfo[] // IPC information structure
)
{
    int           ii;
    unsigned long ipcMemOffset;

// printf("size of data block = 0x%x\n", sizeof(CDS_IPC_COMMS));
// printf("Dolphin num = %d \n",cdsPciModules.dolphinCount);
// printf("\tLocal at 0x%x and 0x%x
// \n",cdsPciModules.dolphinRead[0],cdsPciModules.dolphinWrite[0]);
// printf("\tRFM   at 0x%x and 0x%x
// \n",cdsPciModules.dolphinRead[1],cdsPciModules.dolphinWrite[1]);
#ifdef RFM_DELAY
// printf("Model compiled with RFM DELAY !!\n");
#endif
    for ( ii = 0; ii < connects; ii++ )
    {
        // Set the sendCycle field, used by all send/rcv to determine IPC data
        // block to write/read
        //      All cycle counts sent as part of data sync word are based on max
        //      supported rate of
        // 	65536 cycles/sec, regardless of sender/receiver native cycle
        // rate.
        ipcInfo[ ii ].sendCycle = IPC_MAX_RATE / rate;
        // Sender always sends data at his native rate. It is the responsiblity
        // of the reciever to sync to this rate, regardless of the rate of the
        // receiver application.
        if ( ipcInfo[ ii ].mode == IRCV ) // RCVR
        {
            if ( ipcInfo[ ii ].sendRate >= rate )
            {
                ipcInfo[ ii ].rcvRate = ipcInfo[ ii ].sendRate / rate;
                ipcInfo[ ii ].rcvCycle = 1;
            }
            else
            {
                ipcInfo[ ii ].rcvRate = rate / ipcInfo[ ii ].sendRate;
                ipcInfo[ ii ].rcvCycle = ipcInfo[ ii ].rcvRate;
            }
        }
        // Clear the data point
        ipcInfo[ ii ].data = 0.0;
        ipcInfo[ ii ].pIpcDataRead[ 0 ] = NULL;
        ipcInfo[ ii ].pIpcDataWrite[ 0 ] = NULL;

        // Save pointers to the IPC communications memory locations.
        if ( ipcInfo[ ii ].netType == IRFM0 )
            ipcMemOffset = IPC_PCIE_BASE_OFFSET + RFM0_OFFSET;
        if ( ipcInfo[ ii ].netType == IRFM1 )
            ipcMemOffset = IPC_PCIE_BASE_OFFSET + RFM1_OFFSET;
        if ( ( ipcInfo[ ii ].netType == IRFM0 ||
               ipcInfo[ ii ].netType == IRFM1 ) &&
             ( ipcInfo[ ii ].mode == ISND ) &&
             ( cdsPciModules.dolphinWrite[ 1 ] ) )
        {
            ipcInfo[ ii ].pIpcDataWrite[ 0 ] =
                (CDS_IPC_COMMS*)( (volatile char*)( cdsPciModules
                                                        .dolphinWrite[ 1 ] ) +
                                  ipcMemOffset );
        }
        if ( ( ipcInfo[ ii ].netType == IRFM0 ||
               ipcInfo[ ii ].netType == IRFM1 ) &&
             ( ipcInfo[ ii ].mode == IRCV ) &&
             ( cdsPciModules.dolphinRead[ 1 ] ) )
        {
            ipcInfo[ ii ].pIpcDataRead[ 0 ] =
                (CDS_IPC_COMMS*)( (volatile char*)( cdsPciModules
                                                        .dolphinRead[ 1 ] ) +
                                  ipcMemOffset );
        }
        if ( ipcInfo[ ii ].netType ==
             ISHME ) // Computer shared memory ******************************
        {
            if ( ipcInfo[ ii ].mode == ISND )
                ipcInfo[ ii ].pIpcDataWrite[ 0 ] =
                    (CDS_IPC_COMMS*)( _ipc_shm + IPC_BASE_OFFSET );
            else
                ipcInfo[ ii ].pIpcDataRead[ 0 ] =
                    (CDS_IPC_COMMS*)( _ipc_shm + IPC_BASE_OFFSET );
        }
        // PCIe communications requires one pointer for sending data and a
        // second one for receiving data.
        if ( ( ipcInfo[ ii ].netType == IPCIE ) &&
             ( ipcInfo[ ii ].mode == IRCV ) &&
             ( cdsPciModules.dolphinRead[ 0 ] ) )
        {
            ipcInfo[ ii ].pIpcDataRead[ 0 ] =
                (CDS_IPC_COMMS*)( (volatile char*)( cdsPciModules
                                                        .dolphinRead[ 0 ] ) +
                                  IPC_PCIE_BASE_OFFSET );
        }
        if ( ( ipcInfo[ ii ].netType == IPCIE ) &&
             ( ipcInfo[ ii ].mode == ISND ) &&
             ( cdsPciModules.dolphinWrite[ 0 ] ) )
        {
            ipcInfo[ ii ].pIpcDataWrite[ 0 ] =
                (CDS_IPC_COMMS*)( (volatile char*)( cdsPciModules
                                                        .dolphinWrite[ 0 ] ) +
                                  IPC_PCIE_BASE_OFFSET );
            // printf("Net Type = PCIE SEND IPC at 0x%p
            // *********************************\n",ipcInfo[ii].pIpcData);
        }
#if 0
	// Following for diags, if desired. Otherwise, leave out as it fills dmesg
	if(ipcInfo[ii].mode == ISND && ipcInfo[ii].netType != ISHME) {
        printf("IPC Number = %d\n",ipcInfo[ii].ipcNum);
        printf("IPC Name = %s\n",ipcInfo[ii].name);
        printf("Sender Model Name = %s\n",ipcInfo[ii].senderModelName);
        printf("RCV Rate  = %d\n",ipcInfo[ii].rcvRate);
        printf("Send Computer Number  = %d\n",ipcInfo[ii].sendNode);
        printf("Send address  = %lx\n",(unsigned long)&ipcInfo[ii].pIpcDataWrite[0]->dBlock[0][ipcInfo[ii].ipcNum].data);
	}
#endif
    }
    // Send connection list to dmesg
    for ( ii = 0; ii < connects; ii++ )
    {
        if ( ipcInfo[ ii ].mode == ISND && ipcInfo[ ii ].netType != ISHME )
        {
            // printf("IPC Name = %s
            // \t%d\t%d\t%lx\t%lx\n",ipcInfo[ii].name,ipcInfo[ii].netType,ipcInfo[ii].ipcNum,
            // (unsigned
            // long)&ipcInfo[ii].pIpcDataWrite[0]->dBlock[0][ipcInfo[ii].ipcNum].data,
            // (unsigned
            // long)&ipcInfo[ii].pIpcDataWrite[0]->dBlock[63][ipcInfo[ii].ipcNum].timestamp);
        }
    }
}

// *************************************************************************************************
///	This function is called from the user application to send data via IPC
/// connections.
///	@param[in] connects = total number of IPC connections in the application
///	@param[in,out] ipcInfo[] = Stucture to hold information about each IPC
///	@param[in] timeSec = Present GPS time in GPS seconds
///	@param[in] cycle = Present cycle of the user application making this
/// call.
INLINE void commData3Send(
    int          connects, // Total number of IPC connections in the application
    CDS_IPC_INFO ipcInfo[], // IPC information structure
    int          timeSec, // Present GPS Second
    int          cycle ) // Application cycle count (0 to FE_CODE_RATE)

// This routine sends out all IPC data marked as a send (SND) channel in the IPC
// INFO list. Data is sent at the native rate of the calling application. Data
// sent is of type double, with timestamp and 65536 cycle count combined into
// long.
{
    unsigned long
        syncWord; ///	\param syncWord Combined GPS timestamp and cycle counter
    int ipcIndex; ///	\param ipcIndex Pointer to next IPC data buffer
    int dataCycle; ///	\param dataCycle Cycle counter 0-65535
    int ii = 0; ///	\param ii Loop counter
    int chan; ///	\param chan Local ipc number
    int sendBlock; ///	\param sendBlock Data block data is to be sent to
    int lastPcie = -1;

#ifdef RFM_DELAY
    // Need to write ahead one extra block
    int mycycle = ( cycle + 1 );
    sendBlock = ( ( mycycle + 1 ) * ( IPC_MAX_RATE / IPC_RATE ) ) % IPC_BLOCKS;
    dataCycle = ( ( mycycle + 1 ) * ipcInfo[ 0 ].sendCycle ) % IPC_MAX_RATE;
    if ( dataCycle == 0 || dataCycle == ipcInfo[ 0 ].sendCycle )
        syncWord = timeSec + 1;
    else
        syncWord = timeSec;
    syncWord = ( syncWord << 32 ) + dataCycle;
#else
    sendBlock = ( ( cycle + 1 ) * ( IPC_MAX_RATE / IPC_RATE ) ) % IPC_BLOCKS;
    // Calculate the SYNC word to be sent with all data.
    // Determine the cycle count to be sent with the data
    dataCycle = ( ( cycle + 1 ) * ipcInfo[ 0 ].sendCycle ) % IPC_MAX_RATE;
    // Since this is write ahead, need to increment the GPS second if
    // writing to the first block of a new second.
    if ( dataCycle == 0 )
        syncWord = timeSec + 1;
    else
        syncWord = timeSec;
    // Combine GPS seconds and cycle counter into long word.
    syncWord = ( syncWord << 32 ) + dataCycle;
#endif

    // Want to send out RFM signals first to allow maximum time for data
    // delivery.
    for ( ii = 0; ii < connects; ii++ )
    {
        // If IPC Sender on RFM Network:
        // RFM network has highest latency, so want to get these signals out
        // first.
        if ( ( ipcInfo[ ii ].mode == ISND ) &&
             ( ( ipcInfo[ ii ].netType == IRFM0 ) ||
               ( ipcInfo[ ii ].netType == IRFM1 ) ) )
        {
            chan = ipcInfo[ ii ].ipcNum;
            // Determine next block to write in IPC_BLOCKS block buffer
            ipcIndex = ipcInfo[ ii ].ipcNum;
            // Don't write to PCI RFM if network error detected by IOP
            if ( ipcInfo[ ii ].pIpcDataWrite[ 0 ] != NULL )
            {
                // Write Data
                ipcInfo[ ii ]
                    .pIpcDataWrite[ 0 ]
                    ->dBlock[ sendBlock ][ ipcIndex ]
                    .data = ipcInfo[ ii ].data;
                // Write timestamp/cycle counter word
                ipcInfo[ ii ]
                    .pIpcDataWrite[ 0 ]
                    ->dBlock[ sendBlock ][ ipcIndex ]
                    .timestamp = syncWord;
                lastPcie = ii;
            }
        }
    }
// Flush out the last PCIe transmission
    if ( lastPcie >= 0 )
    {
        clflush_cache_range(
            (void*)&( ipcInfo[ lastPcie ]
                          .pIpcDataWrite[ 0 ]
                          ->dBlock[ sendBlock ][ ipcInfo[ lastPcie ].ipcNum ]
                          .data ),
            16 );
    }
    lastPcie = -1;
#ifdef RFM_DELAY
    // We don't want to delay SHMEM or PCIe writes, so calc block as usual,
    // so need to recalc send block and syncWord.
    sendBlock = ( ( cycle + 1 ) * ( IPC_MAX_RATE / IPC_RATE ) ) % IPC_BLOCKS;
    // Calculate the SYNC word to be sent with all data.
    // Determine the cycle count to be sent with the data
    dataCycle = ( ( cycle + 1 ) * ipcInfo[ 0 ].sendCycle ) % IPC_MAX_RATE;
    // Since this is write ahead, need to increment the GPS second if
    // writing to the first block of a new second.
    if ( dataCycle == 0 )
        syncWord = timeSec + 1;
    else
        syncWord = timeSec;
    // Combine GPS seconds and cycle counter into long word.
    syncWord = ( syncWord << 32 ) + dataCycle;
#endif

    for ( ii = 0; ii < connects; ii++ )
    {
        // If IPC Sender on PCIE Network or via Shared Memory:
        if ( ( ipcInfo[ ii ].mode == ISND ) &&
             ( ( ipcInfo[ ii ].netType == ISHME ) ||
               ( ipcInfo[ ii ].netType == IPCIE ) ) )
        {
            chan = ipcInfo[ ii ].ipcNum;
            // Determine next block to write in IPC_BLOCKS block buffer
            ipcIndex = ipcInfo[ ii ].ipcNum;
            // Don't write to PCI RFM if network error detected by IOP
            if ( ipcInfo[ ii ].pIpcDataWrite[ 0 ] != NULL )
            {
                // Write Data
                ipcInfo[ ii ]
                    .pIpcDataWrite[ 0 ]
                    ->dBlock[ sendBlock ][ ipcIndex ]
                    .data = ipcInfo[ ii ].data;
                // Write timestamp/cycle counter word
                ipcInfo[ ii ]
                    .pIpcDataWrite[ 0 ]
                    ->dBlock[ sendBlock ][ ipcIndex ]
                    .timestamp = syncWord;
                if ( ipcInfo[ ii ].netType == IPCIE )
                {
                    lastPcie = ii;
                }
            }
        }
    }
    // Flush out the last PCIe transmission
    if ( lastPcie >= 0 )
    {
        clflush_cache_range(
            (void*)&( ipcInfo[ lastPcie ]
                          .pIpcDataWrite[ 0 ]
                          ->dBlock[ sendBlock ][ ipcInfo[ lastPcie ].ipcNum ]
                          .data ),
            16 );
    }
}

// *************************************************************************************************
///	This function is called from the user application to receive data via
/// IPC connections.
///	@param[in] connects = total number of IPC connections in the application
///	@param[in,out] ipcInfo[] = Stucture to hold information about each IPC
///	@param[in] timeSec = Present GPS time in GPS seconds
///	@param[in] cycle = Present cycle of the user application making this
/// call.
INLINE void commData3Receive(
    int          connects, // Total number of IPC connections in the application
    CDS_IPC_INFO ipcInfo[], // IPC information structure
    int          timeSec, // Present GPS Second
    int          cycle ) // Application cycle count (0 to FE_CODE_RATE)

// This routine receives all IPC data marked as a read (RCV) channel in the IPC
// INFO list.
{
    unsigned long syncWord; // Combined GPS timestamp and cycle counter word
                            // received with data
    unsigned long mySyncWord; // Local version of syncWord for comparison and
                              // error detection
    int ipcIndex; // Pointer to next IPC data buffer
    int cycle65k; // All data sent with 64K cycle count; need to convert local
                  // app cycle count to match
    int    ii;
    int    rcvBlock; // Which of the IPC_BLOCKS IPC data blocks to read from
    double tmp; // Temp location for data for checking NaN
    // static unsigned long ptim = 0;	// last printing time
    // static unsigned long nskipped = 0;	// number of skipped error
    // messages (couldn't print that fast)

    // Determine which block to read, based on present code cycle
    rcvBlock = ( ( cycle ) * ( IPC_MAX_RATE / IPC_RATE ) ) % IPC_BLOCKS;
    for ( ii = 0; ii < connects; ii++ )
    {
        if ( ipcInfo[ ii ].mode == IRCV ) // Zero = Rcv and One = Send
        {
            if ( !( cycle % ipcInfo[ ii ].rcvCycle ) ) // Time to rcv
            {
                // Determine which data block to read when RCVR runs faster than
                // sender if(ipcInfo[ii].rcvCycle > 1) ipcIndex = (cycle /
                // ipcInfo[ii].rcvCycle) % 64;

                // Data block to read if RCVR runs same speed or slower than
                // sender else ipcIndex = (cycle * ipcInfo[ii].rcvRate) % 64;

                if ( ipcInfo[ ii ].pIpcDataRead[ 0 ] != NULL )
                {

                    ipcIndex = ipcInfo[ ii ].ipcNum;
                    // Read GPS time/cycle count
                    tmp = ipcInfo[ ii ]
                              .pIpcDataRead[ 0 ]
                              ->dBlock[ rcvBlock ][ ipcIndex ]
                              .data;
                    syncWord = ipcInfo[ ii ]
                                   .pIpcDataRead[ 0 ]
                                   ->dBlock[ rcvBlock ][ ipcIndex ]
                                   .timestamp;
                    // Create local 65K cycle count
                    cycle65k = ( ( cycle * ipcInfo[ ii ].sendCycle ) );
                    mySyncWord = timeSec;
                    // Create local GPS time/cycle word for comparison to ipc
                    mySyncWord = ( mySyncWord << 32 ) + cycle65k;
                    // If IPC syncword = local syncword, data is good
                    if ( syncWord == mySyncWord )
                    {
                        ipcInfo[ ii ].data = tmp;
                        // If IPC syncword != local syncword, data is BAD
                        // Set error and leave value same as last good receive
                    }
                    else
                    {
                        ipcInfo[ ii ].errFlag++;
                    }
                }
                else
                {
                    ipcInfo[ ii ].errFlag++;
                }
            }
            // Send error per second output
            // if(cycle == 0)
            //{
            // if (ipcInfo[ii].errFlag) ipcErrBits |= 1 <<
            // (ipcInfo[ii].netType); else ipcErrBits &= ~(1 <<
            // (ipcInfo[ii].netType)); ipcInfo[ii].errFlag = 0;
            // }
        }
    }
    // On cycle 0, set error flags to send back to EPICS
    if ( cycle == 0 )
    {
        ipcErrBits = ipcErrBits & 0xf0;
        for ( ii = 0; ii < connects; ii++ )
        {
            if ( ipcInfo[ ii ].mode == IRCV ) // Zero = Rcv and One = Send
            {
                ipcInfo[ ii ].errTotal = ipcInfo[ ii ].errFlag;
                if ( ipcInfo[ ii ].errFlag )
                {
                    ipcErrBits |= 1 << ( ipcInfo[ ii ].netType );
                    ipcErrBits |= 16 << ( ipcInfo[ ii ].netType );
                    ;
                }
                ipcInfo[ ii ].errFlag = 0;
            }
        }
    }
}
