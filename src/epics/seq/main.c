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
#include <ctype.h>

#ifdef USE_SYSTEM_TIME
#include <time.h>
#endif

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

#ifdef CA_SDF
#include <pthread.h>
#include "cadef.h"
#endif

///< SWSTAT has 32 bits but only 17 of them are used (what does bit 14 do?)
#define ALL_SWSTAT_BITS		(0x1bfff)

#define SDF_LOAD_DB_ONLY	4
#define SDF_READ_ONLY		2
#define SDF_RESET		3
#define SDF_LOAD_PARTIAL	1

#define SDF_MAX_CHANS		125000	///< Maximum number of settings, including alarm settings.
#define SDF_MAX_TSIZE		20000	///< Maximum number of EPICS settings records (No subfields).
#define SDF_ERR_DSIZE		40	///< Size of display reporting tables.
#define SDF_ERR_TSIZE		20000	///< Size of error reporting tables.
#define SDF_MAX_FMSIZE		1000	///< Maximum number of filter modules that can be monitored.

#define MAX_CHAN_LEN		60

#define SDF_TABLE_DIFFS			0
#define SDF_TABLE_NOT_FOUND		1
#define SDF_TABLE_NOT_INIT		2
#define SDF_TABLE_NOT_MONITORED		3
#define SDF_TABLE_FULL			4
#ifdef CA_SDF
#define SDF_TABLE_DISCONNECTED		5
#endif

#define CA_STATE_OFF -1
#define CA_STATE_IDLE 0
#define CA_STATE_PARSING 1
#define CA_STATE_RUNNING 2
#define CA_STATE_EXIT 3

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
#ifdef CA_SDF
	int connected;
#endif
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
	double liveval;
	int sigNum;
	int chFlag;
	int filtNum;
	unsigned int sw[2];
} SET_ERR_TABLE;

#ifdef CA_SDF
/// Structure for holding data from EPICS CA
typedef struct EPICS_CA_TABLE {
	int redirIndex;					// When a connection is in progress writes are redirected as the type may have changed
	int datatype;
	union Data data;
	int connected;
	time_t mod_time;
	chid chanid;
	int chanIndex;					// index of the channel in cdTable, caTable, ...
} EPICS_CA_TABLE;

/// Structure to hold start up information for the CA thread
typedef struct CA_STARTUP_INFO {
	char *fname;		// name of the request file to parse
	char *prefix;		// ifo name
} CA_STARTUP_INFO;

#endif

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
#ifndef USE_SYSTEM_TIME
char timechannel[256];		///< Name of the GPS time channel for timestamping.
#endif
char reloadtimechannel[256];	///< Name of EPICS channel which contains the BURT reload requests.
struct timespec t;
char logfilename[128];

CDS_CD_TABLE cdTable[SDF_MAX_TSIZE];		///< Table used to hold EPICS database info for monitoring settings.
CDS_CD_TABLE cdTableP[SDF_MAX_CHANS];		///< Temp table filled on BURT read and set to EPICS channels.
FILTER_TABLE filterTable[SDF_MAX_FMSIZE];			///< Table for holding filter module switch settings for comparisons.
SET_ERR_TABLE setErrTable[SDF_ERR_TSIZE];	///< Table used to report settings diffs.
SET_ERR_TABLE unknownChans[SDF_ERR_TSIZE];	///< Table used to report channels not found in local database.
SET_ERR_TABLE uninitChans[SDF_ERR_TSIZE];	///< Table used to report channels not initialized by BURT safe.snap.
SET_ERR_TABLE unMonChans[SDF_MAX_TSIZE];	///< Table used to report channels not being monitored.
SET_ERR_TABLE readErrTable[SDF_ERR_TSIZE];	///< Table used to report file read errors..
SET_ERR_TABLE cdTableList[SDF_MAX_TSIZE];	///< Table used to report file read errors..
time_t timeTable[SDF_MAX_CHANS];		///< Holds timestamps for cdTableP

int filterMasks[SDF_MAX_FMSIZE];		///< Filter monitor masks, as manipulated by the user

dbAddr fmMaskChan[SDF_MAX_FMSIZE];		///< We reflect the mask value into these channels
dbAddr fmMaskChanCtrl[SDF_MAX_FMSIZE];		///< We signal bit changes to the masks on these channels

#ifdef CA_SDF
SET_ERR_TABLE disconnectChans[SDF_MAX_TSIZE];

// caTable, caConnTAble, chConnNum, chEnumDetermined are protected by the caTableMutex
EPICS_CA_TABLE caTable[SDF_MAX_TSIZE];		    ///< Table used to hold the data returned by channel access
EPICS_CA_TABLE caConnTable[SDF_MAX_TSIZE]; ///< Table used to hold new connection data
EPICS_CA_TABLE caConnEnumTable[SDF_MAX_TSIZE];  ///< Table used to hold enums as their monitoring type (NUM/STR) is determined
												///< enums are treated as STR records only if they have non-null 0 and 1 strings

// protected by caEvidMutex
evid caEvid[SDF_MAX_TSIZE];			///< Table to hold the CA subscription id for the channels

int chConnNum = 0;					///< Total number of entries to be processed in caConnTable
int chEnumDetermined = 0;			///< Total number of enums whos types have been determined

long chDisconnected = 0;		///< Total number of channels that are disconnected.  This is used as the max index for disconnectedChans

pthread_t caThread;
pthread_mutex_t caStateMutex;
pthread_mutex_t caTableMutex;
pthread_mutex_t caEvidMutex;

int caThreadState;
long droppedPVCount;

#define ADDRESS int
#define SETUP setupCASDF();
#define CLEANUP cleanupCASDF();
#define GET_ADDRESS(NAME,ADDRP) getCAIndex((NAME),(ADDRP))
#define PUT_VALUE(ADDR,TYPE,PVAL) setCAValue((ADDR),(TYPE),(PVAL))
#define PUT_VALUE_INT(ADDR,PVAL) setCAValueLong((ADDR),(PVAL))
#define GET_VALUE_NUM(ADDR,DESTP,TIMEP,CONNP) syncEpicsDoubleValue((ADDR),(DESTP),(TIMEP),(CONNP))
#define GET_VALUE_INT(ADDR,DESTP,TIMEP,CONNP) syncEpicsIntValue((ADDR),(DESTP),(TIMEP),(CONNP))
#define GET_VALUE_STR(ADDR,DESTP,LEN,TIMEP,CONNP) syncEpicsStrValue((ADDR),(DESTP),(LEN),(TIMEP),(CONNP))
#else
#define ADDRESS dbAddr
#define SETUP
#define CLEANUP
#define GET_ADDRESS(NAME,ADDRP) dbNameToAddr((NAME),(ADDRP))
#define PUT_VALUE(ADDR,TYPE,PVAL) dbPutField(&(ADDR),((TYPE)==SDF_NUM ? DBR_DOUBLE : DBR_STRING),(PVAL),1)
#define PUT_VALUE_INT(ADDR,PVAL) dbPutField(&(ADDR),DBR_LONG,(PVAL),1);
#define GET_VALUE_NUM(ADDR,DESTP,TIMEP,CONNP) getDbValueDouble(&(ADDR),(double*)(DESTP),(TIMEP))
#define GET_VALUE_INT(ADDR,DESTP,TIMEP,CONNP) getDbValueLong(&(ADDR),(unsigned int*)(DESTP),(TIMEP))
#define GET_VALUE_STR(ADDR,DESTP,LEN,TIMEP,CONNP) getDbValueString(&(ADDR),(char*)(DESTP),(LEN),(TIMEP))
#endif

#define SDF_NUM		0
#define SDF_STR		1
#define SDF_UNKNOWN	-1


// Function prototypes		****************************************************************************************
int isAlarmChannel(char *);
int checkFileCrc(char *);
int checkFileMod( char* , time_t* , int );
unsigned int filtCtrlBitConvert(unsigned int);
void getSdfTime(char *, int size);
void logFileEntry(char *);
int getEpicsSettings(int, time_t *);
int writeTable2File(char *,char *,int,CDS_CD_TABLE *);
int savesdffile(int,int,char *,char *,char *,char *,char *,dbAddr,dbAddr,dbAddr); 
int createSortTableEntries(int,int,char *,int *,time_t*);
int reportSetErrors(char *,int,SET_ERR_TABLE *,int,int);
int spChecker(int,SET_ERR_TABLE *,int,char *,int,int *);
void newfilterstats(int);
int writeEpicsDb(int,CDS_CD_TABLE *,int);
int readConfig( char *,char *,int,char *);
int parseLine(char *, int,char *,char *,char *,char *,char *,char *);
int modifyTable(int,SET_ERR_TABLE *);
int resetSelectedValues(int,SET_ERR_TABLE *);
void clearTableSelections(int,SET_ERR_TABLE *, int *);
void setAllTableSelections(int,SET_ERR_TABLE *, int *,int);
void changeSelectCB_uninit(int, SET_ERR_TABLE *, int *);
void decodeChangeSelect(int, int, int, SET_ERR_TABLE *, int *, void (*)(int, SET_ERR_TABLE *, int *));
int appendAlarms2File(char *,char *,char *);
void registerFilters();
void setupFMArrays(char *,int*,dbAddr*,dbAddr*);
void resyncFMArrays(int *,dbAddr*);
void processFMChanCommands(int*,dbAddr*,dbAddr*,int*,int,SET_ERR_TABLE*);
#ifdef CA_SDF
void nullCACallback(struct event_handler_args);

int getCAIndex(char *, ADDRESS *);

int canFindCAChannel(char *entry);
int setCAValue(ADDRESS, int, void *);
int setCAValueLong(ADDRESS, unsigned long *);

int syncEpicsDoubleValue(ADDRESS, double *, time_t *, int *);
int syncEpicsIntValue(ADDRESS, unsigned int *, time_t *, int *);
int syncEpicsStrValue(ADDRESS, char *, int, time_t *, int *);

void subscriptionHandler(struct event_handler_args);
void connectCallback(struct connection_handler_args);
void registerPV(char *);
int daqToSDFDataType(int);
void parseChannelListReq(char *, char *);

int getCAThreadState();
void setCAThreadState(int state);

void copyConnectedCAEntry(EPICS_CA_TABLE *);
void syncCAConnections(long *);

void *caMainLoop(void *);


void initCAConnections(char *, char *);
void setupCASDF();
void cleanupCASDF();
#else
int getDbValueDouble(ADDRESS*,double *,time_t *);
int getDbValueString(ADDRESS*,char *, int, time_t *);
int getDbValueLong(ADDRESS *,unsigned int * ,time_t *);
void dbDumpRecords(DBBASE *,const char *);
#endif

#ifdef VERBOSE_DEBUG
#define D_NOW (__output_iter == 0)
#define D(...) {if (__output_iter == 0) { fprintf(stderr, __VA_ARGS__); } }

#define TEST_CHAN "X1:PSL-PMC_BOOST_CALI_SW1S"

int __output_iter = 0;
int __test_chan_idx = -1;
void iterate_output_counter()
{
	++__output_iter;
	if (__output_iter > 10) __output_iter = 0;
}
#else
#define D_NOW (false)
#define D(...) {}
void iterate_output_counter() {}
#endif


// Some helper macros
#define CHFLAG_CURRENT_MONITORED_BIT_VAL 1
#define CHFLAG_REVERT_BIT_VAL 2
#define CHFLAG_ACCEPT_BIT_VAL 4
#define CHFLAG_MONITOR_BIT_VAL 8

#define CHFLAG_CURRENT_MONITORED_BIT(x) ((x) & 1)
#define CHFLAG_REVERT_BIT(x) ((x) & 2)
#define CHFLAG_ACCEPT_BIT(x) ((x) & 4)
#define CHFLAG_MONITOR_BIT(x) ((x) & 8)

// End Header **********************************************************************************************************
//

/// Given a channel name return 1 if it is an alarm channel, else 1
int isAlarmChannel(char *chname) {
	if (!chname) return 0;
	return ((strstr(chname,".HIGH") != NULL) || 
		(strstr(chname,".HIHI") != NULL) || 
		(strstr(chname,".LOW") != NULL) || 
		(strstr(chname,".LOLO") != NULL) || 
		(strstr(chname,".HSV") != NULL) || 
		(strstr(chname,".OSV") != NULL) || 
		(strstr(chname,".ZSV") != NULL) || 
		(strstr(chname,".LSV") != NULL) );
}

