/* Version: $Id$ */
/* Module Name: map.h							*/
/*                                                         		*/
/* Module Description: LIGO Data Acquisition System Reflective Memory   */
/* header file. This file defines the layout of the DAQ RFM.            */
/* This defines structures and field values for DAQ                     */
/* Interprocess Communication (IPC) and data transmission               */
/* from the DCU and EDCU to the Frame Builders.                         */
/*                                                         		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date    Engineer   Comments			   		*/
/* 00    01Jul98 R. Bork    First Release.		   		*/
/* 01    10Jul98 D. Barker  Added field defines, added EDCU offset.     */
/*                          Add channel data status defines.            */
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages:							*/
/*	References:							*/
/*                                                         		*/
/* Author Information:							*/
/*	Name          Telephone    Fax          e-mail 			*/
/*	David Barker. (509)3736203 (509)3722178 barker@ligo.caltech.edu */
/*                                                         		*/
/* Code Compilation and Runtime Specifications:				*/
/*	Code Compiled on: Sun Ultra Enterprise 2 running Solaris2.5.1   */
/*	Compiler Used: Heurikon's gcc-sde				*/
/*	Runtime environment: Baja47 running VxWorks 5.2 Beta B.		*/
/*                                                         		*/
/* Code Standards Conformance:						*/
/*	Code Conforms to: LIGO standards.	TBD			*/
/*			  Lint.			TBD			*/
/*			  ANSI			TBD			*/
/*			  POSIX			TBD			*/
/*									*/
/* Known Bugs, Limitations, Caveats:					*/
/*									*/
/*	BUGS LIMITATIONS AND CAVEATS					*/
/* ONLY DEFINES HANFORD 2K INTERFEROMETER RFM NETWORK. 4K IFO NEEDS     */
/* TO BE ADDED.                                                         */
/*                                                         		*/
/*                      -------------------                             */
/*                                                         		*/
/*                             LIGO					*/
/*                                                         		*/
/*        THE LASER INTERFEROMETER GRAVITATIONAL WAVE OBSERVATORY.	*/
/*                                                         		*/
/*                     (C) The LIGO Project, 1997.			*/
/*                                                         		*/
/*                                                         		*/
/* California Institute of Technology			   		*/
/* LIGO Project MS 51-33				   		*/
/* Pasadena CA 91125					   		*/
/*                                                         		*/
/* Massachusetts Institute of Technology		   		*/
/* LIGO Project MS 20B-145				   		*/
/* Cambridge MA 01239					   		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/


/* The reflected memory for the Hanford 2k DAQS is laid out as follows	*/
/*	--------------------------------------------------------------	*/
/*	Base ----------  Reflected memory control registers		*/
/*	--------------------------------------------------------------	*/
/*	0x40	DAQS controller IPC					*/
/*	--------------------------------------------------------------	*/
/*	0x140	Config Manager IPC					*/
/*	--------------------------------------------------------------	*/
/*	0x240	FB0 IPC							*/
/*	--------------------------------------------------------------	*/
/*	0x340	FB1 IPC							*/
/*	--------------------------------------------------------------	*/
/*	0x440	MAIN MAP						*/
/*	--------------------------------------------------------------	*/
/*	0x1040	DAQS Frame Broadcast Server IPC Area			*/
/*	--------------------------------------------------------------	*/
/*	0x2000	EDCU IPC						*/
/*	--------------------------------------------------------------	*/
/*	0x4000	EDCU DATA (Slow Data Format)				*/
/*	--------------------------------------------------------------	*/
/*	0x40000		ADCU-2k1 IPC (IO SUS - 384K)			*/
/*	--------------------------------------------------------------	*/
/*	0x40100		ADCU-2k1 Data Info Area				*/
/*	--------------------------------------------------------------	*/
/*	0x42000		ADCU-2k1 Data Block 1				*/
/*	--------------------------------------------------------------	*/
/*	0xA0000		ADCU-2k2 IPC (IO SUS - 256K)			*/
/*	--------------------------------------------------------------	*/
/*	0xA0100		ADCU-2k2 Data Info Area				*/
/*	--------------------------------------------------------------	*/
/*	0xA2000		ADCU-2k2 Data Block 1				*/
/*	--------------------------------------------------------------	*/
/*	0xE0000		ADCU-2k3 IPC (IO ASC/LSC/PSL - 384K)		*/
/*	--------------------------------------------------------------	*/
/*	0xE0100		ADCU-2k3 Data Info Area				*/
/*	--------------------------------------------------------------	*/
/*	0xE2000		ADCU-2k3 Data Block 1				*/
/*	--------------------------------------------------------------	*/
/*	0x140000	ADCU-2k4 IPC (PEM - 256K) 			*/
/*	--------------------------------------------------------------	*/
/*	0x140100	ADCU-2k4 Data Info Area				*/
/*	--------------------------------------------------------------	*/
/*	0x142000	ADCU-2k4 Data Block 1				*/
/*	--------------------------------------------------------------	*/
/*	0x180000	ADCU-2k5 IPC (Left Mid Station - 128K)		*/
/*	--------------------------------------------------------------	*/
/*	0x180100	ADCU-2k5 Data Info Area				*/
/*	--------------------------------------------------------------	*/
/*	0x182000	ADCU-2k5 Data Block 1				*/
/*	--------------------------------------------------------------	*/
/*	0x1A0000	ADCU-2k6 IPC (Right Mid Station - 128K)		*/
/*	--------------------------------------------------------------	*/
/*	0x1A0100	ADCU-2k6 Data Info Area 			*/
/*	--------------------------------------------------------------	*/
/*	0x1A2000	ADCU-2k6 Data Block 1				*/
/*	--------------------------------------------------------------	*/
/*	0x1E0000	GDS Temporary Signal Listing			*/
/*	--------------------------------------------------------------	*/
/*	0x1E0100	GDS Temporary Data List 			*/
/*	--------------------------------------------------------------	*/
/*	0x200000 - 0x200040 Reserved (RFM Control Area)			*/
/*	--------------------------------------------------------------	*/
/*	0x202000	DDCU-2kLSC IPC (384K)				*/
/*	--------------------------------------------------------------	*/
/*	0x202100	DDCU-2kLSC Data Info Area			*/
/*	--------------------------------------------------------------	*/
/*	0x204000	DDCU-2kLSC Data Block 1				*/
/*	--------------------------------------------------------------	*/
/*	0x262000	DDCU-2kASC IPC (256K)				*/
/*	--------------------------------------------------------------	*/
/*	0x262100	DDCU-2kASC Data Info Area			*/
/*	--------------------------------------------------------------	*/
/*	0x264000	DDCU-2kASC Data Block 1				*/
/*	--------------------------------------------------------------	*/
/*	0x2A2000	GDS-2kLSC-Ex IPC (256K)				*/
/*	--------------------------------------------------------------	*/
/*	0x2A2100	GDS-2kLSC-Ex Data Info Area			*/
/*	--------------------------------------------------------------	*/
/*	0x2A4000	GDS-2kLSC-Ex Data Block 1			*/
/*	--------------------------------------------------------------	*/
/*	0x2E2000	GDS-2kASC-Ex IPC (128K)				*/
/*	--------------------------------------------------------------	*/
/*	0x2E2100	GDS-2kASC-Ex Data Info Area			*/
/*	--------------------------------------------------------------	*/
/*	0x2E4000	GDS-2kASC-Ex Data Block 1			*/
/*	--------------------------------------------------------------	*/
/*	0x302000	GDS-2kLSC-TP IPC (512K)				*/
/*	--------------------------------------------------------------	*/
/*	0x302100	GDS-2kLSC-TP Data Info Area			*/
/*	--------------------------------------------------------------	*/
/*	0x304000	GDS-2kLSC-TP Data Block 1			*/
/*	--------------------------------------------------------------	*/
/*	0x382000	GDS-2kASC-TP IPC (256K)				*/
/*	--------------------------------------------------------------	*/
/*	0x382100	GDS-2kASC-TP Data Info Area			*/
/*	--------------------------------------------------------------	*/
/*	0x384000	GDS-2kASC-TP Data Block 1			*/
/*	--------------------------------------------------------------	*/
/*	0x3C2000 - 0x400000	GDS-RESERVED (250K)			*/
/*	--------------------------------------------------------------	*/

