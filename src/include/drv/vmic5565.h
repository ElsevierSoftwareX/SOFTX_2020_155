#define TURN_OFF_5565_FAIL	0x80000000
#define UINT32 unsigned int

typedef struct VMIC5565_CSR{
	unsigned char BRV;	/* 0x0 */
	unsigned char BID;	/* 0x1 */
	unsigned char rsv0;     /* 0x2 */
	unsigned char rsv1;     /* 0x3 */
	unsigned char NID;      /* 0x4 */
	unsigned char rsv2;     /* 0x5 */
	unsigned char rsv3;     /* 0x6 */
	unsigned char rsv4;     /* 0x7 */
	unsigned int LCSR1;	/* 0x8 */
	unsigned int rsv5;	/* 0xC */
	unsigned int LISR;	/* 0x10 */
	unsigned int LIER;	/* 0x14 */
	unsigned int NTD;	/* 0x18 */
	unsigned char NTN;	/* 0x1C */
	unsigned char NIC;	/* 0x1D */
	unsigned char rsv15[2];	/* 0x1E */
	unsigned int ISD1;	/* 0x20 */
	unsigned char SID1;	/* 0x24 */
	unsigned char rsv6[3];	/* 0x25 */
	unsigned int ISD2;	/* 0x28 */
	unsigned char SID2;	/* 0x2C */
	unsigned char rsv7[3];	/* 0x2D */
	unsigned int ISD3;	/* 0x30 */
	unsigned char SID3;	/* 0x34 */
	unsigned char rsv8[3];	/* 0x35 */
	unsigned int INITD;	/* 0x38 */
	unsigned char INITN;	/* 0x3C */
	unsigned char rsv9[3];	/* 0x3D */
}VMIC5565_CSR;

typedef struct VMIC5565DMA{
	unsigned int pad[32];
        unsigned int DMA0_MODE;		/* 0x80 */
        unsigned int DMA0_PCI_ADD;	/* 0x84 */
        unsigned int DMA0_LOC_ADD;	/* 0x88 */
        unsigned int DMA0_BTC;		/* 0x8C */
        unsigned int DMA0_DESC;		/* 0x90 */
        unsigned int DMA1_MODE;		/* 0x94 */
        unsigned int DMA1_PCI_ADD;	/* 0x98 */
        unsigned int DMA1_LOC_ADD;	/* 0x9C */
        unsigned int DMA1_BTC;		/* 0xA0 */
        unsigned int DMA1_DESC;		/* 0xA4 */
        unsigned int DMA_CSR;		/* 0xA8 */
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

volatile VMIC5565DMA *p5565Dma;
volatile VMIC5565_CSR *p5565Csr;
volatile VMIC5565RTR *p5565Rtr;
dma_addr_t rfmDmaHandle;
char *pRfmMem;
int rfmType;
