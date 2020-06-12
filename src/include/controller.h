adcInfo_t     adcinfo;
dacInfo_t     dacinfo;
timing_diag_t timeinfo;

/// Maintains present cycle count within a one second period.
int          cycleNum = 0;
unsigned int odcStateWord = 0xffff;
/// Value of readback from DAC FIFO size registers; used in diags for FIFO
/// overflow/underflow.
int          out_buf_size = 0; // test checking DAC buffer size
unsigned int cycle_gps_time = 0; // Time at which ADCs triggered
unsigned int cycle_gps_event_time = 0; // Time at which ADCs triggered
unsigned int cycle_gps_ns = 0;
unsigned int cycle_gps_event_ns = 0;
unsigned int gps_receiver_locked = 0; // Lock/unlock flag for GPS time card
/// GPS time in GPS seconds
unsigned int     timeSec = 0;
unsigned int     timeSecDiag = 0;
unsigned int     ipcErrBits = 0;
int              cardCountErr = 0;
struct rmIpcStr* daqPtr;
int dacOF[ MAX_DAC_MODULES ]; /// @param dacOF[]  DAC overrange counters

char daqArea[ 2 * DAQ_DCU_SIZE ]; // Space allocation for daqLib buffers
int  cpuId = 1;

#ifdef DUAL_DAQ_DC
#define MX_OK 15
#else
#define MX_OK 3
#endif

// Initial diag reset flag
int initialDiagReset = 1;

// Cache flushing mumbo jumbo suggested by Thomas Gleixner, it is probably
// useless Did not see any effect
char fp[ 64 * 1024 ];

// The following bits define the EPICS STATE_WORD
#define FE_ERROR_TIMING 0x2 // bit 1
#define FE_ERROR_ADC 0x4 // bit 2
#define FE_ERROR_DAC 0x8 // bit 3
#define FE_ERROR_DAQ 0x10 // bit 4
#define FE_ERROR_IPC 0x20 // bit 5
#define FE_ERROR_AWG 0x40 // bit 6
#define FE_ERROR_DAC_ENABLE 0x80 // bit 7
#define FE_ERROR_EXC_SET 0x100 // bit 8
#define FE_ERROR_OVERFLOW 0x200 // bit 9
#define FE_ERROR_CFC 0x400 // bit 10, used by and also defined in skeleton.st

#define ODC_ADC_OVF 0x1
#define ODC_DAC_OVF 0x2
#define ODC_EXC_SET 0x4

#define CPURATE ( cpu_khz / 1000 )
#define ONE_PPS_THRESH 2000
#define SYNC_SRC_NONE 0
#define SYNC_SRC_DOLPHIN 1
#define SYNC_SRC_1PPS 2
#define SYNC_SRC_TDS 4
#define SYNC_SRC_MASTER 8
#define SYNC_SRC_TIMER 16
#define TIME_ERR_IRIGB 0x10
#define TIME_ERR_1PPS 0x20
#define TIME_ERR_TDS 0x40

#define CPU_TIMER_CNT 10
#define CPU_TIME_CYCLE_START 0
#define CPU_TIME_CYCLE_END 1
#define CPU_TIME_USR_START 4
#define CPU_TIME_USR_END 5
#define CPU_TIME_RDY_ADC 8
#define CPU_TIME_ADC_WAIT 9

// fe_state defs
#define IO_CONFIG_ERROR -8
#define ADC_TO_ERROR -7
#define DAC_INIT_ERROR -6
#define BURT_RESTORE_ERROR -5
#define CHAN_HOP_ERROR -4
#define RUN_ON_TIMER -3
#define DAQ_INIT_ERROR -2
#define FILT_INIT_ERROR -1
#define LOADING 0
#define FIND_MODULES 1
#define WAIT_BURT 2
#define LOCKING_CORE 3
#define INIT_ADC_MODS 4
#define INIT_DAC_MODS 5
#define INIT_SYNC 6
#define NORMAL_RUN 7