/* Include File Duplication Lock */
#ifndef _DAQ_MAP_H
#define _DAQ_MAP_H
#ifdef __cplusplus
extern "C" {
#endif


/* ----------------------------------------------------------------------*/
/* ---------- GENERAL #DEFINES ------------------------------------------*/

/* A DCU IPC will be at BASE + DCUx_OFFSET	*/
/* A DCU DataInfo area and first data block will be offset from IPC by:	*/
#define IPC2INFO_OFFSET		0x100
#define IPC2DATA_OFFSET		0x6000
#define IPC2DATA_OFFSET_PMC	0x6000

#define IPC2SLOW_DATA_OFFSET   	0x100
#define SLOW_DATA_SIZE 0x48
#define PMC5579_BASE_DAQ_OFFSET	0x02100000

/* Available memory for 1/16 sec data blocks for each type of DCU memory size */
#define DATA_BLOCK_SIZE_640     0x13000 /* 78KByte      */
#define DATA_BLOCK_SIZE_512	0xF400	/* 62KByte 	*/
#define DATA_BLOCK_SIZE_384	0xB400	/* 46KByte 	*/
#define DATA_BLOCK_SIZE_256	0x7400	/* 29KByte	*/
#define DATA_BLOCK_SIZE_128	0x3400	/* 13KByte	*/

#define DATA_BLOCKS	8	/* Number of 1/16sec data blocks in RM	*/
#define MEM_TOP_5588	0x40	/* First usable RM RAM area 		*/
#define DAQSC_IPC	0x40	/* DAQS controller IPC Area		*/
#define CM_IPC   	0x140	/* DAQS controller IPC Area		*/
#define FB0_IPC		0x240	/* FrameBuilder IPC Area		*/
#define FB1_IPC		0x340	/* DAQS Repeater IPC Area		*/
#define	MAIN_MAP	0x440	/* Location of main memory map		*/
#define DAQ_FBS_IPC	0x1040  /* Frame Broadcast Server IPC Area	*/
#define DAQ_NDS_IPC	0x1140  /* Frame Trend/NDS Server IPC Area	*/

/* Offsets from RM BASE ADDRESS to each DCU IPC area			*/
/* These are for the Hanford 2k IFO network				*/
#define EDCU_OFFSET	0x2000		/* Slow Data in one block */
#define DCU0_OFFSET	0x40000		/* ADCU-2k1  	256KByte size	*/
#define DCU1_OFFSET	0x80000		/* ADCU-2k2  	128KByte size	*/
#define DCU2_OFFSET	0xA0000		/* ADCU-2k3  	384KByte size	*/
#define DCU3_OFFSET	0x100000	/* ADCU-2k4  	512KByte size	*/
#define DCU4_OFFSET	0x180000	/* ADCU-2k5  	128KByte size	*/
#define DCU5_OFFSET	0x1a0000	/* ADCU-2k6	128KByte size	*/
#define DCU6_OFFSET	0x202000	/* DDCU-2kLSC	512KByte size	*/
#define DCU7_OFFSET	0x282000	/* DDCU-2kASC	256KByte size	*/
#define DCU8_OFFSET	0x2C2000	/* GDS-2kLSC-Ex	256KByte size	*/
#define DCU9_OFFSET	0x302000	/* GDS-2kASC-Ex	128KByte size	*/
#define DCU10_OFFSET	0x322000	/* GDS-2kLSC-TP	512KByte size	*/
#define DCU11_OFFSET	0x3A2000	/* GDS-2kASC-TP	256KByte size	*/
#define DCU13_OFFSET	0x3E2000	/* GDS-Reserved	128KByte size	*/
#define DCU29_OFFSET	0x1e0000	/* GDS Temp channel info area	*/
#define DCU40_OFFSET	0x000000	/* 5579PMC ADCU-1		*/
#define DCU41_OFFSET	0x100000	/* 5579PMC ADCU-2		*/
#define DCU42_OFFSET	0x200000	/* 5579PMC ADCU-3		*/
#define DCU43_OFFSET	0x300000	/* 5579PMC ADCU-4		*/
#define UNDEFINED       0x0             /* undefined offset (temporary) */

/* ----------------------------------------------------------------------*/
/* ---------- STRUCTURE DEFINITIONS  ------------------------------------*/

struct mainMapStr {
  unsigned long baseOffset;	/* Offset from base to IPC	*/
  unsigned long dataBlockSize;	/* Size of 1/16 sec data block	*/
  unsigned long partitionSize;  /* Offset between data blocks	*/
  unsigned long numBlocks;	/* Total number of data blocks	*/
};

  /* Fast data block properties structure */
typedef struct edcuBlockProp {
  int status;
  unsigned long timeSec;
  unsigned long timeNSec;
  unsigned long run;
  unsigned long cycle;
} edcuBlockPropT;

struct edcuRmIpcStr {		/* IPC area structure			*/
  short dcuId;            /* id of unit, unique within each site  */
  short dcuType;          /* type of unit (e.g. DCU, FB, etc.)    */
  short dcuNodeId;        /* RFM node id, set by jumpers on card. */
  short command;          /* Allows DAQSC to command unit.        */
  short cmdAck;           /* Allows unit to acknowledge DAQS cmd. */
  short request;		/* DCU request of controller		*/
  short reqAck;		/* controller acknowledge of DCU req.	*/
  short status;           /* Unit reports status to DAQSC         */
  short errMsg;           /* Detailed error description           */
  short channelCount;	/* Number of data channels in a DCU	*/
  char confName[32];	/* File name of loaded config file	*/
  unsigned int dataBlockSize;/* Num bytes actually written by 	*/
				/* DCU within a 1/16 data block		*/
  unsigned long cycle;    /* copy of latest cycle num from blocks */
  edcuBlockPropT bp [DATA_BLOCKS]; /* array of block property structures */
  
};

struct edcuDataInfoStr {		/* DataInfo Area structure		*/
  int chNum;          /* Hardware address of channel          */
  char  chName[32];     /* Channel name                         */
  int ifoId;          /* IFO this chan is associated with     */
  int dcuId;          /* DCU this channel is acquired by      */
  int dataType;       /* Channel data type                    */
  unsigned long rate;           /* Chan acquire rate            */
  unsigned long dataOffset;     /* Offset from beginning of a   */
                                /* Data block to channel status */
                                /* Word                         */
  float chGain;                 /* Chan ADC gainin db           */
  char engUnits[32];            /* Chan engineering units       */
  float slope;                  /* Conversion linear multiplier */
  float offset;                 /* Conversion DC offset         */
  int fbcr;                   /* FB control word              */
};

  /* original structures */
 /* Fast data block properties structure */
typedef struct blockProp {
  short status;
  unsigned long timeSec;
  unsigned long timeNSec;
  unsigned long run;
  unsigned long cycle;
} blockPropT;

struct rmIpcStr {       /* IPC area structure                   */
  short dcuId;          /* id of unit, unique within each site  */
  short dcuType;        /* type of unit (e.g. DCU, FB, etc.)    */
  short dcuNodeId;      /* RFM node id, set by jumpers on card. */
  short command;        /* Allows DAQSC to command unit.        */
  short cmdAck;         /* Allows unit to acknowledge DAQS cmd. */
  short request;        /* DCU request of controller            */
  short reqAck;         /* controller acknowledge of DCU req.   */
  short status;         /* Unit reports status to DAQSC         */
  short errMsg;         /* Detailed error description           */
  char confName[32];
  short channelCount;   /* Number of data channels in a DCU     */
  unsigned short dataBlockSize;/* Num bytes actually written by         */
                                /* DCU within a 1/16 data block         */
  unsigned long cycle;    /* copy of latest cycle num from blocks */
  blockPropT bp [DATA_BLOCKS]; /* array of block property structures */

};


struct dataInfoStr {            /* DataInfo Area structure              */
  unsigned short chNum;          /* Hardware address of channel          */
  char  chName[32];     /* Channel name                         */
  short ifoId;          /* IFO this chan is associated with     */
  short dcuId;          /* DCU this channel is acquired by      */
  short dataType;       /* Channel data type                    */
  unsigned long rate;           /* Chan acquire rate            */
  unsigned long dataOffset;     /* Offset from beginning of a   */
                                /* Data block to channel status */
                                /* Word                         */
  float chGain;                 /* Chan ADC gainin db           */
  char engUnits[32];            /* Chan engineering units       */
  float slope;                  /* Conversion linear multiplier */
  float offset;                 /* Conversion DC offset         */
  short fbcr;                   /* FB control word              */
};









  
  /* structure for 1Hz EPICS data					*/
  /* Note that slow EDCU area does not contain a dataInfo area	*/
  /* Instead, this structure appears in the data block itself	*/
  
  struct slowDataStr {		/* EPICS slow (1Hz) data structure	*/
    int   status;         /* Channel's data validity.             */
    float   data;           /* Channel's data value.                */
  };
  
  /* Not shown as a struct; each fast data block has the following layout: */
  /*          int   status;          Data validity                         */
  /*          short val0;            Data for time t                       */
  /*          short val1;            Data for time t+dt                    */
  /*          short val2;            Data for time t+2dt                   */
  /*          ..... */
  
#define DAQS_STATUS_WORD_SIZE     4             /* Fast data status word size*/
  
  
  /* ----------------------------------------------------------------------*/
  /* ---------- IPC STRUCTURE DEFINES -------------------------------------*/
  
  /* DAQ IPC Unit type definitions */
#define DAQS_UTYPE_CS		1		/* Controller */
#define DAQS_UTYPE_CM           2               /* Configuration Manager */
#define DAQS_UTYPE_FB		3		/* Frame Builder */
#define DAQS_UTYPE_ADCU		4		/* Analog DCU */
#define DAQS_UTYPE_DDCU		5		/* ISC DCU */
#define DAQS_UTYPE_EDCU		6		/* Epics DCU */
#define DAQS_UTYPE_GDS_EXC	7		/* GDS Excitation system */
#define DAQS_UTYPE_GDS_TP	8		/* GDS Test Point System */
#define DAQS_UTYPE_GDS_FLAG	9		/* GDS Flag Channel System */
  
  /* DAQ IPC Command bit definitions */
#define DAQS_CMD_NO_CMD         0x00
#define DAQS_CMD_CONFIGURE	0x01		/* Config/reconfig */
#define DAQS_CMD_START		0x02		/* Start acquisition */
#define DAQS_CMD_HALT           0x04
#define DAQS_CMD_CALIBRATE      0x08
#define DAQS_CMD_KILL           0x10
#define DAQS_CMD_RFM_REFRESH    0x20            /* Refresh DCU RFM area */
#define DAQS_CMD_NEW_GAINS	0x50            /* New gain set from operator */

#define DAQS_REQ_FAULT		-1        /* Req is not valid */
#define DAQS_REQ_PENDING	0x30      /* Req is being worked */
#define DAQS_REQ_CALLBACK	0x40      /* Not in state to process REQ */
#define DAQS_REQ_HEARTBEAT	0x60	  /* Heartbeat Monitor */

/* DAQ IPC Command Source definitions */
#define DAQS_CMDSRC_NO_SRC      -1

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

/* ERROR DEFINITIONS */

/* DAQ General, CS and CM Errors (0x000) */
#define DAQ_OK	                0x000	/* No Error */
#define DAQ_DAQSCERR_RFM_MISSING   0x020 /* DAQSC, Error no 5588 RM card */
#define DAQ_DAQSCERR_CMDTASK_ERROR 0x021 /* DAQSC, Error with Cm Cmd task */
#define DAQ_CMERR_RFM_MISSING   0x050 /* CM, Error no 5588 RM card */
#define DAQ_CMERR_CMDTASK_ERROR 0x051 /* CM, Error with Cm Cmd task */
#define DAQ_CMERR_CHNCFG_OPENERR 0x052 /* CM, Chan config file open error */
#define DAQ_CMERR_HW_INVALID   0x053  /* CM, hardware config invalid */
#define DAQ_CMERR_CHNCFG_ERROR 0x054  /* CM, error with chan config entry */
#define DAQ_CMERR_CHANDB_ERROR 0x055  /* CM, error with internal chan db */

/* ADCU Errors (0x100) */

#define DAQ_ADCUERR_ADC_INT_TIMEOUT 0x100 /* DCU, ADC interrupt missing */
#define DAQ_ADCUERR_ADC_INT_T_ERROR 0x101 /* DCU, ADC interrupt too soon */
#define DAQ_ADCUERR_ADC_READOUT_0   0x102 /* DCU, Error reading ADC 0 */
#define DAQ_ADCUERR_ADC_READOUT_1   0x103 /* DCU, Error reading ADC 1 */
#define DAQ_ADCUERR_ADC_READOUT_2   0x104 /* DCU, Error reading ADC 2 */
#define DAQ_ADCUERR_ADC_READOUT_3   0x105 /* DCU, Error reading ADC 3 */
#define DAQ_ADCUERR_RM_WRITE	0x106 /* DCU, Error write to 5588 */
#define DAQ_ADCUERR_GPS_READ	0x107 /* DCU, Error reading GPS time */
#define DAQ_ADCUERR_GPS_INT 	0x108 /* DCU, Error GPS 1Hz missing */
#define DAQ_ADCUERR_GPS_MISSING	0x109 /* DCU, Error GPS Card Missing */
#define DAQ_ADCUERR_ADC_MISSING	0x10a /* DCU, Error no ADCs found */
#define DAQ_ADCUERR_RFM_MISSING	0x10b /* DCU, Error no 5588 RM card */
#define DAQ_ADCUERR_TRIG_MISSING  0x10c	/* DCU, Error no Trigger card */
#define DAQ_ADCUERR_DDA_UNAVAILABLE 0x10d /* DCU, Error cannot access DDA */
#define DAQ_ADCUERR_CHAN_DATABASE 0x10e /* DCU, Error with internal database */
#define DAQ_ADCUERR_ADCMON_ERROR  0x10f /* DCU, Error with ADC monitor task */
#define DAQ_ADCUERR_DCUCMD_ERROR  0x110 /* DCU, Error with Dcu Cmd task */

/* EDCU Errors (0x200) */

/* DDCU Errors (0x300) */

/* Frame Builder Errors (0x400) */

/* IPC  Errors (0x500) */
#define DAQ_IPCERR_CMD_IN_USE 0x500 /* IPC, rem node being commanded */
#define DAQ_IPCERR_REMOTE_INT 0x501 /* IPC, rem node, cannot set interrupt*/
#define DAQ_IPCERR_ACK_TIMEOUT 0x502 /* IPC, rem node did not ack cmd */

/* END OF ERROR DEFINITIONS */

/* Config Name Definitions */
#define DAQ_CNAME_NO_CONFIG "no configuration"


/* ----------------------------------------------------------------------*/
/* ------------ DATA INFO STRUCTURE DEFINES -----------------------------*/

/* IFOID Definitions */
#define DAQ_IFOID_NONE	0	/* Not associated with particular IFO */
#define DAQ_IFOID_2K	1	/* Associated with 2km IFO */
#define DAQ_IFOID_4K	2	/* Associated with 4km IFO */

/* dataType Defintions */
#define DAQ_DATATYPE_16BIT_INT	1	/* Data type signed 16bit integer */
#define DAQ_DATATYPE_32BIT_INT	2	/* Data type signed 32bit integer */
#define DAQ_DATATYPE_64BIT_INT	3	/* Data type signed 64bit integer */
#define DAQ_DATATYPE_FLOAT	4	/* Data type 32bit floating point */
#define DAQ_DATATYPE_DOUBLE	5	/* Data type 64bit double float */
#define DAQ_DATATYPE_COMPLEX    6       /* Data type 64bit complex floating point */
#define DAQ_DATATYPE_32BIT_UINT 7       /* Data type unsigned 32bit integer */

/* dataBlock Definitions */
#define DAQ_SLOW_DATA	1
#define DAQ_FAST_DATA	2

/* rate Definitions (powers of 2) */
#define DAQ_RATE_16	4	/* 16Hz (slowest) */
#define DAQ_RATE_32	5	/* 32Hz */
#define DAQ_RATE_64	6	/* 64 Hz */
#define DAQ_RATE_128	7	/* 128Hz */
#define DAQ_RATE_256	8	/* 256 Hz */
#define DAQ_RATE_512	9	/* 512 Hz */
#define DAQ_RATE_1024	10	/* 1k Hz */
#define DAQ_RATE_2048	11	/* 2k Hz */
#define DAQ_RATE_4096	12	/* 4k Hz */
#define DAQ_RATE_8192	13	/* 8k Hz */
#define DAQ_RATE_16384	14	/* 16k Hz (fastest) */

/* priority Definitions */
#define DAQ_CHAN_PRIORITY_HIGH 1 /* high priority */
#define DAQ_CHAN_PRIORITY_LOW  2 /* low priority */

/* ----------------------------------------------------------------------*/
/* ------------ CHANNEL DATA (FAST & SLOW) DEFINES ----------------------*/

/* Channel Data Status */
#define DAQ_DATA_OK        0      /* data is valid */
#define DAQ_DATA_INVALID  -1      /* data is invalid */

/* ----------------------------------------------------------------------*/
/* ------------ DCU and RFM NODE ID DEFINTIONS     ----------------------*/
#define NET1_TOP	17
#define NET2_TOP	29
#define TOTAL_DCU	35
#define MAX_EPICS_CHANS	10000
#define MAX_GDS_CHANS	10000
#define MAX_DCU         40
#define HANFORD 1
#define LIVINGSTON 2
#define CALTECH 3
#define CM_MAX_NUM_CHANNELS 192
#define CM_MAX_SLOW_CHANNELS 2048


/* LOGICAL UNIT IDENTIFIERS */

/* HANFORD 2K IFO NETWORK */
#define DAQSC_ID            0
#define CM_ID               1
#define FB0_ID              2
#define FB1_ID              3
#define EDCU_ID            29
#define ADCU_2k1_ID         5
#define ADCU_2k2_ID         6
#define ADCU_2k3_ID         7
#define ADCU_2k4_ID         8
#define ADCU_2k5_ID         9
#define ADCU_2k6_ID         10 
#define DDCU_2k_LSC_ID      11
#define DDCU_2k_ASC_ID      12
#define GDS_2k_LSC_EX_ID    13
#define GDS_2k_ASC_EX_ID    14
#define GDS_2k_LSC_TP_ID    15
#define GDS_2k_ASC_TP_ID    16
/* HANFORD 4K IFO NETWORK */
#define ADCU_4k1_ID         17
#define ADCU_4k2_ID         18
#define ADCU_4k3_ID         19
#define ADCU_4k4_ID         20
#define ADCU_4k5_ID         21
#define ADCU_4k6_ID         22
#define DDCU_4k_LSC_ID      23
#define DDCU_4k_ASC_ID      24
#define GDS_4k_LSC_EX_ID    25
#define GDS_4k_ASC_EX_ID    26
#define GDS_4k_LSC_TP_ID    27
#define GDS_4k_ASC_TP_ID    28
#define GDS_MAP_ID	    30
#define ADCU_PMC_40_ID	    31
#define ADCU_PMC_41_ID	    32
#define ADCU_PMC_42_ID	    33
#define ADCU_PMC_43_ID	    34


/* REFLECTIVE MEMORY NODE IDS */

/* HANFORD 2K IFO NETWORK */
#define DAQSC_RFMID            0
#define CM_RFMID               1
#define FB0_RFMID              2
#define FB1_RFMID              3
#define EDCU_RFMID             4
#define ADCU_2k1_RFMID         5
#define ADCU_2k2_RFMID         6
#define ADCU_2k3_RFMID         7
#define ADCU_2k4_RFMID         8
#define ADCU_2k5_RFMID         9
#define ADCU_2k_PEM_RFMID      10
#define DDCU_2k_LSC_RFMID      11
#define DDCU_2k_ASC_RFMID      12
#define GDS_2k_LSC_EX_RFMID    13
#define GDS_2k_ASC_EX_RFMID    13
#define GDS_2k_LSC_TP_RFMID    1
#define GDS_2k_ASC_TP_RFMID    1
#define GDS_2k_ASC_FLAG_RFMID  17
/* HANFORD 4K IFO NETWORK */
#define ADCU_4k1_RFMID         18
#define ADCU_4k2_RFMID         19
#define ADCU_4k3_RFMID         20
#define ADCU_4k4_RFMID         21
#define ADCU_4k_PEM_RFMID      22
#define DDCU_4k_LSC_RFMID      23
#define DDCU_4k_ASC_RFMID      24
#define GDS_4k_LSC_EX_RFMID    25
#define GDS_4k_ASC_EX_RFMID    25
#define GDS_4k_LSC_TP_RFMID    1
#define GDS_4k_ASC_TP_RFMID    1
#define GDS_4k_ASC_FLAG_RFMID  29

  /* Macro to convert unit id to rfm node id */
#define UNIT_ID_TO_RFM_NODE_ID(A) ( \
((A) == DAQSC_ID ? DAQSC_RFMID : \
 ((A) == CM_ID ? CM_RFMID :       \
  ((A) == FB0_ID ? FB0_RFMID :     \
   ((A) == FB1_ID ? FB1_RFMID :     \
    ((A) == EDCU_ID ? EDCU_RFMID :     \
     ((A) == ADCU_2k1_ID ? ADCU_2k1_RFMID :     \
      ((A) == ADCU_2k2_ID ? ADCU_2k2_RFMID :     \
       ((A) == ADCU_2k3_ID ? ADCU_2k3_RFMID :     \
        ((A) == ADCU_2k4_ID ? ADCU_2k4_RFMID :     \
	 ((A) == ADCU_2k5_ID ? ADCU_2k5_RFMID :     \
	  ((A) == DDCU_2k_LSC_ID ? DDCU_2k_LSC_RFMID :     \
	   ((A) == DDCU_2k_ASC_ID ? DDCU_2k_ASC_RFMID :     \
	    ((A) == GDS_2k_LSC_EX_ID ? GDS_2k_LSC_EX_RFMID :     \
	     ((A) == GDS_2k_ASC_EX_ID ? GDS_2k_ASC_EX_RFMID :     \
	      ((A) == GDS_2k_LSC_TP_ID ? GDS_2k_LSC_TP_RFMID :     \
	       ((A) == GDS_2k_ASC_TP_ID ? GDS_2k_ASC_TP_RFMID :     \
		 ((A) == ADCU_4k1_ID ? ADCU_4k1_RFMID :     \
		  ((A) == ADCU_4k2_ID ? ADCU_4k2_RFMID :     \
		   ((A) == ADCU_4k3_ID ? ADCU_4k3_RFMID :     \
		    ((A) == ADCU_4k4_ID ? ADCU_4k4_RFMID :     \
		       ((A) == DDCU_4k_LSC_ID ? DDCU_4k_LSC_RFMID :     \
			((A) == DDCU_4k_ASC_ID ? DDCU_4k_ASC_RFMID :     \
			 ((A) == GDS_4k_LSC_EX_ID ? GDS_4k_LSC_EX_RFMID :     \
			  ((A) == GDS_4k_ASC_EX_ID ? GDS_4k_ASC_EX_RFMID :     \
			   ((A) == GDS_4k_LSC_TP_ID ? GDS_4k_LSC_TP_RFMID :     \
			    ((A) == GDS_4k_ASC_TP_ID ? GDS_4k_ASC_TP_RFMID :     \
			      -1)))))))))))))))))))))))))))
       
       /* Macro to convert unit id to RFM OFFSET to start of IPC. */
