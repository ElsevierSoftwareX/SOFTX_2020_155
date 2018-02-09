#ifndef DAQ_GENERAL_TRANSIT_CORE_H
#define DAQ_GENERAL_TRANSIT_CORE_H

/*
 * This file defines the core structures and definitions needed for the
 * general network agnostic movement of data between the FE and the daqd.
 */

#define DAQ_NUM_DATA_BLOCKS     16
#define DAQ_NUM_DATA_BLOCKS_PER_SECOND  16
#define CDS_DAQ_NET_IPC_OFFSET 0x0
#define CDS_DAQ_NET_GDS_TP_TABLE_OFFSET 0x1000
#define CDS_DAQ_NET_DATA_OFFSET 0x2000
#define DAQ_DCU_SIZE            0x400000
#define DAQ_DCU_BLOCK_SIZE      (DAQ_DCU_SIZE/DAQ_NUM_DATA_BLOCKS)
#define DAQ_GDS_MAX_TP_NUM           0x100
#define MMAP_SIZE 1024*1024*64-5000

//
#define DAQ_TRANSIT_MAX_DC_BYTE_SEC		0x6000000  	// 100MB per sec
#define DAQ_TRANSIT_MAX_FE_BYTE_SEC		0x1000000  	// 100MB per sec
#define DAQ_TRANSIT_DC_DATA_BLOCK_SIZE   	(DAQ_TRANSIT_MAX_DC_BYTE_SEC/DAQ_NUM_DATA_BLOCKS)
#define DAQ_TRANSIT_FE_DATA_BLOCK_SIZE   	(DAQ_TRANSIT_MAX_FE_BYTE_SEC/DAQ_NUM_DATA_BLOCKS)
#define DAQ_DATA_PORT		5555
#define DAQ_GDS_DATA_PORT	5556
#define DAQ_PROXY_PORT		5557
#define DAQ_TRANSIT_MAX_DCU		128


//
//
// DAQ data message header structure
typedef struct daq_msg_header_t {
    unsigned int dcuId;		// Unique DAQ unit id
    unsigned int fileCrc;		// Configuration file checksum
    unsigned int status;		// FE controller status
    unsigned int cycle;		// DAQ cycle count (0-15)
    unsigned int timeSec;		// GPS seconds
    unsigned int timeNSec;	// GPS nanoseconds
    unsigned int dataCrc;		// Data CRC checksum
    unsigned int dataBlockSize;	// Size of data block for this message
    unsigned int tpBlockSize;   // Size of the tp block for this message
    unsigned int tpCount;		// Number of TP chans in this data set
    unsigned int tpNum[DAQ_GDS_MAX_TP_NUM];	// GDS TP TABLE
} daq_msg_header_t;

typedef struct daq_multi_dcu_header_t {
    int dcuTotalModels;                                 // Number of models
    int dataBlockSize;                                  // Number of bytes actually used in the data block
    daq_msg_header_t dcuheader[DAQ_TRANSIT_MAX_DCU];
} daq_multi_dcu_header_t;

// DAQ FE Data Transmission Structure
typedef struct daq_multi_dcu_data_t {
    daq_multi_dcu_header_t header;
    char dataBlock[DAQ_TRANSIT_FE_DATA_BLOCK_SIZE];
}daq_multi_dcu_data_t;

// DAQ DC Data Transmission Structure
typedef struct daq_dc_data_t {
    daq_multi_dcu_header_t header;
    char dataBlock[DAQ_TRANSIT_DC_DATA_BLOCK_SIZE];
}daq_dc_data_t;

typedef struct daq_multi_cycle_header_t {
    unsigned int curCycle;                  // current cycle
    unsigned int maxCycle;                  // max cycle
    unsigned int cycleDataSize;             // stride in bytes of the data
                                            // max data size is assumed to be
                                            // at least maxCycle * cycleDataSize
} daq_multi_cycle_header_t;

// Data structure to support multiple cycles of multiple dcus
typedef struct daq_multi_cycle_data_t {
    daq_multi_cycle_header_t header;
    char dataBlock[DAQ_TRANSIT_DC_DATA_BLOCK_SIZE*DAQ_NUM_DATA_BLOCKS_PER_SECOND];
}daq_multi_cycle_data_t;

#endif /* DAQ_GENERAL_TRANSIT_CORE_H */