// Define standard values based on code rep rate
// **************************************
#ifdef SERVO1024K
#define CYCLE_PER_MINUTE ( 8 * 7864320 )
#define DAQ_CYCLE_CHANGE ( 32 * 8000 )
#define END_OF_DAQ_BLOCK 524287
#define DAQ_RATE ( DAQ_16K_SAMPLE_SIZE * 32 )
#define NET_SEND_WAIT ( 32 * 81920 )
#define CYCLE_TIME_ALRM 15
#define CYCLE_TIME_ALRM_HI (25 * UNDERSAMPLE)
#define CYCLE_TIME_ALRM_LO 10
#define EPICS_128_SYNC 2048
#define DAC_PRELOAD_CNT 0
#endif
#ifdef SERVO512K
#define CYCLE_PER_MINUTE ( 4 * 7864320 )
#define DAQ_CYCLE_CHANGE ( 32 * 8000 )
#define END_OF_DAQ_BLOCK 524287
#define DAQ_RATE ( DAQ_16K_SAMPLE_SIZE * 32 )
#define NET_SEND_WAIT ( 32 * 81920 )
#define CYCLE_TIME_ALRM 15
#define CYCLE_TIME_ALRM_HI 25
#define CYCLE_TIME_ALRM_LO 10
#define EPICS_128_SYNC 2048
#define DAC_PRELOAD_CNT 0
#endif
#ifdef SERVO256K
#define CYCLE_PER_MINUTE ( 2 * 7864320 )
#define DAQ_CYCLE_CHANGE ( 2 * 8000 )
#define END_OF_DAQ_BLOCK ( 2 * 8191 )
#define DAQ_RATE ( DAQ_16K_SAMPLE_SIZE * 16 )
#define NET_SEND_WAIT ( 2 * 655360 )
#define CYCLE_TIME_ALRM 4
#define CYCLE_TIME_ALRM_HI 5
#define CYCLE_TIME_ALRM_LO 1
#define EPICS_128_SYNC 2048
#define DAC_PRELOAD_CNT 0
#endif
#ifdef SERVO128K
#define CYCLE_PER_MINUTE 7864320
#define DAQ_CYCLE_CHANGE 8000
#define END_OF_DAQ_BLOCK 8191
#define DAQ_RATE ( DAQ_16K_SAMPLE_SIZE * 8 )
#define NET_SEND_WAIT 655360
#define CYCLE_TIME_ALRM 7
#define CYCLE_TIME_ALRM_HI 9
#define CYCLE_TIME_ALRM_LO 1
#define EPICS_128_SYNC 1024
#define DAC_PRELOAD_CNT 0
#endif
#ifdef SERVO64K
#define CYCLE_PER_MINUTE ( 2 * 1966080 )
#define DAQ_CYCLE_CHANGE ( 2 * 1540 )
#define END_OF_DAQ_BLOCK 4095
#define DAQ_RATE ( DAQ_16K_SAMPLE_SIZE * 4 )
#define NET_SEND_WAIT ( 2 * 81920 )
#define CYCLE_TIME_ALRM 15
#define CYCLE_TIME_ALRM_HI (20 * UNDERSAMPLE)
#define CYCLE_TIME_ALRM_LO 10
#define EPICS_128_SYNC 512
#ifdef CONTROL_MODEL
#define DAC_PRELOAD_CNT 1
#else
#define DAC_PRELOAD_CNT 0
#endif
#endif
#ifdef SERVO32K
#define CYCLE_PER_MINUTE 1966080
#define DAQ_CYCLE_CHANGE 1540
#define END_OF_DAQ_BLOCK 2047
#define DAQ_RATE ( DAQ_16K_SAMPLE_SIZE * 2 )
#define NET_SEND_WAIT ( 2 * 81920 )
#define CYCLE_TIME_ALRM_HI 38
#define CYCLE_TIME_ALRM_LO 25
#define CYCLE_TIME_ALRM 31
#define EPICS_128_SYNC 256
#ifdef CONTROL_MODEL
#define DAC_PRELOAD_CNT 2
#else
#define DAC_PRELOAD_CNT 1
#endif
#endif

