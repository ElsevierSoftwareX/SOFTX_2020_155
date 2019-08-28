#define TURN_OFF_5565_FAIL 0x80000000
#define UINT32 unsigned int

#define VMIC_VID 0x114a
#define VMIC_TID 0x5565
#define VMIC_TID_5579 0x5579
#define VMIC_DMA_WRITE 0x0
#define VMIC_DMA_READ 0x8
#define VMIC_DMA_START 0x3
#define VMIC_DMA_CLR 0x8

#define TURN_OFF_5565_FAIL 0x80000000
#define VMIC_5565_REDUNDANT_MODE 0x04000000
#define VMIC_5565_ROGUE_MASTER1 0x02000000
#define VMIC_5565_ROGUE_MASTER0 0x01000000
#define VMIC_5565_MEM_SIZE 0x00100000
#define TURN_OFF_5579_FAIL 0x80
typedef struct VMIC5565_CSR {
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
} VMIC5565_CSR;

typedef struct VMIC5565DMA {
  unsigned int pad[26];
  unsigned int INTCSR;       /* 0x68 */
  unsigned int INTCSR2;      /* 0x6C */
  unsigned int RES1;         /* 0x70 */
  unsigned int PCI_H_REV;    /* 0x74 */
  unsigned int RES2;         /* 0x78 */
  unsigned int RES3;         /* 0x7C */
  unsigned int DMA0_MODE;    /* 0x80 */
  unsigned int DMA0_PCI_ADD; /* 0x84 */
  unsigned int DMA0_LOC_ADD; /* 0x88 */
  unsigned int DMA0_BTC;     /* 0x8C */
  unsigned int DMA0_DESC;    /* 0x90 */
  unsigned int
      DMA1_MODE; /* 0x94 NOTE DMA1 Not supported on latest GEFANUC BOARDS*/
  unsigned int DMA1_PCI_ADD; /* 0x98 */
  unsigned int DMA1_LOC_ADD; /* 0x9C */
  unsigned int DMA1_BTC;     /* 0xA0 */
  unsigned int DMA1_DESC;    /* 0xA4 */
  unsigned int DMA_CSR;      /* 0xA8 */
  unsigned int DMA_ARB;
  unsigned int DMA_THRESHOLD;
  unsigned int DMA0_PCI_DUAL;
  unsigned int DMA1_PCI_DUAL;
} VMIC5565DMA;

typedef struct VMIC5565RTR {
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
} VMIC5565RTR;

volatile VMIC5565_CSR
    *p5565Csr[MAX_RFM_MODULES]; // VMIC5565 Control/Status Registers
volatile VMIC5565DMA *p5565Dma[MAX_RFM_MODULES]; // VMIC5565 DMA Engine
