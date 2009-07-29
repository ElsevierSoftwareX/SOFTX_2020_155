#ifndef CDS_HARDWARE_H
#define CDS_HARDWARE_H
/*----------------------------------------------------------------------------- */
/*                                                                      	*/
/*                      -------------------                             	*/
/*                                                                      	*/
/*                             LIGO                                     	*/
/*                                                                      	*/
/*        THE LASER INTERFEROMETER GRAVITATIONAL WAVE OBSERVATORY.      	*/
/*                                                                      	*/
/*                     (C) The LIGO Project, 2005.                      	*/
/*                                                                      	*/
/*                                                                      	*/
/* File: cdsHardware.h								*/
/* Description:									*/
/*	Standard header files describing all PCI hardware used in CDS		*/
/*	front end control systems.						*/
/*                                                                      	*/
/* California Institute of Technology                                   	*/
/* LIGO Project MS 18-34                                                	*/
/* Pasadena CA 91125                                                    	*/
/*                                                                      	*/
/* Massachusetts Institute of Technology                                	*/
/* LIGO Project MS 20B-145                                              	*/
/* Cambridge MA 01239                                                   	*/
/*                                                                      	*/
/*----------------------------------------------------------------------------- */


/* Define maximum number of each PCI module supported.				*/
#define MAX_ADC_MODULES		8
#define MAX_DAC_MODULES		8
#define MAX_DIO_MODULES		4
#define MAX_RFM_MODULES		2
#define MAX_VME_BRIDGES		4

#define GSC_16AI64SSA		0
#define GSC_16AISS8AO4		1
#define GSC_16AO16		2
#define GSC_18AISS8AO8		3
#define CON_32DO		4
#define ACS_16DIO		5
#define ACS_8DIO		6
#define GSC_18AI32SSC1M		7
#define GSC_18AO8		8
#define ACS_24DIO		9

/* Cards configuration */
typedef struct CDS_CARDS {
	int type;
	int instance;
} CDS_CARDS;

/* Remote IPC nodes configuration */
typedef struct CDS_REMOTE_NODES {
	char nodename[64]; /* Host name */
	int port;	   /* port number; we can have multiple FEs on same host */
} CDS_REMOTE_NODES;

/* Standard structure to maintain PCI module information.			*/
typedef struct CDS_HARDWARE{
	int dacCount;			/* Number of DAC modules found 		*/
	long pci_dac[MAX_DAC_MODULES];	/* Remapped addresses of DAC modules	*/
	int dacType[MAX_DAC_MODULES];
	int dacConfig[MAX_DAC_MODULES];
	int adcCount;			/* Number of ADC modules found		*/
	long pci_adc[MAX_ADC_MODULES];	/* Remapped addresses of ADC modules	*/
	int adcType[MAX_ADC_MODULES];
	int adcConfig[MAX_ADC_MODULES];
	int doCount;			/* Number of DIO modules found		*/
	unsigned short pci_do[MAX_DIO_MODULES];	/* io registers of DIO	*/
	int doType[MAX_DIO_MODULES];
	int doInstance[MAX_DIO_MODULES];
	int dioCount;			/* Number of DIO modules found		*/
	unsigned short pci_dio[MAX_DIO_MODULES];	/* io registers of DIO	*/
	int iiroDioCount;	 	/* Number of IIRO-8 isolated DIO modules found */
	unsigned short pci_iiro_dio[MAX_DIO_MODULES];	/* io regs of IIRO mods */
	int iiroDio1Count;	 	/* Number of IIRO-16 isolated DIO modules found */
	unsigned short pci_iiro_dio1[MAX_DIO_MODULES];	/* io regs of IIRO-16 mods */
	int cDo32lCount;	 	/* Number of Contec isolated DO modules found */
	unsigned short pci_cdo_dio1[MAX_DIO_MODULES];	/* io regs of Contec 32BO mods */
	int rfmCount;			/* Number of RFM modules found		*/
	long pci_rfm[MAX_RFM_MODULES];	/* Remapped addresses of RFM modules	*/
	int rfmConfig[MAX_RFM_MODULES];
	int rfmType[MAX_RFM_MODULES];
	int vmeBridgeCount;		/* The number of SBS VME bridges attached */
	volatile unsigned int *vme[MAX_VME_BRIDGES]; /* VME memory */
	volatile unsigned char *vme_reg[MAX_VME_BRIDGES]; /* PCI to VME bridge registers */
	unsigned char *buf;
	volatile unsigned int *gps;	/* GPS card */

	/* Variables controlling cards usage */
	int cards;			/* Sizeof array below */
	CDS_CARDS *cards_used;		/* Cards configuration */
}CDS_HARDWARE;

