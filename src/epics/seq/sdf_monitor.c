///	@file /src/epics/seq/sdf_monitor.c
///	@brief Contains required 'main' function to startup EPICS sequencers, along with supporting routines. 
///<		This code is taken from EPICS example included in the EPICS distribution and modified for LIGO use.

/********************COPYRIGHT NOTIFICATION**********************************
This software was developed under a United States Government license
described on the COPYRIGHT_UniversityOfChicago file included as part
of this distribution.
****************************************************************************/

// TODO:
// - Make appropriate log file entries
// - Get rid of need to build skeleton.st

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "iocsh.h"
#include "dbStaticLib.h"
#include "crc.h"
#include "dbCommon.h"
#include "recSup.h"
#include "dbDefs.h"
#include "dbFldTypes.h"
#include "dbAccess.h"

#define epicsExportSharedSymbols
#include "asLib.h"
#include "asCa.h"
#include "asDbLib.h"

#include <daqmap.h>
#include <param.h>
// #include "fm10Gen.h"
#include "findSharedMemory.h"
#include "cadef.h"
#include "fb.h"
#include "../../drv/symmetricom/symmetricom.h"

#define GSDF_MAX_CHANS	30000
// Gloabl variables		****************************************************************************************
char timechannel[256];		///< Name of the GPS time channel for timestamping.
char reloadtimechannel[256];	///< Name of EPICS channel which contains the BURT reload requests.
struct timespec t;
char logfilename[128];
char logfiledir[128];
char gsdflogfilename[128];
char statelogfilename[128];
unsigned char naughtyList[GSDF_MAX_CHANS][64];

// Function prototypes		****************************************************************************************
int checkFileCrc(char *);
void getSdfTime(char *);
void logFileEntry(char *);

unsigned long daqFileCrc;
typedef struct gsdf_c {
	int num_chans;
	int con_chans;
	int val_events;
	int con_events;
	double channel_value[GSDF_MAX_CHANS];
	char channel_name[GSDF_MAX_CHANS][64];
	char channel_string[GSDF_MAX_CHANS][64];
	int channel_status[GSDF_MAX_CHANS];
	int channel_type[GSDF_MAX_CHANS];
	long gpsTime;
	long epicsSync;
} gsdf_c;

#define MY_DBL_TYPE	0
#define MY_STR_TYPE	1

gsdf_c gsdf;
static struct rmIpcStr *dipc;
static char *shmDataPtr;
static struct cdsDaqNetGdsTpNum *shmTpTable;
static const int buf_size = DAQ_DCU_BLOCK_SIZE * 2;
static const int header_size = sizeof(struct rmIpcStr) + sizeof(struct cdsDaqNetGdsTpNum);
static DAQ_XFER_INFO xferInfo;
static float dataBuffer[2][GSDF_MAX_CHANS];
static int timeIndex;
static int cycleIndex;
static int symmetricom_fd = -1;
int timemarks[16] = {1000,63500,126000,188500,251000,313500,376000,438500,501000,563500,626000,688500,751000,813500,876000,938500};
int nextTrig = 0;


// End Header **********************************************************************************************************
//
void getSdfTime2(char *timestring, int size)
{
        time_t t=0;
        struct tm tdata;

        t = time(NULL);
        localtime_r(&t, &tdata);
        if (timestring && size > 0) {
                if (strftime(timestring, size, "%a_%b_%e_%H_%M_%S_%Y", &tdata) == 0) {
                        timestring[0] = '\0';
                }
        }
}

// **************************************************************************
/// Get current GPS time from the symmetricom IRIG-B card
unsigned long symm_gps_time(unsigned long *frac, int *stt) {
// **************************************************************************
    unsigned long t[3];
    ioctl (symmetricom_fd, IOCTL_SYMMETRICOM_TIME, &t);
    t[1] *= 1000;
    t[1] += t[2];
    if (frac) *frac = t[1];
         if (stt) *stt = 0;
         // return  t[0] + daqd.symm_gps_offset;
         return  t[0];
}
// **************************************************************************
void waitGpsTrigger(unsigned long gpssec, int cycle)
// **************************************************************************
{
unsigned long gpsSec, gpsuSec;
int gpsx;
	do{
		usleep(1000);
		gpsSec = symm_gps_time(&gpsuSec, &gpsx);
		gpsuSec /= 1000;
	}while(gpsSec < gpssec || gpsuSec < timemarks[cycle]); 
}