#define UNIT_ID_TO_RFM_OFFSET(A) ( \
((A) == DAQSC_ID ? DAQSC_IPC : \
 ((A) == CM_ID ? CM_IPC :     \
  ((A) == FB0_ID ? FB0_IPC :     \
   ((A) == FB1_ID ? FB1_IPC :     \
    ((A) == EDCU_ID ? EDCU_OFFSET :     \
     ((A) == ADCU_2k1_ID ? DCU0_OFFSET :     \
      ((A) == ADCU_2k2_ID ? DCU1_OFFSET :     \
       ((A) == ADCU_2k3_ID ? DCU2_OFFSET :     \
        ((A) == ADCU_2k4_ID ? DCU3_OFFSET :     \
	 ((A) == ADCU_2k5_ID ? DCU4_OFFSET :     \
	  ((A) == ADCU_2k6_ID ? DCU5_OFFSET :     \
	   ((A) == DDCU_2k_LSC_ID ? DCU6_OFFSET :     \
	    ((A) == DDCU_2k_ASC_ID ? DCU7_OFFSET :     \
	     ((A) == GDS_2k_LSC_EX_ID ? DCU8_OFFSET :     \
	      ((A) == GDS_2k_ASC_EX_ID ? DCU9_OFFSET :     \
	       ((A) == GDS_2k_LSC_TP_ID ? DCU10_OFFSET :     \
		((A) == GDS_2k_ASC_TP_ID ? DCU11_OFFSET :     \
		 ((A) == ADCU_4k1_ID ? DCU0_OFFSET :     \
		  ((A) == ADCU_4k2_ID ? DCU1_OFFSET :     \
		   ((A) == ADCU_4k3_ID ? DCU2_OFFSET :     \
		    ((A) == ADCU_4k4_ID ? DCU3_OFFSET :     \
		     ((A) == ADCU_4k5_ID ? DCU4_OFFSET :     \
		      ((A) == ADCU_4k6_ID ? DCU5_OFFSET :     \
		       ((A) == DDCU_4k_LSC_ID ? DCU6_OFFSET :     \
			((A) == DDCU_4k_ASC_ID ? DCU7_OFFSET :     \
			 ((A) == GDS_4k_LSC_EX_ID ? DCU8_OFFSET :     \
			  ((A) == GDS_4k_ASC_EX_ID ? DCU9_OFFSET :     \
			   ((A) == GDS_4k_LSC_TP_ID ? DCU10_OFFSET :     \
			    ((A) == GDS_4k_ASC_TP_ID ? DCU11_OFFSET :     \
			    ((A) == 29 ? DCU29_OFFSET :     \
			    ((A) == GDS_MAP_ID ? DCU29_OFFSET :     \
			    ((A) == ADCU_PMC_40_ID ? DCU40_OFFSET :     \
			    ((A) == ADCU_PMC_41_ID ? DCU41_OFFSET :     \
			    ((A) == ADCU_PMC_42_ID ? DCU42_OFFSET :     \
			    ((A) == ADCU_PMC_43_ID ? DCU43_OFFSET :     \
			     -1))))))))))))))))))))))))))))))))))))
       
       
       /* Macro to convert unit id to RFM block size (bytes). */
