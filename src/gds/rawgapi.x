/* Version $Id$ */
/* GDS arbitrary waveform generator rpc interface */

/* fix include problems with VxWorks */
#ifdef RPC_HDR
%#define		_RPC_HDR
#endif
#ifdef RPC_XDR
%#define		_RPC_XDR
#endif
#ifdef RPC_SVC		
%#define		_RPC_SVC
#endif
#ifdef RPC_CLNT		
%#define		_RPC_CLNT
#endif
%#include "dtt/rpcinc.h"


/* wave type for function generator */
/* typedef int awgwavetype_r; */

/* awg component type */
struct awgcomponent_r {
   int		 	wtype;
   int			ramptype;
   double 		par[4];
   hyper		start;
   hyper		duration;
   hyper		restart;
   hyper		ramptime[2];
   double 		ramppar[4];
};

/* awg component list */
typedef awgcomponent_r awgcomponent_list_r<>;

/* waveform */
typedef float awgwaveform_r<>;

/* filter coefficients */
typedef double awgfilter_r<>;

/* awg query result */
struct awgquerywaveforms_r {
   int			status;
   awgcomponent_list_r	wforms;
};

/* awg show result */
struct awgshow_r {
   int			status;
   string		res<>;
};

/* awg statistics result */
struct awgstat_r {
      /* Number of waveform processing cycles */
      double		pwNum;
      /* Average time for processing waveforms */
      double		pwMean;
      /* Standard deviation of process time */
      double		pwStddev;
      /* Maximum time needed to process the waveforms */
      double		pwMax;
      /* Number of reflective memory writes */
      double		rmNum;
      /* Average time to write to reflective memory */
      double		rmMean;
      /* Standard deviation of writing to reflective memory */
      double		rmStddev;
      /* Maximum time needed to write to reflective memory */
      double		rmMax;
      /* Critical time. Smallest time difference between the 
          time the waveform is needed and the time the waveform
          is written to reflective memory. A negative value 
          indicates an instance where the waveform came too late */
      double		rmCrit;
      /* Number of times the waveform was written too late. 
          This not necessarily means that the waveform is 
          corrupted! It only means the writting finished after
          the reading started. */
      double		rmNumCrit;
      /* Number of writes to the DAC */
      double		dcNum;
      /* Average time to write to the DAC */
      double		dcMean;
      /* Standard deviation of writing to teh DAC */
      double		dcStddev;
      /* Maximum time needed to write to the DAC */
      double		dcMax;
      /* Number of times the waveform was written too late */
      double		dcNumCrit;
      /* status */
      int 		status;
};

/*  pointer type to get aound linus rpcgen bug */
typedef awgcomponent_list_r* awgcompl_ptr;

/* rpc interface */
program RAWGPROG {
   version RAWGVERS {

      int AWGNEWCHANNEL (int chntype, int id, int arg1, int arg2) = 1;
      int AWGREMOVECHANNEL (int slot) = 2;
      int AWGADDWAVEFORM (int slot, awgcompl_ptr comps) = 3;
      int AWGSETWAVEFORM (int slot, awgwaveform_r wave) = 4;
      int AWGSENDWAVEFORM (int slot, unsigned int time, int epoch, 
                           awgwaveform_r wave) = 11;
      int AWGSTOPWAVEFORM (int slot, int terminate, hyper tArg) = 10;
      int AWGSETGAIN (int slot, double gain, hyper tArg) = 14;
      int AWGCLEARWAVEFORMS (int slot) = 5;
      awgquerywaveforms_r AWGQUERYWAVEFORMS (int slot, int maxComp) = 6;
      int AWGRESET (void) = 7;
      int AWGSETFILTER (int slot, awgfilter_r filter) = 12;
      awgstat_r AWGSTATISTICS (int reset) = 8;
      awgshow_r AWGSHOW (void) = 9;
      awgshow_r AWGSHOWSLOT (int slot) = 13;

   } = 1;
} = 0x31001002;
