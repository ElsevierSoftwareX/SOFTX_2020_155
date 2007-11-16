#ifndef MAP_5565_H
#define MAP_5565_H

/*
 * DAQ system inter-processor communication definitions.
 */

#define DCU_COUNT 32
#define DAQ_BASE_ADDRESS	0x2000000
#define DAQ_DATA_BASE_ADD	(DAQ_BASE_ADDRESS + 0x100000)

/* Redefine this to change DAQ transmission size */
#if defined(COMPAT_INITIAL_LIGO)
#define DAQ_DCU_SIZE		0x100000
#else
#define DAQ_DCU_SIZE		0x200000
#endif

#define DAQ_EDCU_SIZE		0x400000
#define DAQ_EDCU_BLOCK_SIZE	0x20000

#define DAQ_NUM_DATA_BLOCKS	16
#define DAQ_NUM_DATA_BLOCKS_PER_SECOND	16

#define DAQ_DCU_BLOCK_SIZE	(DAQ_DCU_SIZE/DAQ_NUM_DATA_BLOCKS)

/*
 * Inter-processor communication structures
 */

typedef struct blockProp {
  unsigned int status;
#ifdef SHMEM_DAQ
  unsigned int timeSec;
  unsigned int timeNSec;
  unsigned int run;
  unsigned int cycle;
  unsigned int crc; /* block data CRC checksum */
#else
  unsigned long timeSec;
  unsigned long timeNSec;
  unsigned long run;
  unsigned long cycle;
  unsigned long crc; /* block data CRC checksum */
#endif
} blockPropT;

