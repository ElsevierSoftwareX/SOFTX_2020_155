#ifndef PDE_H_INCLUDED
#define PDE_H_INCLUDED
#define ASC_WFS1_I1 	 0
#define ASC_WFS1_I2 	 1
#define ASC_WFS1_I3 	 2
#define ASC_WFS1_I4 	 3
#define ASC_WFS1_PIT 	 4
#define ASC_WFS1_Q1 	 5
#define ASC_WFS1_Q2 	 6
#define ASC_WFS1_Q3 	 7
#define ASC_WFS1_Q4 	 8
#define ASC_WFS1_YAW 	 9
#define LSC_AS1DC 	 10
#define LSC_AS1I 	 11
#define LSC_AS1Q 	 12
#define LSC_AS2DC 	 13
#define LSC_AS3DC 	 14
#define LSC_CARM 	 15
#define LSC_DARM 	 16
#define LSC_LOCK_DC 	 17
#define LSC_PRC 	 18
#define LSC_REFLI 	 19
#define LSC_REFLQ 	 20
#define LSC_X_TRANS 	 21
#define LSC_Y_TRANS 	 22
#define SUS_BS_ASCP 	 23
#define SUS_BS_ASCY 	 24
#define SUS_BS_LLOUT 	 25
#define SUS_BS_LLPIT 	 26
#define SUS_BS_LLPOS 	 27
#define SUS_BS_LLSEN 	 28
#define SUS_BS_LLYAW 	 29
#define SUS_BS_LROUT 	 30
#define SUS_BS_LRPIT 	 31
#define SUS_BS_LRPOS 	 32
#define SUS_BS_LRSEN 	 33
#define SUS_BS_LRYAW 	 34
#define SUS_BS_LSC 	 35
#define SUS_BS_PIT 	 36
#define SUS_BS_POS 	 37
#define SUS_BS_SIDE 	 38
#define SUS_BS_ULOUT 	 39
#define SUS_BS_ULPIT 	 40
#define SUS_BS_ULPOS 	 41
#define SUS_BS_ULSEN 	 42
#define SUS_BS_ULYAW 	 43
#define SUS_BS_UROUT 	 44
#define SUS_BS_URPIT 	 45
#define SUS_BS_URPOS 	 46
#define SUS_BS_URSEN 	 47
#define SUS_BS_URYAW 	 48
#define SUS_BS_YAW 	 49
#define SUS_ETMX_ASCP 	 50
#define SUS_ETMX_ASCY 	 51
#define SUS_ETMX_LLOUT 	 52
#define SUS_ETMX_LLPIT 	 53
#define SUS_ETMX_LLPOS 	 54
#define SUS_ETMX_LLSEN 	 55
#define SUS_ETMX_LLYAW 	 56
#define SUS_ETMX_LROUT 	 57
#define SUS_ETMX_LRPIT 	 58
#define SUS_ETMX_LRPOS 	 59
#define SUS_ETMX_LRSEN 	 60
#define SUS_ETMX_LRYAW 	 61
#define SUS_ETMX_LSC 	 62
#define SUS_ETMX_PIT 	 63
#define SUS_ETMX_POS 	 64
#define SUS_ETMX_SIDE 	 65
#define SUS_ETMX_ULOUT 	 66
#define SUS_ETMX_ULPIT 	 67
#define SUS_ETMX_ULPOS 	 68
#define SUS_ETMX_ULSEN 	 69
#define SUS_ETMX_ULYAW 	 70
#define SUS_ETMX_UROUT 	 71
#define SUS_ETMX_URPIT 	 72
#define SUS_ETMX_URPOS 	 73
#define SUS_ETMX_URSEN 	 74
#define SUS_ETMX_URYAW 	 75
#define SUS_ETMX_YAW 	 76
#define SUS_ETMY_ASCP 	 77
#define SUS_ETMY_ASCY 	 78
#define SUS_ETMY_LLOUT 	 79
#define SUS_ETMY_LLPIT 	 80
#define SUS_ETMY_LLPOS 	 81
#define SUS_ETMY_LLSEN 	 82
#define SUS_ETMY_LLYAW 	 83
#define SUS_ETMY_LROUT 	 84
#define SUS_ETMY_LRPIT 	 85
#define SUS_ETMY_LRPOS 	 86
#define SUS_ETMY_LRSEN 	 87
#define SUS_ETMY_LRYAW 	 88
#define SUS_ETMY_LSC 	 89
#define SUS_ETMY_PIT 	 90
#define SUS_ETMY_POS 	 91
#define SUS_ETMY_SIDE 	 92
#define SUS_ETMY_ULOUT 	 93
#define SUS_ETMY_ULPIT 	 94
#define SUS_ETMY_ULPOS 	 95
#define SUS_ETMY_ULSEN 	 96
#define SUS_ETMY_ULYAW 	 97
#define SUS_ETMY_UROUT 	 98
#define SUS_ETMY_URPIT 	 99
#define SUS_ETMY_URPOS 	 100
#define SUS_ETMY_URSEN 	 101
#define SUS_ETMY_URYAW 	 102
#define SUS_ETMY_YAW 	 103
#define SUS_ITMX_ASCP 	 104
#define SUS_ITMX_ASCY 	 105
#define SUS_ITMX_LLOUT 	 106
#define SUS_ITMX_LLPIT 	 107
#define SUS_ITMX_LLPOS 	 108
#define SUS_ITMX_LLSEN 	 109
#define SUS_ITMX_LLYAW 	 110
#define SUS_ITMX_LROUT 	 111
#define SUS_ITMX_LRPIT 	 112
#define SUS_ITMX_LRPOS 	 113
#define SUS_ITMX_LRSEN 	 114
#define SUS_ITMX_LRYAW 	 115
#define SUS_ITMX_LSC 	 116
#define SUS_ITMX_PIT 	 117
#define SUS_ITMX_POS 	 118
#define SUS_ITMX_SIDE 	 119
#define SUS_ITMX_ULOUT 	 120
#define SUS_ITMX_ULPIT 	 121
#define SUS_ITMX_ULPOS 	 122
#define SUS_ITMX_ULSEN 	 123
#define SUS_ITMX_ULYAW 	 124
#define SUS_ITMX_UROUT 	 125
#define SUS_ITMX_URPIT 	 126
#define SUS_ITMX_URPOS 	 127
#define SUS_ITMX_URSEN 	 128
#define SUS_ITMX_URYAW 	 129
#define SUS_ITMX_YAW 	 130
#define SUS_ITMY_ASCP 	 131
#define SUS_ITMY_ASCY 	 132
#define SUS_ITMY_LLOUT 	 133
#define SUS_ITMY_LLPIT 	 134
#define SUS_ITMY_LLPOS 	 135
#define SUS_ITMY_LLSEN 	 136
#define SUS_ITMY_LLYAW 	 137
#define SUS_ITMY_LROUT 	 138
#define SUS_ITMY_LRPIT 	 139
#define SUS_ITMY_LRPOS 	 140
#define SUS_ITMY_LRSEN 	 141
#define SUS_ITMY_LRYAW 	 142
#define SUS_ITMY_LSC 	 143
#define SUS_ITMY_PIT 	 144
#define SUS_ITMY_POS 	 145
#define SUS_ITMY_SIDE 	 146
#define SUS_ITMY_ULOUT 	 147
#define SUS_ITMY_ULPIT 	 148
#define SUS_ITMY_ULPOS 	 149
#define SUS_ITMY_ULSEN 	 150
#define SUS_ITMY_ULYAW 	 151
#define SUS_ITMY_UROUT 	 152
#define SUS_ITMY_URPIT 	 153
#define SUS_ITMY_URPOS 	 154
#define SUS_ITMY_URSEN 	 155
#define SUS_ITMY_URYAW 	 156
#define SUS_ITMY_YAW 	 157


