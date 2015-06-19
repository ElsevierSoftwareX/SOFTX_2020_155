///	@file /src/epics/seq/edcu.c
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

#define EDCU_MAX_CHANS	30000
// Gloabl variables		****************************************************************************************
char timechannel[256];		///< Name of the GPS time channel for timestamping.
char reloadtimechannel[256];	///< Name of EPICS channel which contains the BURT reload requests.
struct timespec t;
char logfilename[128];
char edculogfilename[128];
unsigned char naughtyList[EDCU_MAX_CHANS][64];

// Function prototypes		****************************************************************************************
int checkFileCrc(char *);
void getSdfTime(char *);
void logFileEntry(char *);

unsigned long daqFileCrc;
typedef struct daqd_c {
	int num_chans;
	int con_chans;
	int val_events;
	int con_events;
	double channel_value[EDCU_MAX_CHANS];
	char channel_name[EDCU_MAX_CHANS][64];
	int channel_status[EDCU_MAX_CHANS];
	long gpsTime;
	long epicsSync;
} daqd_c;

daqd_c daqd_edcu1;
static struct rmIpcStr *dipc;
static char *shmDataPtr;
static struct cdsDaqNetGdsTpNum *shmTpTable;
static const int buf_size = DAQ_DCU_BLOCK_SIZE * 2;
static const int header_size = sizeof(struct rmIpcStr) + sizeof(struct cdsDaqNetGdsTpNum);
static DAQ_XFER_INFO xferInfo;
static float dataBuffer[2][EDCU_MAX_CHANS];
static int timeIndex;
static int cycleIndex;
static int symmetricom_fd = -1;
int timemarks[16] = {1000,63500,126000,188500,251000,313500,376000,438500,501000,563500,626000,688500,751000,813500,876000,938500};
int nextTrig = 0;


// End Header **********************************************************************************************************
//

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
	daqd_edcu1.channel_status[chnum] = args.op == CA_OP_CONN_UP? 0: 0xbad;
        if (args.op == CA_OP_CONN_UP) daqd_edcu1.con_chans++; else daqd_edcu1.con_chans--;
        daqd_edcu1.con_events++;
}


// **************************************************************************
void subscriptionHandler(struct event_handler_args args) {
// **************************************************************************
 	daqd_edcu1.val_events++;
        if (args.status != ECA_NORMAL) {
        	return;
        }
        if (args.type == DBR_FLOAT) {
        	float val = *((float *)args.dbr);
                daqd_edcu1.channel_value[(unsigned long)args.usr] = val;
        } else if (args.type == DBR_DOUBLE) {
        	double val1 = *((double *)args.dbr);
                daqd_edcu1.channel_value[(unsigned long)args.usr] = val1;
        }else{
		printf("Arg type unknown\n");
	}
}

int edcuClearSdf(char *pref)
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

int edcuFindUnconnChannels()
{
int ii;
int dcc = 0;

	for (ii=0;ii<daqd_edcu1.num_chans;ii++)
	{
		if(daqd_edcu1.channel_status[ii] != 0)
		{
			sprintf(naughtyList[dcc],"%s",daqd_edcu1.channel_name[ii]);
			dcc ++;
		}
	}
	return(dcc);
}
// **************************************************************************
int edcuReportUnconnChannels(char *pref, int dc, int offset)
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
void edcuCreateChanFile(char *fdir, char *edcuinifilename, char *fecid) {
// **************************************************************************
int i;
int status;
char errMsg[64];
FILE *daqfileptr;
FILE *edcuini;
FILE *edcumaster;
FILE *edcuheader;
char masterfile[64]; 
char edcuheaderfilename[64];
char line[128];
char *newname;
char edcufilename[64];
char *dcuid;

	sprintf(errMsg,"%s",fecid);
	dcuid = strtok(errMsg,"-");
	dcuid = strtok(NULL,"-");
	sprintf(masterfile, "%s%s", fdir, "edcumaster.txt");
	sprintf(edcuheaderfilename, "%s%s", fdir, "edcuheader.txt");

	// Open the master file which contains list of EDCU files to read channels from.
	edcumaster = fopen(masterfile,"r");
	if(edcumaster == NULL) {
		sprintf(errMsg,"DAQ FILE ERROR: FILE %s DOES NOT EXIST\n",masterfile);
	        logFileEntry(errMsg);
        }
	// Open the file to write the composite channel list.
	edcuini = fopen(edcuinifilename,"w");
	if(edcuini == NULL) {
		sprintf(errMsg,"DAQ FILE ERROR: FILE %s DOES NOT EXIST\n",edcuinifilename);
	        logFileEntry(errMsg);
        }

	// Write standard header into .ini file
	fprintf(edcuini,"%s","[default] \n");
	fprintf(edcuini,"%s","gain=1.00 \n");
	fprintf(edcuini,"%s","datatype=4 \n");
	fprintf(edcuini,"%s","ifoid=0 \n");
	fprintf(edcuini,"%s","slope=1 \n");
	fprintf(edcuini,"%s","acquire=3 \n");
	fprintf(edcuini,"%s","offset=0 \n");
	fprintf(edcuini,"%s","units=undef \n");
	fprintf(edcuini,"%s%s%s","dcuid=",dcuid," \n");
	fprintf(edcuini,"%s","datarate=16 \n\n");

	// Read the master file entries.
	while(fgets(line,sizeof line,edcumaster) != NULL) {
		newname = strtok(line,"\n");
		strcpy(edcufilename,fdir);
		strcat(edcufilename,newname);
		printf("File in master = %s\n",edcufilename);
		daqfileptr = fopen(edcufilename,"r");
		if(daqfileptr == NULL) {
			sprintf(errMsg,"DAQ FILE ERROR: FILE %s DOES NOT EXIST\n",edcufilename);
			logFileEntry(errMsg);
		}
		while(fgets(line,sizeof line,daqfileptr) != NULL) {
			fprintf(edcuini,"%s",line);
		}
		fclose(daqfileptr);
	}
	fclose(edcumaster);
	fclose(edcuini);
}