// **************************************************************************
/// See if the GPS card is locked.
int symm_gps_ok() {
// **************************************************************************
    unsigned long req = 0;
    ioctl (symmetricom_fd, IOCTL_SYMMETRICOM_STATUS, &req);
    printf("Symmetricom status: %s\n", req? "LOCKED": "UNCLOCKED");
   return req;
}

// **************************************************************************
unsigned long symm_initialize()
// **************************************************************************
{
	symmetricom_fd =  open ("/dev/symmetricom", O_RDWR | O_SYNC);
	if (symmetricom_fd < 0) {
	       perror("/dev/symmetricom");
	       exit(1);
	}
	unsigned long gpsSec, gpsuSec;
	int gpsx;
	int gpssync;
	gpssync =  symm_gps_ok();
	gpsSec = symm_gps_time(&gpsuSec, &gpsx);
	printf("GPS SYNC = %d %d\n",gpssync,gpsx);
	printf("GPS SEC = %ld  USEC = %ld  OTHER = %d\n",gpsSec,gpsuSec,gpsx);
	// Set system to start 2 sec from now.
	gpsSec += 2;
	return(gpsSec);
}

// **************************************************************************
void connectCallback(struct connection_handler_args args) {
// **************************************************************************
        unsigned long chnum = (unsigned long)ca_puser(args.chid);
	gsdf.channel_status[chnum] = args.op == CA_OP_CONN_UP? 0: 0xbad;
        if (args.op == CA_OP_CONN_UP) gsdf.con_chans++; else gsdf.con_chans--;
        gsdf.con_events++;
}


// **************************************************************************
void subscriptionHandler(struct event_handler_args args) {
// **************************************************************************
char timestring[256];
FILE *statelog;
char state[64];
char newfile[256];
 	gsdf.val_events++;
        if (args.status != ECA_NORMAL) {
        	return;
        }
        if (args.type == DBR_FLOAT) {
        	float val = *((float *)args.dbr);
                gsdf.channel_value[(unsigned long)args.usr] = val;
        } else if (args.type == DBR_DOUBLE) {
        	double val1 = *((double *)args.dbr);
                gsdf.channel_value[(unsigned long)args.usr] = val1;
		// printf("Channel %s changed to %lf\n",gsdf.channel_name[(unsigned long)args.usr],val1);
        }else{
		strcpy(gsdf.channel_string[(unsigned long)args.usr],(char *)args.dbr);
		getSdfTime2(timestring, 256);
		printf("Time is %s\n",timestring);
		printf("Channel %s has value %s\n",gsdf.channel_name[(unsigned long)args.usr],gsdf.channel_string[(unsigned long)args.usr]);
		statelog = fopen(statelogfilename,"a");
		fprintf(statelog,"%s %s\n",gsdf.channel_string[(unsigned long)args.usr],timestring);
		fclose(statelog);
		sprintf(newfile,"%sdata/%s",logfiledir,timestring);
		printf("Ready to write %s\n",newfile);
		gsdfWriteData(newfile);
	}
}

// **************************************************************************
int gsdfClearSdf(char *pref)
// **************************************************************************
{
unsigned char clearString[64] = "          ";
int flength = 62;
long status;
dbAddr saddr;
int ii;
char s[64];
char s1[64];
char s2[64];
char s3[64];
char s4[64];
dbAddr baddr;
dbAddr maddr;
dbAddr taddr;
dbAddr daddr;



	for(ii=0;ii<40;ii++)
        {
	 sprintf(s, "%s_%s_STAT%d", pref,"SDF_SP", ii);
	 status = dbNameToAddr(s,&saddr);
	 status = dbPutField(&saddr,DBR_UCHAR,clearString,16);

	 sprintf(s1, "%s_%s_STAT%d_BURT", pref,"SDF_SP", ii);
	 status = dbNameToAddr(s1,&baddr);
	 status = dbPutField(&baddr,DBR_UCHAR,clearString,flength);

	 sprintf(s2, "%s_%s_STAT%d_LIVE", pref,"SDF_SP", ii);
	 status = dbNameToAddr(s2,&maddr);
	 status = dbPutField(&maddr,DBR_UCHAR,clearString,flength);

	 sprintf(s3, "%s_%s_STAT%d_TIME", pref,"SDF_SP", ii);
	 status = dbNameToAddr(s3,&taddr);
	 status = dbPutField(&taddr,DBR_UCHAR,clearString,flength);

	  sprintf(s4, "%s_%s_STAT%d_DIFF", pref,"SDF_SP", ii);
	  status = dbNameToAddr(s4,&daddr);
	  status = dbPutField(&daddr,DBR_UCHAR,clearString,flength);

	 }


}

