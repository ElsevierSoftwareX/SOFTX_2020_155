#ifndef SEI_H_INCLUDED
#define SEI_H_INCLUDED
#define BSC_HP 	 0
#define BSC_RX 	 1
#define BSC_RY 	 2
#define BSC_RZ 	 3
#define BSC_TILTCORR_H1 	 4
#define BSC_TILTCORR_H2 	 5
#define BSC_TILTCORR_H3 	 6
#define BSC_TILTCORR_H4 	 7
#define BSC_TILTCORR_V1 	 8
#define BSC_TILTCORR_V2 	 9
#define BSC_TILTCORR_V3 	 10
#define BSC_TILTCORR_V4 	 11
#define BSC_VP 	 12
#define BSC_X 	 13
#define BSC_Y 	 14
#define BSC_Z 	 15
#define BSC_ACT_H1 	 16
#define BSC_ACT_H2 	 17
#define BSC_ACT_H3 	 18
#define BSC_ACT_H4 	 19
#define BSC_ACT_V1 	 20
#define BSC_ACT_V2 	 21
#define BSC_ACT_V3 	 22
#define BSC_ACT_V4 	 23
#define BSC_FIR_X_OUT 	 24
#define BSC_FIR_Y_OUT 	 25
#define BSC_GEO_H1 	 26
#define BSC_GEO_H2 	 27
#define BSC_GEO_H3 	 28
#define BSC_GEO_H4 	 29
#define BSC_GEO_HP 	 30
#define BSC_GEO_RX 	 31
#define BSC_GEO_RY 	 32
#define BSC_GEO_RZ 	 33
#define BSC_GEO_V1 	 34
#define BSC_GEO_V2 	 35
#define BSC_GEO_V3 	 36
#define BSC_GEO_V4 	 37
#define BSC_GEO_VP 	 38
#define BSC_GEO_X 	 39
#define BSC_GEO_X_RMS 	 40
#define BSC_GEO_X_RMS2 	 41
#define BSC_GEO_Y 	 42
#define BSC_GEO_Y_RMS 	 43
#define BSC_GEO_Y_RMS2 	 44
#define BSC_GEO_Z 	 45
#define BSC_GEO_Z_RMS 	 46
#define BSC_GEO_Z_RMS2 	 47
#define BSC_POS_H1 	 48
#define BSC_POS_H2 	 49
#define BSC_POS_H3 	 50
#define BSC_POS_H4 	 51
#define BSC_POS_HP 	 52
#define BSC_POS_RX 	 53
#define BSC_POS_RY 	 54
#define BSC_POS_RZ 	 55
#define BSC_POS_V1 	 56
#define BSC_POS_V2 	 57
#define BSC_POS_V3 	 58
#define BSC_POS_V4 	 59
#define BSC_POS_VP 	 60
#define BSC_POS_X 	 61
#define BSC_POS_Y 	 62
#define BSC_POS_Z 	 63
#define BSC_SPARE_FP1 	 64
#define BSC_SPARE_FP2 	 65
#define BSC_SPARE_RP1 	 66
#define BSC_SPARE_RP2 	 67
#define BSC_SPARE_RP3 	 68
#define BSC_STS_X 	 69
#define BSC_STS_X_RY 	 70
#define BSC_STS_Y 	 71
#define BSC_STS_Y_RX 	 72
#define BSC_STS_Z 	 73
#define BSC_WD_GEO_H1 	 74
#define BSC_WD_GEO_H2 	 75
#define BSC_WD_GEO_H3 	 76
#define BSC_WD_GEO_H4 	 77
#define BSC_WD_GEO_V1 	 78
#define BSC_WD_GEO_V2 	 79
#define BSC_WD_GEO_V3 	 80
#define BSC_WD_GEO_V4 	 81
#define BSC_WD_POS_H1 	 82
#define BSC_WD_POS_H2 	 83
#define BSC_WD_POS_H3 	 84
#define BSC_WD_POS_H4 	 85
#define BSC_WD_POS_V1 	 86
#define BSC_WD_POS_V2 	 87
#define BSC_WD_POS_V3 	 88
#define BSC_WD_POS_V4 	 89
#define BSC_WD_STSX 	 90
#define BSC_WD_STSY 	 91
#define BSC_WD_STSZ 	 92
#define STS1_FIR_X_CF 	 93
#define STS1_FIR_X_DF 	 94
#define STS1_FIR_X_PP 	 95
#define STS1_FIR_X_UF 	 96
#define STS1_FIR_Y_CF 	 97
#define STS1_FIR_Y_DF 	 98
#define STS1_FIR_Y_PP 	 99
#define STS1_FIR_Y_UF 	 100
#define STS2_FIR_X_CF 	 101
#define STS2_FIR_X_DF 	 102
#define STS2_FIR_X_PP 	 103
#define STS2_FIR_X_UF 	 104
#define STS2_FIR_Y_CF 	 105
#define STS2_FIR_Y_DF 	 106
#define STS2_FIR_Y_PP 	 107
#define STS2_FIR_Y_UF 	 108


