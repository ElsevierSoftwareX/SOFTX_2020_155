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
// Need to check RESET and READ file yet.
// Check top with full model loads for memory and cpu usage.
// Add command error checking, command out of range, etc.
// Check autoBurt, particularly for SDF stuff
//	- Particularly returns from functions.
// ADD ability to add channels from larger files.
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

#define SDF_LOAD_DB_ONLY	4
#define SDF_READ_ONLY		2
#define SDF_RESET		3
#define SDF_LOAD_PARTIAL	1

#define SDF_MAX_CHANS		15000	///< Maximum number of settings, including alarm settings.
#define SDF_MAX_TSIZE		20000	///< Maximum number of EPICS settings records (No subfields).
#define SDF_ERR_TSIZE		40	///< Size of reporting tables.

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
const unsigned int fltrConst[15] = {16, 64, 256, 1024, 4096, 16384,
			     65536, 262144, 1048576, 4194304,
			     0x4, 0x8, 0x4000000,0x1000000,0x1 /* in sw, off sw, out sw , limit sw*/
			     };

/// Structure for holding BURT settings in local memory.
typedef struct CDS_CD_TABLE {
	char chname[128];
	int datatype;
	double chval; 
	char strval[64];
	int mask;
	int initialized;
} CDS_CD_TABLE;

/// Structure for creating/holding filter module switch settings.
typedef struct FILTER_TABLE {
	char fname[128];
	int swreq;
	int swmask;
} FILTER_TABLE;

/// Structure for table data to be presented to operators.
typedef struct SET_ERR_TABLE {
	char chname[64];
	char burtset[64];
	char liveset[64];
	char timeset[64];
} SET_ERR_TABLE;

int chNum = 0;			///< Total number of channels held in the local lookup table.
int chMon = 0;			///< Total number of channels being monitored.
int alarmCnt = 0;		///< Total number of alarm settings loaded from a BURT file.
int chNumP = 0;			///< Total number of settings loaded from a BURT file.
int fmNum = 0;			///< Total number of filter modules found.
int fmtInit = 0;		///< Flag used to indicate that the filter module table needs to be initiialized on startup.
int chNotFound = 0;		///< Total number of channels read from BURT file which did not have a database entry.
int chNotInit = 0;		///< Total number of channels not initialized by the safe.snap BURT file.
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
unsigned int filtCtrlBitConvert(unsigned int v) {
        unsigned int val = 0;
        int i;
        for (i = 0; i < 15; i++) {
                if (v & fltrConst[i]) val |= 1<<i;
        }
        return val;
}

/// Routine for reading GPS time from model EPICS record.
void getSdfTime(char *timestring)
{

	dbAddr paddr;
	long ropts = 0;
	long nvals = 1;
	long status;

	status = dbNameToAddr(timechannel,&paddr);
	status = dbGetField(&paddr,DBR_STRING,timestring,&ropts,&nvals,NULL);
}

/// Routine for logging errors to file.
void logFileEntry(char *message)
{
	FILE *log;
	char timestring[256];
	long status;
	dbAddr paddr;

	getSdfTime(timestring);
	log = fopen(logfilename,"a");
	fprintf(log,"%s\n%s\n",timestring,message);
	fprintf(log,"***************************************************\n");
	fclose(log);
	status = dbNameToAddr(reloadtimechannel,&paddr);
	status = dbPutField(&paddr,DBR_STRING,"ERR - NO FILE FOUND",1);

}


