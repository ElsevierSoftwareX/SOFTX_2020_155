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
// Still need to add save/save as features
// Fix display:
//	- Based on new python script 
//	- channel counts and file names: baseline + changes?
//	- Get rid of 2nd LOAD button.
// Check top with full model loads for memory and cpu usage.
// Add command error checking, command out of range, etc.
// Verify changing of alarm settings will trigger errors
// Add more filter modules to tim16 to boost number of settings for test.
// Check autoBurt, particularly for SDF stuff
// ADD ability to add channels from larger files.

/*
 * Main program for demo sequencer
 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

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

#define BURT_LOAD_FULL		5
#define BURT_READ_ONLY		2
#define BURT_RESET		3
#define BURT_LOAD_PARTIAL	1

#define MAX_BURT_CHANS	300000

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

/// Quick look up table for filter module switch decoding
const unsigned int fltrConst[15] = {16, 64, 256, 1024, 4096, 16384,
			     65536, 262144, 1048576, 4194304,
			     0x4, 0x8, 0x4000000,0x1000000,0x1 /* in sw, off sw, out sw , limit sw*/
			     };

typedef struct CDS_CD_TABLE {
	char chname[128];
	int datatype;
	double chval; 
	char strval[64];
	int mask;
} CDS_CD_TABLE;

typedef struct FILTER_TABLE {
	char fname[128];
	int swreq;
	int swmask;
} FILTER_TABLE;

int chNum = 0;
int chMon = 0;
int chNumP = 0;
int fmNum = 0;
char timechannel[256];
char reloadtimechannel[256];
char fullchancnt[256];
char subchancnt[256];

CDS_CD_TABLE cdTable[MAX_BURT_CHANS];
CDS_CD_TABLE cdTableP[MAX_BURT_CHANS];
FILTER_TABLE filterTable[1000];


struct timespec t;

unsigned int filtCtrlBitConvert(unsigned int v) {
        unsigned int val = 0;
        int i;
        for (i = 0; i < 15; i++) {
                if (v & fltrConst[i]) val |= 1<<i;
        }
        return val;
}


/// Initialize alarm variables.
void init_vars() {
	int i, j;
	for (i = 0; i < 2; i++)
		for (j = 0; j < 10; j++) {
			pdbentry_alarm[i][j] = 0;
			pdbentry_status[i][j] = 0;
		}
}

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

// This function checks that present setpoints match those set by BURT if the channel is marked by a mask
// setting in the BURT file indicating that this channel should be monitored. .
// If settings don't match, then this is reported to the Guardian records in the form:
//	- Signal Name
//	- BURT Setting
//	- Present Value
//	- Time the present setting was applied.
int spChecker(char *pref)
{
   	int errCntr = 0;
	dbAddr paddr;
	dbAddr saddr;
	dbAddr baddr;
	dbAddr maddr;
	dbAddr taddr;
	dbAddr tscaddr;
	long status;
	int ropts = 0;
	double clearentry = 0.0;
	int nvals = 1;
	int ii;
	char s[256];
	char s1[256];
	char s2[256];
	char s3[256];
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
	static int lastcount = 0;
	int localErr = 0;
	char liveset[64];
	char burtset[64];

	     if(chNum) {
		for(ii=0;ii<chNum;ii++) {
			if((errCntr < 40) && (cdTable[ii].mask != 0))
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
						sprintf(burtset,"%.3f",cdTable[ii].chval);
						sprintf(liveset,"%.3f",buffer.rval);
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
				// If a diff was found, then write info the guardian EPICS records.
				if(localErr)
				{
					sprintf(s, "%s_%s_STAT%d", pref,"GRD_SP", errCntr);
					status = dbNameToAddr(s,&saddr);
					status = dbPutField(&saddr,DBR_STRING,&cdTable[ii].chname,1);

					sprintf(s1, "%s_%s_STAT%d_BURT", pref,"GRD_SP", errCntr);
					status = dbNameToAddr(s1,&baddr);
					status = dbPutField(&baddr,DBR_STRING,burtset,1);

					sprintf(s2, "%s_%s_STAT%d_LIVE", pref,"GRD_SP", errCntr);
					status = dbNameToAddr(s2,&maddr);
					status = dbPutField(&maddr,DBR_STRING,liveset,1);

					sprintf(s3, "%s_%s_STAT%d_TIME", pref,"GRD_SP", errCntr);
					strcpy(localtimestring, ctime(&mtime));
					localtimestring[strlen(localtimestring) - 1] = 0;
					status = dbNameToAddr(s3,&taddr);
					status = dbPutField(&taddr,DBR_STRING,localtimestring,1);
					errCntr ++;
				}
			}
		}
		// Clear out error fields if present errors < previous errors
		if(lastcount > errCntr) {
		for(ii=errCntr;ii<lastcount;ii++)
		{
			sprintf(s, "%s_%s_STAT%d", pref,"GRD_SP", ii);
			status = dbNameToAddr(s,&saddr);
			status = dbPutField(&saddr,DBR_STRING," ",1);

			sprintf(s1, "%s_%s_STAT%d_BURT", pref,"GRD_SP", ii);
			status = dbNameToAddr(s1,&baddr);
			status = dbPutField(&baddr,DBR_STRING," ",1);

			sprintf(s2, "%s_%s_STAT%d_LIVE", pref,"GRD_SP", ii);
			status = dbNameToAddr(s2,&maddr);
			status = dbPutField(&maddr,DBR_STRING," ",1);


			sprintf(s3, "%s_%s_STAT%d_TIME", pref,"GRD_SP", ii);
			status = dbNameToAddr(s3,&taddr);
			status = dbPutField(&taddr,DBR_STRING," ",1);
		}
		}
		lastcount = errCntr;
	     }
	return(errCntr);
}

