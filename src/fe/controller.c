/*----------------------------------------------------------------------*/
/*                                                                      */
/*                      -------------------                             */
/*                                                                      */
/*                             LIGO                                     */
/*                                                                      */
/*        THE LASER INTERFEROMETER GRAVITATIONAL WAVE OBSERVATORY.      */
/*                                                                      */
/*                     (C) The LIGO Project, 2012.                      */
/*                                                                      */
/*                                                                      */
/*----------------------------------------------------------------------*/

#include <linux/version.h>
#include <linux/init.h>
#undef printf
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <asm/delay.h>
#include <asm/cacheflush.h>

#include <linux/slab.h>
#define printf printk
#include <drv/cdsHardware.h>
#include "inlineMath.h"

#include </usr/src/linux/arch/x86/include/asm/processor.h>
#include </usr/src/linux/arch/x86/include/asm/cacheflush.h>

char *build_date = __DATE__ " " __TIME__;

extern int iop_rfm_valid;

// Need to get rid of NUM_SYSTEMS
// This is old code to loop thru same code for multiple instances of same device on same CPU
// as was done for HEPI on iLIGO
#ifndef NUM_SYSTEMS
#define NUM_SYSTEMS 1
#endif

#define INLINE  inline
#define MMAP_SIZE (64*1024*1024 - 5000)

volatile char *_epics_shm;		// Ptr to EPICS shared memory
char *_ipc_shm;			// Ptr to inter-process communication area 
#if defined(SHMEM_DAQ)
char *_daq_shm;			// Ptr to frame builder comm shared mem area 
int daq_fd;			// File descriptor to share memory file 
#endif

long daqBuffer;			// Address for daq dual buffers in daqLib.c
CDS_HARDWARE cdsPciModules;	// Structure of PCI hardware addresses
volatile IO_MEM_DATA *ioMemData;
volatile int vmeDone = 0;	// Code kill command
volatile int stop_working_threads = 0;

#define STACK_SIZE    40000
#define TICK_PERIOD   100000
#define PERIOD_COUNT  1
#define MAX_UDELAY    19999


// Code can be run without shutting down CPU by changing this compile flag
#undef NO_CPU_SHUTDOWN 
#ifndef NO_CPU_SHUTDOWN
extern int vprintkl(const char*, va_list);
extern int printkl(const char*, ...);
char fmt1[512];
int printk(const char *fmt, ...) {
    va_list args;
    int r;

    strcat(strcpy(fmt1, SYSTEM_NAME_STRING_LOWER), ": ");
    strcat(fmt1, fmt);
    va_start(args, fmt);
    r = vprintkl(fmt1, args);
    va_end(args);
    return r;
}
#endif

#if 0
int printf(const char *fmt, ...) {
    int r;
    va_list args;
    va_start(args, fmt);
    r = printk(fmt, args);
    va_end(args);
    return r;
}
#endif

#ifdef DOLPHIN_TEST
#include "dolphin.c"
#endif

#include "fm10Gen.h"		// CDS filter module defs and C code
#include "feComms.h"		// Lvea control RFM network defs.
#include "daqmap.h"		// DAQ network layout
#include "controller.h"

extern unsigned int cpu_khz;

#ifndef NO_DAQ
#include "drv/fb.h"
#include "drv/daqLib.c"		// DAQ/GDS connection software
#endif
#include "drv/map.h"		// PCI hardware defs
#include "drv/epicsXfer.c"	// User defined EPICS to/from FE data transfer function
#include "timing.c"		// timing module / IRIG-B  functions

#include "drv/inputFilterModule.h"		
#include "drv/inputFilterModule1.h"		

/// Testpoints which are not part of filter modules
double *testpoint[GDS_MAX_NFM_TP];
#ifndef NO_DAQ
DAQ_RANGE daq;			// Range settings for daqLib.c
int numFb = 0;
int fbStat[2] = {0,0};		// Status of DAQ backend computer
double xExc[GDS_MAX_NFM_EXC];	// GDS EXC not associated with filter modules
#endif
// 1/16 sec cycle counters for DAQS 
int subcycle = 0;		// Internal cycle counter	 
unsigned int daqCycle;		// DAQS cycle counter	

// Sharded memory discriptors
int wfd, ipc_fd;
volatile CDS_EPICS *pLocalEpics;   	// Local mem ptr to EPICS control data


// Filter module variables
/// Standard Filter Module Structure
FILT_MOD dsp[NUM_SYSTEMS];					// SFM structure.	
/// Pointer to local memory SFM Structure
FILT_MOD *dspPtr[NUM_SYSTEMS];					// SFM structure pointer.
/// Pointer to SFM in shared memory.
FILT_MOD *pDsp[NUM_SYSTEMS];					// Ptr to SFM in shmem.	
/// Pointer to filter coeffs local memory.
COEF dspCoeff[NUM_SYSTEMS];					// Local mem for SFM coeffs.
/// Pointer to filter coeffs shared memory.
VME_COEF *pCoeff[NUM_SYSTEMS];					// Ptr to SFM coeffs in shmem	

// ADC Variables
/// Array of ADC values
double dWord[MAX_ADC_MODULES][MAX_ADC_CHN_PER_MOD];		// ADC read values
/// List of ADC channels used by this app. Used to determine if downsampling required.
unsigned int dWordUsed[MAX_ADC_MODULES][MAX_ADC_CHN_PER_MOD];	// ADC chans used by app code
/// Arrary of ADC overflow counters.
int overflowAdc[MAX_ADC_MODULES][MAX_ADC_CHN_PER_MOD];		// ADC overflow diagnostics

// DAC Variables
/// Enables writing of DAC values; Used with DACKILL parts..
int iopDacEnable;						// Returned by feCode to allow writing values or zeros to DAC modules
#ifdef ADC_MASTER
int dacOutBufSize [MAX_DAC_MODULES];	
#endif
/// Array of DAC output values.
double dacOut[MAX_DAC_MODULES][MAX_DAC_CHN_PER_MOD];		// DAC output values
/// DAC output values returned to EPICS
int dacOutEpics[MAX_DAC_MODULES][MAX_DAC_CHN_PER_MOD];		// DAC outputs reported back to EPICS
/// DAC channels used by an app.; determines up sampling required.
unsigned int dacOutUsed[MAX_DAC_MODULES][MAX_DAC_CHN_PER_MOD];	// DAC chans used by app code
/// Array of DAC overflow (overrange) counters.
int overflowDac[MAX_DAC_MODULES][MAX_DAC_CHN_PER_MOD];		// DAC overflow diagnostics
/// DAC outputs stored as floats, to be picked up as test points
double floatDacOut[160]; // DAC outputs stored as floats, to be picked up as test points

/// Counter for total ADC/DAC overflows
int overflowAcc = 0;						// Total ADC/DAC overflow counter

#ifndef ADC_MASTER
// Variables for Digital I/O board values
// DIO board I/O is handled in slave (user) applications for timing reasons (longer I/O access times)
int dioInput[MAX_DIO_MODULES];	
int dioOutput[MAX_DIO_MODULES];
int dioOutputHold[MAX_DIO_MODULES];

int rioInputOutput[MAX_DIO_MODULES];
int rioOutput[MAX_DIO_MODULES];
int rioOutputHold[MAX_DIO_MODULES];

int rioInput1[MAX_DIO_MODULES];
int rioOutput1[MAX_DIO_MODULES];
int rioOutputHold1[MAX_DIO_MODULES];

// Contec 32 bit output modules
unsigned int CDO32Input[MAX_DIO_MODULES];
unsigned int CDO32Output[MAX_DIO_MODULES];

#endif
// Contec 64 input bits plus 64 output bits (Standard for aLIGO)
/// Contec6464 input register values
unsigned int CDIO6464InputInput[MAX_DIO_MODULES]; // Binary input bits
/// Contec6464 output register values read back from the module
unsigned int CDIO6464Input[MAX_DIO_MODULES]; // Current value of the BO bits
/// Contec6464 values to be written to the output register
unsigned int CDIO6464Output[MAX_DIO_MODULES]; // Binary output bits

// This Contect 16 input / 16 output DIO card is used to control timing slave by IOP
unsigned int CDIO1616InputInput[MAX_DIO_MODULES]; // Binary input bits
unsigned int CDIO1616Input[MAX_DIO_MODULES]; // Current value of the BO bits
unsigned int CDIO1616Output[MAX_DIO_MODULES]; // Binary output bits
int tdsControl[3];	// Up to 3 timing control modules allowed in case I/O chassis are daisy chained
int tdsCount = 0;


/// Maintains present cycle count within a one second period.
int cycleNum = 0;
/// Value of readback from DAC FIFO size registers; used in diags for FIFO overflow/underflow.
int out_buf_size = 0; // test checking DAC buffer size
unsigned int cycle_gps_time = 0; // Time at which ADCs triggered
unsigned int cycle_gps_event_time = 0; // Time at which ADCs triggered
unsigned int   cycle_gps_ns = 0;
unsigned int   cycle_gps_event_ns = 0;
unsigned int   gps_receiver_locked = 0; // Lock/unlock flag for GPS time card
unsigned int timeSec = 0;
unsigned int timeSecDiag = 0;
/* 1 - error occured on shmem; 2 - RFM; 3 - Dolphin */
unsigned int ipcErrBits = 0;
int adcTime;			// Used in code cycle timing
int adcHoldTime;		// Stores time between code cycles
int adcHoldTimeMax;		// Stores time between code cycles
int adcHoldTimeEverMax;		// Maximum cycle time recorded
int adcHoldTimeEverMaxWhen;
int startGpsTime;
int adcHoldTimeMin;
int adcHoldTimeAvg;
int adcHoldTimeAvgPerSec;
int usrTime;			// Time spent in user app code
int usrHoldTime;		// Max time spent in user app code
int cardCountErr = 0;
int cycleTime;			// Current cycle time
int timeHold = 0;			// Max code cycle time within 1 sec period
int timeHoldHold = 0;			// Max code cycle time within 1 sec period; hold for another sec
int timeHoldWhen= 0;			// Cycle number within last second when maximum reached; running
int timeHoldWhenHold = 0;		// Cycle number within last second when maximum reached

// The following are for timing histograms written to /proc files
#if defined(SERVO64K) || defined(SERVO32K)
unsigned int cycleHist[32];
unsigned int cycleHistMax[32];
unsigned int cycleHistWhen[32];
unsigned int cycleHistWhenHold[32];
#elif defined(SERVO16K)
unsigned int cycleHist[64];
unsigned int cycleHistMax[64];
unsigned int cycleHistWhen[64];
unsigned int cycleHistWhenHold[64];
#endif


#if defined(SHMEM_DAQ)
struct rmIpcStr *daqPtr;
#endif

int  getGpsTime(unsigned int *tsyncSec, unsigned int *tsyncUsec); 

// Include C code modules
#include "map.c"
#include "fb.c"

char daqArea[2*DAQ_DCU_SIZE];		// Space allocation for daqLib buffers
int cpuId = 1;

// The following are for testing systems where all computers do not have I/O chassis attached.
// Essentially, one computer is designated the time master and sends GPS time to all other
// control computers via RFM networks.
#ifdef TIME_MASTER
volatile unsigned int *rfmTime;
#endif

#ifdef TIME_SLAVE
volatile unsigned int *rfmTime;
#endif

#ifdef IOP_TIME_SLAVE
volatile unsigned int *rfmTime;
#endif


// All systems not running at 64K require up/down sampling to communicate I/O data
// with IOP, which is running at 64K.
// Following defines the filter coeffs for these up/down filters.
#ifdef OVERSAMPLE

#if defined(CORE_BIQUAD)

/* Recalculated filters in biquad form */

/* Oversamping base rate is 64K */
/* Coeffs for the 2x downsampling (32K system) filter */
static double __attribute__ ((unused)) feCoeff2x[9] =
        {0.053628649721183,
	 0.2568759660371100, -0.3225906481359000, 1.2568801238621801, 1.6774135096891700,
	-0.2061764045745400, -1.0941543149527400, 2.0846376586498803, 2.1966597482716801};

/* Coeffs for the 4x downsampling (16K system) filter */
static double __attribute__ ((unused)) feCoeff4x[9] =
	{0.014805052402446,  
	0.7166258547451800, -0.0683289874517300, 0.3031629575762000, 0.5171469569032900,
	0.6838596423885499, -0.2534855521841101, 1.6838609161411500, 1.7447155374502499};
//
// New Brian Lantz 4k decimation filter
static double __attribute__ ((unused)) feCoeff16x[9] =
        {0.010203728365,
	0.8052941009065100, -0.0241751519071000, 0.3920490703701900, 0.5612099784288400,
	0.8339678987936501, -0.0376022631287799, -0.0131581721533700, 0.1145865116421301};

/* Coeffs for the 32x downsampling filter (2K system) */
/* Original Rana coeffs from 40m lab elog */
static double __attribute__ ((unused)) feCoeff32x[9] =
        {0.0001104130574447,
	0.9701834961388200, -0.0010837026165800, -0.0200761119821899, 0.0085463156103800,
	0.9871502388637901, -0.0039246182095299, 3.9871502388637898, 3.9960753817904697};
#else

/* Oversamping base rate is 64K */
/* Coeffs for the 2x downsampling (32K system) filter */
static double __attribute__ ((unused)) feCoeff2x[9] =
        {0.053628649721183,
        -1.25687596603711,    0.57946661417301,    0.00000415782507,    1.00000000000000,
        -0.79382359542546,    0.88797791037820,    1.29081406322442,    1.00000000000000};
/* Coeffs for the 4x downsampling (16K system) filter */
static double __attribute__ ((unused)) feCoeff4x[9] =
	{0.014805052402446,  
	-1.71662585474518,    0.78495484219691,   -1.41346289716898,   0.99893884152400,
	-1.68385964238855,    0.93734519457266,    0.00000127375260,   0.99819981588176};
//
// New Brian Lantz 4k decimation filter
static double __attribute__ ((unused)) feCoeff16x[9] =
        {0.010203728365,
        -1.80529410090651,   0.82946925281361,  -1.41324503053632,   0.99863016087226,
        -1.83396789879365,   0.87157016192243,  -1.84712607094702,   0.99931484571793};
//

#if 0
/* Coeffs for the 32x downsampling filter (2K system) */
/* Original Rana coeffs from 40m lab elog */
static double feCoeff32x[9] =
        {0.0001104130574447,
        -1.97018349613882,    0.97126719875540,   -1.99025960812101,    0.99988962634797,
        -1.98715023886379,    0.99107485707332,    2.00000000000000,    1.00000000000000};
#endif

/* Coeffs for the 32x downsampling filter (2K system) per Brian Lantz May 5, 2009 */
static double __attribute__ ((unused)) feCoeff32x[9] =
	{0.010581064947739,
        -1.90444302586137,    0.91078434629894,   -1.96090276933603,    0.99931924465090,
        -1.92390910024681,    0.93366146580083,   -1.84652529182276,    0.99866506867980};
