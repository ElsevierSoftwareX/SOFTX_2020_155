/*----------------------------------------------------------------------
   File: commData2.h
----------------------------------------------------------------------*/

#ifndef __COMMDATA_H__
#define __COMMDATA_H__

typedef struct CDS_IPC_COMMS {
        double data[64];
        unsigned long timestamp[64];
} CDS_IPC_COMMS;
typedef struct CDS_IPC_INFO {
        double data;
        int sendNode;
        int netType;
        int sendCycle;
        int sendRate;
        int rcvRate;
        int rcvCycle;
        int ipcNum;
        int mode;
        int errFlag;
	int errTotal;
	CDS_IPC_COMMS *pIpcData;
} CDS_IPC_INFO;
typedef struct CDS_IPC_KEY_LIST {
	char name[32];
	unsigned int masterKey;
} CDS_IPC_KEY_LIST;

#define IPC_SEND	1	
#define IPC_RCV		0
#define IPC_SHMEM	0
#define IPC_RFM		1
#define IPC_PCIE	2

// decide between inline or not for commData functions
#ifdef COMMDATA_INLINE
#  include "../fe/commData2.c"
#else
  // initialize the CommDataState struct
  // send data
  //   the cycle counter is included in the checksum,
  //   and is used to index the ring buffer
#endif
void commData2Init(int connects, int rate, CDS_IPC_INFO ipcInfo[], long rfmAddress);
void commData2Send(int connects, CDS_IPC_INFO ipcInfo[], int timeSec, int cycle);
void commData2Receive(int connects, CDS_IPC_INFO ipcInfo[], int timeSec, int cycle);

#endif // __COMMDATA_H__