/// Function to read all settings from the EPICS database.
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
		// Find address of channel
		sprintf(cdTableP[ii].chname,"%s",cdTable[ii].chname);
		cdTableP[ii].datatype = cdTable[ii].datatype;
		cdTableP[ii].mask = cdTable[ii].mask;
		cdTableP[ii].initialized = cdTable[ii].initialized;
		// Find address of channel
		status = dbNameToAddr(cdTableP[ii].chname,&geaddr);
		if(!status) {
			if(cdTableP[ii].datatype == 0)
			{
				statusR = dbGetField(&geaddr,DBR_DOUBLE,&dval,NULL,&nvals,NULL);
				if(!statusR) cdTableP[ii].chval = dval;
			} else {
				statusR = dbGetField(&geaddr,DBR_STRING,&sval,NULL,&nvals,NULL);
				if(!statusR) sprintf(cdTableP[ii].strval,"%s",sval);
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

	// Write out local monitoring table as snap file.
	csFile = fopen(filename,"w");
	for(ii=0;ii<chNum;ii++)
	{
		// printf("%s datatype is %d\n",myTable[ii].chname,myTable[ii].datatype);
		char tabs[8];
		if(strlen(myTable[ii].chname) > 39) sprintf(tabs,"%s","\t");
		else sprintf(tabs,"%s","\t\t");

		switch(ftype)
		{
		   case SDF_WITH_INIT_FLAG:
			if(myTable[ii].datatype == 0)
				fprintf(csFile,"%s%s%d\t%.15e\t\%d\t%d\n",myTable[ii].chname,tabs,1,myTable[ii].chval,myTable[ii].mask,myTable[ii].initialized);
			else
				fprintf(csFile,"%s%s%d\t%s\t\%d\t%d\n",myTable[ii].chname,tabs,1,myTable[ii].strval,myTable[ii].mask,myTable[ii].initialized);
			break;
		   case SDF_FILE_PARAMS_ONLY:
			if(myTable[ii].datatype == 0)
				fprintf(csFile,"%s%s%d\t%.15e\t%d\n",myTable[ii].chname,tabs,1,myTable[ii].chval,myTable[ii].mask);
			else
				fprintf(csFile,"%s%s%d\t%s\t\%dn",myTable[ii].chname,tabs,1,myTable[ii].strval,myTable[ii].mask);
			break;
		   case SDF_FILE_BURT_ONLY:
			if(myTable[ii].datatype == 0)
				fprintf(csFile,"%s%s%d\t%.15e\n",myTable[ii].chname,tabs,1,myTable[ii].chval);
			else
				fprintf(csFile,"%s%s%d\t%s\n",myTable[ii].chname,tabs,1,myTable[ii].strval);
			break;
		   default:
			break;
		}
	}
	fclose(csFile);
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
		char *currentload,
		dbAddr sfaddr,
		dbAddr staddr)
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
			sprintf(filemsg,"Save TABLE as SDF: %s",filename);
			break;
		case SAVE_TABLE_AS_BURT:
			printf("Save table as burt\n");
			status = writeTable2File(filename,SDF_FILE_BURT_ONLY,cdTable);
			sprintf(filemsg,"Save TABLE as BURT: %s",filename);
			break;
		case SAVE_EPICS_AS_SDF:
			printf("Save epics as sdf\n");
			status = getEpicsSettings(chNum);
			status = writeTable2File(filename,SDF_FILE_PARAMS_ONLY,cdTableP);
			sprintf(filemsg,"Save EPICS as SDF: %s",filename);
			break;
		case SAVE_EPICS_AS_BURT:
			printf("Save epics as burt\n");
			status = getEpicsSettings(chNum);
			status = writeTable2File(filename,SDF_FILE_BURT_ONLY,cdTableP);
			sprintf(filemsg,"Save EPICS as BURT: %s",filename);
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
void createSortTableEntries(int numEntries)
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
		sprintf(unMonChans[notMon].chname,"%s"," ");
		sprintf(unMonChans[notMon].burtset,"%s"," ");
		sprintf(unMonChans[notMon].liveset,"%s"," ");
		sprintf(unMonChans[notMon].timeset,"%s"," ");
	}

	// Fill uninit and unmon tables.
	for(jj=0;jj<numEntries;jj++)
	{
		if((!cdTable[jj].initialized) && (chNotInit < SDF_ERR_TSIZE)) {
			// printf("Chan %s not init %d %d %d\n",cdTable[jj].chname,cdTable[jj].initialized,jj,numEntries);
			sprintf(uninitChans[chNotInit].chname,"%s",cdTable[jj].chname);
			chNotInit ++;
		}
		if(cdTable[jj].mask) chMon ++;
		if((!cdTable[jj].mask) && (notMon < SDF_ERR_TSIZE)) {
			sprintf(unMonChans[notMon].chname,"%s",cdTable[jj].chname);
			notMon ++;
		}
	}
}

