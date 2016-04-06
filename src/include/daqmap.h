///	@file daqmap.h
///	@brief File contains defs and structures used in DAQ and GDS/TP routines. \n
///<		NOTE: This file used by daqd, as well as real-time and EPICS code.

#ifndef MAP_5565_H
#define MAP_5565_H

#define DAQ_16K_SAMPLE_SIZE     1024    ///< Num values for 16K system in 1/16 second   
#define DAQ_2K_SAMPLE_SIZE      128     ///< Num values for 2K system in 1/16 second 
#define DAQ_CONNECT             0	///< Initialize DAQ flag
#define DAQ_WRITE               1	///< Runtime DAQ flag ie acquiring data.

#define DAQ_SRC_FM_TP		0	///< Data from filter module testpoint
#define DAQ_SRC_NFM_TP		1	///< Data from non filter module related testpoint
#define DAQ_SRC_FM_EXC		2	///< Data from filter module excitation input
#define DAQ_SRC_NFM_EXC		3	///< Data from non filter module related excitation input
#define DAQ_NUM_FM_TP		3	///< Number of TP avail from each filter module

#define DTAPS   3       		///< Num SOS in decimation filters.   

/*
 * DAQ system inter-processor communication definitions.
 */

#define DCU_COUNT 256		///< MAX number of real-time DAQ processes in single control system
#define DAQ_BASE_ADDRESS	0x2000000			///< DAQ base offset from shared mem start
#define DAQ_DATA_BASE_ADD	(DAQ_BASE_ADDRESS + 0x100000)	///< DAQ data location in shared mem

/* Redefine this to change DAQ transmission size */
#define DAQ_DCU_SIZE		0x400000	///< MAX data in bytes/sec allowed per process
#define DAQ_EDCU_SIZE		0x400000	///< MAX epics data xfer size per process
#define DAQ_EDCU_BLOCK_SIZE	0x20000

#define DAQ_NUM_DATA_BLOCKS	16		///< Number of DAQ data blocks
#define DAQ_NUM_SWING_BUFFERS	2		///< Number of DAQ read/write swing buffers
#define DAQ_NUM_DATA_BLOCKS_PER_SECOND	16	///< Number of DAQ data blocks to xfer each second

#define DAQ_DCU_BLOCK_SIZE	(DAQ_DCU_SIZE/DAQ_NUM_DATA_BLOCKS)	///< Size of one DAQ data block

#define DAQ_DCU_RATE_WARNING	3999	///< KByte to set warning DAQ rate is nearing max of 4MB/sec/model

/// Structure for maintaining DAQ channel information
typedef struct DAQ_LKUP_TABLE {
        int type;       ///< 0=SFM, 1=nonSFM TP, 2= SFM EXC, 3=nonSFM EXC        
	int sysNum;     ///< If multi-dim SFM array, which one to use.          
	int fmNum;      ///< Filter module with signal of interest.         
	int sigNum;     ///< Which signal within a filter.               
	int decFactor;  ///< Decimation factor to use.                
	int offset;     ///< Offset to beginning of next channel in local buff.
	int decn;	///< FIR filter decimation .
}DAQ_LKUP_TABLE;

/// Structure for maintaining TP channel number ranges which are valid for this front end. These typically come from gdsLib.h.
typedef struct DAQ_RANGE {
        int filtTpMin;		///< Value of minimum allowed TP number for filter modules.
        int filtTpMax;		///< Value of maximum allowed TP number for filter modules.
        int filtTpSize;		///< Total number of TP for filter modules.
        int xTpMin;		///< Value of minimum allowed TP number for non-filter TP
        int xTpMax;		///< Value of maximum allowed TP number for non-filter TP
        int filtExMin;		///< Value of mimimum allowed EXC number for filter modules.
        int filtExMax;		///< Value of maximum allowed EXC number for filter modules.
        int filtExSize;		///< Totol number of EXC allowed for filter modules.
        int xExMin;		///< Value of minimum allowed EXC number for non-filter EXC
        int xExMax;		///< Value of maximum allowed EXC number for non-filter EXC
} DAQ_RANGE;