typedef struct IO_MEM_DATA{
	int adcCycle[8][64];
	int dacCycle[8][64];
	int adcVal[8][64][32];
	int dacVal[8][64][32];
	int digOut[8];
}IO_MEM_DATA;

/* ACCESS DIO Module Definitions ********************************************** */
#define ACC_VID		0x494F
#define ACC_TID		0x0C51
#define ACC_TID1	0x0C50

#define DIO_A_OUTPUT	0x8B
#define DIO_C_OUTPUT	0x92
#define DIO_A_REG	0x0
#define DIO_B_REG	0x1
#define DIO_C_REG	0x2
#define DIO_CTRL_REG	0x3

/* ACCESS PCI-IIRO-8 isolated digital input and output */
#define ACC_IIRO_TID 0x0f00
/* ACCESS PCI-IIRO-16 isolated digital input and output (16 channels) */
#define ACC_IIRO_TID1 0x0f08
#define IIRO_DIO_INPUT	0x1
#define IIRO_DIO_OUTPUT 0x0

#define CONTEC_VID	0x1221
#define C_DO_32L_PE	0x86E2

/* PLX Chip Definitions for GSA ADC/DAC Modules ******************************* */
/* Common DMA register definition		*/
typedef struct PLX_9056_DMA{
        unsigned int pad[32];   /* DMA register is at 0x80 offset from base of PLX chip */
        unsigned int DMA0_MODE;         /* 0x80 */
        unsigned int DMA0_PCI_ADD;      /* 0x84 */
        unsigned int DMA0_LOC_ADD;      /* 0x88 */
        unsigned int DMA0_BTC;          /* 0x8C */
        unsigned int DMA0_DESC;         /* 0x90 */
        unsigned int DMA1_MODE;         /* 0x94 */
        unsigned int DMA1_PCI_ADD;      /* 0x98 */
        unsigned int DMA1_LOC_ADD;      /* 0x9C */
        unsigned int DMA1_BTC;          /* 0xA0 */
        unsigned int DMA1_DESC;         /* 0xA4 */
        unsigned int DMA_CSR;           /* 0xA8 */
}PLX_9056_DMA;

/* Struct to point to interrupt control register when using interrupts from ADC	*/
typedef struct PLX_9056_INTCTRL{
        unsigned int pad[26];
        unsigned int INTCSR;
}PLX_9056_INTCTRL;

#define PLX_VID         0x10b5		/* PLX9056 Vendor Id	*/
#define PLX_TID         0x9056		/* PLX9056 Type Id	*/


/* GSA ADC Module Definitions ********************************************************* */
#define ADC_SS_ID       0x3101	/* Subsystem ID to locate module on PCI bus	*/

/* Structure defining ADC module PCI register layout	*/
typedef struct GSA_ADC_REG{
        unsigned int BCR;       /* 0x0 */
        unsigned int INTCR;     /* 0x4 */
        unsigned int IDB;       /* 0x8 */
        unsigned int IDBC;      /* 0xc */
        unsigned int RAG;       /* 0x10 */
        unsigned int RBG;       /* 0x14 */
        unsigned int BUF_SIZE;  /* 0x18 */
        unsigned int BRT_SIZE;      /* 0x1c */
        unsigned int SSC;       /* 0x20 */
        unsigned int ACA;      /* 0x24 */
        unsigned int ASSC;     /* 0x28 */
        unsigned int AC_VAL;    /* 0x2c */
        unsigned int AUX_RWR;   /* 0x30 */
        unsigned int AUX_SIO;   /* 0x34 */
        unsigned int SMUW;      /* 0x38 */
        unsigned int SMLW;      /* 0x3c */
}GSA_ADC_REG;

