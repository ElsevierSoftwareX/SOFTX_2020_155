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

#include "iocsh.h"
#include "dbStaticLib.h"
#include "crc.h"

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

/// Initialize alarm variables.
void init_vars() {
	int i, j;
	for (i = 0; i < 2; i++)
		for (j = 0; j < 10; j++) {
			pdbentry_alarm[i][j] = 0;
			pdbentry_status[i][j] = 0;
		}
}

/// Calculate Alarm (state) setpoint CRC checksums
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
    if(argc>=2) {
        iocsh(argv[1]);
	printf("Executing post script commands\n");
	dbDumpRecords(*iocshPpdbbase);
	init_vars();
	char *pref = getenv("PREFIX");
	for(;;) {
		process_alarms(*iocshPpdbbase, pref);
		sleep(1);
	}
	sleep(0xfffffff);
    } else
    	iocsh(NULL);
    return(0);
}