/// Common routine to load monitoring tables into EPICS channels for MEDM screen.
void reportSetErrors(char *pref,			///< Channel name prefix from EPICS environment. 
		     int numEntries, 			///< Number of entries in table to be reported.
		     SET_ERR_TABLE setErrTable[])	///< Which table to report to EPICS channels.
{

int ii;
dbAddr saddr;
dbAddr baddr;
dbAddr maddr;
dbAddr taddr;
char s[64];
char s1[64];
char s2[64];
char s3[64];
long status;
static int lastcount = 0;

	if(numEntries > SDF_ERR_TSIZE) numEntries = SDF_ERR_TSIZE;
	for(ii=0;ii<numEntries;ii++)
	{
		sprintf(s, "%s_%s_STAT%d", pref,"SDF_SP", ii);
		status = dbNameToAddr(s,&saddr);
		status = dbPutField(&saddr,DBR_STRING,&setErrTable[ii].chname,1);

		sprintf(s1, "%s_%s_STAT%d_BURT", pref,"SDF_SP", ii);
		status = dbNameToAddr(s1,&baddr);
		status = dbPutField(&baddr,DBR_STRING,&setErrTable[ii].burtset,1);

		sprintf(s2, "%s_%s_STAT%d_LIVE", pref,"SDF_SP", ii);
		status = dbNameToAddr(s2,&maddr);
		status = dbPutField(&maddr,DBR_STRING,&setErrTable[ii].liveset,1);

		sprintf(s3, "%s_%s_STAT%d_TIME", pref,"SDF_SP", ii);
		status = dbNameToAddr(s3,&taddr);
		status = dbPutField(&taddr,DBR_STRING,&setErrTable[ii].timeset,1);
	}
	// Clear out error fields if present errors < previous errors
	if(lastcount > numEntries) {
		for(ii=numEntries;ii<lastcount;ii++)
		{
			sprintf(s, "%s_%s_STAT%d", pref,"SDF_SP", ii);
			status = dbNameToAddr(s,&saddr);
			status = dbPutField(&saddr,DBR_STRING," ",1);

			sprintf(s1, "%s_%s_STAT%d_BURT", pref,"SDF_SP", ii);
			status = dbNameToAddr(s1,&baddr);
			status = dbPutField(&baddr,DBR_STRING," ",1);

			sprintf(s2, "%s_%s_STAT%d_LIVE", pref,"SDF_SP", ii);
			status = dbNameToAddr(s2,&maddr);
			status = dbPutField(&maddr,DBR_STRING," ",1);


			sprintf(s3, "%s_%s_STAT%d_TIME", pref,"SDF_SP", ii);
			status = dbNameToAddr(s3,&taddr);
			status = dbPutField(&taddr,DBR_STRING," ",1);
		}
	}
	lastcount = numEntries;
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

	     if(chNum) {
		for(ii=0;ii<chNum;ii++) {
			if((errCntr < SDF_ERR_TSIZE) && ((cdTable[ii].mask != 0) || (monitorAll)) && cdTable[ii].initialized)
			{
				localErr = 0;
				// Find address of channel
				status = dbNameToAddr(cdTable[ii].chname,&paddr);
				// If this is a digital data type, then get as double.
				if(cdTable[ii].datatype == 0)
				{
					status = dbGetField(&paddr,DBR_DOUBLE,&buffer,&options,&nvals,NULL);
					if(cdTable[ii].chval != buffer.rval)
					{
						sprintf(burtset,"%.6f",cdTable[ii].chval);
						sprintf(liveset,"%.6f",buffer.rval);
						mtime = buffer.time.secPastEpoch + POSIX_TIME_AT_EPICS_EPOCH;
						localErr = 1;
					}
				// If this is a string type, then get as string.
				} else {
					status = dbGetField(&paddr,DBR_STRING,&strbuffer,&options,&nvals,NULL);
					if(strcmp(cdTable[ii].strval,strbuffer.sval) != 0)
					{
						sprintf(burtset,"%s",cdTable[ii].strval);
						sprintf(liveset,"%s",strbuffer.sval);
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

/// This function sets filter module request fields to aid in decoding errant filter module switch settings.
int newfilterstats(int numchans,int command) {
	dbAddr paddr;
	long status;
	int myerror = 0;
	int ii;
	FILE *log;
	char chname[128];
	int mask = 0x3fff;

	if(command == SDF_RESET) return(0);
	for(ii=0;ii<numchans;ii++) {
		bzero(chname,strlen(chname));
		// Find address of channel
		strcpy(chname,filterTable[ii].fname);
		strcat(chname,"SWREQ");
		status = dbNameToAddr(chname,&paddr);
		if(!status)
		{
			status = dbPutField(&paddr,DBR_LONG,&filterTable[ii].swreq,1);
		} else {				// Write errors to log file.
			myerror = 4;
			log = fopen("./ioc.log","a");
			fprintf(log,"CDF Load Error - Channel Not Found: %s\n",chname);
			fclose(log);
		}
		bzero(chname,strlen(chname));
		// Find address of channel
		strcpy(chname,filterTable[ii].fname);
		strcat(chname,"SWMASK");
		status = dbNameToAddr(chname,&paddr);
		if(!status)
		{
			status = dbPutField(&paddr,DBR_LONG,&mask,1);
		} else {				// Write errors to log file.
			myerror = 4;
			log = fopen("./ioc.log","a");
			fprintf(log,"CDF Load Error - Channel Not Found: %s\n",chname);
			fclose(log);
		}
	}
	return(myerror);
}

/// This function writes BURT settings to EPICS records.
int newvalue(int numchans,		///< Number of channels to write
	     CDS_CD_TABLE myTable[],	///< Table with data to be written.
	     int command) 		///< Write request.
{
	dbAddr paddr;
	long status;
	double newVal;
	int ii;
	FILE *log;
	int myerror = 0;
	int mychans = 0;

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
				   if(myTable[ii].datatype == 0)	// Value if floating point number
				   {
					status = dbPutField(&paddr,DBR_DOUBLE,&myTable[ii].chval,1);
				   } else {			// Value is a string type
					status = dbPutField(&paddr,DBR_STRING,&myTable[ii].strval,1);
				   }
				   mychans ++;
				}
				else {				// Write errors to log file.
					myerror = 4;
					if(chNotFound < SDF_ERR_TSIZE) {
						sprintf(unknownChans[chNotFound].chname,"%s",myTable[ii].chname);
						if(myTable[ii].datatype == 0)	// Value if floating point number
							sprintf(unknownChans[chNotFound].burtset,"%.6f",myTable[ii].chval);
						else
							sprintf(unknownChans[chNotFound].burtset,"%s",myTable[ii].strval);
						sprintf(unknownChans[chNotFound].liveset,"%s"," ");
						sprintf(unknownChans[chNotFound].timeset,"%s"," ");
					}
					log = fopen("./ioc.log","a");
					fprintf(log,"SDF Load Error - Channel Not Found: %s\n",myTable[ii].chname);
					fclose(log);
					chNotFound ++;
				}
			}
			printf("Wrote out %d channels of data.\n",mychans);
			return(myerror);
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
				   if(myTable[ii].datatype == 0)	// Value if floating point number
				   {
					status = dbPutField(&paddr,DBR_DOUBLE,&myTable[ii].chval,1);
				   } else {			// Value is a string type
					status = dbPutField(&paddr,DBR_STRING,&myTable[ii].strval,1);
				   }
				}
				else {				// Write errors to log file.
					myerror = 4;
					log = fopen("./ioc.log","a");
					fprintf(log,"CDF Load Error - Channel Not Found: %s\n",myTable[ii].chname);
					fclose(log);
				}
			    }
			}
			return(myerror);
			break;
		default:
			printf("newvalue setting routine got unknown request \n");
			return(0);
			break;
	}
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
	char s1[128],s2[128],s3[128],s4[128];
	dbAddr paddr;
	long status;
	int lderror = 0;
	int flderror = 0;
	int ropts = 0;
	int nvals = 1;
	int starttime,totaltime;
	char timestring[256];
	char line[128];
	char *fs;
	char ifo[4];
	double tmpreq = 0;
	char tempstr[128];
	int localCtr = 0;
	int fmatch = 0;
	char errMsg[128];

	clock_gettime(CLOCK_REALTIME,&t);
	starttime = t.tv_nsec;

	getSdfTime(timestring);

	switch(command)
	{
		case SDF_LOAD_DB_ONLY:
		// Read the file and set EPICS records. DO NOT update the local setpoints table.
			// printf("LOAD_ONLY = %s\n",sdfile);
			cdf = fopen(sdfile,"r");
			if(cdf == NULL) {
				sprintf(errMsg,"New SDF request ERROR: FILE %s DOES NOT EXIST\n",sdfile);
				logFileEntry(errMsg);
				lderror = 2;
				return(lderror);
			}
			chNumP = 0;
			alarmCnt = 0;
			strcpy(s4,"x");
			strncpy(ifo,pref,3);
			// Read the settings file
			while(fgets(line,sizeof line,cdf) != NULL)
			{
				// Put dummy in s4 as this column may or may not exist.
				strcpy(s4,"x");
				sscanf(line,"%s%s%s%s",s1,s2,s3,s4);
				// If 1st three chars match IFO ie checking this this line is not BURT header or channel marked RO
				if(	strncmp(s1,ifo,3) == 0 && 
					// Don't allow load of SWSTAT or SWMASK, which are set by this program.
					strstr(s1,"_SWMASK") == NULL &&
					strstr(s1,"_SDF_NAME") == NULL &&
					strstr(s1,"_SWREQ") == NULL)
				{
					// Clear out the local tabel channel name string.
					bzero(cdTableP[chNumP].chname,strlen(cdTableP[chNumP].chname));
					// Load channel name into local table.
					strcpy(cdTableP[chNumP].chname,s1);
					// Determine if setting (s3) is string or numeric type data.
					if(isalpha(s3[0])) {
						strcpy(cdTableP[chNumP].strval,s3);
						cdTableP[chNumP].datatype = 1;
						// printf("%s %s ********************\n",cdTableP[chNumP].chname,cdTableP[chNumP].strval);
					} else {
						cdTableP[chNumP].chval = atof(s3);
						cdTableP[chNumP].datatype = 0;
						// printf("%s %f\n",cdTableP[chNumP].chname,cdTableP[chNumP].chval);
					}
					// Count up all the alarm settings in file. These will be set, but not part of monitoring.
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
					}
					chNumP ++;
				}
				
			}
			fclose(cdf);
			// Load values to EPICS records.
			lderror = newvalue(chNumP,cdTableP,command);
			break;
		case SDF_READ_ONLY:
		case SDF_LOAD_PARTIAL:
			printf("PARTIAL %s\n",sdfile);
			cdf = fopen(sdfile,"r");
			if(cdf == NULL) {
				sprintf(errMsg,"New SDF request ERROR: FILE %s DOES NOT EXIST\n",sdfile);
				logFileEntry(errMsg);
				lderror = 2;
				return(lderror);
			}
			localCtr = 0;
			chNumP = 0;
			alarmCnt = 0;
			if(!fmtInit) {
				fmNum = 0;
				logFileEntry("************* Software Restart ************");
			}
			strcpy(s4,"x");
			strncpy(ifo,pref,3);
			while(fgets(line,sizeof line,cdf) != NULL)
			{
				// Put dummy in s4 as this column may or may not exist.
				strcpy(s4,"x");
				sscanf(line,"%s%s%s%s",s1,s2,s3,s4);
				// If 1st three chars match IFO ie checking this this line is not BURT header or channel marked RO
				if(	strncmp(s1,ifo,3) == 0 && 
					// Don't allow load of SWSTAT or SWMASK, which are set by this program.
					strstr(s1,"_SWMASK") == NULL &&
					strstr(s1,"_SDF_NAME") == NULL &&
					strstr(s1,"_SWREQ") == NULL)
				{
					// Clear out the local tabel channel name string.
					bzero(cdTableP[chNumP].chname,strlen(cdTableP[chNumP].chname));
					// Load channel name into local table.
					strcpy(cdTableP[chNumP].chname,s1);
					// Determine if setting (s3) is string or numeric type data.
					if(isalpha(s3[0])) {
						strcpy(cdTableP[chNumP].strval,s3);
						cdTableP[chNumP].datatype = 1;
						// printf("%s %s ********************\n",cdTable[chNum].chname,cdTable[chNum].strval);
					} else {
						cdTableP[chNumP].chval = atof(s3);
						cdTableP[chNumP].datatype = 0;
						// printf("%s %f\n",cdTable[chNum].chname,cdTable[chNum].chval);
					}
					// Check if s4 (monitor or not) is set (0/1). If doesn/'t exist in file, set to zero in local table.
					if(isdigit(s4[0])) {
						// printf("%s %s %s %s\n",s1,s2,s3,s4);
						cdTableP[chNumP].mask = atoi(s4);
						if((cdTableP[chNumP].mask < 0) || (cdTableP[chNumP].mask > 1))
							cdTableP[chNumP].mask = 0;
					} 
					// else {
						// printf("%s %s %s \n",s1,s2,s3);
					//	cdTableP[chNumP].mask = -1;
					// }
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
					} else {
					// Add settings to local table.
					for(ii=0;ii<chNum;ii++)
					{
						if(strcmp(cdTable[ii].chname,cdTableP[chNumP].chname) == 0)
						{
							// printf("NEW channel compare %s\n",cdTable[chNumP].chname);
							fmatch = 1;
							if(cdTableP[chNumP].datatype == 1)
							{
								strcpy(cdTable[ii].strval,cdTableP[chNumP].strval);
							} else {
								cdTable[ii].chval = cdTableP[chNumP].chval;
							}
							if(cdTableP[chNumP].mask != -1)
								cdTable[ii].mask = cdTableP[chNumP].mask;
							cdTable[ii].initialized = 1;
						}
					}
					if(!fmatch) printf("NEW channel not found %s\n",cdTableP[chNumP].chname);
					}
					if(!fmtInit) {
						// Following is optional:
						// Uses the SWREQ AND SWMASK records of filter modules to decode which switch settings are incorrect.
						// This presently assumes that filter module SW1S will appear before SW2S in the burt files.
						if((strstr(s1,"_SW1S") != NULL) && (strstr(s1,"_SW1S.") == NULL))
						{
							bzero(filterTable[fmNum].fname,strlen(filterTable[fmNum].fname));
							strncpy(filterTable[fmNum].fname,s1,(strlen(s1)-4));
							tmpreq = (int)atof(s3);
						}
						if((strstr(s1,"_SW2S") != NULL) && (strstr(s1,"_SW2S.") == NULL))
						{
							int treq;
							treq = (int) atof(s3);
							tmpreq = tmpreq + (treq << 16);
							filterTable[fmNum].swreq = filtCtrlBitConvert(tmpreq);
							// printf("%s 0x%x %f %f\n",fTable[fmNum].fname,fTable[fmNum].swreq,tmpreq,atof(s3));
							tmpreq = 0;
							fmNum ++;
						}
					}
					localCtr ++;
					chNumP ++;
				}
			}
			fclose(cdf);
			lderror = newvalue(chNumP,cdTableP,command);
			if(!fmtInit) flderror = newfilterstats(fmNum,command);
			fmtInit = 1;
			break;
		case SDF_RESET:
			lderror = newvalue(chNum,cdTable,command);
		default:
			break;
	}
