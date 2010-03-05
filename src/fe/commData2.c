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
INLINE void commData2Init(int connects, int rate, CDS_IPC_INFO ipcInfo[], long rfmAddress)
{
int ipcTemp;
int ii;
int localKey;

// MASTER KEYS **************************************************************************
// Presently hardcoded.
// Should be moved to a header file or sent as EPICS channels

// IPC 0: 
// [G1:SUS-Q1_M0_FACE1_IPC]
// ipcType=SHMEM
// ipcRate=16384
// ipcNum=0;
// ipcSender=0

// IPC 1:
// [G1:OM1-OMC_READOUT_IPC]
// ipcType=RFM
// ipcRate=32768
// ipcNum=0
// ipcSender=0

// IPC 2:
// [G1:DBB-INPUT_1_IPC]
// ipcType=SHMEM
// ipcRate=65536
// ipcNum=1
// ipcSender=0

// ***************************************************************************************

  for(ii=0;ii<connects;ii++)
  {
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
	// Clear the data point
        ipcInfo[ii].data = 0.0;
        printf("IPC DATA for IPC %d ******************\n",ii);
        if(ipcInfo[ii].mode) printf("Mode = SENDER w Cycle = %d\n",ipcInfo[ii].sendCycle);
        else printf("Mode = RECEIVER w rcvRate and cycle = %d %d\n",ipcInfo[ii].rcvRate,ipcInfo[ii].rcvCycle);
        if(ipcInfo[ii].netType)
        {
                ipcInfo[ii].pIpcData  = (CDS_IPC_COMMS *)(cdsPciModules.pci_rfm[0] + 0x12200d8+ 0x400 * ipcInfo[ii].ipcNum);
                printf("Net Type = RFM at 0x%x\n",(int)ipcInfo[ii].pIpcData);
        } else {
                ipcInfo[ii].pIpcData = (CDS_IPC_COMMS *)(_ipc_shm + 0x40000 + 0x400 * ipcInfo[ii].ipcNum);
                printf("Net Type = LOCAL IPC at 0x%x\n",(int)ipcInfo[ii].pIpcData);
        }
        printf("IPC Number = %d\n",ipcInfo[ii].ipcNum);
        printf("RCV Rate  = %d\n",ipcInfo[ii].rcvRate);
        printf("Send Computer Number  = %d\n",ipcInfo[ii].sendNode);

    }
}

INLINE void commData2Send(int connects, CDS_IPC_INFO ipcInfo[], int timeSec, int cycle)
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
                ipcInfo[ii].pIpcData->data[ipcIndex] = ipcInfo[ii].data;
                ipcInfo[ii].pIpcData->timestamp[ipcIndex] = syncWord;
        }
}

}

INLINE void commData2Receive(int connects, CDS_IPC_INFO ipcInfo[], int timeSec, int cycle)
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
			syncWord = ipcInfo[ii].pIpcData->timestamp[ipcIndex];
			cycle65k = cycle;
			mySyncWord = timeSec;
			mySyncWord = (mySyncWord << 32) + cycle65k;
			if(syncWord == mySyncWord) 
			{
				ipcInfo[ii].data = ipcInfo[ii].pIpcData->data[ipcIndex];
				// ipcInfo[ii].errFlag = 0;
			} else {
				ipcInfo[ii].errTotal ++;
				ipcInfo[ii].data = 0;
			}
		}else {
			ipcIndex = (cycle * ipcInfo[ii].rcvRate) % 64;
			syncWord = ipcInfo[ii].pIpcData->timestamp[ipcIndex];
			cycle65k = ((cycle * ipcInfo[ii].sendCycle));
			mySyncWord = timeSec;
			mySyncWord = (mySyncWord << 32) + cycle65k;
			if(syncWord == mySyncWord) 
			{
				ipcInfo[ii].data = ipcInfo[ii].pIpcData->data[ipcIndex];
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