#define UNIT_ID_TO_RFM_SIZE(A) ( \
((A) == DAQSC_ID ? 128 : \
 ((A) == CM_ID ? 128 :     \
  ((A) == FB0_ID ? 128 :     \
   ((A) == FB1_ID ? 128 :     \
    ((A) == EDCU_ID ? 261632 :     \
     ((A) == ADCU_2k1_ID ? 262144 :     \
      ((A) == ADCU_2k2_ID ? 131072 :     \
       ((A) == ADCU_2k3_ID ? 393216 :     \
        ((A) == ADCU_2k4_ID ? 524288 :     \
	 ((A) == ADCU_2k5_ID ? 131072 :     \
	  ((A) == ADCU_2k6_ID ? 131072 :     \
	   ((A) == DDCU_2k_LSC_ID ? 524288 :     \
	    ((A) == DDCU_2k_ASC_ID ? 262144 :     \
	     ((A) == GDS_2k_LSC_EX_ID ? 262144 :     \
	      ((A) == GDS_2k_ASC_EX_ID ? 131072 :     \
	       ((A) == GDS_2k_LSC_TP_ID ? 524288 :     \
		((A) == GDS_2k_ASC_TP_ID ? 262144 :     \
		 ((A) == ADCU_4k1_ID ? 262144 :     \
		  ((A) == ADCU_4k2_ID ? 131072 :     \
		   ((A) == ADCU_4k3_ID ? 393216 :     \
		    ((A) == ADCU_4k4_ID ? 524288 :     \
		     ((A) == ADCU_4k5_ID ? 131072 :     \
		      ((A) == ADCU_4k6_ID ? 131072 :     \
		       ((A) == DDCU_4k_LSC_ID ? 383216 :     \
			((A) == DDCU_4k_ASC_ID ? 262144 :     \
			 ((A) == GDS_4k_LSC_EX_ID ? 262144 :     \
			  ((A) == GDS_4k_ASC_EX_ID ? 131072 :     \
			   ((A) == GDS_4k_LSC_TP_ID ? 524288 :     \
			    ((A) == GDS_4k_ASC_TP_ID ? 262144 :     \
			    ((A) == 29 ? 0 :     \
			    ((A) == 30 ? 0 :     \
			    ((A) == ADCU_PMC_40_ID ? 655360 :     \
			    ((A) == ADCU_PMC_41_ID ? 655360 :     \
			    ((A) == ADCU_PMC_42_ID ? 655360 :     \
			    ((A) == ADCU_PMC_43_ID ? 655360 :     \
			     -1))))))))))))))))))))))))))))))))))))
       
       
       /* Macro to convert unit id to unit type. */