/// Common routine to parse lines read from BURT/SDF files..
///	@param[in] *s	Pointer to line of chars to parse.
///     @param[in] bufSize Size in characters of each output buffer str1 thru str6
///	@param[out] str1 thru str6 	String pointers to return individual words from line.
///	@return wc	Number of words in the line.
int parseLine(char *s, int bufSize, char *str1, char *str2, char *str3, char *str4, char *str5,char *str6)
{
  int wc = 0;
  int ii = 0;
  int cursize = 0;
  int lastwasspace = 0;
  
  char *lastquote = 0;
  char *qch = 0;
  char *out[7];
  
  out[0]=str1;
  out[1]=str2;
  out[2]=str3;
  out[3]=str4;
  out[4]=str5;
  out[5]=str6;
  out[6]=0;
  for (ii = 0; out[ii]; ++ii) { *(out[ii]) = '\0'; }

  while (*s != 0 && *s != '\n') {
    if (*s == '\t') *s = ' ';
    if (*s == ' ') {
      // force the null termination of the output buffer
      out[wc][bufSize-1] = '\0';
      if (!lastwasspace) {
	++wc;
	// check for the end of the output buffers
	if (!out[wc]) break;
      }
      lastwasspace = 1;
      cursize = 0;
    } else if (*s != '"' || !lastwasspace) {
      // regular character
      if (cursize < bufSize) {
	strncat(out[wc], s, 1);
	cursize++;
      }
      lastwasspace = 0;
    } else {
      // quote
      // burt does not escape quotes, you have to look for the last quote and just take it.
      lastquote = 0;
      qch = s+1;
      
      while (*qch && *qch !='\n') {
	if (*qch == '"') lastquote = qch;
	++qch;
      }
      if (!lastquote) lastquote = qch;
      ++s;
      // copy from (s,qch) then set lastwasspace
      while (s < lastquote) {
	if (cursize < bufSize) {
	  strncat(out[wc], s, 1);
	  cursize++;
	}
	++s;
      }
      ++wc;
      lastwasspace = 1;
      if (!(*s) || *s == '\n' || !out[wc]) break;
    }
    ++s;
  }
  if (out[wc] && out[wc][0]) {
    out[wc][bufSize-1]='\0';
    ++wc;
  }
  for (ii = 0; out[ii]; ++ii) {
    if (strcmp(out[ii], "\\0") == 0) strcpy(out[ii], "");
  }
  return wc;
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
int filtNum = -1;
long status = 0;
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
			sc[2] = numEntries;
			for(ii=0;ii<numEntries;ii++) {
				dcsErrTable[ii].chFlag |= 8;
				
				filtNum = dcsErrTable[ii].filtNum;
				if (filtNum >= 0) {
					if (filterMasks[filtNum] == filterTable[filtNum].mask) {
						filterMasks[filtNum] = ((ALL_SWSTAT_BITS & filterMasks[filtNum]) > 0 ? 0 : ~0);
						status = dbPutField(&fmMaskChan[filtNum],DBR_LONG,&(filterMasks[filtNum]),1);
					}
				}
			}
			break;
		default:
			break;
	}
}

/// Callback function to ensure consistency of the select flags for entries in the uninit table
///	@param[in] index		Number providing the index into the dcsErrTable
///	@param[in] dcsErrTable		Pointer to table
///	@param[out] selectCounter	Pointer to change counters
void changeSelectCB_uninit(int index, SET_ERR_TABLE *dcsErrTable, int selectCounter[])
{
	int revertBit = dcsErrTable[index].chFlag & 2;
	int acceptBit = dcsErrTable[index].chFlag & 4;
	int monBit = dcsErrTable[index].chFlag & 8;
	int otherBits = 0; //dcsErrTable[index].chFlag | ~(2 | 4 | 8);

	// revert is not defined on this table
	if (revertBit) {
		revertBit = 0;
		selectCounter[0] --;
	}
	// if monitor is set, then accept must be set
	if (monBit && !acceptBit) {
		acceptBit = 4;
		selectCounter[1] ++;
	} 
	dcsErrTable[index].chFlag = revertBit | acceptBit | monBit | otherBits;
}

/// Common routine to set/unset individual entries for table changes..
///	@param[in] selNum		Number providing line and column of table entry to change.
///	@param[in] page			Number providing the page of table to change.
///	@param[in] totalItems		Number items selected for change.
///	@param[in] dcsErrtable 		Pointer to table
///	@param[out] selectCounter 	Pointer to change counters.
void decodeChangeSelect(int selNum, int page, int totalItems, SET_ERR_TABLE *dcsErrTable, int selectCounter[], void (*cb)(int, SET_ERR_TABLE*, int*))
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
				/* we only handle non-filter bank entries here */
				if (dcsErrTable[selectLine].filtNum < 0) {
					dcsErrTable[selectLine].chFlag ^= 8;
					if(dcsErrTable[selectLine].chFlag & 8) selectCounter[2] ++;
					else selectCounter[2] --;
				}
				break;
			default:
				break;
		}
		if (cb)
			cb(selectLine, dcsErrTable, selectCounter);
	}
}