#ifdef SERVO16K
#define CYCLE_PER_MINUTE 983040
#define DAQ_CYCLE_CHANGE 770
#define END_OF_DAQ_BLOCK 1023
#define DAQ_RATE DAQ_16K_SAMPLE_SIZE
#define NET_SEND_WAIT 81920
#define CYCLE_TIME_ALRM_HI 70
#define CYCLE_TIME_ALRM_LO 50
#define CYCLE_TIME_ALRM 62
#define DAC_PRELOAD_CNT 4
#define EPICS_128_SYNC 128
#endif
#ifdef SERVO4K
#define CYCLE_PER_MINUTE 2 * 122880
#define DAQ_CYCLE_CHANGE 240
#define END_OF_DAQ_BLOCK 255
#define DAQ_RATE 2 * DAQ_2K_SAMPLE_SIZE
#define NET_SEND_WAIT 2 * 10240
#define CYCLE_TIME_ALRM 487 / 2
#define CYCLE_TIME_ALRM_HI 500 / 2
#define CYCLE_TIME_ALRM_LO 460 / 2
#define DAC_PRELOAD_CNT 16
#define EPICS_128_SYNC 32
#endif
#ifdef SERVO2K
#define CYCLE_PER_MINUTE 122880
#define DAQ_CYCLE_CHANGE 120
#define END_OF_DAQ_BLOCK 127
#define DAQ_RATE DAQ_2K_SAMPLE_SIZE
#define NET_SEND_WAIT 10240
#define CYCLE_TIME_ALRM 487
#define CYCLE_TIME_ALRM_HI 500
#define CYCLE_TIME_ALRM_LO 460
#define DAC_PRELOAD_CNT 8
#define EPICS_128_SYNC 16
#endif

// DIAGNOSTIC_RETURNS_FROM_FE
#define FE_NO_ERROR 0x0
#define FE_SYNC_ERR 0x1
#define FE_ADC_HOLD_ERR 0x2
#define FE_FB0_NOT_ONLINE 0x4
#define FE_PROC_TIME_ERR 0x8
#define FE_ADC_SYNC_ERR 0xf0
#define FE_FB_AVAIL 0x1
#define FE_FB_ONLINE 0x2
#define FE_MAX_FB_QUE 0x10
#define ADC_TIMEOUT_ERR 0x1

#define DAC_FOUND_BIT 1
#define DAC_TIMING_BIT 2
#define DAC_OVERFLOW_BIT 4
#define DAC_FIFO_BIT 8
#define DAC_WD_BIT 16
#define DAC_FIFO_EMPTY 32
#define DAC_FIFO_HI_QTR 64
#define DAC_FIFO_FULL 128

#define MAX_IRIGB_SKEW  (20 * UNDERSAMPLE)
#define MIN_IRIGB_SKEW 5
#define DT_SAMPLE_OFFSET 6
#define DT_SAMPLE_CNT 12
#define MAX_DT_DIAG_VAL 6
#define MIN_DT_DIAG_VAL 5

// HOUSEKEEPING CYCLE DEFS
// 1Hz Jobs triggered by cycleNum count in controller code
#define HKP_READ_SYMCOM_IRIGB 0
#define HKP_READ_TSYNC_IRIBB 1
#define HKP_READ_DIO 10
#define HKP_DT_CALC 16
#define HKP_DAC_DT_SWITCH 17
#define HKP_TIMING_UPDATES 18
#define HKP_DIAG_UPDATES 19
#define HKP_DAC_EPICS_UPDATES 20
#define HKP_ADC_DAC_STAT_UPDATES 21
#define HKP_RFM_CHK_CYCLE 300 // ONLY IOP
#define HKP_DAC_WD_CLK 400 // ONLY IOP
#define HKP_DAC_WD_CHK 500 // ONLY IOP
#define HKP_DAC_FIFO_CHK 600 // ONLY IOP
// 16Hz Jobs triggered by subcycle count in controller code
#define HKP_FM_EPICS_UPDATE 30

#define NUM_SYSTEMS 1
#define INLINE inline
#define MMAP_SIZE ( 64 * 1024 * 1024 - 5000 )

#define STACK_SIZE 40000
#define TICK_PERIOD 100000
#define PERIOD_COUNT 1
#define MAX_UDELAY 19999

char*          build_date = __DATE__ " " __TIME__;
extern int     iop_rfm_valid;
volatile char* _epics_shm; ///< Ptr to EPICS shared memory area
char*          _ipc_shm; ///< Ptr to inter-process communication area
char*          _daq_shm; ///< Ptr to frame builder comm shared mem area
// char*          _gds_shm; ///< Ptr to frame builder comm shared mem area
char*          _shmipc_shm; ///< Ptr to IOP I/O data to/from User app shared mem area
char*          _io_shm; ///< Ptr to user space I/O area
int            daq_fd; ///< File descriptor to share memory file

