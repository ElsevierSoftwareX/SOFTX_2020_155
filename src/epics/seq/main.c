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
// File save on exit.

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
#define SDF_ERR_TSIZE		400	///< Size of error reporting tables.

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
	int error;
	char errMsg[64];
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
} SET_ERR_TABLE;


// Gloabl variables		****************************************************************************************
int chNum = 0;			///< Total number of channels held in the local lookup table.
int chMon = 0;			///< Total number of channels being monitored.
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
SET_ERR_TABLE unMonChans[SDF_ERR_TSIZE];	///< Table used to report channels not being monitored.
SET_ERR_TABLE readErrTable[SDF_ERR_TSIZE];	///< Table used to report file read errors..

#define SDF_NUM		0
#define SDF_STR		1

// Function prototypes		****************************************************************************************
int checkFileCrc(char *);
unsigned int filtCtrlBitConvert(unsigned int);
void getSdfTime(char *);
void logFileEntry(char *);
int getEpicsSettings(int);
int writeTable2File(char *,int,CDS_CD_TABLE *);
int savesdffile(int,int,char *,char *,char *,char *,char *,dbAddr,dbAddr); 
int createSortTableEntries(int);
int reportSetErrors(char *,int,SET_ERR_TABLE *,int);
int spChecker(int);
void newfilterstats(int);
int writeEpicsDb(int,CDS_CD_TABLE *,int);
int readConfig( char *,char *,int);
void dbDumpRecords(DBBASE *);
int parseLine(char *,char *,char *,char *,char *,char *,char *);

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
int ii;
int mychar = 0;
char psd[6][64];
#define IS_A_ALPHA_NUM 	0
#define IS_A_SPACE 	1
#define IS_A_QUOTE	2

	while (*s != 0) {
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
				if(inQuotes) {
					strncat(psd[wc],s,1);
				} else {
					lastwasspace = 1;
				}
				break;
			case IS_A_QUOTE:
				inQuotes ^= 1;
				if(inQuotes == 0) lastwasspace = 1;
				break;
		}
		s ++;
	}
	wc ++;
	sprintf(str1,"%s",psd[0]);
	sprintf(str2,"%s",psd[1]);
	sprintf(str3,"%s",psd[2]);
	sprintf(str4,"%s",psd[3]);
	sprintf(str5,"%s",psd[4]);
	sprintf(str6,"%s",psd[5]);
	// printf("WC = %d\n%s \t%s\t%s\t%s\t%s\n",wc,psd[0],psd[1],psd[2],psd[3],psd[4]);
	return(wc);
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

