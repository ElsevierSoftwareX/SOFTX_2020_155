/*----------------------------------------------------------------------*/
/*                                                                      */
/*                      -------------------                             */
/*                                                                      */
/*                             LIGO                                     */
/*                                                                      */
/*        THE LASER INTERFEROMETER GRAVITATIONAL WAVE OBSERVATORY.      */
/*                                                                      */
/*                     (C) The LIGO Project, 2005.                      */
/*                                                                      */
/*                                                                      */
/*                                                                      */
/* California Institute of Technology                                   */
/* LIGO Project MS 18-34                                                */
/* Pasadena CA 91125                                                    */
/*                                                                      */
/* Massachusetts Institute of Technology                                */
/* LIGO Project MS 20B-145                                              */
/* Cambridge MA 01239                                                   */
/*                                                                      */
/*----------------------------------------------------------------------*/

#ifdef NO_RTL
#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <asm/delay.h>
#else
#include <rtl_time.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <string.h>
#endif
#include <linux/slab.h>
#include <drv/cdsHardware.h>
#include "inlineMath.h"
#include "feSelectHeader.h"

#if MX_KERNEL == 1
#include <myriexpress.h>
#endif

#ifdef DOLPHIN_TEST
#include <asm/io.h>
#include <genif.h>

sci_l_segment_handle_t segment;
sci_map_handle_t client_map_handle;
sci_r_segment_handle_t  remote_segment_handle;

static int client = 0;
static int target_node = 0;
double *dolphin_memory = 0;

                signed32 func(void IN *arg,
                        sci_r_segment_handle_t IN remote_segment_handle,
                        unsigned32 IN reason,
                        unsigned32 IN status) {
                                printk("Connect callback %d\n", reason);
                                return 0;
                }

        	signed32 server_func(void IN *arg,
                       sci_l_segment_handle_t IN local_segment_handle,
                       unsigned32 IN reason,
                       unsigned32 IN source_node,
                       unsigned32 IN local_adapter_number)  {
                                printk("Connect callback %d\n", reason);
                                return 0;
        	}
#endif

#ifndef NUM_SYSTEMS
#define NUM_SYSTEMS 1
#endif

#define INLINE  inline
#define MMAP_SIZE (64*1024*1024 - 5000)
char *_epics_shm;		// Ptr to EPICS shared memory
char *_ipc_shm;			// Ptr to inter-process communication area 
#if defined(SHMEM_DAQ)
char *_daq_shm;			// Ptr to frame builder comm shared mem area 
int daq_fd;			// File descriptor to share memory file 
#endif

long daqBuffer;			// Address for daq dual buffers in daqLib.c
#ifndef NO_RTL
sem_t irqsem;			// Semaphore if in IRQ mode.
#endif
CDS_HARDWARE cdsPciModules;	// Structure of PCI hardware addresses
volatile IO_MEM_DATA *ioMemData;
volatile int vmeDone = 0;	// Code kill command
volatile int stop_working_threads = 0;

#ifdef NO_RTL
#define printf printk
#define STACK_SIZE    40000
#define TICK_PERIOD   100000
#define PERIOD_COUNT  1

#else
#include "msr.h"
#endif

#include "fm10Gen.h"		// CDS filter module defs and C code
#include "feComms.h"		// Lvea control RFM network defs.
#include "daqmap.h"		// DAQ network layout
extern unsigned int cpu_khz;
#define CPURATE	(cpu_khz/1000)
#define ONE_PPS_THRESH 2000
#define SYNC_SRC_NONE		0
#define SYNC_SRC_IRIG_B		1
#define SYNC_SRC_1PPS		2
#define SYNC_SRC_TDS		4
#define SYNC_SRC_MASTER		8
#define TIME_ERR_IRIGB		0x10
#define TIME_ERR_1PPS		0x20
#define TIME_ERR_TDS		0x40

#ifndef NO_DAQ
#include "drv/gmnet.h"
#include "drv/fb.h"
#include "drv/myri.h"		// Myricom network defs
#include "drv/daqLib.c"		// DAQ/GDS connection software
#endif
#include "drv/map.h"		// PCI hardware defs
#include "drv/epicsXfer.c"	// User defined EPICS to/from FE data transfer function

// Define standard values based on code rep rate **************************************
#ifdef SERVO256K
        #define CYCLE_PER_SECOND        (2*131072)
        #define CYCLE_PER_MINUTE        (2*7864320)
        #define DAQ_CYCLE_CHANGE        (2*8000)
        #define END_OF_DAQ_BLOCK        (2*8191)
        #define DAQ_RATE                (2*8192)
        #define NET_SEND_WAIT           (2*655360)
        #define CYCLE_TIME_ALRM         4
#endif
#ifdef SERVO128K
        #define CYCLE_PER_SECOND        131072
        #define CYCLE_PER_MINUTE        7864320
        #define DAQ_CYCLE_CHANGE        8000
        #define END_OF_DAQ_BLOCK        8191
        #define DAQ_RATE                8192
        #define NET_SEND_WAIT           655360
        #define CYCLE_TIME_ALRM         7
#endif
#ifdef SERVO64K
	#define CYCLE_PER_SECOND	(2*32768)
	#define CYCLE_PER_MINUTE	(2*1966080)
	#define DAQ_CYCLE_CHANGE	(2*1540)
	#define END_OF_DAQ_BLOCK	4095
	#define DAQ_RATE	(DAQ_16K_SAMPLE_SIZE*4)
	#define NET_SEND_WAIT		(2*81920)
	#define CYCLE_TIME_ALRM		15
	#define CYCLE_TIME_ALRM_HI	25
	#define CYCLE_TIME_ALRM_LO	10
	#define DAC_PRELOAD_CNT		0
#endif
#ifdef SERVO32K
	#define CYCLE_PER_SECOND	32768
	#define CYCLE_PER_MINUTE	1966080
	#define DAQ_CYCLE_CHANGE	1540
	#define END_OF_DAQ_BLOCK	2047
	#define DAQ_RATE	(DAQ_16K_SAMPLE_SIZE*2)
	#define NET_SEND_WAIT		(2*81920)
	#define CYCLE_TIME_ALRM_HI	38
	#define CYCLE_TIME_ALRM_LO	25
	#define DAC_PRELOAD_CNT		0
#endif

#ifdef SERVO16K
	#define CYCLE_PER_SECOND	16384
	#define CYCLE_PER_MINUTE	983040
	#define DAQ_CYCLE_CHANGE	770
	#define END_OF_DAQ_BLOCK	1023
	#define DAQ_RATE	DAQ_16K_SAMPLE_SIZE
	#define NET_SEND_WAIT		81920
	#define CYCLE_TIME_ALRM_HI	70
	#define CYCLE_TIME_ALRM_LO	50
	#define DAC_PRELOAD_CNT		3
#endif
#ifdef SERVO4K
        #define CYCLE_PER_SECOND        4096
        #define CYCLE_PER_MINUTE        2*122880
        #define DAQ_CYCLE_CHANGE        240
        #define END_OF_DAQ_BLOCK        255
        #define DAQ_RATE        2*DAQ_2K_SAMPLE_SIZE
        #define NET_SEND_WAIT           2*10240
        #define CYCLE_TIME_ALRM         487/2
        #define CYCLE_TIME_ALRM_HI      500/2
        #define CYCLE_TIME_ALRM_LO      460/2
        #define DAC_PRELOAD_CNT         8       
#endif
#ifdef SERVO2K
	#define CYCLE_PER_SECOND	2048
	#define CYCLE_PER_MINUTE	122880
	#define DAQ_CYCLE_CHANGE	120
	#define END_OF_DAQ_BLOCK	127
	#define DAQ_RATE	DAQ_2K_SAMPLE_SIZE
	#define NET_SEND_WAIT		10240
	#define CYCLE_TIME_ALRM		487
	#define CYCLE_TIME_ALRM_HI	500
	#define CYCLE_TIME_ALRM_LO	460
	#define DAC_PRELOAD_CNT		16	
#endif

#ifndef NO_DAQ
DAQ_RANGE daq;			// Range settings for daqLib.c
int numFb = 0;
int fbStat[2] = {0,0};		// Status of DAQ backend computer
#endif

#ifdef NO_RTL
;
#else
rtl_pthread_t wthread;
rtl_pthread_t wthread1;
rtl_pthread_t wthread2;
#endif
int wfd, ipc_fd;

/* ADC/DAC overflow variables */
int overflowAdc[MAX_ADC_MODULES][32];;
int overflowDac[MAX_DAC_MODULES][16];;
int overflowAcc = 0;

float *testpoint[50];	// Testpoints which are not part of filter modules


CDS_EPICS *pLocalEpics;   	// Local mem ptr to EPICS control data


// 1/16 sec cycle counters for DAQS and ISC RFM IPC
int subcycle = 0;		// Internal cycle counter	 
unsigned int daqCycle;		// DAQS cycle counter	

int firstTime;			// Dummy var for startup sync.

FILT_MOD dsp[NUM_SYSTEMS];			// SFM structure.	
FILT_MOD *dspPtr[NUM_SYSTEMS];			// SFM structure pointer.
FILT_MOD *pDsp[NUM_SYSTEMS];			// Ptr to SFM in shmem.	
COEF dspCoeff[NUM_SYSTEMS];			// Local mem for SFM coeffs.
VME_COEF *pCoeff[NUM_SYSTEMS];			// Ptr to SFM coeffs in shmem	
double dWord[MAX_ADC_MODULES][32];		// ADC read values
unsigned int dWordUsed[MAX_ADC_MODULES][32];	// ADC chans used by app code
double dacOut[MAX_DAC_MODULES][16];		// DAC output values
int dacOutEpics[MAX_DAC_MODULES][16];		// DAC outputs reported back to EPICS
unsigned int dacOutUsed[MAX_DAC_MODULES][16];	// DAC chans used by app code
int dioInput[MAX_DIO_MODULES];			// BIO card inputs
int dioOutput[MAX_DIO_MODULES];			// BIO card outputs
int dioOutputHold[MAX_DIO_MODULES];			// BIO card outputs

// Relay digital I/O cards read operations
// 0 - do not read card registers
// 1 - read digital inputs
// 2 - read digital outputs
// 3 - both
int rioReadOps[MAX_DIO_MODULES];
// Values read from relay digital I/O cards, current input values
int rioInputInput[MAX_DIO_MODULES];
// Values read from relay digital I/O cards, current output values
int rioInputOutput[MAX_DIO_MODULES];

int rioOutput[MAX_DIO_MODULES];
int rioOutputHold[MAX_DIO_MODULES];
int rioInput1[MAX_DIO_MODULES];
int rioOutput1[MAX_DIO_MODULES];
int rioOutputHold1[MAX_DIO_MODULES];
unsigned int CDO32Input[MAX_DIO_MODULES];
unsigned int CDO32Output[MAX_DIO_MODULES];
unsigned int CDIO1616InputInput[MAX_DIO_MODULES]; // Binary input bits
unsigned int CDIO1616Input[MAX_DIO_MODULES]; // Current value of the BO bits
unsigned int CDIO1616Output[MAX_DIO_MODULES]; // Binary output bits
unsigned long CDIO6464InputInput[MAX_DIO_MODULES]; // Binary input bits
unsigned long CDIO6464Input[MAX_DIO_MODULES]; // Current value of the BO bits
unsigned long CDIO6464Output[MAX_DIO_MODULES]; // Binary output bits
unsigned long writeCDIO6464l(CDS_HARDWARE *pHardware, int modNum, unsigned long data);
unsigned long readCDIO6464l(CDS_HARDWARE *pHardware, int modNum);
unsigned long readInputCDIO6464l(CDS_HARDWARE *pHardware, int modNum);
int clock16K = 0;
int out_buf_size = 0; // test checking DAC buffer size
double cycle_gps_time = 0.; // Time at which ADCs triggered
double cycle_gps_event_time = 0.; // Time at which ADCs triggered
unsigned int   cycle_gps_ns = 0;
unsigned int   cycle_gps_event_ns = 0;
unsigned int   gps_receiver_locked = 0; // Lock/unlock flag for GPS time card
#if defined(SHMEM_DAQ)
struct rmIpcStr *daqPtr;
#endif