// **************************************************************************
int gsdfFindUnconnChannels()
// **************************************************************************
{
int ii;
int dcc = 0;

	for (ii=0;ii<gsdf.num_chans;ii++)
	{
		if(gsdf.channel_status[ii] != 0)
		{
			sprintf(naughtyList[dcc],"%s",gsdf.channel_name[ii]);
			dcc ++;
		}
	}
	return(dcc);
}
// **************************************************************************
int gsdfReportUnconnChannels(char *pref, int dc, int offset)
// **************************************************************************
{
int ii;
dbAddr saddr;
dbAddr laddr;
dbAddr sperroraddr;
char s[64];
char sl[64];
long status;
int flength = 62;
unsigned char tmpstr[64];
int rc = 0;
int myindex = 0;
int numDisp = 0;
int lineNum = 0;
int erucError = 0;


	myindex = offset * 40;
	if(myindex > dc) {
		myindex = 0;
		erucError = -1;
	}
	rc = myindex + 40;
	if(rc > dc) rc = dc;
	// printf("Naught =  %d to %d\n",myindex,rc);
	// printf("In error listing \n");
	for (ii=myindex;ii<rc;ii++)
	{
		sprintf(s, "%s_%s_STAT%d", pref,"SDF_SP", (ii - myindex));
		status = dbNameToAddr(s,&saddr);
		if(status) 
		{
			printf("Can't connect to %s\n",s);
		} else {
			sprintf(tmpstr,"%s",naughtyList[ii]);
			status = dbPutField(&saddr,DBR_UCHAR,tmpstr,flength);
			numDisp ++;
		}
		sprintf(sl, "%s_SDF_LINE_%d", pref, (ii - myindex));
		status = dbNameToAddr(sl,&laddr);
		if(status) 
		{
			printf("Can't connect to %s\n",s);
		} else {
			lineNum = ii + 1;
			status = dbPutField(&laddr,DBR_LONG,&lineNum,1);
		}
	}
	// Clear out remaining reporting channels.
	sprintf(tmpstr,"%s","  ");
	for (ii=numDisp;ii<40;ii++) {
		sprintf(s, "%s_%s_STAT%d", pref,"SDF_SP", ii);
		status = dbNameToAddr(s,&saddr);
		status = dbPutField(&saddr,DBR_UCHAR,tmpstr,flength);
		sprintf(sl, "%s_SDF_LINE_%d", pref, (ii - myindex));
		status = dbNameToAddr(sl,&laddr);
		lineNum = ii + 1;
		status = dbPutField(&laddr,DBR_LONG,&lineNum,1);
	}
	 char speStat[256]; sprintf(speStat, "%s_%s", pref, "SDF_TABLE_ENTRIES");             // Setpoint diff counter
	 status = dbNameToAddr(speStat,&sperroraddr);                    // Get Address
	 status = dbPutField(&sperroraddr,DBR_LONG,&dc,1);          // Init to zero.

	return(erucError);
}

// **************************************************************************
void gsdfCreateChanList(char *daqfilename) {
// **************************************************************************
int i;
int status;
FILE *daqfileptr;
FILE *gsdflog;
char errMsg[64];
// char daqfile[64];
char line[128];
char *newname;

	// sprintf(daqfile, "%s%s", fdir, "EDCU.ini");
	gsdf.num_chans = 0;
	daqfileptr = fopen(daqfilename,"r");
	if(daqfileptr == NULL) {
		sprintf(errMsg,"DAQ FILE ERROR: FILE %s DOES NOT EXIST\n",daqfilename);
	        logFileEntry(errMsg);
        }
	gsdflog = fopen(gsdflogfilename,"w");
	if(daqfileptr == NULL) {
		sprintf(errMsg,"DAQ FILE ERROR: FILE %s DOES NOT EXIST\n",gsdflogfilename);
	        logFileEntry(errMsg);
        }
	while(fgets(line,sizeof line,daqfileptr) != NULL) {
		fprintf(gsdflog,"%s",line);
		status = strlen(line);
		if(strncmp(line,"[",1) == 0 && status > 0) {
			if(strstr(line,"string") != NULL) {
				printf("Found string type %s \n",line);
				gsdf.channel_type[gsdf.num_chans] = MY_STR_TYPE;
			} else {
				gsdf.channel_type[gsdf.num_chans] = MY_DBL_TYPE;
			}
			newname = strtok(line,"]");
			// printf("status = %d New name = %s and %s\n",status,line,newname);
			newname = strtok(line,"[");
			// printf("status = %d New name = %s and %s\n",status,line,newname);
			if(strcmp(newname,"default") == 0) {
				printf("DEFAULT channel = %s\n", newname);
			} else {
				// printf("NEW channel = %s\n", newname);
				sprintf(gsdf.channel_name[gsdf.num_chans],"%s",newname);
				gsdf.num_chans ++;
			}
		}
	}
	fclose(daqfileptr);
	fclose(gsdflog);

	xferInfo.crcLength = 4 * gsdf.num_chans;
	printf("CRC data length = %d\n",xferInfo.crcLength);

	     chid chid1;
	ca_context_create(ca_enable_preemptive_callback);
     	for (i = 0; i < gsdf.num_chans; i++) {
	     status = ca_create_channel(gsdf.channel_name[i], connectCallback, (void *)i, 0, &chid1);
	     if(gsdf.channel_type[i] == MY_STR_TYPE) {
		printf("Found the string channel = %s\n",gsdf.channel_name[i]);
	     	status = ca_create_subscription(DBR_STRING, 0, chid1, DBE_VALUE,
			     subscriptionHandler, (void *)i, 0);
	     } else {
	     	status = ca_create_subscription(DBR_DOUBLE, 0, chid1, DBE_VALUE,
			     subscriptionHandler, (void *)i, 0);
		}
	}
	timeIndex = 0;
}

