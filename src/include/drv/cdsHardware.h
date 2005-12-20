#define MAX_ADC_MODULES		4
#define MAX_DAC_MODULES		4
#define MAX_DIO_MODULES		4
typedef struct CDS_HARDWARE{
	int dacCount;
	long pci_dac[MAX_DAC_MODULES];
	int adcCount;
	long pci_adc[MAX_ADC_MODULES];
	int dioCount;
	unsigned short pci_dio[MAX_DIO_MODULES];
}CDS_HARDWARE;

/* ACCESS DIO Module Definitions ***************************************************** */
#define ACC_VID	0x494F
#define ACC_TID	0x0C50
#define DIO_ALL_INPUT	0x9B
#define DIO_A_OUTPUT	0x8B
#define DIO_C_OUTPUT	0x92
#define DIO_A_REG	0x0
#define DIO_B_REG	0x1
#define DIO_C_REG	0x2
#define DIO_CTRL_REG	0x3

/* PLX Chip Definitions for GSA ADC/DAC Modules ************************************** */
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

typedef struct PLX_9056_INTCTRL{
        unsigned int pad[26];
        unsigned int INTCSR;
}PLX_9056_INTCTRL;

#define PLX_VID         0x10b5
#define PLX_TID         0x9056


/* GSA ADC Module Definitions ********************************************************* */
#define ADC_SS_ID       0x3101

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
        unsigned int BCFIG;     /* 0x28 */
        unsigned int AC_VAL;    /* 0x2c */
        unsigned int AUX_RWR;   /* 0x30 */
        unsigned int AUX_SIO;   /* 0x34 */
        unsigned int SMUW;      /* 0x38 */
        unsigned int SMLW;      /* 0x3c */
}GSA_ADC_REG;

#define GSAI_FULL_DIFFERENTIAL  0x200
#define GSAI_64_CHANNEL         0x6
#define GSAI_SOFT_TRIGGER       0x1000
#define GSAI_RESET              0x8000
#define GSAI_DATA_PACKING       0x40000
#define GSAI_DMA_MODE_NO_INTR   0x10943
#define GSAI_DMA_MODE_INTR      0x10D43
#define GSAI_DMA_LOCAL_ADDR     0x8
#define GSAI_DMA_TO_PCI         0xA
#define GSAI_DMA_START          0x3
#define GSAI_DMA_DONE           0x10
#define GSAI_ISR_ON_SAMPLE      0x3
#define PLX_INT_ENABLE          0x900
#define PLX_INT_DISABLE         0x800
#define GSAI_SAMPLE_START       0x10000
#define GSAI_SET_2S_COMP        0x40
#define GSAI_EXTERNAL_SYNC      0x10
#define GSAI_ENABLE_X_SYNC      0x80
#define GSAI_CLEAR_BUFFER       0x40000
#define GSAI_AUTO_CAL           0x2000

/* GSA DAC Module Definitions ********************************************************* */
typedef struct GSA_DAC_REG{
        unsigned int BCR;               /* 0x00 */
        unsigned int CSR;               /* 0x04 */
        unsigned int SAMPLE_RATE;       /* 0x08 */
        unsigned int BOR;               /* 0x0C */
        unsigned int FIRM_OPTS;         /* 0x10 */
        unsigned int AC_VALS;           /* 0x14 */
        short ODB[2];           /* 0x18 */
        unsigned int ADJ_CLK;           /* 0x1C */
}GSA_DAC_REG;

#define DAC_SS_ID               0x3120
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