double getGpsTime(unsigned int *);
#include "./feSelectCode.c"
#ifdef NO_RTL
#include "map.c"
#include "fb.c"
#endif

char daqArea[2*DAQ_DCU_SIZE];		// Space allocation for daqLib buffers
int cpuId = 1;


#ifdef OVERSAMPLE

/* Oversamping base rate is 64K */
/* Coeffs for the 2x downsampling (32K system) filter */
static double feCoeff2x[9] =
        {0.053628649721183,
        -1.25687596603711,    0.57946661417301,    0.00000415782507,    1.00000000000000,
        -0.79382359542546,    0.88797791037820,    1.29081406322442,    1.00000000000000};
/* Coeffs for the 4x downsampling (16K system) filter */
static double feCoeff4x[9] =
	{0.014805052402446,  
	-1.71662585474518,    0.78495484219691,   -1.41346289716898,   0.99893884152400,
	-1.68385964238855,    0.93734519457266,    0.00000127375260,   0.99819981588176};

#if 0
/* Coeffs for the 32x downsampling filter (2K system) */
/* Original Rana coeffs from 40m lab elog */
static double feCoeff32x[9] =
        {0.0001104130574447,
        -1.97018349613882,    0.97126719875540,   -1.99025960812101,    0.99988962634797,
        -1.98715023886379,    0.99107485707332,    2.00000000000000,    1.00000000000000};
#endif

/* Coeffs for the 32x downsampling filter (2K system) per Brian Lantz May 5, 2009 */
static double feCoeff32x[9] =
	{0.010581064947739,
        -1.90444302586137,    0.91078434629894,   -1.96090276933603,    0.99931924465090,
        -1.92390910024681,    0.93366146580083,   -1.84652529182276,    0.99866506867980};


// History buffers for oversampling filters
double dHistory[96][40];
double dDacHistory[96][40];

#else

#define OVERSAMPLE_TIMES 1
#endif

#define ADC_SAMPLE_COUNT	0x20
#define ADC_DMA_BYTES		0x80

// Whether run on internal timer (when no ADC cards found)
int run_on_timer = 0;

// Initial diag reset flag
int initialDiagReset = 1;

// DIAGNOSTIC_RETURNS_FROM_FE 
#define FE_NO_ERROR		0x0
#define FE_SYNC_ERR		0x1
#define FE_ADC_HOLD_ERR		0x2
#define FE_FB0_NOT_ONLINE	0x4
#define FE_PROC_TIME_ERR	0x8
#define FE_ADC_SYNC_ERR		0xf0
#define FE_FB_AVAIL		0x1
#define FE_FB_ONLINE		0x2
#define FE_MAX_FB_QUE		0x10


// Function to read time from Symmetricom IRIG-B Module ***********************
static double getGpsTime1(unsigned int *ns, int event_flag) {
  double the_time = 0.0;

  if (cdsPciModules.gps) {
    // Sample time
    if (event_flag) {
      //cdsPciModules.gps[1] = 1;
    } else {
      cdsPciModules.gps[0] = 1;
    }
    // Read seconds, microseconds, nanoseconds
    unsigned int time0;
    unsigned int time1;
    if (event_flag) {
      time0 = cdsPciModules.gps[SYMCOM_BC635_EVENT0/4];
      time1 = cdsPciModules.gps[SYMCOM_BC635_EVENT1/4];
    } else {
      time0 = cdsPciModules.gps[SYMCOM_BC635_TIME0/4];
      time1 = cdsPciModules.gps[SYMCOM_BC635_TIME1/4];
    }
    gps_receiver_locked = !(time0 & (1<<24));
    unsigned  int msecs = 0xfffff & time0; // microseconds
    unsigned  int nsecs = 0xf & (time0 >> 20); // nsecs * 100
    the_time = ((double) time1) + .000001 * (double) msecs  /* + .0000001 * (double) nsecs*/;
    if (ns) {
	*ns = 100 * nsecs; // Store nanoseconds
    }
  }
  printf("%f\n", the_time);
  return the_time;
}

//***********************************************************************
// Get current GPS time from TSYNC IRIG-B Rcvr
int  getGpsTimeTsync(unsigned int *tsyncSec, unsigned int *tsyncUsec) {
  TSYNC_REGISTER *timeRead;
  unsigned int timeSec,timeNsec,sync;
  
  if (cdsPciModules.gps) {
	timeRead = (TSYNC_REGISTER *)cdsPciModules.gps;
	timeSec = timeRead->BCD_SEC;
	timeSec -= 315964819;
	*tsyncSec = timeSec;
	timeNsec = timeRead->SUB_SEC;
	*tsyncUsec = ((timeNsec & 0xfffffff) * 5) / 1000; 
	sync = ((timeNsec >> 31) & 0x1) + 1;
  	return(sync);
  }
  return(0);
}

//***********************************************************************
// Get current GPS time from SYMCOM IRIG-B Rcvr
// Nanoseconds (hundreds at most) are available as an optional ns parameter
double getGpsTime(unsigned int *ns) {
  if(cdsPciModules.gpsType == SYMCOM_RCVR) {
	return getGpsTime1(ns, 0);
  }
  return(0);
}

//***********************************************************************
// Get current EVENT time off the card
// Nanoseconds (hundreds at most) are available as an optional ns parameter
double getGpsEventTime(unsigned int *ns) {
	return getGpsTime1(ns, 1);
}

//***********************************************************************
// Get Gps card microseconds from SYMCOM IRIG-B Rcvr
unsigned int getGpsUsec() {

  if (cdsPciModules.gps) {
    // Sample time
    cdsPciModules.gps[0] = 1;
    // Read microseconds, nanoseconds
    unsigned int time0 = cdsPciModules.gps[SYMCOM_BC635_TIME0/4];
    unsigned int time1 = cdsPciModules.gps[SYMCOM_BC635_TIME1/4];
    return 0xfffff & time0; // microseconds
  }
  return 0;
}
//***********************************************************************
// Get Gps card microseconds from SYMCOM IRIG-B Rcvr
#ifdef DUOTONE_TIMING
float duotime(int count, float meanVal, float data[])
{
  float x,y,sumX,sumY,sumXX,sumXY,msumX;
  int ii;
  float xInc;
  float offset,slope,answer;
  float den;

  x = 0;
  sumX = 0;
  sumY = 0;
  sumXX = 0;
  sumXY= 0;
  xInc = 1000000/CYCLE_PER_SECOND;


  for(ii=0;ii<count;ii++)
  {
        y = data[ii];
        sumX += x;
        sumY += y;
        sumXX += x * x;
        sumXY += x * y;
        x += xInc;

  }
        msumX = sumX * -1;
        den = (count*sumXX-sumX*sumX);
        if(den == 0.0)
        {
                // printf("den error\n");
                return(-1000);
        }
        offset = (msumX*sumXY+sumXX*sumY)/den;
        slope = (msumX*sumY+count*sumXY)/den;
        if(slope == 0.0)
        {
                // printf("slope error\n");
                 return(-1000);
        }
        meanVal -= offset;
        answer = meanVal/slope - 19.7;
#ifdef DUOTONE_DAC
        answer-= 42.5;
#endif
        return(answer);

}
#endif


