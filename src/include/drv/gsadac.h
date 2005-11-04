typedef struct GSA_DAC_REG{
	unsigned int BCR;		/* 0x00 */
	unsigned int CSR;		/* 0x04 */
	unsigned int SAMPLE_RATE;	/* 0x08 */
	unsigned int BOR;		/* 0x0C */
	unsigned int FIRM_OPTS;		/* 0x10 */
	unsigned int AC_VALS;		/* 0x14 */
	short ODB[2];		/* 0x18 */
	unsigned int ADJ_CLK;		/* 0x1C */
}GSA_DAC_REG;

#define DAC_SS_ID		0x3120
#define GSAO_RESET              0x8000
#define GSAO_OUT_RANGE_25	0x10000
#define GSAO_OUT_RANGE_05	0x20000
#define GSAO_OUT_RANGE_10	0x30000
#define GSAO_SIMULT_OUT		0x80
#define GSAO_2S_COMP		0x10
#define GSAO_EXTERN_CLK		0x10
#define GSAO_ENABLE_CLK		0x20
#define GSAO_SFT_TRIG		0x80
#define GSAO_FIFO_16		0x1
