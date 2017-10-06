/* GSA DAC Module Definitions ********************************************************* */
/* Structure defining DAC module PCI register layout    */
typedef struct GSC_DAC_REG{
        unsigned int BCR;               /* 0x00 */
        unsigned int CSR;               /* 0x04 */
        unsigned int SAMPLE_RATE;       /* 0x08 */
        unsigned int BOR;               /* 0x0C */
        unsigned int ASSC;              /* 0x10 */
        unsigned int AC_VALS;           /* 0x14 */
        unsigned int ODB;           /* 0x18 */
        unsigned int ADJ_CLK;           /* 0x1C */
}GSC_DAC_REG;

#define DAC_SS_ID               0x3120  /* Subsystem ID to find module on PCI bus       */
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
#define GSAO_FIFO_256           0x5
#define GSAO_FIFO_512           6
#define GSAO_FIFO_1024          7
#define GSAO_FIFO_2048          8
#define GSAO_16BIT_PRELOAD      144     // Number of data points to preload DAC FIFO on startup (16 chan x 9 values)
#define GSAO_16BIT_MASK         0xffff
#define GSAO_16BIT_CHAN_COUNT   16

#ifndef USER_SPACE
volatile PLX_9056_DMA *dacDma[MAX_DAC_MODULES]; /* DMA struct for GSA DAC */
volatile GSC_DAC_REG *dacPtr[MAX_DAC_MODULES];  /* Ptr to DAC registers */
dma_addr_t dac_dma_handle[MAX_DAC_MODULES];     /* PCI add of DAC DMA memory */
#endif


