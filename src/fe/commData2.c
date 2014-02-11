/// 	@file commData2.c
///	@brief This is the generic software for communicating realtime data between CDS applications.
///	@detail This software supports data communication via: \n
///		1) Shared memory, between two processes on the same computer \n
///		2) GE Fanuc 5565 Reflected Memory PCIe hardware \n
///		3) Dolphinics Reflected Memory over a PCIe network.
///     @author R.Bork, A.Ivanov
///     @copyright Copyright (C) 2014 LIGO Project 	\n
///<	California Institute of Technology 		\n
///<	Massachusetts Institute of Technology		\n\n
///     @license This program is free software: you can redistribute it and/or modify
///<    it under the terms of the GNU General Public License as published by
///<    the Free Software Foundation, version 3 of the License.			\n
///<    This program is distributed in the hope that it will be useful,
///<    but WITHOUT ANY WARRANTY; without even the implied warranty of
///<    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
///<    GNU General Public License for more details.



#include "commData2.h"
#include "isnan.h"
#include <asm/cacheflush.h>

#ifdef COMMDATA_INLINE
#  define INLINE inline
#else
#  define INLINE
#endif

///	This function is called from the user application to initialize communications structures
///	and pointers. 
///	@param[in] connects = total number of IPC connections in the application
///	@param[in] rate = Sample rate of the calling application eg 2048
///	@param[in,out] ipcInfo[] = Stucture to hold information about each IPC
INLINE void commData2Init(
			  int connects, 		// total number of IPC connections in the application
			  int rate, 			// Sample rate of the calling application eg 2048, 16384, etc.
			  CDS_IPC_INFO ipcInfo[] 	// IPC information structure
			  )
{
int ii;


printf("size of data block = %lu\n", sizeof(CDS_IPC_COMMS));
  for(ii=0;ii<connects;ii++)
  {
	// Set the sendCycle field, used by all send/rcv to determine IPC data block to write/read
	//      All cycle counts sent as part of data sync word are based on max supported rate of 
	// 	65536 cycles/sec, regardless of sender/receiver native cycle rate.
	ipcInfo[ii].sendCycle = IPC_MAX_RATE / rate;	
	// Sender always sends data at his native rate. It is the responsiblity of the reciever
	// to sync to this rate, regardless of the rate of the receiver application.
        if(ipcInfo[ii].mode == IRCV) // RCVR
	{
		if(ipcInfo[ii].sendRate >= rate)
		{
			ipcInfo[ii].rcvRate = ipcInfo[ii].sendRate / rate;
			ipcInfo[ii].rcvCycle = 1;
		} else {
			ipcInfo[ii].rcvRate = rate / ipcInfo[ii].sendRate;
			ipcInfo[ii].rcvCycle = ipcInfo[ii].rcvRate;
		}
	}
	// Clear the data point
        ipcInfo[ii].data = 0.0;
	ipcInfo[ii].pIpcData = NULL;
        // printf("IPC DATA for IPC %d ******************\n",ii);
        // if(ipcInfo[ii].mode) printf("Mode = SENDER w Cycle = %d\n",ipcInfo[ii].sendCycle);
        // else printf("Mode = RECEIVER w rcvRate and cycle = %d %d\n",ipcInfo[ii].rcvRate,ipcInfo[ii].rcvCycle);

	// Save pointers to the IPC communications memory locations.
        if(ipcInfo[ii].netType == IRFM0)		// VMIC Reflected Memory *******************************
        {
	  if(cdsPciModules.rfmCount > 0) {
	    // Send side will perform direct, individual writes to RFM
	    // Rcv side will use DMA, provided by IOP (Performance reasons ie without DMA, each read takes >2usec)
#ifdef RFM_DIRECT_READ
	    ipcInfo[ii].pIpcData  = (CDS_IPC_COMMS *)(cdsPciModules.pci_rfm[0] + IPC_BASE_OFFSET);
#else	    
	    if(ipcInfo[ii].mode == ISND) ipcInfo[ii].pIpcData  = (CDS_IPC_COMMS *)(cdsPciModules.pci_rfm[0] + IPC_BASE_OFFSET);
	    			    else ipcInfo[ii].pIpcData  = (CDS_IPC_COMMS *)(cdsPciModules.pci_rfm_dma[0]);
#endif
	    // printf("Net Type = RFM 0 at 0x%p\n",ipcInfo[ii].pIpcData);
	  }
	}
        if(ipcInfo[ii].netType == IRFM1)		// VMIC Reflected Memory *******************************
        {
	  if(cdsPciModules.rfmCount > 1) {
#ifdef RFM_DIRECT_READ
	    ipcInfo[ii].pIpcData  = (CDS_IPC_COMMS *)(cdsPciModules.pci_rfm[1] + IPC_BASE_OFFSET);
#else	    
	    if(ipcInfo[ii].mode == ISND) ipcInfo[ii].pIpcData  = (CDS_IPC_COMMS *)(cdsPciModules.pci_rfm[1] + IPC_BASE_OFFSET);
	    			    else ipcInfo[ii].pIpcData  = (CDS_IPC_COMMS *)(cdsPciModules.pci_rfm_dma[1]);
#endif
	    // printf("Net Type = RFM 1 at 0x%p\n",ipcInfo[ii].pIpcData);
	  }
	  // If there isn't a second card (like in the end stations), default to first card
	  if(cdsPciModules.rfmCount == 1) {
#ifdef RFM_DIRECT_READ
	    ipcInfo[ii].pIpcData  = (CDS_IPC_COMMS *)(cdsPciModules.pci_rfm[0] + IPC_BASE_OFFSET);
#else	    
	    if(ipcInfo[ii].mode == ISND) ipcInfo[ii].pIpcData  = (CDS_IPC_COMMS *)(cdsPciModules.pci_rfm[0] + IPC_BASE_OFFSET);
	    			    else ipcInfo[ii].pIpcData  = (CDS_IPC_COMMS *)(cdsPciModules.pci_rfm_dma[0]);
#endif
	    // printf("DEFAULTING TO RFM0 - ONLY ONE CARD\nNet Type = RFM 1 at 0x%p\n",ipcInfo[ii].pIpcData);
	  }
	}
        if(ipcInfo[ii].netType == ISHME)		// Computer shared memory ******************************
	{
                ipcInfo[ii].pIpcData = (CDS_IPC_COMMS *)(_ipc_shm + IPC_BASE_OFFSET);
                // printf("Net Type = LOCAL IPC at 0x%p\n",ipcInfo[ii].pIpcData);
        }
	// PCIe communications requires one pointer for sending data and a second one for receiving data.
        if((ipcInfo[ii].netType == IPCIE) && (ipcInfo[ii].mode == IRCV) && (cdsPciModules.dolphin[0]))
	{
                ipcInfo[ii].pIpcData = (CDS_IPC_COMMS *)((volatile char *)(cdsPciModules.dolphin[0]) + IPC_PCIE_BASE_OFFSET);
                // printf("Net Type = PCIE RCV IPC %d at 0x%p  *********************************\n",ipcInfo[ii].sendRate,ipcInfo[ii].pIpcData);
        }
        if((ipcInfo[ii].netType == IPCIE) && (ipcInfo[ii].mode == ISND) && (cdsPciModules.dolphin[1]))
	{
                ipcInfo[ii].pIpcData = (CDS_IPC_COMMS *)((volatile char *)(cdsPciModules.dolphin[1]) + IPC_PCIE_BASE_OFFSET);
                // printf("Net Type = PCIE SEND IPC at 0x%p  *********************************\n",ipcInfo[ii].pIpcData);
        }
#if 0
	// Following for diags, if desired. Otherwise, leave out as it fills dmesg
        printf("IPC Number = %d\n",ipcInfo[ii].ipcNum);
        printf("IPC Name = %s\n",ipcInfo[ii].name);
        printf("Sender Model Name = %s\n",ipcInfo[ii].senderModelName);
        printf("RCV Rate  = %d\n",ipcInfo[ii].rcvRate);
        printf("Send Computer Number  = %d\n",ipcInfo[ii].sendNode);
#endif

    }
}

