///	@file /src/epics/seq/main.c
///	@brief Contains required 'main' function to startup EPICS sequencers, along with supporting routines. 
///<		This code is taken from EPICS example included in the EPICS distribution and modified for LIGO use.
/* demoMain.c */
/* Author:  Marty Kraimer Date:    17MAR2000 */

/********************COPYRIGHT NOTIFICATION**********************************
This software was developed under a United States Government license
described on the COPYRIGHT_UniversityOfChicago file included as part
of this distribution.
****************************************************************************/

// TODO:
// Add command error checking, command out of range, etc.
//	- Particularly returns from functions.
//	- Add db pointer to cdTable.
//	- Add present value to cdTable.
//	- Fix filter monitor bit settings on change.

/*
 * Main program for demo sequencer
 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include "iocsh.h"
#include "dbStaticLib.h"
#include "crc.h"
#include "dbCommon.h"
#include "recSup.h"
#include "dbDefs.h"
#include "dbFldTypes.h"
#include "dbAccess.h"
#include "math.h"
#include "rcgversion.h"

#define epicsExportSharedSymbols
#include "asLib.h"
#include "asCa.h"
#include "asDbLib.h"

#define SDF_LOAD_DB_ONLY	4
#define SDF_READ_ONLY		2
#define SDF_RESET		3
#define SDF_LOAD_PARTIAL	1

#define SDF_MAX_CHANS		125000	///< Maximum number of settings, including alarm settings.
#define SDF_MAX_TSIZE		20000	///< Maximum number of EPICS settings records (No subfields).
#define SDF_ERR_DSIZE		40	///< Size of display reporting tables.
#define SDF_ERR_TSIZE		20000	///< Size of error reporting tables.

#define SDF_TABLE_DIFFS			0
#define SDF_TABLE_NOT_FOUND		1
#define SDF_TABLE_NOT_INIT		2
#define SDF_TABLE_NOT_MONITORED		3
#define SDF_TABLE_FULL			4

#if 0
/// Pointers to status record
DBENTRY  *pdbentry_status[2][10];

/// Pointers to records currently alarmed
///< Two lists of 10 records, one inputs (ai, bi)
///< the other outputs (ao, bo)
DBENTRY  *pdbentry_alarm[2][10];


/// Alarm configuration CRC checksum.
DBENTRY  *pdbentry_crc = 0;
/// Number of setpoints in alarm..
DBENTRY  *pdbentry_in_err_cnt = 0;
/// Number of readbacks in alarm..
DBENTRY  *pdbentry_out_err_cnt = 0;
#endif

/// Quick look up table for filter module switch decoding
unsigned int fltrConst[17] = {16, 64, 256, 1024, 4096, 16384,
                              65536, 262144, 1048576, 4194304,
                              0x4, 0x8, 0x4000000,0x1000000,0x1,
                              0x2000000,0x8000000
};
union Data {
	double chval;
	char strval[64];
};

/// Structure for holding BURT settings in local memory.
typedef struct CDS_CD_TABLE {
	char chname[128];
	int datatype;
	union Data data;
	int mask;
	int initialized;
	int filterswitch;
	int filterNum;
	int error;
	char errMsg[64];
	int chFlag;
} CDS_CD_TABLE;

/// Structure for creating/holding filter module switch settings.
typedef struct FILTER_TABLE {
	char fname[64];
	int swreq;
	int swmask;
	int sw[2];
	int newSet;
	int init;
	int mask;
} FILTER_TABLE;

/// Structure for table data to be presented to operators.
typedef struct SET_ERR_TABLE {
	char chname[64];
	char burtset[64];
	char liveset[64];
	char timeset[64];
	char diff[64];
	int sigNum;
	int chFlag;
	int filtNum;
	unsigned int sw[2];
} SET_ERR_TABLE;


// Gloabl variables		****************************************************************************************
int chNum = 0;			///< Total number of channels held in the local lookup table.
int chNotMon = 0;		///< Total number of channels not being monitored.
int alarmCnt = 0;		///< Total number of alarm settings loaded from a BURT file.
int chNumP = 0;			///< Total number of settings loaded from a BURT file.
int fmNum = 0;			///< Total number of filter modules found.
int fmtInit = 0;		///< Flag used to indicate that the filter module table needs to be initiialized on startup.
int chNotFound = 0;		///< Total number of channels read from BURT file which did not have a database entry.
int chNotInit = 0;		///< Total number of channels not initialized by the safe.snap BURT file.
int rderror = 0;
char timechannel[256];		///< Name of the GPS time channel for timestamping.
char reloadtimechannel[256];	///< Name of EPICS channel which contains the BURT reload requests.
struct timespec t;
char logfilename[128];

CDS_CD_TABLE cdTable[SDF_MAX_TSIZE];		///< Table used to hold EPICS database info for monitoring settings.
CDS_CD_TABLE cdTableP[SDF_MAX_CHANS];		///< Temp table filled on BURT read and set to EPICS channels.
FILTER_TABLE filterTable[1000];			///< Table for holding filter module switch settings for comparisons.
SET_ERR_TABLE setErrTable[SDF_ERR_TSIZE];	///< Table used to report settings diffs.
SET_ERR_TABLE unknownChans[SDF_ERR_TSIZE];	///< Table used to report channels not found in local database.
SET_ERR_TABLE uninitChans[SDF_ERR_TSIZE];	///< Table used to report channels not initialized by BURT safe.snap.
SET_ERR_TABLE unMonChans[SDF_MAX_TSIZE];	///< Table used to report channels not being monitored.
SET_ERR_TABLE readErrTable[SDF_ERR_TSIZE];	///< Table used to report file read errors..
SET_ERR_TABLE cdTableList[SDF_MAX_TSIZE];	///< Table used to report file read errors..

#define SDF_NUM		0
#define SDF_STR		1

// Function prototypes		****************************************************************************************
int checkFileCrc(char *);
unsigned int filtCtrlBitConvert(unsigned int);
void getSdfTime(char *);
void logFileEntry(char *);
int getEpicsSettings(int);
int writeTable2File(char *,char *,int,CDS_CD_TABLE *);
int savesdffile(int,int,char *,char *,char *,char *,char *,dbAddr,dbAddr,dbAddr); 
int createSortTableEntries(int,int,char *,int *);
int reportSetErrors(char *,int,SET_ERR_TABLE *,int);
int spChecker(int,SET_ERR_TABLE *,int,char *,int,int *);
void newfilterstats(int);
int writeEpicsDb(int,CDS_CD_TABLE *,int);
int readConfig( char *,char *,int,char *);
void dbDumpRecords(DBBASE *);
int parseLine(char *,char *,char *,char *,char *,char *,char *);
int modifyTable(int,SET_ERR_TABLE *);
void clearTableSelections(int,SET_ERR_TABLE *, int *);
void setAllTableSelections(int,SET_ERR_TABLE *, int *,int);
void decodeChangeSelect(int, int, int, SET_ERR_TABLE *, int *);
int appendAlarms2File(char *,char *,char *);

// End Header **********************************************************************************************************
//

/// Common routine to parse lines read from BURT/SDF files..
///	@param[in] *s	Pointer to line of chars to parse.
///	@param[out] str1 thru str6 	String pointers to return individual words from line.
///	@return wc	Number of words in the line.
int parseLine(char *s, char *str1, char *str2, char *str3, char *str4, char *str5,char *str6)
{
int wc = -1;
int inQuotes = 0;
int lastwasspace = 1;
int ii,jj,kk;
int mychar = 0;
char psd[10][64];
const char strval0[] = "0";
const char strval1[] = "1";
#define IS_A_ALPHA_NUM 	0
#define IS_A_SPACE 	1
#define IS_A_QUOTE	2

	while (*s != 0 && *s != 10) {
		mychar = IS_A_ALPHA_NUM;
		if (*s == '\t') *s = ' ';
		if (*s == ' ') mychar = IS_A_SPACE;
		if (*s == '\"') mychar = IS_A_QUOTE;
		switch(mychar) {
			case IS_A_ALPHA_NUM:
				if(lastwasspace) {
					wc ++;
					psd[wc][0] = '\0';
					}
				strncat(psd[wc],s,1);
				lastwasspace = 0;
				break;
			case IS_A_SPACE:
				if(inQuotes == 1) {
					strncat(psd[wc],s,1);
				} else {
					lastwasspace = 1;
				}
				break;
			case IS_A_QUOTE:
				inQuotes += 1;
				if(inQuotes == 2) lastwasspace = 1;
				break;
		}
		s ++;
	}
	wc ++;
	sprintf(str1,"%s",psd[0]);
	if(inQuotes > 2) return(-1);
	if(wc > 4 && inQuotes < 1) {
		if(strcmp(psd[(wc-1)],strval0) == 0 || strcmp(psd[(wc-1)],strval1) == 0) kk = wc - 1;
		else kk = wc;
		jj = 0;
		for(ii=3;ii<kk;ii++) {
			strcat(psd[2]," ");
			strcat(psd[2],psd[ii]);
			jj ++;
		}
		if(strcmp(psd[(wc-1)],strval0) == 0 || strcmp(psd[(wc-1)],strval1) == 0) sprintf(psd[3],"%s",psd[(wc - 1)]);
		wc -= jj;
		// printf("wc for %s is %d = %s \t with monitor %s\n",psd[0],wc,psd[2],psd[3]);
	} 
	if(wc < 4) sprintf(psd[3],"%s","0");
	sprintf(str2,"%s",psd[1]);
	sprintf(str3,"%s",psd[2]);
	sprintf(str4,"%s",psd[3]);
	// printf("WC = %d\n%s \t%s\t%s\t%s\t%s\n",wc,psd[0],psd[1],psd[2],psd[3],psd[4]);
	return(wc);
}

/// Common routine to clear all entries for table changes..
///	@param[in] numEntries		Number of entries in the table.	
///	@param[in] dcsErrtable 		Pointer to table
///	@param[out] sc 			Pointer to change counters.
void clearTableSelections(int numEntries,SET_ERR_TABLE *dcsErrTable, int sc[])
{
int ii;
	for(ii=0;ii<3;ii++) sc[ii] = 0;
	for(ii=0;ii<numEntries;ii++) dcsErrTable[ii].chFlag = 0;
	for(ii=0;ii<numEntries;ii++) dcsErrTable[ii].filtNum = -1;
}


/// Common routine to set all entries for table changes..
///	@param[in] numEntries		Number of entries in the table.	
///	@param[in] dcsErrtable 		Pointer to table
///	@param[out] sc 			Pointer to change counters.
///	@param[in] selectOp 		Present selection.
void setAllTableSelections(int numEntries,SET_ERR_TABLE *dcsErrTable, int sc[],int selectOpt)
{
int ii;
	switch(selectOpt) {
		case 1:
			sc[0] = 0;
			for(ii=0;ii<numEntries;ii++) {
				dcsErrTable[ii].chFlag |= 2;
				sc[0] ++;
				if(dcsErrTable[ii].chFlag & 4) {
					dcsErrTable[ii].chFlag &= ~(4);
					sc[1] --;
				}
			}
			break;
		case 2:
			sc[1] = 0;
			for(ii=0;ii<numEntries;ii++) {
				dcsErrTable[ii].chFlag |= 4;
				sc[1] ++;
				if(dcsErrTable[ii].chFlag & 2) {
					dcsErrTable[ii].chFlag &= ~(2);
					sc[0] --;
				}
			}
			break;
		case 3:
			sc[2] = 0;
			for(ii=0;ii<numEntries;ii++) {
				dcsErrTable[ii].chFlag |= 8;
				sc[2] ++;
			}
			break;
		default:
			break;
	}
}

/// Common routine to set/unset individual entries for table changes..
///	@param[in] selNum		Number providing line and column of table entry to change.
///	@param[in] page			Number providing the page of table to change.
///	@param[in] totalItems		Number items selected for change.
///	@param[in] dcsErrtable 		Pointer to table
///	@param[out] selectCounter 	Pointer to change counters.
void decodeChangeSelect(int selNum, int page, int totalItems, SET_ERR_TABLE *dcsErrTable, int selectCounter[])
{
int selectBit;
int selectLine;

	selectBit = selNum / 100;
	selectLine = selNum % 100 - 1;
	selNum += page * SDF_ERR_DSIZE;
	selectLine += page * SDF_ERR_DSIZE;
	if(selectLine < totalItems) {
		switch (selectBit) {
			case 0:
				dcsErrTable[selectLine].chFlag ^= 2;
				if(dcsErrTable[selectLine].chFlag & 2) selectCounter[0] ++;
				else selectCounter[0] --;
				if(dcsErrTable[selectLine].chFlag & 4) {
					dcsErrTable[selectLine].chFlag &= ~(4);
					selectCounter[1] --;
				}
				break;
			case 1:
				dcsErrTable[selectLine].chFlag ^= 4;
				if(dcsErrTable[selectLine].chFlag & 4) selectCounter[1] ++;
				else selectCounter[1] --;
				if(dcsErrTable[selectLine].chFlag & 2) {
					dcsErrTable[selectLine].chFlag &= ~(2);
					selectCounter[0] --;
				}
				break;
			case 2:
				dcsErrTable[selectLine].chFlag ^= 8;
				if(dcsErrTable[selectLine].chFlag & 8) selectCounter[2] ++;
				else selectCounter[2] --;
				break;
			default:
				break;
		}
	}
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

/// Quick convert of filter switch settings to match SWMASK
///	@param[in] v	UINT32 representation of filter module switch setting.
///	@return Input value converted to 16 bit representation of filter switch settings.
unsigned int filtCtrlBitConvert(unsigned int v) {
        unsigned int val = 0;
        int i;
        for (i = 0; i < 17; i++) {
                if (v & fltrConst[i]) val |= 1<<i;
        }
        return val;
}
/// Quick convert of filter switch SWSTAT to SWSTR value
///	@param[in] v			SWSTAT representation of switch setting.	
///	@param[out] filterstring	Returned string representation of switch setting.	
///	@return				Always zero at this point.
int filtStrBitConvert(int stype, unsigned int v, char *filterstring) {
unsigned int val[5],jj;
unsigned int x;
char bitDefShort[16][8] = {"IN,","OF,","1,","2,",
			   "3,","4,","5,","6,",
			   "7,","8,","9,","10,",
			   "LT,","OT,","DC,","HD,"};
char bitDefLong[16][8] = {"IN,","OF,","F1,","F2,",
			   "F3,","F4,","F5,","F6,",
			   "F7,","F8,","F9,","F10,",
			   "LT,","OT,","DC,","HD,"};

	val[0] = (v >> 10) & 3;		//In and Offset
	val[1] = (v & 1023) << 2;		// F1 thru F10
	val[2] = (v & 0x2000) >> 1;		// Limit
	val[3] = (v & 0x1000) << 1;		// Output
	val[4] = (v & 0x18000) >> 1;		// Dec and Hold
	x = val[0] + val[1] + val[2] + val[3] + val[4];
	sprintf(filterstring,"%s","");
	for(jj=0;jj<16;jj++) {
		if(x & 1 && stype == 0) strcat(filterstring,bitDefShort[jj]);
		if(x & 1 && stype == 1) strcat(filterstring,bitDefLong[jj]);
		x = x >> 1;
	}
	filterstring[strlen(filterstring) - 1] = 0;
	// strcat(filterstring,"\0");
        return(0);
}

/// Routine to check filter switch settings and provide info on switches not set as requested.
/// @param[in] fcount	Number of filters in the control model.
/// @param[in] monitorAll	Global monitoring flag.
/// @return	Number of errant switch settings found.
int checkFilterSwitches(int fcount, SET_ERR_TABLE setErrTable[], int monitorAll, int displayall, int wcflag, char *wcstr, int *diffcntr)
{
unsigned int refVal;
unsigned int presentVal;
unsigned int x,y;
int ii,jj;
int errCnt = 0;
char swname[3][64];
char tmpname[64];
time_t mtime;
char localtimestring[256];
char swstrE[64];
char swstrB[64];
char swstrD[64];
struct buffer {
	DBRstatus
	DBRtime
	unsigned int rval;
	}buffer[2];
long options = DBR_STATUS|DBR_TIME;
dbAddr paddr;
dbAddr paddr1;
dbAddr paddr2;
long nvals = 1;
long status;
char *ret;
	for(ii=0;ii<fcount;ii++)
	{
		bzero(swname[0],strlen(swname[0]));
		bzero(swname[1],strlen(swname[1]));
		strcpy(swname[0],filterTable[ii].fname);
		strcat(swname[0],"SW1S");
		strcpy(swname[1],filterTable[ii].fname);
		strcat(swname[1],"SW2S");
		sprintf(swname[2],"%s",filterTable[ii].fname);
		strcat(swname[2],"SWSTR");
		status = dbNameToAddr(swname[0],&paddr);
		status = dbGetField(&paddr,DBR_LONG,&buffer[0],&options,&nvals,NULL);
		status = dbNameToAddr(swname[1],&paddr1);
		status = dbGetField(&paddr1,DBR_LONG,&buffer[1],&options,&nvals,NULL);
		for(jj=0;jj<2;jj++) {
			if(buffer[jj].rval > 0xffff || buffer[jj].rval < 0)	// Switch setting overrange
			{
				sprintf(setErrTable[errCnt].chname,"%s", swname[jj]);
				sprintf(setErrTable[errCnt].burtset, "%s", " ");
				sprintf(setErrTable[errCnt].liveset, "0x%x", buffer[jj].rval);
				sprintf(setErrTable[errCnt].diff, "%s", "OVERRANGE");
				setErrTable[errCnt].sigNum = filterTable[ii].sw[jj];
				mtime = buffer[jj].time.secPastEpoch + POSIX_TIME_AT_EPICS_EPOCH;
				strcpy(localtimestring, ctime(&mtime));
				localtimestring[strlen(localtimestring) - 1] = 0;
				sprintf(setErrTable[errCnt].timeset, "%s", localtimestring);
				buffer[jj].rval &= 0xffff;
				errCnt ++;
			}
		}
		presentVal = buffer[0].rval + (buffer[1].rval << 16);
		refVal = filtCtrlBitConvert(presentVal);
		filtStrBitConvert(0,refVal,swstrE);
		x = refVal;
		status = dbNameToAddr(swname[2],&paddr2);
		status = dbPutField(&paddr2,DBR_STRING,swstrE,1);
		setErrTable[errCnt].filtNum = -1;
		if((refVal != filterTable[ii].swreq && errCnt < SDF_ERR_TSIZE && (filterTable[ii].mask || monitorAll) && filterTable[ii].init) )
			*diffcntr += 1;
		if((refVal != filterTable[ii].swreq && errCnt < SDF_ERR_TSIZE && (filterTable[ii].mask || monitorAll) && filterTable[ii].init) || displayall)
		{
			filtStrBitConvert(1,refVal,swstrE);
			filtStrBitConvert(1,filterTable[ii].swreq,swstrB);
			x = refVal ^ filterTable[ii].swreq;;
			filtStrBitConvert(1,x,swstrD);
			bzero(tmpname,sizeof(tmpname));
			strncpy(tmpname,filterTable[ii].fname,(strlen(filterTable[ii].fname)-1));

			if(!wcflag || (wcflag && (ret = strstr(tmpname,wcstr) != NULL))) {
			sprintf(setErrTable[errCnt].chname,"%s", tmpname);
			sprintf(setErrTable[errCnt].burtset, "%s", swstrB);
			sprintf(setErrTable[errCnt].liveset, "%s", swstrE);
			sprintf(setErrTable[errCnt].diff, "%s", swstrD);
			setErrTable[errCnt].sigNum = filterTable[ii].sw[0] + (filterTable[ii].sw[1] * SDF_MAX_TSIZE);
			setErrTable[errCnt].filtNum = ii;
			setErrTable[errCnt].sw[0] = buffer[0].rval;;
			setErrTable[errCnt].sw[1] = buffer[1].rval;;
			if(filterTable[ii].mask) setErrTable[errCnt].chFlag |= 0x1;
			else setErrTable[errCnt].chFlag &= 0xe;

			mtime = buffer[0].time.secPastEpoch + POSIX_TIME_AT_EPICS_EPOCH;
			strcpy(localtimestring, ctime(&mtime));
			localtimestring[strlen(localtimestring) - 1] = 0;
			sprintf(setErrTable[errCnt].timeset, "%s", localtimestring);
			errCnt ++;
			}
		}
	}
	return(errCnt);

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


/// Function to read all settings, listed in main table, from the EPICS database.
///	@param[in] numchans The number of channels listed in the main table.
int getEpicsSettings(int numchans)
{
dbAddr geaddr;
int ii;
long status;
long statusR;
double dval;
char sval[64];
int chcount = 0;
int nvals = 1;

	for(ii=0;ii<numchans;ii++)
	{
		// Load main table settings into temp (cdTableP)
		sprintf(cdTableP[ii].chname,"%s",cdTable[ii].chname);
		cdTableP[ii].datatype = cdTable[ii].datatype;
		cdTableP[ii].mask = cdTable[ii].mask;
		cdTableP[ii].initialized = cdTable[ii].initialized;
		// Find address of channel
		status = dbNameToAddr(cdTableP[ii].chname,&geaddr);
		if(!status) {
			if(cdTableP[ii].datatype == SDF_NUM)
			{
				statusR = dbGetField(&geaddr,DBR_DOUBLE,&dval,NULL,&nvals,NULL);
				if(!statusR && cdTable[ii].filterswitch) cdTableP[ii].data.chval = (int)dval & 0xffff;
				if(!statusR && !cdTable[ii].filterswitch) cdTableP[ii].data.chval = dval;
			} else {
				statusR = dbGetField(&geaddr,DBR_STRING,&sval,NULL,&nvals,NULL);
				if(!statusR) sprintf(cdTableP[ii].data.strval,"%s",sval);
			}
			chcount ++;
		}
	}

}


// writeTable2File ftype options:
#define SDF_WITH_INIT_FLAG	0
#define SDF_FILE_PARAMS_ONLY	1
#define SDF_FILE_BURT_ONLY	2
/// Common routine for saving table data to BURT files.
int writeTable2File(char *burtdir,
		    char *filename, 		///< Name of file to write
		    int ftype, 			///< Type of file to write
		    CDS_CD_TABLE myTable[])	///< Table to be written.
{
        int ii;
        FILE *csFile;
        char filemsg[128];
	char timestring[128];
    // Write out local monitoring table as snap file.
        errno=0;
        csFile = fopen(filename,"w");
        if (csFile == NULL)
        {
            sprintf(filemsg,"ERROR Failed to open %s - %s",filename,strerror(errno));
            logFileEntry(filemsg);
	    return(-1);
        }
	// Write BURT header
	getSdfTime(timestring);
	fprintf(csFile,"%s\n","--- Start BURT header");
	fprintf(csFile,"%s%s\n","Time:      ",timestring);
	fprintf(csFile,"%s\n","Login ID: controls ()");
	fprintf(csFile,"%s\n","Eff  UID: 1001 ");
	fprintf(csFile,"%s\n","Group ID: 1001 ");
	fprintf(csFile,"%s\n","Keywords:");
	fprintf(csFile,"%s\n","Comments:");
	fprintf(csFile,"%s\n","Type:     Absolute  ");
	fprintf(csFile,"%s%s\n","Directory: ",burtdir);
	fprintf(csFile,"%s\n","Req File: autoBurt.req ");
	fprintf(csFile,"%s\n","--- End BURT header");

	for(ii=0;ii<chNum;ii++)
	{
		if(myTable[ii].datatype == SDF_STR && strlen(myTable[ii].data.strval) < 1)
			sprintf(myTable[ii].data.strval,"%s"," ");
		switch(ftype)
		{
		   case SDF_WITH_INIT_FLAG:
			if(myTable[ii].datatype == SDF_NUM)
				fprintf(csFile,"%s %d %.15e %d %d\n",myTable[ii].chname,1,myTable[ii].data.chval,myTable[ii].mask,myTable[ii].initialized);
			else
				fprintf(csFile,"%s %d \"%s\" %d %d\n",myTable[ii].chname,1,myTable[ii].data.strval,myTable[ii].mask,myTable[ii].initialized);
			break;
		   case SDF_FILE_PARAMS_ONLY:
			if(myTable[ii].initialized) {
				if(myTable[ii].datatype == SDF_NUM)
					fprintf(csFile,"%s %d %.15e %d\n",myTable[ii].chname,1,myTable[ii].data.chval,myTable[ii].mask);
				else
					fprintf(csFile,"%s %d \"%s\" %d\n",myTable[ii].chname,1,myTable[ii].data.strval,myTable[ii].mask);
			}
			break;
		   case SDF_FILE_BURT_ONLY:
			if(myTable[ii].datatype == SDF_NUM)
				fprintf(csFile,"%s %d %.15e\n",myTable[ii].chname,1,myTable[ii].data.chval);
			else
				fprintf(csFile,"%s %d \"%s\" \n",myTable[ii].chname,1,myTable[ii].data.strval);
			break;
		   default:
			break;
		}
	}
	fclose(csFile);
// Make sure group-write bit is set
        struct stat filestat;
        int res = stat(filename, &filestat);
        mode_t filemod = filestat.st_mode | S_IWGRP;
        errno=0;
        int setstat = chmod(filename,filemod);
        if (setstat) 
        {
            sprintf(filemsg,"ERROR Unable to set group-write on %s - %s",filename,strerror(errno));
            logFileEntry(filemsg);
            return(-2);
        }
        return(0);
}

/// Function to append alarm settings to saved files..
///	@param[in] sdfdir 	Associated model BURT directory.
///	@param[in] sdffile	Name of the file being saved.
///	@param[in] currentload	Base file name of the associated alarms.snap file..
int appendAlarms2File(
		char *sdfdir,
		char *sdffile,
		char *currentload
		)
{
char sdffilename[256];
char alarmfilename[256];
FILE *cdf;
FILE *adf;
char line[128];
char errMsg[128];
int lderror = 0;
	sprintf(sdffilename,"%s%s.snap",sdfdir,sdffile);
	sprintf(alarmfilename,"%s%s_alarms.snap",sdfdir,currentload);
	// printf("sdffile = %s  \nalarmfile = %s\n",sdffilename,alarmfilename);
	adf = fopen(alarmfilename,"r");
	if(adf == NULL) return(-1);
		cdf = fopen(sdffilename,"a");
		if(cdf == NULL) {
			sprintf(errMsg,"New SDF request ERROR: FILE %s DOES NOT EXIST\n",sdffilename);
			logFileEntry(errMsg);
			lderror = 4;
			return(lderror);
		}
		while(fgets(line,sizeof line,adf) != NULL)
		{
			fprintf(cdf,"%s",line);
		}
		fclose(cdf);
	fclose(adf);
	return(0);
}
// Savetype
#define SAVE_TABLE_AS_SDF	1
#define SAVE_EPICS_AS_SDF	2
// Saveopts
#define SAVE_TIME_NOW		1
#define SAVE_OVERWRITE		2
#define SAVE_AS			3
#define SAVE_BACKUP		4
#define SAVE_OVERWRITE_TABLE	5

/// Routine used to decode and handle BURT save requests.
int savesdffile(int saveType, 		///< Save file format definition.
		int saveOpts, 		///< Save file options.
		char *sdfdir, 		///< Directory to save file in.
		char *model, 		///< Name of the model used to build file name.
		char *currentfile, 	///< Name of file last read (Used if option is to overwrite).
		char *saveasfile,	///< Name of file to be saved.
		char *currentload,	///< Name of file, less directory info.
		dbAddr sfaddr,		///< Address of EPICS channel to write save file name.
		dbAddr staddr,		///< Address of EPICS channel to write save file time.
		dbAddr rladdr)
{
char filename[256];
char ftype[16];
int status;
char filemsg[128];
char timestring[64];
char shortfilename[64];

	time_t now = time(NULL);
	struct tm *mytime  = localtime(&now);

	switch(saveOpts)
	{
		case SAVE_TIME_NOW:
			sprintf(filename,"%s%s_%d%02d%02d_%02d%02d%02d.snap", sdfdir,currentload,
			(mytime->tm_year - 100),  (mytime->tm_mon + 1),  mytime->tm_mday,  mytime->tm_hour,  mytime->tm_min,  mytime->tm_sec);
			sprintf(shortfilename,"%s_%d%02d%02d_%02d%02d%02d", currentload,
			(mytime->tm_year - 100),  (mytime->tm_mon + 1),  mytime->tm_mday,  mytime->tm_hour,  mytime->tm_min,  mytime->tm_sec);
			// printf("File to save is TIME NOW: %s\n",filename);
			break;
		case SAVE_OVERWRITE:
			sprintf(filename,"%s",currentfile);
			sprintf(shortfilename,"%s",currentload);
			// printf("File to save is OVERWRITE: %s\n",filename);
			break;
		case SAVE_BACKUP:
			sprintf(filename,"%s%s_%d%02d%02d_%02d%02d%02d.snap",sdfdir,currentload,
			(mytime->tm_year - 100),  (mytime->tm_mon + 1),  mytime->tm_mday,  mytime->tm_hour,  mytime->tm_min,  mytime->tm_sec);
			sprintf(shortfilename,"%s",currentload);
			// printf("File to save is BACKUP: %s\n",filename);
			break;
		case SAVE_OVERWRITE_TABLE:
			sprintf(filename,"%s%s.snap",sdfdir,currentload);
			sprintf(shortfilename,"%s",currentload);
			// printf("File to save is BACKUP OVERWRITE: %s\n",filename);
			break;
		case SAVE_AS:
			sprintf(filename,"%s%s.snap",sdfdir,saveasfile);
			sprintf(shortfilename,"%s",saveasfile);
			// printf("File to save is SAVE_AS: %s\n",filename);
			break;

		default:
			return(-1);
	}
	// SAVE THE DATA TO FILE **********************************************************************
	switch(saveType)
	{
		case SAVE_TABLE_AS_SDF:
			// printf("Save table as sdf\n");
			status = writeTable2File(sdfdir,filename,SDF_FILE_PARAMS_ONLY,cdTable);
			if(status != 0) {
                            sprintf(filemsg,"FAILED FILE SAVE %s",filename);
			    logFileEntry(filemsg);
                            return(-2);
			}
			status = appendAlarms2File(sdfdir,shortfilename,currentload);
			if(status != 0) {
                            sprintf(filemsg,"FAILED To Append Alarms -  %s",currentload);
			    logFileEntry(filemsg);
                            return(-2);
			}
		        sprintf(filemsg,"Save TABLE as SDF: %s",filename);
                        break;
		case SAVE_EPICS_AS_SDF:
			// printf("Save epics as sdf\n");
			status = getEpicsSettings(chNum);
			status = writeTable2File(sdfdir,filename,SDF_FILE_PARAMS_ONLY,cdTableP);
			if(status != 0) {
                            sprintf(filemsg,"FAILED EPICS SAVE %s",filename);
			    logFileEntry(filemsg);
                            return(-2);
			}
			sprintf(filemsg,"Save EPICS as SDF: %s",filename);
                        break;
		default:
			sprintf(filemsg,"BAD SAVE REQUEST %s",filename);
			logFileEntry(filemsg);
			return(-1);
	}
	logFileEntry(filemsg);
	getSdfTime(timestring);
	// printf(" Time of save = %s\n",timestring);
	status = dbPutField(&sfaddr,DBR_STRING,shortfilename,1);
	status = dbPutField(&staddr,DBR_STRING,timestring,1);
	status = dbPutField(&rladdr,DBR_STRING,timestring,1);
	return(0);
}


#if 0
// Routine to change ASG, thereby changing record locking
// Left here for possible future use to lock EPICS settings.
void resetASG(char *name, int lock) {
    
    DBENTRY  *pdbentry;
    dbCommon *precord;
    long status;
    char t[256];

    pdbentry = dbAllocEntry(*iocshPpdbbase);
    status = dbFindRecord(pdbentry, name);
    precord = pdbentry->precnode->precord;
    if(lock) sprintf(t, "DEFAULT");
    else sprintf(t, "MANUAL");
    status = asChangeGroup((ASMEMBERPVT *)&precord->asp,t);
    // printf("ASG = %s\n",t);
    dbFreeEntry(pdbentry);
}
#endif

/// Routine used to create local tables for reporting uninitialize and not monitored channels on request.
int createSortTableEntries(int numEntries,int wcval,char *wcstring,int *noInit)
{
int ii,jj;
int notMon = 0;
int ret = 0;
int lna = 0;
int lnb = 0;
char tmpname[64];
time_t mtime;
char localtimestring[256];
long nvals = 1;
char liveset[64];


	chNotInit = 0;
	chNotMon = 0;


	// Fill uninit and unmon tables.
//	printf("sort table = %d string %s\n",wcval,wcstring);
	for(ii=0;ii<fmNum;ii++) {
		if(!filterTable[ii].init) chNotInit += 1;
		if(filterTable[ii].init && !filterTable[ii].mask) chNotMon += 1;
		if(wcval  && (ret = strstr(filterTable[ii].fname,wcstring) == NULL)) {
			continue;
		}
		bzero(tmpname,sizeof(tmpname));
		strncpy(tmpname,filterTable[ii].fname,(strlen(filterTable[ii].fname)-1));
		if(!filterTable[ii].init) {
			sprintf(uninitChans[lna].chname,"%s",tmpname);
			sprintf(uninitChans[lna].liveset,"%s",cdTableList[ii].liveset);
			uninitChans[lna].sw[0] = cdTableList[ii].sw[0];
			uninitChans[lna].sw[1] = cdTableList[ii].sw[1];
			uninitChans[lna].sigNum = filterTable[ii].sw[0] + (filterTable[ii].sw[1] * SDF_MAX_TSIZE);
			uninitChans[lna].filtNum = ii;
			lna ++;
		}
		if(filterTable[ii].init && !filterTable[ii].mask) {
			sprintf(unMonChans[lnb].chname,"%s",tmpname);
			unMonChans[lnb].filtNum = ii;
			unMonChans[lnb].sigNum = filterTable[ii].sw[0] + (filterTable[ii].sw[1] * SDF_MAX_TSIZE);
			lnb ++;
		}
	}
	for(jj=0;jj<numEntries;jj++)
	{
		if(cdTable[jj].filterswitch) continue;
		if(!cdTable[jj].initialized) chNotInit += 1;
		if(cdTable[jj].initialized && !cdTable[jj].mask) chNotMon += 1;
		if(wcval  && (ret = strstr(cdTable[jj].chname,wcstring) == NULL)) {
			continue;
		}
		if(!cdTable[jj].initialized && !cdTable[jj].filterswitch) {
			// printf("Chan %s not init %d %d %d\n",cdTable[jj].chname,cdTable[jj].initialized,jj,numEntries);
			if(lna < SDF_ERR_TSIZE) {
				sprintf(uninitChans[lna].chname,"%s",cdTable[jj].chname);
				if(cdTable[jj].datatype == SDF_NUM) {
					sprintf(liveset,"%.10lf",cdTableP[jj].data.chval);
				} else {
					sprintf(liveset,"%s",cdTableP[jj].data.strval);
				}
				sprintf(uninitChans[lna].liveset, "%s", liveset);
				sprintf(uninitChans[lna].timeset,"%s"," ");
				sprintf(uninitChans[lna].diff,"%s"," ");
				lna ++;
			}
		}

		if(cdTable[jj].initialized && !cdTable[jj].mask && !cdTable[jj].filterswitch) {
			if(lnb < SDF_ERR_TSIZE) {
				sprintf(unMonChans[lnb].chname,"%s",cdTable[jj].chname);
				if(cdTable[jj].datatype == SDF_NUM)
				{
					sprintf(unMonChans[lnb].burtset,"%.10lf",cdTable[jj].data.chval);
					unMonChans[lnb].filtNum = -1;
				} else {
					sprintf(unMonChans[lnb].burtset,"%s",cdTable[jj].data.strval);
				}
				sprintf(unMonChans[lnb].liveset,"%s"," ");
				sprintf(unMonChans[lnb].timeset,"%s"," ");
				sprintf(unMonChans[lnb].diff,"%s"," ");
			}
			lnb ++;
		}
	}
	// Clear out the uninit tables.
	for(jj=lna;jj<(lna + 50);jj++)
	{
		sprintf(uninitChans[jj].chname,"%s"," ");
		sprintf(uninitChans[jj].burtset,"%s"," ");
		sprintf(uninitChans[jj].liveset,"%s"," ");
		sprintf(uninitChans[jj].timeset,"%s"," ");
		sprintf(uninitChans[jj].diff,"%s"," ");
	}
	// Clear out the unmon tables.
	for(jj=lnb;jj<(lnb + 50);jj++)
	{
		sprintf(unMonChans[jj].chname,"%s"," ");
		sprintf(unMonChans[jj].burtset,"%s"," ");
		sprintf(unMonChans[jj].liveset,"%s"," ");
		sprintf(unMonChans[jj].timeset,"%s"," ");
		sprintf(unMonChans[jj].diff,"%s"," ");
	}
	*noInit = lna;
	return(lnb);
}

/// Common routine to load monitoring tables into EPICS channels for MEDM screen.
int reportSetErrors(char *pref,			///< Channel name prefix from EPICS environment. 
		     int numEntries, 			///< Number of entries in table to be reported.
		     SET_ERR_TABLE setErrTable[],	///< Which table to report to EPICS channels.
		     int page)				///< Which page of 40 to display.
{

int ii;
dbAddr saddr;
dbAddr baddr;
dbAddr maddr;
dbAddr taddr;
dbAddr daddr;
dbAddr laddr;
char s[64];
char s1[64];
char s2[64];
char s3[64];
char s4[64];
char sl[64];
long status;
char clearString[62] = "                       ";
int flength = 62;
int rc = 0;
int myindex = 0;
int numDisp = 0;
int lineNum = 0;
int mypage = 0;
int lineCtr = 0;
int zero = 0;


	// Get the page number to display
	mypage = page;
	// Calculat start index to the diff table.
	myindex = page *  SDF_ERR_DSIZE;
	if(myindex == numEntries && numEntries > 0) {
		mypage --;
		myindex = mypage *  SDF_ERR_DSIZE;
	}
	// If index is > number of entries in the table, then page back.
        if(myindex > numEntries) {
		mypage = numEntries / SDF_ERR_DSIZE;
		myindex = mypage *  SDF_ERR_DSIZE;
        }
	// Set the stop index to the diff table.
	rc = myindex + SDF_ERR_DSIZE;
	// If stop index beyond last diff table entry, set it to last entry.
        if(rc > numEntries) rc = numEntries;
	numDisp = rc - myindex;

	// Fill in table entries.
	for(ii=myindex;ii<rc;ii++)
	{
		sprintf(s, "%s_%s_STAT%d", pref,"SDF_SP", lineNum);
		status = dbNameToAddr(s,&saddr);
		status = dbPutField(&saddr,DBR_UCHAR,&setErrTable[ii].chname,flength);

		sprintf(s1, "%s_%s_STAT%d_BURT", pref,"SDF_SP", lineNum);
		status = dbNameToAddr(s1,&baddr);
		status = dbPutField(&baddr,DBR_UCHAR,&setErrTable[ii].burtset,flength);

		sprintf(s2, "%s_%s_STAT%d_LIVE", pref,"SDF_SP", lineNum);
		status = dbNameToAddr(s2,&maddr);
		status = dbPutField(&maddr,DBR_UCHAR,&setErrTable[ii].liveset,flength);

		sprintf(s3, "%s_%s_STAT%d_TIME", pref,"SDF_SP", lineNum);
		status = dbNameToAddr(s3,&taddr);
		status = dbPutField(&taddr,DBR_UCHAR,&setErrTable[ii].timeset,flength);

		sprintf(s4, "%s_%s_STAT%d_DIFF", pref,"SDF_SP", lineNum);
		status = dbNameToAddr(s4,&daddr);
		status = dbPutField(&daddr,DBR_UCHAR,&setErrTable[ii].diff,flength);

		sprintf(sl, "%s_SDF_BITS_%d", pref, lineNum);
                status = dbNameToAddr(sl,&daddr);
                status = dbPutField(&daddr,DBR_ULONG,&setErrTable[ii].chFlag,1);

		sprintf(sl, "%s_SDF_LINE_%d", pref, lineNum);
		lineNum ++;
		lineCtr = ii + 1;;
                status = dbNameToAddr(sl,&laddr);
                status = dbPutField(&laddr,DBR_LONG,&lineCtr,1);
	}

	// Clear out empty table entries.
	for(ii=numDisp;ii<40;ii++)
	{
		sprintf(s, "%s_%s_STAT%d", pref,"SDF_SP", ii);
		status = dbNameToAddr(s,&saddr);
		status = dbPutField(&saddr,DBR_UCHAR,clearString,flength);

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

		sprintf(sl, "%s_SDF_BITS_%d", pref, ii);
                status = dbNameToAddr(sl,&daddr);
                status = dbPutField(&daddr,DBR_ULONG,&zero,1);

		lineCtr ++;
		sprintf(sl, "%s_SDF_LINE_%d", pref, ii);
		status = dbNameToAddr(sl,&laddr);
		status = dbPutField(&laddr,DBR_LONG,&lineCtr,1);
		
	}
	// Return the number of the display page.
	return(mypage);
}

// This function checks that present setpoints match those set by BURT if the channel is marked by a mask
// setting in the BURT file indicating that this channel should be monitored. .
// If settings don't match, then this is reported to the Guardian records in the form:
//	- Signal Name
//	- BURT Setting
//	- Present Value
//	- Time the present setting was applied.
/// Setpoint monitoring routine.
int spChecker(int monitorAll, SET_ERR_TABLE setErrTable[],int wcVal, char *wcstring, int listAll, int *totalDiffs)
{
   	int errCntr = 0;
	dbAddr paddr;
	long status;
	long ropts = 0;
	long nvals = 1;
	int ii;
	struct buffer {
		DBRstatus
		DBRtime
		double rval;
		}buffer;
	struct strbuffer {
		DBRstatus
		DBRtime
		char sval[128];;
		}strbuffer;
	long options = DBR_STATUS|DBR_TIME;
	time_t mtime;
	char localtimestring[256];
	int localErr = 0;
	char liveset[64];
	char burtset[64];
	char diffB2L[64];
	char swName[64];
	double sdfdiff = 0.0;
	char *ret;
	int filtDiffs;

	// Check filter switch settings first
	     errCntr = checkFilterSwitches(fmNum,setErrTable,monitorAll,listAll,wcVal,wcstring,&filtDiffs);
	     *totalDiffs = filtDiffs;
	     if(chNum) {
		for(ii=0;ii<chNum;ii++) {
			if((errCntr < SDF_ERR_TSIZE) && 		// Within table max size
			  ((cdTable[ii].mask != 0) || (monitorAll)) && 	// Channel is to be monitored
			   cdTable[ii].initialized && 			// Channel was set in BURT
			   cdTable[ii].filterswitch == 0 ||		// Not a filter switch channel
			   (listAll && cdTable[ii].filterswitch == 0))
			{
				localErr = 0;
				// Find address of channel
				status = dbNameToAddr(cdTable[ii].chname,&paddr);
				// If this is a digital data type, then get as double.
				if(cdTable[ii].datatype == SDF_NUM)
				{
					status = dbGetField(&paddr,DBR_DOUBLE,&buffer,&options,&nvals,NULL);
					if(cdTable[ii].data.chval != buffer.rval || listAll)
					{
						sdfdiff = fabs(cdTable[ii].data.chval - buffer.rval);
						sprintf(burtset,"%.10lf",cdTable[ii].data.chval);
						sprintf(liveset,"%.10lf",buffer.rval);
						sprintf(diffB2L,"%.8le",sdfdiff);
						mtime = buffer.time.secPastEpoch + POSIX_TIME_AT_EPICS_EPOCH;
						localErr = 1;
					}
				// If this is a string type, then get as string.
				} else {
					status = dbGetField(&paddr,DBR_STRING,&strbuffer,&options,&nvals,NULL);
					if(strcmp(cdTable[ii].data.strval,strbuffer.sval) != 0 || listAll)
					{
						sprintf(burtset,"%s",cdTable[ii].data.strval);
						sprintf(liveset,"%s",strbuffer.sval);
						sprintf(diffB2L,"%s","                                   ");
						mtime = strbuffer.time.secPastEpoch + POSIX_TIME_AT_EPICS_EPOCH;
						localErr = 1;
					}
				}
				if(localErr) *totalDiffs += 1;
				if(localErr && wcVal  && (ret = strstr(cdTable[ii].chname,wcstring) == NULL))
					localErr = 0;
				// If a diff was found, then write info the EPICS setpoint diff table.
				if(localErr)
				{
					sprintf(setErrTable[errCntr].chname,"%s", cdTable[ii].chname);

					sprintf(setErrTable[errCntr].burtset, "%s", burtset);

					sprintf(setErrTable[errCntr].liveset, "%s", liveset);

					sprintf(setErrTable[errCntr].diff, "%s", diffB2L);
					setErrTable[errCntr].sigNum = ii;
					setErrTable[errCntr].filtNum = -1;

					strcpy(localtimestring, ctime(&mtime));
					localtimestring[strlen(localtimestring) - 1] = 0;
					sprintf(setErrTable[errCntr].timeset, "%s", localtimestring);
					if(cdTable[ii].mask) setErrTable[errCntr].chFlag |= 1;
					else setErrTable[errCntr].chFlag &= ~(1);
					errCntr ++;
				}
			}
		}
	     }
	return(errCntr);
}
int modifyTable(int numEntries,SET_ERR_TABLE modTable[])
{
int ii,jj;
char fname[64];
int fmIndex = 0;
unsigned int sn,sn1;
int found = 0;
	for(ii=0;ii<numEntries;ii++)
	{
		if(modTable[ii].chFlag > 3) 
		{
			found = 0;
			for(jj=0;jj<chNum;jj++)
			{
				// fmIndex = -1;
				if(strcmp(cdTable[jj].chname,modTable[ii].chname) == 0 && (modTable[ii].chFlag & 4)) {
					if(cdTable[jj].datatype == SDF_NUM) cdTable[jj].data.chval = atof(modTable[ii].liveset);
					else sprintf(cdTable[jj].data.strval,"%s",modTable[ii].liveset);
					cdTable[jj].initialized = 1;
					found = 1;
					fmIndex = cdTable[jj].filterNum;
				}
				if(strcmp(cdTable[jj].chname,modTable[ii].chname) == 0 && (modTable[ii].chFlag & 8)) {
					cdTable[jj].mask ^= 1;
					found = 1;
					fmIndex = cdTable[jj].filterNum;
				}
			}
			if(modTable[ii].filtNum >= 0 && !found) { 
				fmIndex = modTable[ii].filtNum;
				// printf("This is a filter from diffs = %s\n",filterTable[fmIndex].fname);
				filterTable[fmIndex].newSet = 1;
				sn = modTable[ii].sigNum;
				sn1 = sn / SDF_MAX_TSIZE;
				sn %= SDF_MAX_TSIZE;
				if(modTable[ii].chFlag & 4) {
					cdTable[sn].data.chval = modTable[ii].sw[0];
					cdTable[sn].initialized = 1;
					cdTable[sn1].data.chval = modTable[ii].sw[1];
					cdTable[sn1].initialized = 1;
					filterTable[fmIndex].init = 1;
				}
				if(modTable[ii].chFlag & 8) {
					filterTable[fmIndex].mask ^= 1;
				 	cdTable[sn].mask = filterTable[fmIndex].mask;
				 	cdTable[sn1].mask = filterTable[fmIndex].mask;
				}
			}
		}
	}
	newfilterstats(fmNum);
	return(0);
}

int resetSelectedValues(int errNum)
{
long status;
int ii;
int sn;
int sn1 = 0;
dbAddr saddr;
	
	for(ii=0;ii<errNum;ii++)
	{
		if (setErrTable[ii].chFlag & 2)
		{
			sn = setErrTable[ii].sigNum;
			if(sn > SDF_MAX_TSIZE) {
				sn1 = sn / SDF_MAX_TSIZE;
				sn %= SDF_MAX_TSIZE;
			}
			status = dbNameToAddr(cdTable[sn].chname,&saddr);
			if(cdTable[sn].datatype == SDF_NUM) status = dbPutField(&saddr,DBR_DOUBLE,&cdTable[sn].data.chval,1);
			else status = dbPutField(&saddr,DBR_STRING,&cdTable[sn].data.strval,1);

			if(sn1) {
				status = dbNameToAddr(cdTable[sn1].chname,&saddr);
				status = dbPutField(&saddr,DBR_DOUBLE,&cdTable[sn1].data.chval,1);
			}
		}
	}
	return(0);
}

/// This function sets filter module request fields to aid in decoding errant filter module switch settings.
void newfilterstats(int numchans) {
	dbAddr paddr;
	long status;
	int ii;
	FILE *log;
	char chname[128];
	int mask = 0x1ffff;
	int tmpreq;
	int counter = 0;
	int rsw1,rsw2;

printf("In newfilterstats\n");
	for(ii=0;ii<numchans;ii++) {
		if(filterTable[ii].newSet) {
			counter ++;
			filterTable[ii].newSet = 0;
			filterTable[ii].init = 1;
			rsw1 = filterTable[ii].sw[0];
			rsw2 = filterTable[ii].sw[1];
			filterTable[ii].mask =  cdTable[rsw1].mask | cdTable[rsw2].mask;;
			cdTable[rsw1].mask = filterTable[ii].mask;
			cdTable[rsw2].mask = filterTable[ii].mask;
			tmpreq =  ((unsigned int)cdTable[rsw1].data.chval & 0xffff) + 
				(((unsigned int)cdTable[rsw2].data.chval & 0xffff) << 16);
			filterTable[ii].swreq = filtCtrlBitConvert(tmpreq);
			bzero(chname,strlen(chname));
			// Find address of channel
			strcpy(chname,filterTable[ii].fname);
			strcat(chname,"SWREQ");
			status = dbNameToAddr(chname,&paddr);
			if(!status)
				status = dbPutField(&paddr,DBR_LONG,&filterTable[ii].swreq,1);
			bzero(chname,strlen(chname));
			// Find address of channel
			strcpy(chname,filterTable[ii].fname);
			strcat(chname,"SWMASK");
			status = dbNameToAddr(chname,&paddr);
			if(!status)
				status = dbPutField(&paddr,DBR_LONG,&mask,1);
			// printf("New filter %d %s = 0x%x\t0x%x\t0x%x\n",ii,filterTable[ii].fname,filterTable[ii].swreq,filterTable[ii].sw[0],filterTable[ii].sw[1]);
		}
	}
	printf("Set filter masks for %d filter modules\n",counter);
}

/// This function writes BURT settings to EPICS records.
int writeEpicsDb(int numchans,		///< Number of channels to write
	         CDS_CD_TABLE myTable[],	///< Table with data to be written.
	     	 int command) 		///< Write request.
{
	dbAddr paddr;
	long status;
	int ii;

	chNotFound = 0;
	switch (command)
	{
		case SDF_LOAD_DB_ONLY:
		case SDF_LOAD_PARTIAL:
			for(ii=0;ii<numchans;ii++) {
				// Find address of channel
				status = dbNameToAddr(myTable[ii].chname,&paddr);
				if(!status)
				{
				   if(myTable[ii].datatype == SDF_NUM)	// Value if floating point number
				   {
					status = dbPutField(&paddr,DBR_DOUBLE,&myTable[ii].data.chval,1);
				   } else {			// Value is a string type
					status = dbPutField(&paddr,DBR_STRING,&myTable[ii].data.strval,1);
				   }
				}
				else {				// Write errors to chan not found table.
					if(chNotFound < SDF_ERR_TSIZE) {
						sprintf(unknownChans[chNotFound].chname,"%s",myTable[ii].chname);
						sprintf(unknownChans[chNotFound].liveset,"%s"," ");
						sprintf(unknownChans[chNotFound].timeset,"%s"," ");
						sprintf(unknownChans[chNotFound].diff,"%s"," ");
						unknownChans[chNotFound].chFlag = 0;
					}
					chNotFound ++;
				}
			}
			break;
		case SDF_READ_ONLY:
			// If request was only to re-read the BURT file, then don't want to apply new settings.
			// This is typically the case where only mask fields were changed in the BURT file to
			// apply setpoint monitoring of channels.
			return(0);
			break;
		case SDF_RESET:
			// Only want to set those channels marked by a mask back to their original BURT setting.
			for(ii=0;ii<numchans;ii++) {
			    if(myTable[ii].mask) {
				// Find address of channel
				status = dbNameToAddr(myTable[ii].chname,&paddr);
				if(!status)
				{
				   if(myTable[ii].datatype == SDF_NUM)	// Value if floating point number
				   {
					status = dbPutField(&paddr,DBR_DOUBLE,&myTable[ii].data.chval,1);
				   } else {			// Value is a string type
					status = dbPutField(&paddr,DBR_STRING,&myTable[ii].data.strval,1);
				   }
				}
			    }
			}
			break;
		default:
			printf("writeEpicsDb setting routine got unknown request \n");
			return(-1);
			break;
	}
	return(0);
}



/// Function to read BURT files and load data into local tables.
int readConfig( char *pref,		///< EPICS channel prefix from EPICS environment.
		char *sdfile, 		///< Name of the file to read.
		int command,		///< Read file request type.
		char *alarmfile)
{
	FILE *cdf;
	FILE *adf;
	char c;
	int ii;
	int lock;
	char s1[128],s2[128],s3[128],s4[128],s5[128],s6[128],s7[128],s8[128];
	char ls[6][64];
	dbAddr paddr;
	long status;
	int lderror = 0;
	int ropts = 0;
	int nvals = 1;
	int starttime,totaltime;
	char timestring[256];
	char line[128];
	char *fs;
	char ifo[4];
	double tmpreq = 0;
	char fname[128];
	int fmatch = 0;
	int fmIndex = 0;
	char errMsg[128];
	int argcount = 0;
	int isalarm = 0;
	int lineCnt = 0;

	rderror = 0;

	clock_gettime(CLOCK_REALTIME,&t);
	starttime = t.tv_nsec;

	getSdfTime(timestring);

	if(command == SDF_RESET) {
		lderror = writeEpicsDb(chNum,cdTable,command);
	} else {
		printf("PARTIAL %s\n",sdfile);
		cdf = fopen(sdfile,"r");
		if(cdf == NULL) {
			sprintf(errMsg,"New SDF request ERROR: FILE %s DOES NOT EXIST\n",sdfile);
			logFileEntry(errMsg);
			lderror = 4;
			return(lderror);
		}
		adf = fopen(alarmfile,"w");
		chNumP = 0;
		alarmCnt = 0;
		// Put dummy in s4 as this column may or may not exist.
		strcpy(s4,"x");
		bzero(s3,strlen(s3));
		strncpy(ifo,pref,3);
		while(fgets(line,sizeof line,cdf) != NULL)
		{
			isalarm = 0;
			lineCnt ++;
			strcpy(s4,"x");
			argcount = parseLine(line,s1,s2,s3,s4,s5,s6);
			if(argcount == -1) {
				sprintf(readErrTable[rderror].chname,"%s", s1);
				sprintf(readErrTable[rderror].burtset, "%s", "Improper quotations ");
				sprintf(readErrTable[rderror].liveset, "Line # %d", lineCnt);
				sprintf(readErrTable[rderror].diff, "%s", sdfile);
				sprintf(readErrTable[rderror].timeset, "%s", timestring);
				rderror ++;
				printf("Read error --- %s\n",s1);
				continue;
			}
			// Only 3 = no monit flag
			// >=4 count be monit flag or string with quotes
			// If 1st three chars match IFO ie checking this this line is not BURT header or channel marked RO
			if(strncmp(s1,ifo,3) == 0 && 
			// Don't allow load of SWSTAT or SWMASK, which are set by this program.
				strstr(s1,"_SWMASK") == NULL &&
				strstr(s1,"_SDF_NAME") == NULL &&
				strstr(s1,"_SWREQ") == NULL &&
				argcount > 2)
			{
				// Clear out the local tabel channel name string.
				bzero(cdTableP[chNumP].chname,strlen(cdTableP[chNumP].chname));
				// Load channel name into local table.
				strcpy(cdTableP[chNumP].chname,s1);
				// Check if s4 (monitor or not) is set (0/1). If doesn/'t exist in file, set to zero in local table.
				if(argcount > 3 && isdigit(s4[0])) {
					// printf("%s %s %s %s\n",s1,s2,s3,s4);
					cdTableP[chNumP].mask = atoi(s4);
					if((cdTableP[chNumP].mask < 0) || (cdTableP[chNumP].mask > 1))
						cdTableP[chNumP].mask = 0;
				} else {
					cdTableP[chNumP].mask = 0;
				}
				// Find channel in full list and replace setting info
				fmatch = 0;
				// We can set alarm values, but do not put them in cdTable
				if((strstr(cdTableP[chNumP].chname,".HIGH") != NULL) || 
					(strstr(s1,".HIHI") != NULL) || 
					(strstr(s1,".LOW") != NULL) || 
					(strstr(s1,".LOLO") != NULL) || 
					(strstr(s1,".HSV") != NULL) || 
					(strstr(s1,".OSV") != NULL) || 
					(strstr(s1,".ZSV") != NULL) || 
					(strstr(s1,".LSV") != NULL) )
				{
					alarmCnt ++;
					isalarm = 1;
					if(isdigit(s3[0])) {
						cdTableP[chNumP].datatype = SDF_NUM;
						cdTableP[chNumP].data.chval = atof(s3);
						// printf("Alarm set - %s = %f\n",cdTableP[chNumP].chname,cdTableP[chNumP].data.chval);
					} else {
						cdTableP[chNumP].datatype = SDF_STR;
						sprintf(cdTableP[chNumP].data.strval,"%s",s3);
						// printf("Alarm set - %s = %s\n",cdTableP[chNumP].chname,cdTableP[chNumP].data.strval);
					}
					fprintf(adf,"%s %s %s\n",s1,s2,s3);
				} 
				if(!isalarm)
				{
				   // Add settings to local table.
				   for(ii=0;ii<chNum;ii++)
				   {
					if(strcmp(cdTable[ii].chname,cdTableP[chNumP].chname) == 0)
					{
						fmatch = 1;
						if(cdTable[ii].datatype == SDF_STR || (!isdigit(s3[0]) && strncmp(s3,"-",1) != 0))
						{
							// s3[strlen(s3) - 1] = 0;
							cdTableP[chNumP].datatype = SDF_STR;
							sprintf(cdTableP[chNumP].data.strval,"%s",s3);
							if(command != SDF_LOAD_DB_ONLY)
							{
								sprintf(cdTable[ii].data.strval,"%s",s3);
							}
						} else {
							cdTableP[chNumP].datatype = SDF_NUM;
							cdTableP[chNumP].data.chval = atof(s3);
							if(cdTable[ii].filterswitch) {
								if(cdTableP[chNumP].data.chval > 0xffff) {
									sprintf(readErrTable[rderror].chname,"%s", cdTable[ii].chname);
									sprintf(readErrTable[rderror].burtset, "0x%x", (int)cdTableP[chNumP].data.chval);
									sprintf(readErrTable[rderror].liveset, "%s", "OVERRANGE");
									sprintf(readErrTable[rderror].diff, "%s", "MAX VAL = 0xffff");
									sprintf(readErrTable[rderror].timeset, "%s", timestring);
									printf("Read error --- %s\n", cdTable[ii].chname);
									rderror ++;
								}
								cdTableP[chNumP].data.chval = (int) cdTableP[chNumP].data.chval & 0xffff;
							}
							if(command != SDF_LOAD_DB_ONLY)
								cdTable[ii].data.chval = cdTableP[chNumP].data.chval;
						}
						if(command != SDF_LOAD_DB_ONLY) {
							if(cdTableP[chNumP].mask != -1)
								cdTable[ii].mask = cdTableP[chNumP].mask;
							cdTable[ii].initialized = 1;
						}
					}
				   }
				   // if(!fmatch) printf("NEW channel not found %s\n",cdTableP[chNumP].chname);
				}
				// The following loads info into the filter module table if a FM switch
				fmIndex = -1;
				if(((strstr(s1,"_SW1S") != NULL) && (strstr(s1,"_SW1S.") == NULL)) ||
					((strstr(s1,"_SW2S") != NULL) && (strstr(s1,"_SW2S.") == NULL)))
				{
				   	bzero(fname,strlen(fname));
					strncpy(fname,s1,(strlen(s1)-4));
				   	for(ii=0;ii<fmNum;ii++)
				   	{
						if(strcmp(filterTable[ii].fname,fname) == 0) 
						{
							fmIndex = ii;
							filterTable[fmIndex].newSet = 1;
							break;
						}
					}
				}
				chNumP ++;
				if(chNumP >= SDF_MAX_CHANS)
				{
					fclose(cdf);
					fclose(adf);
					sprintf(errMsg,"Number of channels in %s exceeds program limit\n",sdfile);
					logFileEntry(errMsg);
					lderror = 4;
					return(lderror);
				}
		   	}
		}
		fclose(cdf);
		fclose(adf);
		printf("Loading epics %d\n",chNumP);
		lderror = writeEpicsDb(chNumP,cdTableP,command);
		sleep(2);
		newfilterstats(fmNum);
		fmtInit = 1;
	}

	// Calc time to load settings and make log entry
	clock_gettime(CLOCK_REALTIME,&t);
	totaltime = t.tv_nsec - starttime;
	if(totaltime < 0) totaltime += 1000000000;
	if(command == SDF_LOAD_PARTIAL) {
		sprintf(errMsg,"New SDF request (w/table update): %s\nFile = %s\nTotal Chans = %d with load time = %d usec\n",timestring,sdfile,chNumP,(totaltime/1000));
	} else if(command == SDF_LOAD_DB_ONLY){
		sprintf(errMsg,"New SDF request (No table update): %s\nFile = %s\nTotal Chans = %d with load time = %d usec\n",timestring,sdfile,chNumP,(totaltime/1000));
	}
	logFileEntry(errMsg);
	status = dbNameToAddr(reloadtimechannel,&paddr);
	status = dbPutField(&paddr,DBR_STRING,timestring,1);
	printf("Number of read errors = %d\n",rderror);
	if(rderror) lderror = 2;
	return(lderror);
}

/// Routine used to extract all settings channels from EPICS database to create local settings table on startup.
void dbDumpRecords(DBBASE *pdbbase)
{
    DBENTRY  *pdbentry;
    long  status;
    char mytype[4][64];
    int ii;
    int fc = 0;
    char errMsg[128];
    char tmpstr[64];
    int amatch,jj;
	int cnt = 0;

    // By convention, the RCG produces ai and bi records for settings.
    sprintf(mytype[0],"%s","ai");
    sprintf(mytype[1],"%s","bi");
    sprintf(mytype[2],"%s","stringin");
    pdbentry = dbAllocEntry(pdbbase);

    chNum = 0;
    fmNum = 0;
    for(ii=0;ii<3;ii++) {
    status = dbFindRecordType(pdbentry,mytype[ii]);

    if(status) {printf("No record descriptions\n");return;}
    while(!status) {
        printf("record type: %s",dbGetRecordTypeName(pdbentry));
        status = dbFirstRecord(pdbentry);
        if (status) printf("  No Records\n"); 
	cnt = 0;
        while (!status) {
	    cnt++;
            if (dbIsAlias(pdbentry)) {
                printf("\n  Alias:%s\n",dbGetRecordName(pdbentry));
            } else {
		sprintf(cdTable[chNum].chname,"%s",dbGetRecordName(pdbentry));
		cdTable[chNum].filterswitch = 0;
		cdTable[chNum].filterNum = 0;
		// Check if this is a filter module
		// If so, initialize parameters
		if((strstr(cdTable[chNum].chname,"_SW1S") != NULL) && (strstr(cdTable[chNum].chname,"_SW1S.") == NULL))
		{
			cdTable[chNum].filterswitch = 1;
			fmNum ++;
		}
		if((strstr(cdTable[chNum].chname,"_SW2S") != NULL) && (strstr(cdTable[chNum].chname,"_SW2S.") == NULL))
		{
			cdTable[chNum].filterswitch = 2;
		}
		if(ii == 0) {
			cdTable[chNum].datatype = SDF_NUM;
			cdTable[chNum].data.chval = 0.0;
		} else {
			cdTable[chNum].datatype = SDF_STR;
			sprintf(cdTable[chNum].data.strval,"");
		}
		cdTable[chNum].mask = 0;
		cdTable[chNum].initialized = 0;
            }
	    
	    chNum ++;
            status = dbNextRecord(pdbentry);
        }
	printf("  %d Records, with %d filters\n", cnt,fmNum);
    }
}
    fmNum = 0;
    for(ii=0;ii<chNum;ii++) {
	if(cdTable[ii].filterswitch == 1)
	{
		strncpy(filterTable[fmNum].fname,cdTable[ii].chname,(strlen(cdTable[ii].chname)-4));
		sprintf(tmpstr,"%s%s",filterTable[fmNum].fname,"SW2S");
		filterTable[fmNum].sw[0] = ii;
		cdTable[ii].filterNum = fmNum;
		amatch = 0;
    		for(jj=0;jj<chNum;jj++) {
			if(strcmp(tmpstr,cdTable[jj].chname) == 0)
			{
				filterTable[fmNum].sw[1] = jj;
				amatch = 1;
			}
		}
		if(!amatch) printf("No match for %s\n",tmpstr);
		fmNum ++;
	}
    }
	printf("  %d Records, with %d filters\n", cnt,fmNum);
    printf("End of all Records\n");
    dbFreeEntry(pdbentry);
}

/// Called on EPICS startup; This is generic EPICS provided function, modified for LIGO use.
int main(int argc,char *argv[])
{
	// Addresses for SDF EPICS records.
	dbAddr reload_addr;
	dbAddr sdfname_addr;
	dbAddr reloadstat_addr;
	dbAddr loadedfile_addr;
	dbAddr sperroraddr;
	dbAddr alrmchcountaddr;
	dbAddr filesetcntaddr;
	dbAddr fulldbcntaddr;
	dbAddr monchancntaddr;
	dbAddr tablesortreqaddr;
	dbAddr wcreqaddr;
	dbAddr chnotfoundaddr;
	dbAddr chnotinitaddr;
	dbAddr sorttableentriesaddr;
	dbAddr monflagaddr;
	dbAddr reloadtimeaddr;
	dbAddr rcgversion_addr;
	dbAddr msgstraddr;
	dbAddr edbloadedaddr;
	dbAddr savecmdaddr;
	dbAddr saveasaddr;
	dbAddr wcstringaddr;
	dbAddr savetypeaddr;
	dbAddr saveoptsaddr;
	dbAddr savefileaddr;
	dbAddr savetimeaddr;
	dbAddr daqmsgaddr;
	dbAddr coeffmsgaddr;
	dbAddr resetoneaddr;
	dbAddr selectaddr[4];
	dbAddr pagelockaddr[3];
	// Initialize request for file load on startup.
	int sdfReq = SDF_LOAD_PARTIAL;
	long status;
	int request;
	long ropts = 0;
	long nvals = 1;
	int rdstatus = 0;
	int burtstatus = 0;
	char loadedSdf[256];
	char sdffileloaded[256];
   	int sperror = 0;
	int noMon;
	int noInit;
	// FILE *csFile;
	int ii;
	int setChans = 0;
	char tsrString[64];
	int tsrVal = 0;
	int wcVal = 0;
	int monFlag = 0;
	int sdfSaveReq = 0;
	int saveType = 0;
	char saveTypeString[64];
	int saveOpts = 0;
	char saveOptsString[64];
	int fivesectimer = 0;
	long daqFileCrc;
	long coeffFileCrc;
	long sdfFileCrc;
	char modfilemsg[] = "Modified File Detected ";
	struct stat st = {0};
	int reqValid = 0;
	int pageDisp = 0;
	int resetByte;
	int resetBit;
	int confirmVal;
	int myexp;
	int selectCounter[4] = {0,0,0,0};
	int selectAll = 0;
	int freezeTable = 0;
	int zero = 0;
	char backupName[64];
	int lastTable = 0;
	int cdSort = 0;
	int diffCnt = 0;
    	char errMsg[128];

    if(argc>=2) {
        iocsh(argv[1]);
	// printf("Executing post script commands\n");
	// dbDumpRecords(*iocshPpdbbase);
	// Get environment variables from startup command to formulate EPICS record names.
	char *pref = getenv("PREFIX");
	char *sdfDir = getenv("SDF_DIR");
	char *sdf = getenv("SDF_FILE");
	char *modelname =  getenv("SDF_MODEL");
	char *targetdir =  getenv("TARGET_DIR");
	char *daqFile =  getenv("DAQ_FILE");
	char *coeffFile =  getenv("COEFF_FILE");
	char *logdir = getenv("LOG_DIR");
	if(stat(logdir, &st) == -1) mkdir(logdir,0777);
	// strcat(sdf,"_safe");
	char sdfile[256];
	char sdalarmfile[256];
	char bufile[256];
	char saveasfilename[128];
	char wcstring[64];
	printf("My prefix is %s\n",pref);
	sprintf(sdfile, "%s%s%s", sdfDir, sdf,".snap");					// Initialize with BURT_safe.snap
	sprintf(bufile, "%s%s", sdfDir, "fec.snap");					// Initialize table dump file
	sprintf(logfilename, "%s%s", logdir, "/ioc.log");					// Initialize table dump file
	printf("SDF FILE = %s\n",sdfile);
	printf("CURRENt FILE = %s\n",bufile);
	printf("LOG FILE = %s\n",logfilename);
	char rcgversionname[256]; 
	int majorversion = RCG_VERSION_MAJOR;
	int subversion1 = RCG_VERSION_MINOR;
	int subversion2 = RCG_VERSION_SUB;
	int myreleased = RCG_VERSION_REL;
	double myversion;
	myversion = majorversion + 0.01 * subversion1 + 0.001 * subversion2;
	if(!myreleased) myversion *= -1.0;
	sprintf(rcgversionname, "%s_%s", pref, "RCG_VERSION");		// Request to load new BURT
	status = dbNameToAddr(rcgversionname,&rcgversion_addr);
	status = dbPutField(&rcgversion_addr,DBR_DOUBLE,&myversion,1);

	// Create BURT/SDF EPICS channel names
	char reloadChan[256]; sprintf(reloadChan, "%s_%s", pref, "SDF_RELOAD");		// Request to load new BURT
	// Set request to load safe.snap on startup
	status = dbNameToAddr(reloadChan,&reload_addr);
	status = dbPutField(&reload_addr,DBR_LONG,&sdfReq,1);		// Init request for startup.

	char reloadStat[256]; sprintf(reloadStat, "%s_%s", pref, "SDF_RELOAD_STATUS");	// Status of last reload
	status = dbNameToAddr(reloadStat,&reloadstat_addr);
	status = dbPutField(&reloadstat_addr,DBR_LONG,&rdstatus,1);	// Init to zero.

	char sdfFileName[256]; sprintf(sdfFileName, "%s_%s", pref, "SDF_NAME");		// Name of file to load next request
	// Initialize BURT file to be loaded next request = safe.snap
	status = dbNameToAddr(sdfFileName,&sdfname_addr);		// Get Address
	status = dbPutField(&sdfname_addr,DBR_STRING,sdf,1);		// Init to safe.snap

	char loadedFile[256]; sprintf(loadedFile, "%s_%s", pref, "SDF_LOADED");		// Name of file presently loaded
	status = dbNameToAddr(loadedFile,&loadedfile_addr);		//Get Address

	char edbloadedFile[256]; sprintf(edbloadedFile, "%s_%s", pref, "SDF_LOADED_EDB");	// Name of file presently loaded
	status = dbNameToAddr(edbloadedFile,&edbloadedaddr);		// Get Address

	char speStat[256]; sprintf(speStat, "%s_%s", pref, "SDF_DIFF_CNT");		// Setpoint diff counter
	status = dbNameToAddr(speStat,&sperroraddr);			// Get Address
	status = dbPutField(&sperroraddr,DBR_LONG,&sperror,1);		// Init to zero.

	char spaStat[256]; sprintf(spaStat, "%s_%s", pref, "SDF_ALARM_CNT");		// Number of alarm settings in a BURT file.
	status = dbNameToAddr(spaStat,&alrmchcountaddr);		// Get Address
	status = dbPutField(&alrmchcountaddr,DBR_LONG,&alarmCnt,1);	// Init to zero.

	char fcc[256]; sprintf(fcc, "%s_%s", pref, "SDF_FULL_CNT");			// Number of setting channels in EPICS db
	status = dbNameToAddr(fcc,&fulldbcntaddr);

	char fsc[256]; sprintf(fsc, "%s_%s", pref, "SDF_FILE_SET_CNT");			// Number of settings inBURT file
	status = dbNameToAddr(fsc,&filesetcntaddr);

	char mcc[256]; sprintf(mcc, "%s_%s", pref, "SDF_UNMON_CNT");			// Number of settings NOT being monitored.
	status = dbNameToAddr(mcc,&monchancntaddr);

	char tsrname[256]; sprintf(tsrname, "%s_%s", pref, "SDF_SORT");			// SDF Table sorting request
	status = dbNameToAddr(tsrname,&tablesortreqaddr);

	char wcname[256]; sprintf(wcname, "%s_%s", pref, "SDF_WILDCARD");			// SDF Table sorting request
	status = dbNameToAddr(wcname,&wcreqaddr);
	status = dbPutField(&wcreqaddr,DBR_LONG,&zero,1);		// Init to zero.

	char cnfname[256]; sprintf(cnfname, "%s_%s", pref, "SDF_DROP_CNT");		// Number of channels not found.
	status = dbNameToAddr(cnfname,&chnotfoundaddr);

	char cniname[256]; sprintf(cniname, "%s_%s", pref, "SDF_UNINIT_CNT");		// Number of channels not initialized.
	status = dbNameToAddr(cniname,&chnotinitaddr);

	char stename[256]; sprintf(stename, "%s_%s", pref, "SDF_TABLE_ENTRIES");	// Number of entries in an SDF reporting table.
	status = dbNameToAddr(stename,&sorttableentriesaddr);

	char monflagname[256]; sprintf(monflagname, "%s_%s", pref, "SDF_MON_ALL");	// Request to monitor all channels.
	status = dbNameToAddr(monflagname,&monflagaddr);		// Get Address.
	status = dbPutField(&monflagaddr,DBR_LONG,&rdstatus,1);		// Init to zero.

	char savecmdname[256]; sprintf(savecmdname, "%s_%s", pref, "SDF_SAVE_CMD");	// SDF Save command.
	status = dbNameToAddr(savecmdname,&savecmdaddr);		// Get Address.
	status = dbPutField(&savecmdaddr,DBR_LONG,&rdstatus,1);		// Init to zero.

	char pagelockname[128]; sprintf(pagelockname, "%s_%s", pref, "SDF_TABLE_LOCK");	// SDF Save command.
	status = dbNameToAddr(pagelockname,&pagelockaddr);		// Get Address.
	status = dbPutField(&pagelockaddr,DBR_LONG,&freezeTable,1);		// Init to zero.

	char saveasname[256]; sprintf(saveasname, "%s_%s", pref, "SDF_SAVE_AS_NAME");	// SDF Save as file name.
	// Clear out the save as file name request
	status = dbNameToAddr(saveasname,&saveasaddr);			// Get Address.
	status = dbPutField(&saveasaddr,DBR_STRING,"default",1);	// Set as dummy 'default'

	char wcstringname[256]; sprintf(wcstringname, "%s_%s", pref, "SDF_WC_STR");	// SDF Save as file name.
	status = dbNameToAddr(wcstringname,&wcstringaddr);			// Get Address.
	status = dbPutField(&wcstringaddr,DBR_STRING,"",1);		// Set as dummy 'default'

	char savetypename[256]; sprintf(savetypename, "%s_%s", pref, "SDF_SAVE_TYPE");	// SDF Save file type.
	status = dbNameToAddr(savetypename,&savetypeaddr);

	char saveoptsname[256]; sprintf(saveoptsname, "%s_%s", pref, "SDF_SAVE_OPTS");	// SDF Save file options.
	status = dbNameToAddr(saveoptsname,&saveoptsaddr);

	char savefilename[256]; sprintf(savefilename, "%s_%s", pref, "SDF_SAVE_FILE");	// SDF Name of last file saved.
	status = dbNameToAddr(savefilename,&savefileaddr);
	status = dbPutField(&savefileaddr,DBR_STRING,"",1);

	char savetimename[256]; sprintf(savetimename, "%s_%s", pref, "SDF_SAVE_TIME");	// SDF Time of last file save.
	status = dbNameToAddr(savetimename,&savetimeaddr);
	status = dbPutField(&savetimeaddr,DBR_STRING,"",1);

	char moddaqfilemsg[256]; sprintf(moddaqfilemsg, "%s_%s", pref, "MSGDAQ");	// Record to write if DAQ file changed.
	status = dbNameToAddr(moddaqfilemsg,&daqmsgaddr);

	char modcoefffilemsg[128]; sprintf(modcoefffilemsg, "%s_%s", pref, "MSG");	// Record to write if Coeff file changed.
	status = dbNameToAddr(modcoefffilemsg,&coeffmsgaddr);

	char msgstrname[128]; sprintf(msgstrname, "%s_%s", pref, "SDF_MSG_STR");	// SDF Time of last file save.
	status = dbNameToAddr(msgstrname,&msgstraddr);
	status = dbPutField(&msgstraddr,DBR_STRING,"",1);


	sprintf(timechannel,"%s_%s", pref, "TIME_STRING");
	// printf("timechannel = %s\n",timechannel);
	sprintf(reloadtimechannel,"%s_%s", pref, "SDF_RELOAD_TIME");			// Time of last BURT reload
	status = dbNameToAddr(reloadtimechannel,&reloadtimeaddr);

	int pageNum = 0;
	int pageNumSet = 0;
	dbAddr pagereqaddr;
        char pagereqname[256]; sprintf(pagereqname, "%s_%s", pref, "SDF_PAGE"); // SDF Save command.
        status = dbNameToAddr(pagereqname,&pagereqaddr);                // Get Address.

	unsigned int resetNum = 0;
	char resetOneName[256]; sprintf(resetOneName, "%s_%s", pref, "SDF_RESET_CHAN");	// SDF reset one value.
	status = dbNameToAddr(resetOneName,&resetoneaddr);
	status = dbPutField(&resetoneaddr,DBR_LONG,&resetNum,1);

	char selectName[256]; 
	sprintf(selectName, "%s_%s", pref, "SDF_SELECT_SUM0");	// SDF reset one value.
	status = dbNameToAddr(selectName,&selectaddr[0]);
	status = dbPutField(&selectaddr[0],DBR_LONG,&resetNum,1);
	sprintf(selectName, "%s_%s", pref, "SDF_SELECT_SUM1");	// SDF reset one value.
	status = dbNameToAddr(selectName,&selectaddr[1]);
	status = dbPutField(&selectaddr[1],DBR_LONG,&resetNum,1);
	sprintf(selectName, "%s_%s", pref, "SDF_SELECT_SUM2");	// SDF reset one value.
	status = dbNameToAddr(selectName,&selectaddr[2]);
	status = dbPutField(&selectaddr[2],DBR_LONG,&resetNum,1);
	sprintf(selectName, "%s_%s", pref, "SDF_SELECT_ALL");	// SDF reset one value.
	status = dbNameToAddr(selectName,&selectaddr[3]);
	status = dbPutField(&selectaddr[3],DBR_LONG,&selectAll,1);

	dbAddr confirmwordaddr;
	char confirmwordname[64]; 
	sprintf(confirmwordname, "%s_%s", pref, "SDF_CONFIRM");	// Record to write if Coeff file changed.
	status = dbNameToAddr(confirmwordname,&confirmwordaddr);
	status = dbPutField(&confirmwordaddr,DBR_LONG,&resetNum,1);

	dbDumpRecords(*iocshPpdbbase);
	sprintf(errMsg,"Software Restart \nRCG VERSION: %.2f",myversion);
	logFileEntry(errMsg);

	// Initialize DAQ and COEFF file CRC checksums for later compares.
	daqFileCrc = checkFileCrc(daqFile);
	coeffFileCrc = checkFileCrc(coeffFile);
	reportSetErrors(pref, 0,setErrTable,0);

	sleep(1);       // Need to wait before first restore to allow sequencers time to do their initialization.
	cdSort = spChecker(monFlag,cdTableList,wcVal,wcstring,1,&status);

	// Start Infinite Loop 		*******************************************************************************
	for(;;) {
		usleep(100000);					// Run loop at 10Hz.
		fivesectimer = (fivesectimer + 1) % 50;		// Increment 5 second timer for triggering CRC checks.
		// Check for reload request
		status = dbGetField(&reload_addr,DBR_LONG,&request,&ropts,&nvals,NULL);
		// Get BURT Read File Name
		status = dbNameToAddr(sdfFileName,&sdfname_addr);
		status = dbGetField(&sdfname_addr,DBR_STRING,sdf,&ropts,&nvals,NULL);
		//  Create full filename including directory and extension.
		sprintf(sdfile, "%s%s%s", sdfDir, sdf,".snap");
		sprintf(sdalarmfile, "%s%s%s", sdfDir, sdf,"_alarms.snap");
		// Check if file name != to one presently loaded
		if(strcmp(sdf,loadedSdf) != 0) burtstatus |= 1;
		else burtstatus &= ~(1);
		if(burtstatus == 0)
				status = dbPutField(&msgstraddr,DBR_STRING," ",1);
		if(burtstatus == 1)
				status = dbPutField(&msgstraddr,DBR_STRING,"New SDF File Pending",1);
		if(burtstatus & 2)
				status = dbPutField(&msgstraddr,DBR_STRING,"Read Error: Errant line(s) in file",1);
		if(burtstatus & 4)
				status = dbPutField(&msgstraddr,DBR_STRING,"Read Error: File Not Found",1);
		if(request != 0) {		// If there is a read file request, then:
			status = dbPutField(&reload_addr,DBR_LONG,&ropts,1);	// Clear the read request.
			reqValid = 1;
			if(reqValid) {
				rdstatus = readConfig(pref,sdfile,request,sdalarmfile);
				if (rdstatus) burtstatus |= rdstatus;
				else burtstatus &= ~(6);
				if(burtstatus < 4) {
					switch (request){
						case SDF_LOAD_DB_ONLY:
							strcpy(loadedSdf,sdf); 
							status = dbPutField(&edbloadedaddr,DBR_STRING,loadedSdf,1);
							break;
						case SDF_RESET:
							break;
						case SDF_LOAD_PARTIAL:
							strcpy(loadedSdf,sdf); 
							status = dbPutField(&loadedfile_addr,DBR_STRING,loadedSdf,1);
							status = dbPutField(&edbloadedaddr,DBR_STRING,loadedSdf,1);
							break;
						case SDF_READ_ONLY:
							strcpy(loadedSdf,sdf); 
							status = dbPutField(&loadedfile_addr,DBR_STRING,loadedSdf,1);
							break;
						default:
							logFileEntry("Invalid READ Request");
							reqValid = 0;
							break;
					}
					status = dbPutField(&reloadstat_addr,DBR_LONG,&rdstatus,1);
					// Get the file CRC for later checking if file changed.
					sprintf(sdffileloaded, "%s%s%s", sdfDir, loadedSdf,".snap");
					sdfFileCrc = checkFileCrc(sdffileloaded);
					// Calculate and report the number of settings in the BURT file.
					setChans = chNumP - alarmCnt;
					status = dbPutField(&filesetcntaddr,DBR_LONG,&setChans,1);
					// Report number of settings in the main table.
					setChans = chNum - fmNum;
					status = dbPutField(&fulldbcntaddr,DBR_LONG,&setChans,1);
					// Sort channels for data reporting via the MEDM table.
					noMon = createSortTableEntries(chNum,0,"",&noInit);
					// Calculate and report number of channels NOT being monitored.
					status = dbPutField(&monchancntaddr,DBR_LONG,&chNotMon,1);
					status = dbPutField(&alrmchcountaddr,DBR_LONG,&alarmCnt,1);
					// Report number of channels in BURT file that are not in local database.
					status = dbPutField(&chnotfoundaddr,DBR_LONG,&chNotFound,1);
					// Report number of channels that have not been initialized via a BURT read.
					status = dbPutField(&chnotinitaddr,DBR_LONG,&chNotInit,1);
					// Write out local monitoring table as snap file.
					status = writeTable2File(sdfDir,bufile,SDF_WITH_INIT_FLAG,cdTable);
				}
			}
		}
		status = dbPutField(&reloadstat_addr,DBR_LONG,&burtstatus,1);
		// sleep(1);
		// Check for SAVE requests
		status = dbGetField(&savecmdaddr,DBR_LONG,&sdfSaveReq,&ropts,&nvals,NULL);
		if(sdfSaveReq)	// If there is a SAVE file request, then:
		{
			// Clear the save file request
			status = dbPutField(&savecmdaddr,DBR_LONG,&ropts,1);
			// Determine file type
			status = dbGetField(&savetypeaddr,DBR_STRING,saveTypeString,&ropts,&nvals,NULL);
			saveType = 0;
                        if(strcmp(saveTypeString,"TABLE TO FILE") == 0) saveType = SAVE_TABLE_AS_SDF;
                        if(strcmp(saveTypeString,"EPICS DB TO FILE") == 0) saveType = SAVE_EPICS_AS_SDF;
			// Determine file options
                        saveOpts = 0;
			status = dbGetField(&saveoptsaddr,DBR_STRING,saveOptsString,&ropts,&nvals,NULL);
                        if(strcmp(saveOptsString,"TIME NOW") == 0) saveOpts = SAVE_TIME_NOW;
                        if(strcmp(saveOptsString,"OVERWRITE") == 0) saveOpts = SAVE_OVERWRITE;
                        if(strcmp(saveOptsString,"SAVE AS") == 0) saveOpts = SAVE_AS;
			// Determine if request is valid.
			if(saveType && saveOpts)
			{
				// Get saveas filename
				status = dbGetField(&saveasaddr,DBR_STRING,saveasfilename,&ropts,&nvals,NULL);
				if(saveOpts == SAVE_OVERWRITE) 
					savesdffile(saveType,SAVE_TIME_NOW,sdfDir,modelname,sdfile,saveasfilename,loadedSdf,savefileaddr,savetimeaddr,reloadtimeaddr);
				// Save the file
				savesdffile(saveType,saveOpts,sdfDir,modelname,sdfile,saveasfilename,loadedSdf,savefileaddr,savetimeaddr,reloadtimeaddr);
				if(saveOpts == SAVE_OVERWRITE) 
						sdfFileCrc = checkFileCrc(sdffileloaded);
			} else {
				logFileEntry("Invalid SAVE File Request");
			}
		}
		// Check present settings vs BURT settings and report diffs.
		// Check if MON ALL CHANNELS is set
		status = dbGetField(&monflagaddr,DBR_LONG,&monFlag,&ropts,&nvals,NULL);
		status = dbGetField(&wcstringaddr,DBR_STRING,wcstring,&ropts,&nvals,NULL);
		status = dbGetField(&wcstringaddr,DBR_STRING,saveasfilename,&ropts,&nvals,NULL);
		// Call the diff checking function.
		if(!freezeTable)
			sperror = spChecker(monFlag,setErrTable,wcVal,wcstring,0,&diffCnt);
		// Report number of diffs found.
		status = dbPutField(&sperroraddr,DBR_LONG,&diffCnt,1);
		// Table sorting and presentation
		status = dbGetField(&tablesortreqaddr,DBR_USHORT,&tsrVal,&ropts,&nvals,NULL);
		status = dbGetField(&wcreqaddr,DBR_USHORT,&wcVal,&ropts,&nvals,NULL);
		status = dbGetField(&pagereqaddr,DBR_LONG,&pageNumSet,&ropts,&nvals,NULL);
		status = dbGetField(&confirmwordaddr,DBR_LONG,&confirmVal,&ropts,&nvals,NULL);
		status = dbGetField(&selectaddr[3],DBR_LONG,&selectAll,&ropts,&nvals,NULL);
		if(pageNumSet != 0) {
			pageNum += pageNumSet;
			if(pageNum < 0) pageNum = 0;
			status = dbPutField(&pagereqaddr,DBR_LONG,&ropts,1);                // Init to zero.
		}
		switch(tsrVal) { 
			case SDF_TABLE_DIFFS:
				// Need to clear selections when moving between tables.
				if(lastTable !=  SDF_TABLE_DIFFS) {
					clearTableSelections(sperror,setErrTable, selectCounter);
					confirmVal = 0;
				}
				pageDisp = reportSetErrors(pref, sperror,setErrTable,pageNum);
				status = dbPutField(&sorttableentriesaddr,DBR_LONG,&sperror,1);
				status = dbGetField(&resetoneaddr,DBR_LONG,&resetNum,&ropts,&nvals,NULL);
				if(selectAll) {
					setAllTableSelections(sperror,setErrTable, selectCounter,selectAll);
					selectAll = 0;
					status = dbPutField(&selectaddr[3],DBR_LONG,&selectAll,1);
				}
				if(resetNum) {
					decodeChangeSelect(resetNum, pageDisp, sperror, setErrTable,selectCounter);
					resetNum = 0;
					status = dbPutField(&resetoneaddr,DBR_LONG,&resetNum,1);
				}
				if(confirmVal) {
					if(selectCounter[0] && (confirmVal & 2)) status = resetSelectedValues(sperror);
					if((selectCounter[1] || selectCounter[2]) && (confirmVal & 2)) {
						// Save present table as timenow.
						status = dbGetField(&loadedfile_addr,DBR_STRING,backupName,&ropts,&nvals,NULL);
						// printf("BACKING UP: %s\n",backupName);
						savesdffile(SAVE_TABLE_AS_SDF,SAVE_TIME_NOW,sdfDir,modelname,sdfile,saveasfilename,backupName,
							    savefileaddr,savetimeaddr,reloadtimeaddr);
						// Overwrite the table with new values
						status = modifyTable(sperror,setErrTable);
						// Overwrite file
						savesdffile(SAVE_TABLE_AS_SDF,SAVE_OVERWRITE,sdfDir,modelname,sdfile,saveasfilename,
							    backupName,savefileaddr,savetimeaddr,reloadtimeaddr);
						sdfFileCrc = checkFileCrc(sdffileloaded);
						status = writeTable2File(sdfDir,bufile,SDF_WITH_INIT_FLAG,cdTable);
					}
					clearTableSelections(sperror,setErrTable, selectCounter);
					confirmVal = 0;
					status = dbPutField(&confirmwordaddr,DBR_LONG,&confirmVal,1);
					noMon = createSortTableEntries(chNum,wcVal,wcstring,&noInit);
					status = dbPutField(&monchancntaddr,DBR_LONG,&chNotMon,1);
				}
				lastTable = SDF_TABLE_DIFFS;
				break;
			case SDF_TABLE_NOT_FOUND:
				// Need to clear selections when moving between tables.
				if(lastTable != SDF_TABLE_NOT_FOUND) {
					clearTableSelections(sperror,setErrTable, selectCounter);
					confirmVal = 0;
				}
				pageDisp = reportSetErrors(pref, chNotFound,unknownChans,pageNum);
				status = dbPutField(&sorttableentriesaddr,DBR_LONG,&chNotFound,1);
				lastTable =  SDF_TABLE_NOT_FOUND;
				break;
			case SDF_TABLE_NOT_INIT:
				if(lastTable != SDF_TABLE_NOT_INIT) {
					clearTableSelections(sperror,setErrTable, selectCounter);
					confirmVal = 0;
				}
				getEpicsSettings(chNum);
				noMon = createSortTableEntries(chNum,wcVal,wcstring,&noInit);
				pageDisp = reportSetErrors(pref, noInit, uninitChans,pageNum);
				status = dbGetField(&resetoneaddr,DBR_LONG,&resetNum,&ropts,&nvals,NULL);
				if(selectAll == 2) {
					setAllTableSelections(noInit,uninitChans,selectCounter,selectAll);
					selectAll = 0;
					status = dbPutField(&selectaddr[3],DBR_LONG,&selectAll,1);
				}
				if(resetNum > 100 && resetNum < 200) {
					decodeChangeSelect(resetNum, pageDisp, noInit, uninitChans,selectCounter);
				}
				if(resetNum) {
					resetNum = 0;
					status = dbPutField(&resetoneaddr,DBR_LONG,&resetNum,1);
				}
				if(confirmVal) {
					if(selectCounter[1] && (confirmVal & 2)) {
						// Save present table as timenow.
						status = dbGetField(&loadedfile_addr,DBR_STRING,backupName,&ropts,&nvals,NULL);
						// printf("BACKING UP: %s\n",backupName);
						savesdffile(SAVE_TABLE_AS_SDF,SAVE_TIME_NOW,sdfDir,modelname,sdfile,saveasfilename,backupName,
							    savefileaddr,savetimeaddr,reloadtimeaddr);
						// Overwrite the table with new values
						status = modifyTable(noInit,uninitChans);
						// Overwrite file
						savesdffile(SAVE_TABLE_AS_SDF,SAVE_OVERWRITE,sdfDir,modelname,sdfile,saveasfilename,
							    backupName,savefileaddr,savetimeaddr,reloadtimeaddr);
						sdfFileCrc = checkFileCrc(sdffileloaded);
						status = writeTable2File(sdfDir,bufile,SDF_WITH_INIT_FLAG,cdTable);
					}
					clearTableSelections(chNotInit,uninitChans, selectCounter);
					confirmVal = 0;
					status = dbPutField(&confirmwordaddr,DBR_LONG,&confirmVal,1);
				}
				status = dbPutField(&sorttableentriesaddr,DBR_LONG,&noInit,1);
				status = dbPutField(&chnotinitaddr,DBR_LONG,&chNotInit,1);
				lastTable = SDF_TABLE_NOT_INIT;
				break;
			case SDF_TABLE_NOT_MONITORED:
				if(lastTable != SDF_TABLE_NOT_MONITORED) {
					clearTableSelections(noMon,unMonChans, selectCounter);
					confirmVal = 0;
					status = dbPutField(&confirmwordaddr,DBR_LONG,&confirmVal,1);
				}
				noMon = createSortTableEntries(chNum,wcVal,wcstring,&noInit);
				status = dbPutField(&monchancntaddr,DBR_LONG,&chNotMon,1);
				pageDisp = reportSetErrors(pref, noMon, unMonChans,pageNum);
				status = dbGetField(&resetoneaddr,DBR_LONG,&resetNum,&ropts,&nvals,NULL);
				if(selectAll == 3) {
					setAllTableSelections(noMon,unMonChans,selectCounter,selectAll);
					selectAll = 0;
					status = dbPutField(&selectaddr[3],DBR_LONG,&selectAll,1);
				}
				if(resetNum > 200) {
					decodeChangeSelect(resetNum, pageDisp, noMon, unMonChans,selectCounter);
				}
				if(resetNum) {
					resetNum = 0;
					status = dbPutField(&resetoneaddr,DBR_LONG,&resetNum,1);
				}
				if(confirmVal) {
					if(selectCounter[2] && (confirmVal & 2)) {
						// Save present table as timenow.
						status = dbGetField(&loadedfile_addr,DBR_STRING,backupName,&ropts,&nvals,NULL);
						// printf("BACKING UP: %s\n",backupName);
						savesdffile(SAVE_TABLE_AS_SDF,SAVE_TIME_NOW,sdfDir,modelname,sdfile,saveasfilename,backupName,
							    savefileaddr,savetimeaddr,reloadtimeaddr);
						// Overwrite the table with new values
						status = modifyTable(noMon,unMonChans);
						// Overwrite file
						savesdffile(SAVE_TABLE_AS_SDF,SAVE_OVERWRITE,sdfDir,modelname,sdfile,saveasfilename,
							    backupName,savefileaddr,savetimeaddr,reloadtimeaddr);
						sdfFileCrc = checkFileCrc(sdffileloaded);
						status = writeTable2File(sdfDir,bufile,SDF_WITH_INIT_FLAG,cdTable);
					}
					// noMon = createSortTableEntries(chNum,wcVal,wcstring);
					clearTableSelections(noMon,unMonChans, selectCounter);
					confirmVal = 0;
					status = dbPutField(&confirmwordaddr,DBR_LONG,&confirmVal,1);
					// Calculate and report number of channels NOT being monitored.
				}
				status = dbPutField(&sorttableentriesaddr,DBR_LONG,&noMon,1);
				lastTable = SDF_TABLE_NOT_MONITORED;
				break;
			case SDF_TABLE_FULL:
				if(lastTable != SDF_TABLE_FULL) {
					clearTableSelections(cdSort,cdTableList, selectCounter);
					confirmVal = 0;
				}
				cdSort = spChecker(monFlag,cdTableList,wcVal,wcstring,1,&status);
				pageDisp = reportSetErrors(pref, cdSort, cdTableList,pageNum);
				status = dbGetField(&resetoneaddr,DBR_LONG,&resetNum,&ropts,&nvals,NULL);
				if(selectAll == 3) {
					selectAll = 0;
					status = dbPutField(&selectaddr[3],DBR_LONG,&selectAll,1);
				}
				if(resetNum > 200) {
					decodeChangeSelect(resetNum, pageDisp, cdSort, cdTableList,selectCounter);
				}
				if(resetNum) {
					resetNum = 0;
					status = dbPutField(&resetoneaddr,DBR_LONG,&resetNum,1);
				}
				if(confirmVal) {
					if(selectCounter[2] && (confirmVal & 2)) {
						// Save present table as timenow.
						status = dbGetField(&loadedfile_addr,DBR_STRING,backupName,&ropts,&nvals,NULL);
						// printf("BACKING UP: %s\n",backupName);
						savesdffile(SAVE_TABLE_AS_SDF,SAVE_TIME_NOW,sdfDir,modelname,sdfile,saveasfilename,
							    backupName,savefileaddr,savetimeaddr,reloadtimeaddr);
						// Overwrite the table with new values
						status = modifyTable(cdSort,cdTableList);
						// Overwrite file
						savesdffile(SAVE_TABLE_AS_SDF,SAVE_OVERWRITE,sdfDir,modelname,sdfile,saveasfilename,
							    backupName,savefileaddr,savetimeaddr,reloadtimeaddr);
						sdfFileCrc = checkFileCrc(sdffileloaded);
						status = writeTable2File(sdfDir,bufile,SDF_WITH_INIT_FLAG,cdTable);
					}
					clearTableSelections(cdSort,cdTableList, selectCounter);
					confirmVal = 0;
					status = dbPutField(&confirmwordaddr,DBR_LONG,&confirmVal,1);
					noMon = createSortTableEntries(chNum,wcVal,wcstring,&noInit);
					// Calculate and report number of channels NOT being monitored.
					status = dbPutField(&monchancntaddr,DBR_LONG,&chNotMon,1);
				}
				status = dbPutField(&sorttableentriesaddr,DBR_LONG,&cdSort,1);
				lastTable = SDF_TABLE_FULL;
				break;
			default:
				pageDisp = reportSetErrors(pref, sperror,setErrTable,pageNum);
				status = dbPutField(&sorttableentriesaddr,DBR_LONG,&sperror,1);
				break;
		}
		if(pageDisp != pageNum) {
			pageNum = pageDisp;
		}
		freezeTable = 0;
		for(ii=0;ii<3;ii++) {
			freezeTable += selectCounter[ii];
			status = dbPutField(&selectaddr[ii],DBR_LONG,&selectCounter[ii],1);
		}
		status = dbPutField(&pagelockaddr,DBR_LONG,&freezeTable,1);

		// Check file CRCs every 5 seconds.
		// DAQ and COEFF file checking was moved from skeleton.st to here RCG V2.9.
		if(!fivesectimer) {
			status = checkFileCrc(daqFile);
			if(status != daqFileCrc) {
				daqFileCrc = status;
				status = dbPutField(&daqmsgaddr,DBR_STRING,modfilemsg,1);
				logFileEntry("Detected Change to DAQ Config file.");
			}
			status = checkFileCrc(coeffFile);
			if(status != coeffFileCrc) {
				coeffFileCrc = status;
				status = dbPutField(&coeffmsgaddr,DBR_STRING,modfilemsg,1);
				logFileEntry("Detected Change to Filter Coeff file.");
			}
			status = checkFileCrc(sdffileloaded);
			if(status == -1) {
				sdfFileCrc = status;
				logFileEntry("SDF file not found.");
				dbPutField(&reloadtimeaddr,DBR_STRING,"File Not Found",1);
			} 
			if(status != sdfFileCrc) {
				sdfFileCrc = status;
				logFileEntry("Detected Change to SDF file.");
				dbPutField(&reloadtimeaddr,DBR_STRING,modfilemsg,1);
			}
		}
	}
	sleep(0xfffffff);
    } else
    	iocsh(NULL);
    return(0);
}
