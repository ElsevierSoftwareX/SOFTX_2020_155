/* GSA 18-bit 6 channel ADC Module Definitions
 * ************************************* */
/* GSA 18-bit ADC module 18AISS6C Definitions
 * ***************************************** */
#define ADC_18BIT_SS_ID 0x3467
#define FADC_SS_ID ADC_18BIT_SS_ID

/* Structure defining ADC module PCI register layout    */
typedef struct GSA_FAD_REG {
  unsigned int BCR;   /* 0x0 */
  unsigned int DIP;   /* 0x4 */
  unsigned int AO_00; /* 0x8 */
  unsigned int AO_01; /* 0xc */
  unsigned int AO_02; /* 0x10 */
  // unsigned int AO_03;     /* 0x14 */
  unsigned int IN_CONF; /* 0x14 */
  unsigned int IN_BUFF; /* 0x18 */
  unsigned int RAG;     /* 0x1c */
  unsigned int RBG;     /* 0x20 */
  // unsigned int IN_CONF;      /* 0x24 */
  unsigned int IN_BURST_BLOCK_SIZE; /* 0x24 */
  unsigned int IN_BUF_SIZE;         /* 0x28 */
  unsigned int IN_BUF_TH;           /* 0x2c */
  unsigned int INTRC;               /* 0x30 */
  unsigned int ASSC;                /* 0x34 */
  unsigned int AUTO_CAL;            /* 0x38 */
  unsigned int DAC_BUFF_OPS;        /* 0x3c */
  unsigned int DAC_BUFF_TH;         /* 0x40 */
  unsigned int DAC_BUF_SIZE;        /* 0x44 */
  unsigned int DAC_BUFF;            /* 0x48 */
  unsigned int RGENC;               /* 0x4C */

  // These are for the 18-bit ADC/DAC board only
  unsigned int RGEND;    /* 0x50 */
  unsigned int OUT_CONF; /* 0x54 */
} GSA_FAD_REG;

#define GSAF_FULL_DIFFERENTIAL 0x0
#define GSAF_IN_RANGE_10 0xA0
#define GSAF_RESET 0x80000000
#define GSAF_SET_2S_COMP 0x2000000
#define GSAF_RATEA_32K 0x263
#define GSAF_INPUT_CLK_INIT 0x1000000
#define GSAF_ENABLE_RAG 0x4000000
#define GSAF_ENABLE_INPUT 0x1000
#define GSAF_DMA_LOCAL_ADDR 0x18
#define GSAF_DMA_BYTE_COUNT 24
#define GSAF_DAC_4CHAN 0xF
#define GSAF_DAC_CLK_INIT 0x10
#define GSAF_DAC_ENABLE_CLK 0x20
#define GSAF_DAC_ENABLE_RGC 0x400000
#define GSAF_DAC_SIMULT 0x60000
#define GSAF_ENABLE_BUFF_OUT 0x80000
#define GSAF_DAC_CLR_BUFFER 0x800
#define GSAF_DAC_DMA_LOCAL_ADDR 0x48
#define GSAF_RATEC_1MHZ 0x28
#define GSAF_DATA_CODE_OFFSET 0x20000
#define GSAF_DATA_MASK 0x3ffff;
#define GSAF_CHAN_COUNT 6
#define GSAF_CHAN_COUNT_M1 5