#endif



// History buffers for oversampling filters
double dHistory[(MAX_ADC_MODULES * 32)][MAX_HISTRY];
double dDacHistory[(MAX_DAC_MODULES * 16)][MAX_HISTRY];
#else

#define OVERSAMPLE_TIMES 1
#endif


// Whether run on internal timer (when no ADC cards found)
int run_on_timer = 0;

// Initial diag reset flag
int initialDiagReset = 1;

// Cache flushing mumbo jumbo suggested by Thomas Gleixner, it is probably useless
// Did not see any effect
  char fp [64*1024];


#ifdef ADC_SLAVE
// Clear DAC channel shared memory map,
// used to keep track of non-overlapping DAC channels among slave models
//
void deallocate_dac_channels(void) {
  int ii, jj;
  for (ii = 0; ii < MAX_DAC_MODULES; ii++) {
    int pd = cdsPciModules.dacConfig[ii] - ioMemData->adcCount;
    for (jj = 0; jj < 16; jj++)
	if (dacOutUsed[ii][jj]) 
	   ioMemData->dacOutUsed[pd][jj] = 0;
  }
}
#endif

//***********************************************************************
// TASK: fe_start()	
// This routine is the skeleton for all front end code	
//***********************************************************************
void *fe_start(void *arg)
{
  int longestWrite2 = 0;
  int tempClock[4];
  int ii,jj,kk,ll,mm;			// Dummy loop counter variables
  static int clock1Min = 0;		// Minute counter (Not Used??)
  static int cpuClock[CPU_TIMER_CNT];	// Code timing diag variables
  static int chanHop = 0;

  int adcData[MAX_ADC_MODULES][MAX_ADC_CHN_PER_MOD];	// ADC raw data
  int adcChanErr[MAX_ADC_MODULES];
  int adcWait = 0;
  int adcOF[MAX_ADC_MODULES];		// ADC overrange counters

  int dacChanErr[MAX_DAC_MODULES];
  int dacOF[MAX_DAC_MODULES];		// DAC overrange counters
  static int dacWriteEnable = 0;	// No DAC outputs until >4 times through code
  					// Code runs longer for first few cycles on startup as it settles in,
					// so this helps prevent long cycles during that time.
  int limit = OVERFLOW_LIMIT_16BIT;      // ADC/DAC overflow test value
  int mask = GSAI_DATA_MASK;            // Bit mask for ADC/DAC read/writes
  int num_outs = MAX_DAC_CHN_PER_MOD;   // Number of DAC channels variable
#ifndef ADC_SLAVE
  volatile int *packedData;		// Pointer to ADC PCI data space
  volatile unsigned int *pDacData;	// Pointer to DAC PCI data space
  int wtmin,wtmax;			// Time window for startup on IRIG-B
  int dacEnable = 0;
  int pBits[9] = {1,2,4,8,16,32,64,128,256};	// Lookup table for quick power of 2 calcs
#endif
  RFM_FE_COMMS *pEpicsComms;		// Pointer to EPICS shared memory space
  int timeHoldMax = 0;			// Max code cycle time since last diag reset
  int myGmError2 = 0;			// Myrinet error variable
  // int attemptingReconnect = 0;		// Myrinet reconnect status
  int status;				// Typical function return value
  float onePps;				// Value of 1PPS signal, if used, for diagnostics
  int onePpsHi = 0;			// One PPS diagnostic check
  int onePpsTime = 0;			// One PPS diagnostic check
#ifdef DIAG_TEST
  float onePpsTest;				// Value of 1PPS signal, if used, for diagnostics
  int onePpsHiTest[10];			// One PPS diagnostic check
  int onePpsTimeTest[10];			// One PPS diagnostic check
#endif
  int dcuId;				// DAQ ID number for this process
  static int missedCycle = 0;		// Incremented error counter when too many values in ADC FIFO
  // int netRetry;				// Myrinet reconnect variable
  int diagWord = 0;			// Code diagnostic bit pattern returned to EPICS
  int system = 0;
  int sampleCount = 1;			// Number of ADC samples to take per code cycle
  int sync21pps = 0;			// Code startup sync to 1PPS flag
  int sync21ppsCycles = 0;		// Number of attempts to sync to 1PPS
  int syncSource = SYNC_SRC_NONE;	// Code startup synchronization source
  int mxStat = 0;			// Net diags when myrinet express is used
  int mxDiag = 0;
  int mxDiagR = 0;
// ****** Share data
  int ioClockDac = DAC_PRELOAD_CNT;
  int ioMemCntr = 0;
  int ioMemCntrDac = DAC_PRELOAD_CNT;
#ifdef ADC_SLAVE
  int memCtr = 0;
  int ioClock = 0;
#endif
#ifdef OVERSAMPLE_DAC
  double dac_in =  0.0;			// DAC value after upsample filtering
#endif
  int dac_out = 0;			// Integer value sent to DAC FIFO

  int feStatus = 0;

  int clk, clk1;			// Used only when run on timer enabled (test mode)

#ifdef ADC_MASTER
  static float duotone[IOP_IO_RATE];		// Duotone timing diagnostic variables
  static float duotoneDac[IOP_IO_RATE];
  float duotoneTimeDac;
  float duotoneTime;
  static float duotoneTotal = 0.0;
  static float duotoneMean = 0.0;
  static float duotoneTotalDac = 0.0;
  static float duotoneMeanDac = 0.0;
  static int dacDuoEnable = 0;

  static int dacBufSelect = 0;			// DAC memory double buffer selector
  volatile GSA_18BIT_DAC_REG *dac18bitPtr;	// Pointer to 16bit DAC memory area
  volatile GSA_DAC_REG *dac16bitPtr;		// Pointer to 18bit DAC memory area
#endif
#ifndef ADC_SLAVE
  unsigned int usec = 0;
  unsigned int offset = 0;
  unsigned int dacBufOffset = 0;
#endif


  int cnt = 0;
  unsigned long cpc;

  memset (tempClock, 0, sizeof(tempClock));

  // Flush L1 cache
  memset (fp, 0, 64*1024);
  memset (fp, 1, 64*1024);
  clflush_cache_range ((void *)fp, 64*1024);

  fz_daz(); // Kill the denorms!

// Set memory for cycle time history diagnostics
#if defined(SERVO64K) || defined(SERVO32K) || defined(SERVO16K)
  memset(cycleHist, 0, sizeof(cycleHist));
  memset(cycleHistMax, 0, sizeof(cycleHistMax));
  memset(cycleHistWhen, 0, sizeof(cycleHistWhen));
  memset(cycleHistWhenHold, 0, sizeof(cycleHistWhenHold));
#endif
// Do all of the initalization ***********************************************************************

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
#ifdef ADC_MASTER
	dacOutBufSize[ii] = 0;
	// Zero out DAC channel map in the shared memory
	// to be used to check on slaves' channel allocation
	ioMemData->dacOutUsed[ii][jj] = 0;
#endif
    }

  // Set pointers to filter module data buffers
    pDsp[system] = (FILT_MOD *)(&pEpicsComms->dspSpace);
    pCoeff[system] = (VME_COEF *)(&pEpicsComms->coeffSpace);
    dspPtr[system] = dsp;

  // Clear the FE reset which comes from Epics
  pLocalEpics->epicsInput.vmeReset = 0;

#ifndef ADC_SLAVE
  // Look for DIO card or IRIG-B Card
  // if Contec 1616 BIO present, TDS slave will be used for timing.
  if(cdsPciModules.cDio1616lCount) syncSource = SYNC_SRC_TDS;
  // if IRIG-B card present, code will use it for startup synchonization
  if(cdsPciModules.gpsType && (syncSource == SYNC_SRC_NONE)) 
	syncSource = SYNC_SRC_IRIG_B;
  	// If no IRIG-B card, will try 1PPS on ADC[0][31] later
  if(syncSource == SYNC_SRC_NONE) 
	syncSource = SYNC_SRC_1PPS;
#else
   // SLAVE processes get sync from ADC_MASTER
   syncSource = SYNC_SRC_MASTER;
#endif

printf("Sync source = %d\n",syncSource);


  // Do not proceed until EPICS has had a BURT restore *******************************
  printf("Waiting for EPICS BURT Restore = %d\n", pLocalEpics->epicsInput.burtRestore);
  cnt = 0;
  do{
	udelay(MAX_UDELAY);
        udelay(MAX_UDELAY);
        udelay(MAX_UDELAY);
  	printf("Waiting for EPICS BURT %d\n", cnt++);
	cpu_relax();
  }while(!pLocalEpics->epicsInput.burtRestore);

  printf("BURT Restore Complete\n");

// BURT has completed *******************************************************************

// Read in all Filter Module EPICS settings
    for(ii=0;ii<MAX_MODULES;ii++)
    {
	updateEpics(ii, dspPtr[0], pDsp[0],
		    &dspCoeff[0], pCoeff[0]);
    }

  // Need this FE dcuId to make connection to FB
  dcuId = pLocalEpics->epicsInput.dcuId;
  pLocalEpics->epicsOutput.dcuId = dcuId;

  // Reset timing diagnostics
  pLocalEpics->epicsOutput.diagWord = 0;
  pLocalEpics->epicsOutput.timeDiag = 0;
  pLocalEpics->epicsOutput.timeErr = syncSource;

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

udelay(1000);

// Initialize DAQ variable/software **************************************************
#if !defined(NO_DAQ) && !defined(IOP_TASK)
  /* Set data range limits for daqLib routine */
#if defined(SERVO2K) || defined(SERVO4K)
  daq.filtExMin = GDS_2K_EXC_MIN;
  daq.filtTpMin = GDS_2K_TP_MIN;
#else
  daq.filtExMin = GDS_16K_EXC_MIN;
  daq.filtTpMin = GDS_16K_TP_MIN;
#endif
  daq.filtExMax = daq.filtExMin + MAX_MODULES;
  daq.filtExSize = MAX_MODULES;
  daq.xExMin = daq.filtExMax;
  daq.xExMax = daq.xExMin + GDS_MAX_NFM_EXC;
  daq.filtTpMax = daq.filtTpMin + MAX_MODULES * 3;
  daq.filtTpSize = MAX_MODULES * 3;
  daq.xTpMin = daq.filtTpMax;
  daq.xTpMax = daq.xTpMin + GDS_MAX_NFM_TP;

  printf("DAQ Ex Min/Max = %d %d\n",daq.filtExMin,daq.filtExMax);
  printf("DAQ XEx Min/Max = %d %d\n",daq.xExMin,daq.xExMax);
  printf("DAQ Tp Min/Max = %d %d\n",daq.filtTpMin,daq.filtTpMax);
  printf("DAQ XTp Min/Max = %d %d\n",daq.xTpMin,daq.xTpMax);

  // Assign DAC testpoint pointers
  for (ii = 0; ii <  cdsPciModules.dacCount; ii++)
	for (jj = 0; jj < MAX_DAC_CHN_PER_MOD; jj++) // 16 per DAC regardless of the actual
		testpoint[MAX_DAC_CHN_PER_MOD * ii + jj] = floatDacOut + MAX_DAC_CHN_PER_MOD * ii + jj;

  // Zero out storage
  memset(floatDacOut, 0, sizeof(floatDacOut));

#endif
  pLocalEpics->epicsOutput.diags[FE_DIAGS_IPC_STAT] = 0;
  pLocalEpics->epicsOutput.diags[FE_DIAGS_FB_NET_STAT] = 0;
  pLocalEpics->epicsOutput.tpCnt = 0;

#if !defined(NO_DAQ) && !defined(IOP_TASK)
  // Initialize DAQ function
  status = daqWrite(0,dcuId,daq,DAQ_RATE,testpoint,dspPtr[0],0, (int *)(pLocalEpics->epicsOutput.gdsMon),xExc);
  if(status == -1) 
  {
    printf("DAQ init failed -- exiting\n");
    vmeDone = 1;
    return(0);
  }
#endif

#ifndef ADC_MASTER
	// SLAVE units read/write their own DIO
	// MASTER units ignore DIO for speed reasons
	// Read Dio card initial values *************************************
     for(kk=0;kk<cdsPciModules.doCount;kk++)
     {
	ii = cdsPciModules.doInstance[kk];
	if(cdsPciModules.doType[kk] == ACS_8DIO)
	{
	  rioInputInput[ii] = readIiroDio(&cdsPciModules, kk) & 0xff;
	  rioInputOutput[ii] = readIiroDioOutput(&cdsPciModules, kk) & 0xff;
	  rioOutputHold[ii] = -1;
	} else if(cdsPciModules.doType[kk] == ACS_16DIO) {
  	  rioInput1[ii] = readIiroDio1(&cdsPciModules, kk) & 0xffff;
	  rioOutputHold1[ii] = -1;
	} else if (cdsPciModules.doType[kk] == CON_32DO) {
  	  CDO32Input[ii] = readCDO32l(&cdsPciModules, kk);
	} else if (cdsPciModules.doType[kk] == CON_1616DIO) {
  	  CDIO1616Input[ii] = readCDIO1616l(&cdsPciModules, kk);
	} else if (cdsPciModules.doType[kk] == CON_6464DIO) {
  	  CDIO6464Input[ii] = readCDIO6464l(&cdsPciModules, kk);
	} else if (cdsPciModules.doType[kk] == CDI64) {
  	  CDIO6464Input[ii] = readCDIO6464l(&cdsPciModules, kk);
	} else if(cdsPciModules.doType[kk] == ACS_24DIO) {
  	  dioInput[ii] = readDio(&cdsPciModules, kk);
	}
     }
#endif


  // Clear the code exit flag
  vmeDone = 0;

  // Initialize user application software ************************************
  printf("Calling feCode() to initialize\n");
  iopDacEnable = feCode(cycleNum,dWord,dacOut,dspPtr[0],&dspCoeff[0], (struct CDS_EPICS *)pLocalEpics,1);

#ifndef ADC_MASTER
  // See if my DAC channel map overlaps with already running models
  for (ii = 0; ii < cdsPciModules.dacCount; ii++) {
    int pd = cdsPciModules.dacConfig[ii] - ioMemData->adcCount; // physical DAC number
    for (jj = 0; jj < 16; jj++) {
	if (dacOutUsed[ii][jj]) {
	  if (ioMemData->dacOutUsed[pd][jj])  {
    		vmeDone = 1;
   		printf("Failed to allocate DAC channel.\n");
		printf("DAC local %d global %d channel %d is already allocated.\n", ii, pd, jj);
	  }
    	}
    }
  }
  if (vmeDone) {
     	return(0);
  } else {
    for (ii = 0; ii < cdsPciModules.dacCount; ii++) {
      int pd = cdsPciModules.dacConfig[ii] - ioMemData->adcCount; // physical DAC number
      for (jj = 0; jj < MAX_DAC_CHN_PER_MOD; jj++) {
	if (dacOutUsed[ii][jj]) {
	  	ioMemData->dacOutUsed[pd][jj] = 1;
		printf("Setting card local=%d global = %d channel=%d dac usage\n", ii, pd, jj);
	}
      }
    }
  }