#define GSAI_FULL_DIFFERENTIAL  0x200
#define GSAI_64_CHANNEL         0x6
#define GSAI_32_CHANNEL         0x5
#define GSAI_8_CHANNEL          0x3
#define GSAI_SOFT_TRIGGER       0x1000
#define GSAI_RESET              0x8000
#define GSAI_DATA_PACKING       0x40000
#define GSAI_DMA_MODE_NO_INTR   0x10943
#define GSAI_DMA_MODE_NO_INTR_DEMAND   0x20943
#define GSAI_DMA_MODE_INTR      0x10D43
#define GSAI_DMA_LOCAL_ADDR     0x8
#define GSAI_DMA_TO_PCI         0xA
#define GSAI_DMA_START          0x3
#define GSAI_DMA1_START          0x300
#define GSAI_DMA_DONE           0x10
#define GSAI_ISR_ON_SAMPLE      0x3
#define PLX_INT_ENABLE          0x900
#define PLX_INT_DISABLE         0x800
#define GSAI_SAMPLE_START       0x10000
#define GSAI_SET_2S_COMP        0x40
#define GSAI_EXTERNAL_SYNC      0x10
#define GSAI_ENABLE_X_SYNC      0x80
#define GSAI_CLEAR_BUFFER       0x40000
#define GSAI_THRESHOLD       	0x001f
#define GSAI_AUTO_CAL           0x2000
#define GSAI_DMA_DEMAND_MODE    0x80000
#define GSAI_18BIT_DATA		0x100000

/* GSA 18-bit DAC Module Defs ********************************************************* */
typedef struct GSA_18BIT_DAC_REG {
        unsigned int BCR;               /* 0x00 */
        unsigned int digital_io_ports;  /* 0x04 */
	unsigned int reserved;
	unsigned int reserved0;
	unsigned int reserved1;
	unsigned int SELFTEST_CONFIG;	/* 0x14 */
	unsigned int SELFTEST_DBUF;	/* 0x18 */
	unsigned int AUX_SYNC_IO_CTRL;	/* 0x1c */
	unsigned int reserved2;
	unsigned int reserved3;
	unsigned int reserved4;
	unsigned int reserved5;
	unsigned int PRIMARY_STATUS;	/* 0x30 */
	unsigned int ASY_CONFIG;	/* 0x34 */
	unsigned int AUTOCAL_VALS;	/* 0x38 */
	unsigned int BUF_OUTPUT_OPS;	/* 0x3C */
	unsigned int OUT_BUF_THRESH;	/* 0x40 */
	unsigned int OUT_BUF_SIZE;	/* 0x44 RO */
	unsigned int OUTPUT_BUF;	/* 0x48 WO */
	unsigned int RATE_GEN_C;	/* 0x4C */
	unsigned int RATE_GEN_D;	/* 0x50 */
	unsigned int OUTPUT_CONFIG;	/* 0x54 */
} GSA_18BIT_DAC_REG;

#define DAC_18BIT_SS_ID		0x3357	/* Subsystem ID to find module on PCI bus	*/
#define GSAO_18BIT_RESET	(1 << 31)
#define GSAO_18BIT_OFFSET_BINARY	(1 << 25)
#define GSAO_18BIT_10VOLT_RANGE	(2 << 8)
#define GSAO_18BIT_EXT_CLOCK_SRC	(2 << 12)
#define GSAO_18BIT_EXT_TRIG_SRC	(2 << 14)
#define GSAO_18BIT_DIFF_OUTS	(1 << 16)
#define GSAO_18BIT_ENABLE_CLOCK (1 << 5)
#define GSAO_18BIT_SIMULT_OUT	(1 << 18)

/* GSA DAC Module Definitions ********************************************************* */
/* Structure defining DAC module PCI register layout	*/
typedef struct GSA_DAC_REG{
        unsigned int BCR;               /* 0x00 */
        unsigned int CSR;               /* 0x04 */
        unsigned int SAMPLE_RATE;       /* 0x08 */
        unsigned int BOR;               /* 0x0C */
        unsigned int ASSC;         	/* 0x10 */
        unsigned int AC_VALS;           /* 0x14 */
        short ODB[2];           /* 0x18 */
        unsigned int ADJ_CLK;           /* 0x1C */
}GSA_DAC_REG;

#define DAC_SS_ID               0x3120	/* Subsystem ID to find module on PCI bus	*/
#define GSAO_RESET              0x8000
#define GSAO_OUT_RANGE_25       0x10000
#define GSAO_OUT_RANGE_05       0x20000
#define GSAO_OUT_RANGE_10       0x30000
#define GSAO_SIMULT_OUT         0x80
#define GSAO_2S_COMP            0x10
#define GSAO_EXTERN_CLK         0x10
#define GSAO_ENABLE_CLK         0x20
#define GSAO_SFT_TRIG           0x80
#define GSAO_CLR_BUFFER         0x800
#define GSAO_FIFO_16            0x1
#define GSAO_FIFO_32            0x2
#define GSAO_FIFO_64            0x3
#define GSAO_FIFO_128           0x4
#define GSAO_FIFO_512           6
#define GSAO_FIFO_1024          7
#define GSAO_FIFO_2048          8

