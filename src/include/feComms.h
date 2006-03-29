#ifdef HEPI
#include <hepi.h>
#endif
#ifdef SUS_CODE
#include <sus.h>
#endif

#ifdef PNM
#include <pnm.h>
#endif

/* Vme bus reset */
#define SYSVME_RESET_MAGIC_WORD 0x13571113

typedef struct FE2FE_COMMS{
	double dData[16];
	float fData[16];
	int iData[16];
}FE2FE_COMMS;

typedef struct FEFE_COMMS{
	FE2FE_COMMS feComms[32];
}FEFE_COMMS;

#define FE2FE_OFFSET	0x10000

typedef struct RFM_FE_COMMS {
    char pad[0x1000];			/* Reserved for 5579 cntrl reg */
    union{				/* Starts at 	0x0000 0040 */
      char sysepics[0x100000];
      CDS_EPICS epicsShm;
    }epicsSpace;
    union{				/*		0x0000 1000 */
      char sysdsp[0x200000];
      FILT_MOD epicsDsp;
    }dspSpace
#ifdef PNM
	[NUM_SYSTEMS]
#endif
    ;
    union{				/*		0x0000 2000 */
      char syscoeff[0x400000];
      VME_COEF epicsCoeff;
    }coeffSpace
#ifdef PNM
	[NUM_SYSTEMS]
#endif
    ;
}RFM_FE_COMMS;

typedef struct GDS_SELECTIONS {
        int gdsExcMon[15];
        int gdsTpMon[30];
}GDS_SELECTIONS; 

