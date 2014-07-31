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
#define MAX_ADC_MODULES		12	
#define MAX_ADC_CHN_PER_MOD	32	
#define MAX_DAC_MODULES		12
#define MAX_DAC_CHN_PER_MOD	16	
#define MAX_DIO_MODULES		8
#define MAX_RFM_MODULES		2
#define MAX_VME_BRIDGES		4

#define GSC_16AI64SSA		0
#define GSC_18AISS6C		1
#define GSC_16AO16		2
// vacant			3
#define CON_32DO		4
#define ACS_16DIO		5
#define ACS_8DIO		6
#define GSC_18AI32SSC1M		7
#define GSC_18AO8		8
#define ACS_24DIO		9
#define CON_1616DIO		10
#define CON_6464DIO		11
#define CDO64			12
#define CDI64			13

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


#define IO_MEMORY_SLOTS		64
#define IO_MEMORY_SLOT_VALS	32
#define MAX_IO_MODULES		24
#define OVERFLOW_LIMIT_16BIT	32760
#define OVERFLOW_LIMIT_18BIT	131060
#define OVERFLOW_CNTR_LIMIT	0x1000000
#define MAX_ADC_WAIT		1000000		// Max time (usec) to wait for ADC data transfer in iop app
#define MAX_ADC_WAIT_CARD_0	17		// Max time (usec) to wait for 1st ADC card data ready
#define MAX_ADC_WAIT_CARD_S	1 		// Max time (usec) to wait for remianing ADC card data ready
#define MAX_ADC_WAIT_ERR_SEC	3 		// Max number of times ADC time > WAIT per sec before alarm set.
#define MAX_ADC_WAIT_SLAVE	1000		// Max time (usec) to wait for ADC data transfer in slave app
#define DUMMY_ADC_VAL		0xf000000	// Dummy value for test last ADC channel has arrived
#define ADC_1ST_CHAN_MARKER	0xf0000		// Only first ADC channel should have upper bits set as first chan marker.
#define IOP_IO_RATE		65536
#define ADC_DUOTONE_BRD		0
#define ADC_DUOTONE_CHAN	31
#define DAC_DUOTONE_CHAN	30

typedef struct MEM_DATA_BLOCK{
	int timeSec;
	int cycle;
	int data[32];
}MEM_DATA_BLOCK;

typedef struct IO_MEM_DATA{
	int gpsSecond;
	int totalCards;
	int adcCount;
	int dacCount;
	int bioCount;
	int model[MAX_IO_MODULES];
	int ipc[MAX_IO_MODULES];
	int rfmCount;
	long pci_rfm[MAX_RFM_MODULES];	/* Remapped addresses of RFM modules	*/
	long pci_rfm_dma[MAX_RFM_MODULES];	/* Remapped addresses of RFM modules	*/
        int dolphinCount;
        volatile unsigned long *dolphin[2]; /* Read and write addresses to the Dolphin memory */
	MEM_DATA_BLOCK iodata[MAX_IO_MODULES][IO_MEMORY_SLOTS];
	// Combined DAC channels map; used to check on slaves DAC channel allocations
	unsigned int dacOutUsed[MAX_DAC_MODULES][16];
}IO_MEM_DATA;


// Timing control register definitions for use with Contec1616 control of timing slave.

#define TDS_STOP_CLOCKS			0x3700000
#define TDS_START_ADC_NEG_DAC_POS	0x7700000
#define TDS_START_ADC_NEG_DAC_NEG	0x7f00000
#define TDS_NO_ADC_DUOTONE		  0x10000
#define TDS_NO_DAC_DUOTONE		  0x20000

/* Offset of the IO_MEM_DATA structure in the IPC shared memory */
#define IO_MEM_DATA_OFFSET 0x4000

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

#define CONTEC_VID	0x1221
/* 	Isolated Digital Output board for PCI Express */
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
	int cDio1616lCount;	 	/* Number of Contec isolated 16 channel DIO modules found */
	int cDio6464lCount;	 	/* Number of Contec isolated 32 channel DIO modules found */
	unsigned short pci_cdo_dio1[MAX_DIO_MODULES];	/* io regs of Contec 32BO mods */
	int rfmCount;			/* Number of RFM modules found		*/
	long pci_rfm[MAX_RFM_MODULES];	/* Remapped addresses of RFM modules	*/
	long pci_rfm_dma[MAX_RFM_MODULES];	/* Remapped addresses of RFM modules	*/
	int rfmConfig[MAX_RFM_MODULES];
	int rfmType[MAX_RFM_MODULES];
	unsigned char *buf;
	volatile unsigned int *gps;	/* GPS card */
	unsigned int gpsType;
	int gpsOffset;
	int dolphinCount;		/* the number of Dolphin cards we have  on the system */
	volatile unsigned long *dolphin[2]; /* read and write Dolphin memory */

	/* Variables controlling cards usage */
	int cards;			/* Sizeof array below */
	CDS_CARDS *cards_used;		/* Cards configuration */
}CDS_HARDWARE;

#endif