// **************************************************************************
void edcuCreateChanList(char *daqfilename) {
// **************************************************************************
int i;
int status;
FILE *daqfileptr;
FILE *edculog;
char errMsg[64];
// char daqfile[64];
char line[128];
char *newname;

	// sprintf(daqfile, "%s%s", fdir, "EDCU.ini");
	daqd_edcu1.num_chans = 0;
	daqfileptr = fopen(daqfilename,"r");
	if(daqfileptr == NULL) {
		sprintf(errMsg,"DAQ FILE ERROR: FILE %s DOES NOT EXIST\n",daqfilename);
	        logFileEntry(errMsg);
        }
	edculog = fopen(edculogfilename,"w");
	if(daqfileptr == NULL) {
		sprintf(errMsg,"DAQ FILE ERROR: FILE %s DOES NOT EXIST\n",edculogfilename);
	        logFileEntry(errMsg);
        }
	while(fgets(line,sizeof line,daqfileptr) != NULL) {
		fprintf(edculog,"%s",line);
		status = strlen(line);
		if(strncmp(line,"[",1) == 0 && status > 0) {
			newname = strtok(line,"]");
			// printf("status = %d New name = %s and %s\n",status,line,newname);
			newname = strtok(line,"[");
			// printf("status = %d New name = %s and %s\n",status,line,newname);
			if(strcmp(newname,"default") == 0) {
				printf("DEFAULT channel = %s\n", newname);
			} else {
				// printf("NEW channel = %s\n", newname);
				sprintf(daqd_edcu1.channel_name[daqd_edcu1.num_chans],"%s",newname);
				daqd_edcu1.num_chans ++;
			}
		}
	}
	fclose(daqfileptr);
	fclose(edculog);

	xferInfo.crcLength = 4 * daqd_edcu1.num_chans;
	printf("CRC data length = %d\n",xferInfo.crcLength);

	     chid chid1;
	ca_context_create(ca_enable_preemptive_callback);
     	for (i = 0; i < daqd_edcu1.num_chans; i++) {
	     status = ca_create_channel(daqd_edcu1.channel_name[i], connectCallback, (void *)i, 0, &chid1);
	     status = ca_create_subscription(DBR_FLOAT, 0, chid1, DBE_VALUE,
			     subscriptionHandler, (void *)i, 0);
	}
	timeIndex = 0;
}

// **************************************************************************
void edcuWriteData(int daqBlockNum, unsigned long cycle_gps_time)
// **************************************************************************
{
unsigned int crcSend;
float *daqData;
int buf_size;
int ii;

		
	buf_size = DAQ_DCU_BLOCK_SIZE*DAQ_NUM_SWING_BUFFERS;
	daqData = (float *)(shmDataPtr + (buf_size * daqBlockNum));
	for(ii=0;ii<daqd_edcu1.num_chans;ii++) {
		*daqData = (float) daqd_edcu1.channel_value[ii];
		daqData ++;
	}
	daqData = (float *)(shmDataPtr + (buf_size * daqBlockNum));
	crcSend = crc_ptr((char *)daqData, xferInfo.crcLength, 0);
	crcSend = crc_len(xferInfo.crcLength, crcSend);
	dipc->dcuId = 119;
	dipc->crc = daqFileCrc;
	dipc->dataBlockSize = xferInfo.crcLength;
	dipc->bp[daqBlockNum].cycle = daqBlockNum;
	dipc->bp[daqBlockNum].crc = crcSend;
	dipc->bp[daqBlockNum].timeSec = (unsigned int) cycle_gps_time;
	dipc->bp[daqBlockNum].timeNSec = (unsigned int)daqBlockNum;
	// DAQ expects data xmission 2 cycles later, so trigger sending of 
	// data already buffered and ready to go to keep sample time correct.
	daqBlockNum -= 2;
	if(daqBlockNum < 0) {
		daqBlockNum += 16;
	}
	dipc->cycle = daqBlockNum;	// Triggers sending of data by mx_stream.

}

