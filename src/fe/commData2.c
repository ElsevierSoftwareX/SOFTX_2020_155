// *************************************************************************************************
// This is the generic software for communicating realtime data between CDS applications.
// This software supports data communication via:
//	1) Shared memory, between two processes on the same computer
//	2) GE Fanuc 5565 Reflected Memory PCIe hardware
//	3) Dolphonics Reflected Memory over a PCIe network.
//

#include "commData2.h"
#include "isnan.h"

#ifdef COMMDATA_INLINE
#  define INLINE inline
#else
#  define INLINE
#endif

// *************************************************************************************************
// initialize the commData state structure
//
INLINE void commData2Init(
			  int connects, 		// total number of IPC connections in the application
			  int rate, 			// Sample rate of the calling application eg 2048, 16384, etc.
			  CDS_IPC_INFO ipcInfo[], 	// IPC information structure
			  long rfmAddress[]		// Address of reflected memory networks
							//	0 = GE 5565 module 1
							//	1 = GE 5565 module 2
							//	2 = PCIe Network receive address
							//	3 = PCIe Network write address
			  )
{
int ii;


  for(ii=0;ii<connects;ii++)
  {
	// All cycle counts sent as part of data sync word are based on max supported rate of 
	// 65536 cycles/sec, regardless of sender/receiver native cycle rate.
	ipcInfo[ii].sendCycle = 65536 / rate;	
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
        printf("IPC DATA for IPC %d ******************\n",ii);
        if(ipcInfo[ii].mode) printf("Mode = SENDER w Cycle = %d\n",ipcInfo[ii].sendCycle);
        else printf("Mode = RECEIVER w rcvRate and cycle = %d %d\n",ipcInfo[ii].rcvRate,ipcInfo[ii].rcvCycle);

	// Save pointers to the IPC communications memory locations.
        if(ipcInfo[ii].netType == IRFM)		// VMIC Reflected Memory *******************************
        {
		if(rfmAddress[0]) 
		{
                ipcInfo[ii].pIpcData  = (CDS_IPC_COMMS *)(rfmAddress[0] + IPC_BASE_OFFSET + IPC_BUFFER_SIZE * ipcInfo[ii].ipcNum);
                printf("Net Type = RFM 1 at 0x%x\n",(int)ipcInfo[ii].pIpcData);
		}
		ipcInfo[ii].pIpcData2 = NULL;
		if(rfmAddress[1]) 
		{
                ipcInfo[ii].pIpcData2  = (CDS_IPC_COMMS *)(rfmAddress[1] + IPC_BASE_OFFSET + IPC_BUFFER_SIZE * ipcInfo[ii].ipcNum);
                printf("Net Type = RFM 2 at 0x%x\n",(int)ipcInfo[ii].pIpcData2);
		}
        } 
        if(ipcInfo[ii].netType == ISHM)		// Computer shared memory ******************************
	{
                ipcInfo[ii].pIpcData = (CDS_IPC_COMMS *)(_ipc_shm + IPC_BASE_OFFSET + IPC_BUFFER_SIZE * ipcInfo[ii].ipcNum);
                printf("Net Type = LOCAL IPC at 0x%x\n",(int)ipcInfo[ii].pIpcData);
        }
	// PCIe communications requires one pointer for sending data and a second one for receiving data.
        if((ipcInfo[ii].netType == IPCI) && (ipcInfo[ii].mode == IRCV))
	{
                ipcInfo[ii].pIpcData = (CDS_IPC_COMMS *)(rfmAddress[IPC_PCIE_READ] + IPC_BUFFER_SIZE * ipcInfo[ii].ipcNum);
                printf("Net Type = PCIE RCV IPC %d at 0x%lx  *********************************\n",ipcInfo[ii].sendRate,(long)ipcInfo[ii].pIpcData);
        }
        if((ipcInfo[ii].netType == IPCI) && (ipcInfo[ii].mode == ISND))
	{
                ipcInfo[ii].pIpcData = (CDS_IPC_COMMS *)(rfmAddress[IPC_PCIE_WRITE] + IPC_BUFFER_SIZE * ipcInfo[ii].ipcNum);
                printf("Net Type = PCIE SEND IPC at 0x%lx  *********************************\n",(long)ipcInfo[ii].pIpcData);
        }
        printf("IPC Number = %d\n",ipcInfo[ii].ipcNum);
        printf("RCV Rate  = %d\n",ipcInfo[ii].rcvRate);
        printf("Send Computer Number  = %d\n",ipcInfo[ii].sendNode);

    }
}

// *************************************************************************************************
INLINE void commData2Send(int connects,  	 	// Total number of IPC connections in the application
			  CDS_IPC_INFO ipcInfo[], 	// IPC information structure
			  int timeSec, 			// Present GPS Second
			  int cycle)			// Application cycle count (0 to FE_CODE_RATE)

