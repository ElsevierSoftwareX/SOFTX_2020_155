
#define FE_DIAGS_USER_TIME	0
#define FE_DIAGS_IPC_STAT	1
#define FE_DIAGS_FB_NET_STAT	2
#define FE_DIAGS_DAQ_BYTE_CNT	3
#define FE_DIAGS_DUOTONE_TIME	4
#define FE_DIAGS_IRIGB_TIME	5
#define FE_DIAGS_ADC_STAT	6
#define FE_DIAGS_DAC_STAT	7
#define FE_DIAGS_DAC_MASTER_STAT	8
#define FE_DIAGS_AWGTPMAN	9
#define FE_DIAGS_DAC_DUO	10	

#define FE_ERROR_OVERFLOW	0x40	// bit 6
#define FE_ERROR_DAC_ENABLE	0x80	// bit 7
#define FE_ERROR_CHAN_HOP	0x100	// bit 8

#define FE_ERROR_TIMING		0x2	// bit 1
#define FE_ERROR_ADC		0x4	// bit 2
#define FE_ERROR_DAC		0x8	// bit 3
#define FE_ERROR_DAQ		0x10	// bit 4
#define FE_ERROR_IPC		0x20	// bit 5
#define FE_ERROR_AWG		0x40	// bit 6
#define FE_ERROR_DAC_ENABLE	0x80	// bit 7
#define FE_ERROR_EXC_SET	0x100	// bit 8
#define FE_ERROR_OVERFLOW	0x200	// bit 9


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

// HOUSEKEEPING CYCLE DEFS
// 1Hz Jobs triggered by cycleNum count in controller code
#define HKP_READ_SYMCOM_IRIGB		0
#define HKP_READ_TSYNC_IRIBB		1
#define HKP_DT_CALC			16
#define HKP_DAC_DT_SWITCH		17
#define HKP_TIMING_UPDATES		18
#define HKP_DIAG_UPDATES		19
#define HKP_ADC_DAC_STAT_UPDATES	21
#define HKP_RFM_CHK_CYCLE		300	// ONLY IOP
#define HKP_DAC_WD_CLK			400	// ONLY IOP
#define HKP_DAC_WD_CHK			500	// ONLY IOP
#define HKP_DAC_FIFO_CHK		600	// ONLY IOP
// 16Hz Jobs triggered by subcycle count in controller code
#define HKP_DAC_EPICS_UPDATES		20
#define HKP_FM_EPICS_UPDATE		30