#endif

  printf("entering the loop\n");

  // Clear a couple of timing diags.
  adcHoldTime = 0;
  adcHoldTimeMax = 0;
  adcHoldTimeEverMax = 0;
  adcHoldTimeEverMaxWhen = 0;
  startGpsTime = 0;
  adcHoldTimeMin = 0xffff;
  adcHoldTimeAvg = 0;
  usrHoldTime = 0;
  missedCycle = 0;

  // Initialize the ADC 
#ifndef ADC_SLAVE
  for(jj=0;jj<cdsPciModules.adcCount;jj++)
  {
	  // Setup the DMA registers
	  status = gsaAdcDma1(jj,cdsPciModules.adcType[jj]);
	  // Preload input memory with dummy variables to test that new ADC data has arrived.
	  packedData = (int *)cdsPciModules.pci_adc[jj];
	  // Write a dummy 0 to first ADC channel location
	  // This location should never be zero when the ADC writes data as it should always
	  // have an upper bit set indicating channel 0.
          *packedData = 0x0;
          if (cdsPciModules.adcType[jj] == GSC_18AISS6C)  packedData += 5;
          else packedData += 31;
	  // Write a number into the last channel which the ADC should never write ie no
	  // upper bits should be set in channel 31.
          *packedData = DUMMY_ADC_VAL;
	  // Set ADC Present Flag
  	  pLocalEpics->epicsOutput.statAdc[jj] = 1;
  }
  printf("ADC setup complete \n");
#endif
#ifdef ADC_SLAVE
  for(jj=0;jj<cdsPciModules.adcCount;jj++)
  {
  	  pLocalEpics->epicsOutput.statAdc[jj] = 1;
  }
#endif

#ifndef ADC_SLAVE
  // Initialize the DAC module variables
  for(jj = 0; jj < cdsPciModules.dacCount; jj++) {
  	pLocalEpics->epicsOutput.statDac[jj] = DAC_FOUND_BIT;
        pDacData = (unsigned int *) cdsPciModules.pci_dac[jj];
	// Arm DAC DMA for full data size
	status = gsaDacDma1(jj, cdsPciModules.dacType[jj]);
  }
  printf("DAC setup complete \n");
#endif

  

