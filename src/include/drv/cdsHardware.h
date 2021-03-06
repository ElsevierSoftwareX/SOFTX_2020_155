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
#define GSC_20AO8               3
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


#define IO_MEMORY_SLOTS		4096
#define IO_MEMORY_SLOT_VALS	32
#define MAX_IO_MODULES		24
#define OVERFLOW_LIMIT_16BIT	32760
#define OVERFLOW_LIMIT_18BIT	131060
#define OVERFLOW_LIMIT_20BIT    512240
#define OVERFLOW_CNTR_LIMIT	0x1000000
#define MAX_ADC_WAIT		1000000		// Max time (usec) to wait for ADC data transfer in iop app
#define MAX_ADC_WAIT_CARD_0	23		// Max time (usec) to wait for 1st ADC card data ready
#define MAX_ADC_WAIT_C0_32K	36		// Max time (usec) to wait for 1st ADC card data ready on 32K IOP
#define MAX_ADC_WAIT_CARD_S	5 		// Max time (usec) to wait for remaining ADC card data ready
#define MAX_ADC_WAIT_ERR_SEC	3 		// Max number of times ADC time > WAIT per sec before alarm set.
#define MAX_ADC_WAIT_CONTROL	100000		// Max time (usec) to wait for ADC data transfer in control app
#define MAX_ADC_WAIT_CYCLE	17
#define DUMMY_ADC_VAL		0xf000000	// Dummy value for test last ADC channel has arrived
#define ADC_1ST_CHAN_MARKER	0xf0000		// Only first ADC channel should have upper bits set as first chan marker.
#ifdef TEST_1PPS
#define ADC_ONEPPS_BRD		2
#else
#define ADC_ONEPPS_BRD		0
#endif
#define ADC_DUOTONE_BRD		0
#define ADC_DUOTONE_CHAN	31
#define DAC_DUOTONE_CHAN	30
#define DAC_START_CYCLE     4
#define ADC_BUS_DELAY		1
#define ADC_SHORT_CYCLE		2
#define ADC_MAPPED  		1
#define ADC_CHAN_HOP  		2
#define ADC_OVERFLOW  		4
#define ADC_CAL_PASS  		8
#define DAC_CAL_PASS  		0x100000
#define DAC_ACR_MASK  		0x01ffff
#define ADC_RD_TIME  		16


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
	volatile unsigned long *dolphinRead[4]; /* read and write Dolphin memory */
	volatile unsigned long *dolphinWrite[4]; /* read and write Dolphin memory */
	MEM_DATA_BLOCK iodata[MAX_IO_MODULES][IO_MEMORY_SLOTS];
	// Combined DAC channels map; used to check on control app DAC channel allocations
	unsigned int dacOutUsed[MAX_DAC_MODULES][16];
	unsigned int ipcDetect[2][8];
	int card[MAX_IO_MODULES];
}IO_MEM_DATA;

typedef struct IO_MEM_DATA_IOP{
	int gpsSecond;
	int cycleNum;
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
	volatile unsigned long *dolphinRead[4]; /* read and write Dolphin memory */
	volatile unsigned long *dolphinWrite[4]; /* read and write Dolphin memory */
	MEM_DATA_BLOCK iodata[6][65536];
	// Combined DAC channels map; used to check on control app DAC channel allocations
	unsigned int dacOutUsed[MAX_DAC_MODULES][16];
	unsigned int ipcDetect[2][8];
}IO_MEM_DATA_IOP;



// Timing control register definitions for use with Contec1616 control of timing receiver.

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

/* Standard structure to maintain PCI module information.			*/
typedef struct CDS_HARDWARE{
	int dacCount;			/* Number of DAC modules found 		*/
	int dac16Count;			/* Number of DAC modules found 		*/
	int dac18Count;			/* Number of DAC modules found 		*/
	int dac20Count;			/* Number of DAC modules found 		*/
	long pci_dac[MAX_DAC_MODULES];	/* Remapped addresses of DAC modules	*/
	int dacType[MAX_DAC_MODULES];
	int dacInstance[MAX_DAC_MODULES];
	int dacConfig[MAX_DAC_MODULES];
	int dacMap[MAX_DAC_MODULES];
    int dacAcr[MAX_DAC_MODULES];
	int adcCount;			/* Number of ADC modules found		*/
	int adc16Count;			/* Number of 16 bit ADC modules found		*/
	int adc18Count;			/* Number of 18 bit ADC modules found		*/
	long pci_adc[MAX_ADC_MODULES];	/* Remapped addresses of ADC modules	*/
	int adcType[MAX_ADC_MODULES];
	int adcInstance[MAX_ADC_MODULES];
    int adcChannels[MAX_ADC_MODULES];
	int adcConfig[MAX_ADC_MODULES];
	int adcMap[MAX_ADC_MODULES];
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
	volatile unsigned long *dolphinRead[4]; /* read and write Dolphin memory */
	volatile unsigned long *dolphinWrite[4]; /* read and write Dolphin memory */

	/* Variables controlling cards usage */
	int cards;			/* Sizeof array below */
	CDS_CARDS *cards_used;		/* Cards configuration */
}CDS_HARDWARE;

#endif
