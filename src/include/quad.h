/* Assign filter names to filter number */
#define FILT_M0_SEN1    0
#define FILT_M0_SEN2    1
#define FILT_M0_SEN3    2
#define FILT_M0_SEN4    3
#define FILT_M0_SEN5    4
#define FILT_M0_SEN6    5
#define FILT_M0_DOF1    6
#define FILT_M0_DOF2    7
#define FILT_M0_DOF3    8
#define FILT_M0_DOF4    9
#define FILT_M0_DOF5    10
#define FILT_M0_DOF6    11
#define FILT_M0_DAC1    12
#define FILT_M0_DAC2    13
#define FILT_M0_DAC3    14
#define FILT_M0_DAC4    15
#define FILT_M0_DAC5    16
#define FILT_M0_DAC6    17
#define FILT_R0_SEN1    18
#define FILT_R0_SEN2    19
#define FILT_R0_SEN3    20
#define FILT_R0_SEN4    21
#define FILT_R0_SEN5    22
#define FILT_R0_SEN6    23
#define FILT_R0_DOF1    24
#define FILT_R0_DOF2    25
#define FILT_R0_DOF3    26
#define FILT_R0_DOF4    27
#define FILT_R0_DOF5    28
#define FILT_R0_DOF6    29
#define FILT_R0_DAC1    30
#define FILT_R0_DAC2    31
#define FILT_R0_DAC3    32
#define FILT_R0_DAC4    33
#define FILT_R0_DAC5    34
#define FILT_R0_DAC6    35
#define FILT_L1_ULSEN    36
#define FILT_L1_LLSEN    37
#define FILT_L1_URSEN    38
#define FILT_L1_LRSEN    39
#define FILT_L1_POS    	40
#define FILT_L1_PIT    	41
#define FILT_L1_YAW    	42
#define FILT_L1_LSC    	43
#define FILT_L1_ASCP   	44
#define FILT_L1_ASCY   	45
#define FILT_L1_ULPOS  	46
#define FILT_L1_ULPIT  	47
#define FILT_L1_ULYAW  	48
#define FILT_L1_LLPOS  	49
#define FILT_L1_LLPIT  	50
#define FILT_L1_LLYAW  	51
#define FILT_L1_URPOS  	52
#define FILT_L1_URPIT  	53
#define FILT_L1_URYAW  	54
#define FILT_L1_LRPOS  	55
#define FILT_L1_LRPIT  	56
#define FILT_L1_LRYAW  	57
#define FILT_L1_ULOUT    58
#define FILT_L1_LLOUT    59
#define FILT_L1_UROUT    60
#define FILT_L1_LROUT    61
#define FILT_L2_ULSEN    62
#define FILT_L2_LLSEN    63
#define FILT_L2_URSEN    64
#define FILT_L2_LRSEN    65
#define FILT_L2_POS    	66
#define FILT_L2_PIT    	67
#define FILT_L2_YAW    	68
#define FILT_L2_LSC    	69
#define FILT_L2_ASCP   	70
#define FILT_L2_ASCY   	71
#define FILT_L2_ULPOS  	72
#define FILT_L2_ULPIT  	73
#define FILT_L2_ULYAW  	74
#define FILT_L2_LLPOS  	75
#define FILT_L2_LLPIT  	76
#define FILT_L2_LLYAW  	77
#define FILT_L2_URPOS  	78
#define FILT_L2_URPIT  	79
#define FILT_L2_URYAW  	80
#define FILT_L2_LRPOS  	81
#define FILT_L2_LRPIT  	82
#define FILT_L2_LRYAW  	83
#define FILT_L2_ULOUT    84
#define FILT_L2_LLOUT    85
#define FILT_L2_UROUT    86
#define FILT_L2_LROUT    87
#define FILT_ESD_LSC    88
#define FILT_ESD_ASCP    89
#define FILT_ESD_ASCY    90
#define FILT_ESD_BIAS    91
#define FILT_ESD_Q1    92
#define FILT_ESD_Q2    93
#define FILT_ESD_Q3    94
#define FILT_ESD_Q4    95


typedef struct CDS_EPICS_IN {
	float m0InputMatrix[6][6];
	float m0OutputMatrix[6][6];
	float r0InputMatrix[6][6];
	float r0OutputMatrix[6][6];
	float l1InputMatrix[4][3];
	float l2InputMatrix[4][3];
	float l3OutputMatrix[3][5];
	float vmeReset;
	int burtRestore;
	int dcuId;
	int diagReset;
	int syncReset;
	int overflowReset;
} CDS_EPICS_IN;

typedef struct CDS_EPICS_OUT {
	int onePps;
	int adcWaitTime;
	int diagWord;
	int cpuMeter;
	int cpuMeterMax;
	int gdsMon[32];
	int diags[4];
	int overflowAdc[4][32];
	int overflowDac[4][16];
	int ovAccum;
} CDS_EPICS_OUT;

typedef struct CDS_EPICS {
	CDS_EPICS_IN epicsInput;
	CDS_EPICS_OUT epicsOutput;
} CDS_EPICS;
