#define ADC_SS_ID	0x3101

typedef struct GSA_ADC_REG{
	unsigned int BCR;	/* 0x0 */
	unsigned int INTCR;	/* 0x4 */
	unsigned int IDB;	/* 0x8 */
	unsigned int IDBC;	/* 0xc */
	unsigned int RAG;	/* 0x10 */
	unsigned int RBG;	/* 0x14 */
	unsigned int BUF_SIZE;	/* 0x18 */
	unsigned int rsv1;	/* 0x1c */
	unsigned int SSC;	/* 0x20 */
	unsigned int rsv2;	/* 0x24 */
	unsigned int BCFIG;	/* 0x28 */
	unsigned int AC_VAL;	/* 0x2c */
	unsigned int AUX_RWR;	/* 0x30 */
	unsigned int AUX_SIO;	/* 0x34 */
	unsigned int SMUW;	/* 0x38 */
	unsigned int SMLW;	/* 0x3c */
}GSA_ADC_REG;

#define GSAI_FULL_DIFFERENTIAL	0x200
#define GSAI_64_CHANNEL		0x6
#define GSAI_SOFT_TRIGGER	0x1000
#define GSAI_RESET		0x8000
#define GSAI_DATA_PACKING	0x40000
#define GSAI_DMA_MODE_NO_INTR	0x10943
#define GSAI_DMA_MODE_INTR	0x10D43
#define GSAI_DMA_LOCAL_ADDR	0x8
#define GSAI_DMA_TO_PCI		0xA
#define GSAI_DMA_START		0x3
#define GSAI_DMA_DONE		0x10
#define GSAI_ISR_ON_SAMPLE	0x3
#define PLX_INT_ENABLE		0x900
#define PLX_INT_DISABLE		0x800
#define GSAI_SAMPLE_START	0x10000
#define GSAI_SET_2S_COMP	0x40
#define GSAI_EXTERNAL_SYNC	0x10
#define GSAI_ENABLE_X_SYNC	0x80
#define GSAI_CLEAR_BUFFER	0x40000
#define GSAI_AUTO_CAL		0x2000
