program daqConfig

#ifdef UNIX

%% #define UINT32 unsigned int
%% #define UINT16 unsigned short
%% #define OK 1
%% #define ERROR 0
%% #define FALSE 0
%% #define TRUE 1
%%
%% #include <errno.h>

#else
%% #include <vxWorks.h>
%% #include <ioLib.h>
#endif

%% #include <stdio.h>
%% #include <string.h>
%% #include <stdlib.h>
%% #include <math.h>
%% #include <pthread.h>
%% #include <daqmap.h>
%% #include <param.h>


int reload;
assign reload to "{ifo}:DAQ-{sys}_LOAD_CONFIG";

int dcmsg;
assign dcmsg to "{ifo}:DAQ-{sys}_MSG";

int chnCnt;
assign chnCnt to "{ifo}:DAQ-{sys}_CHAN_CNT";

int dataSize;
assign dataSize to "{ifo}:DAQ-{sys}_TOTAL";

%% static DAQ_INFO_BLOCK *info;
%% static void *base;
%% static DAQ_INFO_BLOCK infoTmp;
%% static int sysnum;
%% static char site[160];
%% static char ifo[160];
%% static char sys[160];
%% extern pthread_mutex_t seq_lock;

%% static void copy_info() {
%%   int i;
%%   for (i = 0; i < infoTmp.numChans; i++) {
%%     info -> tp [i].tpnum = infoTmp.tp [i].tpnum;
%%     info -> tp [i].dataType = infoTmp.tp [i].dataType;
%%     info -> tp [i].dataRate = infoTmp.tp [i].dataRate;
%%     info -> tp [i].dataGain = infoTmp.tp [i].dataGain;
%%   }
%%   info -> configFileCRC = infoTmp.configFileCRC;
%%   info->numChans = infoTmp.numChans;
%%   info->numChans = infoTmp.numEpicsInts;
%%   info->numChans = infoTmp.numEpicsFloats;
%%   info->numChans = infoTmp.numEpicsFilts;
%%   info->numChans = infoTmp.numEpicsTotal;
%% }

ss daqConfigControl {

state init
{
  when()
    {
%% 	extern void *findRfmCard(unsigned int);
	sysnum = atoi(macValueGet("sysnum"));
%%	if (sysnum < 0 || sysnum >= DCU_COUNT) {
%%	  fprintf(stderr, "Invalid system number %d\n", sysnum);
%%	  exit(1);
%%	}
	strcpy(site, macValueGet("site"));
	strcpy(ifo, macValueGet("ifo"));
	strcpy(sys, macValueGet("sys"));
%%	info = ((DAQ_INFO_BLOCK *)((base = (volatile void *)findRfmCard(0)) + DAQ_INFO_ADDRESS));
%%	printf("daq info at 0x%lx\n",(long)info);
%%	memset(&infoTmp, 0, sizeof(infoTmp));
%%	info->reconfig = 0;
%%	if (loadDaqConfigFile(&infoTmp, site, ifo, sys)) {
%%	   copy_info();
%%	   printf("%s%s daqConfig info is at rfm offset 0x%x\n", ifo, sys, ((char *)info) - ((char *)base));
%%	   info -> reconfig = 1;
%%	   dcmsg = infoTmp.configFileCRC;
%%	   chnCnt = infoTmp.numChans;
%%	   dataSize = 0;
%%	   { int i; 
%%		for(i=0; i < infoTmp.numChans; i++) {
%%			dataSize += infoTmp.tp[i].dataRate * DAQ_DATA_TYPE_SIZE(infoTmp.tp[i].dataType);
%%		}
%%	   }
%%	   dataSize /= 1024;
%%	} else {
%%	   dcmsg = 0xffffffff;
%%	   chnCnt = 0;
%%	   dataSize = 0;
%%	}
        pvPut(dcmsg);
        pvPut(chnCnt);
        pvPut(dataSize);
    } state process
}


state process
{
  when(delay(0.2))
    {
%%	pthread_mutex_lock (&seq_lock);
	pvGet(reload);
%%	pthread_mutex_unlock (&seq_lock);
	if (reload)
%%	  {
%%	    info -> reconfig = 0;
%%	    if (loadDaqConfigFile(&infoTmp, site, ifo, sys)) {
%%	      copy_info();
%%	      printf("%s%s daqConfig info is at rfm offset 0x%x\n", ifo, sys, ((char *)info) - ((char *)base));
%%	      info -> reconfig = 1;
%%	      dcmsg = infoTmp.configFileCRC;
%%	      chnCnt = infoTmp.numChans;
%%	      dataSize = 0;
%%	      { int i;
%%		for(i=0; i < infoTmp.numChans; i++) {
%%			dataSize += infoTmp.tp[i].dataRate * DAQ_DATA_TYPE_SIZE(infoTmp.tp[i].dataType);
%%		}
%%	      }
%%	      dataSize /= 1024;
%%	    } else {
%%	      dcmsg = 0xffffffff;
%%	      chnCnt = 0;
%%	      dataSize = 0;
%%	    }
            pvPut(dcmsg);
            pvPut(chnCnt);
            pvPut(dataSize);
%%	  }
	reload = 0;
        pvPut(reload);
    } state process
}
}
