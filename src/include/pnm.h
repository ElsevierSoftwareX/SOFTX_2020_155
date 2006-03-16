#ifndef PNM_SUS_H_INCLUDED
#define PNM_SUS_H_INCLUDED

#define NUM_SYSTEMS 7

/* Assign filter names to filter number */
#define ULSEN	0
#define URSEN	1
#define LRSEN	2
#define LLSEN	3
#define SDSEN	4

#define SUSPOS	5
#define SUSPIT	6
#define SUSYAW	7

#define LSC	8
#define ASCPIT	9
#define ASCYAW	10

#define ULPOS	11
#define ULPIT	12
#define ULYAW	13
#define URPOS	14
#define URPIT	15
#define URYAW	16
#define LRPOS	17
#define LRPIT	18
#define LRYAW	19
#define LLPOS	20
#define LLPIT	21
#define LLYAW	22

#define ULCOIL	23
#define URCOIL	24
#define LRCOIL	25
#define LLCOIL	26

typedef struct CDS_EPICS_IN {
	float inputMatrix[NUM_SYSTEMS][4][3];
	float ascPhase[4][2];
	float wfsInputMatrixI[4][3];
	float wfsInputMatrixQ[4][3];
	float wfsInputMatrixP[2][4];
	float wfsInputMatrixY[2][4];
	float wfsOutputMatrixP[4][4];
	float wfsOutputMatrixY[4][4];
	float lscReflPhase[2];
	float lscASRFPhase[2];
	float lscInputMatrix[4][3];
	float lscOutputMatrix[3][6];
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

#define MAX_FILTERS     270     /* Max number of filters to one file */
#define MAX_MODULES     27      /* Max number of modules to one file */
#endif