#define VMIC_VID		0x114a
#define VMIC_TID		0x5565
#define VMIC_TID_5579		0x5579

#define TURN_OFF_5565_FAIL      0x80000000
#define TURN_OFF_5579_FAIL	0x80
typedef struct VMIC5565_CSR{
        unsigned char BRV;      /* 0x0 */
        unsigned char BID;      /* 0x1 */
        unsigned char rsv0;     /* 0x2 */
        unsigned char rsv1;     /* 0x3 */
        unsigned char NID;      /* 0x4 */
        unsigned char rsv2;     /* 0x5 */
        unsigned char rsv3;     /* 0x6 */
        unsigned char rsv4;     /* 0x7 */
        unsigned int LCSR1;     /* 0x8 */
        unsigned int rsv5;      /* 0xC */
        unsigned int LISR;      /* 0x10 */
        unsigned int LIER;      /* 0x14 */
        unsigned int NTD;       /* 0x18 */
        unsigned char NTN;      /* 0x1C */
        unsigned char NIC;      /* 0x1D */
        unsigned char rsv15[2]; /* 0x1E */
        unsigned int ISD1;      /* 0x20 */
        unsigned char SID1;     /* 0x24 */
        unsigned char rsv6[3];  /* 0x25 */
        unsigned int ISD2;      /* 0x28 */
        unsigned char SID2;     /* 0x2C */
        unsigned char rsv7[3];  /* 0x2D */
        unsigned int ISD3;      /* 0x30 */
        unsigned char SID3;     /* 0x34 */
        unsigned char rsv8[3];  /* 0x35 */
        unsigned int INITD;     /* 0x38 */
        unsigned char INITN;    /* 0x3C */
        unsigned char rsv9[3];  /* 0x3D */
}VMIC5565_CSR;

typedef struct VMIC5565DMA{
        unsigned int pad[32];
        unsigned int DMA0_MODE;         /* 0x80 */
        unsigned int DMA0_PCI_ADD;      /* 0x84 */
        unsigned int DMA0_LOC_ADD;      /* 0x88 */
        unsigned int DMA0_BTC;          /* 0x8C */
        unsigned int DMA0_DESC;         /* 0x90 */
        unsigned int DMA1_MODE;         /* 0x94 */
        unsigned int DMA1_PCI_ADD;      /* 0x98 */
        unsigned int DMA1_LOC_ADD;      /* 0x9C */
        unsigned int DMA1_BTC;          /* 0xA0 */
        unsigned int DMA1_DESC;         /* 0xA4 */
        unsigned int DMA_CSR;           /* 0xA8 */
        unsigned int DMA_ARB;
        unsigned int DMA_THRESHOLD;
        unsigned int DMA0_PCI_DUAL;
        unsigned int DMA1_PCI_DUAL;
}VMIC5565DMA;

typedef struct VMIC5565RTR{
        unsigned int pad[16];
        unsigned int MBR0;
        unsigned int MBR1;
        unsigned int MBR2;
        unsigned int MBR3;
        unsigned int MBR4;
        unsigned int MBR5;
        unsigned int MBR6;
        unsigned int MBR7;
        unsigned int P2L_DOORBELL;
        unsigned int L2P_DOORBELL;
        unsigned int INTCSR;
}VMIC5565RTR;