/*
	NOTE: This stub remains for EPICS channel locking using ASG if desired at some point.
	while((c = fscanf(cdf,"%s%s",cdTable[chNum].chname,s1) != EOF))
	{
		// Convert setting ascii to float
		cdTable[chNum].chval = atof(s1);
		// By default, set the data type in cdTable to floating point.
		cdTable[chNum].datatype = 0;
		// if(!strcmp(s2,"L")) cdTable[chNum].chlock = 1;
		// else cdTable[chNum].chlock = 0;
		// printf("Channel %s = %f\n",cdTable[chNum].chname, cdTable[chNum].chval);
		// Determine if chan value is really a string and not a float.
		if((cdTable[chNum].chval == 0) && (s1[0] != '0')) {
			// printf("Channel %s is not a number %s\n",cdTable[chNum].chname, s1);
			// Set data type to string
			cdTable[chNum].datatype = 1;
			// Copy string to cdTable string variable.
			strcpy(cdTable[chNum].strval,s1);
		}
		chNum ++;
	}
*/


	// Calc time to load settings and make log entry
	clock_gettime(CLOCK_REALTIME,&t);
	totaltime = t.tv_nsec - starttime;
	if(totaltime < 0) totaltime += 1000000000;
	if(command == SDF_LOAD_PARTIAL) {
		sprintf(errMsg,"New SDF request (w/table update): %s\nFile = %s\nTotal Chans = %d with load time = %d usec\n",timestring,sdfile,chNumP,(totaltime/1000));
	} else if(command == SDF_LOAD_DB_ONLY){
		sprintf(errMsg,"New SDF request (No table update): %s\nFile = %s\nTotal Chans = %d with load time = %d usec\n",timestring,sdfile,chNum,(totaltime/1000));
	}
	logFileEntry(errMsg);
	status = dbNameToAddr(reloadtimechannel,&paddr);
	status = dbPutField(&paddr,DBR_STRING,timestring,1);
	printf("Number of FM = %d\n",fmNum);
	return(lderror);
}