#define MAX_MODULES 	 109
#define MAX_FILTERS 	 1090

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

typedef struct SEI_WATCHDOG {
	int trip;
	int reset;
	int status[3];
	int senCount[20];
	int senCountHold[20];
	int filtMax[20];
	int filtMaxHold[20];
	int tripSetF[5];
	int tripSetR[5];
	int tripState;
} SEI_WATCHDOG;

typedef struct SEI {
	float BSC_LOOP_GAIN;
	int BSC_LOOP_GAIN_TRAMP;
	int BSC_LOOP_GAIN_RMON;
	int BSC_LOOP_SW;
	float BSC_MTRX[8][8];
	float BSC_TILTCORR_GAIN;
	int BSC_TILTCORR_GAIN_TRAMP;
	int BSC_TILTCORR_GAIN_RMON;
	float BSC_TILTCORR_MAT_H1;
	float BSC_TILTCORR_MAT_H2;
	float BSC_TILTCORR_MAT_H3;
	float BSC_TILTCORR_MAT_H4;
	float BSC_TILTCORR_MAT_V1;
	float BSC_TILTCORR_MAT_V2;
	float BSC_TILTCORR_MAT_V3;
	float BSC_TILTCORR_MAT_V4;
	float BSC_TILTCORR_MTRX[8][8];
	int BSC_TILTCORR_SW;
	float BSC_ACT_GAIN;
	int BSC_ACT_GAIN_TRAMP;
	int BSC_ACT_GAIN_RMON;
	int BSC_ACT_SW;
	float BSC_DAC_OUTPUT_H1;
	float BSC_DAC_OUTPUT_H2;
	float BSC_DAC_OUTPUT_H3;
	float BSC_DAC_OUTPUT_H4;
	float BSC_DAC_OUTPUT_V1;
	float BSC_DAC_OUTPUT_V2;
	float BSC_DAC_OUTPUT_V3;
	float BSC_DAC_OUTPUT_V4;
	float BSC_DC_BIAS_1;
	float BSC_DC_BIAS_2;
	float BSC_DC_BIAS_3;
	float BSC_DC_BIAS_4;
	float BSC_DC_BIAS_5;
	float BSC_DC_BIAS_6;
	float BSC_DC_BIAS_MTRX[8][6];
	int BSC_DC_BIAS_SW;
	float BSC_FIR_MTRX[2][4];
	float BSC_GEO_MTRX[8][8];
	float BSC_GEO_SH1;
	float BSC_GEO_SH2;
	float BSC_GEO_SH3;
	float BSC_GEO_SH4;
	float BSC_GEO_SV1;
	float BSC_GEO_SV2;
	float BSC_GEO_SV3;
	float BSC_GEO_SV4;
	int BSC_GEO_SW;
	float BSC_GEO_X_RMS_1;
	float BSC_GEO_X_RMS_2;
	float BSC_GEO_Y_RMS_1;
	float BSC_GEO_Y_RMS_2;
	float BSC_GEO_Z_RMS_1;
	float BSC_GEO_Z_RMS_2;
	float BSC_POS_MTRX[8][8];
	int BSC_POS_SW;
	float BSC_STS_IN_MTRX[3][6];
	float BSC_STS_MTRX[8][3];
	int BSC_STS_RAMP_SW;
	SEI_WATCHDOG BSC_WD_M;
	float BSC_WIT_1_SENSOR;
	float BSC_WIT_2_SENSOR;
	float BSC_WIT_3_SENSOR;
	float BSC_WIT_4_SENSOR;
	float STS1_X;
	float STS1_Y;
	float STS1_Z;
	float STS2_X;
	float STS2_Y;
	float STS2_Z;
} SEI;

typedef struct CDS_EPICS {
	CDS_EPICS_IN epicsInput;
	CDS_EPICS_OUT epicsOutput;
	SEI sei;
} CDS_EPICS;
#endif
