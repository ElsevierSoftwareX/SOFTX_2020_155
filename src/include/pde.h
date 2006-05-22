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
#define ASC_WFS2_PIT 	 10
#define ASC_WFS2_YAW 	 11
#define LSC_AS1DC 	 12
#define LSC_AS1I 	 13
#define LSC_AS1Q 	 14
#define LSC_AS2DC 	 15
#define LSC_AS3DC 	 16
#define LSC_CARM 	 17
#define LSC_DARM 	 18
#define LSC_LOCK_DC 	 19
#define LSC_PRC 	 20
#define LSC_REFLI 	 21
#define LSC_REFLQ 	 22
#define LSC_X_TRANS 	 23
#define LSC_Y_TRANS 	 24
#define SUS_BS_ASCP 	 25
#define SUS_BS_ASCY 	 26
#define SUS_BS_LLOUT 	 27
#define SUS_BS_LLPIT 	 28
#define SUS_BS_LLPOS 	 29
#define SUS_BS_LLSEN 	 30
#define SUS_BS_LLYAW 	 31
#define SUS_BS_LROUT 	 32
#define SUS_BS_LRPIT 	 33
#define SUS_BS_LRPOS 	 34
#define SUS_BS_LRSEN 	 35
#define SUS_BS_LRYAW 	 36
#define SUS_BS_LSC 	 37
#define SUS_BS_PIT 	 38
#define SUS_BS_POS 	 39
#define SUS_BS_SIDE 	 40
#define SUS_BS_ULOUT 	 41
#define SUS_BS_ULPIT 	 42
#define SUS_BS_ULPOS 	 43
#define SUS_BS_ULSEN 	 44
#define SUS_BS_ULYAW 	 45
#define SUS_BS_UROUT 	 46
#define SUS_BS_URPIT 	 47
#define SUS_BS_URPOS 	 48
#define SUS_BS_URSEN 	 49
#define SUS_BS_URYAW 	 50
#define SUS_BS_YAW 	 51
#define SUS_ETMX_ASCP 	 52
#define SUS_ETMX_ASCY 	 53
#define SUS_ETMX_LLOUT 	 54
#define SUS_ETMX_LLPIT 	 55
#define SUS_ETMX_LLPOS 	 56
#define SUS_ETMX_LLSEN 	 57
#define SUS_ETMX_LLYAW 	 58
#define SUS_ETMX_LROUT 	 59
#define SUS_ETMX_LRPIT 	 60
#define SUS_ETMX_LRPOS 	 61
#define SUS_ETMX_LRSEN 	 62
#define SUS_ETMX_LRYAW 	 63
#define SUS_ETMX_LSC 	 64
#define SUS_ETMX_PIT 	 65
#define SUS_ETMX_POS 	 66
#define SUS_ETMX_SIDE 	 67
#define SUS_ETMX_ULOUT 	 68
#define SUS_ETMX_ULPIT 	 69
#define SUS_ETMX_ULPOS 	 70
#define SUS_ETMX_ULSEN 	 71
#define SUS_ETMX_ULYAW 	 72
#define SUS_ETMX_UROUT 	 73
#define SUS_ETMX_URPIT 	 74
#define SUS_ETMX_URPOS 	 75
#define SUS_ETMX_URSEN 	 76
#define SUS_ETMX_URYAW 	 77
#define SUS_ETMX_YAW 	 78
#define SUS_ETMY_ASCP 	 79
#define SUS_ETMY_ASCY 	 80
#define SUS_ETMY_LLOUT 	 81
#define SUS_ETMY_LLPIT 	 82
#define SUS_ETMY_LLPOS 	 83
#define SUS_ETMY_LLSEN 	 84
#define SUS_ETMY_LLYAW 	 85
#define SUS_ETMY_LROUT 	 86
#define SUS_ETMY_LRPIT 	 87
#define SUS_ETMY_LRPOS 	 88
#define SUS_ETMY_LRSEN 	 89
#define SUS_ETMY_LRYAW 	 90
#define SUS_ETMY_LSC 	 91
#define SUS_ETMY_PIT 	 92
#define SUS_ETMY_POS 	 93
#define SUS_ETMY_SIDE 	 94
#define SUS_ETMY_ULOUT 	 95
#define SUS_ETMY_ULPIT 	 96
#define SUS_ETMY_ULPOS 	 97
#define SUS_ETMY_ULSEN 	 98
#define SUS_ETMY_ULYAW 	 99
#define SUS_ETMY_UROUT 	 100
#define SUS_ETMY_URPIT 	 101
#define SUS_ETMY_URPOS 	 102
#define SUS_ETMY_URSEN 	 103
#define SUS_ETMY_URYAW 	 104
#define SUS_ETMY_YAW 	 105
#define SUS_ITMX_ASCP 	 106
#define SUS_ITMX_ASCY 	 107
#define SUS_ITMX_LLOUT 	 108
#define SUS_ITMX_LLPIT 	 109
#define SUS_ITMX_LLPOS 	 110
#define SUS_ITMX_LLSEN 	 111
#define SUS_ITMX_LLYAW 	 112
#define SUS_ITMX_LROUT 	 113
#define SUS_ITMX_LRPIT 	 114
#define SUS_ITMX_LRPOS 	 115
#define SUS_ITMX_LRSEN 	 116
#define SUS_ITMX_LRYAW 	 117
#define SUS_ITMX_LSC 	 118
#define SUS_ITMX_PIT 	 119
#define SUS_ITMX_POS 	 120
#define SUS_ITMX_SIDE 	 121
#define SUS_ITMX_ULOUT 	 122
#define SUS_ITMX_ULPIT 	 123
#define SUS_ITMX_ULPOS 	 124
#define SUS_ITMX_ULSEN 	 125
#define SUS_ITMX_ULYAW 	 126
#define SUS_ITMX_UROUT 	 127
#define SUS_ITMX_URPIT 	 128
#define SUS_ITMX_URPOS 	 129
#define SUS_ITMX_URSEN 	 130
#define SUS_ITMX_URYAW 	 131
#define SUS_ITMX_YAW 	 132
#define SUS_ITMY_ASCP 	 133
#define SUS_ITMY_ASCY 	 134
#define SUS_ITMY_LLOUT 	 135
#define SUS_ITMY_LLPIT 	 136
#define SUS_ITMY_LLPOS 	 137
#define SUS_ITMY_LLSEN 	 138
#define SUS_ITMY_LLYAW 	 139
#define SUS_ITMY_LROUT 	 140
#define SUS_ITMY_LRPIT 	 141
#define SUS_ITMY_LRPOS 	 142
#define SUS_ITMY_LRSEN 	 143
#define SUS_ITMY_LRYAW 	 144
#define SUS_ITMY_LSC 	 145
#define SUS_ITMY_PIT 	 146
#define SUS_ITMY_POS 	 147
#define SUS_ITMY_SIDE 	 148
#define SUS_ITMY_ULOUT 	 149
#define SUS_ITMY_ULPIT 	 150
#define SUS_ITMY_ULPOS 	 151
#define SUS_ITMY_ULSEN 	 152
#define SUS_ITMY_ULYAW 	 153
#define SUS_ITMY_UROUT 	 154
#define SUS_ITMY_URPIT 	 155
#define SUS_ITMY_URPOS 	 156
#define SUS_ITMY_URSEN 	 157
#define SUS_ITMY_URYAW 	 158
#define SUS_ITMY_YAW 	 159


#define MAX_MODULES 	 160
#define MAX_FILTERS 	 1600

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
	float ASC_INMTRX[4][4];
	float ASC_I_MTRX[3][4];
	float ASC_I_MTRX1[3][4];
	float ASC_I_PIT;
	float ASC_I_SUM;
	float ASC_I_YAW;
	float ASC_OUTMTRX[4][4];
	float ASC_Q_PIT;
	float ASC_Q_SUM;
	float ASC_Q_YAW;
	float LSC_INMTRX[3][4];
	float LSC_OUTMTRX[6][3];
	float PZT_M1_PIT;
	float PZT_M1_YAW;
	float PZT_M2_PIT;
	float PZT_M2_YAW;
	float SUS_BS_INMTRX[3][4];
	float SUS_ETMX_INMTRX[3][4];
	float SUS_ETMY_INMTRX[3][4];
	float SUS_ITMX_INMTRX[3][4];
	float SUS_ITMY_INMTRX[3][4];
} PDE;

typedef struct CDS_EPICS {
	CDS_EPICS_IN epicsInput;
	CDS_EPICS_OUT epicsOutput;
	PDE pde;
} CDS_EPICS;
#endif