//***********************************************************************
// TASK: fe_start()	
// This routine is the skeleton for all front end code	
//***********************************************************************
void *fe_start(void *arg)
{

  int ii,jj,kk,ll,mm;			// Dummy loop counter variables
  static int clock1Min = 0;		// Minute counter (Not Used??)
  static int cpuClock[10];		// Code timing diag variables
  int adcData[MAX_ADC_MODULES][32];	// ADC raw data
  volatile int *packedData;		// Pointer to ADC PCI data space
  volatile unsigned int *pDacData;	// Pointer to DAC PCI data space
  RFM_FE_COMMS *pEpicsComms;		// Pointer to EPICS shared memory space
  int cycleTime;			// Max code cycle time within 1 sec period
  int timeHold = 0;
  int timeHoldMax = 0;			// Max code cycle time since last diag reset
  int myGmError2 = 0;			// Myrinet error variable
  int attemptingReconnect = 0;		// Myrinet reconnect status
  int status;				// Typical function return value
  float onePps;				// Value of 1PPS signal, if used, for diagnostics
  int onePpsHi = 0;			// One PPS diagnostic check
  int onePpsTime = 0;			// One PPS diagnostic check
  int dcuId;				// DAQ ID number for this process
  static int adcTime;			// Used in code cycle timing
  static int adcHoldTime;		// Stores time between code cycles
  static int adcHoldTimeMax;		// Stores time between code cycles
  static int usrTime;			// Time spent in user app code
  static int usrHoldTime;		// Max time spent in user app code
  static int missedCycle = 0;		// Incremented error counter when too many values in ADC FIFO
  int netRetry;				// Myrinet reconnect variable
  float xExc[50];			// GDS EXC not associated with filter modules
  int diagWord = 0;			// Code diagnostic bit pattern returned to EPICS
  int timeDiag = 0;			// GPS seconds, passed to EPICS
  int epicsCycle = 0;
  int system = 0;
  double dac_in =  0.0;			// DAC value after upsample filtering
  int dac_out = 0;			// Integer value sent to DAC FIFO
  int sampleCount = 1;			// Number of ADC samples to take per code cycle
  int sync21pps = 0;			// Code startup sync to 1PPS flag
  int sync21ppsCycles = 0;		// Number of attempts to sync to 1PPS
  int limit = 32000;                    // ADC/DAC overflow test value
  int offset = 0; //0x8000;
  int mask = 0xffff;                    // Bit mask for ADC/DAC read/writes
  int num_outs = 16;                    // Number of DAC channels variable
  int syncSource = SYNC_SRC_NONE;	// Code startup synchronization source
  int mxStat = 0;			// Net diags when myrinet express is used
  int mxDiag = 0;
  int mxDiagR = 0;
  unsigned int ns;			// GPS time from IRIG-B
  double time;	
  unsigned int timeSec,usec;
  int wtmin,wtmax;			// Time window for startup on IRIG-B
// ****** Share data
  int ioClock = 0;
  int ioClockDac = DAC_PRELOAD_CNT;
  int ioMemCntr = 0;
  int ioMemCntrDac = DAC_PRELOAD_CNT;
  int memCtr = 0;
  int dacEnable = 0;

  static int dacWriteEnable = 0;
  struct timespec next;

#ifdef DUOTONE_TIMING
  static float duotone[65536];
  float duotoneTime;
  static float duotoneTotal = 0.0;
  static float duotoneMean = 0.0;
#endif



#ifdef NO_RTL
  void printf(){}
#endif

// Do all of the initalization

  /* Init comms with EPICS processor */
  pEpicsComms = (RFM_FE_COMMS *)_epics_shm;
  pLocalEpics = (CDS_EPICS *)&pEpicsComms->epicsSpace;

#ifdef OVERSAMPLE
  // Zero out filter histories
  memset(dHistory, 0, sizeof(dHistory));
  memset(dDacHistory, 0, sizeof(dDacHistory));
#endif

  // Zero out DAC outputs
  for (ii = 0; ii < MAX_DAC_MODULES; ii++)
    for (jj = 0; jj < 16; jj++) {
 	dacOut[ii][jj] = 0.0;
 	dacOutUsed[ii][jj] = 0;
    }

  // Set pointers to filter module data buffers
#if NUM_SYSTEMS > 1
  for (system = 0; system < NUM_SYSTEMS; system++) {
    pDsp[system] = (FILT_MOD *)(&pEpicsComms->dspSpace[system]);
    pCoeff[system] = (VME_COEF *)(&pEpicsComms->coeffSpace[system]);
    dspPtr[system] = dsp + system;
  }
#else
    pDsp[system] = (FILT_MOD *)(&pEpicsComms->dspSpace);
    pCoeff[system] = (VME_COEF *)(&pEpicsComms->coeffSpace);
    dspPtr[system] = dsp;
#endif

  // Clear the FE reset which comes from Epics
  pLocalEpics->epicsInput.vmeReset = 0;

#ifndef ADC_SLAVE
  // if Contec 1616 BIO present, TDS slave will be used for timing.
  if(cdsPciModules.cDio1616lCount) syncSource = SYNC_SRC_TDS;
  // if IRIG-B card present, code will use it for startup synchonization
  if(cdsPciModules.gpsType && (syncSource == SYNC_SRC_NONE)) 
	syncSource = SYNC_SRC_IRIG_B;
  	// If no IRIG-B card, will try 1PPS on ADC[0][31] later
  if(syncSource == SYNC_SRC_NONE) 
	syncSource = SYNC_SRC_1PPS;
#else
   syncSource = SYNC_SRC_MASTER;
#endif

printf("Sync source = %d\n",syncSource);


  // Do not proceed until EPICS has had a BURT restore *******************************
#ifdef NO_RTL
  printf("Waiting for EPICS BURT Restore = %d\n", pLocalEpics->epicsInput.burtRestore);
  int cnt = 0;
#else
  if(cdsPciModules.gpsType == SYMCOM_RCVR) 
  {
  	time = getGpsTime(&ns);
        printf("Found SYMCOM IRIG-B - Time is %f and %d ns 0x%x\n", time, ns);
  }
  if(cdsPciModules.gpsType == TSYNC_RCVR) 
  {
  	gps_receiver_locked = getGpsTimeTsync(&timeSec,&usec);
        printf("Found TSYNC IRIG-B - Time is %d and %d us 0x%x\n", timeSec, usec);
  }
  printf("Waiting for EPICS BURT \n");
#endif
  do{
#ifdef NO_RTL
	udelay(20000);
        udelay(20000);
        udelay(20000);
  	printf("Waiting for EPICS BURT %d\n", cnt++);
#else
	usleep(1000000);
#endif
  }while(!pLocalEpics->epicsInput.burtRestore);

  printf("BURT Restore Complete\n");
// BURT has completed *******************************************************************


// Read in all EPICS settings
  for (system = 0; system < NUM_SYSTEMS; system++)
  {
    for(ii=0;ii<END_OF_DAQ_BLOCK;ii++)
    {
	updateEpics(ii, dspPtr[system], pDsp[system],
		    &dspCoeff[system], pCoeff[system]);
    }
  }

  // Need this FE dcuId to make connection to FB
  dcuId = pLocalEpics->epicsInput.dcuId;

  // Reset timing diagnostics
  pLocalEpics->epicsOutput.diagWord = 0;
  pLocalEpics->epicsOutput.timeDiag = 0;
  pLocalEpics->epicsOutput.timeErr = syncSource;


#ifdef PNM
  dcuId = 9;
#endif

// Attempt to make connection to DAQ system ******************************
#if !defined(NO_DAQ) && !defined(IOP_TASK)
  printf("Waiting for Network connect to FB - %d\n",dcuId);
  netRetry = 0;
  status = cdsDaqNetReconnect(dcuId);
  printf("Reconn status = %d %d\n",status,numFb);
  do{
#ifdef NO_RTL
	udelay(10);
#else
	usleep(10000);
#endif
	status = cdsDaqNetCheckReconnect();
	netRetry ++;
  }while((status < numFb) && (netRetry < 10));
  printf("Reconn Check = %d %d\n",status,numFb);

  if(fbStat[0] & 1) fbStat[0] = 3;
  if(fbStat[1] & 1) fbStat[1] = 3;
#endif

//   Initialize filter banks  *********************************************
  for (system = 0; system < NUM_SYSTEMS; system++) {
    for(ii=0;ii<MAX_MODULES;ii++){
      for(jj=0;jj<FILTERS;jj++){
        for(kk=0;kk<MAX_COEFFS;kk++){
          dspCoeff[system].coeffs[ii].filtCoeff[jj][kk] = 0.0;
        }
        dspCoeff[system].coeffs[ii].filtSections[jj] = 0;
      }
    }
  }

  // Initialize all filter module excitation signals to zero 
  for (system = 0; system < NUM_SYSTEMS; system++)
    for(ii=0;ii<MAX_MODULES;ii++)
       dsp[system].data[ii].exciteInput = 0.0;


  /* Initialize DSP filter bank values */
  for (system = 0; system < NUM_SYSTEMS; system++)
    if (initVars(dsp + system, pDsp[system], dspCoeff + system, MAX_MODULES, pCoeff[system])) {
    	printf("Filter module init failed, exiting\n");
	return 0;
    }

  printf("Initialized servo control parameters.\n");


// Initialize DAQ variable/software **************************************************
#if !defined(NO_DAQ) && !defined(IOP_TASK)
  /* Set data range limits for daqLib routine */
#if defined(SERVO2K) || defined(SERVO4K)
  daq.filtExMin = 20001;
  daq.filtTpMin = 30001;
#else
  daq.filtExMin = 1;
  daq.filtTpMin = 10001;
#endif
  daq.filtExMax = daq.filtExMin + MAX_MODULES;
  daq.filtExSize = MAX_MODULES;
  daq.xExMin = daq.filtExMax;
  daq.xExMax = daq.xExMin + 50;
  daq.filtTpMax = daq.filtTpMin + MAX_MODULES * 3;
  daq.filtTpSize = MAX_MODULES * 3;
  daq.xTpMin = daq.filtTpMax;
  daq.xTpMax = daq.xTpMin + 50;

  printf("DAQ Ex Min/Max = %d %d\n",daq.filtExMin,daq.filtExMax);
  printf("DAQ XEx Min/Max = %d %d\n",daq.xExMin,daq.xExMax);
  printf("DAQ Tp Min/Max = %d %d\n",daq.filtTpMin,daq.filtTpMax);
  printf("DAQ XTp Min/Max = %d %d\n",daq.xTpMin,daq.xTpMax);

  // Set an xtra TP to read out one pps signal
  testpoint[0] = (float *)&onePps;
#endif
  pLocalEpics->epicsOutput.diags[1] = 0;
  pLocalEpics->epicsOutput.diags[2] = 0;

#if defined(OMC_CODE) && !defined(COMPAT_INITIAL_LIGO)
  if (cdsPciModules.pci_rfm[0] != 0) {
	lscRfmPtr = (float *)(cdsPciModules.pci_rfm[0] + 0x3000);
  } else {
	lscRfmPtr = 0;
  }
#endif

#if !defined(NO_DAQ) && !defined(IOP_TASK)
  // Initialize DAQ function
  status = daqWrite(0,dcuId,daq,DAQ_RATE,testpoint,dspPtr[0],0,pLocalEpics->epicsOutput.gdsMon,xExc);
  if(status == -1) 
  {
    printf("DAQ init failed -- exiting\n");
    vmeDone = 1;
#if NUM_SYSTEMS > 1
    feDone();
#endif
    return(0);
  }
#endif

	// Read Dio card initial values ***********************************************
     for(kk=0;kk<cdsPciModules.doCount;kk++)
     {
	ii = cdsPciModules.doInstance[kk];
	if(cdsPciModules.doType[kk] == ACS_8DIO)
	{
	  if (rioReadOps[ii] & 1) rioInputInput[ii] = readIiroDio(&cdsPciModules, kk) & 0xff;
	  if (rioReadOps[ii] & 2) rioInputOutput[ii] = readIiroDioOutput(&cdsPciModules, kk) & 0xff;
	} else if(cdsPciModules.doType[kk] == ACS_16DIO) {
  	  rioInput1[ii] = readIiroDio1(&cdsPciModules, kk) & 0xffff;
	} else if (cdsPciModules.doType[kk] == CON_32DO) {
  	  CDO32Input[ii] = readCDO32l(&cdsPciModules, kk);
	} else if (cdsPciModules.doType[kk] == CON_1616DIO) {
  	  CDIO1616Input[ii] = readCDIO1616l(&cdsPciModules, kk);
	} else if (cdsPciModules.doType[kk] == CON_6464DIO) {
  	  CDIO6464Input[ii] = readCDIO6464l(&cdsPciModules, kk);
	} else if(cdsPciModules.doType[kk] == ACS_24DIO) {
  	  dioInput[ii] = readDio(&cdsPciModules, kk);
	}
     }

  // Clear the startup sync counter
  firstTime = 0;

  // Clear the code exit flag
  vmeDone = 0;

  // Initialize user application software *********************************************
  printf("Calling feCode() to initialize\n");
  feCode(clock16K,dWord,dacOut,dspPtr[0],&dspCoeff[0],pLocalEpics,1);

  printf("entering the loop\n");

  // Clear a couple of timing diags.
  adcHoldTime = 0;
  adcHoldTimeMax = 0;
  usrHoldTime = 0;
  missedCycle = 0;

  // Initialize the ADC 
#ifndef ADC_SLAVE
  for(jj=0;jj<cdsPciModules.adcCount;jj++)
  {
	  // Setup the DMA registers
	  status = gsaAdcDma1(jj,cdsPciModules.adcType[jj],ADC_DMA_BYTES);
	  // Preload input memory with dummy variables to test that new ADC data has arrived.
	  packedData = (int *)cdsPciModules.pci_adc[jj];
	  // Write a dummy 0 to first ADC channel location
	  // This location should never be zero when the ADC writes data as it should always
	  // have an upper bit set indicating channel 0.
          *packedData = 0x0;
          if (cdsPciModules.adcType[jj] == GSC_16AISS8AO4
              || cdsPciModules.adcType[jj] == GSC_18AISS8AO8) {
                 packedData += 3;
          } else packedData += 31;
	  // Write a number into the last channel which the ADC should never write ie no
	  // upper bits should be set in channel 31.
          *packedData = 0x110000;
  }
  printf("ADC setup complete \n");
#endif

#ifndef ADC_SLAVE
  // Initialize the DAC module variables
  for(jj = 0; jj < cdsPciModules.dacCount; jj++) {
        pDacData = (unsigned int *) cdsPciModules.pci_dac[jj];
 	// Reset DAC values to zero
        for(ii = 0; ii < (OVERSAMPLE_TIMES * 16); ii++) {
 	    *pDacData = 0; // (unsigned int) 10000; // for testing with the scope
	    pDacData ++;
        }
	// Preload DAC values
	if(DAC_PRELOAD_CNT)
	{
		// Arm DAC DMA for preload data size
		status = dacDmaPreload(jj,DAC_PRELOAD_CNT);
printf("Preloading DAC with %d samples\n",DAC_PRELOAD_CNT);
		// DMA preload samples
		gsaDacDma2(jj, cdsPciModules.dacType[jj]);
		// Wait plenty of time for DMA to complete
#ifdef NO_RTL
		udelay(1000);
#else
		usleep(1000);
#endif
	}

	// Arm DAC DMA for full data size
	status = gsaDacDma1(jj, cdsPciModules.dacType[jj]);
  }
#endif
  printf("DAC setup complete \n");

  
#ifdef NO_RTL

#ifndef NO_CPU_DISABLE
  // Take the CPU away from Linux
  //__cpu_disable(); 
#endif
#endif

#ifndef ADC_SLAVE
  if (!run_on_timer) {
  switch(syncSource)
  {
	case SYNC_SRC_TDS:
		CDIO1616Output[0] = 0x3300000;
		CDIO1616Input[0] = writeCDIO1616l(&cdsPciModules, 0, CDIO1616Output[0]);
		printf("writing BIO\n");
		usleep(40000);
		// Arm ADC modules
    		gsaAdcTrigger(cdsPciModules.adcCount,cdsPciModules.adcType);
		// Start clocking the DAC outputs
		gsaDacTrigger(&cdsPciModules);
		// Set synched flag so later code will not check for 1PPS
		sync21pps = 1;
		usleep(40000);
		// Start ADC/DAC clocks
		CDIO1616Output[0] = 0x7300000;
		CDIO1616Input[0] = writeCDIO1616l(&cdsPciModules, 0, CDIO1616Output[0]);
		break;
	case SYNC_SRC_IRIG_B:
		 // If IRIG-B card present, use it for startup synchronization
		wtmax = 999999;
		// wtmin = 999970;
		wtmin = 900970;
		#ifndef NO_RTL
			printf("Waiting until usec = %d to start the ADCs\n", wtmin);
		#endif
		do{
			if(cdsPciModules.gpsType == SYMCOM_RCVR) usec = getGpsUsec();
			if(cdsPciModules.gpsType == TSYNC_RCVR) gps_receiver_locked = getGpsTimeTsync(&timeSec,&usec);
		}while ((usec < wtmin) || (usec > wtmax));
		// Arm ADC modules
		gsaAdcTrigger(cdsPciModules.adcCount,cdsPciModules.adcType);
		// Start clocking the DAC outputs
		gsaDacTrigger(&cdsPciModules);
		// Set synched flag so later code will not check for 1PPS
		sync21pps = 1;
		// Send IRIG-B locked/not locked diagnostic info
		if(!gps_receiver_locked)pLocalEpics->epicsOutput.timeErr |= TIME_ERR_IRIGB;
		#ifndef NO_RTL
			printf("Running time %d us\n", usec);
		  	printf("Triggered the DAC\n");
		#endif
		break;
	default:
		    // IRIG-B card not found, so use CPU time to get close to 1PPS on startup
			// Pause until this second ends
		#ifdef NO_RTL
		/*
			:TODO:
			wait_delay = nano2count(WAIT_DELAY);
			period     = nano2count(PERIOD);
			for(msg = 0; msg < MAXDIM; msg++) {
				a[msg] = b[msg] = 3.141592;
			}
			rtai_cli();
			aim_time  = rt_get_time();
			sync_time = aim_time + wait_delay;
		*/
		#else
			clock_gettime(CLOCK_REALTIME, &next);
			// printf("Start time %ld s %ld ns\n", next.tv_sec, next.tv_nsec);
			next.tv_nsec = 0;
			next.tv_sec += 1;
			clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next, NULL);
			clock_gettime(CLOCK_REALTIME, &next);
			// printf("Running time without IRIGb%ld s %ld ns\n", next.tv_sec, next.tv_nsec);
		#endif
		break;
  }
  }


  if (run_on_timer) {
#ifndef NO_RTL
    printf("*******************************\n");
    printf("* Running with RTLinux timer! *\n");
    printf("*******************************\n");

#endif
  }