/// Routine to check filter switch settings and provide info on switches not set as requested.
/// @param[in] fcount	Number of filters in the control model.
/// @param[in] monitorAll	Global monitoring flag.
/// @return	Number of errant switch settings found.
int checkFilterSwitches(int fcount, int monitorAll)
{
unsigned int refVal;
unsigned int presentVal;
unsigned int x,y;
int ii,jj;
int errCnt = 0;
char swname[2][64];
char tmpname[64];
char swstate[2][4] = {"OFF","ON"};
time_t mtime;
char localtimestring[256];
struct buffer {
	DBRstatus
	DBRtime
	unsigned int rval;
	}buffer[2];
long options = DBR_STATUS|DBR_TIME;
dbAddr paddr;
dbAddr paddr1;
long nvals = 1;
long status;
// Define names for SWSTAT bits.
char bitDef[17][12] = { " [FM1]"," [FM2]", " [FM3]", " [FM4]",
			" [FM5]"," [FM6]", " [FM7]", " [FM8]",
			" [FM9]", " [FM10]"," [INPUT]", " [OFFSET]",
			" [OUTPUT]"," [LIMIT]"," [CLR]"," [DEC]",
			" [HOLD]"};
// Map SWSTAT bits as being part os SW1S or SW2S
int sw2record[17] = {0,0,0,0,
		    0,0,1,1,	
		    1,1,0,0,	
		    1,1,0,1,1};	

	for(ii=0;ii<fcount;ii++)
	{
		if(filterTable[ii].init && (filterTable[ii].mask || monitorAll))
		{
			bzero(swname[0],strlen(swname[0]));
			bzero(swname[1],strlen(swname[1]));
			strcpy(swname[0],filterTable[ii].fname);
			strcat(swname[0],"SW1S");
			strcpy(swname[1],filterTable[ii].fname);
			strcat(swname[1],"SW2S");
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
			if(refVal != filterTable[ii].swreq && errCnt < SDF_ERR_TSIZE)
			{
				x = refVal;
				y = filterTable[ii].swreq;
				for(jj=0;jj<17;jj++)
				{
					if((x & 1) != (y & 1) && errCnt < SDF_ERR_TSIZE)
					{
						bzero(tmpname,sizeof(tmpname));
						strncpy(tmpname,filterTable[ii].fname,(strlen(filterTable[ii].fname)-1));
						strcat(tmpname,bitDef[jj]);

						sprintf(setErrTable[errCnt].chname,"%s", tmpname);
						sprintf(setErrTable[errCnt].burtset, "%s", swstate[(y&1)]);
						sprintf(setErrTable[errCnt].liveset, "%s", swstate[(x&1)]);
						setErrTable[errCnt].sigNum = filterTable[ii].sw[sw2record[jj]];

						mtime = buffer[sw2record[jj]].time.secPastEpoch + POSIX_TIME_AT_EPICS_EPOCH;
						strcpy(localtimestring, ctime(&mtime));
						localtimestring[strlen(localtimestring) - 1] = 0;
						sprintf(setErrTable[errCnt].timeset, "%s", localtimestring);
						sprintf(setErrTable[errCnt].diff, "%s", "               ");
						errCnt ++;
					}
					x = x >> 1;
					y = y >> 1;
				}
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
int writeTable2File(char *filename, 		///< Name of file to write
		    int ftype, 			///< Type of file to write
		    CDS_CD_TABLE myTable[])	///< Table to be written.
{
        int ii;
        FILE *csFile;
        char filemsg[128];
	char timestring[128];
    // Write out local monitoring table as spap file.
        errno=0;
        csFile = fopen(filename,"w");
        if (csFile == NULL)
        {
            sprintf(filemsg,"ERROR Failed to open %s - %s",filename,strerror(errno));
            logFileEntry(filemsg);
	    return(-1);
        }
	if(ftype != SDF_WITH_INIT_FLAG) {
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
		fprintf(csFile,"%s\n","Directory /home/controls ");
		fprintf(csFile,"%s\n","Req File: autoBurt.req ");
		fprintf(csFile,"%s\n","--- End BURT header");
	}
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
			if(myTable[ii].datatype == SDF_NUM)
				fprintf(csFile,"%s %d %.15e %d\n",myTable[ii].chname,1,myTable[ii].data.chval,myTable[ii].mask);
			else
				fprintf(csFile,"%s %d \"%s\" %d\n",myTable[ii].chname,1,myTable[ii].data.strval,myTable[ii].mask);
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
// Savetype
#define SAVE_TABLE_AS_SDF	1
#define SAVE_TABLE_AS_BURT	2
#define SAVE_EPICS_AS_SDF	3
#define SAVE_EPICS_AS_BURT	4
// Saveopts
#define SAVE_TIME_NOW		1
#define SAVE_OVERWRITE		2
#define SAVE_AS			3

/// Routine used to decode and handle BURT save requests.
int savesdffile(int saveType, 		///< Save file format definition.
		int saveOpts, 		///< Save file options.
		char *sdfdir, 		///< Directory to save file in.
		char *model, 		///< Name of the model used to build file name.
		char *currentfile, 	///< Name of file last read (Used if option is to overwrite).
		char *saveasfile,	///< Name of file to be saved.
		char *currentload,	///< Name of file, less directory info.
		dbAddr sfaddr,		///< Address of EPICS channel to write save file name.
		dbAddr staddr)		///< Address of EPICS channel to write save file time.
{
char filename[256];
char ftype[16];
int status;
char filemsg[128];
char timestring[64];
char shortfilename[64];

	time_t now = time(NULL);
	struct tm *mytime  = localtime(&now);

	// Figure out the name of the file to save *******************************************************
	if(saveType == 1 || saveType == 3) sprintf(ftype,"%s","sdf");
	else sprintf(ftype,"%s","burt");

	switch(saveOpts)
	{
		case SAVE_TIME_NOW:
			sprintf(filename,"%s%s_%s_%d%02d%02d_%02d%02d%02d.snap", sdfdir,model,ftype,
			(mytime->tm_year - 100),  (mytime->tm_mon + 1),  mytime->tm_mday,  mytime->tm_hour,  mytime->tm_min,  mytime->tm_sec);
			sprintf(shortfilename,"%s_%s_%d%02d%02d_%02d%02d%02d", model,ftype,
			(mytime->tm_year - 100),  (mytime->tm_mon + 1),  mytime->tm_mday,  mytime->tm_hour,  mytime->tm_min,  mytime->tm_sec);
			printf("File to save is TIME NOW: %s\n",filename);
			break;
		case SAVE_OVERWRITE:
			sprintf(filename,"%s",currentfile);
			sprintf(shortfilename,"%s",currentload);
			printf("File to save is OVERWRITE: %s\n",filename);
			break;
		case SAVE_AS:
			sprintf(filename,"%s%s_%s.snap",sdfdir,saveasfile,ftype);
			sprintf(shortfilename,"%s_%s",saveasfile,ftype);
			printf("File to save is SAVE_AS: %s\n",filename);
			break;

		default:
			return(-1);
	}
	// SAVE THE DATA TO FILE **********************************************************************
	switch(saveType)
	{
		case SAVE_TABLE_AS_SDF:
			printf("Save table as sdf\n");
			status = writeTable2File(filename,SDF_FILE_PARAMS_ONLY,cdTable);
			if (!status) 
                        {    
                            sprintf(filemsg,"Save TABLE as SDF: %s",filename);
			}
			else
                        {
                            sprintf(filemsg,"FAILED FILE SAVE %s",filename);
			    logFileEntry(filemsg);
                            return(-2);
			}
                        break;
		case SAVE_TABLE_AS_BURT:
			printf("Save table as burt\n");
			status = writeTable2File(filename,SDF_FILE_BURT_ONLY,cdTable);
			if (!status) 
                        {    
			    sprintf(filemsg,"Save TABLE as BURT: %s",filename);
                        }
			else
                        {
                            sprintf(filemsg,"FAILED FILE SAVE %s",filename);
			    logFileEntry(filemsg);
                            return(-2);
			}
			break;
		case SAVE_EPICS_AS_SDF:
			printf("Save epics as sdf\n");
			status = getEpicsSettings(chNum);
			status = writeTable2File(filename,SDF_FILE_PARAMS_ONLY,cdTableP);
			if (!status) 
                        {    
			    sprintf(filemsg,"Save EPICS as SDF: %s",filename);
			}
			else
                        {
                            sprintf(filemsg,"FAILED FILE SAVE %s",filename);
			    logFileEntry(filemsg);
                            return(-2);
			}
                        break;
		case SAVE_EPICS_AS_BURT:
			printf("Save epics as burt\n");
			status = getEpicsSettings(chNum);
			status = writeTable2File(filename,SDF_FILE_BURT_ONLY,cdTableP);
			if (!status)
                        {
                            sprintf(filemsg,"Save EPICS as BURT: %s",filename);
			}
			else
                        {
                            sprintf(filemsg,"FAILED FILE SAVE %s",filename);
			    logFileEntry(filemsg);
                            return(-2);
			}
                        break;
		default:
			sprintf(filemsg,"BAD SAVE REQUEST %s",filename);
			logFileEntry(filemsg);
			return(-1);
	}
	logFileEntry(filemsg);
	getSdfTime(timestring);
	printf(" Time of save = %s\n",timestring);
	status = dbPutField(&sfaddr,DBR_STRING,shortfilename,1);
	status = dbPutField(&staddr,DBR_STRING,timestring,1);
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

/// Routine used to create local tables for reporting on request.
int createSortTableEntries(int numEntries)
{
int jj;
int notMon = 0;

	chNotInit = 0;
	chMon = 0;

	// Clear out the uninit and unmon tables.
	for(jj=0;jj<SDF_ERR_TSIZE;jj++)
	{
		sprintf(uninitChans[chNotInit].chname,"%s"," ");
		sprintf(uninitChans[chNotInit].burtset,"%s"," ");
		sprintf(uninitChans[chNotInit].liveset,"%s"," ");
		sprintf(uninitChans[chNotInit].timeset,"%s"," ");
		sprintf(uninitChans[chNotInit].diff,"%s"," ");
		sprintf(unMonChans[notMon].chname,"%s"," ");
		sprintf(unMonChans[notMon].burtset,"%s"," ");
		sprintf(unMonChans[notMon].liveset,"%s"," ");
		sprintf(unMonChans[notMon].timeset,"%s"," ");
		sprintf(unMonChans[notMon].diff,"%s"," ");
	}

	// Fill uninit and unmon tables.
	for(jj=0;jj<numEntries;jj++)
	{
		if(!cdTable[jj].initialized) {
			// printf("Chan %s not init %d %d %d\n",cdTable[jj].chname,cdTable[jj].initialized,jj,numEntries);
			if(chNotInit < SDF_ERR_TSIZE) sprintf(uninitChans[chNotInit].chname,"%s",cdTable[jj].chname);
			chNotInit ++;
		}
		if(cdTable[jj].initialized && cdTable[jj].mask) chMon ++;
		if(cdTable[jj].initialized && !cdTable[jj].mask) {
			if(notMon < SDF_ERR_TSIZE) {
				sprintf(unMonChans[notMon].chname,"%s",cdTable[jj].chname);
				if(cdTable[jj].datatype == SDF_NUM)
				{
					if(cdTable[jj].filterswitch) sprintf(unMonChans[notMon].burtset,"0x%x",(unsigned int)cdTable[jj].data.chval);
					else sprintf(unMonChans[notMon].burtset,"%.10lf",cdTable[jj].data.chval);
				} else {
					sprintf(unMonChans[notMon].burtset,"%s",cdTable[jj].data.strval);
				}
			}
			notMon ++;
		}
	}
	return(notMon);
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


	// Get the page number to display
	mypage = page;
	// Calculat start index to the diff table.
	myindex = page *  SDF_ERR_DSIZE;
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
int spChecker(int monitorAll)
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

	// Check filter switch settings first
	     errCntr = checkFilterSwitches(fmNum,monitorAll);
	     if(chNum) {
		for(ii=0;ii<chNum;ii++) {
			if((errCntr < SDF_ERR_TSIZE) && 		// Within table max size
			  ((cdTable[ii].mask != 0) || (monitorAll)) && 	// Channel is to be monitored
			   cdTable[ii].initialized && 			// Channel was set in BURT
			   cdTable[ii].filterswitch == 0)		// Not a filter switch channel
			{
				localErr = 0;
				// Find address of channel
				status = dbNameToAddr(cdTable[ii].chname,&paddr);
				// If this is a digital data type, then get as double.
				if(cdTable[ii].datatype == SDF_NUM)
				{
					status = dbGetField(&paddr,DBR_DOUBLE,&buffer,&options,&nvals,NULL);
					if(cdTable[ii].data.chval != buffer.rval)
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
					if(strcmp(cdTable[ii].data.strval,strbuffer.sval) != 0)
					{
						sprintf(burtset,"%s",cdTable[ii].data.strval);
						sprintf(liveset,"%s",strbuffer.sval);
						sprintf(diffB2L,"%s","                                   ");
						mtime = strbuffer.time.secPastEpoch + POSIX_TIME_AT_EPICS_EPOCH;
						localErr = 1;
					}
				}
				// If a diff was found, then write info the EPICS setpoint diff table.
				if(localErr)
				{
					sprintf(setErrTable[errCntr].chname,"%s", cdTable[ii].chname);

					sprintf(setErrTable[errCntr].burtset, "%s", burtset);

					sprintf(setErrTable[errCntr].liveset, "%s", liveset);

					sprintf(setErrTable[errCntr].diff, "%s", diffB2L);
					setErrTable[errCntr].sigNum = ii;

					strcpy(localtimestring, ctime(&mtime));
					localtimestring[strlen(localtimestring) - 1] = 0;
					sprintf(setErrTable[errCntr].timeset, "%s", localtimestring);
					errCntr ++;
				}
			}
		}
	     }
	return(errCntr);
}
int resetSelectedValues(int errNum)
{
long status;
int ln = errNum - 1;
int ii = setErrTable[ln].sigNum;
dbAddr saddr;
	
	status = dbNameToAddr(cdTable[ii].chname,&saddr);
	if(cdTable[ii].datatype == SDF_NUM) status = dbPutField(&saddr,DBR_DOUBLE,&cdTable[ii].data.chval,1);
	else status = dbPutField(&saddr,DBR_STRING,&cdTable[ii].data.strval,1);
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

	for(ii=0;ii<numchans;ii++) {
		if(filterTable[ii].newSet) {
			counter ++;
			filterTable[ii].newSet = 0;
			filterTable[ii].init = 1;
			tmpreq =  ((unsigned int)cdTable[filterTable[ii].sw[0]].data.chval & 0xffff) + 
				(((unsigned int)cdTable[filterTable[ii].sw[1]].data.chval & 0xffff) << 16);
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
			// printf("New filter %d %s = 0x%x\t0x%x\t0x%x\n",ii,filterTable[ii].fname,filterTable[ii].swreq,filterTable[ii].sw1,filterTable[ii].sw2);
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
		int command)		///< Read file request type.
{
	FILE *cdf;
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
	int qcnt = 0;

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
			lderror = 2;
			return(lderror);
		}
		chNumP = 0;
		alarmCnt = 0;
		// Put dummy in s4 as this column may or may not exist.
		strcpy(s4,"x");
		bzero(s3,strlen(s3));
		strncpy(ifo,pref,3);
		while(fgets(line,sizeof line,cdf) != NULL)
		{
			isalarm = 0;
			strcpy(s4,"x");
			argcount = parseLine(line,s1,s2,s3,s4,s5,s6);
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
						strcpy(cdTableP[chNumP].data.strval,s3);
						// printf("Alarm set - %s = %s\n",cdTableP[chNumP].chname,cdTableP[chNumP].data.strval);
					}
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
							cdTableP[chNumP].datatype = SDF_STR;
							strcpy(cdTableP[chNumP].data.strval,s3);
							if(command != SDF_LOAD_DB_ONLY)
								strcpy(cdTable[ii].data.strval,cdTableP[chNumP].data.strval);
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
							filterTable[fmIndex].mask =  cdTableP[chNumP].mask;
							break;
						}
					}
				}
				chNumP ++;
				if(chNumP >= SDF_MAX_CHANS)
				{
					fclose(cdf);
					sprintf(errMsg,"Number of channels in %s exceeds program limit\n",sdfile);
					logFileEntry(errMsg);
					lderror = 2;
					return(lderror);
				}
		   	}
		}
		fclose(cdf);
		printf("Loading epics %d\n",chNumP);
		lderror = writeEpicsDb(chNumP,cdTableP,command);
		// if(!fmtInit) newfilterstats(fmNum);
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

    logFileEntry("************* Software Restart ************");
    // By convention, the RCG produces ai and bi records for settings.
    sprintf(mytype[0],"%s","ai");
    sprintf(mytype[1],"%s","bi");
    sprintf(mytype[2],"%s","stringin");
    pdbentry = dbAllocEntry(pdbbase);

    chNum = 0;
    fmNum = 0;
    for(ii=0;ii<3;ii++) {
    status = dbFindRecordType(pdbentry,mytype[ii]);

    // status = dbFirstRecordType(pdbentry);
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
                // printf("\n  Record:%s\n",dbGetRecordName(pdbentry));
		sprintf(cdTable[chNum].chname,"%s",dbGetRecordName(pdbentry));
		cdTable[chNum].filterswitch = 0;
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
       //  status = dbNextRecordType(pdbentry);
    }
}
    fmNum = 0;
    for(ii=0;ii<chNum;ii++) {
	if(cdTable[ii].filterswitch == 1)
	{
		strncpy(filterTable[fmNum].fname,cdTable[ii].chname,(strlen(cdTable[ii].chname)-4));
		sprintf(tmpstr,"%s%s",filterTable[fmNum].fname,"SW2S");
		filterTable[fmNum].sw[0] = ii;
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
    sprintf(errMsg,"Number of filter modules in db = %d \n",fmNum);
    logFileEntry(errMsg);
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
	dbAddr chnotfoundaddr;
	dbAddr chnotinitaddr;
	dbAddr sorttableentriesaddr;
	dbAddr monflagaddr;
	dbAddr reloadtimeaddr;
	dbAddr edbloadedaddr;
	dbAddr savecmdaddr;
	dbAddr saveasaddr;
	dbAddr savetypeaddr;
	dbAddr saveoptsaddr;
	dbAddr savefileaddr;
	dbAddr savetimeaddr;
	dbAddr daqmsgaddr;
	dbAddr coeffmsgaddr;
	dbAddr resetoneaddr;
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
	// FILE *csFile;
	int ii;
	int setChans = 0;
	char tsrString[64];
	int tsrVal = 0;
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
	int numReport = 0;

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
	char bufile[256];
	char saveasfilename[128];
	printf("My prefix is %s\n",pref);
	sprintf(sdfile, "%s%s%s", sdfDir, sdf,".snap");					// Initialize with BURT_safe.snap
	sprintf(bufile, "%s%s", sdfDir, "fec.snap");					// Initialize table dump file
	sprintf(logfilename, "%s%s", logdir, "/ioc.log");					// Initialize table dump file
	printf("SDF FILE = %s\n",sdfile);
	printf("CURRENt FILE = %s\n",bufile);
	printf("LOG FILE = %s\n",logfilename);
sleep(5);
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

	char saveasname[256]; sprintf(saveasname, "%s_%s", pref, "SDF_SAVE_AS_NAME");	// SDF Save as file name.
	// Clear out the save as file name request
	status = dbNameToAddr(saveasname,&saveasaddr);			// Get Address.
	status = dbPutField(&saveasaddr,DBR_STRING,"default",1);	// Set as dummy 'default'

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

	char modcoefffilemsg[256]; sprintf(modcoefffilemsg, "%s_%s", pref, "MSG");	// Record to write if Coeff file changed.
	status = dbNameToAddr(modcoefffilemsg,&coeffmsgaddr);

	sprintf(timechannel,"%s_%s", pref, "TIME_STRING");
	// printf("timechannel = %s\n",timechannel);
	sprintf(reloadtimechannel,"%s_%s", pref, "SDF_RELOAD_TIME");			// Time of last BURT reload
	status = dbNameToAddr(reloadtimechannel,&reloadtimeaddr);

	unsigned int pageNum = 0;
	dbAddr pagereqaddr;
        char pagereqname[256]; sprintf(pagereqname, "%s_%s", pref, "SDF_PAGE"); // SDF Save command.
        status = dbNameToAddr(pagereqname,&pagereqaddr);                // Get Address.

	unsigned int resetNum = 0;
	char resetOneName[256]; sprintf(resetOneName, "%s_%s", pref, "SDF_RESET_CHAN");	// SDF reset one value.
	status = dbNameToAddr(resetOneName,&resetoneaddr);
	status = dbPutField(&resetoneaddr,DBR_LONG,&resetNum,1);

	dbDumpRecords(*iocshPpdbbase);

	// Initialize DAQ and COEFF file CRC checksums for later compares.
	daqFileCrc = checkFileCrc(daqFile);
	coeffFileCrc = checkFileCrc(coeffFile);
	reportSetErrors(pref, 0,setErrTable,0);

	sleep(1);       // Need to wait before first restore to allow sequencers time to do their initialization.

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
		// Check if file name != to one presently loaded
		if(strcmp(sdf,loadedSdf) != 0) burtstatus |= 1;
		else burtstatus &= ~(1);
		// if(burtstatus & 1) status = dbPutField(&reloadtimeaddr,DBR_STRING,"New SDF File Pending",1);
		switch(burtstatus) {
			case 1:
				status = dbPutField(&reloadtimeaddr,DBR_STRING,"New SDF File Pending",1);
				break;
			case 2:
			case 3:
				status = dbPutField(&reloadtimeaddr,DBR_STRING,"Read Error: File Not Found",1);
				break;
			default:
				break;
		}
	
		if(request != 0) {		// If there is a read file request, then:
			status = dbPutField(&reload_addr,DBR_LONG,&ropts,1);	// Clear the read request.
			reqValid = 1;
			if(reqValid) {
				rdstatus = readConfig(pref,sdfile,request);
				if (rdstatus) burtstatus |= rdstatus;
				else burtstatus &= ~(6);
				if(burtstatus < 2) {
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
					status = dbPutField(&fulldbcntaddr,DBR_LONG,&chNum,1);
					// Sort channels for data reporting via the MEDM table.
					noMon = createSortTableEntries(chNum);
					// Calculate and report number of channels NOT being monitored.
					// noMon = chNum - chMon;
					status = dbPutField(&monchancntaddr,DBR_LONG,&noMon,1);
					status = dbPutField(&alrmchcountaddr,DBR_LONG,&alarmCnt,1);
					// Report number of channels in BURT file that are not in local database.
					status = dbPutField(&chnotfoundaddr,DBR_LONG,&chNotFound,1);
					// Report number of channels that have not been initialized via a BURT read.
					status = dbPutField(&chnotinitaddr,DBR_LONG,&chNotInit,1);
					// Write out local monitoring table as snap file.
					status = writeTable2File(bufile,SDF_WITH_INIT_FLAG,cdTable);
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
			if(strcmp(saveTypeString,"TABLE AS SDF") == 0) saveType = 1;
			else if(strcmp(saveTypeString,"TABLE AS BURT") == 0) saveType = 2;
			else if(strcmp(saveTypeString,"EPICS DB AS SDF") == 0) saveType = 3;
			else if(strcmp(saveTypeString,"EPICS DB AS BURT") == 0) saveType = 4;
			else saveType = 0;
			// Determine file options
			status = dbGetField(&saveoptsaddr,DBR_STRING,saveOptsString,&ropts,&nvals,NULL);
			if(strcmp(saveOptsString,"TIME NOW") == 0) saveOpts = 1;
			else if(strcmp(saveOptsString,"OVERWRITE") == 0) saveOpts = 2;
			else if(strcmp(saveOptsString,"SAVE AS") == 0) saveOpts = 3;
			else saveOpts = 0;
			// Determine if request is valid.
			if(saveType && saveOpts)
			{
				// Get saveas filename
				status = dbGetField(&saveasaddr,DBR_STRING,saveasfilename,&ropts,&nvals,NULL);
				// Save the file
				savesdffile(saveType,saveOpts,sdfDir,modelname,sdfile,saveasfilename,loadedSdf,savefileaddr,savetimeaddr);
			} else {
				logFileEntry("Invalid SAVE File Request");
			}
		}
		// Check present settings vs BURT settings and report diffs.
		// Check if MON ALL CHANNELS is set
		status = dbGetField(&monflagaddr,DBR_LONG,&monFlag,&ropts,&nvals,NULL);
		// Call the diff checking function.
		sperror = spChecker(monFlag);
		// Report number of diffs found.
		status = dbPutField(&sperroraddr,DBR_LONG,&sperror,1);
		// Table sorting and presentation
		status = dbGetField(&tablesortreqaddr,DBR_USHORT,&tsrVal,&ropts,&nvals,NULL);
		status = dbGetField(&pagereqaddr,DBR_USHORT,&pageNum,&ropts,&nvals,NULL);
		switch(tsrVal) { 
			case 0:
				numReport = reportSetErrors(pref, sperror,setErrTable,pageNum);
				status = dbPutField(&sorttableentriesaddr,DBR_LONG,&sperror,1);
				status = dbGetField(&resetoneaddr,DBR_LONG,&resetNum,&ropts,&nvals,NULL);
				if(resetNum) {
					resetNum += pageNum * SDF_ERR_DSIZE;
					if(resetNum <= sperror) status = resetSelectedValues(resetNum);
					resetNum = 0;
					status = dbPutField(&resetoneaddr,DBR_LONG,&resetNum,1);
				}
				break;
			case 1:
				numReport = reportSetErrors(pref, chNotFound,unknownChans,pageNum);
				status = dbPutField(&sorttableentriesaddr,DBR_LONG,&chNotFound,1);
				break;
			case 2:
				numReport = reportSetErrors(pref, chNotInit, uninitChans,pageNum);
				status = dbPutField(&sorttableentriesaddr,DBR_LONG,&chNotInit,1);
				break;
			case 3:
				numReport = reportSetErrors(pref, noMon, unMonChans,pageNum);
				status = dbPutField(&sorttableentriesaddr,DBR_LONG,&noMon,1);
				break;
			case 4:
				numReport = reportSetErrors(pref, rderror, readErrTable,pageNum);
				status = dbPutField(&sorttableentriesaddr,DBR_LONG,&rderror,1);
				break;
			default:
				numReport = reportSetErrors(pref, sperror,setErrTable,pageNum);
				status = dbPutField(&sorttableentriesaddr,DBR_LONG,&sperror,1);
				break;
		}
		if(numReport != pageNum) {
			pageNum = numReport;
			status = dbPutField(&pagereqaddr,DBR_USHORT,&pageNum,1);                // Init to zero.
		}
		if(resetNum) {
			resetNum = 0;
			status = dbPutField(&resetoneaddr,DBR_LONG,&resetNum,1);
		}


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
			if(status != sdfFileCrc) {
				sdfFileCrc = status;
				logFileEntry("Detected Change to SDF file.");
				status = dbPutField(&reloadtimeaddr,DBR_STRING,modfilemsg,1);
			}
		}
	}
	sleep(0xfffffff);
    } else
    	iocsh(NULL);
    return(0);
}
