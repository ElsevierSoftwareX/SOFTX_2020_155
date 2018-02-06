#ifndef ZMQ_DAQ_CORE_H
#define ZMQ_DAQ_CORE_H

/*
 * This is split out from zmq_daq.h as zmq_daq.h contains its own
 * channel_t definition which conflicts with the daqd channel_t
 * structures.
 *
 * So move the core parts needed for data transmission out to allow
 * a data concentrator to be built which does not care about the
 * zmq_daqd.h channel_t types.
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


// Structure for combination DAQ and TP data
typedef struct daq_data_t_v1 {
    unsigned int dcuId;		// Unique DAQ unit id
    unsigned int fileCrc;		// Configuration file checksum
    unsigned int status;		// FE controller status
    unsigned int cycle;		// DAQ cycle count (0-15)
    unsigned int timeSec;		// GPS seconds
    unsigned int timeNSec;	// GPS nanoseconds
    unsigned int dataCrc;		// Data CRC checksum
    unsigned int dataBlockSize;	// Size of data block this message
    unsigned int tpCount;		// Number of TP chans this data set
    unsigned int tpNum[DAQ_GDS_MAX_TP_NUM];	// GDS TP TABLE
    char daq_data_block[DAQ_DCU_BLOCK_SIZE];	// DAQ data
} daq_data_t_v1;
//
#define DAQ_ZMQ_MAX_DC_BYTE_SEC		0x6000000  	// 100MB per sec
#define DAQ_ZMQ_MAX_FE_BYTE_SEC		0x1000000  	// 100MB per sec
#define DAQ_ZMQ_DC_DATA_BLOCK_SIZE   	(DAQ_ZMQ_MAX_DC_BYTE_SEC/DAQ_NUM_DATA_BLOCKS)
#define DAQ_ZMQ_FE_DATA_BLOCK_SIZE   	(DAQ_ZMQ_MAX_FE_BYTE_SEC/DAQ_NUM_DATA_BLOCKS)
#define DAQ_DATA_PORT		5555
#define DAQ_GDS_DATA_PORT	5556
#define DAQ_PROXY_PORT		5557
#define DAQ_ZMQ_MAX_DCU		128


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
    unsigned int dataBlockSize;	// Size of data block this message
} daq_msg_header_t;

// GDS TP message header structure
typedef struct gds_msg_header_t {
    unsigned int dcuId;		// Unique DAQ unit id
    unsigned int status;		// FE controller status
    unsigned int cycle;		// DAQ cycle count (0-15)
    unsigned int timeSec;		// GPS seconds
    unsigned int timeNSec;	// GPS nanoseconds
    unsigned int dataBlockSize;	// Size of data block this message
    unsigned int tpCount;		// Number of TP chans this data set
    unsigned int tpNum[DAQ_GDS_MAX_TP_NUM];	// GDS TP TABLE
} gds_msg_header_t;


// DAQ FE Data Transmission Structure
typedef struct daq_multi_dcu_data_t {
    int dcuTotalModels;
    daq_msg_header_t zmqheader[DAQ_ZMQ_MAX_DCU];
    char zmqDataBlock[DAQ_ZMQ_FE_DATA_BLOCK_SIZE];
}daq_multi_dcu_data_t;

// DAQ DC Data Transmission Structure
typedef struct daq_dc_data_t {
    int dcuTotalModels;
    // int dataBlockSize;   // <-- do we need a marker of the max actually used?
    daq_msg_header_t zmqheader[DAQ_ZMQ_MAX_DCU];
    char zmqDataBlock[DAQ_ZMQ_DC_DATA_BLOCK_SIZE];
}daq_dc_data_t;

// GDS TP Data Transmission Structure
typedef struct gds_multi_dcu_data_t {
    int dcuTotalModels;
    gds_msg_header_t zmqheader[DAQ_ZMQ_MAX_DCU];
    char zmqDataBlock[DAQ_ZMQ_FE_DATA_BLOCK_SIZE];
}gds_multi_dcu_data_t;

// Data structure to support multiple cylces of multiple dcus
typedef struct daq_multi_cycle_data_t {
    unsigned int curCycle;
    unsigned int maxCycle;
    unsigned int maxDcuCount;
    unsigned int cycleDataSize;
    char dataBlock[DAQ_ZMQ_DC_DATA_BLOCK_SIZE]; // daq_dc_data_t/daq_multi_dcu...
}daq_multi_cycle_data_t;

#define DAQ_ZMQ_HEADER_SIZE	(sizeof(daq_msg_header_t) * DAQ_ZMQ_MAX_DCU + sizeof(int))

#endif /* ZMQ_DAQ_CORE_H */