// **************************************************************************
void edcuInitialize(char *shmem_fname)
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

    if(argc>=2) {
        iocsh(argv[1]);
	// printf("Executing post script commands\n");
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
	printf("LOG FILE = %s\n",logfilename);
sleep(2);
	// **********************************************
	//
	dbAddr eccaddr;
	char eccname[256]; sprintf(eccname, "%s_%s", pref, "EDCU_CHAN_CONN");			// Number of setting channels in EPICS db
	status = dbNameToAddr(eccname,&eccaddr);

	dbAddr chcntaddr;
	// char chcntname[256]; sprintf(chcntname, "%s", "X1:DAQ-FEC_54_EPICS_CHAN_CNT");	// Request to monitor all channels.
	char chcntname[256]; sprintf(chcntname, "%s_%s", pref, "EDCU_CHAN_CNT");	// Request to monitor all channels.
	status = dbNameToAddr(chcntname,&chcntaddr);		// Get Address.

	dbAddr chnotfoundaddr;
	char cnfname[256]; sprintf(cnfname, "%s_%s", pref, "EDCU_CHAN_NOCON");		// Number of channels not found.
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

// EDCU STUFF ********************************************************************************************************
	
	sprintf(edculogfilename, "%s%s", logdir, "/edcu.log");
	edcuInitialize(daqsharedmemname);
	edcuCreateChanFile(daqDir,daqFile,pref);
	edcuCreateChanList(daqFile);
	status = dbPutField(&chcntaddr,DBR_LONG,&daqd_edcu1.num_chans,1);
	int datarate = daqd_edcu1.num_chans * 4 / 1000;
	status = dbPutField(&daqbyteaddr,DBR_LONG,&datarate,1);

// Start SPECT
	daqd_edcu1.gpsTime = symm_initialize();
	daqd_edcu1.epicsSync = 0;

// End SPECT
	for (ii=0;ii<EDCU_MAX_CHANS;ii++) daqd_edcu1.channel_status[ii] = 0xbad;

	int dropout = 0;
	int numDC = 0;
	int cycle = 0;
	int numReport = 0;

	// Initialize DAQ and COEFF file CRC checksums for later compares.
	daqFileCrc = checkFileCrc(daqFile);
	printf("DAQ file CRC = %u \n",daqFileCrc);  
	coeffFileCrc = checkFileCrc(coeffFile);
	sprintf(logmsg,"%s\n%s = %u\n%s = %d","EDCU code restart","File CRC",daqFileCrc,"Chan Cnt",daqd_edcu1.num_chans);
	logFileEntry(logmsg);
	edcuClearSdf(pref);
	// Start Infinite Loop 		*******************************************************************************
	for(;;) {
		dropout = 0;
		waitGpsTrigger(daqd_edcu1.gpsTime, daqd_edcu1.epicsSync);
		edcuWriteData(daqd_edcu1.epicsSync, daqd_edcu1.gpsTime);
		// printf("EDCU %ld - %d\n",daqd_edcu1.gpsTime,daqd_edcu1.epicsSync);
		status = dbPutField(&gpstimedisplayaddr,DBR_LONG,&daqd_edcu1.gpsTime,1);		// Init to zero.
		daqd_edcu1.epicsSync = (daqd_edcu1.epicsSync + 1) % 16;
		if (daqd_edcu1.epicsSync == 0) daqd_edcu1.gpsTime ++;
		status = dbPutField(&daqbyteaddr,DBR_LONG,&datarate,1);
		int conChans = daqd_edcu1.con_chans;
		status = dbPutField(&eccaddr,DBR_LONG,&conChans,1);
		// if((conChans != daqd_edcu1.num_chans) || (numDC != 0)) numDC = edcuReportUnconnChannels(pref);
		// if(conChans != daqd_edcu1.num_chans) numDC = edcuReportUnconnChannels(pref);
		if (daqd_edcu1.epicsSync == 0) {
			status = dbGetField(&pagereqaddr,DBR_USHORT,&pageNum,&ropts,&nvals,NULL);
			// printf("Page is %d\n",pageNum);
			numDC = edcuFindUnconnChannels();
			numReport = edcuReportUnconnChannels(pref,numDC,pageNum);
			if(numReport == -1) {
				pageNum = 0;
				status = dbPutField(&pagereqaddr,DBR_USHORT,&pageNum,1);		// Init to zero.
			}
		}
		status = dbPutField(&chnotfoundaddr,DBR_LONG,&numDC,1);

        // printf("EDCU C1 = %f status = %d\n",daqd_edcu1.channel_value[2],daqd_edcu1.channel_status[2]);
	// printf("Channels CONN = %d   NO_CONN = %d TIME = %ld SYNC = %ld \n",daqd_edcu1.con_chans,(daqd_edcu1.num_chans - daqd_edcu1.con_chans),daqd_edcu1.gpsTime, daqd_edcu1.epicsSync);
        //printf("EDCU C2 = %f\n",daqd_edcu1.channel_value[1]);
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