// **************************************************************************
void gsdfWriteData(char *filename)
// **************************************************************************
{
int ii;
FILE *log;

		
	log = fopen(filename,"w");
	for(ii=0;ii<gsdf.num_chans;ii++) {
		if(gsdf.channel_type[ii] == MY_STR_TYPE)
			fprintf(log,"%s %s STRING\n",gsdf.channel_name[ii],gsdf.channel_string[ii]);
		else
			fprintf(log,"%s %.15e DOUBLE\n",gsdf.channel_name[ii],gsdf.channel_value[ii]);
	}
	fclose(log);
}

// **************************************************************************
void gsdfInitialize(char *shmem_fname)
// **************************************************************************
{

	// Find start of DAQ shared memory
	void *dcu_addr = findSharedMemory(shmem_fname);
	// Find the IPC area to communicate with mxstream
	dipc = (struct rmIpcStr *)((char *)dcu_addr + CDS_DAQ_NET_IPC_OFFSET);
	// Find the DAQ data area.
        shmDataPtr = (char *)((char *)dcu_addr + CDS_DAQ_NET_DATA_OFFSET);
}



/// Common routine to check file CRC.
///	@param[in] *fName	Name of file to check.
///	@return File CRC or -1 if file not found.
int checkFileCrc(char *fName)
{
char buffer[256];
FILE *pipePtr;
struct stat statBuf;
long chkSum = -99999;
      	strcpy(buffer, "cksum "); 
      	strcat(buffer, fName); 
      	if (!stat(fName, &statBuf) ) { 
         	if ((pipePtr = popen(buffer, "r")) != NULL) {
            	fgets(buffer, 256, pipePtr);
            	pclose(pipePtr); 
            	sscanf(buffer, "%ld", &chkSum);
         	} 
		return(chkSum);
    	}    
	return(-1);
}

/// Routine for reading GPS time from model EPICS record.
///	@param[out] timestring 	Pointer to char string in which GPS time is to be written.
void getSdfTime(char *timestring)
{

	dbAddr paddr;
	long ropts = 0;
	long nvals = 1;
	long status;

	status = dbNameToAddr(timechannel,&paddr);
	status = dbGetField(&paddr,DBR_STRING,timestring,&ropts,&nvals,NULL);
}

/// Routine for logging messages to ioc.log file.
/// 	@param[in] message Ptr to string containing message to be logged.
void logFileEntry(char *message)
{
	FILE *log;
	char timestring[256];
	long status;
	dbAddr paddr;

	getSdfTime(timestring);
	log = fopen(logfilename,"a");
	if(log == NULL) {
		status = dbNameToAddr(reloadtimechannel,&paddr);
		status = dbPutField(&paddr,DBR_STRING,"ERR - NO LOG FILE FOUND",1);
	} else {
		fprintf(log,"%s\n%s\n",timestring,message);
		fprintf(log,"***************************************************\n");
		fclose(log);
	}

}

