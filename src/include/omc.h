#ifndef OMC_H_INCLUDED
#define OMC_H_INCLUDED
#define ASC_P1_DC 	 0
#define ASC_P1_I 	 1
#define ASC_P1_Q 	 2
#define ASC_P2_DC 	 3
#define ASC_P2_I 	 4
#define ASC_P2_Q 	 5
#define ASC_PZT_1A 	 6
#define ASC_PZT_1B 	 7
#define ASC_PZT_2A 	 8
#define ASC_PZT_2B 	 9
#define ASC_QPD1_P 	 10
#define ASC_QPD1_S1 	 11
#define ASC_QPD1_S2 	 12
#define ASC_QPD1_S3 	 13
#define ASC_QPD1_S4 	 14
#define ASC_QPD1_Y 	 15
#define ASC_QPD2_P 	 16
#define ASC_QPD2_S1 	 17
#define ASC_QPD2_S2 	 18
#define ASC_QPD2_S3 	 19
#define ASC_QPD2_S4 	 20
#define ASC_QPD2_Y 	 21
#define ASC_Y1_DC 	 22
#define ASC_Y1_I 	 23
#define ASC_Y1_Q 	 24
#define ASC_Y2_DC 	 25
#define ASC_Y2_I 	 26
#define ASC_Y2_Q 	 27
#define LSC_DC 	 28
#define LSC_I 	 29
#define LSC_Q 	 30
#define LSC_DRIVE 	 31
#define LSC_GAIN 	 32
#define LSC_OUTPUT 	 33
#define LSC_TRANS1_DC 	 34
#define LSC_TRANS1_DCO 	 35
#define LSC_TRANS2_DC 	 36
#define LSC_TRANS2_DCO 	 37


#define MAX_MODULES 	 38
#define MAX_FILTERS 	 380

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

typedef struct OMC {
	float ASC_INMTRX1[4][2];
	float ASC_OUTMTRX[4][8];
	float ASC_P1_OSC1_FREQ;
	float ASC_P1_OSC1_CLKGAIN;
	float ASC_P1_OSC1_SINGAIN;
	float ASC_P1_OSC1_COSGAIN;
	float ASC_P1_MOUT;
	float ASC_P2_OSC1_FREQ;
	float ASC_P2_OSC1_CLKGAIN;
	float ASC_P2_OSC1_SINGAIN;
	float ASC_P2_OSC1_COSGAIN;
	float ASC_P2_MOUT;
	float ASC_PZT_1X_DVMA;
	float ASC_PZT_1X_DVMB;
	float ASC_PZT_1X_SG;
	float ASC_PZT_1Y_DVMA;
	float ASC_PZT_1Y_DVMB;
	float ASC_PZT_1Y_SG;
	float ASC_PZT_2X_DVMA;
	float ASC_PZT_2X_DVMB;
	float ASC_PZT_2X_SG;
	float ASC_PZT_2Y_DVMA;
	float ASC_PZT_2Y_DVMB;
	float ASC_PZT_2Y_SG;
	float ASC_QPD1_INMTRX[3][4];
	float ASC_QPD1_PIT;
	float ASC_QPD1_SUM;
	float ASC_QPD1_YAW;
	float ASC_QPD2_INMTRX[3][4];
	float ASC_QPD2_PIT;
	float ASC_QPD2_SUM;
	float ASC_QPD2_YAW;
	float ASC_Y1_OSC1_FREQ;
	float ASC_Y1_OSC1_CLKGAIN;
	float ASC_Y1_OSC1_SINGAIN;
	float ASC_Y1_OSC1_COSGAIN;
	float ASC_Y1_MOUT;
	float ASC_Y2_OSC1_FREQ;
	float ASC_Y2_OSC1_CLKGAIN;
	float ASC_Y2_OSC1_SINGAIN;
	float ASC_Y2_OSC1_COSGAIN;
	float ASC_Y2_MOUT;
	float LSC_OSC1_FREQ;
	float LSC_OSC1_CLKGAIN;
	float LSC_OSC1_SINGAIN;
	float LSC_OSC1_COSGAIN;
	float LSC_FEED;
	float LSC_INMTRX[1][2];
	float LSC_I_MON;
	float LSC_OUTMTRX[1][2];
	float LSC_PD1_UF;
	float LSC_PD2_UF;
	float LSC_PHASE[2];
	float LSC_PZT_DVM_AC;
	float LSC_PZT_DVM_DC;
	float LSC_Q_MON;
} OMC;

typedef struct CDS_EPICS {
	CDS_EPICS_IN epicsInput;
	CDS_EPICS_OUT epicsOutput;
	OMC omc;
} CDS_EPICS;
#endif