#define MAX_MODULES 	 158
#define MAX_FILTERS 	 1580

#define MAX_FIR 	 0
#define MAX_FIR_POLY 	 0

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

typedef struct PDE {
	float ASC_INMTRX[2][4];
	float ASC_OUTMTRX[4][2];
	float ASC_WFS1_I1_MON;
	float ASC_WFS1_I2_MON;
	float ASC_WFS1_I3_MON;
	float ASC_WFS1_I4_MON;
	float ASC_WFS1_I_MTRX[3][4];
	float ASC_WFS1_I_PIT;
	float ASC_WFS1_I_SUM;
	float ASC_WFS1_I_YAW;
	float ASC_WFS1_PHASE1[2][2];
	float ASC_WFS1_PHASE2[2][2];
	float ASC_WFS1_PHASE3[2][2];
	float ASC_WFS1_PHASE4[2][2];
	float ASC_WFS1_Q1_MON;
	float ASC_WFS1_Q2_MON;
	float ASC_WFS1_Q3_MON;
	float ASC_WFS1_Q4_MON;
	float ASC_WFS1_Q_MTRX[3][4];
	float ASC_WFS1_Q_PIT;
	float ASC_WFS1_Q_SUM;
	float ASC_WFS1_Q_YAW;
	float LSC_INMTRX[3][4];
	float LSC_OUTMTRX[6][3];
	float PZT_M1_PIT;
	float PZT_M1_YAW;
	float PZT_M2_PIT;
	float PZT_M2_YAW;
	int SEI_HMY_ACT_SW;
	float SUS_BS_INMTRX[3][4];
	float SUS_BS_LL_DRV;
	float SUS_BS_LR_DRV;
	int SUS_BS_MASTER_SW;
	float SUS_BS_SD_DRV;
	float SUS_BS_UL_DRV;
	float SUS_BS_UR_DRV;
	int SUS_BS_WD;
	int SUS_BS_WD_STAT;
	int SUS_BS_WD_MAX;
	float SUS_BS_WD_VAR[5];
	float SUS_ETMX_INMTRX[3][4];
	float SUS_ETMX_LL_DRV;
	float SUS_ETMX_LR_DRV;
	int SUS_ETMX_MASTER_SW;
	float SUS_ETMX_SD_DRV;
	float SUS_ETMX_UL_DRV;
	float SUS_ETMX_UR_DRV;
	int SUS_ETMX_WD;
	int SUS_ETMX_WD_STAT;
	int SUS_ETMX_WD_MAX;
	float SUS_ETMX_WD_VAR[5];
	float SUS_ETMY_INMTRX[3][4];
	float SUS_ETMY_LL_DRV;
	float SUS_ETMY_LR_DRV;
	int SUS_ETMY_MASTER_SW;
	float SUS_ETMY_SD_DRV;
	float SUS_ETMY_UL_DRV;
	float SUS_ETMY_UR_DRV;
	int SUS_ETMY_WD;
	int SUS_ETMY_WD_STAT;
	int SUS_ETMY_WD_MAX;
	float SUS_ETMY_WD_VAR[5];
	float SUS_ITMX_INMTRX[3][4];
	float SUS_ITMX_LL_DRV;
	float SUS_ITMX_LR_DRV;
	int SUS_ITMX_MASTER_SW;
	float SUS_ITMX_SD_DRV;
	float SUS_ITMX_UL_DRV;
	float SUS_ITMX_UR_DRV;
	int SUS_ITMX_WD;
	int SUS_ITMX_WD_STAT;
	int SUS_ITMX_WD_MAX;
	float SUS_ITMX_WD_VAR[5];
	float SUS_ITMY_INMTRX[3][4];
	float SUS_ITMY_LL_DRV;
	float SUS_ITMY_LR_DRV;
	int SUS_ITMY_MASTER_SW;
	float SUS_ITMY_SD_DRV;
	float SUS_ITMY_UL_DRV;
	float SUS_ITMY_UR_DRV;
	int SUS_ITMY_WD;
	int SUS_ITMY_WD_STAT;
	int SUS_ITMY_WD_MAX;
	float SUS_ITMY_WD_VAR[5];
} PDE;

typedef struct CDS_EPICS {
	CDS_EPICS_IN epicsInput;
	CDS_EPICS_OUT epicsOutput;
	PDE pde;
} CDS_EPICS;
#endif