#ifndef NO_RTL
  // printf("Triggered the ADC\n");
#endif
#endif
#ifdef ADC_SLAVE
        ll = cdsPciModules.adcConfig[0];
        printf("waiting to sync %d\n",ioMemData->iodata[ll][0].cycle);
        rdtscl(cpuClock[0]);
        do{
                usleep(1);
        }while(ioMemData->iodata[0][0].cycle != 0);
        rdtscl(cpuClock[1]);
        cycleTime = (cpuClock[1] - cpuClock[0])/CPURATE;
        printf("Synched %d\n",cycleTime);
	sync21pps=1;
        // return(1);

#endif
  onePpsTime = clock16K;
  if(cdsPciModules.gpsType == SYMCOM_RCVR)
  {
  	time = getGpsTime(&ns);
	timeSec = time - 252806386;
  }
  if(cdsPciModules.gpsType == TSYNC_RCVR) 
  {
	gps_receiver_locked = getGpsTimeTsync(&timeSec,&usec);
	timeSec += 284083219;
  }

  rdtscl(adcTime);

  // **********************************************************************************************
  // Enter the infinite FE control loop  **********************************************************
  // **********************************************************************************************
  while(!vmeDone){ 	// Run forever until user hits reset

  	if (run_on_timer) {  // NO ADC present, so run on CPU realtime clock
	  // Pause until next cycle begins
#ifdef NO_RTL
	  udelay(1000);
#else
    	  struct timespec next;
    	  clock_gettime(CLOCK_REALTIME, &next);
	  if (clock16K == 0) {
 	    next.tv_nsec = 0;
	    next.tv_sec += 1;
	  } else {
    	    next.tv_nsec = 1000000000 / CYCLE_PER_SECOND * clock16K;
	  }
          clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next, NULL);
		rdtscl(cpuClock[0]);
#endif
	} else {
	// NORMAL OPERATION -- Wait for ADC data ready
	// On startup, only want to read one sample such that first cycle
	// coincides with GPS 1PPS. Thereafter, sampleCount will be 
	// increased to appropriate number of 65536 s/sec to match desired
	// code rate eg 32 samples each time thru before proceeding to match 2048 system.
	// **********************************************************************************************************
#ifndef ADC_SLAVE
       if(clock16K == 0)
        {
          timeSec ++;
          pLocalEpics->epicsOutput.timeDiag = timeSec;
        }
#endif
        for(ll=0;ll<sampleCount;ll++)
        {
#ifndef ADC_SLAVE
               for(jj=0;jj<cdsPciModules.adcCount;jj++)
		{
		    // Check ADC DMA has completed
		    // This is detected when last channel in memory no longer contains the
		    // dummy variable written during initialization and reset after the read.
		    packedData = (int *)cdsPciModules.pci_adc[jj];
               	    packedData += 31;
                    kk = 0;
                    do {
                        kk ++;
			// Need to delay if not ready as constant banging of the input register
			// will slow down the ADC DMA.
			if(*packedData == 0x110000) {
#ifdef NO_RTL
				udelay(1);
#else
				if(jj==0) usleep(1);
#endif
			}
			// Allow 1msec for data to be ready (should never take that long).
                    }while((*packedData == 0x110000) && (kk < 1000000));

		    // If data not ready in time, abort
		    // Either the clock is missing or code is running too slow and ADC FIFO
		    // is overflowing.
		    if (kk == 1000000) {
                        stop_working_threads = 1;
	  		pLocalEpics->epicsOutput.diagWord |= 0x1;
                        printf("timeout %d\n",jj);
                    }
		    if(jj == 0) 
		    {
			rdtscl(cpuClock[0]);
#ifdef ADC_MASTER
			for(mm=0;mm<cdsPciModules.dacCount;mm++)
	   		   if(dacWriteEnable > 4) gsaDacDma2(mm,cdsPciModules.dacType[mm]);
#endif
		    }
#if 0
// When running code on multiple cores, this causes delays as all tasks tend to collide on I/O.
// Need to spread this out over time between cores if this is going to work.
		    {
			// Read CPU clock for timing info for overall cycle time
		    	rdtscl(cpuClock[0]);
			// Check number of samples remaining in ADC FIFO
                    	status = checkAdcBuffer(0);
			if(status > (ADC_SAMPLE_COUNT * OVERSAMPLE_TIMES))
			{
				missedCycle ++;
				pLocalEpics->epicsOutput.diags[1] ++;
			}
		    }
#endif

                    // Read adc data
                    packedData = (int *)cdsPciModules.pci_adc[jj];
		    // FIrst, and only first, channel should have upper bit marker set.
                    // Return 0x10 if first ADC channel does not have sync bit set
                    if(*packedData & 0xf0000) status = 0;
                    else status = 16;
                    kk = jj + 1;
                    diagWord |= status * kk;

                    limit = 32700;
		    // Various ADC models have different number of channels/data bits
                    offset = 0x8000;
                    mask = 0xffff;
                    num_outs = 32;
#if 0
		// In this version, the following two ADC models are not fully supported.
		// This bit of code is left behind in case there is a need to reintroduce it.
                    if (cdsPciModules.adcType[jj] == GSC_18AISS8AO8) {
			limit *= 4; // 18 bit limit
			offset = 0x20000; // Data coding offset in 18-bit DAC
			mask = 0x3ffff;
			num_outs = 8;
                    }
                    if (cdsPciModules.adcType[jj] == GSC_16AISS8AO4) {
			num_outs = 4;
                    }
#endif
#ifdef ADC_MASTER
		    ioMemCntr = (clock16K % 64);
#endif
                    // Read adc data from PCI mapped memory into local variables
                    for(ii=0;ii<num_outs;ii++)
                    {
			// adcData is the integer representation of the ADC data
			adcData[jj][ii] = (*packedData & mask);
			adcData[jj][ii]  -= offset;
			// dWord is the double representation of the ADC data
			// This is the value used by the rest of the code calculations.
			dWord[jj][ii] = adcData[jj][ii];
#ifdef ADC_MASTER
			ioMemData->iodata[jj][ioMemCntr].data[ii] = adcData[jj][ii];
#endif
#ifdef OVERSAMPLE
			// Downsample filter only used channels to save time
			// This is defined in user C code
			if (dWordUsed[jj][ii]) {
				dWord[jj][ii] = iir_filter(dWord[jj][ii],FE_OVERSAMPLE_COEFF,2,&dHistory[ii+jj*32][0]);
			}
#endif
			packedData ++;
                    }
#ifdef ADC_MASTER
	  	    ioMemData->gpsSecond = timeSec;;
		    ioMemData->iodata[jj][ioMemCntr].cycle = clock16K;
#endif

		    // Check for ADC overflows
                    for(ii=0;ii<num_outs;ii++)
                    {
			if((adcData[jj][ii] > limit) || (adcData[jj][ii] < -limit))
			  {
				overflowAdc[jj][ii] ++;
				overflowAcc ++;
				diagWord |= 0x100 *  (jj+1);
			  }
                    }

		   // Clear out last ADC data read for test on next cycle
                   packedData = (int *)cdsPciModules.pci_adc[jj];
                   *packedData = 0x0;
#if 0
		   // Old support which may or may not come back
                   if(cdsPciModules.adcType[jj] == GSC_16AISS8AO4) packedData += 3;
                   else if(cdsPciModules.adcType[jj] == GSC_18AISS8AO8) packedData += 3;
                   else packedData += 31;
#endif
                   packedData += 31;
                   *packedData = 0x110000;
		   // Reset DMA Start Flag
		   gsaAdcDma2(jj);
            }
#endif

		  // Try synching to 1PPS on ADC[0][31] if not using IRIG-B
		  // Only try for 1 sec.
                  if(!sync21pps)
                  {
			// 1PPS signal should rise above 4000 ADC counts if present.
                        if((adcData[0][31] < ONE_PPS_THRESH) && (sync21ppsCycles < (CYCLE_PER_SECOND*OVERSAMPLE_TIMES))) 
			{
				ll = -1;
				sync21ppsCycles ++;
                        }else {
				// Need to start clocking the DAC outputs.
			        gsaDacTrigger(&cdsPciModules);
                                sync21pps = 1;
				// 1PPS never found, so indicate NO SYNC to user
				if(sync21ppsCycles >= (CYCLE_PER_SECOND*OVERSAMPLE_TIMES))
				{
					syncSource = SYNC_SRC_NONE;
					printf("NO SYNC SOURCE FOUND %d %d\n",sync21ppsCycles,adcData[0][31]);
				} else {
				// 1PPS found and synched to
					printf("GPS Trigg %d %d\n",adcData[0][31],sync21pps);
					syncSource = SYNC_SRC_1PPS;
				}
				pLocalEpics->epicsOutput.timeErr = syncSource;
                        }
                }
#ifdef ADC_SLAVE
               for(jj=0;jj<cdsPciModules.adcCount;jj++)
		{
        		mm = cdsPciModules.adcConfig[jj];
                        kk = 0;
                        do{
                                usleep(1);
                                kk++;
                        }while((ioMemData->iodata[mm][ioMemCntr].cycle != ioClock) && (kk < 1000));
                        if(kk >= 1000)
                        {
                                printf ("ADC TIMEOUT %d %d %d %d\n",jj,ioMemData->iodata[mm][ioMemCntr].cycle, ioMemCntr,ioClock);
printf("got here %d %d\n",clock16K,ioClock);
                                return(-1);
                        }
                        for(ii=0;ii<32;ii++)
                        {
                                adcData[jj][ii] = ioMemData->iodata[mm][ioMemCntr].data[ii];
                                dWord[jj][ii] = adcData[jj][ii];
#ifdef OVERSAMPLE
				if (dWordUsed[jj][ii]) {
					dWord[jj][ii] = iir_filter(dWord[jj][ii],FE_OVERSAMPLE_COEFF,2,&dHistory[ii+jj*32][0]);
				}
#endif
                                // No filter  dWord[kk][ll] = adcData[kk][ll];
                        }
                }
                ioClock = (ioClock + 1) % 65536;
                ioMemCntr = (ioMemCntr + 1) % IO_MEMORY_SLOTS;
        rdtscl(cpuClock[0]);

#endif
	}

	// After first synced ADC read, must set to code to read number samples/cycle
        sampleCount = OVERSAMPLE_TIMES;
	}
