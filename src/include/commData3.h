/*----------------------------------------------------------------------
   File: commData3.h
----------------------------------------------------------------------*/

#ifndef __COMMDATA3_H__
#define __COMMDATA3_H__

///	\file commData3.h
///	\brief Header file with IPC communications structures
///

///	Defines max number of IPC allowed per network type
#define MAX_IPC		1024
///	Defines max number of IPC allowed on a RFM network if using DMA 
#define MAX_IPC_RFM	1024
/// The number of data blocks buffered per IPC channel
#define IPC_BLOCKS 	64
#define PCIE_SEG_SIZE	16


///	Struct for a single IPC xmission
typedef struct CDS_IPC_XMIT {
///	Signal value being xmitted
        double data;
///	Combination of GDS seconds and cycle count
        unsigned long timestamp;
} CDS_IPC_XMIT;


///	Defines the array buffer in memory for IPC comms
typedef struct CDS_IPC_COMMS {
	CDS_IPC_XMIT dBlock[IPC_BLOCKS][MAX_IPC];
} CDS_IPC_COMMS;


///	Structure to maintain all IPC information
typedef struct CDS_IPC_INFO {
///	Data value to be sent or being received
        double data;
///	Not Used
        int sendNode;
///	Communication mechanism (as defined ISHME, IRFM0, IRFM1,IPCIE)
        int netType;
///	Cycle count to be sent as part of the timestamp data
        int sendCycle;
///	Rate at which data is being sent
        int sendRate;
///	Rate at which data is to be received
        int rcvRate;
///	Cycle on which to receive data
        int rcvCycle;
/// IPC number given from the IPC configuration file
        int ipcNum;
///	Should code send or receive this IPC
        int mode;
///	Errors/sec detected for a single IPC
	int errFlag;
        int errFlagS[2];
///	Marks error to IPC status by network type
	int errTotal;
///	Name of the IPC signal from the user model
        char *name;
///	Name of the model which contains the IPC sender part
        char *senderModelName;
	unsigned long lastSyncWord[2];
///	Pointer to the IPC data memory location
	CDS_IPC_COMMS *pIpcDataRead[2];
	CDS_IPC_COMMS *pIpcDataWrite[2];
///	Pointer to the IPC data memory location
} CDS_IPC_INFO;

typedef struct CDS_IPC_KEY_LIST {
	char name[32];
	unsigned int masterKey;
} CDS_IPC_KEY_LIST;

typedef struct CDS_DOLPHIN_INFO {
        volatile unsigned long *dolphinRead[4]; /* read and write Dolphin memory */
        volatile unsigned long *dolphinWrite[4]; /* read and write Dolphin memory */
        int dolphinCount;
} CDS_DOLPHIN_INFO;

/// Indicates data is to be sent
#define ISND		1	
/// Indicates data is to be received
#define IRCV		0
#define ISHME		0
#define IPCIE		1
#define IRFM0		2
#define IRFM1		3
#define IPC_BUFFER_SIZE		sizeof(struct CDS_IPC_COMMS)
#define IPC_BASE_OFFSET		0x80000
#define IPC_PCIE_BASE_OFFSET	0x100
#define IPC_TOTAL_ALLOC_SIZE	(IPC_PCIE_BASE_OFFSET + (3 * sizeof(CDS_IPC_COMMS)))
#define RFM0_OFFSET		((1 * sizeof(CDS_IPC_COMMS)))
#define RFM1_OFFSET		((2 * sizeof(CDS_IPC_COMMS)))
#define IPC_PCIE_READ	2
#define IPC_PCIE_WRITE	3
#define IPC_MAX_RATE		65536
#define IPC_RFM_BLOCK_SIZE	(sizeof(struct CDS_IPC_XMIT) * MAX_IPC)
#define IPC_RFM_XFER_SIZE	(sizeof(struct CDS_IPC_XMIT) * MAX_IPC_RFM)

// decide between inline or not for commData functions
#ifdef COMMDATA_INLINE
#  include "../fe/commData3.c"
#else
  // initialize the CommDataState struct
  // send data
  //   the cycle counter is included in the checksum,
  //   and is used to index the ring buffer
#endif

//void commData3Init(int connects, int rate, CDS_IPC_INFO ipcInfo[], long rfmAddress[]);
//void commData3Send(int connects, CDS_IPC_INFO ipcInfo[], int timeSec, int cycle);
//void commData3Receive(int connects, CDS_IPC_INFO ipcInfo[], int timeSec, int cycle);

#endif // __COMMDATA3_H__
