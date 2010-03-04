/*----------------------------------------------------------------------
   File: commData.h
   Date: October 2009
   Author: Matthew Evans & Tobin Fricke
 
   These functions are used to communicated between real-time front-end
   machines.  A cycle counter, maintained externally, is used to move
   between 2 entries in a double buffer.  A checksum based on the 
   cycle counter and on the data is included to verify that the data
   block is intact and is from the correct cycle.

   No attempt is made to deal with endianness issues, nor to correct
   any errors which are detected.
----------------------------------------------------------------------*/

#ifndef __COMMDATA_H__
#define __COMMDATA_H__

typedef struct CDS_IPC_COMMS {
        double data[64];
        unsigned long timestamp[64];
} CDS_IPC_COMMS;
typedef struct CDS_IPC_INFO {
        double data;
        int sendComputerNum;
        int netType;
        int sendCycle;
        int sendRate;
        int rcvRate;
        int rcvCycle;
        int ipcNum;
        int mode;
        int errFlag;
	int errTotal;
} CDS_IPC_INFO;
typedef struct CDS_IPC_KEY_LIST {
	char name[32];
	unsigned int masterKey;
} CDS_IPC_KEY_LIST;

#define IPC_SEND	1	
#define IPC_RCV		0
#define IPC_LOCAL	0
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
void commData2Init(int connects, CDS_IPC_KEY_LIST keylist[], int rate, CDS_IPC_INFO ipcInfo[], CDS_IPC_COMMS *pIpcData[],long rfmAddress);
void commData2Send(int connects, CDS_IPC_INFO ipcInfo[], CDS_IPC_COMMS *pIpcData[], int timeSec, int cycle);
void commData2Receive(int connects, CDS_IPC_INFO ipcInfo[], CDS_IPC_COMMS *pIpcData[], int timeSec, int cycle);

#endif // __COMMDATA_H__