// This function sets filter module request fields to aid in decoding errant filter module switch settings.
int newfilterstats(int numchans,int command) {
	dbAddr paddr;
	long status;
	int myerror = 0;
	int ii;
	FILE *log;
	char chname[128];
	int mask = 0x3fff;

	if(command == BURT_RESET) return(0);
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

// This function writes BURT settings to EPICS records.
int newvalue(int numchans,CDS_CD_TABLE cdTable[],int command) {
	dbAddr paddr;
	long status;
	double newVal;
	int ii;
	FILE *log;
	int myerror = 0;

	switch (command)
	{
		case BURT_LOAD_FULL:
		case BURT_LOAD_PARTIAL:
			for(ii=0;ii<numchans;ii++) {
				// Find address of channel
				status = dbNameToAddr(cdTable[ii].chname,&paddr);
				if(!status)
				{
				   if(cdTable[ii].datatype == 0)	// Value if floating point number
				   {
					status = dbPutField(&paddr,DBR_DOUBLE,&cdTable[ii].chval,1);
				   } else {			// Value is a string type
					status = dbPutField(&paddr,DBR_STRING,&cdTable[ii].strval,1);
				   }
				}
				else {				// Write errors to log file.
					myerror = 4;
					log = fopen("./ioc.log","a");
					fprintf(log,"CDF Load Error - Channel Not Found: %s\n",cdTable[ii].chname);
					fclose(log);
				}
			}
			return(myerror);
			break;
		case BURT_READ_ONLY:
			// If request was only to re-read the BURT file, then don't want to apply new settings.
			// This is typically the case where only mask fields were changed in the BURT file to
			// apply setpoint monitoring of channels.
			return(0);
			break;
		case BURT_RESET:
			// Only want to set those channels marked by a mask back to their original BURT setting.
			for(ii=0;ii<numchans;ii++) {
			    if(cdTable[ii].mask) {
				// Find address of channel
				status = dbNameToAddr(cdTable[ii].chname,&paddr);
				if(!status)
				{
				   if(cdTable[ii].datatype == 0)	// Value if floating point number
				   {
					status = dbPutField(&paddr,DBR_DOUBLE,&cdTable[ii].chval,1);
				   } else {			// Value is a string type
					status = dbPutField(&paddr,DBR_STRING,&cdTable[ii].strval,1);
				   }
				}
				else {				// Write errors to log file.
					myerror = 4;
					log = fopen("./ioc.log","a");
					fprintf(log,"CDF Load Error - Channel Not Found: %s\n",cdTable[ii].chname);
					fclose(log);
				}
			    }
			}
			return(myerror);
			break;
	}
}

void logFileError(char *filename)
{
	FILE *cdf;
	FILE *log;
	char timestring[256];
	long status;
	dbAddr paddr;

	getSdfTime(timestring);
	printf("Error - no file found : %s \n",filename);
	log = fopen("./ioc.log","a");
	fprintf(log,"New SDF request at %s\n ERROR: FILE %s DOES NOT EXIST\n",timestring,filename);
	fprintf(log,"***************************************************\n");
	fclose(log);
	status = dbNameToAddr(reloadtimechannel,&paddr);
	status = dbPutField(&paddr,DBR_STRING,"ERR - NO FILE FOUND",1);

}

void getSdfTime(char *timestring)
{

	dbAddr paddr;
	int ropts = 0;
	int nvals = 1;
	long status;

	status = dbNameToAddr(timechannel,&paddr);
	status = dbGetField(&paddr,DBR_STRING,timestring,&ropts,&nvals,NULL);
}


// Function to read BURT files and load data into local tables.
int readConfig(char *pref,char *sdfile, char *ssdfile,int command)
{
	FILE *cdf;
	FILE *log;
	char c;
	int ii;
	int lock;
	char s1[128],s2[128],s3[128],s4[128];
	dbAddr paddr;
	long status;
	int lderror = 0;
	int flderror = 0;
	int request;
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
	// FILTER_TABLE fTable[1000];

	clock_gettime(CLOCK_REALTIME,&t);
	starttime = t.tv_nsec;

	getSdfTime(timestring);

	switch(command)
	{
		case BURT_LOAD_FULL:
			cdf = fopen(sdfile,"r");
			if(cdf == NULL) {
				logFileError(sdfile);
				lderror = 2;
				return(lderror);
			}
			chNum = 0;
			fmNum = 0;
			chMon = 0;
			localCtr = 0;
			strcpy(s4,"x");
			strncpy(ifo,pref,3);
			// Read the settings file
			while(fgets(line,sizeof line,cdf) != NULL)
			{
				// Put dummy in s4 as this column may or may not exist.
				strcpy(s4,"x");
				sscanf(line,"%s%s%s%s",s1,s2,s3,s4);
				// If 1st three chars match IFO ie checking this this line is not BURT header or channel marked RO
				if(strncmp(s1,ifo,3) == 0)
				{
					// Clear out the local tabel channel name string.
					bzero(cdTable[chNum].chname,strlen(cdTable[chNum].chname));
					// Load channel name into local table.
					strcpy(cdTable[chNum].chname,s1);
					// Determine if setting (s3) is string or numeric type data.
					if(isalpha(s3[0])) {
						strcpy(cdTable[chNum].strval,s3);
						cdTable[chNum].datatype = 1;
						// printf("%s %s ********************\n",cdTable[chNum].chname,cdTable[chNum].strval);
					} else {
						cdTable[chNum].chval = atof(s3);
						cdTable[chNum].datatype = 0;
						// printf("%s %f\n",cdTable[chNum].chname,cdTable[chNum].chval);
					}
					// Check if s4 (monitor or not) is set (0/1). If doesn/'t exist in file, set to zero in local table.
					if(isdigit(s4[0])) {
						// printf("%s %s %s %s\n",s1,s2,s3,s4);
						cdTable[chNum].mask = atoi(s4);
					} else {
						// printf("%s %s %s \n",s1,s2,s3);
						cdTable[chNum].mask = 0;
					}
					chNum ++;
					localCtr ++;
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
				
			}
			fclose(cdf);
			lderror = newvalue(chNum,cdTable,command);
			flderror = newfilterstats(fmNum,command);
			break;
		case BURT_READ_ONLY:
		case BURT_LOAD_PARTIAL:
			cdf = fopen(sdfile,"r");
			if(cdf == NULL) {
				logFileError(sdfile);
				lderror = 2;
				return(lderror);
			}
			localCtr = 0;
			chNumP = 0;
			strcpy(s4,"x");
			strncpy(ifo,pref,3);
			while(fgets(line,sizeof line,cdf) != NULL)
			{
				// Put dummy in s4 as this column may or may not exist.
				strcpy(s4,"x");
				sscanf(line,"%s%s%s%s",s1,s2,s3,s4);
				// If 1st three chars match IFO ie checking this this line is not BURT header or channel marked RO
				if(strncmp(s1,ifo,3) == 0)
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
							cdTableP[chNumP].mask = -1;
					} else {
						// printf("%s %s %s \n",s1,s2,s3);
						cdTableP[chNumP].mask = -1;
					}
					// Find channel in full list and replace setting info
					fmatch = 0;
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
						}
					}
					if(!fmatch) printf("NEW channel not found %s\n",cdTable[chNumP].chname);
					localCtr ++;
					chNumP ++;
				}
			}
			fclose(cdf);
			chMon = 0;
			for(ii=0;ii<chNum;ii++)
				if(cdTable[ii].mask) chMon ++;
			lderror = newvalue(chNumP,cdTableP,command);
			break;
		case BURT_RESET:
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
	log = fopen("./ioc.log","a");
	if(command == BURT_LOAD_PARTIAL) {
		fprintf(log,"New SDF request (SUBSET): %s\nFile = %s\nTotal Chans = %d with load time = %d usec\n",timestring,sdfile,chNumP,(totaltime/1000));
	} else {
		fprintf(log,"New SDF request (FULL): %s\nFile = %s\nTotal Chans = %d with load time = %d usec\n",timestring,sdfile,chNum,(totaltime/1000));
	}
	fprintf(log,"***************************************************\n");
	fclose(log);
	status = dbNameToAddr(reloadtimechannel,&paddr);
	status = dbPutField(&paddr,DBR_STRING,timestring,1);
	printf("Number of FM = %d\n",fmNum);
	return(lderror);
}