#define UNIT_ID_TO_UTYPE(A) ( \
((A) == DAQSC_ID ? DAQS_UTYPE_CS : \
 ((A) == CM_ID ? DAQS_UTYPE_CM :     \
  ((A) == FB0_ID ? DAQS_UTYPE_FB :     \
   ((A) == FB1_ID ? DAQS_UTYPE_FB :     \
    ((A) == EDCU_ID ? DAQS_UTYPE_EDCU :     \
     ((A) == ADCU_2k1_ID ? DAQS_UTYPE_ADCU :     \
      ((A) == ADCU_2k2_ID ? DAQS_UTYPE_ADCU :     \
       ((A) == ADCU_2k3_ID ? DAQS_UTYPE_ADCU :     \
        ((A) == ADCU_2k4_ID ? DAQS_UTYPE_ADCU :     \
	 ((A) == ADCU_2k5_ID ? DAQS_UTYPE_ADCU :     \
	  ((A) == ADCU_2k6_ID ? DAQS_UTYPE_ADCU :     \
	   ((A) == DDCU_2k_LSC_ID ? DAQS_UTYPE_DDCU :     \
	    ((A) == DDCU_2k_ASC_ID ? DAQS_UTYPE_DDCU :     \
	     ((A) == GDS_2k_LSC_EX_ID ? DAQS_UTYPE_GDS_EXC :     \
	      ((A) == GDS_2k_ASC_EX_ID ? DAQS_UTYPE_GDS_EXC :     \
	       ((A) == GDS_2k_LSC_TP_ID ? DAQS_UTYPE_GDS_TP :     \
		((A) == GDS_2k_ASC_TP_ID ? DAQS_UTYPE_GDS_TP :     \
		 ((A) == ADCU_4k1_ID ? DAQS_UTYPE_ADCU :     \
		  ((A) == ADCU_4k2_ID ? DAQS_UTYPE_ADCU :     \
		   ((A) == ADCU_4k3_ID ? DAQS_UTYPE_ADCU :     \
		    ((A) == ADCU_4k4_ID ? DAQS_UTYPE_ADCU :     \
		     ((A) == ADCU_4k5_ID ? DAQS_UTYPE_ADCU :     \
		      ((A) == ADCU_4k6_ID ? DAQS_UTYPE_ADCU :     \
		       ((A) == DDCU_4k_LSC_ID ? DAQS_UTYPE_DDCU :     \
			((A) == DDCU_4k_ASC_ID ? DAQS_UTYPE_DDCU :     \
			 ((A) == GDS_4k_LSC_EX_ID ? DAQS_UTYPE_GDS_EXC :     \
			  ((A) == GDS_4k_ASC_EX_ID ? DAQS_UTYPE_GDS_EXC :     \
			   ((A) == GDS_4k_LSC_TP_ID ? DAQS_UTYPE_GDS_TP :     \
			    ((A) == GDS_4k_ASC_TP_ID ? DAQS_UTYPE_GDS_TP :     \
			    ((A) == ADCU_PMC_40_ID ? DAQS_UTYPE_ADCU :     \
			    ((A) == ADCU_PMC_41_ID ? DAQS_UTYPE_ADCU :     \
			    ((A) == ADCU_PMC_42_ID ? DAQS_UTYPE_ADCU :     \
			    ((A) == ADCU_PMC_43_ID ? DAQS_UTYPE_ADCU :     \
			     -1))))))))))))))))))))))))))))))))))
       
       
       /* Macro to convert unit id to unit name . */
