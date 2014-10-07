/* GSA 18-bit DAC Module Defs ********************************************************* */
typedef struct GSA_18BIT_DAC_REG {
        unsigned int BCR;               /* 0x00 */
        unsigned int digital_io_ports;  /* 0x04 */
        unsigned int reserved;
        unsigned int reserved0;
        unsigned int reserved1;
        unsigned int SELFTEST_CONFIG;   /* 0x14 */
        unsigned int SELFTEST_DBUF;     /* 0x18 */
        unsigned int AUX_SYNC_IO_CTRL;  /* 0x1c */
        unsigned int reserved2;
        unsigned int reserved3;
        unsigned int reserved4;
        unsigned int reserved5;
        unsigned int PRIMARY_STATUS;    /* 0x30 */
        unsigned int ASY_CONFIG;        /* 0x34 */
        unsigned int AUTOCAL_VALS;      /* 0x38 */
        unsigned int BUF_OUTPUT_OPS;    /* 0x3C */
        unsigned int OUT_BUF_THRESH;    /* 0x40 */
        unsigned int OUT_BUF_SIZE;      /* 0x44 RO */
        unsigned int OUTPUT_BUF;        /* 0x48 WO */
        unsigned int RATE_GEN_C;        /* 0x4C */
        unsigned int RATE_GEN_D;        /* 0x50 */
        unsigned int OUTPUT_CONFIG;     /* 0x54 */
} GSA_18BIT_DAC_REG;

#define DAC_18BIT_SS_ID         0x3357  /* Subsystem ID to find module on PCI bus       */
#define GSAO_18BIT_RESET        (1 << 31)
#define GSAO_18BIT_OFFSET_BINARY        (1 << 25)
#define GSAO_18BIT_10VOLT_RANGE (2 << 8)
#define GSAO_18BIT_EXT_CLOCK_SRC        (2 << 12)
#define GSAO_18BIT_EXT_TRIG_SRC (2 << 14)
#define GSAO_18BIT_DIFF_OUTS    (1 << 16)
#define GSAO_18BIT_ENABLE_CLOCK (1 << 5)
#define GSAO_18BIT_SIMULT_OUT   (1 << 18)
#define GSAO_18BIT_DIO_RW       0x80    // Set first nibble write, second read for Watchdog
#define GSAO_18BIT_PRELOAD      64      // Number of data points to preload DAC FIFO on startup (8 chan x 8 values)
#define GSAO_18BIT_MASK         0x3ffff
#define GSAO_18BIT_CHAN_COUNT   8
#define GSAO_18BIT_DMA_LOCAL_ADDR   0x48
#define GSAO_18BIT_AUTOCAL_SET		(1 << 28)
#define GSAO_18BIT_AUTOCAL_PASS		(1 << 29)
