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
#define MAX_ADC_MODULES		4
#define MAX_DAC_MODULES		4
#define MAX_DIO_MODULES		4
#define MAX_RFM_MODULES		2
#define MAX_VME_BRIDGES		4

#define GSC_16AI64SSA		0
#define GSC_16AISS8AO4		1
#define GSC_16AO16		2
#define GSC_18AISS8AO8		3

/* Cards configuration */
typedef struct CDS_CARDS {
	int type;
	int instance;
} CDS_CARDS;

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
	int dioCount;			/* Number of DIO modules found		*/
	unsigned short pci_dio[MAX_DIO_MODULES];	/* io registers of DIO	*/
	int iiroDioCount;	 	/* Number of IIRO-8 isolated DIO modules found */
	unsigned short pci_iiro_dio[MAX_DIO_MODULES];	/* io regs of IIRO mods */
	int iiroDio1Count;	 	/* Number of IIRO-16 isolated DIO modules found */
	unsigned short pci_iiro_dio1[MAX_DIO_MODULES];	/* io regs of IIRO-16 mods */
	int rfmCount;			/* Number of RFM modules found		*/
	long pci_rfm[MAX_RFM_MODULES];	/* Remapped addresses of RFM modules	*/
	int rfmConfig[MAX_RFM_MODULES];
	int vmeBridgeCount;		/* The number of SBS VME bridges attached */
	volatile unsigned int *vme[MAX_VME_BRIDGES]; /* VME memory */
	volatile unsigned char *vme_reg[MAX_VME_BRIDGES]; /* PCI to VME bridge registers */
	unsigned char *buf;

	/* Variables controlling cards usage */
	int cards;			/* Sizeof array below */
	CDS_CARDS *cards_used;		/* Cards configuration */
}CDS_HARDWARE;

/* ACCESS DIO Module Definitions ********************************************** */
#define ACC_VID	0x494F
#define ACC_TID	0x0C51

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
        unsigned int rsv1;      /* 0x1c */
        unsigned int SSC;       /* 0x20 */
        unsigned int rsv2;      /* 0x24 */
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
#define GSAO_FIFO_16            0x1
#define GSAO_FIFO_32            0xA

#define VMIC_VID		0x114a
#define VMIC_TID		0x5565

#define TURN_OFF_5565_FAIL      0x80000000
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


/* GSA 2MS ADC Module Definitions ***************************************************** */
#define FADC_SS_ID       0x3172	/* Subsystem ID to locate module on PCI bus	*/

/* Structure defining ADC module PCI register layout	*/
typedef struct GSA_FAD_REG{
        unsigned int BCR;       /* 0x0 */
        unsigned int DIP;     	/* 0x4 */
        unsigned int AO_00;     /* 0x8 */
        unsigned int AO_01;     /* 0xc */
        unsigned int AO_02;     /* 0x10 */
        //unsigned int AO_03;     /* 0x14 */
        unsigned int IN_CONF;      /* 0x24 */
        unsigned int IN_BUFF;  /* 0x18 */
        unsigned int RAG;      /* 0x1c */
        unsigned int RBG;       /* 0x20 */
        //unsigned int IN_CONF;      /* 0x24 */
        unsigned int AO_03;     /* 0x14 */
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
#define GSAF_DAC_ENABLE_RGC     0x200000
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