/// Calculate Alarm (state) setpoint CRC checksums
// Not presently used.
unsigned int
field_crc(DBENTRY *pdbentry, char *field, unsigned int crc, unsigned int *len_crc) {
	long status = dbFindField(pdbentry, field);
  	if (status) {
		printf("No field %s was found\n", field);
		exit(1);
      	}
 	char *s = dbGetString(pdbentry);
	int l = strlen(s);
	*len_crc += l;
	return crc_ptr(s, l, crc);
}

/// Main subroutine for monitoring alarms
// NOTE: This code is not presently called. It was used when alarm monitoring was intended
// 	 as the means to determine diffs between BURT and present settings.
void process_alarms(DBBASE *pdbbase, char *pref)
{
    DBENTRY  *pdbentry;
    DBENTRY  *pdbentry1;
    long  status;
    int i, j;
    unsigned int crc = 0;
    unsigned int len_crc = 0;

    pdbentry = dbAllocEntry(pdbbase);

    if (0 == pdbentry_status[0][0]) {
      status = dbFindRecordType(pdbentry, "stringout");
      if (status) { 
    	printf("No record type \"stringout\" was found\n");
	exit(1);
      }
      for (i = 0; i < 2; i++) {
        for (j = 0; j < 10; j++) {
	char s[256]; sprintf(s, "%s_%s_STAT%d", pref, i? "GRD_RB": "GRD_SP", j);
        status = dbFindRecord(pdbentry, s);
        if (status) { 
    	  printf("Could not find %s record\n", s);
	  exit(1);
        }
        status = dbFindField(pdbentry, "VAL");
        if (status) {
	  printf("No field \"VAL\" was found\n");
	  exit(1);
        }
        pdbentry_status[i][j] = dbCopyEntry(pdbentry);
      }
      }
      char s[256];
      sprintf(s, "%s_GRD_ALH_CRC", pref);
      status = dbFindRecord(pdbentry, s);
      if (status) { 
    	  printf("Could not find %s record\n", s);
	  exit(1);
      }
      status = dbFindField(pdbentry, "VAL");
      if (status) {
	  printf("No field \"VAL\" was found\n");
	  exit(1);
      }
      pdbentry_crc = dbCopyEntry(pdbentry);

      sprintf(s, "%s_GRD_SP_ERR_CNT", pref);
      status = dbFindRecord(pdbentry, s);
      if (status) { 
    	  printf("Could not find %s record\n", s);
	  exit(1);
      }
      status = dbFindField(pdbentry, "VAL");
      if (status) {
	  printf("No field \"VAL\" was found\n");
	  exit(1);
      }
      pdbentry_in_err_cnt = dbCopyEntry(pdbentry);

      sprintf(s, "%s_GRD_RB_ERR_CNT", pref);
      status = dbFindRecord(pdbentry, s);
      if (status) { 
    	  printf("Could not find %s record\n", s);
	  exit(1);
      }
      status = dbFindField(pdbentry, "VAL");
      if (status) {
	  printf("No field \"VAL\" was found\n");
	  exit(1);
      }
      pdbentry_out_err_cnt = dbCopyEntry(pdbentry);
    }

    for (i = 0; i < 2; i++) {
      int nalrm = 0;
      for (j = 0; j < 2; j++) {
	char rtype[16];

	sprintf(rtype, "%c%c", j? 'b': 'a', i? 'o': 'i');
   	status = dbFindRecordType(pdbentry, rtype);
    	if (status) { 
    		printf("No record type %s was found\n", rtype);
		exit(1);
    	}
    	status = dbFirstRecord(pdbentry);
    	while (!status) {
      		pdbentry1 = dbCopyEntry(pdbentry);
		if (j) { // binary
			crc = field_crc(pdbentry, "ZSV", crc, &len_crc);
			crc = field_crc(pdbentry, "OSV", crc, &len_crc);
			crc = field_crc(pdbentry, "COSV", crc, &len_crc);
		} else { // analog
			crc = field_crc(pdbentry, "HIGH", crc, &len_crc);
			crc = field_crc(pdbentry, "LOW", crc, &len_crc);
			crc = field_crc(pdbentry, "HSV", crc, &len_crc);
			crc = field_crc(pdbentry, "LSV", crc, &len_crc);
		}
      		status = dbFindField(pdbentry, "NAME");
      		if (status) {
			printf("No field \"NAME\" was found\n");
			exit(1);
      		}
      		status = dbFindField(pdbentry1, "STAT");
      		if (status) {
			printf("No field \"STAT\" was found\n");
			exit(1);
      		}
      		char *als = dbGetString(pdbentry1);
      		if (strcmp(als, "NO_ALARM") && strcmp(als, "UDF")) {
        		//printf("%s STAT=%s\n", dbGetString(pdbentry), als);
			if (nalrm < 10) {
	  			pdbentry_alarm[i][nalrm] = dbCopyEntry(pdbentry);
	  			nalrm++;
			}
      		}
      		dbFreeEntry(pdbentry1);
      		status = dbNextRecord(pdbentry);
    	}
      }
    }

    // Display alarmed record names in status records
    for (i = 0; i < 2; i++) {
     int nalrm = 0;
     for (j = 0; j < 10; j++) {
      char *alarm_rec_name = "CLR";
      if (pdbentry_alarm[i][j]) {
	alarm_rec_name = dbGetString(pdbentry_alarm[i][j]);
	nalrm++;
      }
      status = dbPutString(pdbentry_status[i][j], alarm_rec_name);
      if (status) {
         printf("Could not put field\n");
         exit(1);
      }
      if (pdbentry_alarm[i][j]) {
        dbFreeEntry(pdbentry_alarm[i][j]);
        pdbentry_alarm[i][j] = 0;
      }
     }
     char s[16]; sprintf(s, "%d", nalrm);
     status = dbPutString(i? pdbentry_out_err_cnt: pdbentry_in_err_cnt, s);
     if (status) {
         printf("Could not put field\n");
         exit(1);
     }
    }

    // Output the CRC
    crc = crc_len(len_crc, crc);
    char s[16]; sprintf(s, "%d", crc);
    status = dbPutString(pdbentry_crc, s);
    if (status) {
         printf("Could not put field\n");
         exit(1);
    }
     //status = dbProcess(pdbentry_crc);

    dbFreeEntry(pdbentry);
}

