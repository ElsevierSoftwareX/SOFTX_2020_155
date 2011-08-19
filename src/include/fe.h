extern int printk(const char *fmt, ...);
#define printf printk
#include "linux/types.h"
#include "inlineMath.h"
#include "drv/cdsHardware.h"
#include FE_HEADER
#include "fm10Gen.h"
extern unsigned int dWordUsed[MAX_ADC_MODULES][32];
extern unsigned int dacOutUsed[MAX_DAC_MODULES][16];
extern unsigned int CDIO6464Output[MAX_DIO_MODULES];
extern unsigned int CDIO1616Output[MAX_DIO_MODULES];
extern unsigned int CDIO1616InputInput[MAX_DIO_MODULES];
extern unsigned int CDIO6464InputInput[MAX_DIO_MODULES];
extern float *testpoint[500];
extern char *_ipc_shm;
extern int clock16K;
extern CDS_HARDWARE cdsPciModules;
extern unsigned int ipcErrBits;
extern unsigned int timeSec;
