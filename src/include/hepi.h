#include "fmFir.h"
#define CHAM_FILT	96

#define EPICS_IN_SIZE   189
#define EPICS_OUT_SIZE  234


#define POS_V1		 0
#define POS_V2		 1
#define POS_V3		 2
#define POS_V4		 3
#define POS_H1		 4
#define POS_H2		 5
#define POS_H3		 6
#define POS_H4		 7
#define GEO_V1		 8
#define GEO_V2		 9
#define GEO_V3		10
#define GEO_V4		11
#define GEO_H1		12
#define GEO_H2		13
#define GEO_H3		14
#define GEO_H4		15
#define TILTCORR_V1	16
#define TILTCORR_V2	17
#define TILTCORR_V3	18
#define TILTCORR_V4	19
#define TILTCORR_H1	20
#define TILTCORR_H2	21
#define TILTCORR_H3	22
#define TILTCORR_H4	23
#define POS_X		24
#define POS_Y		25
#define POS_Z		26
#define POS_RX		27
#define POS_RY		28	
#define POS_RZ		29
#define POS_VP		30
#define POS_HP		31
#define GEO_X		32
#define GEO_Y		33
#define GEO_Z		34
#define GEO_RX		35
#define GEO_RY		36	
#define GEO_RZ		37
#define GEO_VP		38
#define GEO_HP		39
#define MODAL_X		40
#define MODAL_Y		41
#define MODAL_Z		42
#define MODAL_RX	43
#define MODAL_RY	44	
#define MODAL_RZ	45
#define MODAL_VP	46
#define MODAL_HP	47
#define ACT_V1		48
#define ACT_V2		49
#define ACT_V3		50
#define ACT_V4		51
#define ACT_H1		52
#define ACT_H2		53
#define ACT_H3		54
#define ACT_H4		55
#define TIDAL_DM	56
#define TIDAL_CM	57
#define TIDAL_PD	58
#define POS_V1E		59
#define POS_V2E		60
#define POS_V3E		61
#define POS_V4E		62
#define POS_H1E		63
#define POS_H2E		64
#define POS_H3E		65
#define POS_H4E		66
#define GEO_V1E		67
#define GEO_V2E		68
#define GEO_V3E		69
#define GEO_V4E		70
#define GEO_H1E		71
#define GEO_H2E		72
#define GEO_H3E		73
#define GEO_H4E		74
#define STS_XE		75
#define STS_YE		76
#define STS_ZE		77
#define GEO_X_RMS	78
#define GEO_Y_RMS	79
#define GEO_Z_RMS	80
#define GEO_X_RMS2	81
#define GEO_Y_RMS2	82
#define GEO_Z_RMS2	83
#define LSC_X		84
#define LSC_Y		85
#define LSC_Z		86
#define OPLEV_P		87
#define OPLEV_Y		88
#define STS_X_RY	89
#define STS_Y_RX	90
#define FIR_X_OUT	91
#define FIR_Y_OUT	92
#define STS_X		93
#define STS_Y		94
#define STS_Z		95
#define FIR_DF_X1	288
#define FIR_DF_Y1	289
#define FIR_UF_X1	290
#define FIR_UF_Y1	291
#define FIR_CF_X1	292
#define FIR_CF_Y1	293
#define FIR_DF_X2	294
#define FIR_DF_Y2	295
#define FIR_UF_X2	296
#define FIR_UF_Y2	297
#define FIR_CF_X2	298
#define FIR_CF_Y2	299

typedef struct HEPI_IO{
        float sts_matrix[8][4];
        float pos_matrix[8][8];
        float geo_matrix[8][8];
        float act_matrix[8][8];
        float dcm_matrix[8][6];
	float dcModalBias[2][6];
	float lscMatrix[3][8];
	float sts_in_matrix[2][6];
	float lscPostMatrix[3];
	float olMatrix[2][2];
        float tc_matrix[8][8];
	int hepiSwitch[14];
	float gain;
	int rampTime;
	int tramp[6];
	float tidalLimit[4];
	float lscLimit[4];
	float loopGain;
	float tcGain;
	int wdLimit[4];
	int wdLimitF[6];
	int lscLockTime;
	float tidal[6];
	int hepiBinary[20];
	int hepiOutput[8];
	int modalMatOut[8];
	int tcMatOut[8];
	int spares[4];
	int lscLock;
	int wdLoBytes;
	int wdHiBytes;
	int wdLoBytesF;
	int wdHiBytesF;
	int wdTrip;
	int wdReset;
	int wdFabs[2][24];
	float geoRms[6];
	int geoSen[8];
}HEPI_IO;

typedef struct STS{
	float stsIn[3];
	float stsOut[3];
	float firOffset[2];
	float firGain[2];
	float firLimit[2];
	int firSw1[2];
	int firSw1R[2];
	int massPos[4];
}STS;

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

typedef struct CDS_EPICS {
        CDS_EPICS_IN epicsInput;
        CDS_EPICS_OUT epicsOutput;
	HEPI_IO hepi[3];
	STS sts[2];
} CDS_EPICS;