#ifdef ADC_SLAVE
       if(clock16K == 0)
        {
	  timeSec = ioMemData->gpsSecond;
          pLocalEpics->epicsOutput.timeDiag = timeSec;
        }
#endif

#ifdef DOLPHIN_TEST
	if (dolphin_memory) {
	  if (client == 0) *dolphin_memory = clock16K;
	}
	
#endif

        // Update internal cycle counters
        if(firstTime != 0)
        {
          clock16K += 1;
          clock16K %= CYCLE_PER_SECOND;
	  clock1Min += 1;
	  clock1Min %= CYCLE_PER_MINUTE;
          if(subcycle == DAQ_CYCLE_CHANGE) daqCycle = (daqCycle + 1) % 16;
          if(subcycle == END_OF_DAQ_BLOCK) /*we have reached the 16Hz second barrier*/
            {
              /* Reset the data cycle counter */
              subcycle = 0;
  
            }
          else{
            /* Increment the internal cycle counter */
            subcycle ++;                                                
          }
        }

#ifdef TEST_SYSTEM
        dWord[0][0] = cycleTime;
	dWord[0][1] = usrTime;
	dWord[0][2] = adcHoldTime;
#endif

	// Call the front end specific software ***********************************************
        rdtscl(cpuClock[4]);
	feCode(clock16K,dWord,dacOut,dspPtr[0],&dspCoeff[0],pLocalEpics,0);
        rdtscl(cpuClock[5]);

// START OF DAC WRITE ***********************************************************************************
	// Write out data to DAC modules
	for(jj=0;jj<cdsPciModules.dacCount;jj++)
	{
#ifdef ADC_SLAVE
	   mm = cdsPciModules.dacConfig[jj];
// printf("mm = %d\n",mm);
#else
	   // Check Dac output overflow and write to DMA buffer
	   pDacData = (unsigned int *)cdsPciModules.pci_dac[jj];
#endif
#ifdef ADC_MASTER
	mm = cdsPciModules.dacConfig[jj];
	// if((jj==1) )
	// {
			if(ioMemData->iodata[mm][ioMemCntrDac].cycle == ioClockDac)
			{
				dacEnable |= (jj+1);
                		// pLocalEpics->epicsOutput.diags[1] |= (jj+1);
			}else {
				dacEnable &= ~(jj+1);
                		// pLocalEpics->epicsOutput.diags[1] &= ~(jj+1);
                		// pLocalEpics->epicsOutput.diags[1] = ioClockDac;
			}
                		// if(ioClockDac == 15) // pLocalEpics->epicsOutput.diags[1] = ioMemData->iodata[mm][ioMemCntrDac].cycle;
			// 	printf("ioclock 8 = %d %d %d\n",mm, ioClockDac,ioMemData->iodata[mm][ioMemCntrDac].cycle);
                		// pLocalEpics->epicsOutput.diags[1] = mm;
	// }
#endif
#ifdef OVERSAMPLE_DAC
	   for (kk=0; kk < OVERSAMPLE_TIMES; kk++) {
#endif
		int limit = 32000;
		int offset = 0; //0x8000;
		int mask = 0xffff;
		int num_outs = 16;
		if (cdsPciModules.dacType[jj] == GSC_18AISS8AO8) {
			limit *= 4; // 18 bit limit
			offset = 0x20000; // Data coding offset in 18-bit DAC
			mask = 0x3ffff;
			num_outs = 8;
		}
		if (cdsPciModules.dacType[jj] == GSC_18AO8) {
			limit *= 4; // 18 bit limit
			//offset = 0x20000; // Data coding offset in 18-bit DAC
			mask = 0x3ffff;
			num_outs = 8;

	// Uncomment to read 18-bit DAC buffer size
	//extern volatile GSA_DAC_REG *dacPtr[MAX_DAC_MODULES];
        //volatile GSA_18BIT_DAC_REG *dac18bitPtr = dacPtr[0];
        //out_buf_size = dac18bitPtr->OUT_BUF_SIZE;

		}
		if (cdsPciModules.dacType[jj] == GSC_16AISS8AO4) {
		  	num_outs = 4;
		}
		for (ii=0; ii < num_outs; ii++)
		{
#ifdef OVERSAMPLE_DAC
			if (dacOutUsed[jj][ii]) {
#ifdef NO_ZERO_PAD
		 	  dac_in = dacOut[jj][ii];
		 	  dac_in = iir_filter(dac_in,FE_OVERSAMPLE_COEFF,2,&dDacHistory[ii+jj*16][0]);
#else
			  dac_in =  kk == 0? (double)dacOut[jj][ii]: 0.0;
		 	  dac_in = iir_filter(dac_in,FE_OVERSAMPLE_COEFF,2,&dDacHistory[ii+jj*16][0]);
			  dac_in *= OVERSAMPLE_TIMES;
#endif
			}
			else dac_in = 0.0;
			// Smooth out some of the double > short roundoff errors
			if(dac_in > 0.0) dac_in += 0.5;
			else dac_in -= 0.5;
			dac_out = dac_in;
#else
#ifdef ADC_MASTER
			if(dacEnable & (jj+1))
			dac_out = ioMemData->iodata[mm][ioMemCntrDac].data[ii];
			else dac_out = 0;
#else
			dac_out = dacOut[jj][ii];
#endif
#endif
			if(dac_out > limit) 
			{
				dac_out = limit;
				overflowDac[jj][ii] ++;
				overflowAcc ++;
				diagWord |= 0x1000 *  (jj+1);
			}
			if(dac_out < -limit) 
			{
				dac_out = -limit;
				overflowDac[jj][ii] ++;
				overflowAcc ++;
				diagWord |= 0x1000 *  (jj+1);
			}
			// Load last values to EPICS channels for monitoring on GDS_TP screen.
		 	dacOutEpics[jj][ii] = dac_out;
#ifdef ADC_SLAVE
			memCtr = (ioMemCntrDac + kk) % IO_MEMORY_SLOTS;
	   		ioMemData->iodata[mm][memCtr].data[ii] = dac_out;
#else
			dac_out += offset;
			//if (ii == 0) printf("%d\n", dac_out);
			*pDacData =  (unsigned int)(dac_out & mask);
			pDacData ++;
#endif
		}
#ifdef ADC_SLAVE
		ioMemData->iodata[mm][memCtr].cycle = ioClockDac + kk;
		// ioMemData->iodata[mm][(ioMemCntrDac+kk)].cycle = ioClockDac + kk;
		// ioClockDac = (ioClockDac + 1) % 65536;
		// ioMemCntrDac = (ioMemCntrDac + 1) % IO_MEMORY_SLOTS;
#endif
#ifdef OVERSAMPLE_DAC
  	   }
#endif
  	   // DMA out dac values
	   // If ADC read was late, don't write DAC ie probably missed a clock
	   // if(missedCycle) missedCycle --;
#ifndef ADC_SLAVE
#ifndef ADC_MASTER
	   if(dacWriteEnable > 4) gsaDacDma2(jj,cdsPciModules.dacType[jj]);
#endif
#endif
	}
#ifdef ADC_SLAVE
		// ioMemData->iodata[mm][ioMemCntrDac].cycle = ioClockDac;
		ioClockDac = (ioClockDac + OVERSAMPLE_TIMES) % 65536;
		ioMemCntrDac = (ioMemCntrDac  + OVERSAMPLE_TIMES) % IO_MEMORY_SLOTS;
#endif
#ifdef ADC_MASTER
		ioMemData->iodata[mm][ioMemCntrDac].cycle = -1;
		ioClockDac = (ioClockDac + 1) % 65536;
		ioMemCntrDac = (ioMemCntrDac + 1) % IO_MEMORY_SLOTS;
#endif
	if(dacWriteEnable < 10) dacWriteEnable ++;
// END OF DAC WRITE ***********************************************************************************

#ifdef DUOTONE_TIMING
        if(clock16K == 0)
        {
                duotoneMean = duotoneTotal/CYCLE_PER_SECOND;
                duotoneTotal = 0.0;
        }
#ifdef DUOTONE_DAC
        duotone[clock16K] = dWord[0][0];
        duotoneTotal += dWord[0][0];
#else
        duotone[clock16K] = dWord[0][31];
        duotoneTotal += dWord[0][31];