void dbDumpRecords(DBBASE *pdbbase)
{
    DBENTRY  *pdbentry;
    long  status;

    pdbentry = dbAllocEntry(pdbbase);
    status = dbFirstRecordType(pdbentry);
    if(status) {printf("No record descriptions\n");return;}
    while(!status) {
        printf("record type: %s",dbGetRecordTypeName(pdbentry));
        status = dbFirstRecord(pdbentry);
        //if (status) printf("  No Records\n"); 
	int cnt = 0;
        while (!status) {
	    cnt++;
            /*f (dbIsAlias(pdbentry)) {
                printf("\n  Alias:%s\n",dbGetRecordName(pdbentry));
            } else*/ {
                //printf("\n  Record:%s\n",dbGetRecordName(pdbentry));
                status = dbFirstField(pdbentry,TRUE);
                    if(status) printf("    No Fields\n");
                while(!status) {
                    //printf("    %s: %s",dbGetFieldName(pdbentry), dbGetString(pdbentry));
                    status=dbNextField(pdbentry,TRUE);
                }
            }
            status = dbNextRecord(pdbentry);
        }
	printf("  %d Records\n", cnt);
        status = dbNextRecordType(pdbentry);
    }
    printf("End of all Records\n");
    dbFreeEntry(pdbentry);
}