struct rmIpcStr {       /* IPC area structure                   */
#ifdef SHMEM_DAQ
  unsigned int cycle;  /* Copy of latest cycle num from blocks */
#else
  unsigned long cycle;  /* Copy of latest cycle num from blocks */
#endif
  unsigned int dcuId;          /* id of unit, unique within each site  */
  unsigned int crc;	       /* Configuration file's checksum        */
  unsigned int command;        /* Allows DAQSC to command unit.        */
  unsigned int cmdAck;         /* Allows unit to acknowledge DAQS cmd. */
  unsigned int request;        /* DCU request of controller            */
  unsigned int reqAck;         /* controller acknowledge of DCU req.   */
  unsigned int status;         /* Status is set by the controller.     */
  unsigned int channelCount;   /* Number of data channels in a DCU     */
  unsigned int dataBlockSize; /* Num bytes actually written by DCU within a 1/16 data block */
  blockPropT bp [DAQ_NUM_DATA_BLOCKS];  /* An array of block property structures */
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
#ifdef _ADVANCED_LIGO
#define IS_HEPI_DCU(dcuid) ((dcuid) == DCU_ID_SUS_ETMY || (dcuid) == DCU_ID_HEPI_1 || (dcuid) == DCU_ID_HEPI_2 || (dcuid) == DCU_ID_HEPI_EX || (dcuid) == DCU_ID_HEPI_EY)
#define IS_2K_DCU(dcuid)      IS_HEPI_DCU(dcuid)
#define IS_32K_DCU(dcuid)     (daqd.cit_40m? dcuid == 11: dcuid == DCU_ID_SUS_2)
#else
#define IS_HEPI_DCU(dcuid) ((dcuid) == DCU_ID_HEPI_1 || (dcuid) == DCU_ID_HEPI_2 || (dcuid) == DCU_ID_HEPI_EX || (dcuid) == DCU_ID_HEPI_EY)
#endif
#define IS_EXC_DCU(dcuid) ((dcuid) == DCU_ID_EX_16K || (dcuid) == DCU_ID_EX_2K)


/* DCU 11 and 22 are the 40m Myrinet DCUs */
#define IS_MYRINET_DCU(dcuid) (daqd.cit_40m? ((dcuid) == 11 || (dcuid) == DCU_ID_HEPI_1)  : (((dcuid) >= DCU_ID_SUS_1 && (dcuid) <= DCU_ID_SUS_4) || ((dcuid) >= DCU_ID_SUS_SOS && (dcuid) <= DCU_ID_HEPI_EY)) )

static const char * const dcuName[DCU_COUNT] = {"DAQSC",
					 "FB0", "FB1", "FB2",
					 "EDCU",
					 "ADCU_PEM", "ADCU_SUS",
					 "ADCU_EX", "ADCU_EY",
					 "SUS1", "SUS2", "SUS3", "SUS4",
					 "EX16K", "EX2K",
					 "TP16K", "TP2K",
					 "LSC", "ASC",
                                         "SOS", "SUS_EX",
#ifdef _ADVANCED_LIGO
"SEI",
#else
"SUS_EY",
#endif
					 "SEI1", "SEI2",
					 "SEI_EX", "SEI_EY", "IOO",
					 "FAST1", "FAST2", "FAST3",
					 "FAST4", "FAST5"};

/* Which network DCU is on (1 for 5565, 2 for 5579 or 3 for both; 0 - none) */
static unsigned int const dcuNet[DCU_COUNT] = {3,3,3,3,0,
			/* ADCUs */	2,2,2,2,
					1,1,1,1,
					3,3,
					3,3,
			/* LSC ASC */	1,1,
					1,2,2,
					2,2,
					2,2,1,
			/* FAST ADCU */	2,2,2,2,2};

/* How the things are layed out at the 40m lab */
static unsigned int const dcuNet40m[DCU_COUNT] = {2,2,2,2,0,
			/* ADCUs */	2,2,1,1,
					1,1,1,1,
					3,3,
					3,3,
			/* LSC ASC */	1,1,
					1,1,1,
					2,2,
					2,2,1,
			/* FAST ADCU */	2,2,2,2,2};

#define IPC_OFFSET_DCU(dcuid)   ((dcuid) * IPC_BLOCK_SIZE + DAQ_BASE_ADDRESS)
#define DATA_OFFSET_DCU(dcuid)  (DAQ_DATA_BASE_ADD + DAQ_DCU_SIZE*((dcuid) - 5))

#define IS_TP_DCU(dcuid) ((dcuid) == DCU_ID_EX_16K || (dcuid) == DCU_ID_TP_16K || (dcuid) == DCU_ID_EX_2K || (dcuid) == DCU_ID_TP_2K)

/* dataType Defintions */
#define DAQ_DATATYPE_16BIT_INT  1       /* Data type signed 16bit integer */
#define DAQ_DATATYPE_32BIT_INT  2       /* Data type signed 32bit integer */
#define DAQ_DATATYPE_64BIT_INT  3       /* Data type signed 64bit integer */
#define DAQ_DATATYPE_FLOAT      4       /* Data type 32bit floating point */
#define DAQ_DATATYPE_DOUBLE     5       /* Data type 64bit double float */
#define DAQ_DATATYPE_COMPLEX    6       /* Complex data; two 32bit floats */

#define DAQ_DATA_TYPE_SIZE(a) (((a)==DAQ_DATATYPE_16BIT_INT)? 2: 4)

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

/* Gds control block starts at this offset */
#define DAQ_GDS_BLOCK_ADD	(DAQ_BASE_ADDRESS + 0x40000)

/* The maximum possibile size (allocated space) of the test point table */
#define DAQ_GDS_MAX_TP_NUM           0x100

/* The total number of test point DCUs */
#define DAQ_GDS_DCU_NUM       4

/* Total number of channels allowed per DCU */
#define DCU_MAX_CHANNELS	512

/* RFM offset of DCU DAQ configuration area; used for communicating
   configuration from Epics processor to a front-end processor */
#define DAQ_INFO_ADDRESS	0x01ff0000

/* DCU DAQ configuration area starts at DAQ_INFO_ADDRESS offset in RFM
   and contains an array of DAQ_INFO_BLOCK of DCU_COUNT elements */
typedef struct DAQ_INFO_BLOCK {
  int reconfig; /* Set to 1 by the Epics to indicate configuration change */
  int numChans; /* Defines how many channels are configured int struct tp[] */
  unsigned long configFileCRC; /* DAQ config file checksum */
  struct {
    unsigned int tpnum; /* Test point number to which this DAQ channel connects */
    unsigned int dataType;
    unsigned int dataRate;
    float dataGain;
  } tp[DCU_MAX_CHANNELS];
} DAQ_INFO_BLOCK;


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

static const int daqGdsTpNum[4] = { DAQ_GDS_TP_LSC_EX_NUM, DAQ_GDS_TP_ASC_EX_NUM, DAQ_GDS_TP_LSC_TP_NUM, DAQ_GDS_TP_ASC_TP_NUM };

/* GDS layout:
   4K GDS GDS_CNTRL_BLOCK at DAQ_GDS_BLOCK_ADD
   2K GDS GDS_CNTRL_BLOCK at DAQ_GDS_BLOCK_ADD + sizeof(GDS_CNTRL_BLOCK)
   
   GDS_CNTRL_BLOCK.excNum16K[0] has the test point numbers
   GDS_CNTRL_BLOCK.excNum16K[1] has the channel status words
*/

#if defined(COMPAT_INITIAL_LIGO)
typedef union GDS_CNTRL_BLOCK {
  unsigned short tp [4][2][DAQ_GDS_MAX_TP_NUM];
  struct {
    unsigned short excNum16k[2][DAQ_GDS_MAX_TP_NUM];
    unsigned short excNum2k[2][DAQ_GDS_MAX_TP_NUM];
    unsigned short tpNum16k[2][DAQ_GDS_MAX_TP_NUM];
    unsigned short tpNum2k[2][DAQ_GDS_MAX_TP_NUM];
  } tpe;
} GDS_CNTRL_BLOCK;

#else
typedef union GDS_CNTRL_BLOCK {
  unsigned int tp [4][2][DAQ_GDS_MAX_TP_NUM];
  struct {
    unsigned int excNum16k[2][DAQ_GDS_MAX_TP_NUM];
    unsigned int excNum2k[2][DAQ_GDS_MAX_TP_NUM];
    unsigned int tpNum16k[2][DAQ_GDS_MAX_TP_NUM];
    unsigned int tpNum2k[2][DAQ_GDS_MAX_TP_NUM];
  } tpe;
} GDS_CNTRL_BLOCK;

/* GDS test point table structure for FE to frame builder communication */
typedef struct cdsDaqNetGdsTpNum {
   int count; /* test points count */
   int tpNum[DAQ_GDS_MAX_TP_NUM];
} cdsDaqNetGdsTpNum;
#endif

#define GDS_TP_MAX_FE	1250

#endif
