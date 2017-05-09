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
#define DAQ_ZMQ_MODELS_PER_FE	6
#define DAQ_ZMQ_DCU_SIZE            0x1000000
#define DAQ_ZMQ_BLOCK_SIZE      (DAQ_ZMQ_DCU_SIZE/DAQ_NUM_DATA_BLOCKS)
#define DAQ_DATA_PORT		5555
#define DAQ_GDS_DATA_PORT	5556
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

// DAQ Data Transmission Structure
typedef struct daq_multi_dcu_data_t {
  int dcuTotalModels;
  daq_msg_header_t zmqheader[DAQ_ZMQ_MODELS_PER_FE];
  char zmqDataBlock[DAQ_ZMQ_BLOCK_SIZE];
}daq_multi_dcu_data_t;

// GDS TP Data Transmission Structure
typedef struct gds_multi_dcu_data_t {
  int dcuTotalModels;
  gds_msg_header_t zmqheader[DAQ_ZMQ_MODELS_PER_FE];
  char zmqDataBlock[DAQ_ZMQ_BLOCK_SIZE];
}gds_multi_dcu_data_t;


// DAQ Data Concentrator Transmission Structure
typedef struct daq_dc_data_t {
  int dcuTotalModels;
  daq_msg_header_t zmqheader[128];
  char zmqDataBlock[DAQ_ZMQ_DCU_SIZE];
}daq_dc_data_t;