/*
 * Inter-processor communication structures
 */

/// Stucture to provide timing and crc info DAQ network driver via shared memory.
typedef struct blockProp {
  unsigned int status;
  unsigned int timeSec;		///< DAQ data timestamp seconds
  unsigned int timeNSec;	///< DAQ data timestamp nanoseconds
  unsigned int run;
  unsigned int cycle;
  unsigned int crc; 	///< block data CRC checksum 
} blockPropT;

/// Structure for passing data info to DAQ network writer via shared memory
struct rmIpcStr {    
  unsigned int cycle;  ///< Copy of latest cycle num from blocks 
  unsigned int dcuId;          ///< id of unit, unique within each site  
  unsigned int crc;	       ///< Configuration file's checksum       
  unsigned int command;        ///< Allows DAQSC to command unit.      
  unsigned int cmdAck;         ///< Allows unit to acknowledge DAQS cmd. 
  unsigned int request;        ///< DCU request of controller           
  unsigned int reqAck;         ///< controller acknowledge of DCU req. 
  unsigned int status;         ///< Status is set by the controller.   
  unsigned int channelCount;   ///< Number of data channels in a DCU  
  unsigned int dataBlockSize; 	///< Num bytes actually written by DCU within a 1/16 data block
  blockPropT bp [DAQ_NUM_DATA_BLOCKS];  ///< An array of block property structures 
};

/*
 * DCU IDs
 */

#define IPC_BLOCK_SIZE          0x1000

#define DCU_ID_DAQSC		0
#define IPC_OFFSET_DAQSC	(DCU_ID_DAQSC * IPC_BLOCK_SIZE) /* Controller IPC area   */


#define DCU_ID_FB0	        1
#define DCU_ID_FB1	        2
#define DCU_ID_FB2	        3
#define DCU_ID_EDCU		4

#define DCU_ID_ADCU_1           5
#define DCU_ID_ADCU_2           6
#define DCU_ID_ADCU_3           7
#define DCU_ID_ADCU_4           8

#define DCU_ID_SUS_1            9 
#define DCU_ID_SUS_2            10
#define DCU_ID_SUS_3            11
#define DCU_ID_SUS_4            12

#define DCU_ID_EX_16K           13
#define DCU_ID_EX_2K            14
#define DCU_ID_TP_16K           15
#define DCU_ID_TP_2K            16

#define DCU_ID_LSC              17
#define DCU_ID_ASC              18

#define DCU_ID_SUS_SOS		19
#define DCU_ID_SUS_ETMX		20
#define DCU_ID_SUS_ETMY		21

#define DCU_ID_HEPI_1           22
#define DCU_ID_HEPI_2           23
#define DCU_ID_HEPI_EX          24
#define DCU_ID_HEPI_EY          25

#define DCU_ID_IOO              26

#define DCU_ID_FIRST_GDS        DCU_ID_EX_16K
#define DCU_ID_FIRST_ADCU	DCU_ID_ADCU_1
#define ADCU_TOTAL		4

#define IS_ANALOG_DCU(dcuid) ((dcuid) == DCU_ID_ADCU_1 || (dcuid) == DCU_ID_ADCU_2 || (dcuid) == DCU_ID_ADCU_3 || (dcuid) == DCU_ID_ADCU_4)
#define IS_EPICS_DCU(dcuid) ((dcuid) == DCU_ID_EDCU)
#define IS_HEPI_DCU(dcuid) ((dcuid) == DCU_ID_SUS_ETMY || (dcuid) == DCU_ID_HEPI_1 || (dcuid) == DCU_ID_HEPI_2 || (dcuid) == DCU_ID_HEPI_EX || (dcuid) == DCU_ID_HEPI_EY)
#define IS_2K_DCU(dcuid)      IS_HEPI_DCU(dcuid)
#define IS_32K_DCU(dcuid)     (daqd.cit_40m? dcuid == 11: dcuid == DCU_ID_SUS_2)
#define IS_EXC_DCU(dcuid) ((dcuid) == DCU_ID_EX_16K || (dcuid) == DCU_ID_EX_2K)

