/*----------------------------------------------------------------------
   File: commData2.h
----------------------------------------------------------------------*/

#ifndef __COMMDATA2_H__
#define __COMMDATA2_H__

typedef struct CDS_IPC_XMIT {
        double data;
        unsigned long timestamp;
} CDS_IPC_XMIT;
typedef struct CDS_IPC_COMMS {
	CDS_IPC_XMIT dBlock[64][64];
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

#define ISND		1	
#define IRCV		0
#define ISHME		0
#define IPCIE		1
#define IRFM0		2
#define IRFM1		3
#define IPC_MAX_RFM		64
#define IPC_MAX			128
#define IPC_BUFFER_SIZE		sizeof(struct CDS_IPC_COMMS)
#define IPC_BASE_OFFSET		0x80000
#define IPC_PCIE_BASE_OFFSET		0x100
#define IPC_PCIE_READ	2
#define IPC_PCIE_WRITE	3
#define IPC_RFM_BLOCK_SIZE 	(IPC_MAX_RFM * IPC_BUFFER_SIZE)
#define IPC_DBLOCK_SIZE 		(IPC_MAX * IPC_BUFFER_SIZE)
#define IPC_MAX_RATE		65536
#define IPC_RFM_XFER_SIZE	0x400	// Set to 1k Byte ie 64 channels

// decide between inline or not for commData functions
#ifdef COMMDATA_INLINE
#  include "../fe/commData2.c"
#else
  // initialize the CommDataState struct
  // send data
  //   the cycle counter is included in the checksum,
  //   and is used to index the ring buffer
#endif

//void commData2Init(int connects, int rate, CDS_IPC_INFO ipcInfo[], long rfmAddress[]);
//void commData2Send(int connects, CDS_IPC_INFO ipcInfo[], int timeSec, int cycle);
//void commData2Receive(int connects, CDS_IPC_INFO ipcInfo[], int timeSec, int cycle);

#endif // __COMMDATA2_H__