#define UNIT_ID_TO_NAME(A) ( \
((A) == DAQSC_ID ? "LHO DAQS Controller" : \
 ((A) == CM_ID ? "LHO DAQS Configuration Manager" :     \
  ((A) == FB0_ID ? "LHO Frame Builder 0" :     \
   ((A) == FB1_ID ? "LHO Frame Builder 1" :     \
    ((A) == EDCU_ID ? "LHO EDCU" :     \
     ((A) == ADCU_2k1_ID ? "LHO 2k ADCU 1"  :     \
      ((A) == ADCU_2k2_ID ? "LHO 2k ADCU 2" :     \
       ((A) == ADCU_2k3_ID ? "LHO 2k ADCU 3" :     \
        ((A) == ADCU_2k4_ID ? "LHO 2k ADCU 4" :     \
	 ((A) == ADCU_2k5_ID ? "LHO 2k ADCU 5" :     \
	  ((A) == ADCU_2k6_ID ? "LHO 2k ADCU 6" :     \
	   ((A) == DDCU_2k_LSC_ID ? "LHO 2k DDCU LSC" :     \
	    ((A) == DDCU_2k_ASC_ID ? "LHO 2k DDCU ASC" :     \
	     ((A) == GDS_2k_LSC_EX_ID ? "LHO 2k GDS LSC Excitation" :     \
	      ((A) == GDS_2k_ASC_EX_ID ? "LHO 2k GDS ALSC Excitation" :     \
	       ((A) == GDS_2k_LSC_TP_ID ? "LHO 2k GDS LSC Test Point" :     \
		((A) == GDS_2k_ASC_TP_ID ? "LHO 2k GDS ASC Test Point" :     \
		 ((A) == ADCU_4k1_ID ? "LHO 4k ADCU 1" :     \
		  ((A) == ADCU_4k2_ID ? "LHO 4k ADCU 2" :     \
		   ((A) == ADCU_4k3_ID ? "LHO 4k ADCU 3" :     \
		    ((A) == ADCU_4k4_ID ? "LHO 4k ADCU 4" :     \
		     ((A) == ADCU_4k5_ID ? "LHO 4k ADCU 5" :     \
		      ((A) == ADCU_4k6_ID ? "LHO 4k ADCU 6" :     \
		       ((A) == DDCU_4k_LSC_ID ? "LHO 4k DDCU LSC" :     \
			((A) == DDCU_4k_ASC_ID ? "LHO 4k DDCU ASC" :     \
			 ((A) == GDS_4k_LSC_EX_ID ?  "LHO 4k GDS LSC Excitation" :     \
			  ((A) == GDS_4k_ASC_EX_ID ? "LHO 4k GDS ASC Excitation" :     \
			   ((A) == GDS_4k_LSC_TP_ID ? "LHO 4k GDS LSC Test Point" :     \
			    ((A) == GDS_4k_ASC_TP_ID ? "LHO 4k GDS ASC Test Point" :     \
			    ((A) == ADCU_PMC_40_ID ? "PMC ADCU 1" :     \
			    ((A) == ADCU_PMC_41_ID ? "PMC ADCU 2" :     \
			    ((A) == ADCU_PMC_42_ID ? "PMC ADCU 3" :     \
			    ((A) == ADCU_PMC_43_ID ? "PMC ADCU 4" :     \
			     "Not Found"))))))))))))))))))))))))))))))))))
       
       
       
       /* Macro to convert unit id to RFM OFFSET to start of IPC. */
