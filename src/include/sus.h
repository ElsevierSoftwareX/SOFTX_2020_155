#ifndef SUS_H_INCLUDED
#define SUS_H_INCLUDED
#define Q1_ESD_ASCP 	 0
#define Q1_ESD_ASCY 	 1
#define Q1_ESD_BIAS 	 2
#define Q1_ESD_LSC 	 3
#define Q1_ESD_Q1 	 4
#define Q1_ESD_Q2 	 5
#define Q1_ESD_Q3 	 6
#define Q1_ESD_Q4 	 7
#define Q1_L1_ASCP 	 8
#define Q1_L1_ASCY 	 9
#define Q1_L1_LLOUT 	 10
#define Q1_L1_LLPIT 	 11
#define Q1_L1_LLPOS 	 12
#define Q1_L1_LLSEN 	 13
#define Q1_L1_LLYAW 	 14
#define Q1_L1_LROUT 	 15
#define Q1_L1_LRPIT 	 16
#define Q1_L1_LRPOS 	 17
#define Q1_L1_LRSEN 	 18
#define Q1_L1_LRYAW 	 19
#define Q1_L1_LSC 	 20
#define Q1_L1_PIT 	 21
#define Q1_L1_POS 	 22
#define Q1_L1_ULOUT 	 23
#define Q1_L1_ULPIT 	 24
#define Q1_L1_ULPOS 	 25
#define Q1_L1_ULSEN 	 26
#define Q1_L1_ULYAW 	 27
#define Q1_L1_UROUT 	 28
#define Q1_L1_URPIT 	 29
#define Q1_L1_URPOS 	 30
#define Q1_L1_URSEN 	 31
#define Q1_L1_URYAW 	 32
#define Q1_L1_YAW 	 33
#define Q1_L2_ASCP 	 34
#define Q1_L2_ASCY 	 35
#define Q1_L2_LLOUT 	 36
#define Q1_L2_LLPIT 	 37
#define Q1_L2_LLPOS 	 38
#define Q1_L2_LLSEN 	 39
#define Q1_L2_LLYAW 	 40
#define Q1_L2_LROUT 	 41
#define Q1_L2_LRPIT 	 42
#define Q1_L2_LRPOS 	 43
#define Q1_L2_LRSEN 	 44
#define Q1_L2_LRYAW 	 45
#define Q1_L2_LSC 	 46
#define Q1_L2_PIT 	 47
#define Q1_L2_POS 	 48
#define Q1_L2_ULOUT 	 49
#define Q1_L2_ULPIT 	 50
#define Q1_L2_ULPOS 	 51
#define Q1_L2_ULSEN 	 52
#define Q1_L2_ULYAW 	 53
#define Q1_L2_UROUT 	 54
#define Q1_L2_URPIT 	 55
#define Q1_L2_URPOS 	 56
#define Q1_L2_URSEN 	 57
#define Q1_L2_URYAW 	 58
#define Q1_L2_YAW 	 59
#define Q1_M0_DOF1 	 60
#define Q1_M0_DOF2 	 61
#define Q1_M0_DOF3 	 62
#define Q1_M0_DOF4 	 63
#define Q1_M0_DOF5 	 64
#define Q1_M0_DOF6 	 65
#define Q1_M0_F1_ACT 	 66
#define Q1_M0_F2_ACT 	 67
#define Q1_M0_F3_ACT 	 68
#define Q1_M0_FACE1 	 69
#define Q1_M0_FACE2 	 70
#define Q1_M0_FACE3 	 71
#define Q1_M0_LEFT 	 72
#define Q1_M0_L_ACT 	 73
#define Q1_M0_RIGHT 	 74
#define Q1_M0_R_ACT 	 75
#define Q1_M0_SIDE 	 76
#define Q1_M0_S_ACT 	 77
#define Q1_R0_DOF1 	 78
#define Q1_R0_DOF2 	 79
#define Q1_R0_DOF3 	 80
#define Q1_R0_DOF4 	 81
#define Q1_R0_DOF5 	 82
#define Q1_R0_DOF6 	 83
#define Q1_R0_F1_ACT 	 84
#define Q1_R0_F2_ACT 	 85
#define Q1_R0_F3_ACT 	 86
#define Q1_R0_FACE1 	 87
#define Q1_R0_FACE2 	 88
#define Q1_R0_FACE3 	 89
#define Q1_R0_LEFT 	 90
#define Q1_R0_L_ACT 	 91
#define Q1_R0_RIGHT 	 92
#define Q1_R0_R_ACT 	 93
#define Q1_R0_SIDE 	 94
#define Q1_R0_S_ACT 	 95


#define MAX_MODULES 	 96
#define MAX_FILTERS 	 960

typedef struct CDS_EPICS_IN {
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

typedef struct SUS {
	float Q1_ESD_INMTRX[3][5];
	float Q1_L1_INMTRX[4][3];
	float Q1_L2_INMTRX[4][3];
	float Q1_M0_INMTRX[6][6];
	float Q1_M0_OUTMTRX[6][6];
	float Q1_R0_INMTRX[6][6];
	float Q1_R0_OUTMTRX[6][6];
} SUS;

typedef struct CDS_EPICS {
	CDS_EPICS_IN epicsInput;
	CDS_EPICS_OUT epicsOutput;
	SUS sus;
} CDS_EPICS;
#endif