#ifndef ADC_SLAVE
  if (!run_on_timer) {
  switch(syncSource)
  {
	case SYNC_SRC_TDS:
		// Turn off the ADC/DAC clocks
		for(ii=0;ii<tdsCount;ii++)
		{
		CDIO1616Output[ii] = TDS_STOP_CLOCKS;
		CDIO1616Input[ii] = writeCDIO1616l(&cdsPciModules, tdsControl[ii], CDIO1616Output[ii]);
		printf("writing BIO %d\n",tdsControl[ii]);
		}
		udelay(MAX_UDELAY);
		udelay(MAX_UDELAY);
		// Arm ADC modules
    		gsaAdcTrigger(cdsPciModules.adcCount,cdsPciModules.adcType);
		// Arm DAC outputs
		gsaDacTrigger(&cdsPciModules);
		// Set synched flag so later code will not check for 1PPS
		sync21pps = 1;
		udelay(MAX_UDELAY);
		udelay(MAX_UDELAY);
		for(jj=0;jj<cdsPciModules.dacCount;jj++)
		{       
			if(cdsPciModules.dacType[jj] == GSC_18AO8)
			{       
				dac18bitPtr = (volatile GSA_18BIT_DAC_REG *)(dacPtr[jj]);
				for(ii=0;ii<GSAO_18BIT_PRELOAD;ii++) dac18bitPtr->OUTPUT_BUF = 0;
			}else{  
				dac16bitPtr = dacPtr[jj];
				printf("writing DAC %d\n",jj);
				for(ii=0;ii<GSAO_16BIT_PRELOAD;ii++) dac16bitPtr->ODB = 0;
			}       
		}       
		// Start ADC/DAC clocks
		// CDIO1616Output[tdsControl] = 0x7B00000;
		for(ii=0;ii<tdsCount;ii++)
		{
		// CDIO1616Output[ii] = TDS_START_ADC_NEG_DAC_POS;
		CDIO1616Output[ii] = TDS_START_ADC_NEG_DAC_POS | TDS_NO_DAC_DUOTONE;
		CDIO1616Input[ii] = writeCDIO1616l(&cdsPciModules, tdsControl[ii], CDIO1616Output[ii]);
		}
		break;
	case SYNC_SRC_IRIG_B:
		 // If IRIG-B card present, use it for startup synchronization
		wtmax = 20;
		wtmin = 5;
		printf("Waiting until usec = %d to start the ADCs\n", wtmin);
		do{
			if(cdsPciModules.gpsType == SYMCOM_RCVR) gps_receiver_locked = getGpsTime(&timeSec,&usec);
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
		printf("Running time %d us\n", usec);
	  	printf("Triggered the DAC\n");
		break;
	case SYNC_SRC_1PPS:
		for(jj=0;jj<cdsPciModules.dacCount;jj++)
		{       
			if(cdsPciModules.dacType[jj] == GSC_18AO8)
			{       
				dac18bitPtr = (volatile GSA_18BIT_DAC_REG *)(dacPtr[jj]);
				for(ii=0;ii<GSAO_18BIT_PRELOAD;ii++) dac18bitPtr->OUTPUT_BUF = 0;
			}else{  
				dac16bitPtr = dacPtr[jj];
				printf("writing DAC %d\n",jj);
				for(ii=0;ii<GSAO_16BIT_PRELOAD;ii++) dac16bitPtr->ODB = 0;
			}       
		}       
		// Arm ADC modules
		gsaAdcTrigger(cdsPciModules.adcCount,cdsPciModules.adcType);
		// Start clocking the DAC outputs
		// gsaDacTrigger(&cdsPciModules);
		break;
	default: {
		    // IRIG-B card not found, so use CPU time to get close to 1PPS on startup
			// Pause until this second ends
#ifdef TIME_SLAVE
// sync up with the time master on its gps time via Dolphin RFM
// This is NOT a normal operating mode.
  waitDolphintime();
#endif

#ifdef RFM_TIME_SLAVE
// sync up with the time master on its gps time via GEFANUC RFM
// This is NOT a normal operating mode.
	  for(;((volatile long *)(cdsPciModules.pci_rfm[0]))[0] != 0;) udelay(1);
	  timeSec = ((volatile long *)(cdsPciModules.pci_rfm[0]))[1];
	  timeSec --;
#endif

		break;
	}
  }
  }


  if (run_on_timer) {	// NOT normal operating mode; used for test systems without I/O cards/chassis
    printf("*******************************\n");
    printf("*     Running on timer!       *\n");
    printf("*******************************\n");
  } else {
    printf("Triggered the ADC\n");
  }
#endif
#ifdef ADC_SLAVE
	// SLAVE needs to sync with MASTER by looking for cycle 0 count in ipc memory
	// Find memory buffer of first ADC to be used in SLAVE application.
        ll = cdsPciModules.adcConfig[0];
        printf("waiting to sync %d\n",ioMemData->iodata[ll][0].cycle);
        rdtscl(cpuClock[CPU_TIME_CYCLE_START]);

 	if (boot_cpu_has(X86_FEATURE_MWAIT)) {
	  for(;;) {
	    if (ioMemData->iodata[ll][0].cycle == 0) break;
	    __monitor((void *)&(ioMemData->iodata[ll][0].cycle), 0, 0);
	    if (ioMemData->iodata[ll][0].cycle == 0) break;
	    __mwait(0, 0);
	  }
 	} else {
	  // Spin until cycle 0 detected in first ADC buffer location.
          do {
		udelay(1);
          } while(ioMemData->iodata[ll][0].cycle != 0);
	}
        timeSec = ioMemData->iodata[ll][0].timeSec;

        rdtscl(cpuClock[CPU_TIME_CYCLE_END]);
        cycleTime = (cpuClock[CPU_TIME_CYCLE_END] - cpuClock[CPU_TIME_CYCLE_START])/CPURATE;
        printf("Synched %d\n",cycleTime);
	// Need to set sync21pps so code will not try to sync with 1pps pulse later.
	sync21pps=1;
	// Get GPS seconds from MASTER
	timeSec = ioMemData->gpsSecond;
        pLocalEpics->epicsOutput.timeDiag = timeSec;
	// Decrement GPS seconds as it will be incremented on first read cycle.
	timeSec --;

#endif
  onePpsTime = cycleNum;
  timeSec = current_time() -1;
#ifdef TIME_SLAVE
	timeSec = *rfmTime;
#endif

  rdtscl(adcTime);

  // **********************************************************************************************
  // Enter the infinite FE control loop  **********************************************************
  // **********************************************************************************************
  // Calculate how many CPU cycles per code cycle
  cpc = cpu_khz * 1000;
  cpc /= CYCLE_PER_SECOND;

  while(!vmeDone){ 	// Run forever until user hits reset
  	if (run_on_timer) {  // NO ADC present, so run on CPU realtime clock
	  // Pause until next cycle begins
	  if (cycleNum == 0) {
	    	//printf("awgtpman gps = %d local = %d\n", pEpicsComms->padSpace.awgtpman_gps, timeSec);
	  	pLocalEpics->epicsOutput.diags[FE_DIAGS_AWGTPMAN] = (pEpicsComms->padSpace.awgtpman_gps != timeSec);
	  }
#if !defined (TIME_SLAVE) && !defined (RFM_TIME_SLAVE)
	  // This is local CPU timer (no ADCs)
	  // advance to the next cycle polling CPU cycles and microsleeping
	  rdtscl(clk);
	  clk += cpc;
	  for(;;) {
	  	rdtscl(clk1);
		if (clk1 >= clk) break;
		udelay(1);
	  }
#else
#ifdef RFM_TIME_SLAVE
	  unsigned long d = ((volatile long *)(cdsPciModules.pci_rfm[0]))[0];
	  d=cycleNum;
	  for(;((volatile long *)(cdsPciModules.pci_rfm[0]))[0] != d;) udelay(1);
	  timeSec = ((volatile long *)(cdsPciModules.pci_rfm[0]))[1];
#elif defined(TIME_SLAVE)
	  // sync up with the time master on its gps time
	  //
	  unsigned long d = 0;
          //if (iop_rfm_valid) d = cdsPciModules.dolphin[0][1];
	  d = cycleNum;
	  for(;iop_rfm_valid? cdsPciModules.dolphin[0][1] != d: 0;) udelay(1);
#endif
	    ioMemCntr = (cycleNum % IO_MEMORY_SLOTS);
	    for(ii=0;ii<IO_MEMORY_SLOT_VALS;ii++)
	    {
		ioMemData->iodata[0][ioMemCntr].data[ii] = cycleNum/4;
		ioMemData->iodata[1][ioMemCntr].data[ii] = cycleNum/4;
	    }
	    // Write GPS time and cycle count as indicator to slave that adc data is ready
	    ioMemData->gpsSecond = timeSec;
	    ioMemData->iodata[0][ioMemCntr].timeSec = timeSec;;
	    ioMemData->iodata[1][ioMemCntr].timeSec = timeSec;;
	    ioMemData->iodata[0][ioMemCntr].cycle = cycleNum;
	    ioMemData->iodata[1][ioMemCntr].cycle = cycleNum;
#endif
	  rdtscl(cpuClock[CPU_TIME_CYCLE_START]);
#if defined(RFM_TIME_SLAVE) || defined(TIME_SLAVE)
#ifndef ADC_SLAVE
	  // Write GPS time and cycle count as indicator to slave that adc data is ready
	  ioMemData->gpsSecond = timeSec;
	  ioMemData->iodata[0][0].cycle = cycleNum;
	  #ifndef RFM_TIME_SLAVE
          if(cycleNum == 0) timeSec++;
	  #endif
#endif
#endif
         if(cycleNum == 0) {

#ifdef TIME_SLAVE
	  if (iop_rfm_valid) timeSec = *rfmTime;
	//*((volatile unsigned int *)dolphin_memory) = timeSec ++;
#else
	  // Increment GPS second on cycle 0
          timeSec ++;
#endif
          pLocalEpics->epicsOutput.timeDiag = timeSec;
	  }
	} else {
	// **********************************************************************************************************
	// NORMAL OPERATION -- Wait for ADC data ready
	// On startup, only want to read one sample such that first cycle
	// coincides with GPS 1PPS. Thereafter, sampleCount will be 
	// increased to appropriate number of 65536 s/sec to match desired
	// code rate eg 32 samples each time thru before proceeding to match 2048 system.
	// **********************************************************************************************************
#ifdef ADC_MASTER
#ifndef RFM_DIRECT_READ
// Used in block transfers of data from GEFANUC RFM
// Want to start the DMA ASAP, before ADC data starts coming in.
// Note that data only xferred every 4th cycle of IOP, so max data rate on RFM is 16K.
	if((cycleNum % 4) == 0)
	{
		if (cdsPciModules.pci_rfm[0]) rfm55DMA(&cdsPciModules,0,(cycleNum % IPC_BLOCKS));
		if (cdsPciModules.pci_rfm[1]) rfm55DMA(&cdsPciModules,1,(cycleNum % IPC_BLOCKS));
	}
#endif
#endif
       if(cycleNum == 0)
        {
	  //printf("awgtpman gps = %d local = %d\n", pEpicsComms->padSpace.awgtpman_gps, timeSec);
	  pLocalEpics->epicsOutput.diags[FE_DIAGS_AWGTPMAN] = (pEpicsComms->padSpace.awgtpman_gps != timeSec);
	  if(pLocalEpics->epicsOutput.diags[FE_DIAGS_AWGTPMAN]) feStatus |= FE_ERROR_AWG;
	  if(!iopDacEnable) feStatus |= FE_ERROR_DAC_ENABLE;

	  // Increment GPS second on cycle 0
#ifndef ADC_SLAVE
          timeSec ++;
          pLocalEpics->epicsOutput.timeDiag = timeSec;
	  // printf("cycle = %d  time = %d\n",cycleNum,timeSec);
#endif
#ifdef TIME_MASTER
	  // Update time on RFM if this is GPS time master
          rdtscl(tempClock[0]);
	  *rfmTime = timeSec;
	  //sci_flush(&sci_dev_info, 0);
          rdtscl(tempClock[1]);
  	  clflush_cache_range (cdsPciModules.dolphin[1], 8);
#endif
	}
        for(ll=0;ll<sampleCount;ll++)
        {
#ifndef ADC_SLAVE
// Start of ADC Read *************************************************************************************
		// Read ADC data
               for(jj=0;jj<cdsPciModules.adcCount;jj++)
		{
		    // Check ADC DMA has completed
		    // This is detected when last channel in memory no longer contains the
		    // dummy variable written during initialization and reset after the read.
		    packedData = (int *)cdsPciModules.pci_adc[jj];
                    if (cdsPciModules.adcType[jj] == GSC_18AISS6C) packedData += 5;
               	    else packedData += 31;
                    kk = 0;
		
		    rdtscl(cpuClock[CPU_TIME_RDY_ADC]);

#if 0
		    // Monitor the first ADC
		    if (jj == 0) {
		       for (;;) {
		         if (*packedData != DUMMY_ADC_VAL) break;
			 __monitor((void *)packedData, 0, 0);
		         if (*packedData != DUMMY_ADC_VAL) break;
			 __mwait(0, 0);
		       }
		       rdtscl(cpuClock[9]);
		       adcWait = (cpuClock[9] - cpuClock[8])/CPURATE;
		    } else 
#endif
                    do {
                        kk ++;
			// Need to delay if not ready as constant banging of the input register
			// will slow down the ADC DMA.
			if(*packedData == DUMMY_ADC_VAL) {
				// if(jj==0) usleep(0);
		    		rdtscl(cpuClock[CPU_TIME_ADC_WAIT]);
				adcWait = (cpuClock[CPU_TIME_ADC_WAIT] - cpuClock[CPU_TIME_RDY_ADC])/CPURATE;
			}
			// Allow 1msec for data to be ready (should never take that long).
                    }while((*packedData == DUMMY_ADC_VAL) && (adcWait < MAX_ADC_WAIT));

		    // If data not ready in time, abort
		    // Either the clock is missing or code is running too slow and ADC FIFO
		    // is overflowing.
		    if (adcWait >= MAX_ADC_WAIT) {
			pLocalEpics->epicsOutput.stateWord = FE_ERROR_ADC;
	  		pLocalEpics->epicsOutput.diagWord |= ADC_TIMEOUT_ERR;
                        stop_working_threads = 1;
			vmeDone = 1;
                        printf("timeout %d %d \n",jj,adcWait);
#if 0
			// Commented out debugging code
			printk("register BCR = 0x%x\n",fadcPtr[0]->BCR);
			if (fadcPtr[0]->BCR & (1 << 15)) printk("input buffer overflow\n");
			if (fadcPtr[0]->BCR & (1 << 16)) printk("input buffer underflow\n");
#endif
			continue;
		    }
		    if(jj == 0) 
		    {
			// Capture cpu clock for cpu meter diagnostics
			rdtscl(cpuClock[CPU_TIME_CYCLE_START]);
#ifdef ADC_MASTER
			// if(cycleNum == 65535) 
			if(cycleNum == 0) 
			{
				// if SymCom type, just do write to lock current time and read later
				// This save a couple three microseconds here
				if(cdsPciModules.gpsType == SYMCOM_RCVR) lockGpsTime();
				if(cdsPciModules.gpsType == TSYNC_RCVR) 
				{
					// Reading second info will lock the time register, allowing
					// nanoseconds to be read later (on next cycle). Two step process used to 
					// save CPU time here, as each read can take 2usec or more.
					timeSec = getGpsSecTsync();
				}
#ifdef IOP_TIME_SLAVE_RFM
	  timeSec = ((volatile long *)(cdsPciModules.pci_rfm[0]))[1];
	  timeSec ++;
#endif
			}
			
#ifdef IOP_TIME_SLAVE
	  		if (iop_rfm_valid) timeSec = *rfmTime;
#endif
#endif
		    }
#ifdef TIME_MASTER
		if (iop_rfm_valid) {
			//*rfmTime = timeSec;
          		rdtscl(tempClock[2]);
			cdsPciModules.dolphin[1][1] = cycleNum;
	  		//sci_flush(&sci_dev_info, 0);
          		rdtscl(tempClock[3]);
  			clflush_cache_range (cdsPciModules.dolphin[1] + 1, 8);
		}
		if (cdsPciModules.pci_rfm[0]) {
		    	((volatile long *)(cdsPciModules.pci_rfm[0]))[1] = timeSec;
  			clflush_cache_range (((volatile long *)(cdsPciModules.pci_rfm[0])) + 1, 8);
		    	((volatile long *)(cdsPciModules.pci_rfm[0]))[0] = cycleNum;
  			clflush_cache_range (((volatile long *)(cdsPciModules.pci_rfm[0])), 8);
		}
		if (cdsPciModules.pci_rfm[1]) {
		    	((volatile long *)(cdsPciModules.pci_rfm[1]))[1] = timeSec;
  			clflush_cache_range (((volatile long *)(cdsPciModules.pci_rfm[1])) + 1, 8);
		    	((volatile long *)(cdsPciModules.pci_rfm[1]))[0] = cycleNum;
  			clflush_cache_range (((volatile long *)(cdsPciModules.pci_rfm[1])), 8);
		}
#endif

                    // Read adc data
                    packedData = (int *)cdsPciModules.pci_adc[jj];
		    // First, and only first, channel should have upper bit marker set.
                    // Return 0x10 if first ADC channel does not have sync bit set
                    if(!(*packedData & ADC_1ST_CHAN_MARKER)) 
		    {
  			 adcChanErr[jj] = 1;
			 chanHop = 1;
	  	 	 pLocalEpics->epicsOutput.stateWord |= FE_ERROR_ADC;
	 	    }	

                    limit = OVERFLOW_LIMIT_16BIT;
                    if (cdsPciModules.adcType[jj] == GSC_18AISS6C) {
			limit = OVERFLOW_LIMIT_18BIT; // 18 bit limit
			offset = GSAF_DATA_CODE_OFFSET; // Data coding offset in 18-bit DAC
			mask = GSAF_DATA_MASK;
			num_outs = GSAF_CHAN_COUNT;
                    } else {
		    	// Various ADC models have different number of channels/data bits
                    	offset = GSAI_DATA_CODE_OFFSET;
                    	mask = GSAI_DATA_MASK;
                    	num_outs = GSAI_CHAN_COUNT;
		    }
#ifdef ADC_MASTER
		    // Determine next ipc memory location to load ADC data
		    ioMemCntr = (cycleNum % IO_MEMORY_SLOTS);
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
			// Load adc value into ipc memory buffer
			ioMemData->iodata[jj][ioMemCntr].data[ii] = adcData[jj][ii];
#endif
#ifdef OVERSAMPLE
			// Downsample filter only used channels to save time
			// This is defined in user C code
			if (dWordUsed[jj][ii]) {
#ifdef CORE_BIQUAD
				dWord[jj][ii] = iir_filter_biquad(dWord[jj][ii],FE_OVERSAMPLE_COEFF,2,&dHistory[ii+jj*MAX_ADC_CHN_PER_MOD][0]);
#else
				dWord[jj][ii] = iir_filter(dWord[jj][ii],FE_OVERSAMPLE_COEFF,2,&dHistory[ii+jj*MAX_ADC_CHN_PER_MOD][0]);
#endif
			}
#endif
			packedData ++;
                    }
#ifdef ADC_MASTER
		    // Write GPS time and cycle count as indicator to slave that adc data is ready
	  	    ioMemData->gpsSecond = timeSec;;
		    ioMemData->iodata[jj][ioMemCntr].timeSec = timeSec;;
		    ioMemData->iodata[jj][ioMemCntr].cycle = cycleNum;
#endif

		    // Check for ADC overflows
                    for(ii=0;ii<num_outs;ii++)
                    {
			if((adcData[jj][ii] > limit) || (adcData[jj][ii] < -limit))
			  {
				overflowAdc[jj][ii] ++;
				overflowAcc ++;
				adcOF[jj] = 1;
			  }
                    }

		   // Clear out last ADC data read for test on next cycle
                   packedData = (int *)cdsPciModules.pci_adc[jj];
                   *packedData = 0x0;

                   if (cdsPciModules.adcType[jj] == GSC_18AISS6C) packedData += GSAF_CHAN_COUNT_M1;
                   else packedData += GSAI_CHAN_COUNT_M1;

                   *packedData = DUMMY_ADC_VAL;

#ifdef DIAG_TEST
// For DIAGS ONLY !!!!!!!!
// This will change ADC DMA BYTE count
// -- Greater than normal will result in channel hopping.
// -- Less than normal will result in ADC timeout.
// In both cases, real-time kernel code should exit with errors to dmesg
          	   if(pLocalEpics->epicsInput.bumpAdcRd != 0) {
		   	gsaAdcDmaBump(jj,pLocalEpics->epicsInput.bumpAdcRd);
		   	pLocalEpics->epicsInput.bumpAdcRd = 0;
		   }
#endif
		   // Reset DMA Start Flag
		   // This allows ADC to dump next data set whenever it is ready
		   gsaAdcDma2(jj);
            }
#endif


		  // Try synching to 1PPS on ADC[0][31] if not using IRIG-B or TDS
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
		// SLAVE gets its adc data from MASTER via ipc shared memory
               for(jj=0;jj<cdsPciModules.adcCount;jj++)
		{
        		mm = cdsPciModules.adcConfig[jj];
                        kk = 0;
		    		rdtscl(cpuClock[CPU_TIME_RDY_ADC]);
                        do{
                                // usleep(1);
		    		rdtscl(cpuClock[CPU_TIME_ADC_WAIT]);
				adcWait = (cpuClock[CPU_TIME_ADC_WAIT] - cpuClock[CPU_TIME_RDY_ADC])/CPURATE;
                                // kk++;
                        }while((ioMemData->iodata[mm][ioMemCntr].cycle != ioClock) && (adcWait < MAX_ADC_WAIT_SLAVE));
			timeSec = ioMemData->iodata[mm][ioMemCntr].timeSec;

                        if(adcWait >= MAX_ADC_WAIT_SLAVE)
                        {
                                printf ("ADC TIMEOUT %d %d %d %d\n",mm,ioMemData->iodata[mm][ioMemCntr].cycle, ioMemCntr,ioClock);
				pLocalEpics->epicsOutput.stateWord = FE_ERROR_ADC;
	  			pLocalEpics->epicsOutput.diagWord |= ADC_TIMEOUT_ERR;
				deallocate_dac_channels();
  				return (void *)-1;
                        }
                        for(ii=0;ii<MAX_ADC_CHN_PER_MOD;ii++)
                        {
                                adcData[jj][ii] = ioMemData->iodata[mm][ioMemCntr].data[ii];
#ifdef FLIP_SIGNALS
                                adcData[jj][ii] *= -1;
#endif
                                dWord[jj][ii] = adcData[jj][ii];
#ifdef OVERSAMPLE
				if (dWordUsed[jj][ii]) {
#ifdef CORE_BIQUAD
					dWord[jj][ii] = iir_filter_biquad(dWord[jj][ii],FE_OVERSAMPLE_COEFF,2,&dHistory[ii+jj*32][0]);
#else
					dWord[jj][ii] = iir_filter(dWord[jj][ii],FE_OVERSAMPLE_COEFF,2,&dHistory[ii+jj*32][0]);
#endif
				}
			if((adcData[jj][ii] > limit) || (adcData[jj][ii] < -limit))
			  {
				overflowAdc[jj][ii] ++;
				overflowAcc ++;
				adcOF[jj] = 1;
			  }
#endif
                                // No filter  dWord[kk][ll] = adcData[kk][ll];
                        }
                }
		// Set counters for next read from ipc memory
                ioClock = (ioClock + 1) % IOP_IO_RATE;
                ioMemCntr = (ioMemCntr + 1) % IO_MEMORY_SLOTS;
        	rdtscl(cpuClock[CPU_TIME_CYCLE_START]);

#endif
	}

	// After first synced ADC read, must set to code to read number samples/cycle
        sampleCount = OVERSAMPLE_TIMES;
	}
// End of ADC Read **************************************************************************************

// Call the front end specific software *****************************************************************
        rdtscl(cpuClock[CPU_TIME_USR_START]);
 	iopDacEnable = feCode(cycleNum,dWord,dacOut,dspPtr[0],&dspCoeff[0],(struct CDS_EPICS *)pLocalEpics,0);
	
        rdtscl(cpuClock[CPU_TIME_USR_END]);


// START OF DAC WRITE ***********************************************************************************
#ifdef ADC_MASTER
	// Following was put in before DAC FIFO was used as buffer by preloading; Purpose was to ensure
	// data xfer to DAC was not happening same time this code was writing data to the DAC transfer
	// memory space; Can be removed at some point
	   dacBufSelect = (dacBufSelect + 1) % 2;
	   dacBufOffset = dacBufSelect * 0x100;
	   dacBufOffset = 0;
#endif
	// Write out data to DAC modules
	for(jj=0;jj<cdsPciModules.dacCount;jj++)
	{
#ifdef ADC_SLAVE
	   // SLAVE writes to MASTER via ipc memory
	   mm = cdsPciModules.dacConfig[jj];
#else
	   // Point to DAC memory buffer
	   pDacData = (unsigned int *)(cdsPciModules.pci_dac[jj] + dacBufOffset);
#endif
#ifdef ADC_MASTER
	// locate the proper DAC memory block
	mm = cdsPciModules.dacConfig[jj];
	// Determine if memory block has been set with the correct cycle count by Slave app.
	if(ioMemData->iodata[mm][ioMemCntrDac].cycle == ioClockDac)
	{
		dacEnable |= pBits[jj];
	}else {
		dacEnable &= ~(pBits[jj]);
		dacChanErr[jj] += 1;
	}
#endif
// If code is to run <64K rate, need to upsample from code rate to 64K
#ifdef OVERSAMPLE_DAC
	   for (kk=0; kk < OVERSAMPLE_TIMES; kk++) {
#else
		kk = 0;
#endif
		limit = OVERFLOW_LIMIT_16BIT;
		mask = GSAO_16BIT_MASK;
		num_outs = GSAO_16BIT_CHAN_COUNT;
		if (cdsPciModules.dacType[jj] == GSC_18AO8) {
			limit = OVERFLOW_LIMIT_18BIT; // 18 bit limit
			mask = GSAO_18BIT_MASK;
			num_outs = GSAO_18BIT_CHAN_COUNT;


		}
		for (ii=0; ii < num_outs; ii++)
		{
#ifdef FLIP_SIGNALS
			dacOut[jj][ii] *= -1;
#endif
#ifdef OVERSAMPLE_DAC
			if (dacOutUsed[jj][ii]) {
#ifdef NO_ZERO_PAD
		 	  dac_in = dacOut[jj][ii];
#ifdef CORE_BIQUAD
		 	  dac_in = iir_filter_biquad(dac_in,FE_OVERSAMPLE_COEFF,2,&dDacHistory[ii+jj*MAX_DAC_CHN_PER_MOD][0]);
#else
		 	  dac_in = iir_filter(dac_in,FE_OVERSAMPLE_COEFF,2,&dDacHistory[ii+jj*MAX_DAC_CHN_PER_MOD][0]);
#endif
#else
			  dac_in =  kk == 0? (double)dacOut[jj][ii]: 0.0;
#ifdef CORE_BIQUAD
		 	  dac_in = iir_filter_biquad(dac_in,FE_OVERSAMPLE_COEFF,2,&dDacHistory[ii+jj*MAX_DAC_CHN_PER_MOD][0]);
#else
		 	  dac_in = iir_filter(dac_in,FE_OVERSAMPLE_COEFF,2,&dDacHistory[ii+jj*MAX_DAC_CHN_PER_MOD][0]);
#endif
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
			if((!dacChanErr[jj]) && (iopDacEnable)) {
				dac_out = ioMemData->iodata[mm][ioMemCntrDac].data[ii];
				// Zero out data in case user app dies by next cycle
				// when two or more apps share same DAC module.
				ioMemData->iodata[mm][ioMemCntrDac].data[ii] = 0;
			} else dac_out = 0;
			// Write out ADC duotone if DAC duotone is enabled.
			if((dacDuoEnable) && (ii==(num_outs-1)) && (jj == 0))
			{       
				dac_out = adcData[0][ADC_DUOTONE_CHAN];
				// dac_out = dacOut[0][7];
			}      
#ifdef DIAG_TEST
			if((ii==0) && (jj == 0))
			{       
				if(cycleNum < 100) dac_out = limit / 20;
				else dac_out = 0;
			}      
			if((ii==0) && (jj == 6))
			{       
				if(cycleNum < 100) dac_out = limit / 20;
				else dac_out = 0;
			}      
#endif
#else
			// If DAQKILL tripped, send zeroes to IOP
			if(iopDacEnable) dac_out = dacOut[jj][ii];
			else dac_out = 0;
#endif
#endif
			// Check for outputs > range of DAC
			// If overflow, clip at DAC limits and report errors
			if(dac_out > limit) 
			{
				dac_out = limit;
				overflowDac[jj][ii] ++;
				overflowAcc ++;
				dacOF[jj] = 1;
			}
			if(dac_out < -limit) 
			{
				dac_out = -limit;
				overflowDac[jj][ii] ++;
				overflowAcc ++;
				dacOF[jj] = 1;
			}
		        // If DAQKILL tripped, send zeroes to IOP
		        if(!iopDacEnable) dac_out = 0;
			// Load last values to EPICS channels for monitoring on GDS_TP screen.
		 	dacOutEpics[jj][ii] = dac_out;

			// Load DAC testpoints
			floatDacOut[16*jj + ii] = dac_out;
#ifdef ADC_SLAVE
			// Slave needs to write to memory buffer for IOP
			memCtr = (ioMemCntrDac + kk) % IO_MEMORY_SLOTS;
			// Only write to DAC channels being used to allow two or more
			// slaves to write to same DAC module.
			if (dacOutUsed[jj][ii]) 
	   			ioMemData->iodata[mm][memCtr].data[ii] = dac_out;
#else

			// Write to DAC local memory area, for later xmit to DAC module
			*pDacData =  (unsigned int)(dac_out & mask);
			pDacData ++;
#endif
		}
#ifdef ADC_SLAVE
		// Write cycle count to make DAC data complete
		if(iopDacEnable)
		ioMemData->iodata[mm][memCtr].cycle = (ioClockDac + kk) % IOP_IO_RATE;
#endif
#ifdef OVERSAMPLE_DAC
  	   }
#endif
#ifdef ADC_MASTER
		// Mark cycle count as having been used -1
		// Forces slaves to mark this cycle or will not be used again by Master
		ioMemData->iodata[mm][ioMemCntrDac].cycle = -1;
		// DMA Write data to DAC module
	       if(dacWriteEnable > 4) gsaDacDma2(jj,cdsPciModules.dacType[jj],dacBufOffset);
#endif
	}


#ifdef ADC_SLAVE
		// Increment DAC memory block pointers for next cycle
		ioClockDac = (ioClockDac + OVERSAMPLE_TIMES) % IOP_IO_RATE;
		ioMemCntrDac = (ioMemCntrDac  + OVERSAMPLE_TIMES) % IO_MEMORY_SLOTS;
#endif
#ifdef ADC_MASTER
		// Increment DAC memory block pointers for next cycle
		ioClockDac = (ioClockDac + 1) % IOP_IO_RATE;
		ioMemCntrDac = (ioMemCntrDac + 1) % IO_MEMORY_SLOTS;
#endif
	if(dacWriteEnable < 10) dacWriteEnable ++;
// END OF DAC WRITE ***********************************************************************************
// BEGIN HOUSEKEEPING *********************************************************************************

        pLocalEpics->epicsOutput.cycle = cycleNum;
#ifdef ADC_MASTER
// The following, to endif, is all duotone timing diagnostics.
        if(cycleNum == HKP_READ_SYMCOM_IRIGB)
        {
		if(cdsPciModules.gpsType == SYMCOM_RCVR) 
		{
		// Retrieve time set in at adc read and report offset from 1PPS
			gps_receiver_locked = getGpsTime(&timeSec,&usec);
			pLocalEpics->epicsOutput.diags[FE_DIAGS_IRIGB_TIME] = usec;
		}
		if((usec > MAX_IRIGB_SKEW) || (usec < MIN_IRIGB_SKEW)) 
		{
			diagWord |= TIME_ERR_IRIGB;
			feStatus |= FE_ERROR_TIMING;
		}
                duotoneMean = duotoneTotal/CYCLE_PER_SECOND;
                duotoneTotal = 0.0;
                duotoneMeanDac = duotoneTotalDac/CYCLE_PER_SECOND;
                duotoneTotalDac = 0.0;
        }

	if(cycleNum == HKP_READ_TSYNC_IRIBB)
	{
	        if(cdsPciModules.gpsType == TSYNC_RCVR)
	        {
	                gps_receiver_locked = getGpsuSecTsync(&usec);
	                pLocalEpics->epicsOutput.diags[FE_DIAGS_IRIGB_TIME] = usec;
	                if((usec > MAX_IRIGB_SKEW) || (usec < MIN_IRIGB_SKEW)) 
			{
				diagWord |= TIME_ERR_IRIGB;;
				feStatus |= FE_ERROR_TIMING;
			}
	        }
        }

        duotoneDac[(cycleNum + DT_SAMPLE_OFFSET) % CYCLE_PER_SECOND] = dWord[ADC_DUOTONE_BRD][DAC_DUOTONE_CHAN];
        duotoneTotalDac += dWord[ADC_DUOTONE_BRD][DAC_DUOTONE_CHAN];
        duotone[(cycleNum + DT_SAMPLE_OFFSET) % CYCLE_PER_SECOND] = dWord[ADC_DUOTONE_BRD][ADC_DUOTONE_CHAN];
        duotoneTotal += dWord[ADC_DUOTONE_BRD][ADC_DUOTONE_CHAN];

        if(cycleNum == HKP_DT_CALC)
        {
                duotoneTime = duotime(DT_SAMPLE_CNT, duotoneMean, duotone);
                pLocalEpics->epicsOutput.diags[FE_DIAGS_DUOTONE_TIME] = duotoneTime;
		if((duotoneTime < MIN_DT_DIAG_VAL) || (duotoneTime > MAX_DT_DIAG_VAL)) feStatus |= FE_ERROR_TIMING;
		duotoneTimeDac = duotime(DT_SAMPLE_CNT, duotoneMeanDac, duotoneDac);
                pLocalEpics->epicsOutput.diags[FE_DIAGS_DAC_DUO] = duotoneTimeDac;
        }

	if(cycleNum == HKP_DAC_DT_SWITCH)
	{
		if(dacDuoEnable != pLocalEpics->epicsInput.dacDuoSet)
		{
			dacDuoEnable = pLocalEpics->epicsInput.dacDuoSet;
			// printf("duo set = %d\n",dacDuoEnable);
			if(dacDuoEnable)
				CDIO1616Output[0] = TDS_START_ADC_NEG_DAC_POS;
			else CDIO1616Output[0] = TDS_START_ADC_NEG_DAC_POS | TDS_NO_DAC_DUOTONE;
			CDIO1616Input[0] = writeCDIO1616l(&cdsPciModules, tdsControl[0], CDIO1616Output[0]);
		}
	}
#endif


	// Send timing info to EPICS at 1Hz
	if(cycleNum ==HKP_TIMING_UPDATES)	
        {
	  pLocalEpics->epicsOutput.cpuMeter = timeHold;
	  pLocalEpics->epicsOutput.cpuMeterMax = timeHoldMax;
#ifdef ADC_MASTER
  	  pLocalEpics->epicsOutput.diags[FE_DIAGS_DAC_MASTER_STAT] = dacEnable;
#endif
          timeHoldHold = timeHold;
          timeHold = 0;
	  timeHoldWhenHold = timeHoldWhen;

#if defined(SERVO64K) || defined(SERVO32K) || defined(SERVO16K)
	  memcpy(cycleHistMax, cycleHist, sizeof(cycleHist));
	  memset(cycleHist, 0, sizeof(cycleHist));
	  memcpy(cycleHistWhenHold, cycleHistWhen, sizeof(cycleHistWhen));
	  memset(cycleHistWhen, 0, sizeof(cycleHistWhen));
#endif
	  if (timeSec % 4 == 0) pLocalEpics->epicsOutput.adcWaitTime = adcHoldTimeMin;
	  else if (timeSec % 4 == 1)
		pLocalEpics->epicsOutput.adcWaitTime =  adcHoldTimeMax;
	  else
	  	pLocalEpics->epicsOutput.adcWaitTime = adcHoldTimeAvg/CYCLE_PER_SECOND;
	  adcHoldTimeAvgPerSec = adcHoldTimeAvg/CYCLE_PER_SECOND;
	  adcHoldTimeMax = 0;
	  adcHoldTimeMin = 0xffff;
	  adcHoldTimeAvg = 0;
	  if((adcHoldTime > CYCLE_TIME_ALRM_HI) || (adcHoldTime < CYCLE_TIME_ALRM_LO)) 
	  {
	  	diagWord |= FE_ADC_HOLD_ERR;
		feStatus |= FE_ERROR_TIMING;;
	  }
	  if(timeHoldMax > CYCLE_TIME_ALRM) 
	  {
	  	diagWord |= FE_PROC_TIME_ERR;
		feStatus |= FE_ERROR_TIMING;;
	  }
	  pLocalEpics->epicsOutput.stateWord = feStatus;
  	  feStatus = 0;
  	  if(pLocalEpics->epicsInput.diagReset || initialDiagReset)
	  {
		initialDiagReset = 0;
		pLocalEpics->epicsInput.diagReset = 0;
		pLocalEpics->epicsInput.ipcDiagReset = 1;
  		// pLocalEpics->epicsOutput.diags[1] = 0;
		timeHoldMax = 0;
	  	diagWord = 0;
		ipcErrBits = 0;
		// feStatus = 0;
	  }
	  // Flip the onePPS various once/sec as a watchdog monitor.
	  // pLocalEpics->epicsOutput.onePps ^= 1;
	  pLocalEpics->epicsOutput.diagWord = diagWord;
        }


	/* Update User code Filter Module Epics variables */
	if(subcycle == HKP_FM_EPICS_UPDATE)
	{
	   if(!(daqCycle % 4))
	   {
		//.for(ii=0;ii<MAX_MODULES;ii++)
		//{
		// Following call sends all filter module data to epics each time called.
			//updateEpics(ii, dspPtr[0], pDsp[0],
			    //&dspCoeff[0], pCoeff[0]);
		//}
		// Send sync signal to EPICS sequencer
		pLocalEpics->epicsOutput.epicsSync = daqCycle;
	   }
	}
	// Spread out filter update, but keep updates at 16 Hz
	// here we are rounding up:
	//   x/y rounded up equals (x + y - 1) / y
	//
	{ 
		static const unsigned int mpc = (MAX_MODULES + (FE_RATE / 16) - 1) / (FE_RATE / 16); // Modules per cycle
		unsigned int smpc = mpc * subcycle; // Start module counter
		unsigned int empc = smpc + mpc; // End module counter
		unsigned int i;
		for (i = smpc; i < MAX_MODULES && i < empc ; i++) 
			updateEpics(i, dspPtr[0], pDsp[0], &dspCoeff[0], pCoeff[0]);
	}

	// Check if code exit is requested
	if(cycleNum == MAX_MODULES) 
		vmeDone = stop_working_threads | checkEpicsReset(cycleNum, (struct CDS_EPICS *)pLocalEpics);

	// If synced to 1PPS on startup, continue to check that code
	// is still in sync with 1PPS.
	// This is NOT normal aLIGO mode.
	if(syncSource == SYNC_SRC_1PPS)
	{

		// Assign chan 32 to onePps 
		onePps = adcData[ADC_DUOTONE_BRD][ADC_DUOTONE_CHAN];
		if((onePps > ONE_PPS_THRESH) && (onePpsHi == 0))  
		{
			onePpsTime = cycleNum;
			onePpsHi = 1;
		}
		if(onePps < ONE_PPS_THRESH) onePpsHi = 0;  

		// Check if front end continues to be in sync with 1pps
		// If not, set sync error flag
		if(onePpsTime > 1) pLocalEpics->epicsOutput.timeErr |= TIME_ERR_1PPS;
	}
#ifdef DIAG_TEST
	for(ii=0;ii<10;ii++)
	{
		if(ii<5) onePpsTest = adcData[0][ii];
		else onePpsTest = adcData[1][(ii-5)];
		if((onePpsTest > 400) && (onePpsHiTest[ii] == 0))  
		{
			onePpsTimeTest[ii] = cycleNum;
			onePpsHiTest[ii] = 1;
			if((ii == 0) || (ii == 5)) pLocalEpics->epicsOutput.timingTest[ii] = cycleNum * 15.26;
			// Slaves do not see 1pps until after IOP signal loops around and back into ADC channel 0,
			// therefore, need to subtract IOP loop time.
			else pLocalEpics->epicsOutput.timingTest[ii] = (cycleNum * 15.26) - pLocalEpics->epicsOutput.timingTest[0];
		}
		// Reset the diagnostic for next cycle
		if(cycleNum > 2000) onePpsHiTest[ii] = 0;  
	}
#endif

#ifndef ADC_MASTER
 // Read Dio cards once per second
 // IOP does not handle binary I/O module traffic, as it can take too long
        if(cycleNum < cdsPciModules.doCount)
        {
                kk = cycleNum;
                ii = cdsPciModules.doInstance[kk];
                if(cdsPciModules.doType[kk] == ACS_8DIO)
                {
	  		rioInputInput[ii] = readIiroDio(&cdsPciModules, kk) & 0xff;
	  		rioInputOutput[ii] = readIiroDioOutput(&cdsPciModules, kk) & 0xff;
                }
                if(cdsPciModules.doType[kk] == ACS_16DIO)
                {
                        rioInput1[ii] = readIiroDio1(&cdsPciModules, kk) & 0xffff;
                }
		if(cdsPciModules.doType[kk] == ACS_24DIO)
		{
		  dioInput[ii] = readDio(&cdsPciModules, kk);
		}
		if (cdsPciModules.doType[kk] == CON_6464DIO) {
	 		CDIO6464InputInput[ii] = readInputCDIO6464l(&cdsPciModules, kk);
		}
		if (cdsPciModules.doType[kk] == CDI64) {
	 		CDIO6464InputInput[ii] = readInputCDIO6464l(&cdsPciModules, kk);
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
                } else 
                if(cdsPciModules.doType[kk] == CON_32DO)
                {
                        if (CDO32Input[ii] != CDO32Output[ii]) {
                          CDO32Input[ii] = writeCDO32l(&cdsPciModules, kk, CDO32Output[ii]);
                        }
		} else if (cdsPciModules.doType[kk] == CON_1616DIO) {
#if 0
// Do not want to have user code access this type module, as it is used by IOP for timing control
			if (CDIO1616Input[ii] != CDIO1616Output[ii]) {
			  CDIO1616Input[ii] = writeCDIO1616l(&cdsPciModules, kk, CDIO1616Output[ii]);
			}
#endif
			CDIO1616InputInput[ii] = readInputCDIO1616l(&cdsPciModules, kk);
		} else if (cdsPciModules.doType[kk] == CON_6464DIO) {
			if (CDIO6464Input[ii] != CDIO6464Output[ii]) {
			  CDIO6464Input[ii] = writeCDIO6464l(&cdsPciModules, kk, CDIO6464Output[ii]);
			}
		} else if (cdsPciModules.doType[kk] == CDO64) {
			if (CDIO6464Input[ii] != CDIO6464Output[ii]) {
			  CDIO6464Input[ii] = writeCDIO6464l(&cdsPciModules, kk, CDIO6464Output[ii]);
			}
                } else
                if((cdsPciModules.doType[kk] == ACS_24DIO) && (dioOutputHold[ii] != dioOutput[ii]))
		{
                        writeDio(&cdsPciModules, kk, dioOutput[ii]);
			dioOutputHold[ii] = dioOutput[ii];
		}
        }

#endif

#ifndef NO_DAQ
		
		// Call daqLib
		if (cycle_gps_time == 0) startGpsTime = timeSec;
		cycle_gps_time = timeSec; // Time at which ADCs triggered
		pLocalEpics->epicsOutput.diags[FE_DIAGS_DAQ_BYTE_CNT] = 
			daqWrite(1,dcuId,daq,DAQ_RATE,testpoint,dspPtr[0],myGmError2,(int *)(pLocalEpics->epicsOutput.gdsMon),xExc);
		// Send the current DAQ block size to the awgtpman for TP number checking
	  	pEpicsComms->padSpace.feDaqBlockSize = curDaqBlockSize;
	  	pLocalEpics->epicsOutput.tpCnt = tpPtr->count & 0xff;
		feStatus |= (FE_ERROR_EXC_SET & tpPtr->count);
		if(pLocalEpics->epicsOutput.diags[FE_DIAGS_DAQ_BYTE_CNT] > DAQ_DCU_RATE_WARNING) 
			feStatus |= FE_ERROR_DAQ;;
#endif

	if(cycleNum == HKP_DIAG_UPDATES)	
        {
	  pLocalEpics->epicsOutput.diags[FE_DIAGS_USER_TIME] = usrHoldTime;
	  pLocalEpics->epicsOutput.diags[FE_DIAGS_IPC_STAT] = ipcErrBits;
	  if(ipcErrBits) feStatus |= FE_ERROR_IPC;
	  // Create FB status word for return to EPICS
#ifdef USE_GM
  	  pLocalEpics->epicsOutput.diags[FE_DIAGS_FB_NET_STAT] = (fbStat[1] & 3) * 4 + (fbStat[0] & 3);
#endif
#if defined(SHMEM_DAQ)
	  mxStat = 0;
	  mxDiagR = daqPtr->status;
	  if((mxDiag & 1) != (mxDiagR & 1)) mxStat = 1;
	  if((mxDiag & 2) != (mxDiagR & 2)) mxStat += 2;
	  pLocalEpics->epicsOutput.diags[FE_DIAGS_FB_NET_STAT] = mxStat;
  	  mxDiag = mxDiagR;
	  if(mxStat != 3)
		feStatus |= FE_ERROR_DAQ;;
#endif
	  usrHoldTime = 0;
  	  if((pLocalEpics->epicsInput.overflowReset) || (overflowAcc > OVERFLOW_CNTR_LIMIT))
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

#ifdef ADC_SLAVE
        if(cycleNum == 1)
	{
          pLocalEpics->epicsOutput.timeDiag = timeSec;
	}
#endif
        if(subcycle == HKP_DAC_EPICS_UPDATES)
	{
	      // Send DAC output values at 16Hzfb
	      for(jj=0;jj<cdsPciModules.dacCount;jj++)
	      {
	    	for(ii=0;ii<MAX_DAC_CHN_PER_MOD;ii++)
	    	{
			pLocalEpics->epicsOutput.dacValue[jj][ii] = dacOutEpics[jj][ii];
		}
	      }
	}

        if(cycleNum == HKP_ADC_DAC_STAT_UPDATES)
        {
	  // Pass overflow total count to EPICS
	  pLocalEpics->epicsOutput.ovAccum = overflowAcc;
	  for(jj=0;jj<cdsPciModules.adcCount;jj++)
	  {
	    // SET/CLR Channel Hopping Error
	    if(adcChanErr[jj]) 
	    {
	    	pLocalEpics->epicsOutput.statAdc[jj] &= ~(2);
		feStatus |= FE_ERROR_ADC;;
	    }
 	    else pLocalEpics->epicsOutput.statAdc[jj] |= 2;
	    adcChanErr[jj] = 0;
	    // SET/CLR Overflow Error
	    if(adcOF[jj]) 
	    {
	    	pLocalEpics->epicsOutput.statAdc[jj] &= ~(4);
		feStatus |= FE_ERROR_OVERFLOW;;
	    }
 	    else pLocalEpics->epicsOutput.statAdc[jj] |= 4;
	    adcOF[jj] = 0;
	    for(ii=0;ii<32;ii++)
	    {
		pLocalEpics->epicsOutput.overflowAdc[jj][ii] = overflowAdc[jj][ii];

#ifdef ROLLING_OVERFLOWS
                if (overflowAdc[jj][ii] > OVERFLOW_CNTR_LIMIT) {
		   overflowAdc[jj][ii] = 0;
                }
#else
		overflowAdc[jj][ii] = 0;
#endif

	    }
	  }
	  // If ADC channels not where they should be, we have no option but to exit
	  // from the RT code ie loops would be working with wrong input data.
	  if (chanHop) {
		// Write error condition to STATE_WORD
		pLocalEpics->epicsOutput.stateWord = FE_ERROR_ADC;
	         stop_working_threads = 1;
	         vmeDone = 1;
	         printf("Channel Hopping Detected on one or more ADC modules !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	         printf("Check GDSTP screen ADC status bits to id affected ADC modules\n");
	         printf("Code is exiting ..............\n");
	 	 continue;    
	  }
	  for(jj=0;jj<cdsPciModules.dacCount;jj++)
	  {
	    if(dacOF[jj]) 
	    {
	    	pLocalEpics->epicsOutput.statDac[jj] &= ~(DAC_OVERFLOW_BIT);
		feStatus |= FE_ERROR_OVERFLOW;;
	    }
 	    else pLocalEpics->epicsOutput.statDac[jj] |= DAC_OVERFLOW_BIT;
	    dacOF[jj] = 0;
	    if(dacChanErr[jj]) 
	    {
	    	pLocalEpics->epicsOutput.statDac[jj] &= ~(DAC_TIMING_BIT);
	    }
 	    else pLocalEpics->epicsOutput.statDac[jj] |= DAC_TIMING_BIT;
	    dacChanErr[jj] = 0;
	    for(ii=0;ii<MAX_DAC_CHN_PER_MOD;ii++)
	    {
		pLocalEpics->epicsOutput.overflowDac[jj][ii] = overflowDac[jj][ii];

#ifdef ROLLING_OVERFLOWS
                if (overflowDac[jj][ii] > OVERFLOW_CNTR_LIMIT) {
		   overflowDac[jj][ii] = 0;
                }
#else
		overflowDac[jj][ii] = 0;
#endif

	    }
	  }
        }
#ifdef ADC_MASTER
	// Deal with the own-data bits on the VMIC 5565 rfm cards
	if (cdsPciModules.rfmCount > 0) {
        	if (cycleNum >= HKP_RFM_CHK_CYCLE && cycleNum < (HKP_RFM_CHK_CYCLE + cdsPciModules.rfmCount)) {
			int mod = cycleNum - HKP_RFM_CHK_CYCLE;
			if (cdsPciModules.rfmType[mod] == 0x5565) {
				// Check the own-data light
				if ((cdsPciModules.rfm_reg[mod]->LCSR1 & 1) == 0) ipcErrBits |= 4 + (mod * 4);
				//printk("RFM %d own data %d\n", mod, cdsPciModules.rfm_reg[mod]->LCSR1);
			}
		}
		if (cycleNum >= (HKP_RFM_CHK_CYCLE + cdsPciModules.rfmCount) && cycleNum < (HKP_RFM_CHK_CYCLE + cdsPciModules.rfmCount*2)) {
			int mod = cycleNum - HKP_RFM_CHK_CYCLE - cdsPciModules.rfmCount;
			if (cdsPciModules.rfmType[mod] == 0x5565) {
				// Reset the own-data light
				cdsPciModules.rfm_reg[mod]->LCSR1 &= ~1;
			}
		}
		if (cycleNum >= (HKP_RFM_CHK_CYCLE + 2*cdsPciModules.rfmCount) && cycleNum < (HKP_RFM_CHK_CYCLE + cdsPciModules.rfmCount*3)) {
			int mod = cycleNum - HKP_RFM_CHK_CYCLE - cdsPciModules.rfmCount*2;
			if (cdsPciModules.rfmType[mod] == 0x5565) {
				// Write data out to the RFM to trigger the light
	  			((volatile long *)(cdsPciModules.pci_rfm[mod]))[2] = 0;
			}
		}
	}
// DAC WD Write for 18 bit DAC modules
// Check once per second on code cycle 400 to dac count
// Only one write per code cycle to reduce time
       	if (cycleNum >= HKP_DAC_WD_CLK && cycleNum < (HKP_DAC_WD_CLK + cdsPciModules.dacCount)) 
	{
		jj = cycleNum - HKP_DAC_WD_CLK;
		if(cdsPciModules.dacType[jj] == GSC_18AO8)
		{
			static int dacWatchDog = 0;
			volatile GSA_18BIT_DAC_REG *dac18bitPtr;
			if (cycleNum == HKP_DAC_WD_CLK) dacWatchDog ^= 1;
			dac18bitPtr = (volatile GSA_18BIT_DAC_REG *)(dacPtr[jj]);
			if(iopDacEnable)
				dac18bitPtr->digital_io_ports = (dacWatchDog | GSAO_18BIT_DIO_RW);

		}
	}
// AI Chassis WD CHECK for 18 bit DAC modules
// Check once per second on code cycle HKP_DAC_WD_CHK to dac count
// Only one read per code cycle to reduce time
       	if (cycleNum >= HKP_DAC_WD_CHK && cycleNum < (HKP_DAC_WD_CHK + cdsPciModules.dacCount)) 
	{
		jj = cycleNum - HKP_DAC_WD_CHK;
		if(cdsPciModules.dacType[jj] == GSC_18AO8)
		{
			static int dacWDread = 0;
			volatile GSA_18BIT_DAC_REG *dac18bitPtr = (volatile GSA_18BIT_DAC_REG *)(dacPtr[jj]);
			dacWDread = dac18bitPtr->digital_io_ports;
			if(((dacWDread >> 8) & 1) > 0)
			    {
			    	pLocalEpics->epicsOutput.statDac[jj] &= ~(DAC_WD_BIT);
			    }
			    else 
			    	pLocalEpics->epicsOutput.statDac[jj] |= DAC_WD_BIT;

		}
	}

// 18bit FIFO size check, once per second
// Used to verify DAC is clocking correctly and code is transmitting data on time.
	// Uncomment to read 18-bit DAC buffer size
	//extern volatile GSA_DAC_REG *dacPtr[MAX_DAC_MODULES];
        //volatile GSA_18BIT_DAC_REG *dac18bitPtr = dacPtr[0];
        //out_buf_size = dac18bitPtr->OUT_BUF_SIZE;
//
// Check DAC FIFO Sizes to determine if DAC modules are synched to code 
// 18bit DAC reads out FIFO size dirrectly, but 16bit module only has a 4 bit register
// area for FIFO empty, quarter full, etc. So, to make these bits useful in 16 bit module,
// code must set a proper FIFO size in map.c code.
// This code runs once per second.
       	if (cycleNum >= HKP_DAC_FIFO_CHK && cycleNum < (HKP_DAC_FIFO_CHK + cdsPciModules.dacCount)) 
	{
		jj = cycleNum - HKP_DAC_FIFO_CHK;
		if(cdsPciModules.dacType[jj] == GSC_18AO8)
		{
			volatile GSA_18BIT_DAC_REG *dac18bitPtr = (volatile GSA_18BIT_DAC_REG *)(dacPtr[jj]);
			out_buf_size = dac18bitPtr->OUT_BUF_SIZE;
			dacOutBufSize[jj] = out_buf_size;
			if((out_buf_size < 8) || (out_buf_size > 24))
			{
			    pLocalEpics->epicsOutput.statDac[jj] &= ~(DAC_FIFO_BIT);
			    feStatus |= FE_ERROR_DAC;
			} else pLocalEpics->epicsOutput.statDac[jj] |= DAC_FIFO_BIT;

		}
		if(cdsPciModules.dacType[jj] == GSC_16AO16)
		{
			status = checkDacBuffer(jj);
			dacOutBufSize[jj] = status;
			if(status != 2)
			{
			    pLocalEpics->epicsOutput.statDac[jj] &= ~(DAC_FIFO_BIT);
			    feStatus |= FE_ERROR_DAC;
			} else pLocalEpics->epicsOutput.statDac[jj] |= DAC_FIFO_BIT;
		}
	}

#endif
	// Capture end of cycle time.
        rdtscl(cpuClock[CPU_TIME_CYCLE_END]);

	// Compute max time of one cycle.
	cycleTime = (cpuClock[CPU_TIME_CYCLE_END] - cpuClock[CPU_TIME_CYCLE_START])/CPURATE;
	if (longestWrite2 < ((tempClock[3]-tempClock[2])/CPURATE)) longestWrite2 = (tempClock[3]-tempClock[2])/CPURATE;
	if (cycleTime > (1000000/CYCLE_PER_SECOND))  {
		// printf("cycle %d time %d; adcWait %d; write1 %d; write2 %d; longest write2 %d\n", cycleNum, cycleTime, adcWait, (tempClock[1]-tempClock[0])/CPURATE, (tempClock[3]-tempClock[2])/CPURATE, longestWrite2);
	}
	// Hold the max cycle time over the last 1 second
	if(cycleTime > timeHold) { 
		timeHold = cycleTime;
		timeHoldWhen = cycleNum;
	}
	// Hold the max cycle time since last diag reset
	if(cycleTime > timeHoldMax) timeHoldMax = cycleTime;
#if defined(SERVO64K) || defined(SERVO32K) || defined(SERVO16K)
// This produces cycle time histogram in /proc file
	{
#if defined(SERVO64K) || defined(SERVO32K)
		static const int nb = 31;
#elif defined(SERVO16K)
		static const int nb = 63;
#endif

		cycleHist[cycleTime<nb?cycleTime:nb]++;
		cycleHistWhen[cycleTime<nb?cycleTime:nb] = cycleNum;
	}
#endif
	adcHoldTime = (cpuClock[CPU_TIME_CYCLE_START] - adcTime)/CPURATE;
	// Avoid calculating the max hold time on the very first cycle
	// since we can be holding for up to 1 second when we start running
	if (cycleNum == 0 && startGpsTime == cycle_gps_time) ; else {
		if(adcHoldTime > adcHoldTimeMax) adcHoldTimeMax = adcHoldTime;
		if(adcHoldTime < adcHoldTimeMin) adcHoldTimeMin = adcHoldTime;
		adcHoldTimeAvg += adcHoldTime;
		if (adcHoldTimeMax > adcHoldTimeEverMax)  {
			adcHoldTimeEverMax = adcHoldTimeMax;
			adcHoldTimeEverMaxWhen = cycle_gps_time;
			//printf("Maximum adc hold time %d on cycle %d gps %d\n", adcHoldTimeMax, cycleNum, cycle_gps_time);
		}
	}
	adcTime = cpuClock[CPU_TIME_CYCLE_START];
	// Calc the max time of one cycle of the user code
	// For IOP, more interested in time to get thru ADC read code and send to slave apps
#ifdef ADC_MASTER
	usrTime = (cpuClock[CPU_TIME_USR_START] - cpuClock[CPU_TIME_CYCLE_START])/CPURATE;
#else
	usrTime = (cpuClock[CPU_TIME_USR_END] - cpuClock[CPU_TIME_USR_START])/CPURATE;
#endif
	if(usrTime > usrHoldTime) usrHoldTime = usrTime;

        // Update internal cycle counters
          cycleNum += 1;
#ifdef DIAG_TEST
          if(pLocalEpics->epicsInput.bumpCycle != 0) {
	  	cycleNum += pLocalEpics->epicsInput.bumpCycle;
		pLocalEpics->epicsInput.bumpCycle = 0;
	  }
#endif
          cycleNum %= CYCLE_PER_SECOND;
	  clock1Min += 1;
	  clock1Min %= CYCLE_PER_MINUTE;
          if(subcycle == DAQ_CYCLE_CHANGE) daqCycle = (daqCycle + 1) % DAQ_NUM_DATA_BLOCKS_PER_SECOND;
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

  printf("exiting from fe_code()\n");
  pLocalEpics->epicsOutput.cpuMeter = 0;

#ifdef ADC_SLAVE
  deallocate_dac_channels();
#endif

  /* System reset command received */
  return (void *)-1;
}
// MAIN routine: Code starting point ****************************************************************

// These externs and "16" need to go to a header file (mbuf.h)
extern void *kmalloc_area[16];
extern int mbuf_allocate_area(char *name, int size, struct file *file);

// /proc filesystem entry
struct proc_dir_entry *proc_entry;

int
procfile_read(char *buffer,
	      char **buffer_location,
	      off_t offset, int buffer_length, int *eof, void *data)
{
	int ret, i;
	i = 0;
	*buffer = 0;

	/* 
	 * We give all of our information in one go, so if the
	 * user asks us if we have more information the
	 * answer should always be no.
	 *
	 * This is important because the standard read
	 * function from the library would continue to issue
	 * the read system call until the kernel replies
	 * that it has no more information, or until its
	 * buffer is filled.
	 */
	if (offset > 0) {
		/* we have finished to read, return 0 */
		ret  = 0;
	} else {
#if defined(SERVO64K) || defined(SERVO32K) || defined(SERVO16K) || defined(COMMDATA_INLINE)
		char b[128];
#if defined(SERVO64K) || defined(SERVO32K)
		static const int nb = 32;
#elif defined(SERVO16K)
		static const int nb = 64;
#endif
#endif
		/* fill the buffer, return the buffer size */
		ret = sprintf(buffer,

			"startGpsTime=%d\n"
			"uptime=%d\n"
			"adcHoldTime=%d\n"
			"adcHoldTimeEverMax=%d\n"
			"adcHoldTimeEverMaxWhen=%d\n"
			"adcHoldTimeMax=%d\n"
			"adcHoldTimeMin=%d\n"
			"adcHoldTimeAvg=%d\n"
			"usrTime=%d\n"
			"usrHoldTime=%d\n"
			"cycle=%d\n"
			"gps=%d\n"
			"buildDate=%s\n"
			"cpuTimeMax(cur,past sec)=%d,%d\n"
			"cpuTimeMaxCycle(cur,past sec)=%d,%d\n",

			startGpsTime,
			cycle_gps_time - startGpsTime,
			adcHoldTime,
			adcHoldTimeEverMax,
			adcHoldTimeEverMaxWhen,
			adcHoldTimeMax,
			adcHoldTimeMin,
			adcHoldTimeAvgPerSec,
			usrTime,
			usrHoldTime,
			cycleNum,
			cycle_gps_time,
			build_date,
			cycleTime, timeHoldHold,
			timeHoldWhen, timeHoldWhenHold);
#if defined(SERVO64K) || defined(SERVO32K) || defined(SERVO16K)
		strcat(buffer, "cycleHist: ");
		for (i = 0; i < nb; i++) {
			if (!cycleHistMax[i]) continue;
			sprintf(b, "%d=%d@%d ", i, cycleHistMax[i], cycleHistWhenHold[i]);
			strcat(buffer, b);
		}
		strcat(buffer, "\n");
#endif

#ifdef ADC_MASTER
		/* Output DAC buffer size information */
		for (i = 0; i < cdsPciModules.dacCount; i++) {
			if (cdsPciModules.dacType[i] == GSC_18AO8) {
				sprintf(b, "DAC #%d 18-bit buf_size=%d\n", i, dacOutBufSize[i]);
			} else {
				sprintf(b, "DAC #%d 16-bit fifo_status=%d (%s)\n", i, dacOutBufSize[i],
					dacOutBufSize[i] & 8? "full":
						(dacOutBufSize[i] & 1? "empty":
							(dacOutBufSize[i] & 4? "high quarter": "OK")));
			}
			strcat(buffer, b);
		}
#endif
#ifdef COMMDATA_INLINE
		// See if we have any IPC with errors and print the numbers out
		//
		sprintf(b, "ipcErrBits=0x%x\n", ipcErrBits);
		strcat(buffer, b);

		// The following loop has a chance to overflow the buffer,
		// which is set to PROC_BLOCK_SIZE. (PAGE_SIZE-1024 = 3072 bytes).
		// We will simply stop printing at that point.
#define PROC_BLOCK_SIZE (3*1024)
		unsigned int byte_cnt = strlen(buffer) + 1;
		for (i = 0; i < myIpcCount; i++) {
	  	  if (ipcInfo[i].errTotal) {
	  		unsigned int cnt =
				sprintf(b, "IPC net=%d num=%d name=%s sender=%s errcnt=%d\n", 
					ipcInfo[i].netType, ipcInfo[i].ipcNum,
					ipcInfo[i].name, ipcInfo[i].senderModelName,
					ipcInfo[i].errTotal);
			if (byte_cnt + cnt > PROC_BLOCK_SIZE) break;
			byte_cnt += cnt;
	  		strcat(buffer, b);
	  	  }
 		}
#endif
		ret = strlen(buffer);
	}

	return ret;
}

/* Track dependency between the IOP and the slaves */
#ifdef ADC_MASTER
int need_to_load_IOP_first;
EXPORT_SYMBOL(need_to_load_IOP_first);
#endif
#ifdef ADC_SLAVE
extern int need_to_load_IOP_first;
#endif

extern void set_fe_code_idle(void *(*ptr)(void *), unsigned int cpu);
extern void msleep(unsigned int);

int init_module (void)
{
 	int status;
	int ii,jj,kk;
	char fname[128];
	int cards;
#ifdef ADC_SLAVE
	int adcCnt;
	int dacCnt;
        int dac18Cnt;
	int doCnt;
	int do32Cnt;
	int doIIRO16Cnt;
	int doIIRO8Cnt;
	int cdo64Cnt;
	int cdi64Cnt;
#endif
	int ret;
	int cnt;
	extern int cpu_down(unsigned int);
	extern int is_cpu_taken_by_rcg_model(unsigned int cpu);

	kk = 0;
#ifdef SPECIFIC_CPU
#define CPUID SPECIFIC_CPU
#else 
#define CPUID 1
#endif

#ifndef NO_CPU_SHUTDOWN
	// See if our CPU core is free
        if (is_cpu_taken_by_rcg_model(CPUID)) {
		printk(KERN_ALERT "Error: CPU %d already taken\n", CPUID);
		return -1;
	}
#endif

#ifdef ADC_SLAVE
	need_to_load_IOP_first = 0;
#endif

#ifdef DOLPHIN_TEST
	status = init_dolphin();
	if (status != 0) {
		return -1;
	}
#endif

	proc_entry = create_proc_entry(SYSTEM_NAME_STRING_LOWER, 0644, NULL);
	
	if (proc_entry == NULL) {
		remove_proc_entry(SYSTEM_NAME_STRING_LOWER, NULL);
		printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
		       SYSTEM_NAME_STRING_LOWER);
		return -ENOMEM;
	}
	
	proc_entry->read_proc = procfile_read;
	//proc_entry->owner 	 = THIS_MODULE;
	proc_entry->mode 	 = S_IFREG | S_IRUGO;
	proc_entry->uid 	 = 0;
	proc_entry->gid 	 = 0;
	proc_entry->size 	 = 10240;

	printf("startup time is %ld\n", current_time());
	jj = 0;
	printf("cpu clock %u\n",cpu_khz);


        ret =  mbuf_allocate_area(SYSTEM_NAME_STRING_LOWER, 64*1024*1024, 0);
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

	printf("IPC at 0x%p\n",_ipc_shm);
  	ioMemData = (IO_MEM_DATA *)(_ipc_shm+ 0x4000);


// If DAQ is via shared memory (Framebuilder code running on same machine or MX networking is used)
// attach DAQ shared memory location.
#if defined(SHMEM_DAQ)
        sprintf(fname, "%s_daq", SYSTEM_NAME_STRING_LOWER);
        ret =  mbuf_allocate_area(fname, 64*1024*1024, 0);
        if (ret < 0) {
                printf("mbuf_allocate_area() failed; ret = %d\n", ret);
                return -1;
        }
        _daq_shm = (unsigned char *)(kmalloc_area[ret]);
        printf("Allocated daq shmem; set at 0x%p\n", _daq_shm);
 	daqPtr = (struct rmIpcStr *) _daq_shm;
#endif

	// Find and initialize all PCI I/O modules *******************************************************
	  // Following I/O card info is from feCode
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

#ifndef ADC_SLAVE
	// Call PCI initialization routine in map.c file.
	status = mapPciModules(&cdsPciModules);
	 //return 0;
#endif
#ifdef ADC_SLAVE
// If running as a slave process, I/O card information is via ipc shared memory
	printf("%d PCI cards found\n",ioMemData->totalCards);
	status = 0;
	adcCnt = 0;
	dacCnt = 0;
	dac18Cnt = 0;
	doCnt = 0;
	do32Cnt = 0;
	cdo64Cnt = 0;
	cdi64Cnt = 0;
	doIIRO16Cnt = 0;
	doIIRO8Cnt = 0;

	// Have to search thru all cards and find desired instance for application
	// Master will map ADC cards first, then DAC and finally DIO
	for(ii=0;ii<ioMemData->totalCards;ii++)
	{
		/*
		printf("Model %d = %d\n",ii,ioMemData->model[ii]);
		*/
		for(jj=0;jj<cards;jj++)
		{
			/*
			printf("Model %d = %d, type = %d, instance = %d, dacCnt = %d \n",
				ii,ioMemData->model[ii],
				cdsPciModules.cards_used[jj].type,
				cdsPciModules.cards_used[jj].instance,
 				dacCnt);
				*/
		   switch(ioMemData->model[ii])
		   {
			case GSC_16AI64SSA:
				if((cdsPciModules.cards_used[jj].type == GSC_16AI64SSA) && 
					(cdsPciModules.cards_used[jj].instance == adcCnt))
				{
					printf("Found ADC at %d %d\n",jj,ioMemData->ipc[ii]);
					kk = cdsPciModules.adcCount;
					cdsPciModules.adcType[kk] = GSC_16AI64SSA;
					cdsPciModules.adcConfig[kk] = ioMemData->ipc[ii];
					cdsPciModules.adcCount ++;
					status ++;
				}
				break;
			case GSC_16AO16:
				if((cdsPciModules.cards_used[jj].type == GSC_16AO16) && 
					(cdsPciModules.cards_used[jj].instance == dacCnt))
				{
					printf("Found DAC at %d %d\n",jj,ioMemData->ipc[ii]);
					kk = cdsPciModules.dacCount;
					cdsPciModules.dacType[kk] = GSC_16AO16;
					cdsPciModules.dacConfig[kk] = ioMemData->ipc[ii];
	   				cdsPciModules.pci_dac[kk] = (long)(ioMemData->iodata[ii]);
					cdsPciModules.dacCount ++;
					status ++;
				}
				break;
			case GSC_18AO8:
				if((cdsPciModules.cards_used[jj].type == GSC_18AO8) && 
					(cdsPciModules.cards_used[jj].instance == dac18Cnt))
				{
					printf("Found DAC at %d %d\n",jj,ioMemData->ipc[ii]);
					kk = cdsPciModules.dacCount;
					cdsPciModules.dacType[kk] = GSC_18AO8;
					cdsPciModules.dacConfig[kk] = ioMemData->ipc[ii];
	   				cdsPciModules.pci_dac[kk] = (long)(ioMemData->iodata[ii]);
					cdsPciModules.dacCount ++;
					status ++;
				}
				break;
			case CON_6464DIO:
				if((cdsPciModules.cards_used[jj].type == CON_6464DIO) && 
					(cdsPciModules.cards_used[jj].instance == doCnt))
				{
					kk = cdsPciModules.doCount;
					printf("Found 6464 DIO CONTEC at %d 0x%x\n",jj,ioMemData->ipc[ii]);
					cdsPciModules.doType[kk] = ioMemData->model[ii];
					cdsPciModules.pci_do[kk] = ioMemData->ipc[ii];
					cdsPciModules.doCount ++;
					cdsPciModules.cDio6464lCount ++;
					cdsPciModules.pci_do[kk] = ioMemData->ipc[ii];
					cdsPciModules.doInstance[kk] = doCnt;
					status += 2;
				}
				if((cdsPciModules.cards_used[jj].type == CDO64) && 
					(cdsPciModules.cards_used[jj].instance == doCnt))
				{
					kk = cdsPciModules.doCount;
					cdsPciModules.doType[kk] = CDO64;
					cdsPciModules.pci_do[kk] = ioMemData->ipc[ii];
					cdsPciModules.doCount ++;
					cdsPciModules.cDio6464lCount ++;
					cdsPciModules.doInstance[kk] = doCnt;
					printf("Found 6464 DOUT CONTEC at %d 0x%x index %d \n",jj,ioMemData->ipc[ii],doCnt);
					cdo64Cnt ++;
					status ++;
				}
				if((cdsPciModules.cards_used[jj].type == CDI64) && 
					(cdsPciModules.cards_used[jj].instance == doCnt))
				{
					kk = cdsPciModules.doCount;
					// printf("Found 6464 DIN CONTEC at %d 0x%x\n",jj,ioMemData->ipc[ii]);
					cdsPciModules.doType[kk] = CDI64;
					cdsPciModules.pci_do[kk] = ioMemData->ipc[ii];
					cdsPciModules.doInstance[kk] = doCnt;
					cdsPciModules.doCount ++;
					printf("Found 6464 DIN CONTEC at %d 0x%x index %d \n",jj,ioMemData->ipc[ii],doCnt);
					cdsPciModules.cDio6464lCount ++;
					cdi64Cnt ++;
					status ++;
				}
				break;
                        case CON_32DO:
		                if((cdsPciModules.cards_used[jj].type == CON_32DO) &&
		                        (cdsPciModules.cards_used[jj].instance == do32Cnt))
		                {
					kk = cdsPciModules.doCount;
               			      	printf("Found 32 DO CONTEC at %d 0x%x\n",jj,ioMemData->ipc[ii]);
		                      	cdsPciModules.doType[kk] = ioMemData->model[ii];
                                      	cdsPciModules.pci_do[kk] = ioMemData->ipc[ii];
                                      	cdsPciModules.doCount ++; 
                                      	cdsPciModules.cDo32lCount ++; 
					cdsPciModules.doInstance[kk] = do32Cnt;
					status ++;
				 }
				 break;
			case ACS_16DIO:
				if((cdsPciModules.cards_used[jj].type == ACS_16DIO) && 
					(cdsPciModules.cards_used[jj].instance == doIIRO16Cnt))
				{
					kk = cdsPciModules.doCount;
					printf("Found Access IIRO-16 at %d 0x%x\n",jj,ioMemData->ipc[ii]);
					cdsPciModules.doType[kk] = ioMemData->model[ii];
					cdsPciModules.pci_do[kk] = ioMemData->ipc[ii];
					cdsPciModules.doCount ++;
					cdsPciModules.iiroDio1Count ++;
					cdsPciModules.doInstance[kk] = doIIRO16Cnt;
					status ++;
				}
				break;
			case ACS_8DIO:
			       if((cdsPciModules.cards_used[jj].type == ACS_8DIO) &&
			               (cdsPciModules.cards_used[jj].instance == doIIRO8Cnt))
			       {
			               kk = cdsPciModules.doCount;
			               printf("Found Access IIRO-8 at %d 0x%x\n",jj,ioMemData->ipc[ii]);
			               cdsPciModules.doType[kk] = ioMemData->model[ii];
			               cdsPciModules.pci_do[kk] = ioMemData->ipc[ii];
			               cdsPciModules.doCount ++;
			               cdsPciModules.iiroDioCount ++;
			               cdsPciModules.doInstance[kk] = doIIRO8Cnt;
			               status ++;
			       }
		 	       break;
			default:
				break;
		   }
		}
		if(ioMemData->model[ii] == GSC_16AI64SSA) adcCnt ++;
		if(ioMemData->model[ii] == GSC_16AO16) dacCnt ++;
		if(ioMemData->model[ii] == GSC_18AO8) dac18Cnt ++;
		if(ioMemData->model[ii] == CON_6464DIO) doCnt ++;
		if(ioMemData->model[ii] == CON_32DO) do32Cnt ++;
		if(ioMemData->model[ii] == ACS_16DIO) doIIRO16Cnt ++;
		if(ioMemData->model[ii] == ACS_8DIO) doIIRO8Cnt ++;
	}
	// If no ADC cards were found, then SLAVE cannot run
	if(!cdsPciModules.adcCount)
	{
		printf("No ADC cards found - exiting\n");
		return -1;
	}
	// This did not quite work for some reason
	// Need to find a way to handle skipped DAC cards in slaves
	//cdsPciModules.dacCount = ioMemData->dacCount;
#endif
	printf("%d PCI cards found \n",status);
	if(status < cards)
	{
		printf(" ERROR **** Did not find correct number of cards! Expected %d and Found %d\n",cards,status);
		cardCountErr = 1;
	}

	// Print out all the I/O information
        printf("***************************************************************************\n");
#ifdef ADC_MASTER
		// Master send module counds to SLAVE via ipc shm
		ioMemData->totalCards = status;
		ioMemData->adcCount = cdsPciModules.adcCount;
		ioMemData->dacCount = cdsPciModules.dacCount;
		ioMemData->bioCount = cdsPciModules.doCount;
		// kk will act as ioMem location counter for mapping modules
		kk = cdsPciModules.adcCount;
#if defined (TIME_SLAVE) || defined (RFM_TIME_SLAVE)
		ioMemData->totalCards = 2;
		ioMemData->adcCount = 2;
		ioMemData->dacCount = 0;
		ioMemData->bioCount = 0;
		ioMemData->model[0] = GSC_16AI64SSA;
		ioMemData->ipc[0] = 0;	// ioData memory buffer location for SLAVE to use
		ioMemData->model[1] = GSC_16AI64SSA;
		ioMemData->ipc[1] = 1;	// ioData memory buffer location for SLAVE to use
#endif
#endif
	printf("%d ADC cards found\n",cdsPciModules.adcCount);
	for(ii=0;ii<cdsPciModules.adcCount;ii++)
        {
#ifdef ADC_MASTER
		// MASTER maps ADC modules first in ipc shm for SLAVES
		ioMemData->model[ii] = cdsPciModules.adcType[ii];
		ioMemData->ipc[ii] = ii;	// ioData memory buffer location for SLAVE to use
#endif
                if(cdsPciModules.adcType[ii] == GSC_18AISS6C)
                {
                        printf("\tADC %d is a GSC_18AISS6C module\n",ii);
                        printf("\t\tChannels = 6 \n");
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
		// Pass DAC info to SLAVE processes
		ioMemData->model[kk] = cdsPciModules.dacType[ii];
		ioMemData->ipc[kk] = kk;
		// Following used by MASTER to point to ipc memory for inputting DAC data from SLAVES
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
#ifdef ADC_MASTER
	// MASTER sends DIO module information to SLAVES
	// Note that for DIO, SLAVE modules will perform the I/O directly and therefore need to
	// know the PCIe address of these modules.
	ioMemData->bioCount = cdsPciModules.doCount;
	for(ii=0;ii<cdsPciModules.doCount;ii++)
        {
		// MASTER needs to find Contec 1616 I/O card to control timing slave.
		if(cdsPciModules.doType[ii] == CON_1616DIO)
		{
			tdsControl[tdsCount] = ii;
			printf("TDS controller %d is at %d\n",tdsCount,ii);
			tdsCount ++;
		}
		ioMemData->model[kk] = cdsPciModules.doType[ii];
		// Unlike ADC and DAC, where a memory buffer number is passed, a PCIe address is passed
		// for DIO cards.
		ioMemData->ipc[kk] = cdsPciModules.pci_do[ii];
		kk ++;
	}
	printf("Total of %d I/O modules found and mapped\n",kk);
#endif
        printf("***************************************************************************\n");
// Following section maps Reflected Memory, both VMIC hardware style and Dolphin PCIe network style.
// Slave units will perform I/O transactions with RFM directly ie MASTER does not do RFM I/O.
// Master unit only maps the RFM I/O space and passes pointers to SLAVES.

#ifdef ADC_SLAVE
	// Slave gets RFM module count from MASTER.
	cdsPciModules.rfmCount = ioMemData->rfmCount;
	cdsPciModules.dolphinCount = ioMemData->dolphinCount;
	cdsPciModules.dolphin[0] = ioMemData->dolphin[0];
	cdsPciModules.dolphin[1] = ioMemData->dolphin[1];
#endif
	printf("%d RFM cards found\n",cdsPciModules.rfmCount);
#ifdef ADC_MASTER
	ioMemData->rfmCount = cdsPciModules.rfmCount;
#endif
	for(ii=0;ii<cdsPciModules.rfmCount;ii++)
        {
                 printf("\tRFM %d is a VMIC_%x module with Node ID %d\n", ii, cdsPciModules.rfmType[ii], cdsPciModules.rfmConfig[ii]);
#ifdef ADC_SLAVE
		cdsPciModules.pci_rfm[ii] = ioMemData->pci_rfm[ii];
		cdsPciModules.pci_rfm_dma[ii] = ioMemData->pci_rfm_dma[ii];
#endif
		printf("address is 0x%lx\n",cdsPciModules.pci_rfm[ii]);
#ifdef ADC_MASTER
		// Master sends RFM memory pointers to SLAVES
		ioMemData->pci_rfm[ii] = cdsPciModules.pci_rfm[ii];
		ioMemData->pci_rfm_dma[ii] = cdsPciModules.pci_rfm_dma[ii];
#endif
	}
	ioMemData->dolphinCount = 0;
#ifdef DOLPHIN_TEST
	ioMemData->dolphinCount = cdsPciModules.dolphinCount;
	ioMemData->dolphin[0] = cdsPciModules.dolphin[0];
	ioMemData->dolphin[1] = cdsPciModules.dolphin[1];

// Where not all FE computers have an IRIG-B module to tell time, a master and multiple
// slaves may be defined to send/receive GPS time seconds via PCIe RFM. This is only
// for use by ADC_MASTER applications.
#ifdef TIME_MASTER
	rfmTime = (volatile unsigned int *)cdsPciModules.dolphin[1];
	printf("TIME MASTER AT 0x%x\n",(int)rfmTime);
#endif
#ifdef TIME_SLAVE
	rfmTime = (volatile unsigned int *)cdsPciModules.dolphin[0];
	printf("TIME SLAVE AT 0x%x\n",(int)rfmTime);
	printf("rfmTime = %d\n", *rfmTime);
#endif
#ifdef IOP_TIME_SLAVE
	rfmTime = (volatile unsigned int *)cdsPciModules.dolphin[0];
	printf("TIME SLAVE IOP AT 0x%x\n",(int)rfmTime);
	printf("rfmTime = %d\n", *rfmTime);
#endif
#else
#ifdef ADC_MASTER
// Clear Dolphin pointers so the slave sees NULLs
	ioMemData->dolphinCount = 0;
        ioMemData->dolphin[0] = 0;
        ioMemData->dolphin[1] = 0;
#endif
#endif
        printf("***************************************************************************\n");
  	if (cdsPciModules.gps) {
	printf("IRIG-B card found %d\n",cdsPciModules.gpsType);
        printf("***************************************************************************\n");
  	}

 	//cdsPciModules.adcCount = 2;

	// Code will run on internal timer if no ADC modules are found
	if (cdsPciModules.adcCount == 0) {
		printf("No ADC modules found, running on timer\n");
		run_on_timer = 1;
        	//munmap(_epics_shm, MMAP_SIZE);
        	//close(wfd);
        	//return 0;
	}

	// Initialize buffer for daqLib.c code
	printf("Initializing space for daqLib buffers\n");
	daqBuffer = (long)&daqArea[0];
 
#ifndef NO_DAQ
	printf("Initializing Network\n");
	numFb = cdsDaqNetInit(2);
	if (numFb <= 0) {
		printf("Couldn't initialize Myrinet network connection\n");
		return -1;
	}
	printf("Found %d frameBuilders on network\n",numFb);
#endif


        pLocalEpics = (CDS_EPICS *)&((RFM_FE_COMMS *)_epics_shm)->epicsSpace;
	for (cnt = 0;  cnt < 10 && pLocalEpics->epicsInput.burtRestore == 0; cnt++) {
        	printf("Epics burt restore is %d\n", pLocalEpics->epicsInput.burtRestore);
        	msleep(1000);
	}
	if (cnt == 10) {
		// Cleanup
		remove_proc_entry(SYSTEM_NAME_STRING_LOWER, NULL);
#ifdef DOLPHIN_TEST
		finish_dolphin();
#endif
		return -1;
	}

        pLocalEpics->epicsInput.vmeReset = 0;

#ifdef NO_CPU_SHUTDOWN
        struct task_struct *p;
        p = kthread_create(fe_start, 0, "fe_start/%d", CPUID);
        if (IS_ERR(p)){
                printf("Failed to kthread_create()\n");
                return -1;
        }
        kthread_bind(p, CPUID);
        wake_up_process(p);
#endif


#ifndef NO_CPU_SHUTDOWN
        set_fe_code_idle(fe_start, CPUID);
        msleep(100);

	cpu_down(CPUID);

	// The code runs on the disabled CPU
#endif


#ifdef DOLPHIN_TEST
	//finish_dolphin();
#endif

        return 0;
}

void cleanup_module (void) {
	extern int __cpuinit cpu_up(unsigned int cpu);

	remove_proc_entry(SYSTEM_NAME_STRING_LOWER, NULL);
//	printk("Setting vmeReset flag to 1\n");
	//pLocalEpics->epicsInput.vmeReset = 1;
        //msleep(1000);

#ifndef NO_CPU_SHUTDOWN
	// Unset the code callback
        set_fe_code_idle(0, CPUID);
#endif

	printk("Setting stop_working_threads to 1\n");
	// Stop the code and wait
        stop_working_threads = 1;
        msleep(1000);

#ifdef DOLPHIN_TEST
	finish_dolphin();
#endif

#ifndef NO_CPU_SHUTDOWN

	// Unset the code callback
        set_fe_code_idle(0, CPUID);
	printkl("Will bring back CPU %d\n", CPUID);
        msleep(1000);
	// Bring the CPU back up
        cpu_up(CPUID);
        //msleep(1000);
	printkl("Brought the CPU back up\n");
#endif
	printk("Just before returning from cleanup_module for " SYSTEM_NAME_STRING_LOWER "\n");

}

MODULE_DESCRIPTION("Control system");
MODULE_AUTHOR("LIGO");
MODULE_LICENSE("Dual BSD/GPL");