/* DCU 11 and 22 are the 40m Myrinet DCUs */
#define IS_MYRINET_DCU(dcuid) (daqd.cit_40m? ((dcuid) == 11 || (dcuid) == DCU_ID_HEPI_1)  : (((dcuid) >= 5 && (dcuid) <= 12) || ((dcuid) >= 17 && (dcuid) <= (DCU_COUNT-1))) )

#define IPC_OFFSET_DCU(dcuid)   ((dcuid) * IPC_BLOCK_SIZE + DAQ_BASE_ADDRESS)
#define DATA_OFFSET_DCU(dcuid)  DAQ_DATA_BASE_ADD

#define IS_TP_DCU(dcuid) ((dcuid) == DCU_ID_EX_16K || (dcuid) == DCU_ID_TP_16K || (dcuid) == DCU_ID_EX_2K || (dcuid) == DCU_ID_TP_2K)

/* dataType Defintions */
#define DAQ_DATATYPE_16BIT_INT  1       ///< Data type signed 16bit integer 
#define DAQ_DATATYPE_32BIT_INT  2       ///< Data type signed 32bit integer 
#define DAQ_DATATYPE_64BIT_INT  3       ///< Data type signed 64bit integer 
#define DAQ_DATATYPE_FLOAT      4       ///< Data type 32bit floating point 
#define DAQ_DATATYPE_DOUBLE     5       ///< Data type 64bit double float 
#define DAQ_DATATYPE_COMPLEX    6       ///< Complex data; two 32bit floats
#define DAQ_DATATYPE_32BIT_UINT  7       ///< Data type unsigned signed 32bit integer 

static const int daqSizeByType[8] = {0,2,4,8,4,8,16,4};
#define DAQ_DATA_TYPE_SIZE(a) (daqSizeByType[(a)])


/*
 * Compatibility definitions
 */

#define DAQ_OK              0
#define DATA_BLOCKS         DAQ_NUM_DATA_BLOCKS

#define GDS_2k_LSC_EX_ID    DCU_ID_EX_16K
#define GDS_2k_ASC_EX_ID    DCU_ID_EX_2K
#define GDS_2k_LSC_TP_ID    DCU_ID_TP_16K
#define GDS_2k_ASC_TP_ID    DCU_ID_TP_2K

#define GDS_4k_LSC_EX_ID    DCU_ID_EX_16K
#define GDS_4k_ASC_EX_ID    DCU_ID_EX_2K
#define GDS_4k_LSC_TP_ID    DCU_ID_TP_16K
#define GDS_4k_ASC_TP_ID    DCU_ID_TP_2K


#define UNIT_ID_TO_RFM_OFFSET(a) IPC_OFFSET_DCU(a)
#define UNIT_ID_TO_RFM_SIZE(a) DAQ_DCU_SIZE
#define UNIT_ID_TO_RFM_NODE_ID(a) 0

#define DAQSC_ID        DCU_ID_DAQSC
#define DAQSC_IPC	IPC_OFFSET_DCU(DCU_ID_DAQSC)

#define FB0_ID          DCU_ID_FB0
#define FB0_IPC		IPC_OFFSET_DCU(DCU_ID_FB0)

#define DAQS_UTYPE_CS		1		/* Controller */
#define DAQS_UTYPE_CM           2               /* Configuration Manager */
#define DAQS_UTYPE_FB		3		/* Frame Builder */
#define DAQS_UTYPE_ADCU		4		/* Analog DCU */
#define DAQS_UTYPE_DDCU		5		/* ISC DCU */
#define DAQS_UTYPE_EDCU		6		/* Epics DCU */
#define DAQS_UTYPE_GDS_EXC	7		/* GDS Excitation system */
#define DAQS_UTYPE_GDS_TP	8		/* GDS Test Point System */
#define DAQS_UTYPE_GDS_FLAG	9		/* GDS Flag Channel System */