// *************************************************************************************************
///	This function is called from the user application to send data via IPC connections.
///	@param[in] connects = total number of IPC connections in the application
///	@param[in,out] ipcInfo[] = Stucture to hold information about each IPC
///	@param[in] timeSec = Present GPS time in GPS seconds
///	@param[in] cycle = Present cycle of the user application making this call.
INLINE void commData2Send(int connects,  	 	// Total number of IPC connections in the application
			  CDS_IPC_INFO ipcInfo[], 	// IPC information structure
			  int timeSec, 			// Present GPS Second
			  int cycle)			// Application cycle count (0 to FE_CODE_RATE)

// This routine sends out all IPC data marked as a send (SND) channel in the IPC INFO list.
// Data is sent at the native rate of the calling application.
// Data sent is of type double, with timestamp and 65536 cycle count combined into long.
{
  unsigned long syncWord;	///	\param syncWord Combined GPS timestamp and cycle counter
  int ipcIndex;			///	\param ipcIndex Pointer to next IPC data buffer
  int dataCycle;		///	\param dataCycle Cycle counter 0-65535
  int ii = 0;			///	\param ii Loop counter
  int chan;			///	\param chan Local ipc number
  int sendBlock;		///	\param sendBlock Data block data is to be sent to
  int lastPcie = -1;

  sendBlock = ((cycle + 1) * (IPC_MAX_RATE / FE_RATE)) % IPC_BLOCKS;
  //int num_pcie = 0;

// Calculate the SYNC word to be sent with all data.
// Determine the cycle count to be sent with the data
  dataCycle = ((cycle + 1) * ipcInfo[0].sendCycle) % IPC_MAX_RATE;
// Since this is write ahead, need to increment the GPS second if
// writing to the first block of a new second.
  if(dataCycle == 0) syncWord = timeSec + 1;
  else syncWord = timeSec;
// Combine GPS seconds and cycle counter into long word.
  syncWord = (syncWord << 32) + dataCycle;

  for(ii=0;ii<connects;ii++)
  {
	// If IPC Sender on RFM Network:
	// RFM network has highest latency, so want to get these signals out first.
        if((ipcInfo[ii].mode == ISND) && ((ipcInfo[ii].netType == IRFM0) || (ipcInfo[ii].netType == IRFM1)))
        {
		chan = ipcInfo[ii].ipcNum;
		// Determine next block to write in IPC_BLOCKS block buffer
		ipcIndex = ipcInfo[ii].ipcNum;
		// Don't write to PCI RFM if network error detected by IOP
        	if(ipcInfo[ii].pIpcData != NULL)
		{
			// Write Data
                	ipcInfo[ii].pIpcData->dBlock[sendBlock][ipcIndex].data = ipcInfo[ii].data;
			// Write timestamp/cycle counter word
                	ipcInfo[ii].pIpcData->dBlock[sendBlock][ipcIndex].timestamp = syncWord;
		}
  	}
  }

  for(ii=0;ii<connects;ii++)
  {
	// If IPC Sender on PCIE Network or via Shared Memory:
        if((ipcInfo[ii].mode == ISND) && ((ipcInfo[ii].netType == ISHME) || (ipcInfo[ii].netType == IPCIE)))
        {
		chan = ipcInfo[ii].ipcNum;
		// Determine next block to write in IPC_BLOCKS block buffer
		ipcIndex = ipcInfo[ii].ipcNum;
		// Don't write to PCI RFM if network error detected by IOP
        	if(ipcInfo[ii].pIpcData != NULL)
		{
			// Write Data
                	ipcInfo[ii].pIpcData->dBlock[sendBlock][ipcIndex].data = ipcInfo[ii].data;
			// Write timestamp/cycle counter word
                	ipcInfo[ii].pIpcData->dBlock[sendBlock][ipcIndex].timestamp = syncWord;
		 	if(ipcInfo[ii].netType == IPCIE) {
				lastPcie = ii;
 			}
		}
        }
  }
  // Flush out the last PCIe transmission
  if (lastPcie >= 0) clflush_cache_range (&(ipcInfo[lastPcie].pIpcData->dBlock[sendBlock][ipcInfo[lastPcie].ipcNum].data), 16);
}