long                      daqBuffer; // Address for daq dual buffers in daqLib.c
CDS_HARDWARE              cdsPciModules; // Structure of PCI hardware addresses
volatile IO_MEM_DATA*     ioMemData;
volatile IO_MEM_DATA_IOP* ioMemDataIop;
volatile int              vmeDone = 0; // Code kill command
volatile int              fe_status_return = 0; // fe code status return to module_exit
volatile int              fe_status_return_subcode = 0; // fe code status return to module_exit
volatile int              stop_working_threads = 0;

extern unsigned int cpu_khz;

/// Testpoints which are not part of filter modules
double* testpoint[ GDS_MAX_NFM_TP ];
#ifndef NO_DAQ
DAQ_RANGE daq; // Range settings for daqLib.c
int       numFb = 0;
int       fbStat[ 2 ] = { 0, 0 }; // Status of DAQ backend computer
/// Excitation points which are not part of filter modules
double xExc[ GDS_MAX_NFM_EXC ]; // GDS EXC not associated with filter modules
#endif
/// 1/16 sec cycle counters for DAQS
int subcycle = 0; // Internal cycle counter
/// DAQ cycle counter (0-15)
unsigned int daqCycle; // DAQS cycle counter

// Sharded memory discriptors
int                 wfd, ipc_fd;
volatile CDS_EPICS* pLocalEpics; // Local mem ptr to EPICS control data
volatile char*      pEpicsDaq; // Local mem ptr to EPICS daq data

// Filter module variables
/// Standard Filter Module Structure
FILT_MOD dsp[ NUM_SYSTEMS ]; // SFM structure.
/// Pointer to local memory SFM Structure
FILT_MOD* dspPtr[ NUM_SYSTEMS ]; // SFM structure pointer.
/// Pointer to SFM in shared memory.
FILT_MOD* pDsp[ NUM_SYSTEMS ]; // Ptr to SFM in shmem.
/// Pointer to filter coeffs local memory.
COEF dspCoeff[ NUM_SYSTEMS ]; // Local mem for SFM coeffs.
/// Pointer to filter coeffs shared memory.
VME_COEF* pCoeff[ NUM_SYSTEMS ]; // Ptr to SFM coeffs in shmem

// ADC Variables
/// Array of ADC values
#ifdef IOP_MODEL
double dWord[ MAX_ADC_MODULES ][ MAX_ADC_CHN_PER_MOD ][ 16 ]; // ADC read values
#else
double dWord[ MAX_ADC_MODULES ][ MAX_ADC_CHN_PER_MOD ]; // ADC read values
#endif
/// List of ADC channels used by this app. Used to determine if downsampling
/// required.
unsigned int dWordUsed[ MAX_ADC_MODULES ]
                      [ MAX_ADC_CHN_PER_MOD ]; // ADC chans used by app code

// DAC Variables
/// Enables writing of DAC values; Used with DACKILL parts..
int iopDacEnable; // Returned by feCode to allow writing values or zeros to DAC
                  // modules
int dacChanErr[ MAX_DAC_MODULES ];
#ifdef IOP_MODEL
int dacOutBufSize[ MAX_DAC_MODULES ];
#endif
/// Array of DAC output values.
double dacOut[ MAX_DAC_MODULES ][ MAX_DAC_CHN_PER_MOD ]; // DAC output values
/// DAC output values returned to EPICS
int dacOutEpics[ MAX_DAC_MODULES ]
               [ MAX_DAC_CHN_PER_MOD ]; // DAC outputs reported back to EPICS
/// DAC channels used by an app.; determines up sampling required.
unsigned int dacOutUsed[ MAX_DAC_MODULES ]
                       [ MAX_DAC_CHN_PER_MOD ]; // DAC chans used by app code
/// Array of DAC overflow (overrange) counters.
int overflowDac[ MAX_DAC_MODULES ]
               [ MAX_DAC_CHN_PER_MOD ]; // DAC overflow diagnostics