/* DAQ IPC Status bit definitions */
#define DAQ_STATE_NO_CONFIG	0x0	/* Config state */
#define DAQ_STATE_CONFIG	0x01	/* Config state */
#define DAQ_STATE_RUN		0x02	/* Running state */
#define DAQ_STATE_RUN_CONFIG	0x03	/* Running, loading new config  */
#define DAQ_STATE_READY		0x04	/* Ready state */
#define DAQ_STATE_RUN_READY	0x06	/* Running, new config ready */
#define DAQ_STATE_FAULT		0x08	/* Fault state */
#define DAQ_STATE_CAL 		0x10	/* Calibration state */
#define DAQ_STATE_SYNC_ERR	0x20

/* DAQ IPC Command bit definitions */
#define DAQS_CMD_NO_CMD         0x00
#define DAQS_CMD_CONFIGURE	0x01		/* Config/reconfig */
#define DAQS_CMD_START		0x02		/* Start acquisition */
#define DAQS_CMD_HALT           0x04
#define DAQS_CMD_CALIBRATE      0x08
#define DAQS_CMD_KILL           0x10
#define DAQS_CMD_RFM_REFRESH    0x20            /* Refresh DCU RFM area */
#define DAQS_CMD_NEW_GAINS	0x50            /* New gain set from operator */


/*
 * GDS communication data
 */

/// Gds control block starts at this offset 
#define DAQ_GDS_BLOCK_ADD	(DAQ_BASE_ADDRESS + 0x600000)

/// The maximum possibile size (allocated space) of the test point table 
#define DAQ_GDS_MAX_TP_NUM           0x100

/// We only allow this many TPs to be set 
#define DAQ_GDS_MAX_TP_ALLOWED	64

/// The total number of test point DCUs 
#define DAQ_GDS_DCU_NUM       4

// Have to be careful with the next two number definitions
// Size of DAQ_INFO_BLOCK is set by DCU_MAX_CHANNELS and size could
// cause overflow from base DAQ_INFO_ADDRESS into DAQ_BASE_ADDRESS
/// Total number of channels allowed per DCU 
#define DCU_MAX_CHANNELS	512

/// Offset to DAQ configuration info from base address of shared memory.
#define DAQ_INFO_ADDRESS	0x01ff0000

/// Cycle (16Hz Daq Clk) on which to transfer EPICS integer data.
#define DAQ_XFER_CYCLE_INT	0
/// Cycle (16Hz Daq Clk) on which to transfer EPICS double data.
#define DAQ_XFER_CYCLE_DBL	1
/// Cycle (16Hz Daq Clk) on which to start transfer of Filter Module EPICS data.
#define DAQ_XFER_CYCLE_FMD	2
/// Number of filter modules for which data is to be transferred per DAQ cycle.
#define DAQ_XFER_FMD_PER_CYCLE	100

/// Structure for DAQ setup information in shared memory from EPICS
typedef struct DAQ_INFO_BLOCK {
  int reconfig; 		///< Set to 1 by the Epics to indicate configuration change 
  int numChans; 		///< Defines how many channels are configured int struct tp[] 
  int numEpicsInts;		///< Number of EPICS integer values to acquire.
  int numEpicsFloats;		///< Number of EPICS floating values to acquire.
  int numEpicsFilts; 		///< Number of filter module EPICS channels to acquire.
  int numEpicsTotal; 		///< Total number of EPICS channels to acquire.
  int epicsdblDataOffset;	///< Offset from start of data buffer to start of float data.
  int cpyepics2times;		///< Set if EPICS integers need to be copied twice.
  int cpyIntSize[2];		///< Size of each memcpy of EPICS integer data.
  int numEpicsFiltXfers;	///< Number of cycles required to copy filter module data.
  int numEpicsFiltsLast;	///< Number of filter modules to copy data from on last xfer.
  unsigned long configFileCRC; 	///< DAQ config file checksum 
  struct {
    unsigned int tpnum; 	///< Test point number to which this DAQ channel connects 
    unsigned int dataType;	///< Type cast of DAQ data channel
    unsigned int dataRate;	///< Acquisition rate of DAQ channel
    float dataGain;		///< Gain to be applied to TP data.
  } tp[DCU_MAX_CHANNELS];
} DAQ_INFO_BLOCK;

