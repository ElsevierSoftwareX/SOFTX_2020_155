#include "commData2.h"

// make these inline?
#ifdef COMMDATA_INLINE
#  define INLINE inline
#else
#  define INLINE
#endif

// =================================================
// initialize the commData state structure
//
INLINE void commData2Init(int connects, CDS_IPC_KEY_LIST keylist[], int rate, CDS_IPC_INFO ipcInfo[], CDS_IPC_COMMS *pIpcData[], long rfmAddress)
{
int ipcTemp;
int ii,jj;
int localKey;
CDS_IPC_KEY_LIST masterList[3];
int ipcTotal = 3;

// MASTER KEYS **************************************************************************
// Presently hardcoded.
// Should be moved to a header file or sent as EPICS channels
// Bit pattern
// Lower four bits
//	0 = Client / Server (0 = Client (receiver), 1 = Server (sender)
//	1-2 = Communication Link Type (0 = shared memory, 1 = RFM, 2 = PCIE
//	3 = Unassigned
//	
//	4-11 = IPC Number (Must be unique for a given Link Type
//
//	12-19 = Sender rate given as this number times 2048
//	
//	20-27 = Sender PCIE network node id
//

strcpy(masterList[0].name,"DBB2SUS");
strcpy(masterList[1].name,"OM12SUS");
strcpy(masterList[2].name,"SUS2DBB");
masterList[0].masterKey = 0x20000; // Input to SUS:Q1_L2_ASCY from dbb at 65KHz w/err at SUS:Q1_L2_ASCP
masterList[1].masterKey = 0x10001; // Input to SUS:Q1_L2_LSC from om1 at 32KHz w/err at SUS:Q1_L1_ASCY (RFM)
masterList[2].masterKey = 0x08010; // Output to DBB:INPUT_1 from SUS at 16KHz w/err at DBB:INPUT_2

// ***************************************************************************************

  for(ii=0;ii<connects;ii++)
  {
     for(jj=0;jj<ipcTotal;jj++)
     {
		if((strcmp(masterList[jj].name,keylist[ii].name)) == 0)
		{
			localKey = masterList[jj].masterKey;
			ipcInfo[ii].mode = keylist[ii].masterKey; // Zero = Rcv and One = Send
	// Check bits 2-3 for Communication link type
        ipcInfo[ii].netType = localKey & 3; // Zero = Ipc, One = Rfm and Two = PCIE
	// Check bits 4-11 for ipc number
        ipcTemp = localKey >> 4;
        ipcInfo[ii].ipcNum = ipcTemp & 0xff;
	// Check bits 12-19 for sender rate
        ipcTemp = localKey >> 12;
	ipcInfo[ii].sendRate = (ipcTemp & 0xff) * 2048;
	ipcInfo[ii].sendCycle = 65536 / rate;
        if(!ipcInfo[ii].mode) // RCVR
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
	// Check bits 20-27 for Sender PCIE node id
        ipcTemp = localKey >> 20;
        ipcInfo[ii].sendComputerNum = ipcTemp & 0xff;
	// Clear the data point
        ipcInfo[ii].data = 0.0;
        printf("IPC DATA for IPC %d ******************\n",ii);
        if(ipcInfo[ii].mode) printf("Mode = SENDER w Cycle = %d\n",ipcInfo[ii].sendCycle);
        else printf("Mode = RECEIVER w rcvRate and cycle = %d %d\n",ipcInfo[ii].rcvRate,ipcInfo[ii].rcvCycle);
        if(ipcInfo[ii].netType)
        {
                pIpcData[ii]  = (CDS_IPC_COMMS *)(cdsPciModules.pci_rfm[0] + 0x12200d8+ 0x400 * ipcInfo[ii].ipcNum);
                printf("Net Type = RFM at 0x%x\n",(int)pIpcData[ii]);
        } else {
                pIpcData[ii] = (CDS_IPC_COMMS *)(_ipc_shm + 0x40000 + 0x400 * ipcInfo[ii].ipcNum);
                printf("Net Type = LOCAL IPC at 0x%x\n",(int)pIpcData[ii]);
        }
        printf("IPC Number = %d\n",ipcInfo[ii].ipcNum);
        printf("RCV Rate  = %d\n",ipcInfo[ii].rcvRate);
        printf("Send Computer Number  = %d\n",ipcInfo[ii].sendComputerNum);
		}
     }

    }
}

INLINE void commData2Send(int connects, CDS_IPC_INFO ipcInfo[], CDS_IPC_COMMS *pIpcData[], int timeSec, int cycle)
{
unsigned long syncWord;
int ipcIndex;
int dataCycle;
int ii;

for(ii=0;ii<connects;ii++)
{
        if(ipcInfo[ii].mode == IPC_SEND) // Zero = Rcv and One = Send
        {
                dataCycle = ((cycle + 1) * ipcInfo[ii].sendCycle) % 65536;
		if(dataCycle == 0) timeSec ++;
		syncWord = timeSec;
		syncWord = (syncWord << 32) + dataCycle;
		ipcIndex = (cycle+1) % 64;
                pIpcData[ii]->data[ipcIndex] = ipcInfo[ii].data;
                pIpcData[ii]->timestamp[ipcIndex] = syncWord;
        }
}

}

INLINE void commData2Receive(int connects, CDS_IPC_INFO ipcInfo[], CDS_IPC_COMMS *pIpcData[], int timeSec, int cycle)
{
unsigned long syncWord;
unsigned long mySyncWord;
int ipcIndex;
unsigned long rcvTime;
int cycle65k;
int ii;

for(ii=0;ii<connects;ii++)
{
        if(ipcInfo[ii].mode == IPC_RCV) // Zero = Rcv and One = Send
        {
	   if(!(cycle % ipcInfo[ii].rcvCycle)) // Time to rcv
	   {
		if(ipcInfo[ii].rcvCycle > 1)
		{
			ipcIndex = (cycle / ipcInfo[ii].rcvCycle) % 64;
			syncWord = pIpcData[ii]->timestamp[ipcIndex];
			cycle65k = cycle;
			mySyncWord = timeSec;
			mySyncWord = (mySyncWord << 32) + cycle65k;
			if(syncWord == mySyncWord) 
			{
				ipcInfo[ii].data = pIpcData[ii]->data[ipcIndex];
				// ipcInfo[ii].errFlag = 0;
			} else {
				ipcInfo[ii].errTotal ++;
				ipcInfo[ii].data = 0;
			}
		}else {
			ipcIndex = (cycle * ipcInfo[ii].rcvRate) % 64;
			syncWord = pIpcData[ii]->timestamp[ipcIndex];
			cycle65k = ((cycle * ipcInfo[ii].sendCycle));
			mySyncWord = timeSec;
			mySyncWord = (mySyncWord << 32) + cycle65k;
			if(syncWord == mySyncWord) 
			{
				ipcInfo[ii].data = pIpcData[ii]->data[ipcIndex];
				// ipcInfo[ii].errFlag = 0;
			} else {
				// ipcInfo[ii].errTotal ++;
				ipcInfo[ii].errTotal ++;
// printf("Rcv error %ld %ld\n",(syncWord & 0xffffffff),(mySyncWord & 0xffffffff));
				// ipcInfo[ii].data = 0;
			}
		}
                if(ipcInfo[ii].errFlag > 20000000) ipcInfo[ii].errFlag = 0;
	   }
	   if(cycle == 0)
	   {
			ipcInfo[ii].errFlag = ipcInfo[ii].errTotal;
				ipcInfo[ii].errTotal = 0;
	   }
        }
}
}
