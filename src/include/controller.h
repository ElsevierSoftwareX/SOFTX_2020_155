// The following bits define the EPICS STATE_WORD
#define FE_ERROR_TIMING         0x2     // bit 1
#define FE_ERROR_ADC            0x4     // bit 2
#define FE_ERROR_DAC            0x8     // bit 3
#define FE_ERROR_DAQ            0x10    // bit 4
#define FE_ERROR_IPC            0x20    // bit 5
#define FE_ERROR_AWG            0x40    // bit 6
#define FE_ERROR_DAC_ENABLE     0x80    // bit 7
#define FE_ERROR_EXC_SET        0x100   // bit 8
#define FE_ERROR_OVERFLOW       0x200   // bit 9

#define ODC_ADC_OVF		0x1
#define ODC_DAC_OVF		0x2
#define ODC_EXC_SET		0x4



#define CPURATE (cpu_khz/1000)
#define ONE_PPS_THRESH 2000
#define SYNC_SRC_NONE           0
#define SYNC_SRC_IRIG_B         1
#define SYNC_SRC_1PPS           2
#define SYNC_SRC_TDS            4
#define SYNC_SRC_MASTER         8
#define TIME_ERR_IRIGB          0x10
#define TIME_ERR_1PPS           0x20
#define TIME_ERR_TDS            0x40

#define CPU_TIMER_CNT		10
#define CPU_TIME_CYCLE_START	0
#define CPU_TIME_CYCLE_END	1
#define CPU_TIME_USR_START	4
#define CPU_TIME_USR_END	5
#define CPU_TIME_RDY_ADC	8
#define CPU_TIME_ADC_WAIT	9


// Define standard values based on code rep rate **************************************
#ifdef SERVO256K
        #define CYCLE_PER_MINUTE        (2*7864320)
        #define DAQ_CYCLE_CHANGE        (2*8000)
        #define END_OF_DAQ_BLOCK        (2*8191)
        #define DAQ_RATE                (2*8192)
        #define NET_SEND_WAIT           (2*655360)
        #define CYCLE_TIME_ALRM         4
#endif
#ifdef SERVO128K
        #define CYCLE_PER_MINUTE        7864320
        #define DAQ_CYCLE_CHANGE        8000
        #define END_OF_DAQ_BLOCK        8191
        #define DAQ_RATE                8192
        #define NET_SEND_WAIT           655360
        #define CYCLE_TIME_ALRM         7
#endif
#ifdef SERVO64K
	#define CYCLE_PER_MINUTE	(2*1966080)
	#define DAQ_CYCLE_CHANGE	(2*1540)
	#define END_OF_DAQ_BLOCK	4095
	#define DAQ_RATE	(DAQ_16K_SAMPLE_SIZE*4)
	#define NET_SEND_WAIT		(2*81920)
	#define CYCLE_TIME_ALRM		15
	#define CYCLE_TIME_ALRM_HI	25
	#define CYCLE_TIME_ALRM_LO	10
	#define EPICS_128_SYNC		512
#ifdef ADC_SLAVE
	#define DAC_PRELOAD_CNT		1
#else
	#define DAC_PRELOAD_CNT		0
#endif
#endif
#ifdef SERVO32K
	#define CYCLE_PER_MINUTE	1966080
	#define DAQ_CYCLE_CHANGE	1540
	#define END_OF_DAQ_BLOCK	2047
	#define DAQ_RATE	(DAQ_16K_SAMPLE_SIZE*2)
	#define NET_SEND_WAIT		(2*81920)
	#define CYCLE_TIME_ALRM_HI	38
	#define CYCLE_TIME_ALRM_LO	25
        #define CYCLE_TIME_ALRM         31
	#define EPICS_128_SYNC		256
#ifdef ADC_SLAVE
	#define DAC_PRELOAD_CNT		2
#else
	#define DAC_PRELOAD_CNT		1
#endif
#endif

#ifdef SERVO16K
	#define CYCLE_PER_MINUTE	983040
	#define DAQ_CYCLE_CHANGE	770
	#define END_OF_DAQ_BLOCK	1023
	#define DAQ_RATE	DAQ_16K_SAMPLE_SIZE
	#define NET_SEND_WAIT		81920
	#define CYCLE_TIME_ALRM_HI	70
	#define CYCLE_TIME_ALRM_LO	50
        #define CYCLE_TIME_ALRM         62
	#define DAC_PRELOAD_CNT		4
	#define EPICS_128_SYNC		128