struct VMIC5579_MEM_REGISTER {
	unsigned char rsv0;	/* 0x0 */
	unsigned char BID;	/* 0x1 */
	unsigned char rsv2;	/* 0x2 */
	unsigned char rsv3;	/* 0x3 */
	unsigned char NID;	/* 0x4 */
	unsigned char rsv5;	/* 0x5 */
	unsigned char rsv6;	/* 0x6 */
	unsigned char rsv7;	/* 0x7 */
	unsigned char IRS;	/* 0x8 */
	unsigned char CSR1;	/* 0x9 */
	unsigned char rsvA;	/* 0xa */
	unsigned char rsvB;	/* 0xb */
	unsigned char CSR2;	/* 0xc */
	unsigned char CSR3;	/* 0xd */
	unsigned char rsvE;	/* 0xe */
	unsigned char rsvF;	/* 0xf */
	unsigned char CMDND;	/* 0x10 */
	unsigned char CMD;	/* 0x11 */
	unsigned char CDR1;	/* 0x12 */
	unsigned char CDR2;	/* 0x13 */
	unsigned char ICSR;	/* 0x14 */
	unsigned char rsv15;	/* 0x15 */
	unsigned char rsv16;	/* 0x16 */
	unsigned char rsv17;	/* 0x17 */
	unsigned char SID1;	/* 0x18 */
	unsigned char IFR1;	/* 0x19 */
	unsigned short IDR1;	/* 0x1a & 0x1b */
	unsigned char SID2;	/* 0x1c */
	unsigned char IFR2;	/* 0x1d */
	unsigned short IDR2;	/* 0x1e & 0x1f */
	unsigned char SID3;	/* 0x20 */
	unsigned char IFR3;	/* 0x21 */
	unsigned short IDR3;	/* 0x22 & 0x23 */
	unsigned long DADD;	/* 0x24 to 0x27 */
	unsigned char EIS;	/* 0x28 */
	unsigned char ECSR3;	/* 0x29 */
	unsigned char rsv2A;	/* 0x2a */
	unsigned char rsv2B;	/* 0x2b */
	unsigned char rsv2C;	/* 0x2c */
	unsigned char MACR;	/* 0x2d */
	unsigned char rsv2E;	/* 0x2e */
	unsigned char rsv2F;	/* 0x2f */
};

/* GSA 2MS ADC Module Definitions ***************************************************** */
#define FADC_SS_ID       0x3172	/* Subsystem ID to locate module on PCI bus	*/

/* Structure defining ADC module PCI register layout	*/
typedef struct GSA_FAD_REG{
        unsigned int BCR;       /* 0x0 */
        unsigned int DIP;     	/* 0x4 */
        unsigned int AO_00;     /* 0x8 */
        unsigned int AO_01;     /* 0xc */
        unsigned int AO_02;     /* 0x10 */
        unsigned int AO_03;     /* 0x14 */
	// 18-bit ADC/DAC card has IN_CONF at 0x14 (taken care in map.c)
        //unsigned int IN_CONF;      /* 0x14 */
        unsigned int IN_BUFF;  /* 0x18 */
        unsigned int RAG;      /* 0x1c */
        unsigned int RBG;       /* 0x20 */
        unsigned int IN_CONF;      /* 0x24 */
        unsigned int IN_BUF_SIZE;     /* 0x28 */
        unsigned int IN_BUF_TH;    /* 0x2c */
        unsigned int INTRC;   /* 0x30 */
        unsigned int ASSC;   /* 0x34 */
        unsigned int AUTO_CAL;      /* 0x38 */
        unsigned int DAC_BUFF_OPS;      /* 0x3c */
        unsigned int DAC_BUFF_TH;      /* 0x40 */
        unsigned int DAC_BUF_SIZE;      /* 0x44 */
        unsigned int DAC_BUFF;      /* 0x48 */
        unsigned int RGENC;      /* 0x4C */

// These are for the 18-bit ADC/DAC board only
        unsigned int RGEND;      /* 0x50 */
        unsigned int OUT_CONF;      /* 0x54 */
}GSA_FAD_REG;

#define GSAF_FULL_DIFFERENTIAL  0x0
#define GSAF_IN_RANGE_10        0xA0
#define GSAF_RESET              0x80000000
#define GSAF_SET_2S_COMP        0x2000000
#define GSAF_RATEA_32K		0x263
#define GSAF_INPUT_CLK_INIT	0x1000000
#define GSAF_ENABLE_RAG		0x4000000
#define GSAF_ENABLE_INPUT	0x1000
#define GSAF_DMA_LOCAL_ADDR     0x18

#define GSAF_DAC_4CHAN     	0xF
#define GSAF_DAC_CLK_INIT     	0x10
#define GSAF_DAC_ENABLE_CLK     0x20
#define GSAF_DAC_ENABLE_RGC     0x400000
#define GSAF_DAC_SIMULT     	0x60000
#define GSAF_ENABLE_BUFF_OUT    0x80000
#define GSAF_DAC_CLR_BUFFER     0x800
#define GSAF_DAC_DMA_LOCAL_ADDR      0x48
#define GSAF_RATEC_1MHZ     	0x28

