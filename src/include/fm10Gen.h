#ifndef FM10GEN_H
#define FM10GEN_H

static const char *fm10Gen_h_cvsid = "$Id: fm10Gen.h,v 1.5 2009/09/17 18:59:01 aivanov Exp $";

/*****************************************************************************/
/*ORGANIZATION OF THE SWITCH CONTROL REGISTER*/
/*

  bit          0 = reset coefficients.  Momentary: When hit, rereads and loads 
                   new filter coefficients from file. 
  bit          1 = master reset.  Momentary: When hit, clears out filt history
  bit          2 = input enable switch. Enables/Disables input into filter.
  bit          3 = offset switch. Enables/Disables offset calculation.
  even bits 4-22 = Filter stage On/Off readback. When on/off this will show 
                   that a filter has been requested to turn on/off.
  odd bits  5-23 = Filter output Switch On/Off readback. This will show that
                   a filter has been engaged or shut off after a request.
  bit         24 = limiter switch. Enables/Disables limiter calculation.
  bit         25 = Decimation filter switch.  Enables/Disables the dec filter.
  bit         26 = Output enable switch. Enables/Disables output from filter.
  bit         27 = Hold Output switch.  if(!bit26 && bit27) will hold outputs 
                   at last value. if(!bit26 && !bit27) will zero outputs.
                   else, nothing.  
*/
/*****************************************************************************/
/*FILTER TYPE DEFINITIONS*/
/*
  There are two types of Filter Stage input on/off switch types:

  Type 1:  When turned off/on, filter input remains on and filter output switch 
           turned off/on according to filter output type.

  Type 2:  When turned off/on, filter input and output turned off/on according
           to filter output type.

  There are three types of filter stage output on/off switch types.

  Type 1:  This will simply enable or disable the output of the filter block 
           instantaniously. This type does not use the rmpcmp structure element.

  Type 2:  This will ramp in/out a filter output slowly according to the number
           set by the rmpcmp structure element when the servo block is enabled.

  Type 3:  This will compare filter output with the straight-through input. Once
           the difference comes within the number set in the rmpcmp structure 
	   element, the filter output will turn on/off in the system.

  The system will recognize the total filter type by a numbered system of
  input/output types.  ie, they will be represented as type 11,12,13,21,... etc.
  These will be determined in the coefficient file.

*/
/*****************************************************************************/
/* #include <sysLib.h> */
#ifndef UINT32
#define UINT32 unsigned int
#endif

/*DEFINITIONS*/

#define FILTERS         10      /* Num filters per Filter Module */
#define MAX_COEFFS      41      /* Max coeff per filter, including gain */
#define MAX_HISTRY      20      /* Max num filter history elements */
#define MAX_SO_SECTIONS 10	/* Maximum number of second order sections supported */

#ifdef FIR_FILTERS
#define MAX_FIR_MODULES 4      /* Maximum total number of FIR filters allowed per system */
#include "fmFir.h"
#define MAX_FIR_SO_SECTIONS (FIR_TAPS/4)	/* Maximum SOS supported for FIR filters */
#define MAX_FIR_COEFFS	(FIR_TAPS+1)
#ifdef SERVO2K
#define FIR_POLYPHASE_SIZE	32		/* The number of parallel polyphase filters */
#endif
#ifdef SERVO4K
#define FIR_POLYPHASE_SIZE	64		/* The number of parallel polyphase filters */
#endif
#endif

/*masks*/
#define COEF_MASK       0xFFFFFFFE
#define HIST_MASK       0xFFFFFFFD

/*ADDRESSES*/

#define FM_PENT_BASE    0x70220000 /* Pentium VME BAddr for overall structure */
#define FM_MV162_BASE   0x220000   /* MV162 VME BAddr for overall structure */

/*****************************************************************************/

/*STRUCTURES*/


/* Struct of operator input to each filter module */

typedef struct FM_OP_IN{
  UINT32 opSwitchE;     /* Epics Switch Control Register; 28/32 bits used*/
  UINT32 opSwitchP;     /* PIII Switch Control Register; 28/32 bits used*/
  UINT32 rset;          /* reset switches */
  float offset;         /* signal offset */
  float outgain;        /* module gain */
  float limiter;        /* used to limit the filter output to +/- limit val */
  int rmpcmp[FILTERS];  /* ramp counts: ramps on a filter for type 2 output*/
                        /* comparison limit: compare limit for type 3 output*/
                        /* not used for type 1 output filter */
  int timeout[FILTERS]; /* used to timeout wait in type 3 output filter */
  int cnt[FILTERS];     /* used to keep track of up and down cnt of rmpcmp */
                        /* should be initialized to zero */
  float gain_ramp_time; /* gain change ramping time in seconds */
} FM_OP_IN;