/// DAC outputs stored as floats, to be picked up as test points
double floatDacOut[ 160 ]; // DAC outputs stored as floats, to be picked up as
                           // test points

/// Counter for total ADC/DAC overflows
int overflowAcc = 0; // Total ADC/DAC overflow counter

#ifndef IOP_MODEL
// Variables for Digital I/O board values
// DIO board I/O is handled in control (user) applications for timing reasons
// (longer I/O access times)
/// Read value from Acces I/O 24bit module
int dioInput[ MAX_DIO_MODULES ];
/// Write value to Acces I/O 24bit module
int dioOutput[ MAX_DIO_MODULES ];
/// Last value written to Acces I/O 24bit module
int dioOutputHold[ MAX_DIO_MODULES ];

int rioInputOutput[ MAX_DIO_MODULES ];
int rioOutput[ MAX_DIO_MODULES ];
int rioOutputHold[ MAX_DIO_MODULES ];

int rioInput1[ MAX_DIO_MODULES ];
int rioOutput1[ MAX_DIO_MODULES ];
int rioOutputHold1[ MAX_DIO_MODULES ];

// Contec 32 bit output modules
/// Read value from Contec 32bit I/O module
unsigned int CDO32Input[ MAX_DIO_MODULES ];
/// Write value to Contec 32bit I/O module
unsigned int CDO32Output[ MAX_DIO_MODULES ];

#endif

// Up/Down Sampling Filter Coeffs
// All systems not running at 64K require up/down sampling to communicate I/O
// data with IOP, which is running at 64K. Following defines the filter coeffs
// for these up/down filters.
#ifdef OVERSAMPLE
/* Recalculated filters in biquad form */

/* Oversamping base rate is 64K */
/* Coeffs for the 2x downsampling (32K system) filter */
static double __attribute__( ( unused ) ) feCoeff2x[ 9 ] = {
    0.053628649721183,   0.2568759660371100, -0.3225906481359000,
    1.2568801238621801,  1.6774135096891700, -0.2061764045745400,
    -1.0941543149527400, 2.0846376586498803, 2.1966597482716801
};

/* RCG V3.0 Coeffs for the 4x downsampling (16K system) filter */
static double __attribute__( ( unused ) )
feCoeff4x[ 9 ] = { 0.054285975,         0.3890221,        -0.17645085,
                   -0.0417771600000001, 0.41775916,       0.52191125,
                   -0.37884382,         1.52190741336686, 1.69347541336686 };

// Pre RCG 3.0 filters
//      {0.014805052402446,
//      0.7166258547451800, -0.0683289874517300, 0.3031629575762000,
//      0.5171469569032900, 0.6838596423885499,
//      -0.2534855521841101, 1.6838609161411500, 1.7447155374502499};

//
// New Brian Lantz 4k decimation filter
static double __attribute__( ( unused ) ) feCoeff16x[ 9 ] = {
    0.010203728365,      0.8052941009065100,  -0.0241751519071000,
    0.3920490703701900,  0.5612099784288400,  0.8339678987936501,
    -0.0376022631287799, -0.0131581721533700, 0.1145865116421301
};

/* Coeffs for the 32x downsampling filter (2K system) */
#if 0
/* Original Rana coeffs from 40m lab elog */
static double __attribute__ ((unused)) feCoeff32x[9] =
        {0.0001104130574447,
        0.9701834961388200, -0.0010837026165800, -0.0200761119821899, 0.0085463156103800,
        0.9871502388637901, -0.0039246182095299, 3.9871502388637898, 3.9960753817904697};
#endif

/* Coeffs for the 32x downsampling filter (2K system) per Brian Lantz May 5,
 * 2009 */
static double __attribute__( ( unused ) ) feCoeff32x[ 9 ] = {
    0.010581064947739,      0.90444302586137004,  -0.0063413204375699639,
    -0.056459743474659874,  0.032075154877300172, 0.92390910024681006,
    -0.0097523655540199261, 0.077383808424050127, 0.14238741130302013
};

// History buffers for oversampling filters
double dHistory[ ( MAX_ADC_MODULES * 32 ) ][ MAX_HISTRY ];
double dDacHistory[ ( MAX_DAC_MODULES * 16 ) ][ MAX_HISTRY ];

#else

#define OVERSAMPLE_TIMES 1

#endif