/// Called on EPICS startup; This is generic EPICS provided function, modified for LIGO use.
int main(int argc,char *argv[])
{
	dbAddr taddr;
	dbAddr saddr;
	dbAddr raddr;
	dbAddr naddr;
	dbAddr ssnaddr;
	dbAddr speaddr;
	// dbAddr ssaddr;
	dbAddr sccaddr;
	dbAddr fccaddr;
	dbAddr mccaddr;
	int cdfReq = BURT_LOAD_FULL;
	long status;
	int request;
	int ropts = 0;
	int nvals = 1;
	int rdstatus = 0;
	int burtstatus = 0;
	char loadedSdf[256];
	char loadedSsdf[256];
   	int sperror = 0;
	char *ssdf;
	int noMon;
	FILE *csFile;
	int ii;

    if(argc>=2) {
        iocsh(argv[1]);
	printf("Executing post script commands\n");
	dbDumpRecords(*iocshPpdbbase);
	init_vars();
	// Get environment variables from startup command to formulate EPICS record names.
	char *pref = getenv("PREFIX");
	char *sdfDir = getenv("SDF_DIR");
	char *sdf = getenv("SDF_FILE");
	char ssdf[256];;
	strcpy(ssdf,sdf);
	// strcat(sdf,"_safe");
	char sdfile[256];
	char ssdfile[256];
	char bufile[256];
	// sprintf(sdfile, "%s%s%s", sdfDir, sdf,".sdf");
	sprintf(sdfile, "%s%s%s", sdfDir, sdf,".snap");					// Initialize with BURT_safe.snap
	sprintf(bufile, "%s%s", sdfDir, "fec.snap");				// Initialize table dump file
	printf("SDF FILE = %s\n",sdfile);
	printf("CURRENt FILE = %s\n",bufile);
	// Create BURT/SDF EPICS channel names
	char reloadChan[256]; sprintf(reloadChan, "%s_%s", pref, "SDF_RELOAD");		// Request to load new BURT
	char reloadStat[256]; sprintf(reloadStat, "%s_%s", pref, "SDF_RELOAD_STATUS");	// Status of last reload
	char sdfFileName[256]; sprintf(sdfFileName, "%s_%s", pref, "SDF_NAME");		// Name of file to load next request
	// char ssdfFileName[256]; sprintf(ssdfFileName, "%s_%s", pref, "SDF_NAME_SUBSET");	// Name of file to load next subset request
	char loadedFile[256]; sprintf(loadedFile, "%s_%s", pref, "SDF_LOADED");		// Name of file presently loaded
	char ssloadedFile[256]; sprintf(ssloadedFile, "%s_%s", pref, "SDF_LOADED_INIT");	// Name of subset file presently loaded
	char speStat[256]; sprintf(speStat, "%s_%s", pref, "GRD_SP_ERR_CNT");		// Setpoint error counter
	char fcc[256]; sprintf(fcc, "%s_%s", pref, "SDF_FULL_CH_COUNT");		// Channels in Full BURT file
	char scc[256]; sprintf(scc, "%s_%s", pref, "SDF_SUBSET_CH_COUNT");		// Channels in Partial BURT file
	char mcc[256]; sprintf(mcc, "%s_%s", pref, "SDF_MON_COUNT");		// Channels in Partial BURT file
	printf("SDF FILE EPICS = %s\n",sdfFileName);
	sprintf(timechannel,"%s_%s", pref, "TIME_STRING");
	printf("timechannel = %s\n",timechannel);
	sprintf(reloadtimechannel,"%s_%s", pref, "SDF_RELOAD_TIME");			// Time of last BURT reload

	// Clear request to load new BURT file
	status = dbNameToAddr(reloadChan,&taddr);
	status = dbPutField(&taddr,DBR_LONG,&cdfReq,1);

	// Initialize BURT file to be loaded next request = BURT_safe.snap
	status = dbNameToAddr(sdfFileName,&saddr);
	status = dbPutField(&saddr,DBR_STRING,sdf,1);

	// status = dbNameToAddr(ssdfFileName,&ssaddr);
	status = dbNameToAddr(fcc,&fccaddr);
	status = dbNameToAddr(scc,&sccaddr);
	status = dbNameToAddr(mcc,&mccaddr);

	status = dbNameToAddr(reloadStat,&raddr);
	status = dbPutField(&raddr,DBR_LONG,&rdstatus,1);

	status = dbNameToAddr(loadedFile,&naddr);
	status = dbNameToAddr(ssloadedFile,&ssnaddr);

	printf("sp channel = %s\n",speStat);
	status = dbNameToAddr(speStat,&speaddr);
	status = dbPutField(&speaddr,DBR_LONG,&sperror,1);
	printf("sp channel = %s is loaded\n",speStat);
char fullchancnt[256];
char subchancnt[256];

	sleep(2);

	for(;;) {
		// Following line is from old alarm monitor version.
		// process_alarms(*iocshPpdbbase, pref);
		sleep(1);
		// Check for reload request
		status = dbGetField(&taddr,DBR_LONG,&request,&ropts,&nvals,NULL);
		// Get File Name
		status = dbNameToAddr(sdfFileName,&saddr);
		status = dbGetField(&saddr,DBR_STRING,sdf,&ropts,&nvals,NULL);
		// status = dbNameToAddr(ssdfFileName,&ssaddr);
		// status = dbGetField(&ssaddr,DBR_STRING,ssdf,&ropts,&nvals,NULL);
		// sprintf(sdfile, "%s%s%s", sdfDir, sdf,".sdf");
		sprintf(sdfile, "%s%s%s", sdfDir, sdf,".snap");
		sprintf(ssdfile, "%s%s%s", sdfDir, ssdf,".snap");
		// Check if file name != to one presently loaded
		if(strcmp(sdf,loadedSdf) != 0) burtstatus |= 1;
		else burtstatus &= ~(1);
		
		if(request) {
			// Clear Request
			status = dbPutField(&taddr,DBR_LONG,&ropts,1);
			switch (request){
				case BURT_LOAD_FULL:
					strcpy(loadedSdf,sdf); 
					status = dbPutField(&naddr,DBR_STRING,loadedSdf,1);
					status = dbPutField(&ssnaddr,DBR_STRING,loadedSdf,1);
					chNumP = 0;
				case BURT_LOAD_PARTIAL:
				case BURT_RESET:
					strcpy(loadedSdf,sdf); 
					status = dbPutField(&naddr,DBR_STRING,loadedSdf,1);
					chNumP = 0;
					printf("NEW FULL SDF REQ = %s   \n%s   %s\n%s\nReqest = %d\n",sdfile,sdf,loadedSdf,ssdfile,request);
					break;
				case BURT_READ_ONLY:
					strcpy(loadedSdf,sdf); 
					status = dbPutField(&naddr,DBR_STRING,loadedSdf,1);
					printf("NEW READ SDF REQ = %s   \n%s   %s\n%s\nReqest = %d\n",sdfile,sdf,loadedSdf,ssdfile,request);
					break;
				default:
					break;
			}
			rdstatus = readConfig(pref,sdfile,ssdfile,request);
			if (rdstatus) burtstatus |= rdstatus;
			else burtstatus &= ~(6);
			status = dbPutField(&raddr,DBR_LONG,&rdstatus,1);
			status = dbPutField(&sccaddr,DBR_LONG,&chNumP,1);
			if(request == BURT_LOAD_FULL)
				status = dbPutField(&sccaddr,DBR_LONG,&chNum,1);
				status = dbPutField(&fccaddr,DBR_LONG,&chNum,1);
			noMon = chNum - chMon;
			status = dbPutField(&mccaddr,DBR_LONG,&noMon,1);
			csFile = fopen(bufile,"w");
			for(ii=0;ii<chNum;ii++)
			{
				// printf("%s datatype is %d\n",cdTable[ii].chname,cdTable[ii].datatype);
				char tabs[8];
				if(strlen(cdTable[ii].chname) > 39) sprintf(tabs,"%s","\t");
				else sprintf(tabs,"%s","\t\t");

				if(cdTable[ii].datatype == 0)
					fprintf(csFile,"%s%s%d\t%e\t\%d\n",cdTable[ii].chname,tabs,1,cdTable[ii].chval,cdTable[ii].mask);
				else
					fprintf(csFile,"%s%s%d\t%s\t\%d\n",cdTable[ii].chname,tabs,1,cdTable[ii].strval,cdTable[ii].mask);
			}
			fclose(csFile);
		}
		status = dbPutField(&raddr,DBR_LONG,&burtstatus,1);
		sleep(1);
		// Check present settings vs BURT settings and report diffs.
		sperror = spChecker(pref);
		status = dbPutField(&speaddr,DBR_LONG,&sperror,1);
	}
	sleep(0xfffffff);
    } else
    	iocsh(NULL);
    return(0);
}