// *************************************************************************************************
///	This function is called from the user application to receive data via IPC connections.
///	@param[in] connects = total number of IPC connections in the application
///	@param[in,out] ipcInfo[] = Stucture to hold information about each IPC
///	@param[in] timeSec = Present GPS time in GPS seconds
///	@param[in] cycle = Present cycle of the user application making this call.
INLINE void commData2Receive(int connects,  	 	// Total number of IPC connections in the application
			     CDS_IPC_INFO ipcInfo[], 	// IPC information structure
			     int timeSec, 		// Present GPS Second
			     int cycle)			// Application cycle count (0 to FE_CODE_RATE)

// This routine receives all IPC data marked as a read (RCV) channel in the IPC INFO list.
{
unsigned long syncWord;		// Combined GPS timestamp and cycle counter word received with data
unsigned long mySyncWord;	// Local version of syncWord for comparison and error detection
int ipcIndex;			// Pointer to next IPC data buffer
int cycle65k;			// All data sent with 64K cycle count; need to convert local app cycle count to match
int ii;
int rcvBlock;			// Which of the IPC_BLOCKS IPC data blocks to read from
double tmp;			// Temp location for data for checking NaN
static unsigned long ptim = 0;	// last printing time
static unsigned long nskipped = 0;	// number of skipped error messages (couldn't print that fast)

  // Determine which block to read, based on present code cycle
  rcvBlock = ((cycle) * (IPC_MAX_RATE / FE_RATE)) % IPC_BLOCKS;
  for(ii=0;ii<connects;ii++)
  {
        if(ipcInfo[ii].mode == IRCV) // Zero = Rcv and One = Send
        {
	   if(!(cycle % ipcInfo[ii].rcvCycle)) // Time to rcv
	   {
		// Determine which data block to read when RCVR runs faster than sender
		// if(ipcInfo[ii].rcvCycle > 1) ipcIndex = (cycle / ipcInfo[ii].rcvCycle) % 64;

		// Data block to read if RCVR runs same speed or slower than sender
		// else ipcIndex = (cycle * ipcInfo[ii].rcvRate) % 64;
		
		if(ipcInfo[ii].pIpcData != NULL)
		{

			ipcIndex = ipcInfo[ii].ipcNum;
			// Read GPS time/cycle count 
			tmp = ipcInfo[ii].pIpcData->dBlock[rcvBlock][ipcIndex].data;
			syncWord = ipcInfo[ii].pIpcData->dBlock[rcvBlock][ipcIndex].timestamp;
			// Create local 65K cycle count
			cycle65k = ((cycle * ipcInfo[ii].sendCycle));
			mySyncWord = timeSec;
			// Create local GPS time/cycle word for comparison to ipc
			mySyncWord = (mySyncWord << 32) + cycle65k;
			// If IPC syncword = local syncword, data is good
			if(syncWord == mySyncWord) 
			{
			#if 0
				double tmp = ipcInfo[ii].pIpcData->data[ipcIndex];
				if (isnan(tmp)) ipcInfo[ii].errFlag ++;
				else ipcInfo[ii].data = tmp;
			#endif
				ipcInfo[ii].data = tmp;
			// If IPC syncword != local syncword, data is BAD
			} else {
#if 0
				if ((cycle_gps_time - startGpsTime) > 2) { // Do not print for the first 2 seconds
					if (ptim < cycle_gps_time) {
						if (nskipped) printf("IPC RCV ERROR: skipped %lu sync error messages\n", nskipped);
						printf("IPC RCV ERROR: ipc=%d name=%s sender=%s sync error my=0x%lx remote=0x%lx @ %d\n",
							ipcIndex, ipcInfo[ii].name, ipcInfo[ii].senderModelName, mySyncWord, syncWord, rcvBlock);
						ptim = cycle_gps_time;	// Print a single message per second
						nskipped = 0;
					} else {
						nskipped++;
					}
					
				}
#endif
				// ipcInfo[ii].data = ipcInfo[ii].pIpcData->data[ipcIndex];
				// ipcInfo[ii].data = tmp;
				ipcInfo[ii].errFlag ++;
			}
		} else {
				ipcInfo[ii].errFlag ++;
		}
	   }
	   // Send error per second output
	   if(cycle == 0)
	   {
		ipcInfo[ii].errTotal = ipcInfo[ii].errFlag;
		if (ipcInfo[ii].errFlag) ipcErrBits |= 1 << (ipcInfo[ii].netType);
		ipcInfo[ii].errFlag = 0;
	   }
        }
  }
}