typedef struct DAQ_XFER_INFO {
   int crcLength;		///< Number of bytes in 1/16 sec data block
   int xferSize;		///< Tracks remaining xfer size for crc
   int xferSize1;		///< Amount of data to transfer on each cycle
   int xferLength;
   int totalSize;		///< DAQ + TP + EXC chans size in bytes.
   int totalSizeNet;		///<  DAQ + TP + EXC chans size in bytes sent to network driver.
   int offsetAccum;
   int fileCrc;			///< CRC checksum of the DAQ configuration file.
} DAQ_XFER_INFO;

/*
 * The following four numbers are matched with the GDS rmorg.h header
 */

/* Defines the number of LSC excitation outputs */
#define DAQ_GDS_TP_LSC_EX_NUM		7

/* Defines the number of LSC test point outputs */
#define DAQ_GDS_TP_LSC_TP_NUM		15

/* Defines the number of ASC excitation engine test points */
#define DAQ_GDS_TP_ASC_EX_NUM		24

/* Defines the number of ASC test point outputs */
#define DAQ_GDS_TP_ASC_TP_NUM		56

#define DAQ_DBL_PER_CYCLE	50
#define DAQ_DBL_CYCLE_START	40

static const int daqGdsTpNum[4] = { DAQ_GDS_TP_LSC_EX_NUM, DAQ_GDS_TP_ASC_EX_NUM, DAQ_GDS_TP_LSC_TP_NUM, DAQ_GDS_TP_ASC_TP_NUM };

/* GDS layout:
   4K GDS GDS_CNTRL_BLOCK at DAQ_GDS_BLOCK_ADD
   2K GDS GDS_CNTRL_BLOCK at DAQ_GDS_BLOCK_ADD + sizeof(GDS_CNTRL_BLOCK)
   
   GDS_CNTRL_BLOCK.excNum16K[0] has the test point numbers
   GDS_CNTRL_BLOCK.excNum16K[1] has the channel status words
*/
/// Structure of GDS TP table in shared memory.
typedef union GDS_CNTRL_BLOCK {
  unsigned int tp [4][2][DAQ_GDS_MAX_TP_NUM];
  struct {
    unsigned int excNum16k[2][DAQ_GDS_MAX_TP_NUM];
    unsigned int excNum2k[2][DAQ_GDS_MAX_TP_NUM];
    unsigned int tpNum16k[2][DAQ_GDS_MAX_TP_NUM];
    unsigned int tpNum2k[2][DAQ_GDS_MAX_TP_NUM];
  } tpe;
} GDS_CNTRL_BLOCK;

/// GDS test point table structure for FE to frame builder communication
typedef struct cdsDaqNetGdsTpNum {
   int count; /* test points count */
   int tpNum[DAQ_GDS_MAX_TP_NUM];
} cdsDaqNetGdsTpNum;

#define GDS_TP_MAX_FE	1250
#define GDS_MAX_NFM_TP	500
#define GDS_MAX_NFM_EXC	50
#define GDS_2K_EXC_MIN	20001
#define GDS_16K_EXC_MIN	1
#define GDS_2K_TP_MIN	30001
#define GDS_16K_TP_MIN	10001

#endif

#define MAX_DEC                 128
#define DEC_FILT_LENGTH         21