#endif
        if(clock16K == 16)
        {
                duotoneTime = duotime(7, duotoneMean, duotone);
                // pLocalEpics->epicsOutput.diags[1] = duotoneTime;
        }
#endif


	// Send timing info to EPICS at 1Hz
        if((subcycle == 0) && (daqCycle == 15))
        {
	  pLocalEpics->epicsOutput.cpuMeter = timeHold;
	  pLocalEpics->epicsOutput.cpuMeterMax = timeHoldMax;
  	  pLocalEpics->epicsOutput.diags[1] = dacEnable;
          timeHold = 0;
	  pLocalEpics->epicsOutput.adcWaitTime = adcHoldTimeMax;
		adcHoldTimeMax = 0;
	  if((adcHoldTime > CYCLE_TIME_ALRM_HI) || (adcHoldTime < CYCLE_TIME_ALRM_LO)) diagWord |= FE_ADC_HOLD_ERR;
	  if(timeHoldMax > CYCLE_TIME_ALRM_HI) diagWord |= FE_PROC_TIME_ERR;
  	  if(pLocalEpics->epicsInput.diagReset || initialDiagReset)
	  {
		initialDiagReset = 0;
		pLocalEpics->epicsInput.diagReset = 0;
  		// pLocalEpics->epicsOutput.diags[1] = 0;
		timeHoldMax = 0;
	  	diagWord = 0;
#ifndef NO_RTL
		// printf("DIAG RESET\n");
#endif
	  }
	  // Flip the onePPS various once/sec as a watchdog monitor.
	  // pLocalEpics->epicsOutput.onePps ^= 1;
	  pLocalEpics->epicsOutput.diagWord = diagWord;
        }

	/* Update User code Epics variables */
#if MAX_MODULES > END_OF_DAQ_BLOCK
	epicsCycle = (epicsCycle + 1) % (MAX_MODULES + 2);
#else
	epicsCycle = subcycle;
#endif

  	for (system = 0; system < NUM_SYSTEMS; system++)
		updateEpics(epicsCycle, dspPtr[system], pDsp[system],
			    &dspCoeff[system], pCoeff[system]);

	// Check if user has pushed reset button.
	// If so, kill the task
        vmeDone = stop_working_threads | checkEpicsReset(epicsCycle, pLocalEpics);
	// usleep(1);

	// Clear the first cycle flag
	if(firstTime == 0)
	{
		firstTime = 200;
		onePpsHi = 0;
	}

  
	// If synced to 1PPS on startup, continue to check that code
	// is still in sync with 1PPS.
	if(syncSource == SYNC_SRC_1PPS)
	{

		// Assign chan 32 to onePps 
		onePps = adcData[0][31];
		if((onePps > ONE_PPS_THRESH) && (onePpsHi == 0))  
		{
			onePpsTime = clock16K;
			onePpsHi = 1;
		}
		if(onePps < ONE_PPS_THRESH) onePpsHi = 0;  

		// Check if front end continues to be in sync with 1pps
		// If not, set sync error flag
		if(onePpsTime > 1) pLocalEpics->epicsOutput.timeErr |= TIME_ERR_1PPS;
	}

 // Read Dio cards once per second
        if(clock16K < cdsPciModules.doCount)
        {
                kk = clock16K;
                ii = cdsPciModules.doInstance[kk];
                if(cdsPciModules.doType[kk] == ACS_8DIO)
                {
	  		if (rioReadOps[ii] & 1) rioInputInput[ii] = readIiroDio(&cdsPciModules, kk) & 0xff;
	  		if (rioReadOps[ii] & 2) rioInputOutput[ii] = readIiroDioOutput(&cdsPciModules, kk) & 0xff;
                }
                if(cdsPciModules.doType[kk] == ACS_16DIO)
                {
                        rioInput1[ii] = readIiroDio1(&cdsPciModules, kk) & 0xffff;
                }
		if(cdsPciModules.doType[kk] == ACS_24DIO)
		{
		  dioInput[ii] = readDio(&cdsPciModules, kk);
		}
        }
        // Write Dio cards on change
        for(kk=0;kk < cdsPciModules.doCount;kk++)
        {
                ii = cdsPciModules.doInstance[kk];
                if((cdsPciModules.doType[kk] == ACS_8DIO) && (rioOutput[ii] != rioOutputHold[ii]))
                {
                        writeIiroDio(&cdsPciModules, kk, rioOutput[ii]);
                        rioOutputHold[ii] = rioOutput[ii];
                } else 
                if((cdsPciModules.doType[kk] == ACS_16DIO) && (rioOutput1[ii] != rioOutputHold1[ii]))
                {
                        writeIiroDio1(&cdsPciModules, kk, rioOutput1[ii]);
                        rioOutputHold1[ii] = rioOutput1[ii];
                        // printf("write relay mod %d = %d\n",kk,rioOutput1[ii]);
                } else 
                if(cdsPciModules.doType[kk] == CON_32DO)
                {
                        if (CDO32Input[ii] != CDO32Output[ii]) {
                          CDO32Input[ii] = writeCDO32l(&cdsPciModules, kk, CDO32Output[ii]);
                        }
		} else if (cdsPciModules.doType[kk] == CON_1616DIO) {
#if 0
			if (CDIO1616Input[ii] != CDIO1616Output[ii]) {
			  CDIO1616Input[ii] = writeCDIO1616l(&cdsPciModules, kk, CDIO1616Output[ii]);
			}
#endif
			CDIO1616InputInput[ii] = readInputCDIO1616l(&cdsPciModules, kk);
		} else if (cdsPciModules.doType[kk] == CON_6464DIO) {
			if (CDIO6464Input[ii] != CDIO6464Output[ii]) {
			  CDIO6464Input[ii] = writeCDIO6464l(&cdsPciModules, kk, CDIO6464Output[ii]);
			}
			CDIO6464InputInput[ii] = readInputCDIO6464l(&cdsPciModules, kk);
                } else
                if((cdsPciModules.doType[kk] == ACS_24DIO) && (dioOutputHold[ii] != dioOutput[ii]))
		{
                        writeDio(&cdsPciModules, kk, dioOutput[ii]);
			dioOutputHold[ii] = dioOutput[ii];
		}
        }


#ifndef NO_DAQ
	// Write DAQ and GDS values once we are synched to 1pps
  	if(firstTime != 0) 
	{
		// Call daqLib
		pLocalEpics->epicsOutput.diags[3] = 
			daqWrite(1,dcuId,daq,DAQ_RATE,testpoint,dspPtr[0],myGmError2,pLocalEpics->epicsOutput.gdsMon,xExc);
		// Check if any messages received and get xmit que count.
		status = cdsDaqNetCheckCallback();
		// If callbacks pending count is high, FB must not be responding.
		// Check FB0
		if(((status & 0xff) > FE_MAX_FB_QUE) && (fbStat[0] & FE_FB_ONLINE))
		{
			printf("Net fail to fb0 \n");
			// Set error flag
			attemptingReconnect = 1;
			// Send FB status to on net, but not online.
			fbStat[0] = 0x1;
		}
		// Check FB1
		if((((status >> 8) & 0xff) > FE_MAX_FB_QUE) && (fbStat[1] & FE_FB_ONLINE))
		{
			printf("Net fail to fb1 - recon try\n");
			// Set error flag
			attemptingReconnect = 1;
			// Send FB status to on net, but not online.
			fbStat[1] = 0x1;
		}

		// Check if any FB have returned to net as indicated by reduced send que count
		if((attemptingReconnect) && (clock16K == 1000)) 
		{
			// Check FB0
			if(((status & 0xff) < FE_MAX_FB_QUE) && (fbStat[0] == FE_FB_AVAIL))
			{
				// Set fbStat to include recon bit needed by myri.c code
				fbStat[0] = 5;
				// Send recon message to FB
				status = cdsDaqNetReconnect(dcuId);
				// printf("Send recon command fb0\n");
			}
			// Check FB1
			if((((status >> 8) & 0xff) < FE_MAX_FB_QUE) && (fbStat[1] == FE_FB_AVAIL))
			{
				// Set fbStat to include recon bit needed by myri.c code
				fbStat[1] = 5;
				// Send recon message to FB
				status = cdsDaqNetReconnect(dcuId);
				// printf("Send recon command fb1\n");
			}
		}
		// Once per second, check if FB are back on line
		if((attemptingReconnect) && (clock16K == 0)) 
		{
			// Clear error flag
			attemptingReconnect = 0;
			if((fbStat[0] & FE_FB_AVAIL) && ((fbStat[0] & FE_FB_ONLINE) == 0))
				// Set error flag
				attemptingReconnect = 1;
			if((fbStat[1] & FE_FB_AVAIL) && ((fbStat[1] & FE_FB_ONLINE) == 0))
				// Set error flag
				attemptingReconnect = 1;
		}
	}
#endif

#ifdef NO_RTL
	//rt_sleep(nano2count(2000));
	//msleep(1);
#else
	// usleep(1);
#endif

        if((subcycle == 0) && (daqCycle == 14))
        {
	  pLocalEpics->epicsOutput.diags[0] = usrHoldTime;
	  // Create FB status word for return to EPICS
#ifdef USE_GM
  	  pLocalEpics->epicsOutput.diags[2] = (fbStat[1] & 3) * 4 + (fbStat[0] & 3);
#endif
#if defined(SHMEM_DAQ)
	  mxStat = 0;
	  mxDiagR = daqPtr->status;
	  if((mxDiag & 1) != (mxDiagR & 1)) mxStat = 1;
	  if((mxDiag & 2) != (mxDiagR & 2)) mxStat += 2;
	  pLocalEpics->epicsOutput.diags[2] = mxStat;
  	  mxDiag = mxDiagR;
#endif
	  usrHoldTime = 0;
  	  if((pLocalEpics->epicsInput.overflowReset) || (overflowAcc > 0x1000000))
	  {

#ifdef ROLLING_OVERFLOWS
                if (pLocalEpics->epicsInput.overflowReset) {
                   for (ii = 0; ii < 16; ii++) {
                      for (jj = 0; jj < cdsPciModules.adcCount; jj++) {
                         overflowAdc[jj][ii] = 0;
                         overflowAdc[jj][ii + 16] = 0;
                      }

                      for (jj = 0; jj < cdsPciModules.dacCount; jj++) {
                         overflowDac[jj][ii] = 0;
                      }
                   }
                }
#endif

		pLocalEpics->epicsInput.overflowReset = 0;
		pLocalEpics->epicsOutput.ovAccum = 0;
		overflowAcc = 0;
	  }
        }

        if(clock16K == 220)
	{
	      // Send DAC output values at 16Hzfb
	      for(jj=0;jj<cdsPciModules.dacCount;jj++)
	      {
	    	for(ii=0;ii<16;ii++)
	    	{
			pLocalEpics->epicsOutput.dacValue[jj][ii] = dacOutEpics[jj][ii];
		}
	      }
	}

        if(clock16K == 200)
        {
		pLocalEpics->epicsOutput.ovAccum = overflowAcc;
	  for(jj=0;jj<cdsPciModules.adcCount;jj++)
	  {
	    for(ii=0;ii<32;ii++)
	    {
		pLocalEpics->epicsOutput.overflowAdc[jj][ii] = overflowAdc[jj][ii];

#ifdef ROLLING_OVERFLOWS
                if (overflowAdc[jj][ii] > 0x1000000) {
		   overflowAdc[jj][ii] = 0;
                }
#else
		overflowAdc[jj][ii] = 0;
#endif

	    }
	  }
	  for(jj=0;jj<cdsPciModules.dacCount;jj++)
	  {
	    for(ii=0;ii<16;ii++)
	    {
		pLocalEpics->epicsOutput.overflowDac[jj][ii] = overflowDac[jj][ii];

#ifdef ROLLING_OVERFLOWS
                if (overflowDac[jj][ii] > 0x1000000) {
		   overflowDac[jj][ii] = 0;
                }
#else
		overflowDac[jj][ii] = 0;
#endif

	    }
	  }
        }
	// Measure time to complete 1 cycle
        rdtscl(cpuClock[1]);

	// Compute max time of one cycle.
	cycleTime = (cpuClock[1] - cpuClock[0])/CPURATE;
	//if (clock16K < 2) printf("cycle %d time %d\n", clock16K, cycleTime);
	// Hold the max cycle time over the last 1 second
	if(cycleTime > timeHold) timeHold = cycleTime;
	cycleTime = (cpuClock[1] - cpuClock[0])/CPURATE;
	// Hold the max cycle time since last diag reset
	if(cycleTime > timeHoldMax) timeHoldMax = cycleTime;
	adcHoldTime = (cpuClock[0] - adcTime)/CPURATE;
	if(adcHoldTime > adcHoldTimeMax) adcHoldTimeMax = adcHoldTime;
	adcTime = cpuClock[0];
	// Calc the max time of one cycle of the user code
	usrTime = (cpuClock[5] - cpuClock[4])/CPURATE;
	if(usrTime > usrHoldTime) usrHoldTime = usrTime;

  }

