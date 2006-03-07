#ifndef PNM_SUS_H_INCLUDED
#define PNM_SUS_H_INCLUDED

#define NUM_SYSTEMS 5

/* Assign filter names to filter number */
#define LSC	0
#define SUSPOS	1	
#define SUSPIT	2
#define SUSYAW	3
#define ASCPIT	4
#define ASCYAW	5
#define ULPOS	6
#define ULPIT	7
#define ULYAW	8
#define LLPOS	9
#define LLPIT	10
#define LLYAW	11
#define URPOS	12
#define URPIT	13
#define URYAW	14
#define LRPOS	15
#define LRPIT	16
#define LRYAW	17
#define ULCOIL	18
#define LLCOIL	19
#define URCOIL	20
#define LRCOIL	21
#define ULSEN	22
#define LLSEN	23
#define URSEN	24
#define LRSEN	25
#define SDSEN	26

typedef struct CDS_EPICS_IN {
	float inputMatrix[NUM_SYSTEMS][4][3];
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