typedef struct FM_GAIN_RAMP {
  float prev_outgain;   /* Normally the same as outgain */
			/* When gain is changed it holds old gain value */
  unsigned int gain_ramp_cycles; /* overall cycles to ramp gain */
  unsigned int ramp_cycles_left; /* cycles left to ramp gain */
} FM_GAIN_RAMP;

/* Struct of data output for each filter module */

typedef struct FM_OP_DATA{
  float filterInput;    /* Input to the filter bank module */
  float exciteInput;    /* an Excitation point/bypasses input switch */ 
  float inputTestpoint; /* Filter Bank switched input plus excitation */
  float testpoint;      /* Filter Bank output;always enabled */
  float output;         /* Filter Bank Output;ENABLE/DISABLE/HOLD */
  float output16Hz;     /* 16hz Output; ENABLE/DISABLE/HOLD */
}FM_OP_DATA;

/* Struct of filter module coefficients */

typedef struct FM_OP_COEF{
  double filtCoeff[FILTERS][MAX_COEFFS]; /* coefficients for filter string */
  double filtHist[FILTERS][MAX_HISTRY];  /* history buffer for filter string */
  int filtSections[FILTERS];    /* number of sections in each filter */
  int sType[FILTERS];   /* indicates the filter switch in/out type */
  /* float decHist[1024];   history for the decimation filter */
  double decHist[8];
  /* 0 - IIR; N - FIR, where 0 > N < MAX_FIR_MODULES */
  /* N-1 shows where coeffictients are. COEF.firFiltCoeff[N-1] */
  int filterType[FILTERS];
}FM_OP_COEF;


typedef struct COEF{
  FM_OP_COEF coeffs[MAX_MODULES];

#ifdef FIR_FILTERS
  double firFiltCoeff[MAX_FIR_MODULES][FILTERS][MAX_FIR_COEFFS];
  /* firHistory is huge, 5 megabytes. may need to get rid of FILTERS dimension */
  double firHistory[MAX_FIR_MODULES][FILTERS][FIR_POLYPHASE_SIZE][FIR_TAPS];
  double prevFirOutput[MAX_FIR_MODULES];
#endif

}COEF;

typedef struct VME_FM_OP_COEF{
  double filtCoeff[FILTERS][MAX_COEFFS];
  char filtName[FILTERS][32];
  int bankNum;
  int filtSections[FILTERS];    /* number of sections in each filter */
  int sType[FILTERS];   /* indicates the filter switch in/out type */
  int ramp[FILTERS];
  int timout[FILTERS];
  unsigned int crc;     /* Epics-calculated data checksum */
  int filterType[FILTERS];       /* 0 - IIR; N - FIR, where 0 > N < MAX_FIR_MODULES  */
}VME_FM_OP_COEF;

typedef struct VME_COEF{

#ifdef FIR_FILTERS
  double firFiltCoeff[MAX_FIR_MODULES][FILTERS][MAX_FIR_COEFFS];
#endif

  VME_FM_OP_COEF vmeCoeffs[MAX_MODULES];

  /* Do not put anything at the end of this structure */
  /* fmReadCoeff code gets compiled with MAX_MODULES == 0 */
}VME_COEF;

/* Struct of filter names for use in epics screens */

typedef struct FILT_NAME{ 
  char filtName[FILTERS][32];    /* holds filter names for each filter bank */
}FILT_NAME;


/* Filter modules all in one structure */

typedef struct FILT_MOD{

  FM_OP_IN   inputs[MAX_MODULES];  /* operator inputs to each filter module */
  FM_OP_DATA data[MAX_MODULES];    /* data output for each filter module */
  int cycle;
  int coef_load_error; /* Error flag inidicating problems loading coeffs by a front-end */

}FILT_MOD;

#if (defined(__i386__) || defined(__amd64__)) && !defined(NO_FM10GEN_C_CODE)
#include "../drv/crc.c"
#include "drv/fm10Gen.c"
#endif

#endif