/* GSA 18Bit ADC/DAC Module Definitions ***************************************************** */
#define AD18_SS_ID       0x3172	/* Subsystem ID to locate module on PCI bus	*/

/* Structure defining ADC module PCI register layout	*/
typedef struct GSA_AD18_REG{
        unsigned int BCR;       /* 0x0000 	Board Control */
        unsigned int DIO;       /* 0x0004 	Digital IO Port */
        unsigned int RES1;      /* 0x0008 	Reserved */
        unsigned int CLS;       /* 0x000C 	Current Loop Select */
        unsigned int RES2;      /* 0x0010 	Reserved */
        unsigned int AIC;       /* 0x0014 	Analog Input Configuration */
        unsigned int AIB;       /* 0x0018 	Analog Input BUFFER */
        unsigned int RGA;       /* 0x001C 	Rate Generator A */
        unsigned int RGB;       /* 0x0020 	Rate Generator B */
        unsigned int ABS;       /* 0x0024 	AI Burst Block Size */
        unsigned int IBS;       /* 0x0028 	Input Buffer Size */
        unsigned int IBT;       /* 0x002C 	Input Buffer Threshold */
        unsigned int PSF;       /* 0x0030 	Principal status flag */
        unsigned int ASC;       /* 0x0034 	Assembly Configuration */
        unsigned int AVR;       /* 0x0038 	Autocal value readback */
        unsigned int BOO;       /* 0x003C 	Buffered Output Operation */
        unsigned int OBT;       /* 0x0040 	Output Buffer Threshold */
        unsigned int OBS;       /* 0x0044 	Output Buffer Size */
        unsigned int AOB;       /* 0x0048 	Anallog Output Buffer */
        unsigned int RGC;       /* 0x004C 	Rate Generator C */
        unsigned int RGD;       /* 0x004C 	Rate Generator D */
        unsigned int AOC;       /* 0x0050 	Analog Output Configuration */
}GSA_AD18_REG;

/* SBS Technologies VME Bridge Model 618 */
#define SBS_618_VID		0x108a
#define SBS_618_TID		0x0010

/* The I/O space offsets to various interesting registers */
#define SBS_618_LOCAL_COMMAND_REGISTER	0x00
#define SBS_618_LOCAL_STATUS_REGISTER	0x02
#define SBS_618_REMOTE_STATUS_REGISTER	0x08
#define SBS_618_REMOTE_COMMAND_REGISTER	SBS_618_REMOTE_STATUS_REGISTER
#define SBS_618_REMOTE_COMMAND_REGISTER2	0x09
#define SBS_618_REMOTE_VME_ADDRES_MODIFIER	0x0d
#define SBS_618_DMA_COMMAND		0x10
#define SBS_618_DMA_REMAINDER_COUNT	0x11
#define SBS_618_DMA_PACKET_COUNT0	0x12
#define SBS_618_DMA_PACKET_COUNT1	0x13
#define SBS_618_DMA_PCI_ADDRESS0	0x14
#define SBS_618_DMA_PCI_ADDRESS1	0x15
#define SBS_618_DMA_PCI_ADDRESS2	0x16
#define SBS_618_DMA_REMOTE_REMAINDER_COUNT	0x18
#define SBS_618_DMA_VME_ADDRESS2	0x1a
#define SBS_618_DMA_VME_ADDRESS3	0x1b
#define SBS_618_DMA_VME_ADDRESS0	0x1c
#define SBS_618_DMA_VME_ADDRESS1	0x1d

/* The BASE address 2 memory space offsets */
#define SBS_618_MAPPING_REGS_PCI_VME	0x0000
#define SBS_618_MAPPING_REGS_VME_PCI	0x8000
#define SBS_618_MAPPING_REGS_DMA_PCI	0xC000


// Symmertricom GPS input card
// model BC635PCI-U

#define SYMCOM_VID		0x12e2
#define SYMCOM_BC635_TID	0x4013
#define SYMCOM_BC635_TIMEREQ	0
#define SYMCOM_BC635_EVENTREQ	4
#define SYMCOM_BC635_CONTROL	0x10
#define SYMCOM_BC635_TIME0	0x30
#define SYMCOM_BC635_TIME1	0x34
#define SYMCOM_BC635_EVENT0	0x38
#define SYMCOM_BC635_EVENT1	0x3C
#endif