#endif
#ifdef SERVO4K
        #define CYCLE_PER_MINUTE        2*122880
        #define DAQ_CYCLE_CHANGE        240
        #define END_OF_DAQ_BLOCK        255
        #define DAQ_RATE        2*DAQ_2K_SAMPLE_SIZE
        #define NET_SEND_WAIT           2*10240
        #define CYCLE_TIME_ALRM         487/2
        #define CYCLE_TIME_ALRM_HI      500/2
        #define CYCLE_TIME_ALRM_LO      460/2
        #define DAC_PRELOAD_CNT         16      
	#define EPICS_128_SYNC		32
#endif
#ifdef SERVO2K
	#define CYCLE_PER_MINUTE	122880
	#define DAQ_CYCLE_CHANGE	120
	#define END_OF_DAQ_BLOCK	127
	#define DAQ_RATE	DAQ_2K_SAMPLE_SIZE
	#define NET_SEND_WAIT		10240
	#define CYCLE_TIME_ALRM		487
	#define CYCLE_TIME_ALRM_HI	500
	#define CYCLE_TIME_ALRM_LO	460
	#define DAC_PRELOAD_CNT		8	
	#define EPICS_128_SYNC		16
#endif


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
#define ADC_TIMEOUT_ERR		0x1

#define DAC_FOUND_BIT		1
#define DAC_TIMING_BIT		2
#define DAC_OVERFLOW_BIT	4
#define DAC_FIFO_BIT		8
#define DAC_WD_BIT		16	

#define MAX_IRIGB_SKEW		20
#define MIN_IRIGB_SKEW		5
#define DT_SAMPLE_OFFSET	6
#define DT_SAMPLE_CNT		12
#define MAX_DT_DIAG_VAL         6
#define MIN_DT_DIAG_VAL         5


// HOUSEKEEPING CYCLE DEFS
// 1Hz Jobs triggered by cycleNum count in controller code
#define HKP_READ_SYMCOM_IRIGB		0
#define HKP_READ_TSYNC_IRIBB		1
#define HKP_READ_DIO			10
#define HKP_DT_CALC			16
#define HKP_DAC_DT_SWITCH		17
#define HKP_TIMING_UPDATES		18
#define HKP_DIAG_UPDATES		19
#define HKP_DAC_EPICS_UPDATES		20
#define HKP_ADC_DAC_STAT_UPDATES	21
#define HKP_RFM_CHK_CYCLE		300	// ONLY IOP
#define HKP_DAC_WD_CLK			400	// ONLY IOP
#define HKP_DAC_WD_CHK			500	// ONLY IOP
#define HKP_DAC_FIFO_CHK		600	// ONLY IOP
// 16Hz Jobs triggered by subcycle count in controller code
#define HKP_FM_EPICS_UPDATE		30

#define NUM_SYSTEMS 1
#define INLINE  inline
#define MMAP_SIZE (64*1024*1024 - 5000)

#define STACK_SIZE    40000
#define TICK_PERIOD   100000
#define PERIOD_COUNT  1
#define MAX_UDELAY    19999


char *build_date = __DATE__ " " __TIME__;
extern int iop_rfm_valid;
volatile char *_epics_shm;      ///< Ptr to EPICS shared memory area
char *_ipc_shm;                 ///< Ptr to inter-process communication area 
char *_daq_shm;                 ///< Ptr to frame builder comm shared mem area 
int daq_fd;                     ///< File descriptor to share memory file 

long daqBuffer;                 // Address for daq dual buffers in daqLib.c
CDS_HARDWARE cdsPciModules;     // Structure of PCI hardware addresses
volatile IO_MEM_DATA *ioMemData;
volatile int vmeDone = 0;       // Code kill command
volatile int stop_working_threads = 0;

extern unsigned int cpu_khz;

/// Testpoints which are not part of filter modules
double *testpoint[GDS_MAX_NFM_TP];
#ifndef NO_DAQ
DAQ_RANGE daq;                  // Range settings for daqLib.c
int numFb = 0;
int fbStat[2] = {0,0};          // Status of DAQ backend computer
/// Excitation points which are not part of filter modules
double xExc[GDS_MAX_NFM_EXC];   // GDS EXC not associated with filter modules
#endif
/// 1/16 sec cycle counters for DAQS 
int subcycle = 0;               // Internal cycle counter        
/// DAQ cycle counter (0-15)
unsigned int daqCycle;          // DAQS cycle counter   

// Sharded memory discriptors
int wfd, ipc_fd;
volatile CDS_EPICS *pLocalEpics;        // Local mem ptr to EPICS control data
volatile char *pEpicsDaq;        // Local mem ptr to EPICS daq data