#define UNIT_ID_TO_DATA_BLOCK_SIZE(A) ( \
((A) == DAQSC_ID ? 0 : \
 ((A) == CM_ID ? 0 :     \
  ((A) == FB0_ID ? 0 :     \
   ((A) == FB1_ID ? 0:     \
    ((A) == EDCU_ID ? 1 :     \
     ((A) == ADCU_2k1_ID ? DATA_BLOCK_SIZE_256 :     \
      ((A) == ADCU_2k2_ID ? DATA_BLOCK_SIZE_128 :     \
       ((A) == ADCU_2k3_ID ? DATA_BLOCK_SIZE_384 :     \
        ((A) == ADCU_2k4_ID ? DATA_BLOCK_SIZE_512 :     \
	 ((A) == ADCU_2k5_ID ? DATA_BLOCK_SIZE_128 :     \
	  ((A) == ADCU_2k6_ID ? DATA_BLOCK_SIZE_128 :     \
	   ((A) == DDCU_2k_LSC_ID ? DATA_BLOCK_SIZE_512 :     \
	    ((A) == DDCU_2k_ASC_ID ? DATA_BLOCK_SIZE_256 :     \
	     ((A) == GDS_2k_LSC_EX_ID ? DATA_BLOCK_SIZE_256 :     \
	      ((A) == GDS_2k_ASC_EX_ID ? DATA_BLOCK_SIZE_128 :     \
	       ((A) == GDS_2k_LSC_TP_ID ? DATA_BLOCK_SIZE_512 :     \
		((A) == GDS_2k_ASC_TP_ID ? DATA_BLOCK_SIZE_256 :     \
		 ((A) == ADCU_4k1_ID ? DATA_BLOCK_SIZE_256 :     \
		  ((A) == ADCU_4k2_ID ? DATA_BLOCK_SIZE_128 :     \
		   ((A) == ADCU_4k3_ID ? DATA_BLOCK_SIZE_384 :     \
		    ((A) == ADCU_4k4_ID ? DATA_BLOCK_SIZE_512 :     \
		     ((A) == ADCU_4k5_ID ? DATA_BLOCK_SIZE_128 :     \
		      ((A) == ADCU_4k6_ID ? DATA_BLOCK_SIZE_128 :     \
		       ((A) == DDCU_4k_LSC_ID ? DATA_BLOCK_SIZE_512 :     \
			((A) == DDCU_4k_ASC_ID ? DATA_BLOCK_SIZE_256 :     \
			 ((A) == GDS_4k_LSC_EX_ID ? DATA_BLOCK_SIZE_256 :     \
			  ((A) == GDS_4k_ASC_EX_ID ? DATA_BLOCK_SIZE_128 :     \
			   ((A) == GDS_4k_LSC_TP_ID ? DATA_BLOCK_SIZE_512 :     \
			    ((A) == GDS_4k_ASC_TP_ID ? DATA_BLOCK_SIZE_256 :     \
			    ((A) == GDS_MAP_ID ? 0 :     \
			    ((A) == ADCU_PMC_40_ID ? DATA_BLOCK_SIZE_640 :     \
			    ((A) == ADCU_PMC_41_ID ? DATA_BLOCK_SIZE_640 :     \
			    ((A) == ADCU_PMC_42_ID ? DATA_BLOCK_SIZE_640 :     \
			    ((A) == ADCU_PMC_43_ID ? DATA_BLOCK_SIZE_640 :     \
			     -1)))))))))))))))))))))))))))))))))))
       
#ifdef __cplusplus
       }
#endif
#endif /* _DAQ_MAP_H */



