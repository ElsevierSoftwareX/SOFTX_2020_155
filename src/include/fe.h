extern int printk(const char *fmt, ...);
#define printf printk
#include "linux/types.h"
#include "inlineMath.h"
#include "drv/cdsHardware.h"
#include FE_HEADER
#include "tRamp.h"
#include "fm10Gen.h"
#include "drv/dtoal.c"
extern unsigned int dWordUsed[MAX_ADC_MODULES][32];
extern unsigned int dacOutUsed[MAX_DAC_MODULES][16];
extern unsigned int CDIO6464Output[MAX_DIO_MODULES];
extern unsigned int CDIO1616Output[MAX_DIO_MODULES];
extern unsigned int CDIO1616InputInput[MAX_DIO_MODULES];
extern unsigned int CDIO6464InputInput[MAX_DIO_MODULES];
extern double *testpoint[500];
extern double xExc[50];
extern char *_ipc_shm;
extern int clock16K;
extern unsigned int cycle_gps_time;
extern CDS_HARDWARE cdsPciModules;
extern unsigned int ipcErrBits;
extern unsigned int timeSec;
int rioReadOps[MAX_DIO_MODULES];
int rioOutput[MAX_DIO_MODULES];
int rioInputInput[MAX_DIO_MODULES];
int gainRamp(float gainReq, int rampTime, int id, float *gain, int gainRate);
unsigned int CDO32Output[MAX_DIO_MODULES];

#if defined(SERVO256K)
#define CYCLE_PER_SECOND (256*1024)
#elif defined(SERVO128K)
#define CYCLE_PER_SECOND (128*1024)
#elif defined(SERVO64K)
#define CYCLE_PER_SECOND (64*1024)
#elif defined(SERVO32K)
#define CYCLE_PER_SECOND (32*1024)
#elif defined(SERVO16K)
#define CYCLE_PER_SECOND (16*1024)
#elif defined(SERVO4K)
#define CYCLE_PER_SECOND (4*1024)
#elif defined(SERVO2K)
#define CYCLE_PER_SECOND (2*1024)
#endif

