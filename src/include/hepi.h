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
#define BSC_GEO_H1E 	 27
#define BSC_GEO_H2 	 28
#define BSC_GEO_H2E 	 29
#define BSC_GEO_H3 	 30
#define BSC_GEO_H3E 	 31
#define BSC_GEO_H4 	 32
#define BSC_GEO_H4E 	 33
#define BSC_GEO_HP 	 34
#define BSC_GEO_RX 	 35
#define BSC_GEO_RY 	 36
#define BSC_GEO_RZ 	 37
#define BSC_GEO_V1 	 38
#define BSC_GEO_V1E 	 39
#define BSC_GEO_V2 	 40
#define BSC_GEO_V2E 	 41
#define BSC_GEO_V3 	 42
#define BSC_GEO_V3E 	 43
#define BSC_GEO_V4 	 44
#define BSC_GEO_V4E 	 45
#define BSC_GEO_VP 	 46
#define BSC_GEO_X 	 47
#define BSC_GEO_Y 	 48
#define BSC_GEO_Z 	 49
#define BSC_POS_H1 	 50
#define BSC_POS_H1E 	 51
#define BSC_POS_H2 	 52
#define BSC_POS_H2E 	 53
#define BSC_POS_H3 	 54
#define BSC_POS_H3E 	 55
#define BSC_POS_H4 	 56
#define BSC_POS_H4E 	 57
#define BSC_POS_HP 	 58
#define BSC_POS_RX 	 59
#define BSC_POS_RY 	 60
#define BSC_POS_RZ 	 61
#define BSC_POS_V1 	 62
#define BSC_POS_V1E 	 63
#define BSC_POS_V2 	 64
#define BSC_POS_V2E 	 65
#define BSC_POS_V3 	 66
#define BSC_POS_V3E 	 67
#define BSC_POS_V4 	 68
#define BSC_POS_V4E 	 69
#define BSC_POS_VP 	 70
#define BSC_POS_X 	 71
#define BSC_POS_Y 	 72
#define BSC_POS_Z 	 73
#define BSC_SPARE_FP1 	 74
#define BSC_SPARE_FP2 	 75
#define BSC_SPARE_RP1 	 76
#define BSC_SPARE_RP2 	 77
#define BSC_SPARE_RP3 	 78
#define BSC_STS_X 	 79
#define BSC_STS_XE 	 80
#define BSC_STS_X_RX 	 81
#define BSC_STS_X_RY 	 82
#define BSC_STS_Y 	 83
#define BSC_STS_YE 	 84
#define BSC_STS_Z 	 85
#define BSC_STS_ZE 	 86
#define STS1_FIR_X 	 87
#define STS1_FIR_X_DF 	 88
#define STS1_FIR_X_UF 	 89
#define STS1_FIR_X_CF 	 90
#define STS1_FIR_X_FIR 	 0
#define STS1_FIR_Y 	 91
#define STS1_FIR_Y_DF 	 92
#define STS1_FIR_Y_UF 	 93
#define STS1_FIR_Y_CF 	 94
#define STS1_FIR_Y_FIR 	 1
#define STS2_FIR_X 	 95
#define STS2_FIR_X_DF 	 96
#define STS2_FIR_X_UF 	 97
#define STS2_FIR_X_CF 	 98
#define STS2_FIR_X_FIR 	 2
#define STS2_FIR_Y 	 99
#define STS2_FIR_Y_DF 	 100
#define STS2_FIR_Y_UF 	 101
#define STS2_FIR_Y_CF 	 102
#define STS2_FIR_Y_FIR 	 3


#define MAX_MODULES 	 103
#define MAX_FILTERS 	 1030

#define MAX_FIR 	 4
#define MAX_FIR_POLY 	 4

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

typedef struct SEI {
	float BSC_LOOP_GAIN;
	int BSC_LOOP_GAIN_TRAMP;
	int BSC_LOOP_GAIN_RMON;
	int BSC_LOOP_SW;
	float BSC_MTRX[8][8];
	float BSC_TILTCORR_GAIN;
	int BSC_TILTCORR_GAIN_TRAMP;
	int BSC_TILTCORR_GAIN_RMON;
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
	float BSC_POS_MTRX[8][8];
	int BSC_POS_SW;
	float BSC_STS_IN_MTRX[3][6];
	float BSC_STS_MTRX[8][3];
	int BSC_STS_RAMP_SW;
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