#if (NUM_SYSTEMS > 1) && !defined(PNM)
  feDone();
#endif

  /* System reset command received */
  return (void *)-1;
}

// MAIN routine: Code starting point ****************************************************************
#ifdef NO_RTL

// These externs and "16" need to go to a header file (mbuf.h)
extern void *kmalloc_area[16];
extern int mbuf_allocate_area(char *name, int size, struct file *file);

int init_module (void)
#else
int main(int argc, char **argv)
#endif
{
#ifndef NO_RTL
        pthread_attr_t attr;
#endif
 	int status;
	int ii,jj,kk;
	char fname[128];
	int cards;
	int adcCnt[6];
	int dacCnt[6];

#ifdef DOLPHIN_TEST
	if (argc > 1) {
		client = 1;
		target_node = atoi(argv[1]);
		printf("Running Dolphin client, server at target %d\n", target_node);

                scierror_t err =
                sci_connect_segment(NO_BINDING,
                                target_node,
                                0,
                                0,
                                1,
                                0, /* FLAGS */
                                func, 0,
                                &remote_segment_handle);
                printk("DIS connect segment status %d\n", err);
                if (err) return -1;

	        usleep(20000);
        	err = sci_map_segment(remote_segment_handle,
                                PHYS_MAP_ATTR_STRICT_IO,
                                0,
                                16,
                                &client_map_handle);
        	printk("DIS segment mapping status %d\n", err);
        	if (err) {
                	sci_disconnect_segment(&remote_segment_handle, 0);
                	return -1;
	        }

		double *addr = sci_kernel_virtual_address_of_mapping(client_map_handle);
        	if (addr == 0) {
                	printk ("Got zero pointer from sci_kernel_virtual_address_of_mapping\n");
                	return -1;
        	} else {
			printk ("Dolphin memory at 0x%p\n", addr);
			dolphin_memory = addr;
		}
	} else {
		printf("Running Dolphin server\n");

        	scierror_t err =
        	sci_create_segment(NO_BINDING,
                                0,
                                1,
                                0,
                                16,
                                server_func,
                                0,
                                &segment);
        	printk("DIS segment alloc status %d\n", err);
        	if (err) return -1;

        	err = sci_export_segment(segment, 0, 0);
        	printk("DIS segment export status %d\n", err);
        	if (err) {
                	sci_remove_segment(&segment, 0);
                	return -1;
        	}

	        err = sci_set_local_segment_available(segment, 0);
       	 	printk("DIS segment making available status %d\n", err);
        	if (err) {
                	sci_remove_segment(&segment, 0);
                	return -1;
        	}

	        double *addr = sci_local_kernel_virtual_address(segment);
       		if (addr == 0) {
                	printk("DIS sci_local_kernel_virtual_address returned 0\n");
                	sci_remove_segment(&segment, 0);
                	return -1;
        	} else {
			printk("Dolphin memory at 0x%p\n", addr);
			dolphin_memory = addr;
		}
	}

	if (dolphin_memory) *dolphin_memory = 0;
#endif
	jj = 0;
	printf("cpu clock %ld\n",cpu_khz);


#ifdef NO_RTL

        int ret =  mbuf_allocate_area(SYSTEM_NAME_STRING_LOWER, 64*1024*1024, 0);
        if (ret < 0) {
                printf("mbuf_allocate_area() failed; ret = %d\n", ret);
                return -1;
        }
        _epics_shm = (unsigned char *)(kmalloc_area[ret]);
        printf("Epics shmem set at 0x%p\n", _epics_shm);
        ret =  mbuf_allocate_area("ipc", 4*1024*1024, 0);
        if (ret < 0) {
                printf("mbuf_allocate_area(ipc) failed; ret = %d\n", ret);
                return -1;
        }
        _ipc_shm = (unsigned char *)(kmalloc_area[ret]);

#else
        /*
         * Create the shared memory area.  By passing a non-zero value
         * for the mode, this means we also create a node in the GPOS.
         */       

	/* See if we can open new-style shared memory file */
	sprintf(fname, "/rtl_mem_%s", SYSTEM_NAME_STRING_LOWER);
        wfd = shm_open(fname, RTL_O_RDWR, 0666);
	if (wfd == -1) {
          printf("Warning, couldn't open `%s' read/write (errno=%d)\n", fname, errno);
          wfd = shm_open("/rtl_epics", RTL_O_RDWR, 0666);
          if (wfd == -1) {
                printf("open failed for write on /rtl_epics (%d)\n",errno);
                rtl_perror("shm_open()");
                return -1;
          }
	}

	// Set pointer to EPICS shared memory
        _epics_shm = (unsigned char *)rtl_mmap(NULL,MMAP_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,wfd,0);
        if (_epics_shm == MAP_FAILED) {
                printf("mmap failed for epics shared memory area\n");
                rtl_perror("mmap()");
                return -1;
        }

	// See if IPC area is available, open and map it
	// IPC area is used to pass data between realtime processes on different cores.
	strcpy(fname, "/rtl_mem_ipc");
        ipc_fd = shm_open(fname, RTL_O_RDWR, 0666);
	if (ipc_fd == -1) {
          //printf("Warning, couldn't open `%s' read/write (errno=%d)\n", fname, errno);
	} else {
          _ipc_shm = (unsigned char *)rtl_mmap(NULL,MMAP_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,ipc_fd,0);
          if (_ipc_shm == MAP_FAILED) {
                printf("mmap failed for IPC shared memory area\n");
                rtl_perror("mmap()");
                return -1;
	  }
        }
	printf("IPC at 0x%x\n",(int)_ipc_shm);
  	ioMemData = (IO_MEM_DATA *)(_ipc_shm+ 0x4000);

#endif

// If DAQ is via shared memory (Framebuilder code running on same machine or MX networking is used)
// attach DAQ shared memory location.
#if defined(SHMEM_DAQ)
#ifdef NO_RTL
        sprintf(fname, "%s_daq", SYSTEM_NAME_STRING_LOWER);
        ret =  mbuf_allocate_area(fname, 64*1024*1024, 0);
        if (ret < 0) {
                printf("mbuf_allocate_area() failed; ret = %d\n", ret);
                return -1;
        }
        _daq_shm = (unsigned char *)(kmalloc_area[ret]);
        printf("Allocated daq shmem; set at 0x%p\n", _daq_shm);
 	daqPtr = (struct rmIpcStr *) _daq_shm;
#else
	// See if frame builder DAQ comm area available
	sprintf(fname, "/rtl_mem_%s_daq", SYSTEM_NAME_STRING_LOWER);
        daq_fd = shm_open(fname, RTL_O_RDWR, 0666);
	if (daq_fd == -1) {
          printf("Error: couldn't open `%s' read/write (errno=%d)\n", fname, errno);
	  return -1;
	} else {
          _daq_shm = (unsigned char *)rtl_mmap(NULL,MMAP_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,daq_fd,0);
          if (_daq_shm == MAP_FAILED) {
                printf("mmap failed for DAQ shared memory area\n");
                rtl_perror("mmap()");
                return -1;
	  }
 	  daqPtr = (struct rmIpcStr *) _daq_shm;
        }
#endif
#endif

	// Find and initialize all PCI I/O modules
	  cards = sizeof(cards_used)/sizeof(cards_used[0]);

	  printf("configured to use %d cards\n", cards);
	  cdsPciModules.cards = cards;
	  cdsPciModules.cards_used = cards_used;
          //return -1;
	printf("Initializing PCI Modules\n");
	cdsPciModules.adcCount = 0;
	cdsPciModules.dacCount = 0;
	cdsPciModules.dioCount = 0;
	cdsPciModules.doCount = 0;

	// Call PCI initialization routine in map.c file.
#ifndef ADC_SLAVE
	status = mapPciModules(&cdsPciModules);
#endif
#ifdef ADC_SLAVE
	printf("%d PCI cards found\n",ioMemData->totalCards);
	status = 0;
	adcCnt[0] = 0;
	dacCnt[0] = 0;
	for(ii=0;ii<ioMemData->totalCards;ii++)
	{
printf("Model %d = %d\n",ii,ioMemData->model[ii]);
		for(jj=0;jj<cards;jj++)
		{
		   switch(ioMemData->model[ii])
		   {
			case GSC_16AI64SSA:
				if((cdsPciModules.cards_used[jj].type == GSC_16AI64SSA) && 
					(cdsPciModules.cards_used[jj].instance == adcCnt[0]))
				{
					printf("Found ADC at %d %d\n",jj,ii);
					kk = cdsPciModules.adcCount;
					cdsPciModules.adcType[kk] = GSC_16AI64SSA;
					cdsPciModules.adcConfig[kk] = ii;
					cdsPciModules.adcCount ++;
					status ++;
				}
				break;
			case GSC_16AO16:
				if((cdsPciModules.cards_used[jj].type == GSC_16AO16) && 
					(cdsPciModules.cards_used[jj].instance == dacCnt[0]))
				{
					printf("Found DAC at %d %d\n",jj,ii);
					kk = cdsPciModules.dacCount;
					cdsPciModules.dacType[kk] = GSC_16AO16;
					cdsPciModules.dacConfig[kk] = ii;
	   				cdsPciModules.pci_dac[kk] = (long)(ioMemData->iodata[ii]);
					cdsPciModules.dacCount ++;
					status ++;
				}
				break;
			default:
				break;
		   }
		}
		if(ioMemData->model[ii] == GSC_16AI64SSA) adcCnt[0] ++;
		if(ioMemData->model[ii] == GSC_16AO16) dacCnt[0] ++;
	}
#endif
	printf("%d PCI cards found %d %d\n",status,adcCnt[0],dacCnt[0]);

	// Print out all the I/O information
        printf("***************************************************************************\n");
#ifdef ADC_MASTER
		ioMemData->totalCards = status;
		ioMemData->adcCount = cdsPciModules.adcCount;
		ioMemData->dacCount = cdsPciModules.dacCount;
#endif
	printf("%d ADC cards found\n",cdsPciModules.adcCount);
	for(ii=0;ii<cdsPciModules.adcCount;ii++)
        {
#ifdef ADC_MASTER
		ioMemData->model[ii] = cdsPciModules.adcType[ii];
		kk = cdsPciModules.adcCount;
#endif
                if(cdsPciModules.adcType[ii] == GSC_18AISS8AO8)
                {
                        printf("\tADC %d is a GSC_18AISS8AO8 module\n",ii);
                        if((cdsPciModules.adcConfig[ii] & 0x10000) > 0) jj = 4;
                        else jj = 8;
                        printf("\t\tChannels = %d \n",jj);
                        printf("\t\tFirmware Rev = %d \n\n",(cdsPciModules.adcConfig[ii] & 0xfff));
                }
                if(cdsPciModules.adcType[ii] == GSC_16AISS8AO4)
                {
                        printf("\tADC %d is a GSC_16AISS8AO4 module\n",ii);
                        if((cdsPciModules.adcConfig[ii] & 0x10000) > 0) jj = 4;
                        else jj = 8;
                        printf("\t\tChannels = %d \n",jj);
                        printf("\t\tFirmware Rev = %d \n\n",(cdsPciModules.adcConfig[ii] & 0xfff));
                }
                if(cdsPciModules.adcType[ii] == GSC_16AI64SSA)
                {
                        printf("\tADC %d is a GSC_16AI64SSA module\n",ii);
                        if((cdsPciModules.adcConfig[ii] & 0x10000) > 0) jj = 32;
                        else jj = 64;
                        printf("\t\tChannels = %d \n",jj);
                        printf("\t\tFirmware Rev = %d \n\n",(cdsPciModules.adcConfig[ii] & 0xfff));
                }
        }
        printf("***************************************************************************\n");
	printf("%d DAC cards found\n",cdsPciModules.dacCount);
	for(ii=0;ii<cdsPciModules.dacCount;ii++)
        {
                if(cdsPciModules.dacType[ii] == GSC_18AISS8AO8)
                {
                        printf("\tDAC %d is a GSC_18AISS8AO8 module\n",ii);
                        if((cdsPciModules.dacConfig[ii] & 0x20000) > 0) jj = 0;
                        else jj = 4;
                        printf("\t\tChannels = %d \n",jj);
                        printf("\t\tFirmware Rev = %d \n\n",(cdsPciModules.dacConfig[ii] & 0xfff));
                }
                if(cdsPciModules.dacType[ii] == GSC_16AISS8AO4)
                {
                        printf("\tDAC %d is a GSC_16AISS8AO4 module\n",ii);
                        if((cdsPciModules.dacConfig[ii] & 0x20000) > 0) jj = 0;
                        else jj = 4;
                        printf("\t\tChannels = %d \n",jj);
                        printf("\t\tFirmware Rev = %d \n\n",(cdsPciModules.dacConfig[ii] & 0xfff));
                }
                if(cdsPciModules.dacType[ii] == GSC_18AO8)
		{
                        printf("\tDAC %d is a GSC_18AO8 module\n",ii);
		}
                if(cdsPciModules.dacType[ii] == GSC_16AO16)
                {
                        printf("\tDAC %d is a GSC_16AO16 module\n",ii);
                        if((cdsPciModules.dacConfig[ii] & 0x10000) == 0x10000) jj = 8;
                        if((cdsPciModules.dacConfig[ii] & 0x20000) == 0x20000) jj = 12;
                        if((cdsPciModules.dacConfig[ii] & 0x30000) == 0x30000) jj = 16;
                        printf("\t\tChannels = %d \n",jj);
                        if((cdsPciModules.dacConfig[ii] & 0xC0000) == 0x0000)
			{
                        	printf("\t\tFilters = None\n");
			}
                        if((cdsPciModules.dacConfig[ii] & 0xC0000) == 0x40000)
			{
                        	printf("\t\tFilters = 10kHz\n");
			}
                        if((cdsPciModules.dacConfig[ii] & 0xC0000) == 0x80000)
			{
                        	printf("\t\tFilters = 100kHz\n");
			}
                        if((cdsPciModules.dacConfig[ii] & 0x100000) == 0x100000)
			{
                        	printf("\t\tOutput Type = Differential\n");
			}
                        printf("\t\tFirmware Rev = %d \n\n",(cdsPciModules.dacConfig[ii] & 0xfff));
                }
#ifdef ADC_MASTER
		ioMemData->model[kk] = cdsPciModules.dacType[ii];
                cdsPciModules.dacConfig[ii]  = kk;
printf("MASTER DAC SLOT %d %d\n",ii,cdsPciModules.dacConfig[ii]);
		kk ++;
#endif
	}
        printf("***************************************************************************\n");
	printf("%d DIO cards found\n",cdsPciModules.dioCount);
        printf("***************************************************************************\n");
	printf("%d IIRO-8 Isolated DIO cards found\n",cdsPciModules.iiroDioCount);
        printf("***************************************************************************\n");
	printf("%d IIRO-16 Isolated DIO cards found\n",cdsPciModules.iiroDio1Count);
        printf("***************************************************************************\n");
	printf("%d Contec 32ch PCIe DO cards found\n",cdsPciModules.cDo32lCount);
	printf("%d Contec PCIe DIO1616 cards found\n",cdsPciModules.cDio1616lCount);
	printf("%d Contec PCIe DIO6464 cards found\n",cdsPciModules.cDio6464lCount);
	printf("%d DO cards found\n",cdsPciModules.doCount);
        printf("***************************************************************************\n");
	printf("%d RFM cards found\n",cdsPciModules.rfmCount);
	for(ii=0;ii<cdsPciModules.rfmCount;ii++)
        {
                 printf("\tRFM %d is a VMIC_%x module with Node ID %d\n", ii, cdsPciModules.rfmType[ii], cdsPciModules.rfmConfig[ii]);
	}
        printf("***************************************************************************\n");
  	if (cdsPciModules.gps) {
	printf("IRIG-B card found %d\n",cdsPciModules.gpsType);
        printf("***************************************************************************\n");
  	}

	// Code will run on internal timer if no ADC modules are found
	if (cdsPciModules.adcCount == 0) {
		printf("No ADC modules found, running on timer\n");
		run_on_timer = 1;
        	//munmap(_epics_shm, MMAP_SIZE);
        	//close(wfd);
        	//return 0;
	}
#ifdef ONE_ADC
	cdsPciModules.adcCount = 1;
#endif

	// Initialize buffer for daqLib.c code
	printf("Initializing space for daqLib buffers\n");
	daqBuffer = (long)&daqArea[0];
 
#ifndef NO_DAQ
	printf("Initializing Network\n");
	numFb = cdsDaqNetInit(2);
	if (numFb <= 0) {
		printf("Couldn't initialize Myrinet network connection\n");
		return 0;
	}
	printf("Found %d frameBuilders on network\n",numFb);
#endif

#ifdef NO_RTL

#ifdef SPECIFIC_CPU
#define CPUID SPECIFIC_CPU
#else 
#define CPUID 1
#endif

        pLocalEpics = (CDS_EPICS *)&((RFM_FE_COMMS *)_epics_shm)->epicsSpace;
        printf("Epics burt restore is %d\n", pLocalEpics->epicsInput.burtRestore);

#ifdef NO_CPU_SHUTDOWN
        struct task_struct *p;
        p = kthread_create(fe_start, 0, "fe_start/%d", CPUID);
        if (IS_ERR(p)){
                printf("Failed to kthread_create()\n");
                return 1;
        }
        kthread_bind(p, CPUID);
        wake_up_process(p);
#endif


#ifndef NO_CPU_SHUTDOWN
	// TODO: add a check to see whether there is already another
	// front-end running on the same CPU. Return an error in that case.
	// 
	extern void set_fe_code_idle(void (*ptr)(void), unsigned int cpu);
        set_fe_code_idle(fe_start, CPUID);
        msleep(100);

	extern int cpu_down(unsigned int);
	cpu_down(CPUID);

	// The code runs on the disabled CPU
#endif

	// Waiting for FE reset
        while(pLocalEpics->epicsInput.vmeReset == 0) {
                msleep(1000);
        }

#ifndef NO_CPU_SHUTDOWN
	// Unset the code callback
        set_fe_code_idle(0, CPUID);
#endif

	// Stop the code and wait
        stop_working_threads = 1;
        msleep(1000);

#ifndef NO_CPU_SHUTDOWN
	// Bring the CPU back up
	extern int __cpuinit cpu_up(unsigned int cpu);
        cpu_up(CPUID);

#endif
        msleep(1000);
        return 0;
#else

        rtl_pthread_attr_init(&attr);
#ifdef RESERVE_CPU2
        rtl_pthread_attr_setcpu_np(&attr, 2);
        rtl_pthread_attr_setreserve_np(&attr, 1);
        rtl_pthread_create(&wthread1, &attr, cpu2_start, 0);
	printf("Started cpu2 task\n");
	usleep(1000000);
#endif

#ifdef RESERVE_CPU3
        rtl_pthread_attr_setcpu_np(&attr, 3);
        rtl_pthread_attr_setreserve_np(&attr, 1);
        rtl_pthread_create(&wthread2, &attr, cpu3_start, 0);
	printf("Started cpu3 task\n");
	usleep(1000000);
#endif

#ifdef SPECIFIC_CPU
        rtl_pthread_attr_setcpu_np(&attr, SPECIFIC_CPU);
#else
        rtl_pthread_attr_setcpu_np(&attr, 1);
#endif
        /* mark this CPU as reserved - only RTLinux runs on it */
        rtl_pthread_attr_setreserve_np(&attr, 1);
        rtl_pthread_create(&wthread, &attr, fe_start, 0);


	// Main thread is now running. Wait here until thread is killed
        // ************************************************************
        rtl_main_wait();

	// Clean up after main thread is dead
#ifndef NO_DAQ
	status = cdsDaqNetClose();
#endif

	//cds_mx_finalize();

	printf("Killing work threads\n");
	stop_working_threads = 1;

        /* kill the threads */
        rtl_pthread_cancel(wthread);
        rtl_pthread_join(wthread, NULL);

#ifdef RESERVE_CPU2
        rtl_pthread_cancel(wthread1);
        rtl_pthread_join(wthread1, NULL);
#endif
#ifdef RESERVE_CPU3
        rtl_pthread_cancel(wthread2);
        rtl_pthread_join(wthread2, NULL);
#endif

        /* unmap the shared memory areas */
        munmap(_epics_shm, MMAP_SIZE);

        /* Note that this is a shared area created with shm_open() - we close
         * it with close(), but use shm_unlink() to actually destroy the area
         */
        close(wfd);

#endif

#ifdef DOLPHIN_TEST
	if (client) {
                sci_unmap_segment(&client_map_handle, 0);
                sci_disconnect_segment(&remote_segment_handle, 0);
	} else {
		sci_set_local_segment_unavailable(segment, 0);
                sci_unexport_segment(segment, 0, 0);
                sci_remove_segment(&segment, 0);
	}
#endif

        return 0;
}

#ifdef NO_RTL
void cleanup_module (void) {
}
#endif