// Filter module variables
/// Standard Filter Module Structure
FILT_MOD dsp[NUM_SYSTEMS];                                      // SFM structure.       
/// Pointer to local memory SFM Structure
FILT_MOD *dspPtr[NUM_SYSTEMS];                                  // SFM structure pointer.
/// Pointer to SFM in shared memory.
FILT_MOD *pDsp[NUM_SYSTEMS];                                    // Ptr to SFM in shmem. 
/// Pointer to filter coeffs local memory.
COEF dspCoeff[NUM_SYSTEMS];                                     // Local mem for SFM coeffs.
/// Pointer to filter coeffs shared memory.
VME_COEF *pCoeff[NUM_SYSTEMS];                                  // Ptr to SFM coeffs in shmem   

// ADC Variables
/// Array of ADC values
double dWord[MAX_ADC_MODULES][MAX_ADC_CHN_PER_MOD];             // ADC read values
/// List of ADC channels used by this app. Used to determine if downsampling required.
unsigned int dWordUsed[MAX_ADC_MODULES][MAX_ADC_CHN_PER_MOD];   // ADC chans used by app code
/// Arrary of ADC overflow counters.
int overflowAdc[MAX_ADC_MODULES][MAX_ADC_CHN_PER_MOD];          // ADC overflow diagnostics

// DAC Variables
/// Enables writing of DAC values; Used with DACKILL parts..
int iopDacEnable;                                               // Returned by feCode to allow writing values or zeros to DAC modules
int dacChanErr[MAX_DAC_MODULES];
#ifdef ADC_MASTER
int dacOutBufSize [MAX_DAC_MODULES];    
#endif
/// Array of DAC output values.
double dacOut[MAX_DAC_MODULES][MAX_DAC_CHN_PER_MOD];            // DAC output values
/// DAC output values returned to EPICS
int dacOutEpics[MAX_DAC_MODULES][MAX_DAC_CHN_PER_MOD];          // DAC outputs reported back to EPICS
/// DAC channels used by an app.; determines up sampling required.
unsigned int dacOutUsed[MAX_DAC_MODULES][MAX_DAC_CHN_PER_MOD];  // DAC chans used by app code
/// Array of DAC overflow (overrange) counters.
int overflowDac[MAX_DAC_MODULES][MAX_DAC_CHN_PER_MOD];          // DAC overflow diagnostics
/// DAC outputs stored as floats, to be picked up as test points
double floatDacOut[160]; // DAC outputs stored as floats, to be picked up as test points

/// Counter for total ADC/DAC overflows
int overflowAcc = 0;                                            // Total ADC/DAC overflow counter

#ifndef ADC_MASTER
// Variables for Digital I/O board values
// DIO board I/O is handled in slave (user) applications for timing reasons (longer I/O access times)
/// Read value from Acces I/O 24bit module
int dioInput[MAX_DIO_MODULES];
/// Write value to Acces I/O 24bit module
int dioOutput[MAX_DIO_MODULES];
/// Last value written to Acces I/O 24bit module
int dioOutputHold[MAX_DIO_MODULES];

int rioInputOutput[MAX_DIO_MODULES];
int rioOutput[MAX_DIO_MODULES];
int rioOutputHold[MAX_DIO_MODULES];

int rioInput1[MAX_DIO_MODULES];
int rioOutput1[MAX_DIO_MODULES];
int rioOutputHold1[MAX_DIO_MODULES];

// Contec 32 bit output modules
/// Read value from Contec 32bit I/O module
unsigned int CDO32Input[MAX_DIO_MODULES];
/// Write value to Contec 32bit I/O module
unsigned int CDO32Output[MAX_DIO_MODULES];

#endif

// /proc epics channel interface
struct proc_epics {
	char *name;
	unsigned long idx;
	unsigned long mask_idx;
	unsigned char type;
	unsigned char in;
	unsigned char nrow; /* matrix */
	unsigned char ncol; /* matrix */
};

// Maximum number of setpoint futures
#define MAX_PROC_FUTURES 256

// Array of setpoint futures
struct proc_futures {
	struct proc_epics *proc_epics; // Pointer to the proc_epics array entry
	unsigned long gps;	// GPS time of the setting in the future
	unsigned long cycle;	// code cycle within "gps" second of the setting in the future
	double val;		// New value
	unsigned int idx;	// index (positive for matrix only)
} proc_futures[MAX_PROC_FUTURES];

