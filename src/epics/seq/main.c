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

#define BURT_LOAD_SETTINGS	1
#define BURT_READ_ONLY		2
#define BURT_RESET		3

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
int fmNum = 0;
char timechannel[256];
char reloadtimechannel[256];

CDS_CD_TABLE cdTable[20000];
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
	long options = DBR_STATUS|DBR_TIME;
	time_t mtime;
	char localtimestring[256];
	static int lastcount = 0;

	     if(chNum) {
		for(ii=0;ii<chNum;ii++) {
			if((cdTable[ii].datatype == 0) && (errCntr < 40) && (cdTable[ii].mask != 0))
			{
				// Find address of channel
				status = dbNameToAddr(cdTable[ii].chname,&paddr);
				// status = dbGetField(&paddr,DBR_DOUBLE,&rval,&ropts,&nvals,NULL);
				status = dbGetField(&paddr,DBR_DOUBLE,&buffer,&options,&nvals,NULL);
				if(cdTable[ii].chval != buffer.rval)
				{
					sprintf(s, "%s_%s_STAT%d", pref,"GRD_SP", errCntr);
					status = dbNameToAddr(s,&saddr);
					status = dbPutField(&saddr,DBR_STRING,&cdTable[ii].chname,1);

					sprintf(s1, "%s_%s_STAT%d_BURT", pref,"GRD_SP", errCntr);
					status = dbNameToAddr(s1,&baddr);
					status = dbPutField(&baddr,DBR_DOUBLE,&cdTable[ii].chval,1);

					sprintf(s2, "%s_%s_STAT%d_LIVE", pref,"GRD_SP", errCntr);
					status = dbNameToAddr(s2,&maddr);
					status = dbPutField(&maddr,DBR_DOUBLE,&buffer.rval,1);

					sprintf(s3, "%s_%s_STAT%d_TIME", pref,"GRD_SP", errCntr);
					mtime = buffer.time.secPastEpoch + POSIX_TIME_AT_EPICS_EPOCH;
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
			status = dbPutField(&baddr,DBR_DOUBLE,&clearentry,1);

			sprintf(s2, "%s_%s_STAT%d_LIVE", pref,"GRD_SP", ii);
			status = dbNameToAddr(s2,&maddr);
			status = dbPutField(&maddr,DBR_DOUBLE,&clearentry,1);


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

	if(command == BURT_RESET) return(0);
	for(ii=0;ii<numchans;ii++) {
		// Find address of channel
		status = dbNameToAddr(filterTable[ii].fname,&paddr);
		if(!status)
		{
			status = dbPutField(&paddr,DBR_LONG,&filterTable[ii].swreq,1);
		} else {				// Write errors to log file.
			myerror = 4;
			log = fopen("./ioc.log","a");
			fprintf(log,"CDF Load Error - Channel Not Found: %s\n",filterTable[ii].fname);
			fclose(log);
		}
	}
	return(myerror);
}

// This function writes BURT settings to EPICS records.
int newvalue(int numchans,int command) {
	dbAddr paddr;
	long status;
	double newVal;
	int ii;
	FILE *log;
	int myerror = 0;

	switch (command)
	{
		case BURT_LOAD_SETTINGS:
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

// Function to read BURT files and load data into local tables.
int readConfig(char *pref,char *sdfile, int command)
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
	// FILTER_TABLE fTable[1000];

	clock_gettime(CLOCK_REALTIME,&t);
	starttime = t.tv_nsec;

	status = dbNameToAddr(timechannel,&paddr);
	status = dbGetField(&paddr,DBR_STRING,timestring,&ropts,&nvals,NULL);

	// If BURT_RESET command, do not need to read the file.
	if(command != BURT_RESET)
	{
		cdf = fopen(sdfile,"r");
		if(cdf == NULL) {
			printf("Error - no file found : %s \n",sdfile);
			log = fopen("./ioc.log","a");
			fprintf(log,"New SDF request at %s\n ERROR: FILE %s DOES NOT EXIST\n",timestring,sdfile);
			fprintf(log,"***************************************************\n");
			fclose(log);
			status = dbNameToAddr(reloadtimechannel,&paddr);
			status = dbPutField(&paddr,DBR_STRING,"ERR - NO FILE FOUND",1);
			lderror = 2;
			return(lderror);
		}
		chNum = 0;
		fmNum = 0;
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
				// Following is optional:
				// Uses the SWREQ AND SWMASK records of filter modules to decode which switch settings are incorrect.
				// This presently assumes that filter module SW1S will appear before SW2S in the burt files.
				if((strstr(s1,"_SW1S") != NULL) && (strstr(s1,"_SW1S.") == NULL))
				{
					bzero(filterTable[fmNum].fname,strlen(filterTable[fmNum].fname));
					strncpy(filterTable[fmNum].fname,s1,(strlen(s1)-4));
					if(strstr(filterTable[fmNum].fname,"SWREQ") == NULL) 
					strcat(filterTable[fmNum].fname,"SWREQ");
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

	// Set New Values
	lderror = newvalue(chNum,command);
	flderror = newfilterstats(fmNum,command);

	// Calc time to load settings and make log entry
	clock_gettime(CLOCK_REALTIME,&t);
	totaltime = t.tv_nsec - starttime;
	log = fopen("./ioc.log","a");
	fprintf(log,"New SDF request: %s\nFile = %s\nTotal Chans = %d with load time = %d usec\n",timestring,sdfile,chNum,(totaltime/1000));
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
	dbAddr speaddr;
	int cdfReq = 1;
	long status;
	int request;
	int ropts = 0;
	int nvals = 1;
	int rdstatus = 0;
	int burtstatus = 0;
	char loadedSdf[256];
   	int sperror = 0;

    if(argc>=2) {
        iocsh(argv[1]);
	printf("Executing post script commands\n");
	dbDumpRecords(*iocshPpdbbase);
	init_vars();
	// Get environment variables from startup command to formulate EPICS record names.
	char *pref = getenv("PREFIX");
	char *sdfDir = getenv("SDF_DIR");
	char *sdf = getenv("SDF_FILE");
	strcat(sdf,"_safe");
	char sdfile[256];
	// sprintf(sdfile, "%s%s%s", sdfDir, sdf,".sdf");
	sprintf(sdfile, "%s%s%s", sdfDir, sdf,".snap");					// Initialize with BURT_safe.snap
	printf("SDF FILE = %s\n",sdfile);
	// Create BURT/SDF EPICS channel names
	char reloadChan[256]; sprintf(reloadChan, "%s_%s", pref, "SDF_RELOAD");		// Request to load new BURT
	char reloadStat[256]; sprintf(reloadStat, "%s_%s", pref, "SDF_RELOAD_STATUS");	// Status of last reload
	char sdfFileName[256]; sprintf(sdfFileName, "%s_%s", pref, "SDF_NAME");		// Name of file to load next request
	char loadedFile[256]; sprintf(loadedFile, "%s_%s", pref, "SDF_LOADED");		// Name of file presently loaded
	char speStat[256]; sprintf(speStat, "%s_%s", pref, "GRD_SP_ERR_CNT");		// Setpoint error counter
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

	status = dbNameToAddr(reloadStat,&raddr);
	status = dbPutField(&raddr,DBR_LONG,&rdstatus,1);

	status = dbNameToAddr(loadedFile,&naddr);

	printf("sp channel = %s\n",speStat);
	status = dbNameToAddr(speStat,&speaddr);
	status = dbPutField(&speaddr,DBR_LONG,&sperror,1);
	printf("sp channel = %s is loaded\n",speStat);

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
		// sprintf(sdfile, "%s%s%s", sdfDir, sdf,".sdf");
		sprintf(sdfile, "%s%s%s", sdfDir, sdf,".snap");
		// Check if file name != to one presently loaded
		if(strcmp(sdf,loadedSdf) != 0) burtstatus |= 1;
		else burtstatus &= ~(1);
		
		if(request) {
			// Clear Request
			status = dbPutField(&taddr,DBR_LONG,&ropts,1);
			strcpy(loadedSdf,sdf); 
			status = dbPutField(&naddr,DBR_STRING,loadedSdf,1);
			printf("NEW SDF REQ = %s   \n%s   \n%s\nReqest = %d\n",sdfile,sdf,loadedSdf,request);
			rdstatus = readConfig(pref,sdfile,request);
			if (rdstatus) burtstatus |= rdstatus;
			else burtstatus &= ~(6);
			status = dbPutField(&raddr,DBR_LONG,&rdstatus,1);
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
