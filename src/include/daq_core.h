#ifndef DAQ_GENERAL_TRANSIT_CORE_H
#define DAQ_GENERAL_TRANSIT_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "daq_core_defs.h"

/*
 * This file defines the core structures and definitions needed for the
 * general network agnostic movement of data between the FE and the daqd.
 */

#define CDS_DAQ_NET_IPC_OFFSET 0x0
#define CDS_DAQ_NET_GDS_TP_TABLE_OFFSET 0x1000
#define CDS_DAQ_NET_DATA_OFFSET 0x2000

#define DAQ_GDS_MAX_TP_NUM 0x100
#define MMAP_SIZE 1024 * 1024 * 64 - 5000

//
#define DAQ_TRANSIT_MAX_DC_BYTE_SEC 0x6000000 // 100MB per sec
#define DAQ_TRANSIT_MAX_FE_BYTE_SEC 0x1000000 // 100MB per sec
#define DAQ_TRANSIT_DC_DATA_BLOCK_SIZE                                         \
    ( DAQ_TRANSIT_MAX_DC_BYTE_SEC / DAQ_NUM_DATA_BLOCKS )
#define DAQ_TRANSIT_FE_DATA_BLOCK_SIZE                                         \
    ( DAQ_TRANSIT_MAX_FE_BYTE_SEC / DAQ_NUM_DATA_BLOCKS )
#define DAQ_DATA_PORT 5555
#define DAQ_GDS_DATA_PORT 5556
#define DAQ_PROXY_PORT 5557
#define DAQ_TRANSIT_MAX_DCU 128

//
//
// DAQ data message header structure
typedef struct daq_msg_header_t
{
    unsigned int dcuId; // Unique DAQ unit id
    unsigned int fileCrc; // Configuration file checksum
    unsigned int status; // FE controller status
    unsigned int cycle; // DAQ cycle count (0-15)
    unsigned int timeSec; // GPS seconds
    unsigned int timeNSec; // GPS nanoseconds
    unsigned int dataCrc; // Data CRC checksum
    unsigned int dataBlockSize; // Size of data block for this message (regular
                                // data only, does not include TP data)
    unsigned int tpBlockSize; // Size of the tp block for this message
    unsigned int tpCount; // Number of TP chans in this data set
    unsigned int
        tpNum[ DAQ_GDS_MAX_TP_NUM ]; // GDS TP TABLE.  MUST be the last field!
} daq_msg_header_t;

typedef struct daq_multi_dcu_header_t
{
    int dcuTotalModels; // Number of models
    int fullDataBlockSize; // Number of bytes used in the data block (including
                           // TP data)
    daq_msg_header_t dcuheader[ DAQ_TRANSIT_MAX_DCU ];
} daq_multi_dcu_header_t;

// DAQ FE Data Transmission Structure
typedef struct daq_multi_dcu_data_t
{
    daq_multi_dcu_header_t header;
    char                   dataBlock[ DAQ_TRANSIT_FE_DATA_BLOCK_SIZE ];
} daq_multi_dcu_data_t;

// DAQ DC Data Transmission Structure
typedef struct daq_dc_data_t
{
    daq_multi_dcu_header_t header;
    char                   dataBlock[ DAQ_TRANSIT_DC_DATA_BLOCK_SIZE ];
} daq_dc_data_t;

typedef struct daq_multi_cycle_header_t
{
    unsigned int curCycle; // current cycle
    unsigned int maxCycle; // max cycle
    unsigned int cycleDataSize; // stride in bytes of the data
    // max data size is assumed to be
    // at least maxCycle * cycleDataSize
    int msgcrc; // Data CRC checksum for DC -> FB/NDS
} daq_multi_cycle_header_t;

// Data structure to support multiple cycles of multiple dcus
typedef struct daq_multi_cycle_data_t
{
    daq_multi_cycle_header_t header;
    char                     dataBlock[ DAQ_TRANSIT_DC_DATA_BLOCK_SIZE *
                    DAQ_NUM_DATA_BLOCKS_PER_SECOND ];
} daq_multi_cycle_data_t;

#ifdef __cplusplus
}
#endif

#endif /* DAQ_GENERAL_TRANSIT_CORE_H */