/// Called on EPICS startup; This is generic EPICS provided function, modified for LIGO use.
int main(int argc,char *argv[])
{
	// Addresses for SDF EPICS records.
	// Initialize request for file load on startup.
	long status;
	int request;
	int daqTrigger;
	long ropts = 0;
	long nvals = 1;
	int rdstatus = 0;
	char timestring[128];
	int ii;
	int fivesectimer = 0;
	long coeffFileCrc;
	char modfilemsg[] = "Modified File Detected ";
	struct stat st = {0};
	char filemsg[128];
	char logmsg[256];
	unsigned int pageNum = 0;
	unsigned int dataDump = 0;

printf("Entering main\n");
    if(argc>=2) {
        iocsh(argv[1]);
	printf("Executing post script commands\n");
	// Get environment variables from startup command to formulate EPICS record names.
	char *pref = getenv("PREFIX");
	char *modelname =  getenv("SDF_MODEL");
	char daqsharedmemname[64];
	sprintf(daqsharedmemname, "%s%s", modelname, "_daq");
	char *targetdir =  getenv("TARGET_DIR");
	char *daqFile =  getenv("DAQ_FILE");
	char *daqDir =  getenv("DAQ_DIR");
	char *coeffFile =  getenv("COEFF_FILE");
	char *logdir = getenv("LOG_DIR");
	if(stat(logdir, &st) == -1) mkdir(logdir,0777);
	// strcat(sdf,"_safe");
	printf("My prefix is %s\n",pref);
	sprintf(logfilename, "%s%s", logdir, "/ioc.log");
	sprintf(logfiledir,"%s%s",logdir,"/");
	printf("LOG FILE = %s\n",logfilename);
sleep(2);
	// **********************************************
	//
	dbAddr eccaddr;
	char eccname[256]; sprintf(eccname, "%s_%s", pref, "GSDF_CHAN_CONN");			// Number of setting channels in EPICS db
	status = dbNameToAddr(eccname,&eccaddr);

	dbAddr chcntaddr;
	// char chcntname[256]; sprintf(chcntname, "%s", "X1:DAQ-FEC_54_EPICS_CHAN_CNT");	// Request to monitor all channels.
	char chcntname[256]; sprintf(chcntname, "%s_%s", pref, "GSDF_CHAN_CNT");	// Request to monitor all channels.
	status = dbNameToAddr(chcntname,&chcntaddr);		// Get Address.

	dbAddr chnotfoundaddr;
	char cnfname[256]; sprintf(cnfname, "%s_%s", pref, "GSDF_CHAN_NOCON");		// Number of channels not found.
	status = dbNameToAddr(cnfname,&chnotfoundaddr);

	dbAddr daqbyteaddr;
	char daqbytename[256]; sprintf(daqbytename, "%s_%s", pref, "DAQ_BYTE_COUNT");	// Request to monitor all channels.
	status = dbNameToAddr(daqbytename,&daqbyteaddr);		// Get Address.

	dbAddr daqmsgaddr;
	char moddaqfilemsg[256]; sprintf(moddaqfilemsg, "%s_%s", pref, "MSGDAQ");	// Record to write if DAQ file changed.
	status = dbNameToAddr(moddaqfilemsg,&daqmsgaddr);

	sprintf(timechannel,"%s_%s", pref, "TIME_STRING");
	// printf("timechannel = %s\n",timechannel);
	
	dbAddr reloadtimeaddr;
	sprintf(reloadtimechannel,"%s_%s", pref, "MSGDAQ");			// Time of last BURT reload
	status = dbNameToAddr(reloadtimechannel,&reloadtimeaddr);

	getSdfTime(timestring);
	status = dbPutField(&reloadtimeaddr,DBR_STRING,timestring,1);

	dbAddr gpstimedisplayaddr;
	char gpstimedisplayname[256]; sprintf(gpstimedisplayname, "%s_%s", pref, "TIME_DIAG");	// SDF Save command.
	status = dbNameToAddr(gpstimedisplayname,&gpstimedisplayaddr);		// Get Address.

	dbAddr pagereqaddr;
	char pagereqname[256]; sprintf(pagereqname, "%s_%s", pref, "SDF_PAGE");	// SDF Save command.
	status = dbNameToAddr(pagereqname,&pagereqaddr);		// Get Address.

	dbAddr datadumpaddr;
	char datadumpname[256]; sprintf(datadumpname, "%s_%s", pref, "GSDF_DUMP_DATA");	// SDF Save command.
	status = dbNameToAddr(datadumpname,&datadumpaddr);		// Get Address.

// EDCU STUFF ********************************************************************************************************
	
	sprintf(gsdflogfilename, "%s%s", logdir, "/gsdf.log");
	sprintf(statelogfilename, "%s%s", logdir, "/state.log");
printf("Going to initialize\n");
	gsdfInitialize(daqsharedmemname);
	gsdfCreateChanList(daqFile);
	status = dbPutField(&chcntaddr,DBR_LONG,&gsdf.num_chans,1);
	int datarate = gsdf.num_chans * 4 / 1000;
	status = dbPutField(&daqbyteaddr,DBR_LONG,&datarate,1);

// Start SPECT
	gsdf.gpsTime = symm_initialize();
	gsdf.epicsSync = 0;

// End SPECT
	for (ii=0;ii<GSDF_MAX_CHANS;ii++) gsdf.channel_status[ii] = 0xbad;

	int dropout = 0;
	int numDC = 0;
	int cycle = 0;
	int numReport = 0;

	// Initialize DAQ and COEFF file CRC checksums for later compares.
	daqFileCrc = checkFileCrc(daqFile);
	printf("DAQ file CRC = %u \n",daqFileCrc);  
	coeffFileCrc = checkFileCrc(coeffFile);
	sprintf(logmsg,"%s\n%s = %u\n%s = %d","GSDF code restart","File CRC",daqFileCrc,"Chan Cnt",gsdf.num_chans);
	logFileEntry(logmsg);
	gsdfClearSdf(pref);
	// Start Infinite Loop 		*******************************************************************************
	for(;;) {
		dropout = 0;
		waitGpsTrigger(gsdf.gpsTime, gsdf.epicsSync);
		// printf("GSDF %ld - %d\n",gsdf.gpsTime,gsdf.epicsSync);
		status = dbPutField(&gpstimedisplayaddr,DBR_LONG,&gsdf.gpsTime,1);		// Init to zero.
		gsdf.epicsSync = (gsdf.epicsSync + 1) % 16;
		if (gsdf.epicsSync == 0) gsdf.gpsTime ++;
		status = dbPutField(&daqbyteaddr,DBR_LONG,&datarate,1);
		int conChans = gsdf.con_chans;
		status = dbPutField(&eccaddr,DBR_LONG,&conChans,1);
		// if((conChans != gsdf.num_chans) || (numDC != 0)) numDC = gsdfReportUnconnChannels(pref);
		// if(conChans != gsdf.num_chans) numDC = gsdfReportUnconnChannels(pref);
		if (gsdf.epicsSync == 0) {
			status = dbGetField(&pagereqaddr,DBR_USHORT,&pageNum,&ropts,&nvals,NULL);
			// printf("Page is %d\n",pageNum);
			numDC = gsdfFindUnconnChannels();
			numReport = gsdfReportUnconnChannels(pref,numDC,pageNum);
			if(numReport == -1) {
				pageNum = 0;
				status = dbPutField(&pagereqaddr,DBR_USHORT,&pageNum,1);		// Init to zero.
			}
			status = dbGetField(&datadumpaddr,DBR_USHORT,&dataDump,&ropts,&nvals,NULL);
			if(dataDump) {
				printf("Received data request\n");
				gsdfWriteData("/tmp/gsdfdata");
				dataDump = 0;
				status = dbPutField(&datadumpaddr,DBR_USHORT,&dataDump,1);		// Init to zero.
			}
		}
		status = dbPutField(&chnotfoundaddr,DBR_LONG,&numDC,1);

        // printf("GSDF C1 = %f status = %d\n",gsdf.channel_value[2],gsdf.channel_status[2]);
	// printf("Channels CONN = %d   NO_CONN = %d TIME = %ld SYNC = %ld \n",gsdf.con_chans,(gsdf.num_chans - gsdf.con_chans),gsdf.gpsTime, gsdf.epicsSync);
        //printf("GSDF C2 = %f\n",gsdf.channel_value[1]);
		fivesectimer = (fivesectimer + 1) % 50;		// Increment 5 second timer for triggering CRC checks.
		// Check file CRCs every 5 seconds.
		// DAQ and COEFF file checking was moved from skeleton.st to here RCG V2.9.
		if(!fivesectimer) {
			status = checkFileCrc(daqFile);
			if(status != daqFileCrc) {
				daqFileCrc = status;
				status = dbPutField(&daqmsgaddr,DBR_STRING,modfilemsg,1);
				logFileEntry("Detected Change to DAQ Config file.");
			}
		}
	}
	sleep(0xfffffff);
    } else
    	iocsh(NULL);
    return(0);
}