// Check for file modification time changes
int
checkFileMod( char* fName, time_t* last_mod_time, int gettime )
{
    struct stat statBuf;
    if ( !stat( fName, &statBuf ) )
    {
        if ( gettime )
        {
            *last_mod_time = statBuf.st_mtime;
            return 2;
        }
        if ( statBuf.st_mtime > *last_mod_time )
        {
            *last_mod_time = statBuf.st_mtime;
            return 1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }
}

/// Common routine to check file CRC.
///	@param[in] *fName	Name of file to check.
///	@return File CRC or -1 if file not found.
int checkFileCrc(char *fName)
{
    char         cbuf[ 128 ];
    char*        cp;
    int          flen = 0;
    int          clen = 0;
    unsigned int crc = 0;
    FILE*        fp = fopen( fName, "r" );
    if ( fp == NULL )
    {
        return 0;
    }

    while ( ( cp = fgets( cbuf, 128, fp ) ) != NULL )
    {
        clen = strlen( cbuf );
        flen += clen;
        crc = crc_ptr( cbuf, clen, crc );
    }
    crc = crc_len( flen, crc );
    fclose( fp );
    return crc;
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

/// Given the a filter table entry print out the bits set in the mask.
/// @param[in] filter the filter table entry to extract the current value from
/// @param[out] dest buffer to put the results in. must be >= 64 bytes
void createSwitchText(FILTER_TABLE *filter, char *dest) {
#if 1
	int sw1s=0;
	int sw2s=0;

	dest[0] = 0;
	sw1s = (int)cdTableP[filter->sw[0]].data.chval;
	sw2s = (int)cdTableP[filter->sw[1]].data.chval;
#else
	ADDRESS paddr;
	ADDRESS paddr2;
	int sw1s=0;
	int sw2s=0;
	char name[256];
	char name2[256];
	
	if (snprintf(name, sizeof(name), "%s%s", filter->fname, "SW1S") > sizeof(name)) {	
		return;
	}
	if (snprintf(name2, sizeof(name2), "%s%s", filter->fname, "SW2S") > sizeof(name2)) {	
		return;
	}
	GET_ADDRESS(name, &paddr);
	GET_ADDRESS(name2, &paddr2);
	GET_VALUE_INT(paddr, &sw1s, NULL, NULL);
	GET_VALUE_INT(paddr2, &sw2s, NULL, NULL);
#endif
	filtStrBitConvert(1,filtCtrlBitConvert(sw1s + (sw2s << 16)), dest);
}

/// Routine to check filter switch settings and provide info on switches not set as requested.
/// @param[in] fcount	Number of filters in the control model.
/// @param[in] monitorAll	Global monitoring flag.
/// @return	Number of errant switch settings found.
int checkFilterSwitches(int fcount, SET_ERR_TABLE setErrTable[], int monitorAll, int displayall, int wcflag, char *wcstr, int *diffcntr)
{
unsigned int refVal=0;
unsigned int presentVal=0;
unsigned int x=0,y=0;
unsigned int mask = 0;
unsigned int maskedRefVal = 0;
unsigned int maskedSwReqVal = 0;
int ii=0,jj=0;
int errCnt = 0;
char swname[3][64];
char tmpname[64];
time_t mtime=0;
char localtimestring[256];
char swstrE[64];
char swstrB[64];
char swstrD[64];
struct buffer {
	time_t t;
	unsigned int rval;
	}buffer[2];
ADDRESS paddr;
ADDRESS paddr1;
ADDRESS paddr2;
long status=0;

    swstrE[0]='\0';
    swstrB[0]='\0';
    swstrD[0]='\0';
	for(ii=0;ii<fcount;ii++)
	{
		bzero(buffer, sizeof(struct buffer)*2);
		bzero(swname[0],sizeof(swname[0]));
		bzero(swname[1],sizeof(swname[1]));
		strcpy(swname[0],filterTable[ii].fname);
		strcat(swname[0],"SW1S");
		strcpy(swname[1],filterTable[ii].fname);
		strcat(swname[1],"SW2S");
		sprintf(swname[2],"%s",filterTable[ii].fname);
		strcat(swname[2],"SWSTR");
		status = GET_ADDRESS(swname[0],&paddr);
		status = GET_VALUE_INT(paddr,&(buffer[0].rval),&(buffer[0].t), 0);
		status = GET_ADDRESS(swname[1],&paddr1);
		status = GET_VALUE_INT(paddr1,&(buffer[1].rval),&(buffer[1].t), 0);
		for(jj=0;jj<2;jj++) {
			if(buffer[jj].rval > 0xffff || buffer[jj].rval < 0)	// Switch setting overrange
			{
				sprintf(setErrTable[errCnt].chname,"%s", swname[jj]);
				sprintf(setErrTable[errCnt].burtset, "%s", " ");
				sprintf(setErrTable[errCnt].liveset, "0x%x", buffer[jj].rval);
				sprintf(setErrTable[errCnt].diff, "%s", "OVERRANGE");
				setErrTable[errCnt].liveval = 0.0;
				setErrTable[errCnt].sigNum = filterTable[ii].sw[jj];
				// take the latest change time between the SW1S & SW2S channels
				mtime = (buffer[0].t >= buffer[1].t ? buffer[0].t : buffer[1].t);
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
		status = GET_ADDRESS(swname[2],&paddr2);
		status = PUT_VALUE(paddr2,SDF_STR,swstrE);
		setErrTable[errCnt].filtNum = -1;

		mask = (unsigned int)filterTable[ii].mask;
		maskedRefVal = refVal & mask;
		maskedSwReqVal = filterTable[ii].swreq & mask;
		if(( maskedRefVal != maskedSwReqVal && errCnt < SDF_ERR_TSIZE && (mask || monitorAll) && filterTable[ii].init) )
			*diffcntr += 1;
		if(( maskedRefVal != maskedSwReqVal && errCnt < SDF_ERR_TSIZE && (mask || monitorAll) && filterTable[ii].init) || displayall)
		{
			filtStrBitConvert(1,refVal,swstrE);
			filtStrBitConvert(1,filterTable[ii].swreq,swstrB);
			x = maskedRefVal ^ maskedSwReqVal;
			filtStrBitConvert(1,x,swstrD);
			bzero(tmpname,sizeof(tmpname));
			strncpy(tmpname,filterTable[ii].fname,(strlen(filterTable[ii].fname)-1));

			if(!wcflag || (wcflag && (strstr(tmpname,wcstr) != NULL))) {
			sprintf(setErrTable[errCnt].chname,"%s", tmpname);
			sprintf(setErrTable[errCnt].burtset, "%s", swstrB);
			sprintf(setErrTable[errCnt].liveset, "%s", swstrE);
			sprintf(setErrTable[errCnt].diff, "%s", swstrD);
			
			setErrTable[errCnt].liveval = 0.0;
			setErrTable[errCnt].sigNum = filterTable[ii].sw[0] + (filterTable[ii].sw[1] * SDF_MAX_TSIZE);
			setErrTable[errCnt].filtNum = ii;
			setErrTable[errCnt].sw[0] = buffer[0].rval;
			setErrTable[errCnt].sw[1] = buffer[1].rval;
			if(filterTable[ii].mask & ALL_SWSTAT_BITS) setErrTable[errCnt].chFlag |= 0x1;
			else setErrTable[errCnt].chFlag &= 0xe;

			// take the latest change time between the SW1S & SW2S channels
			mtime = (buffer[0].t >= buffer[1].t ? buffer[0].t : buffer[1].t);
			strcpy(localtimestring, ctime(&mtime));
			localtimestring[strlen(localtimestring) - 1] = 0;
			sprintf(setErrTable[errCnt].timeset, "%s", localtimestring);
			errCnt ++;
			}
		}
	}
	return(errCnt);
}

/// Routine for reading and formatting the time as a string.
///	@param[out] timestring 	Pointer to char string in which GPS time is to be written.
/// @note This can use the GPS time from the model, or if configured with USE_SYSTEM_TIME the system time.
void getSdfTime(char *timestring, int size)
{
#ifdef USE_SYSTEM_TIME
	time_t t=0;
	struct tm tdata;

	t = time(NULL);
	localtime_r(&t, &tdata);
	if (timestring && size > 0) {
		if (strftime(timestring, size, "%a %b %e %H:%M:%S %Y", &tdata) == 0) {
			timestring[0] = '\0';
		}
	}
#else
	dbAddr paddr;
	long ropts = 0;
	long nvals = 1;
	long status;

	status = dbNameToAddr(timechannel,&paddr);
	status = dbGetField(&paddr,DBR_STRING,timestring,&ropts,&nvals,NULL);
#endif
}

/// Routine for logging messages to ioc.log file.
/// 	@param[in] message Ptr to string containing message to be logged.
void logFileEntry(char *message)
{
	FILE *log=0;
	char timestring[256];
	long status=0;
	dbAddr paddr;

	getSdfTime(timestring, 256);
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
///	@param[out] times (Optional) An array of at least numchans time_t values to put change times in
int getEpicsSettings(int numchans, time_t *times)
{
int ii;
long status;
int chcount = 0;
double dval = 0;
ADDRESS geaddr;
long statusR = 0;
char sval[64];
time_t *change_t = 0;

	for(ii=0;ii<numchans;ii++)
	{
		// Load main table settings into temp (cdTableP)
		sprintf(cdTableP[ii].chname,"%s",cdTable[ii].chname);
		cdTableP[ii].datatype = cdTable[ii].datatype;
		cdTableP[ii].mask = cdTable[ii].mask;
		cdTableP[ii].initialized = cdTable[ii].initialized;
		// Find address of channel
		status = GET_ADDRESS(cdTableP[ii].chname,&geaddr);
		if(!status) {
			change_t = ( times ? &times[ii] : 0 );
			if(cdTableP[ii].datatype == SDF_NUM)
			{
				statusR = GET_VALUE_NUM(geaddr,&dval,change_t, NULL); //&(cdTableP[ii].connected));
				if (!statusR) cdTableP[ii].data.chval = (cdTable[ii].filterswitch ? (int)dval & 0xffff : dval);
			} else {
				statusR = GET_VALUE_STR(geaddr,sval,sizeof(sval),change_t, NULL); //&(cdTableP[ii].connected));
				if(!statusR) sprintf(cdTableP[ii].data.strval,"%s",sval);
			}
			chcount ++;
		}
	}

}

void encodeBURTString(char *src, char *dest, int dest_size) {
	char *ch = 0;
	int expand = 0;

	if (!src || !dest || dest_size < 1) return;
	dest[0]='\0';
	// make sure the destination can handle expansion which is two characters + a NULL
	if (dest_size < (strlen(src) + 3)) return;
	if (strlen(src) == 0) {
		strcpy(dest, "\\0");
	} else {
		for (ch = src; *ch; ++ch) {
			if (*ch == ' ' || *ch == '\t' || *ch == '\n' || *ch == '"') {
				expand = 1;
				break;
			}
		}
		// we already bounds check this above
		if (expand) {
			sprintf(dest, "\"%s\"", src);
		} else {
			strcpy(dest, src);
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
        FILE *csFile=0;
        char filemsg[128];
	char timestring[128];
	char monitorstring[128];
	char burtString[64+2];
#ifdef CA_SDF
	int precision = 20;
#else
	int precision = 15;
#endif
    // Write out local monitoring table as snap file.
        errno=0;
	burtString[0] = '\0';
        csFile = fopen(filename,"w");
        if (csFile == NULL)
        {
            sprintf(filemsg,"ERROR Failed to open %s - %s",filename,strerror(errno));
            logFileEntry(filemsg);
	    return(-1);
        }
	// Write BURT header
	getSdfTime(timestring, 128);
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
		if (myTable[ii].filterswitch) {
			if ((myTable[ii].mask & ALL_SWSTAT_BITS)== ~0) {
				strcpy(monitorstring, "1");
			} else if ((myTable[ii].mask & ALL_SWSTAT_BITS) == 0) {
				strcpy(monitorstring, "0");
			} else {
				snprintf(monitorstring, sizeof(monitorstring), "0x%x", myTable[ii].mask);
			}
		} else {
			if (myTable[ii].mask)
				strcpy(monitorstring,"1");
			else
				strcpy(monitorstring,"0");
		}
		switch(ftype)
		{
		   case SDF_WITH_INIT_FLAG:
			if(myTable[ii].datatype == SDF_NUM) {
				fprintf(csFile,"%s %d %.*e %s %d\n",myTable[ii].chname,1,precision,myTable[ii].data.chval,monitorstring,myTable[ii].initialized);
			} else {
				encodeBURTString(myTable[ii].data.strval, burtString, sizeof(burtString));
				fprintf(csFile,"%s %d %s %s %d\n",myTable[ii].chname,1,burtString,monitorstring,myTable[ii].initialized);
			}
			break;
		   case SDF_FILE_PARAMS_ONLY:
			if(myTable[ii].initialized) {
				if(myTable[ii].datatype == SDF_NUM) {
					fprintf(csFile,"%s %d %.*e %s\n",myTable[ii].chname,1,precision,myTable[ii].data.chval,monitorstring);
				} else {
					encodeBURTString(myTable[ii].data.strval, burtString, sizeof(burtString));
					fprintf(csFile,"%s %d %s %s\n",myTable[ii].chname,1,burtString,monitorstring);
				}
			}
			break;
		   case SDF_FILE_BURT_ONLY:
			if(myTable[ii].datatype == SDF_NUM) {
				fprintf(csFile,"%s %d %.*e\n",myTable[ii].chname,1,precision,myTable[ii].data.chval);
			} else {
				encodeBURTString(myTable[ii].data.strval, burtString, sizeof(burtString));
				fprintf(csFile,"%s %d %s \n",myTable[ii].chname,1,burtString);
			}
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
FILE *cdf=0;
FILE *adf=0;
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
			status = getEpicsSettings(chNum, NULL);
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
	getSdfTime(timestring, 128);
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
int createSortTableEntries(int numEntries,int wcval,char *wcstring,int *noInit,time_t *times)
{
int ii,jj;
int notMon = 0;
int ret = 0;
int lna = 0;	// line a - uninit chan list index
int lnb = 0;	// line b - non mon chan list index
#ifdef CA_SDF
int lnc = 0;	// line c - disconnected chan list index
#endif
char tmpname[64];
time_t mtime=0;
long nvals = 1;
char liveset[64];
double liveval = 0.0;

	chNotInit = 0;
	chNotMon = 0;
#ifdef CA_SDF
	chDisconnected = 0;
#endif

	D("In createSortTableEntries filter:'%s'\n", (wcval ? wcstring : "None"));
	// Fill uninit and unmon tables.
//	printf("sort table = %d string %s\n",wcval,wcstring);
	for(ii=0;ii<fmNum;ii++) {
		if(!filterTable[ii].init) chNotInit += 1;
		if(filterTable[ii].init && !(filterTable[ii].mask & ALL_SWSTAT_BITS)) chNotMon += 1;
		if(wcval  && (ret = strstr(filterTable[ii].fname,wcstring) == NULL)) {
			continue;
		}
		bzero(tmpname,sizeof(tmpname));
		strncpy(tmpname,filterTable[ii].fname,(strlen(filterTable[ii].fname)-1));
		if(!filterTable[ii].init) {
			sprintf(uninitChans[lna].chname,"%s",tmpname);
			strcpy(uninitChans[lna].burtset,"");
			uninitChans[lna].liveval = 0.0;
			uninitChans[lna].sw[0] = (int)cdTableP[filterTable[ii].sw[0]].data.chval;
			uninitChans[lna].sw[1] = (int)cdTableP[filterTable[ii].sw[1]].data.chval;
			uninitChans[lna].sigNum = filterTable[ii].sw[0] + (filterTable[ii].sw[1] * SDF_MAX_TSIZE);
			uninitChans[lna].filtNum = ii;
			createSwitchText(&(filterTable[ii]),uninitChans[lna].liveset);
			lna ++;
		}
		if(filterTable[ii].init && !(filterTable[ii].mask & ALL_SWSTAT_BITS)) {
			sprintf(unMonChans[lnb].chname,"%s",tmpname);
			unMonChans[lnb].liveval = 0.0;
			unMonChans[lnb].sw[0] = (int)cdTableP[filterTable[ii].sw[0]].data.chval;
			unMonChans[lnb].sw[1] = (int)cdTableP[filterTable[ii].sw[1]].data.chval;
			unMonChans[lnb].sigNum = filterTable[ii].sw[0] + (filterTable[ii].sw[1] * SDF_MAX_TSIZE);
			unMonChans[lnb].filtNum = ii;
			filtStrBitConvert(1,filterTable[ii].swreq, unMonChans[lnb].burtset);
			createSwitchText(&(filterTable[ii]),unMonChans[lnb].liveset);
			lnb ++;
		}
	}
	for(jj=0;jj<numEntries;jj++)
	{
#ifdef VERBOSE_DEBUG
		if (D_NOW) {
			if (strcmp(cdTable[jj].chname, TEST_CHAN) == 0) {
				D("%s: init: %d mask: %d filter: %d\n", TEST_CHAN, cdTable[jj].initialized, cdTable[jj].mask, cdTable[jj].filterswitch);	
			}
		}
#endif
		if(cdTable[jj].filterswitch) continue;
		if(!cdTable[jj].initialized) chNotInit += 1;
		if(cdTable[jj].initialized && !cdTable[jj].mask) chNotMon += 1;
#ifdef CA_SDF
		if(!cdTable[jj].connected) chDisconnected += 1;
#endif
		if(wcval  && (ret = strstr(cdTable[jj].chname,wcstring) == NULL)) {
			continue;
		}
		// Uninitialized channels
		if(!cdTable[jj].initialized && !cdTable[jj].filterswitch) {
			// printf("Chan %s not init %d %d %d\n",cdTable[jj].chname,cdTable[jj].initialized,jj,numEntries);
			if(lna < SDF_ERR_TSIZE) {
				sprintf(uninitChans[lna].chname,"%s",cdTable[jj].chname);
				if(cdTable[jj].datatype == SDF_NUM) {
					liveval = cdTableP[jj].data.chval;
					snprintf(liveset, sizeof(liveset),"%.10lf", liveval);

				} else {
					liveval = 0.0;
					strncpy(liveset, cdTableP[jj].data.strval, sizeof(liveset));
					liveset[sizeof(liveset)-1] = '\0';
				}
				strncpy(uninitChans[lna].liveset, liveset, sizeof(uninitChans[lna].liveset));
				uninitChans[lna].liveset[sizeof(uninitChans[lna].liveset)-1] = '\0';
				uninitChans[lna].liveval = liveval;
				uninitChans[lna].sigNum = jj;
				uninitChans[lna].filtNum = -1;

				if (times) {
					snprintf(unMonChans[lna].timeset, sizeof(unMonChans[lna].timeset), "%s", (times ? ctime(&times[jj]) : " "));
				} else {
					sprintf(unMonChans[lna].timeset,"%s"," ");
				}

				sprintf(uninitChans[lna].timeset,"%s"," ");
				sprintf(uninitChans[lna].diff,"%s"," ");
				lna ++;
			}
		}
		// Unmonitored channels
		if(cdTable[jj].initialized && !cdTable[jj].mask && !cdTable[jj].filterswitch) {
			if(lnb < SDF_ERR_TSIZE) {
				sprintf(unMonChans[lnb].chname,"%s",cdTable[jj].chname);
				if(cdTable[jj].datatype == SDF_NUM)
				{
					snprintf(unMonChans[lnb].burtset, sizeof(unMonChans[lnb].burtset),"%.10lf",cdTable[jj].data.chval);
					snprintf(unMonChans[lnb].liveset, sizeof(unMonChans[lnb].liveset),"%.10lf",cdTableP[jj].data.chval);
					unMonChans[lnb].liveval = cdTableP[jj].data.chval;
				} else {
					sprintf(unMonChans[lnb].burtset,"%s",cdTable[jj].data.strval);
					sprintf(unMonChans[lnb].liveset,"%s",cdTableP[jj].data.strval);
					unMonChans[lnb].liveval = 0.0;
				}

                unMonChans[lnb].sigNum = jj;
				unMonChans[lnb].filtNum = -1;

				if (times) {
					snprintf(unMonChans[lnb].timeset, sizeof(unMonChans[lnb].timeset), "%s", (times ? ctime(&times[jj]) : " "));
					sprintf(unMonChans[lnb].diff,"%s"," ");
				} else {
					sprintf(unMonChans[lnb].timeset,"%s"," ");
					sprintf(unMonChans[lnb].diff,"%s"," ");
				}

				lnb ++;
			}
		}
#ifdef CA_SDF
		if(!cdTable[jj].connected && !cdTable[jj].filterswitch) {
			if (lnc < SDF_ERR_TSIZE) {
				sprintf(disconnectChans[lnc].chname, "%s", cdTable[jj].chname);
				if(cdTable[jj].datatype == SDF_NUM) {
					liveval = cdTableP[jj].data.chval;
					snprintf(liveset, sizeof(liveset),"%.10lf", liveval);

				} else {
					liveval = 0.0;
					strncpy(liveset, cdTableP[jj].data.strval, sizeof(liveset));
					liveset[sizeof(liveset)-1] = '\0';
				}
				strncpy(disconnectChans[lnc].liveset, liveset, sizeof(disconnectChans[lnc].liveset));
				disconnectChans[lnc].liveset[sizeof(disconnectChans[lnc].liveset)-1] = '\0';
				disconnectChans[lnc].liveval = liveval;
                disconnectChans[lnc].sigNum = jj;
				disconnectChans[lnc].filtNum = -1;
				sprintf(disconnectChans[lnc].timeset,"%s"," ");
				sprintf(disconnectChans[lnc].diff,"%s"," ");
				lnc ++;

			}
		}
#endif
	}
	// Clear out the uninit tables.
	for(jj=lna;jj<(lna + 50);jj++)
	{
		sprintf(uninitChans[jj].chname,"%s"," ");
		sprintf(uninitChans[jj].burtset,"%s"," ");
		sprintf(uninitChans[jj].liveset,"%s"," ");
		uninitChans[jj].liveval = 0.0;
		sprintf(uninitChans[jj].timeset,"%s"," ");
		sprintf(uninitChans[jj].diff,"%s"," ");
        uninitChans[jj].sigNum = 0;         // is this the right value, should it be -1?
		uninitChans[jj].filtNum = -1;
	}
	// Clear out the unmon tables.
	for(jj=lnb;jj<(lnb + 50);jj++)
	{
		sprintf(unMonChans[jj].chname,"%s"," ");
		sprintf(unMonChans[jj].burtset,"%s"," ");
		sprintf(unMonChans[jj].liveset,"%s"," ");
		unMonChans[jj].liveval = 0.0;
		sprintf(unMonChans[jj].timeset,"%s"," ");
		sprintf(unMonChans[jj].diff,"%s"," ");
        unMonChans[jj].sigNum = 0;
		unMonChans[jj].filtNum = -1;
	}
#ifdef CA_SDF
	// Clear out the disconnected tables.
	for(jj=lnc;jj<(lnc + 50);jj++)
	{
		sprintf(disconnectChans[jj].chname,"%s"," ");
		sprintf(disconnectChans[jj].burtset,"%s"," ");
		sprintf(disconnectChans[jj].liveset,"%s"," ");
		disconnectChans[jj].liveval = 0.0;
		sprintf(disconnectChans[jj].timeset,"%s"," ");
		sprintf(disconnectChans[jj].diff,"%s"," ");
        disconnectChans[jj].sigNum = 0;
		disconnectChans[jj].filtNum = -1;
	}
#endif
	*noInit = lna;
	return(lnb);
}

/// Common routine to load monitoring tables into EPICS channels for MEDM screen.
int reportSetErrors(char *pref,			///< Channel name prefix from EPICS environment. 
		     int numEntries, 			///< Number of entries in table to be reported.
		     SET_ERR_TABLE setErrTable[],	///< Which table to report to EPICS channels.
		     int page,                      ///< Which page of 40 to display.
             int linkInFilters)				///< Should the SDF_FM_LINE values be set.
{

int ii;
dbAddr saddr;
dbAddr baddr;
dbAddr maddr;
dbAddr taddr;
dbAddr daddr;
dbAddr laddr;
dbAddr faddr;
char s[64];
char s1[64];
char s2[64];
char s3[64];
char s4[64];
char sl[64];
char sf[64];
//char stmp[128];
long status = 0;
char clearString[62] = "                       ";
int flength = 62;
int rc = 0;
int myindex = 0;
int numDisp = 0;
int lineNum = 0;
int mypage = 0;
int lineCtr = 0;
int zero = 0;
int minusOne = -1;


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
		//sprintf(stmp, "%s%s", (setErrTable[ii].filtNum >= 0 ? "* " : ""), setErrTable[ii].chname);
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


		sprintf(sf, "%s_SDF_FM_LINE_%d", pref, lineNum);
		status = dbNameToAddr(sf,&faddr);
		status = dbPutField(&faddr,DBR_LONG,(linkInFilters ? &setErrTable[ii].filtNum : &minusOne),1);

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

		sprintf(sf, "%s_SDF_FM_LINE_%d", pref, ii);
		status = dbNameToAddr(sf,&faddr);
		status = dbPutField(&faddr,DBR_LONG,&minusOne,1);

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
	ADDRESS paddr;
	long status=0;
	int ii;
	double rval;
	char sval[128];
	time_t mtime;
	char localtimestring[256];
	int localErr = 0;
	char liveset[64];
	char burtset[64];
	char diffB2L[64];
	char swName[64];
	double sdfdiff = 0.0;
	double liveval = 0.0;
	int filtDiffs=0;

#ifdef VERBOSE_DEBUG
/*
	char *__table_name = "setErrTable or unknown";
	if (setErrTable == unknownChans) __table_name = "unknownChans";
	else if (setErrTable == uninitChans) __table_name == "uninitChans";
	else if (setErrTable == unMonChans) __table_name == "unMonChans";
	else if (setErrTable == readErrTable) __table_name == "readErrTable";
	else if (setErrTable == cdTableList) __table_name == "cdTableList";
	D("In spChecker cdTable -> '%s' monAll(%d) listAll(%d)\n", __table_name, monitorAll, listAll);
*/
#endif
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
				mtime = 0;
				// Find address of channel
				status = GET_ADDRESS(cdTable[ii].chname,&paddr);
				// If this is a digital data type, then get as double.
				if(cdTable[ii].datatype == SDF_NUM)
				{
					status = GET_VALUE_NUM(paddr,&rval,&mtime, NULL); //&(cdTable[ii].connected));
					if(cdTable[ii].data.chval != rval || listAll)
					{
						sdfdiff = fabs(cdTable[ii].data.chval - rval);
						snprintf(burtset,sizeof(burtset),"%.10lf",cdTable[ii].data.chval);
						snprintf(liveset,sizeof(liveset),"%.10lf",rval);
						liveval = rval;
						snprintf(diffB2L,sizeof(diffB2L),"%.8le",sdfdiff);
						localErr = 1;
					}
				// If this is a string type, then get as string.
				} else {
					status = GET_VALUE_STR(paddr,sval,sizeof(sval),&mtime, NULL); //&(cdTable[ii].connected));
					if(strcmp(cdTable[ii].data.strval,sval) != 0 || listAll)
					{
						sprintf(burtset,"%s",cdTable[ii].data.strval);
						sprintf(liveset,"%s",sval);
						liveval = 0.0;
						sprintf(diffB2L,"%s","                                   ");
						localErr = 1;
					}
				}
				if(localErr) *totalDiffs += 1;
				if(localErr && wcVal  && (strstr(cdTable[ii].chname,wcstring) == NULL))
					localErr = 0;
				// If a diff was found, then write info the EPICS setpoint diff table.
				if(localErr)
				{
					sprintf(setErrTable[errCntr].chname,"%s", cdTable[ii].chname);

					sprintf(setErrTable[errCntr].burtset, "%s", burtset);

					sprintf(setErrTable[errCntr].liveset, "%s", liveset);
					setErrTable[errCntr].liveval = liveval;
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

/// Modify the global cdTable as directed by applying changes from modTable where the channel is marked as apply or monitor
///
/// @param numEntries[in] The number of entries in modTable
/// @param modTable[in] An array of SET_ERR_TABLE entries that hold the current views state to be merged with cdTable
///
/// @remark modifies cdTable
int modifyTable(int numEntries,SET_ERR_TABLE modTable[])
{
int ii,jj;
int fmIndex = -1;
unsigned int sn,sn1;
int found = 0;
	for(ii=0;ii<numEntries;ii++)
	{
		// if accept or monitor is set
		if(modTable[ii].chFlag > 3) 
		{
			found = 0;
			for(jj=0;jj<chNum;jj++)
			{
				if (strcmp(cdTable[jj].chname,modTable[ii].chname) == 0) {
					if ( CHFLAG_ACCEPT_BIT(modTable[ii].chFlag) ) {
						if(cdTable[jj].datatype == SDF_NUM) cdTable[jj].data.chval = modTable[ii].liveval;/* atof(modTable[ii].liveset);*/
						else sprintf(cdTable[jj].data.strval,"%s",modTable[ii].liveset);
						cdTable[jj].initialized = 1;
						found = 1;
						fmIndex = cdTable[jj].filterNum;
					}
					if(CHFLAG_MONITOR_BIT(modTable[ii].chFlag)) {
						fmIndex = cdTable[jj].filterNum;
						if (fmIndex >= 0 && fmIndex < SDF_MAX_FMSIZE) {
							// filter module use the manualy modified state, cannot just toggle
							cdTable[jj].mask = filterMasks[fmIndex];
						} else {
							// regular channel just toggle on/off state
							cdTable[jj].mask = (cdTable[jj].mask ? 0 : ~0);
						}
						found = 1;
					}
				}
			}
			if(modTable[ii].filtNum >= 0 && !found) { 
				fmIndex = modTable[ii].filtNum;
				// printf("This is a filter from diffs = %s\n",filterTable[fmIndex].fname);
				filterTable[fmIndex].newSet = 1;
				sn = modTable[ii].sigNum;
				sn1 = sn / SDF_MAX_TSIZE;
				sn %= SDF_MAX_TSIZE;
				if(CHFLAG_ACCEPT_BIT(modTable[ii].chFlag)) {
					cdTable[sn].data.chval = modTable[ii].sw[0];
					cdTable[sn].initialized = 1;
					cdTable[sn1].data.chval = modTable[ii].sw[1];
					cdTable[sn1].initialized = 1;
					filterTable[fmIndex].init = 1;
				}
				if(CHFLAG_MONITOR_BIT(modTable[ii].chFlag)) {
					filterTable[fmIndex].mask = filterMasks[fmIndex];
				 	cdTable[sn].mask = filterMasks[fmIndex];
				 	cdTable[sn1].mask = filterMasks[fmIndex];
				}
			}
		}
	}
	newfilterstats(fmNum);
	return(0);
}

int resetSelectedValues(int errNum, SET_ERR_TABLE modTable[])
{
long status;
int ii;
int sn;
int sn1 = 0;
ADDRESS saddr;

	for(ii=0;ii<errNum;ii++)
	{
		if (modTable[ii].chFlag & 2)
		{
			sn = modTable[ii].sigNum;
			if(sn > SDF_MAX_TSIZE) {
				sn1 = sn / SDF_MAX_TSIZE;
				sn %= SDF_MAX_TSIZE;
			}
			status = GET_ADDRESS(cdTable[sn].chname,&saddr);
			if (cdTable[sn].datatype == SDF_NUM) status = PUT_VALUE(saddr,SDF_NUM,&(cdTable[sn].data.chval));
			else status = PUT_VALUE(saddr,SDF_STR,cdTable[sn].data.strval);;
			if(sn1) {
				status = GET_ADDRESS(cdTable[sn1].chname,&saddr);
				status = PUT_VALUE(saddr,SDF_NUM,&(cdTable[sn1].data.chval));
			}
		}
	}
	return(0);
}

/// This function sets filter module request fields to aid in decoding errant filter module switch settings.
void newfilterstats(int numchans) {
	ADDRESS paddr;
	long status;
	int ii;
	FILE *log=0;
	char chname[128];
	unsigned int mask = 0x1ffff;
	int tmpreq;
	int counter = 0;
	int rsw1,rsw2;
	unsigned int tmpL = 0;

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
			bzero(chname,sizeof(chname));
			// Find address of channel
			strcpy(chname,filterTable[ii].fname);
			strcat(chname,"SWREQ");
			status = GET_ADDRESS(chname,&paddr);
			if(!status) {
				tmpL = (unsigned int)filterTable[ii].swreq;
				status = PUT_VALUE_INT(paddr,&tmpL);
			}
			bzero(chname,sizeof(chname));
			// Find address of channel
			strcpy(chname,filterTable[ii].fname);
			strcat(chname,"SWMASK");
			status = GET_ADDRESS(chname,&paddr);
			if(!status) {
				status = PUT_VALUE_INT(paddr,&mask);
			}
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
	ADDRESS paddr;
	long status;
	int ii;

	// chNotFound = 0;
	switch (command)
	{
		case SDF_LOAD_DB_ONLY:
		case SDF_LOAD_PARTIAL:
			for(ii=0;ii<numchans;ii++) {
				// Find address of channel
				status = GET_ADDRESS(myTable[ii].chname,&paddr);
				if (!status)
				{
					if (myTable[ii].datatype == SDF_NUM)
					{
						status = PUT_VALUE(paddr,SDF_NUM,&(myTable[ii].data.chval));
					} else {
						status = PUT_VALUE(paddr,SDF_STR,myTable[ii].data.strval);
					}
				}
				else {				// Write errors to chan not found table.
				printf("CNF for %s = %d\n",myTable[ii].chname,status);
				#if 0
					if(chNotFound < SDF_ERR_TSIZE) {
						sprintf(unknownChans[chNotFound].chname,"%s",myTable[ii].chname);
						sprintf(unknownChans[chNotFound].liveset,"%s"," ");
						unknownChans[chNotFound].liveval = 0.0;
						sprintf(unknownChans[chNotFound].timeset,"%s"," ");
						sprintf(unknownChans[chNotFound].diff,"%s"," ");
						unknownChans[chNotFound].chFlag = 0;
					}
					chNotFound ++;
				#endif
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
				// FIXME: check does this make sense w/ a bitmask in mask?
				// can filter modules get here?  It seems that they will, so would we need
				// to mask the value we set ?
			    if(myTable[ii].mask) {
			    	//Find address of channel
			    	status = GET_ADDRESS(myTable[ii].chname,&paddr);
			    	if (!status)
			    	{
			    		if (myTable[ii].datatype == SDF_NUM)
						{
							status = PUT_VALUE(paddr,SDF_NUM,&(myTable[ii].data.chval));
						} else {
							status = PUT_VALUE(paddr,SDF_STR,myTable[ii].data.strval);
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
	FILE *cdf=0;
	FILE *adf=0;
	char c=0;
	int ii=0;
	int lock=0;
	char s1[128],s2[128],s3[128],s4[128],s5[128],s6[128],s7[128],s8[128];
	char ls[6][64];
	dbAddr paddr;
	long status=0;
	int lderror = 0;
	int ropts = 0;
	int nvals = 1;
	int starttime=0,totaltime=0;
	char timestring[256];
	char line[128];
	char *fs=0;
	char ifo[4];
	double tmpreq = 0;
	char fname[128];
	int fmatch = 0;
	int fmIndex = -1;
	char errMsg[128];
	int argcount = 0;
	int isalarm = 0;
	int lineCnt = 0;

	s1[0]=s2[0]=s3[0]=s4[0]=s5[0]=s6[0]=s7[0]=s8[0]='\0';
	timestring[0]='\0';
	line[0]='\0';
	ifo[0]='\0';
	fname[0]=errMsg[0]='\0';
	for (ii = 0; ii < 6; ii++) ls[ii][0]='\0';
	rderror = 0;

	clock_gettime(CLOCK_REALTIME,&t);
	starttime = t.tv_nsec;

	getSdfTime(timestring, 256);

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
		bzero(s3,sizeof(s3));
		strncpy(ifo,pref,3);
		chNotFound = 0;
		while(fgets(line,sizeof line,cdf) != NULL)
		{
			isalarm = 0;
			lineCnt ++;
			strcpy(s4,"x");
			argcount = parseLine(line,sizeof(s1),s1,s2,s3,s4,s5,s6);
			if (strcmp(s4, "") == 0) strcpy(s4, "0");
			if(argcount == -1) {
				sprintf(readErrTable[rderror].chname,"%s", s1);
				sprintf(readErrTable[rderror].burtset, "%s", "Improper quotations ");
				sprintf(readErrTable[rderror].liveset, "Line # %d", lineCnt);
				readErrTable[rderror].liveval = 0.0;
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
					// 0x... -> bit mask
					// 0|1 -> 0|0xfffffffff
					if (s4[1] == 'x') {
						if (sscanf(s4, "0x%xd", &(cdTableP[chNumP].mask)) <= 0)
							cdTableP[chNumP].mask = 0;
					} else {
						cdTableP[chNumP].mask = atoi(s4);
						if((cdTableP[chNumP].mask < 0) || (cdTableP[chNumP].mask > 1))	
							cdTableP[chNumP].mask = 0;
						if (cdTableP[chNumP].mask > 0) cdTableP[chNumP].mask = ~0;
					}
					// printf("mask: %d %s\n", cdTableP[chNumP].mask, cdTableP[chNumP].chname);
				} else {
					cdTableP[chNumP].mask = 0;
				}
				// Find channel in full list and replace setting info
				fmatch = 0;
				// We can set alarm values, but do not put them in cdTable
				if( isAlarmChannel(cdTableP[chNumP].chname) )
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
									readErrTable[rderror].liveval = 0.0;
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
							//if(cdTableP[chNumP].mask != -1)
								cdTable[ii].mask = cdTableP[chNumP].mask;
							cdTable[ii].initialized = 1;
						}
					}
				   }
				   // if(!fmatch) printf("NEW channel not found %s %d\n",cdTableP[chNumP].chname,chNumP);
				}
				// The following loads info into the filter module table if a FM switch
				fmIndex = -1;
				if(((strstr(s1,"_SW1S") != NULL) && (strstr(s1,"_SW1S.") == NULL)) ||
					((strstr(s1,"_SW2S") != NULL) && (strstr(s1,"_SW2S.") == NULL)))
				{
				   	bzero(fname,sizeof(fname));
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
				if(fmatch) {
					chNumP ++;
				} else {
					// printf("CNF for %s \n",cdTableP[chNumP].chname);
					if(chNotFound < SDF_ERR_TSIZE) {
						sprintf(unknownChans[chNotFound].chname,"%s",cdTableP[chNumP].chname);
						status = GET_ADDRESS(cdTableP[chNumP].chname,&paddr);
						if(!status) { 
							sprintf(unknownChans[chNotFound].liveset,"%s","RO Channel ");
						} else {
							sprintf(unknownChans[chNotFound].liveset,"%s","  ");
						}
						unknownChans[chNotFound].liveval = 0.0;
						sprintf(unknownChans[chNotFound].timeset,"%s"," ");
						sprintf(unknownChans[chNotFound].diff,"%s"," ");
						unknownChans[chNotFound].chFlag = 0;
					}
					chNotFound ++;
				}
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

void registerFilters() {
	int ii = 0;
	char tmpstr[64];
	int amatch = 0, jj = 0;
	fmNum = 0;
	for(ii=0;ii<chNum;ii++) {
		if(cdTable[ii].filterswitch == 1)
		{
			strncpy(filterTable[fmNum].fname,cdTable[ii].chname,(strlen(cdTable[ii].chname)-4));
			snprintf(tmpstr, sizeof(tmpstr),"%s%s",filterTable[fmNum].fname,"SW2S");
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
	printf("%d filters\n",fmNum);
}

/// Provide initial values to the filterMasks arrray and the SDF_FM_MASK and SDF_FM_MASK_CTRL variables
///
void setupFMArrays(char *pref, int *fmMasks, dbAddr *fmMaskAddr, dbAddr *fmCtrlAddr) {
	int ii = 0;
	int sw1 = 0;
	int sw2 = 0;
	long status = 0;
	char name[256];
	char ctrl[256];
	int zero = 0;

	for (ii = 0; ii < SDF_MAX_FMSIZE; ++ii) {
		sw1 = filterTable[ii].sw[0];
		sw2 = filterTable[ii].sw[1];
		fmMasks[ii] = filterTable[ii].mask = cdTable[sw1].mask | cdTable[sw2].mask;

		sprintf(name, "%s_SDF_FM_MASK_%d", pref, ii);
		status = dbNameToAddr(name, &fmMaskAddr[ii]);
		status = dbPutField(&fmMaskAddr[ii],DBR_LONG,&(fmMasks[ii]),1);

		sprintf(ctrl, "%s_SDF_FM_MASK_CTRL_%d", pref, ii);
		status = dbNameToAddr(ctrl, &fmCtrlAddr[ii]);
		status = dbPutField(&fmCtrlAddr[ii],DBR_LONG,&zero,1);

	} 
}

/// Copy the filter mask information from cdTable into fmMasks
///
void resyncFMArrays(int *fmMasks, dbAddr *fmMaskAddr) {
	int ii = 0;
	int sw1 = 0;
	int sw2 = 0;
	long status = 0;

	for (ii = 0; ii < SDF_MAX_FMSIZE; ++ii) {
		sw1 = filterTable[ii].sw[0];
		sw2 = filterTable[ii].sw[1];
		fmMasks[ii] = filterTable[ii].mask = (cdTable[sw1].mask | cdTable[sw2].mask) & ALL_SWSTAT_BITS;

		status = dbPutField(&fmMaskAddr[ii],DBR_LONG,&(fmMasks[ii]),1);
	}
}

/// Process a command to flip or set filter bank monitor bits
///
/// @param fMask[in/out] A pointer to a list of int filter monitoring masks
/// @param fmMaskAddr[in/out] A pointer to the list of monitor masks addresses
/// @param fmCtrlAddr[in/out] A pointer to the list of control word addresses
/// @param selectCounter[in/out] A array of 3 ints with the count of changes selected
/// @param errCnt[in] Number or entries in setErrTable
/// @param setErrTable[in/out] The array of SET_ERR_TABLE entries to update the chFlag field on
///
/// @remark The SDF_FM_MASK_CTRL channels signal a change in filter bank
/// monitoring.  This checks all of the MASK_CTRL signals and modifies the
/// filter mask in the fMask array as needed.
///
/// @remark Each of the input arrays is expected to have a length of SDF_MAX_FMSIZE
/// with fmNum in actual use.
void processFMChanCommands(int *fMask, dbAddr *fmMaskAddr, dbAddr *fmCtrlAddr, int *selectCounter, int errCnt, SET_ERR_TABLE *setErrTable) {
	int ii = 0;
	int jj = 0;
	int refMask = 0;
	int preMask = 0;
	int differsPre = 0, differsPost = 0;
	int ctrl = 0;
	long status = 0;
	long ropts = 0;
	long nvals = 1;
	int foundCh = 0;

	for (ii = 0; ii < SDF_MAX_FMSIZE && ii < fmNum; ++ii) {
		status = dbGetField(&fmCtrlAddr[ii], DBR_LONG, &ctrl, &ropts, &nvals, NULL);

		// only do work if there is a change
		if (!ctrl) continue;

		refMask = filterTable[ii].mask;
		preMask = fMask[ii];
		differsPre = ((refMask & ALL_SWSTAT_BITS) != (fMask[ii] & ALL_SWSTAT_BITS));

		fMask[ii] ^= ctrl;

		differsPost = ((refMask & ALL_SWSTAT_BITS) != (fMask[ii] & ALL_SWSTAT_BITS));

		foundCh = 0;
		/* if there is a change, update4 the selection count) */
		if (differsPre != differsPost) {
			for (jj = 0; jj < errCnt && setErrTable[jj].filtNum >= 0 && setErrTable[jj].filtNum != ii; ++jj) {}
			if (jj < errCnt && setErrTable[jj].filtNum == ii) {
				setErrTable[jj].chFlag ^= CHFLAG_MONITOR_BIT_VAL;
				foundCh = setErrTable[jj].chFlag;
				selectCounter[2] += ( differsPre ? -1 : 1 );
			}
		}

		printf("Signal 0x%x set on '%s' ref=0x%x pre=0x%x post=0x%x pre/post diff(%d,%d) chFlag = 0x%x\n", ctrl, filterTable[ii].fname, refMask, preMask, fMask[ii], differsPre, differsPost, foundCh);

		dbPutField(&fmMaskAddr[ii],DBR_LONG,&(fMask[ii]),1);

		ctrl = 0;
		dbPutField(&fmCtrlAddr[ii],DBR_LONG,&ctrl,1);
	}
}

#ifdef CA_SDF
void nullCACallback(struct event_handler_args args) {}

int getCAIndex(char *entry, ADDRESS *addr) {
	int ii = 0;

	if (!entry || !addr) return;
	for (ii = 0; ii < chNum; ++ii) {
		if (strcmp(cdTable[ii].chname, entry) == 0) {
			*addr = ii;
			return 0;
		}
	}
	*addr = -1;
	return 1;
}

int setCAValue(ADDRESS ii, int type, void *data)
{
	int status = ECA_NORMAL;
	int result = 1;
	
	if (ii >= 0 && ii < chNum) {
		if (type == SDF_NUM) {
			status = ca_put_callback(DBR_DOUBLE, caTable[ii].chanid, (double *)data, nullCACallback, NULL);
		} else {
			status = ca_put_callback(DBR_STRING, caTable[ii].chanid, (char *)data, nullCACallback, NULL);
		}
		result = (status == ECA_NORMAL ? 0 : 1);
	}
	return result;
}

int setCAValueLong(ADDRESS ii, unsigned long *data) {
	double tmp = 0.0;

	if (!data) return 1;
	tmp = (double)*data;
	return setCAValue(ii,SDF_NUM,(void*)&tmp);
}

int syncEpicsDoubleValue(int index, double *dest, time_t *tp, int *connp) {
	int debug = 0;
	if (!dest || index < 0 || index >= chNum) return 1;
#if VERBOSE_DEBUG
	if (strcmp(cdTable[caTable[index].chanIndex].chname, TEST_CHAN) == 0) {
		debug=1;
	}
#endif
	pthread_mutex_lock(&caTableMutex);
	if (caTable[index].datatype == SDF_NUM) {
		*dest = caTable[index].data.chval;
		if (tp) {
			*tp = caTable[index].mod_time;
		}
		if (connp) {
			*connp = caTable[index].connected;
		}
	}
	pthread_mutex_unlock(&caTableMutex);
	return 0;
}

int syncEpicsIntValue(ADDRESS index, unsigned int *dest, time_t *tp, int *connp) {
	double tmp = 0.0;
	int result = 0;

	if (!dest) return 1;
	result = syncEpicsDoubleValue(index, &tmp, tp, connp);
	*dest = (unsigned int)(tmp);
	return result;
}

int syncEpicsStrValue(int index, char *dest, int dest_size, time_t *tp, int *connp) {
	if (!dest || index < 0 || index >= chNum || dest_size < 1) return 1;
	dest[0] = '\0';
	pthread_mutex_lock(&caTableMutex);
	if (caTable[index].datatype == SDF_STR) {
		const int MAX_STR_LEN = sizeof(caTable[index].data.strval);
		strncpy(dest, caTable[index].data.strval, (dest_size < MAX_STR_LEN ? dest_size : MAX_STR_LEN));
		dest[dest_size-1] = '\0';
		if (tp) {
			*tp = caTable[index].mod_time;
		}
		if (connp) {
			*connp = caTable[index].connected;
		}
	}
	pthread_mutex_unlock(&caTableMutex);
	return 0;
}

/// Routine to handle subscription callbacks
void subscriptionHandler(struct event_handler_args args) {
	float val = 0.0;
	EPICS_CA_TABLE *entry = args.usr;
	EPICS_CA_TABLE *origEntry = entry;
	int initialRedirIndex = 0;

	if (args.status != ECA_NORMAL ||  !entry) {
		return;
	}

	pthread_mutex_lock(&caTableMutex);

	// If this entry has just reconnected, then do not write the old copy, write to the temporary/redir dest
	initialRedirIndex = entry->redirIndex;
	if (entry->redirIndex >= 0) {
		if (entry->redirIndex < SDF_MAX_TSIZE) {
			entry = &(caConnTable[entry->redirIndex]);
		} else {
			entry = &(caConnEnumTable[entry->redirIndex - SDF_MAX_TSIZE]);
		}
	}

	// if we are getting data, we must be connected.
	entry->connected = 1;
	const int MAX_STR_LEN = sizeof(entry->data.strval);
	if (args.type == DBR_TIME_DOUBLE) {
		struct dbr_time_double *dVal = (struct dbr_time_double *)args.dbr;
		entry->data.chval = dVal->value;
		entry->mod_time = dVal->stamp.secPastEpoch + POSIX_TIME_AT_EPICS_EPOCH;
	} else if (args.type == DBR_TIME_STRING) {
		struct dbr_time_string *sVal = (struct dbr_time_string *)args.dbr;
		strncpy(entry->data.strval, sVal->value, MAX_STR_LEN);
		entry->data.strval[MAX_STR_LEN - 1] = '\0';
		entry->mod_time = sVal->stamp.secPastEpoch + POSIX_TIME_AT_EPICS_EPOCH;
	} else if (args.type == DBR_GR_ENUM) {
		struct dbr_gr_enum *eVal = (struct dbr_gr_enum *)args.dbr;
		if (initialRedirIndex >= SDF_MAX_TSIZE) {			
			if (entry->datatype == SDF_UNKNOWN) {
				// determine the proper type
				entry->datatype = SDF_NUM;
				if (eVal->no_str >= 2) {
					// call it a string if there are two non-null distinct
					// strings for the first two entries
					if ((strlen(eVal->strs[0]) > 0 && strlen(eVal->strs[1]) > 0) &&
						strcmp(eVal->strs[0], eVal->strs[1]) != 0)
					{
						entry->datatype = SDF_STR;
					}
				}
				++chEnumDetermined;
			}
		}
		if (entry->datatype == SDF_NUM) {
			entry->data.chval = (double)(eVal->value);
		} else {
			if (eVal->value > 0 && eVal->value < eVal->no_str) {
				strncpy(entry->data.strval, eVal->strs[eVal->value], MAX_STR_LEN);
			} else {
				snprintf(entry->data.strval, MAX_STR_LEN, "Unexpected enum value received - %d", (int)eVal->value);
			}
			entry->data.strval[MAX_STR_LEN - 1] = '\0';
		}
		// The dbr_gr_enum type does not have time information, so we use current time
		entry->mod_time = time(NULL);
	}
	pthread_mutex_unlock(&caTableMutex);
}

void connectCallback(struct connection_handler_args args) {
	EPICS_CA_TABLE *entry = (EPICS_CA_TABLE*)ca_puser(args.chid);
	EPICS_CA_TABLE *connEntry = 0;
	int dbr_type = -1;
	int sdf_type = SDF_NUM;
	int chanIndex = SDF_MAX_TSIZE;
	int typeChange = 0;
	int is_enum = 0;

	if (entry) {
		chanIndex = entry->chanIndex;

		// determine the field type for conn up events
		// do this outside of the critical section
		if (args.op == CA_OP_CONN_UP) {
			dbr_type = dbf_type_to_DBR(ca_field_type(args.chid));
			if (dbr_type_is_ENUM(dbr_type)) {
				is_enum = 1;
				sdf_type = SDF_UNKNOWN;
			} else if (dbr_type == DBR_STRING) {
				sdf_type = SDF_STR;
			}
		}

		pthread_mutex_lock(&caTableMutex);
		if (args.op == CA_OP_CONN_UP) {
			// connection
			if (is_enum) {
				entry->redirIndex = entry->chanIndex + SDF_MAX_TSIZE;
				connEntry = &(caConnEnumTable[entry->chanIndex]);
			} else {
				// reserve a new conn table entry if one is not already reserved
				if (entry->redirIndex < 0) {
					entry->redirIndex = chConnNum;
					++chConnNum;
				}
				connEntry = &(caConnTable[entry->redirIndex]);
			}
			entry->chanid = args.chid;

			// now copy items over to the connection record
			connEntry->redirIndex = -1;

			connEntry->datatype = sdf_type;
			connEntry->data.chval = 0.0;
			entry->connected = 1;
			connEntry->connected = 1;
			connEntry->mod_time = 0;
			connEntry->chanid = args.chid;
			connEntry->chanIndex = entry->chanIndex;
			typeChange = (connEntry->datatype != entry->datatype);

		} else {
			// disconnect
			entry->connected = 0;
		}
		pthread_mutex_unlock(&caTableMutex);

		// now register/clear the subscription callback
		pthread_mutex_lock(&caEvidMutex);
		if (args.op == CA_OP_CONN_UP) {
			// connect
			// if we are subscribed but the types are wrong, then unsubscribe
			if (caEvid[chanIndex] && typeChange) {
				ca_clear_subscription(caEvid[chanIndex]);
				caEvid[chanIndex] = 0;
			}
			// if we are not subscribed become subscribed
			if (!caEvid[chanIndex]) {
				chtype subtype = (sdf_type == SDF_NUM ? DBR_TIME_DOUBLE : DBR_TIME_STRING);
				if (is_enum) {
					subtype = DBR_GR_ENUM;
				}
				ca_create_subscription(subtype, 0, args.chid, DBE_VALUE, subscriptionHandler, entry, &(caEvid[chanIndex]));
			}
		} else {
			// disconnect
			if (caEvid[chanIndex]) {
				ca_clear_subscription(caEvid[chanIndex]);
				caEvid[chanIndex] = 0;
			}
		}
		pthread_mutex_unlock(&caEvidMutex);
	}
}

/// Routine to register a channel
void registerPV(char *PVname)
{
	long status=0;
	chid chid1;

	if (chNum >= SDF_MAX_TSIZE) {
		droppedPVCount++;
		return;
	}
	//printf("Registering %s\n", PVname);
	pthread_mutex_lock(&caTableMutex);
	caTable[chNum].datatype = SDF_NUM;
	caTable[chNum].connected = 0;
	strncpy(cdTable[chNum].chname, PVname, 128);
	cdTable[chNum].chname[128-1] = '\0';
	cdTable[chNum].datatype = SDF_NUM;
	cdTable[chNum].initialized = 0;
	cdTable[chNum].filterswitch = 0;
	cdTable[chNum].filterNum = -1;
	cdTable[chNum].error = 0;
	cdTable[chNum].initialized = 0;
	cdTable[chNum].mask = 0;
	pthread_mutex_unlock(&caTableMutex);

	status = ca_create_channel(PVname, connectCallback, &(caTable[chNum]), 0, &chid1);

	if((strstr(PVname,"_SW1S") != NULL) && (strstr(PVname,"_SW1S.") == NULL))
	{
		cdTable[chNum].filterswitch = 1;
	}
	if((strstr(PVname,"_SW2S") != NULL) && (strstr(PVname,"_SW2S.") == NULL))
	{
		cdTable[chNum].filterswitch = 2;
	}

	caTable[chNum].chanid = chid1;
	++chNum;
}

/// Convert a daq data type to a SDF type
int daqToSDFDataType(int daqtype) {
	if (daqtype == 4) {
		return SDF_NUM;
	}
	if (daqtype == 128) {
		return SDF_STR;
	}
	return SDF_UNKNOWN;
}

/// Parse an BURT request file and register each channel to be monitored
/// @input fname - name of the file to open
/// @input pref - the ifo name, should be 3 characters
void parseChannelListReq(char *fname, char *pref) {
	FILE *f = 0;
	char ifo[4];
	char line[128];
	char s1[128],s2[128],s3[128],s4[128],s5[128],s6[128],s7[128],s8[128];
	int argcount = 0;

	line[0]=s1[0]=s2[0]=s3[0]=s4[0]=s5[0]=s6[0]=s7[0]=s8[0]='\0';
	strncpy(ifo, pref, 3);
	ifo[3] = '\0';
	f = fopen(fname, "r");
	if (!f) return;

	while (fgets(line, sizeof(line), f) != NULL) {
		argcount = parseLine(line, sizeof(s1), s1, s2, s3, s4, s5, s6);
		if (argcount < 1) continue;
		if (strncmp(s1, ifo, 3) != 0) continue;
		if (strstr(s1,"_SWMASK") != NULL ||	
			strstr(s1,"_SDF_NAME") != NULL ||
			strstr(s1,"_SWREQ") != NULL) continue;
		if (isAlarmChannel(s1)) continue;
		registerPV(s1);
	}
	fclose(f);
}

/// Routine to get the state of the CA Thread
int getCAThreadState() {
	int state = 0;
	pthread_mutex_lock(&caStateMutex);
	state = caThreadState;
	pthread_mutex_unlock(&caStateMutex);
	return state;
}

/// Routine to set the state of the CA Thread
void setCAThreadState(int state) {
	pthread_mutex_lock(&caStateMutex);
	caThreadState = state;
	pthread_mutex_unlock(&caStateMutex);
}

// copy the given entry (which should be in caConnTable or caConnEnumTable) to caTable
// update cdTable with type information if needed.
void copyConnectedCAEntry(EPICS_CA_TABLE *src) {
	int cdt_jj = 0;
	EPICS_CA_TABLE *dest = 0;

	if (!src) return;

	dest = &(caTable[src->chanIndex]);
	src->connected = 1;
	// just copy the table entry into place
	memcpy((void *)(dest), (void *)(src), sizeof(*src));
	
	// clearing the redir field
	dest->redirIndex = -1;
	cdt_jj = src->chanIndex;
	cdTable[cdt_jj].connected = 1;
	caTable[cdt_jj].connected = 1;
	// now update cdTable type information
	// iff the type is different, clear the data field
	if (cdTable[cdt_jj].datatype != src->datatype) {
		cdTable[cdt_jj].datatype = src->datatype;
		cdTable[cdt_jj].data.chval = 0.0;
	}
}

void syncCAConnections(long *disconnected_count)
{
	int ii = 0;
	long tmp = 0;
	int chanIndex = SDF_MAX_TSIZE;
	chtype subtype = DBR_TIME_DOUBLE;

	pthread_mutex_lock(&caTableMutex);
	// sync all non-enum channel connections that have taken place
	for (ii = 0; ii < chConnNum; ++ii) {
		copyConnectedCAEntry(&(caConnTable[ii]));
	}
	chConnNum = 0;

	// enums take special work, as we must receive an value to determine if we can use it as string or numeric value
	// try to short circuit the reading of the entire table, most cycles we have nothing to do.
	if (chEnumDetermined > 0) {
		for (ii = 0; ii < chNum; ++ii) {
			if (caConnEnumTable[ii].datatype != SDF_UNKNOWN) {
				// we have a concrete type here, migrate it over
				copyConnectedCAEntry(&(caConnEnumTable[ii]));
				caConnEnumTable[ii].datatype = SDF_UNKNOWN;

				chanIndex = caTable[ii].chanIndex;
				ca_clear_subscription(caEvid[chanIndex]);
				caEvid[chanIndex] = 0;
				subtype = (caTable[ii].datatype == SDF_NUM ? DBR_TIME_DOUBLE : DBR_TIME_STRING);
				ca_create_subscription(subtype, 0, caTable[ii].chanid, DBE_VALUE, subscriptionHandler, &(caTable[ii]), &(caEvid[chanIndex]));

				chanIndex = SDF_MAX_TSIZE;
				subtype = DBR_TIME_DOUBLE;
			}
		}
	}
	chEnumDetermined = 0;
	for (ii = 0 ; ii < chNum ; ++ii)  {
		cdTable[ii].connected = caTable[ii].connected;
	}
	// count the disconnected chans
	//if (disconnected_count) {
	//	for (ii = 0; ii < chNum; ++ii) {
	//		tmp += !caTable[ii].connected;
	//	}
	//	*disconnected_count = tmp;
	//}
	pthread_mutex_unlock(&caTableMutex);
}

/// Main loop of the CA Thread
void *caMainLoop(void *param)
{
	CA_STARTUP_INFO *info = (CA_STARTUP_INFO*)param;
	
	if (!param) {
		setCAThreadState(CA_STATE_EXIT);
		return NULL;
	}
	ca_context_create(ca_enable_preemptive_callback);
	setCAThreadState(CA_STATE_PARSING);
	parseChannelListReq(info->fname, info->prefix);
	registerFilters();
	printf("Done with parsing, CA thread continuing\n");
	setCAThreadState(CA_STATE_RUNNING);
}

/// Routine used to read in all channels to monitor from an INI file and create local connections
void initCAConnections(char *fname, char *prefix)
{
	int err = 0;
	int state = CA_STATE_OFF;
	CA_STARTUP_INFO param;

	param.fname = fname;
	param.prefix = prefix;


	setCAThreadState(CA_STATE_OFF);
	caMainLoop(&param);
}

/// Routine to setup mutexes and other resources used by the CA SDF system
void setupCASDF()
{
	int ii;

	// set some defaults
	for (ii = 0; ii < SDF_MAX_TSIZE; ++ii) {
		caTable[ii].redirIndex = -1;
		caTable[ii].datatype = SDF_NUM;
		caTable[ii].data.chval = 0.0;
		caTable[ii].connected = 0;
		caTable[ii].chanid = -1;
		caTable[ii].mod_time = (time_t)0;
		caTable[ii].chanIndex = ii;

		caConnTable[ii].redirIndex = -1;
		caConnTable[ii].datatype = SDF_NUM;
		caConnTable[ii].data.chval = 0.0;
		caConnTable[ii].connected = 0;
		caConnTable[ii].chanid = -1;
		caConnTable[ii].mod_time = (time_t)0;
		caConnTable[ii].chanIndex = 0;

		caConnEnumTable[ii].redirIndex = -1;
		caConnEnumTable[ii].datatype = SDF_UNKNOWN;
		caConnEnumTable[ii].data.chval = 0.0;
		caConnEnumTable[ii].connected = 0;
		caConnEnumTable[ii].chanid = -1;
		caConnEnumTable[ii].mod_time = (time_t)0;
		caConnEnumTable[ii].chanIndex = ii;

		bzero((void *)&(cdTable[ii]), sizeof(cdTable[ii]));
		bzero((void *)&(unMonChans[ii]), sizeof(unMonChans[ii]));
		bzero((void *)&(cdTableList[ii]), sizeof(cdTableList[ii]));
		bzero((void *)&(disconnectChans[ii]), sizeof(disconnectChans[ii]));

		caEvid[ii] = 0;
	}

	// set more defaults
	for (ii = 0; ii < SDF_MAX_CHANS; ++ii) {
		bzero((void *)&(cdTableP[ii]), sizeof(cdTableP[ii]));
	}
	for (ii = 0; ii < 1000; ++ii) {
		bzero((void *)&(filterTable[1000]), sizeof(filterTable[ii]));
	}
	for (ii = 0; ii < SDF_ERR_TSIZE; ++ii) {
		bzero((void *)&(setErrTable[ii]), sizeof(setErrTable[ii]));
		bzero((void *)&(unknownChans[ii]), sizeof(unknownChans[ii]));
		bzero((void *)&(uninitChans[ii]), sizeof(uninitChans[ii]));
		bzero((void *)&(readErrTable[ii]), sizeof(readErrTable[ii]));
	}
	droppedPVCount = 0;
	pthread_mutex_init(&caStateMutex, NULL);	// FIXME check for errors
	pthread_mutex_init(&caTableMutex, NULL);
	pthread_mutex_init(&caEvidMutex, NULL);
}

/// Routine to tear-down mutexes and other resources used by the CA SDF system
void cleanupCASDF()
{
	pthread_mutex_destroy(&caEvidMutex);
	pthread_mutex_destroy(&caTableMutex);
	pthread_mutex_destroy(&caStateMutex);
}
#else
int getDbValueDouble(ADDRESS *paddr,double *dest,time_t *tp) {
	struct buffer {
		DBRtime
		double dval;
	} buffer;
	long options = DBR_TIME;
	long nvals = 1;
	int result = 1;

	if (dest && paddr) {
		result = dbGetField(paddr, DBR_DOUBLE, &buffer, &options, &nvals, NULL);
		*dest = buffer.dval;
		if (tp) {
			*tp = buffer.time.secPastEpoch + POSIX_TIME_AT_EPICS_EPOCH;
		}
	}
	return result;
}
int getDbValueLong(ADDRESS *paddr,unsigned int *dest,time_t *tp) {
	struct buffer {
		DBRtime
		unsigned int dval;
	} buffer;
	long options = DBR_TIME;
	long nvals = 1;
	int result = 1;

	if (dest && paddr) {
		result = dbGetField(paddr, DBR_LONG, &buffer, &options, &nvals, NULL);
		*dest = buffer.dval;
		if (tp) {
			*tp = buffer.time.secPastEpoch + POSIX_TIME_AT_EPICS_EPOCH;
		}
	}
	return result;
}
int getDbValueString(ADDRESS *paddr,char *dest, int max_len, time_t *tp) {
	struct buffer {
		DBRtime
		char sval[128];
	} strbuffer;
	long options = DBR_TIME;
	long nvals = 1;
	int result = 1;
	if (dest && paddr && (max_len > 0)) {
		result = dbGetField(paddr,DBR_STRING,&strbuffer,&options,&nvals,NULL);
		strncpy(dest, strbuffer.sval, max_len);
		dest[max_len-1]='\0';
		if (tp) {
			*tp = strbuffer.time.secPastEpoch + POSIX_TIME_AT_EPICS_EPOCH;
		}
	}
	return result;
}

/// Routine used to extract all settings channels from EPICS database to create local settings table on startup.
void dbDumpRecords(DBBASE *pdbbase, const char *pref)
{
    DBENTRY  *pdbentry = 0;
    long  status = 0;
    char mask_pref[64];
    char mytype[4][64];
    int ii;
    int cnt = 0;
    size_t pref_len = strlen(pref);

    // By convention, the RCG produces ai and bi records for settings.
    sprintf(mytype[0],"%s","ai");
    sprintf(mytype[1],"%s","bi");
    sprintf(mytype[2],"%s","stringin");
    mytype[3][0]='\0';
    pdbentry = dbAllocEntry(pdbbase);

    sprintf(mask_pref, "%s_SDF_FM_MASK_", pref);
    pref_len = strlen(mask_pref);

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
		//fprintf(stderr, "processing %s\n", dbGetRecordName(pdbentry));
		sprintf(cdTable[chNum].chname,"%s",dbGetRecordName(pdbentry));
		// do not monitor the the SDF mask channels, they are part of this IOC
		if (strncmp(cdTable[chNum].chname, mask_pref, pref_len)==0) {
            --cnt;
            continue;
		}
		cdTable[chNum].filterswitch = 0;
		cdTable[chNum].filterNum = -1;
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
	registerFilters();
    printf("End of all Records\n");
    dbFreeEntry(pdbentry);
}
#endif

void listLocalRecords(DBBASE *pdbbase) {
	DBENTRY  *pdbentry = 0;
	long status = 0;
	int ii = 0;

	pdbentry = dbAllocEntry(pdbbase);
	status = dbFindRecordType(pdbentry, "ao");
	if (status) {
		printf("No record descriptions\n");
		return;
	}
	status = dbFirstRecord(pdbentry);
	if (status) {
		printf("No records found\n");
		return;
	}
	while (!status) {
		// printf("%d: %s\n",ii, dbGetRecordName(pdbentry));
		++ii;
		status = dbNextRecord(pdbentry);
	}
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
#ifdef CA_SDF
	dbAddr disconnectcountaddr;
	dbAddr droppedcountaddr;
#endif
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
	dbAddr pagelockaddr[3];	// why is this an array of 3.  It looks like we can make this a single value
#ifdef CA_SDF
	// CA_SDF does not do a partial load on startup.
	int sdfReq = SDF_READ_ONLY;
#else
	// Initialize request for file load on startup.
	int sdfReq = SDF_LOAD_PARTIAL;
#endif
	int status = 0;
	int request = 0;
	long ropts = 0;
	long nvals = 1;
	int rdstatus = 0;
	int burtstatus = 0;
	char loadedSdf[256];
	char sdffileloaded[256];
   	int sperror = 0;
	int noMon = 0;
	int noInit = 0;
	// FILE *csFile;
	int ii = 0;
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
	long daqFileCrc = 0;
	long coeffFileCrc = 0;
	long fotonFileCrc = 0;
	long prevFotonFileCrc = 0;
	long prevCoeffFileCrc = 0;
	long sdfFileCrc = 0;
    time_t      daqFileMt;
    time_t      coeffFileMt;
    time_t      fotonFileMt;
    time_t      sdfFileMt;
	char modfilemsg[] = "Modified File Detected ";
	struct stat st = {0};
	int reqValid = 0;
	int pageDisp = 0;
	int resetByte = 0;
	int resetBit = 0;
	int confirmVal = 0;
	int myexp = 0;
	int selectCounter[4] = {0,0,0,0};
	int selectAll = 0;
	int freezeTable = 0;
	int zero = 0;
	char backupName[64];
	int lastTable = 0;
	int cdSort = 0;
	int diffCnt = 0;
    	char errMsg[128];

    loadedSdf[0] = '\0';
    sdffileloaded[0] = '\0';
    tsrString[0] = '\0';
    saveTypeString[0] = '\0';
    saveOptsString[0] = '\0';
    backupName[0] = '\0';
    errMsg[0] = '\0';

    if(argc>=2) {
        iocsh(argv[1]);

	for (ii = 2; ii < argc; ++ii) {
		if (strcmp(argv[ii], "--no-sdf-restore") == 0) {
			sdfReq = SDF_READ_ONLY;
			printf("The SDF system will not force a restore of EPICS values at startup\n");
		} else if (strcmp(argv[ii], "--sdf-restore") == 0) {
			sdfReq = SDF_LOAD_PARTIAL;
			printf("The SDF system will force a restore of EPICS values at startup\n");
		}
	}

	// printf("Executing post script commands\n");
	// dbDumpRecords(*iocshPpdbbase);
	// Get environment variables from startup command to formulate EPICS record names.
	char *pref = getenv("PREFIX");
	char *sdfDir = getenv("SDF_DIR");
	char *sdfenv = getenv("SDF_FILE");
	char *modelname =  getenv("SDF_MODEL");
	char *targetdir =  getenv("TARGET_DIR");
	char *daqFile =  getenv("DAQ_FILE");
	char *coeffFile =  getenv("COEFF_FILE");
	char *fotonFile =  getenv("FOTON_FILE");
	char *fotonDiffFile = getenv("FOTON_DIFF_FILE");
	char *logdir = getenv("LOG_DIR");
	char myDiffCmd[256];
	SET_ERR_TABLE *currentTable = 0;
	int currentTableCnt = 0;

	if(stat(logdir, &st) == -1) mkdir(logdir,0777);
	// strcat(sdf,"_safe");
	char sdf[256];
	char sdfile[256];
	char sdalarmfile[256];
	char bufile[256];
	char saveasfilename[128];
	char wcstring[64];
	
    strncpy(sdf, sdfenv, sizeof(sdf));
    sdf[sizeof(sdf)-1] = '\0';
	
	printf("My prefix is %s\n",pref);
	sprintf(sdfile, "%s%s%s", sdfDir, sdf,".snap");					// Initialize with BURT_safe.snap
	sprintf(bufile, "%s%s", sdfDir, "fec.snap");					// Initialize table dump file
	sprintf(logfilename, "%s%s", logdir, "/ioc.log");					// Initialize table dump file
	printf("SDF FILE = %s\n",sdfile);
	printf("CURRENt FILE = %s\n",bufile);
	printf("LOG FILE = %s\n",logfilename);
sleep(5);
	int majorversion = RCG_VERSION_MAJOR;
	int subversion1 = RCG_VERSION_MINOR;
	int subversion2 = RCG_VERSION_SUB;
	int myreleased = RCG_VERSION_REL;
	double myversion;

	SETUP;
	// listLocalRecords(*iocshPpdbbase);
	myversion = majorversion + 0.1 * subversion1 + 0.01 * subversion2;
	if(!myreleased) myversion *= -1.0;
	char rcgversionname[256]; sprintf(rcgversionname, "%s_%s", pref, "RCG_VERSION");	// Set RCG Version EPICS
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

#ifdef CA_SDF
	char dsc[256]; sprintf(dsc, "%s_%s", pref, "SDF_DISCONNECTED_CNT");
	status = dbNameToAddr(dsc,&disconnectcountaddr);

	char dpdc[256]; sprintf(dpdc, "%s_%s", pref, "SDF_DROPPED_CNT");
	status = dbNameToAddr(dpdc,&droppedcountaddr);
#endif

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
	status = dbNameToAddr(pagelockname,&(pagelockaddr[0]));		// Get Address.
	status = dbPutField(&(pagelockaddr[0]),DBR_LONG,&freezeTable,1);		// Init to zero.

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

	char modcoefffilemsg[128]; sprintf(modcoefffilemsg, "%s_%s", pref, "MSG2");	// Record to write if Coeff file changed.
	status = dbNameToAddr(modcoefffilemsg,&coeffmsgaddr);

	char msgstrname[128]; sprintf(msgstrname, "%s_%s", pref, "SDF_MSG_STR");	// SDF Time of last file save.
	status = dbNameToAddr(msgstrname,&msgstraddr);
	status = dbPutField(&msgstraddr,DBR_STRING,"",1);

#ifndef USE_SYSTEM_TIME
	sprintf(timechannel,"%s_%s", pref, "TIME_STRING");
	// printf("timechannel = %s\n",timechannel);
#endif
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

#ifdef CA_SDF
	{
		char *buffer=0;
		char fname[]="/monitor.req";
		int len = strlen(sdfDir)+strlen(fname)+1;
		buffer = malloc(len);
		if (!buffer) {
			fprintf(stderr, "Unable to allocate memory to hold the path to monitor.req, aborting!");
			exit(1);
		}
		strcpy(buffer, sdfDir);
		strcat(buffer, fname);
		initCAConnections(buffer, pref);
		free(buffer);
	}
#else
	dbDumpRecords(*iocshPpdbbase, pref);
#endif
	setupFMArrays(pref,filterMasks,fmMaskChan,fmMaskChanCtrl);

	sprintf(errMsg,"Software Restart \nRCG VERSION: %.2f",myversion);
	logFileEntry(errMsg);

	

	// Initialize DAQ and COEFF file CRC checksums for later compares.
	daqFileCrc = checkFileCrc(daqFile);
	coeffFileCrc = checkFileCrc(coeffFile);
	fotonFileCrc = checkFileCrc(fotonFile);
	prevFotonFileCrc = fotonFileCrc;
	prevCoeffFileCrc = coeffFileCrc;
    // Initialize file modification times
    status = checkFileMod( daqFile, &daqFileMt, 1 );
    status = checkFileMod( coeffFile, &coeffFileMt, 1 );
    status = checkFileMod( fotonFile, &fotonFileMt, 1 );

	reportSetErrors(pref, 0,setErrTable,0,1);

	sleep(1);       // Need to wait before first restore to allow sequencers time to do their initialization.
	cdSort = spChecker(monFlag,cdTableList,wcVal,wcstring,1,&status);

	// Start Infinite Loop 		*******************************************************************************
	for(;;) {
		usleep(100000);					// Run loop at 10Hz.
#ifdef CA_SDF
		{
			// even with a pre-emptive callback enabled we need to do a ca_poll to get writes to work
			// see bug 965
			ca_poll();
			syncCAConnections(NULL);
			status = dbPutField(&disconnectcountaddr,DBR_LONG,&chDisconnected,1);
			status = dbPutField(&droppedcountaddr,DBR_LONG,&droppedPVCount,1); 
		}
#endif
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
				resyncFMArrays(filterMasks,fmMaskChan);
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
                    // Get the file mod time
                    status = checkFileMod( sdffileloaded, &sdfFileMt, 1 );
					// Calculate and report the number of settings in the BURT file.
					setChans = chNumP - alarmCnt;
					status = dbPutField(&filesetcntaddr,DBR_LONG,&setChans,1);
					// Report number of settings in the main table.
					setChans = chNum - fmNum;
					status = dbPutField(&fulldbcntaddr,DBR_LONG,&setChans,1);
					// Sort channels for data reporting via the MEDM table.
					getEpicsSettings(chNum,NULL);
					noMon = createSortTableEntries(chNum,0,"",&noInit,NULL);
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
				if(saveOpts == SAVE_OVERWRITE)  {
						sdfFileCrc = checkFileCrc(sdffileloaded);
                        status = checkFileMod( sdffileloaded, &sdfFileMt, 1 );
                }
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
					resyncFMArrays(filterMasks,fmMaskChan);
					confirmVal = 0;
				}
				pageDisp = reportSetErrors(pref, sperror,setErrTable,pageNum,1);
				currentTable = setErrTable;
				currentTableCnt = sperror;
				status = dbPutField(&sorttableentriesaddr,DBR_LONG,&sperror,1);
				status = dbGetField(&resetoneaddr,DBR_LONG,&resetNum,&ropts,&nvals,NULL);
				if(selectAll) {
					setAllTableSelections(sperror,setErrTable, selectCounter,selectAll);
				}
				if(resetNum) {
					decodeChangeSelect(resetNum, pageDisp, sperror, setErrTable,selectCounter, NULL);
				}
				if(confirmVal) {
					if(selectCounter[0] && (confirmVal & 2)) status = resetSelectedValues(sperror, setErrTable);
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
                        status = checkFileMod( sdffileloaded, &sdfFileMt, 1 );
						status = writeTable2File(sdfDir,bufile,SDF_WITH_INIT_FLAG,cdTable);
					}
					clearTableSelections(sperror,setErrTable, selectCounter);
					resyncFMArrays(filterMasks,fmMaskChan);
					confirmVal = 0;
					status = dbPutField(&confirmwordaddr,DBR_LONG,&confirmVal,1);
					noMon = createSortTableEntries(chNum,wcVal,wcstring,&noInit,NULL);
					status = dbPutField(&monchancntaddr,DBR_LONG,&chNotMon,1);
				}
				lastTable = SDF_TABLE_DIFFS;
				break;
			case SDF_TABLE_NOT_FOUND:
				// Need to clear selections when moving between tables.
				if(lastTable != SDF_TABLE_NOT_FOUND) {
					clearTableSelections(sperror,setErrTable, selectCounter);
					resyncFMArrays(filterMasks,fmMaskChan);
					confirmVal = 0;
				}
				pageDisp = reportSetErrors(pref, chNotFound,unknownChans,pageNum,1);
				currentTable = unknownChans;
				currentTableCnt = chNotFound;
				status = dbPutField(&sorttableentriesaddr,DBR_LONG,&chNotFound,1);
				/*if (resetNum > 200) {
					decodeChangeSelect(resetNum, pageDisp, chNotFound, unknownChans,selectCounter, NULL);
				}*/
				lastTable =  SDF_TABLE_NOT_FOUND;
				break;
			case SDF_TABLE_NOT_INIT:
				if(lastTable != SDF_TABLE_NOT_INIT) {
					clearTableSelections(sperror,setErrTable, selectCounter);
					resyncFMArrays(filterMasks,fmMaskChan);
					confirmVal = 0;
				}
				if (!freezeTable)
					getEpicsSettings(chNum,NULL);
				noMon = createSortTableEntries(chNum,wcVal,wcstring,&noInit,NULL);
				pageDisp = reportSetErrors(pref, noInit, uninitChans,pageNum,1);
				currentTable = uninitChans;
				currentTableCnt = noInit;
				status = dbGetField(&resetoneaddr,DBR_LONG,&resetNum,&ropts,&nvals,NULL);
				if(selectAll == 2 || selectAll == 3) {
					setAllTableSelections(noInit,uninitChans,selectCounter,selectAll);
					if (selectAll == 3)
						setAllTableSelections(noInit,uninitChans,selectCounter,2);
				}
				if(resetNum > 100) {
					decodeChangeSelect(resetNum, pageDisp, noInit, uninitChans,selectCounter, changeSelectCB_uninit);
				}
				if(confirmVal) {
					if(selectCounter[0] && (confirmVal & 2)) status = resetSelectedValues(noInit, uninitChans);
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
                        status = checkFileMod( sdffileloaded, &sdfFileMt, 1 );
						status = writeTable2File(sdfDir,bufile,SDF_WITH_INIT_FLAG,cdTable);
					}
					clearTableSelections(chNotInit,uninitChans, selectCounter);
					resyncFMArrays(filterMasks,fmMaskChan);
					confirmVal = 0;
					status = dbPutField(&confirmwordaddr,DBR_LONG,&confirmVal,1);
				}
				status = dbPutField(&sorttableentriesaddr,DBR_LONG,&noInit,1);
				status = dbPutField(&chnotinitaddr,DBR_LONG,&chNotInit,1);
				lastTable = SDF_TABLE_NOT_INIT;
				break;
			case SDF_TABLE_NOT_MONITORED:
				D("In not mon\n");
				if(lastTable != SDF_TABLE_NOT_MONITORED) {
					clearTableSelections(noMon,unMonChans, selectCounter);
					resyncFMArrays(filterMasks,fmMaskChan);
					confirmVal = 0;
					status = dbPutField(&confirmwordaddr,DBR_LONG,&confirmVal,1);
				}
				if (!freezeTable)
					getEpicsSettings(chNum,NULL); //timeTable);
				noMon = createSortTableEntries(chNum,wcVal,wcstring,&noInit,NULL);//timeTable);
				status = dbPutField(&monchancntaddr,DBR_LONG,&chNotMon,1);
				pageDisp = reportSetErrors(pref, noMon, unMonChans,pageNum,1);
				currentTable = unMonChans;
				currentTableCnt = noMon;
				status = dbGetField(&resetoneaddr,DBR_LONG,&resetNum,&ropts,&nvals,NULL);
				if(selectAll) {
					setAllTableSelections(noMon,unMonChans,selectCounter,selectAll);
				}
				if(resetNum) {
					decodeChangeSelect(resetNum, pageDisp, noMon, unMonChans,selectCounter, NULL);
				}
				if(confirmVal) {
					if(selectCounter[0] && (confirmVal & 2)) status = resetSelectedValues(noMon, unMonChans);
					if((selectCounter[1] || selectCounter[2]) && (confirmVal & 2)) {
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
                        status = checkFileMod( sdffileloaded, &sdfFileMt, 1 );
						status = writeTable2File(sdfDir,bufile,SDF_WITH_INIT_FLAG,cdTable);
					}
					// noMon = createSortTableEntries(chNum,wcVal,wcstring);
					clearTableSelections(noMon,unMonChans, selectCounter);
					resyncFMArrays(filterMasks,fmMaskChan);
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
					resyncFMArrays(filterMasks,fmMaskChan);
					confirmVal = 0;
				}
				if (!freezeTable)
					cdSort = spChecker(monFlag,cdTableList,wcVal,wcstring,1,&status);
				pageDisp = reportSetErrors(pref, cdSort, cdTableList,pageNum,1);
				currentTable = cdTableList;
				currentTableCnt = cdSort;
				status = dbGetField(&resetoneaddr,DBR_LONG,&resetNum,&ropts,&nvals,NULL);
				if(selectAll == 3) {
					setAllTableSelections(cdSort,cdTableList,selectCounter,selectAll);
				}
				if(resetNum) {
					decodeChangeSelect(resetNum, pageDisp, cdSort, cdTableList,selectCounter, NULL);
				}
				if(confirmVal) {
					if(selectCounter[0] && (confirmVal & 2)) status = resetSelectedValues(cdSort, cdTableList);
					if((selectCounter[1] || selectCounter[2]) && (confirmVal & 2)) {
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
                        status = checkFileMod( sdffileloaded, &sdfFileMt, 1 );
						status = writeTable2File(sdfDir,bufile,SDF_WITH_INIT_FLAG,cdTable);
					}
					clearTableSelections(cdSort,cdTableList, selectCounter);
					resyncFMArrays(filterMasks,fmMaskChan);
					confirmVal = 0;
					status = dbPutField(&confirmwordaddr,DBR_LONG,&confirmVal,1);
					noMon = createSortTableEntries(chNum,wcVal,wcstring,&noInit,NULL);
					// Calculate and report number of channels NOT being monitored.
					status = dbPutField(&monchancntaddr,DBR_LONG,&chNotMon,1);
				}
				status = dbPutField(&sorttableentriesaddr,DBR_LONG,&cdSort,1);
				lastTable = SDF_TABLE_FULL;
				break;
#ifdef CA_SDF
			case SDF_TABLE_DISCONNECTED:
				if(lastTable != SDF_TABLE_DISCONNECTED) {
					clearTableSelections(cdSort,cdTableList, selectCounter);
					resyncFMArrays(filterMasks,fmMaskChan);
					confirmVal = 0;
				}
				noMon = createSortTableEntries(chNum,wcVal,wcstring,&noInit,NULL);
				pageDisp =  reportSetErrors(pref, chDisconnected, disconnectChans, pageNum,0);
				currentTable = disconnectChans;
				currentTableCnt = chDisconnected;
				status = dbPutField(&sorttableentriesaddr,DBR_LONG,&chDisconnected, 1);
				if (resetNum > 200) {
					decodeChangeSelect(resetNum, pageDisp, chDisconnected, disconnectChans, selectCounter, NULL);
				}
				break;
#endif
			default:
				pageDisp = reportSetErrors(pref, sperror,setErrTable,pageNum,1);
				status = dbPutField(&sorttableentriesaddr,DBR_LONG,&sperror,1);
				currentTable = setErrTable;
				currentTableCnt = sperror;
				break;
		}
		if (selectAll) {
			selectAll = 0;
			status = dbPutField(&selectaddr[3],DBR_LONG,&selectAll,1);
		}
		if (resetNum) {
			resetNum = 0;
			status = dbPutField(&resetoneaddr,DBR_LONG,&resetNum,1);
		}
		if(pageDisp != pageNum) {
			pageNum = pageDisp;
		}
		freezeTable = 0;
		for(ii=0;ii<3;ii++) {
			freezeTable += selectCounter[ii];
			status = dbPutField(&selectaddr[ii],DBR_LONG,&selectCounter[ii],1);
		}
		status = dbPutField(&(pagelockaddr[0]),DBR_LONG,&freezeTable,1);

		processFMChanCommands(filterMasks, fmMaskChan,fmMaskChanCtrl,selectCounter, currentTableCnt, currentTable);

		// Check file CRCs every 5 seconds.
		// DAQ and COEFF file checking was moved from skeleton.st to here RCG V2.9.
		if(!fivesectimer) {
            // Check DAQ ini file modified
            int fm_flag = checkFileMod( daqFile, &daqFileMt, 0 );
            if ( fm_flag )
            {
			status = checkFileCrc(daqFile);
			if(status != daqFileCrc) {
				daqFileCrc = status;
				status = dbPutField(&daqmsgaddr,DBR_STRING,modfilemsg,1);
				logFileEntry("Detected Change to DAQ Config file.");
			}
            }
            // Check Foton file modified
            fm_flag = checkFileMod( fotonFile, &fotonFileMt, 0 );
            int fm_flag1 = checkFileMod( coeffFile, &coeffFileMt, 0 );
            if ( fm_flag  || fm_flag1)
            {
			coeffFileCrc = checkFileCrc(coeffFile);
			fotonFileCrc = checkFileCrc(fotonFile);
			if(fotonFileCrc != coeffFileCrc) {
				status = dbPutField(&coeffmsgaddr,DBR_STRING,modfilemsg,1);
			} else {
				status = dbPutField(&coeffmsgaddr,DBR_STRING,"",1);
			}
			if(fotonFileCrc != prevFotonFileCrc || prevCoeffFileCrc != coeffFileCrc) {
				sprintf(myDiffCmd,"%s %s %s %s %s","diff",fotonFile,coeffFile," > ",fotonDiffFile);
				status = system(myDiffCmd);
				prevFotonFileCrc = fotonFileCrc;
				prevCoeffFileCrc = coeffFileCrc;
			}
            }
            // Check SDF file modified
            fm_flag = checkFileMod( sdffileloaded, &sdfFileMt, 0 );
            if ( fm_flag )
            {
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
		iterate_output_counter();
	}
	sleep(0xfffffff);
    } else
    	iocsh(NULL);
    CLEANUP;
    return(0);
}