/// Routine used to extract all settings channels from EPICS database to create local settings table on startup.
void dbDumpRecords(DBBASE *pdbbase)
{
    DBENTRY  *pdbentry;
    long  status;
    char mytype[4][64];
    int ii;

    // By convention, the RCG produces ai and bi records for settings.
    sprintf(mytype[0],"%s","ai");
    sprintf(mytype[1],"%s","bi");
    sprintf(mytype[2],"%s","stringin");
    pdbentry = dbAllocEntry(pdbbase);

    chNum = 0;
    for(ii=0;ii<2;ii++) {
    status = dbFindRecordType(pdbentry,mytype[ii]);

    // status = dbFirstRecordType(pdbentry);
    if(status) {printf("No record descriptions\n");return;}
    while(!status) {
        printf("record type: %s",dbGetRecordTypeName(pdbentry));
        status = dbFirstRecord(pdbentry);
        if (status) printf("  No Records\n"); 
	int cnt = 0;
	// if((dbGetRecordTypeName(pdbentry),"ai") == 0){
        while (!status) {
	    cnt++;
            if (dbIsAlias(pdbentry)) {
                printf("\n  Alias:%s\n",dbGetRecordName(pdbentry));
            } else {
                // printf("\n  Record:%s\n",dbGetRecordName(pdbentry));
		sprintf(cdTable[chNum].chname,"%s",dbGetRecordName(pdbentry));
		// cdTable[chNum].chname = dbGetRecordName(pdbentry);
		if(ii == 0) {
			cdTable[chNum].datatype = 0;
			cdTable[chNum].chval = 0.0;
		} else {
			cdTable[chNum].datatype = 1;
			sprintf(cdTable[chNum].strval,"");
		}
		cdTable[chNum].mask = 0;
		cdTable[chNum].initialized = 0;
                // status = dbFirstField(pdbentry,TRUE);
                    // if(status) printf("    No Fields\n");
                // while(!status) {
                    // printf("    %s: %s",dbGetFieldName(pdbentry), dbGetString(pdbentry));
                    // status=dbNextField(pdbentry,TRUE);
                // }
            }
	    
	    cdTable[chNum].mask = 1;
	    chNum ++;
            status = dbNextRecord(pdbentry);
        }
	printf("  %d Records\n", cnt);
       //  status = dbNextRecordType(pdbentry);
    }
}
    // }
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

    if(argc>=2) {
        iocsh(argv[1]);
	// printf("Executing post script commands\n");
	dbDumpRecords(*iocshPpdbbase);
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
sleep(2);
	// Create BURT/SDF EPICS channel names
	char reloadChan[256]; sprintf(reloadChan, "%s_%s", pref, "SDF_RELOAD");		// Request to load new BURT
	char reloadStat[256]; sprintf(reloadStat, "%s_%s", pref, "SDF_RELOAD_STATUS");	// Status of last reload
	char sdfFileName[256]; sprintf(sdfFileName, "%s_%s", pref, "SDF_NAME");		// Name of file to load next request
	char loadedFile[256]; sprintf(loadedFile, "%s_%s", pref, "SDF_LOADED");		// Name of file presently loaded
	char edbloadedFile[256]; sprintf(edbloadedFile, "%s_%s", pref, "SDF_LOADED_EDB");	// Name of file presently loaded
	char speStat[256]; sprintf(speStat, "%s_%s", pref, "SDF_SP_ERR_CNT");		// Setpoint error counter
	char spaStat[256]; sprintf(spaStat, "%s_%s", pref, "SDF_ALARM_COUNT");		// Setpoint error counter
	char fcc[256]; sprintf(fcc, "%s_%s", pref, "SDF_FULL_CNT");		// Channels in Full BURT file
	char fsc[256]; sprintf(fsc, "%s_%s", pref, "SDF_FILE_SET_CNT");		// Channels in Partial BURT file
	char mcc[256]; sprintf(mcc, "%s_%s", pref, "SDF_MON_CNT");			// Channels in Partial BURT file
	char tsrname[256]; sprintf(tsrname, "%s_%s", pref, "SDF_SORT");			// SDF Table sorting request
	char cnfname[256]; sprintf(cnfname, "%s_%s", pref, "SDF_DROP_CNT");		// Number of channels not found.
	char cniname[256]; sprintf(cniname, "%s_%s", pref, "SDF_UNSET_CNT");		// Number of channels not initialized.
	char stename[256]; sprintf(stename, "%s_%s", pref, "SDF_TABLE_ENTRIES");	// Number of entries in an SDF reporting table.
	char monflagname[256]; sprintf(monflagname, "%s_%s", pref, "SDF_MON_ALL");	// Request to monitor all channels, not just those marked.
	char savecmdname[256]; sprintf(savecmdname, "%s_%s", pref, "SDF_SAVE_CMD");	// SDF Save command.
	char saveasname[256]; sprintf(saveasname, "%s_%s", pref, "SDF_SAVE_AS_NAME");	// SDF Save as file name.
	char savetypename[256]; sprintf(savetypename, "%s_%s", pref, "SDF_SAVE_TYPE");	// SDF Save file type.
	char saveoptsname[256]; sprintf(saveoptsname, "%s_%s", pref, "SDF_SAVE_OPTS");	// SDF Save file options.
	char savefilename[256]; sprintf(savefilename, "%s_%s", pref, "SDF_SAVE_FILE");	// SDF Name of last file saved.
	char savetimename[256]; sprintf(savetimename, "%s_%s", pref, "SDF_SAVE_TIME");	// SDF Time of last file save.
	char moddaqfilemsg[256]; sprintf(moddaqfilemsg, "%s_%s", pref, "MSGDAQ");	// SDF Time of last file save.
	char modcoefffilemsg[256]; sprintf(modcoefffilemsg, "%s_%s", pref, "MSG");	// SDF Time of last file save.
	// printf("SDF FILE EPICS = %s\n",sdfFileName);
	sprintf(timechannel,"%s_%s", pref, "TIME_STRING");
	// printf("timechannel = %s\n",timechannel);
	sprintf(reloadtimechannel,"%s_%s", pref, "SDF_RELOAD_TIME");			// Time of last BURT reload

	// Set request to load safe.snap on startup
	// printf("reload = %s\n",reloadChan);
	status = dbNameToAddr(reloadChan,&reload_addr);
	status = dbPutField(&reload_addr,DBR_LONG,&sdfReq,1);

	// Initialize BURT file to be loaded next request = BURT_safe.snap
	// printf("burt1 = %s\n",sdfFileName);
	status = dbNameToAddr(sdfFileName,&sdfname_addr);
	status = dbPutField(&sdfname_addr,DBR_STRING,sdf,1);

	status = dbNameToAddr(moddaqfilemsg,&daqmsgaddr);
	status = dbNameToAddr(modcoefffilemsg,&coeffmsgaddr);
	status = dbNameToAddr(fcc,&fulldbcntaddr);
	status = dbNameToAddr(fsc,&filesetcntaddr);
	status = dbNameToAddr(mcc,&monchancntaddr);
	status = dbNameToAddr(tsrname,&tablesortreqaddr);
	status = dbNameToAddr(cnfname,&chnotfoundaddr);
	status = dbNameToAddr(cniname,&chnotinitaddr);
	status = dbNameToAddr(stename,&sorttableentriesaddr);
	status = dbNameToAddr(reloadtimechannel,&reloadtimeaddr);
	// Zero out the MONITOR ALL request
	status = dbNameToAddr(monflagname,&monflagaddr);
	status = dbPutField(&monflagaddr,DBR_LONG,&rdstatus,1);
 	// Zero out the save cmd
	status = dbNameToAddr(savecmdname,&savecmdaddr);
	status = dbPutField(&savecmdaddr,DBR_LONG,&rdstatus,1);
	// Clear out the save as file name request
	status = dbNameToAddr(saveasname,&saveasaddr);
	status = dbPutField(&saveasaddr,DBR_STRING,"default",1);
	status = dbNameToAddr(savetypename,&savetypeaddr);
	status = dbNameToAddr(saveoptsname,&saveoptsaddr);
	status = dbNameToAddr(savefilename,&savefileaddr);
	status = dbPutField(&savefileaddr,DBR_STRING,"",1);
	status = dbNameToAddr(savetimename,&savetimeaddr);
	status = dbPutField(&savetimeaddr,DBR_STRING,"",1);

	status = dbNameToAddr(reloadStat,&reloadstat_addr);
	status = dbPutField(&reloadstat_addr,DBR_LONG,&rdstatus,1);

	status = dbNameToAddr(loadedFile,&loadedfile_addr);
	status = dbNameToAddr(edbloadedFile,&edbloadedaddr);

	// printf("sp channel = %s\n",speStat);
	status = dbNameToAddr(speStat,&sperroraddr);
	status = dbPutField(&sperroraddr,DBR_LONG,&sperror,1);
	// printf("sp channel = %s is loaded\n",speStat);
	status = dbNameToAddr(spaStat,&alrmchcountaddr);
	status = dbPutField(&alrmchcountaddr,DBR_LONG,&alarmCnt,1);

	sleep(2);
	daqFileCrc = checkFileCrc(daqFile);
	coeffFileCrc = checkFileCrc(coeffFile);

	for(;;) {
		// Following line is from old alarm monitor version.
		usleep(100000);
		fivesectimer = (fivesectimer + 1) % 50;
		// Check for reload request
		status = dbGetField(&reload_addr,DBR_LONG,&request,&ropts,&nvals,NULL);
		// Get File Name
		status = dbNameToAddr(sdfFileName,&sdfname_addr);
		status = dbGetField(&sdfname_addr,DBR_STRING,sdf,&ropts,&nvals,NULL);
		sprintf(sdfile, "%s%s%s", sdfDir, sdf,".snap");
		// Check if file name != to one presently loaded
		if(strcmp(sdf,loadedSdf) != 0) burtstatus |= 1;
		else burtstatus &= ~(1);
		if(burtstatus & 1) status = dbPutField(&reloadtimeaddr,DBR_STRING,"New SDF File Pending",1);
	
		if(request != 0) {
			// Clear Request
			status = dbPutField(&reload_addr,DBR_LONG,&ropts,1);
			printf("New request is %d\n",request);
			switch (request){
				case SDF_LOAD_DB_ONLY:
					strcpy(loadedSdf,sdf); 
					status = dbPutField(&edbloadedaddr,DBR_STRING,loadedSdf,1);
					chNumP = 0;
					break;
				case SDF_RESET:
					break;
				case SDF_LOAD_PARTIAL:
					strcpy(loadedSdf,sdf); 
					status = dbPutField(&loadedfile_addr,DBR_STRING,loadedSdf,1);
					status = dbPutField(&edbloadedaddr,DBR_STRING,loadedSdf,1);
					chNumP = 0;
					// printf("NEW FULL SDF REQ = %s   \n%s   %s\n%s\nReqest = %d\n",sdfile,sdf,loadedSdf,request);
					break;
				case SDF_READ_ONLY:
					strcpy(loadedSdf,sdf); 
					status = dbPutField(&loadedfile_addr,DBR_STRING,loadedSdf,1);
					// printf("NEW READ SDF REQ = %s   \n%s   %s\n%s\nReqest = %d\n",sdfile,sdf,loadedSdf,request);
					break;
				default:
					// printf("ERROR: UNKNOWN REQUEST --  request is %d\n",request);
					break;
			}
			rdstatus = readConfig(pref,sdfile,request);
			// printf("Chan count = %d and alarmCnt = %d\n\n\n\n",chNumP,alarmCnt);
			if (rdstatus) burtstatus |= rdstatus;
			else burtstatus &= ~(6);
			status = dbPutField(&reloadstat_addr,DBR_LONG,&rdstatus,1);
			sprintf(sdffileloaded, "%s%s%s", sdfDir, loadedSdf,".snap");
			sdfFileCrc = checkFileCrc(sdffileloaded);
			setChans = chNumP - alarmCnt;
			status = dbPutField(&filesetcntaddr,DBR_LONG,&setChans,1);
			status = dbPutField(&fulldbcntaddr,DBR_LONG,&chNum,1);
			createSortTableEntries(chNum);
			noMon = chNum - chMon;
			status = dbPutField(&monchancntaddr,DBR_LONG,&noMon,1);
			status = dbPutField(&alrmchcountaddr,DBR_LONG,&alarmCnt,1);
			status = dbPutField(&chnotfoundaddr,DBR_LONG,&chNotFound,1);
			status = dbPutField(&chnotinitaddr,DBR_LONG,&chNotInit,1);
			// Write out local monitoring table as snap file.
			status = writeTable2File(bufile,SDF_WITH_INIT_FLAG,cdTable);
		}
		status = dbPutField(&reloadstat_addr,DBR_LONG,&burtstatus,1);
		// sleep(1);
		// Check for SAVE requests
		status = dbGetField(&savecmdaddr,DBR_LONG,&sdfSaveReq,&ropts,&nvals,NULL);
		if(sdfSaveReq)
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
			// Get saveas filename
			status = dbGetField(&saveasaddr,DBR_STRING,saveasfilename,&ropts,&nvals,NULL);
			// Save the file
			printf("savefile cmd: type = %d  opts = %d\n",saveType,saveOpts);
			savesdffile(saveType,saveOpts,sdfDir,modelname,sdfile,saveasfilename,loadedSdf,savefileaddr,savetimeaddr);
		}
		// Check present settings vs BURT settings and report diffs.
		// Check if MON ALL CHANNELS is set
		status = dbGetField(&monflagaddr,DBR_LONG,&monFlag,&ropts,&nvals,NULL);
		sperror = spChecker(monFlag);
		status = dbPutField(&sperroraddr,DBR_LONG,&sperror,1);
		// Table sorting and presentation
		status = dbGetField(&tablesortreqaddr,DBR_STRING,tsrString,&ropts,&nvals,NULL);
		if(strcmp(tsrString,"CHANS NOT FOUND") == 0) tsrVal = 1;
		else if(strcmp(tsrString,"CHANS NOT INIT") == 0) tsrVal = 2;
		else if(strcmp(tsrString,"CHANS NOT MON") == 0) tsrVal = 3;
		else tsrVal = 0;
		switch(tsrVal) { 
			case 0:
				reportSetErrors(pref, sperror,setErrTable);
				status = dbPutField(&sorttableentriesaddr,DBR_LONG,&sperror,1);
				break;
			case 1:
				reportSetErrors(pref, chNotFound,unknownChans);
				status = dbPutField(&sorttableentriesaddr,DBR_LONG,&chNotFound,1);
				break;
			case 2:
				reportSetErrors(pref, chNotInit, uninitChans);
				status = dbPutField(&sorttableentriesaddr,DBR_LONG,&chNotInit,1);
				break;
			case 3:
				reportSetErrors(pref, noMon, unMonChans);
				status = dbPutField(&sorttableentriesaddr,DBR_LONG,&noMon,1);
				break;
			default:
				reportSetErrors(pref, sperror,setErrTable);
				status = dbPutField(&sorttableentriesaddr,DBR_LONG,&sperror,1);
				break;
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