// This routine sends out all IPC data marked as a send (SND) channel in the IPC INFO list.
// Data is sent at the native rate of the calling application.
// Data sent is of type double, with timestamp and 65536 cycle count combined into long.
{
unsigned long syncWord;	// Combined GPS timestamp and cycle counter
int ipcIndex;		// Pointer to next IPC data buffer
int dataCycle;		// Cycle counter 0-65535
int ii;

for(ii=0;ii<connects;ii++)
{
        if(ipcInfo[ii].mode == ISND)
        {
		// Determine the cycle count to be sent with the data
                dataCycle = ((cycle + 1) * ipcInfo[ii].sendCycle) % 65536;
		// Since this is write ahead, need to increment the GPS second if
		// writing to the first block of a new second.
		if(dataCycle == 0) syncWord = timeSec + 1;
		else syncWord = timeSec;
		// Combine GPS seconds and cycle counter into long word.
		syncWord = (syncWord << 32) + dataCycle;
		// Determine next block to write in 64 block buffer
		ipcIndex = (cycle+1) % 64;
		// Don't write to PCI RFM if network error detected by IOP
        	if((ipcInfo[ii].netType != IPCI) || (iop_rfm_valid))
		{
		// Write Data
                ipcInfo[ii].pIpcData->data[ipcIndex] = ipcInfo[ii].data;
		clflush_cache_range (ipcInfo[ii].pIpcData->data+ipcIndex,
				     sizeof(ipcInfo[ii].pIpcData->data[0]));
		// Write timestamp/cycle counter word
                ipcInfo[ii].pIpcData->timestamp[ipcIndex] = syncWord;
		clflush_cache_range (ipcInfo[ii].pIpcData->timestamp+ipcIndex,
				     sizeof(ipcInfo[ii].pIpcData->timestamp[0]));
		}
		// If 2 RFM cards, send data to both networks
		if((ipcInfo[ii].netType == IRFM) && (ipcInfo[ii].pIpcData2 != NULL))	
		{
			// Write Data
			ipcInfo[ii].pIpcData2->data[ipcIndex] = ipcInfo[ii].data;
			clflush_cache_range (ipcInfo[ii].pIpcData2->data+ipcIndex,
				     	     sizeof(ipcInfo[ii].pIpcData2->data[0]));
			// Write timestamp/cycle counter word
			ipcInfo[ii].pIpcData2->timestamp[ipcIndex] = syncWord;
			clflush_cache_range (ipcInfo[ii].pIpcData2->timestamp+ipcIndex,
				     	     sizeof(ipcInfo[ii].pIpcData2->timestamp[0]));
		}
        }
}

}

// *************************************************************************************************
INLINE void commData2Receive(int connects,  	 	// Total number of IPC connections in the application
			     CDS_IPC_INFO ipcInfo[], 	// IPC information structure
			     int timeSec, 		// Present GPS Second
			     int cycle)			// Application cycle count (0 to FE_CODE_RATE)

// This routine receives all IPC data marked as a read (RCV) channel in the IPC INFO list.
{
unsigned long syncWord;		// Combined GPS timestamp and cycle counter word received with data
unsigned long mySyncWord;	// Local version of syncWord for comparison and error detection
int ipcIndex;			// Pointer to next IPC data buffer
int cycle65k;
//int testCycle;
int ii;

for(ii=0;ii<connects;ii++)
{
        if(ipcInfo[ii].mode == IRCV) // Zero = Rcv and One = Send
        {
	   if(!(cycle % ipcInfo[ii].rcvCycle)) // Time to rcv
	   {
		// Determine which data block to read when RCVR runs faster than sender
		if(ipcInfo[ii].rcvCycle > 1) ipcIndex = (cycle / ipcInfo[ii].rcvCycle) % 64;

		// Data block to read if RCVR runs same speed or slower than sender
		else ipcIndex = (cycle * ipcInfo[ii].rcvRate) % 64;
		
		// Read GPS time/cycle count 
		syncWord = ipcInfo[ii].pIpcData->timestamp[ipcIndex];
		//testCycle = syncWord & 0xffffffff;
		// Create local 65K cycle count
		cycle65k = ((cycle * ipcInfo[ii].sendCycle));
		mySyncWord = timeSec;
		// Create local GPS time/cycle word for comparison to ipc
		mySyncWord = (mySyncWord << 32) + cycle65k;
		// If IPC syncword = local syncword, data is good
		if(syncWord == mySyncWord) 
		//if(cycle65k == testCycle) 
		{
			double tmp = ipcInfo[ii].pIpcData->data[ipcIndex];
			if (isnan(tmp)) ipcInfo[ii].errFlag ++;
			else ipcInfo[ii].data = tmp;
		// If IPC syncword != local syncword, data is BAD
		} else {
			//printf("sync error my=0x%lx remote=0x%lx\n", mySyncWord, syncWord);
			//ipcInfo[ii].data = ipcInfo[ii].pIpcData->data[ipcIndex];
			ipcInfo[ii].errFlag ++;
		}
	   }
	   // Send error per second output
	   if(cycle == 0)
	   {
			ipcInfo[ii].errTotal = ipcInfo[ii].errFlag;
				ipcInfo[ii].errFlag = 0;
	   }
        }
}
}


