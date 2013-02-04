%union{
  float  y_real;
  int	y_int;
  char	*y_str;
};

%{
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>
#include <sys/mman.h>
#include <pthread.h>
#include <iostream>
#include <fstream>
#include <iomanip>
using namespace std;
#include "config.h"
#include "FlexLexer.h"
#include "circ.hh"
#include "channel.hh"
#include "daqc.h"
#include "daqd.hh"
#include "MyLexer.hh"
#include "archive.hh"

#if EPICS_EDCU == 1
#include "registryFunction.h"
#include "epicsThread.h"
#include "dbStaticLib.h"
#include "subRecord.h"
#include "dbAccess.h"
#include "asDbLib.h"
#include "iocInit.h"
#include "iocsh.h"
//extern "C" int softIoc_registerRecordDeviceDriver(struct dbBase *);
#endif

void *interpreter (void *);
extern daqd_c daqd;
static int channel_compare (const void *, const void *);

extern "C" {
char *strdup (const char *);
}
namespace diag {
int packetBurst = 1;
}

int shutdown_server ();
void print_block_stats (ostream *yyout, circ_buffer_t *cb);
void print_command_help (ostream *yyout);

#define YYPARSE_PARAM lexer
#define yylex ((my_lexer *)lexer)->my_yylex
#define yyerror ((my_lexer *)lexer)->my_yyerror

#define AUTH_CHECK(lx) if (! ((lx)->auth_ok) && daqd.password [0]) { *(((my_lexer *)lexer)->get_yyout ()) << "password required" << endl; YYABORT; }

static int prompt_lineno;

 static void rfmUpdate() {
 }

%}

%token <y_void>  CYCLE_DELAY
%token <y_void>  SYMM_GPS_OFFSET
%token <y_void>  NO_COMPRESSION
%token <y_void>  ALLOW_TPMAN_CONNECT_FAILURE
%token <y_void>	 BUFFER_SIZE
%token <y_void>	 UPTIME
%token <y_void>  PERIODIC_MAIN_FILESYS_SCAN
%token <y_void>  PERIODIC_TREND_FILESYS_SCAN
%token <y_void>  AVOID_RECONNECT
%token <y_void>  TP_ALLOW
%token <y_void>  CONTROLLER_DCU
%token <y_void>  DO_DIRECTIO
%token <y_void>  DO_FSYNC
%token <y_void>  GPS_LEAPS
%token <y_void>	 PARALLEL_PRODUCERS
%token <y_void>  CIT_40M
%token <y_void>	 CYCLE_INPUT
%token <y_void>  MALLOC
%token <y_void>  BR_PACKET_BURST
%token <y_void>  DCU_STATUS_CHECK
%token <y_void>  CRC_T
%token <y_void>  SERVER
%token <y_void>  EPICS
%token <y_void>  DCU
%token <y_void>  MASTER_CONFIG
%token <y_void>  BROADCAST_CONFIG
%token <y_void>  SYSTEM
%token <y_void>  UPDATE
%token <y_void>  ADD
%token <y_void>  DELETE
%token <y_void>  ARCHIVE
%token <y_void>  ARCHIVES
%token <y_void>  CONDITIONAL_RFM_REFRESH
%token <y_void>  RFM_REFRESH
%token <y_void>  CKSUM_FNAME
%token <y_void>  RAW_MINUTE_TREND_SAVING_PERIOD
%token <y_void>  FULL_FRAMES_PER_FILE
%token <y_void>  FULL_FRAMES_BLOCKS_PER_FRAME

%token <y_void>  DETECTOR_NAME
%token <y_void>  DETECTOR_PREFIX
%token <y_void>  DETECTOR_LONGITUDE
%token <y_void>  DETECTOR_LATITUDE
%token <y_void>  DETECTOR_ELEVATION
%token <y_void>  DETECTOR_AZIMUTHS
%token <y_void>  DETECTOR_ALTITUDES
%token <y_void>  DETECTOR_MIDPOINTS

%token <y_void>  DETECTOR_NAME1
%token <y_void>  DETECTOR_PREFIX1
%token <y_void>  DETECTOR_LONGITUDE1
%token <y_void>  DETECTOR_LATITUDE1
%token <y_void>  DETECTOR_ELEVATION1
%token <y_void>  DETECTOR_AZIMUTHS1
%token <y_void>  DETECTOR_ALTITUDES1
%token <y_void>  DETECTOR_MIDPOINTS1

%token <y_void>  NDS_JOBS_DIR
%token <y_void>  UNITS
%token <y_void>  FLOCK
%token <y_void>  DO_SCAN_FRAME_READS
%token <y_void>  CLEAR
%token <y_void>  FAULT
%token <y_int>   BROADCAST
%token <y_int>   DOWNSAMPLE
%token <y_int>   AVERAGE
%token <y_void>  IPC_OFFSET
%token <y_void>  WORD_STRING
%token <y_void>  WORD_GPS
%token <y_void>  TPCONFIG
%token <y_void>  TRANSMISSION
%token <y_void>  UPLWP
%token <y_void>  FILESYS_CB_BLOCKS
%token <y_void>  PROFILER
%token <y_void>  PROFILING_PERIOD
%token <y_void>  PROFILING_CORE_DUMP
%token <y_void>  SYNC
%token <y_void>  PSRINFO
%token <y_void>  PROCESS_LOCK
%token <y_void>  PROCESS_UNLOCK

%token <y_void>  RAW_MINUTE_TREND_DIR
%token <y_void>  OLD_RAW_MINUTE_TREND_DIRS

%token <y_void>  MINUTE_TREND_FRAMES_PER_DIR
%token <y_void>  MINUTE_TREND_FRAMES
%token <y_void>  MINUTE_TREND_NUM_DIRS
%token <y_void>  MINUTE_TREND_FRAME_WIPER
%token <y_void>  MINUTE_TREND_FRAME_DIR
%token <y_void>  MINUTE_TREND

%token <y_void>  SWEPTSINE_FILENAME
%token <y_void>  OFFLINE
%token <y_void>  FRAMES_PER_DIR
%token <y_void>  TREND_FRAMES_PER_DIR
%token <y_void>  THREAD_STACK_SIZE
%token <y_void>  HELP
%token <y_void>  PASSWORD
%token <y_void>  FILESYS
%token <y_void>  LOG_T
%token <y_void>  DEBUG_T
%token <y_void>  INPUT
%token <y_void>  ECHO_ECHO
%token <y_void>  SLEEP
%token <y_void>  PRODUCER

%token <y_void>  FRAMES
%token <y_void>  FRAME_SAVER
%token <y_void>  NUM_DIRS
%token <y_void>  FRAME_WIPER
%token <y_void>  FRAME_DIR

%token <y_void>  TREND_FRAMES
%token <y_void>  TREND_FRAME_SAVER
%token <y_void>  MINUTE_TREND_FRAME_SAVER
%token <y_void>  RAW_MINUTE_TREND_SAVER
%token <y_void>  TREND_NUM_DIRS
%token <y_void>  TREND_FRAME_WIPER
%token <y_void>  TREND_FRAME_DIR
%token <y_void>  TREND_ASCII_OUTPUT

%token <y_void>  PSCAN
%token <y_void>  SCAN

%token <y_void>  SET

%token <y_void>  LISTENER
%token <y_void>  DAQD_VERSION
%token <y_void>  REVISION
%token <y_void>  ALL
%token <y_int>   CONFIGURE
%token <y_void>  BEGIN_BEGIN
%token <y_void>  END
%token <y_void>  ZERO_BAD_DATA

%token <y_int>  TREND
%token <y_int>  CHANNELS
%token <y_int>  CHANNEL_GROUPS
%token <y_int>  NET_WRITER
%token <y_int>  FRAME_WRITER
%token <y_int>  FAST_WRITER
%token <y_int>  NAME_WRITER
%token <y_int>  MAIN

%token <y_int>  SHUTDOWN
%token <y_int>  ABORT
%token <y_int>  QUIT

%token <y_str>	TEXT
%token <y_str>	TOKREF

%token <y_int>  BLOCKS
%token <y_int>	STATUS
%token <y_int>	START
%token <y_int>	ENABLE
%token <y_int>	DISABLE
%token <y_int>	KILL
%token <y_int>  INTNUM

%token <y_real> REALNUM

%token <y_str>  CAT
%token <y_str>  MKNUMBER
%token <y_str>  SUBSTR
%token <y_str>  DECODE

%type <y_str>  TextExpression
%type <y_str>  OptionalTextExpression
%type <y_void> ConfigureChannelsBody
%type <y_int>  OptionalIntnum
%type <y_str>  BroadcastOption
%type <y_int>  DecimateOption
%type <y_int>  ChannelNames
%type <y_int>  ChannelNames1
%type <y_int>  WriterType
%type <y_int>  allOrNothing
%pure_parser

%%

All: {if (((my_lexer *)lexer)->prompt) *(((my_lexer *)lexer)->get_yyout ()) << "daqd> " << flush;} Commands
   ;

Commands: /* empty file */
	|  CommandLines
	;

CommandLines: /* Nothing */
	| CommandLine ';' {if (((my_lexer *)lexer)->prompt && ((my_lexer *)lexer)->lineno() > ((my_lexer *)lexer)->prompt_lineno) {((my_lexer *)lexer)->prompt_lineno = ((my_lexer *)lexer)->lineno(); *(((my_lexer *)lexer)->get_yyout ()) << "daqd> " << flush;}} CommandLines
	;

CommandLine: /* Nothing */
	| SET CYCLE_DELAY '=' INTNUM {
                AUTH_CHECK(((my_lexer *)lexer));
		daqd.cycle_delay = $4;
	}
	| SET SYMM_GPS_OFFSET '=' INTNUM {
                AUTH_CHECK(((my_lexer *)lexer));
		daqd.symm_gps_offset = $4;
	}
	| SET NO_COMPRESSION INTNUM{
		AUTH_CHECK(((my_lexer *)lexer));
		daqd.no_compression = $3;
	}
	| SET ALLOW_TPMAN_CONNECT_FAILURE {
		AUTH_CHECK(((my_lexer *)lexer));
		daqd.allow_tpman_connect_failure = 1;
	}
	| START PERIODIC_MAIN_FILESYS_SCAN INTNUM {
		AUTH_CHECK(((my_lexer *)lexer));
		daqd.fsd.start_periodic_scan ($3);
	}
	| START PERIODIC_TREND_FILESYS_SCAN INTNUM {
		AUTH_CHECK(((my_lexer *)lexer));
		daqd.trender.fsd.start_periodic_scan ($3);
	}
	| SET AVOID_RECONNECT {
	  	daqd.avoid_reconnect = 1;
	}
	| SET TP_ALLOW '=' TextExpression {
	   long ip_addr = -1;
	   int failed = 0;
	   ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

	   if (strcmp($4, "255.255.255.255")) {
	     ip_addr = net_writer_c::get_inet_addr ($4);
	     if (ip_addr == -1) failed = 1;
	     else daqd.tp_allow = ip_addr;
	   } else daqd.tp_allow = -1; /* allow everything */
	   free($4);
	   if (failed) {
	     	if (((my_lexer *)lexer) -> strict)
		  *yyout << S_DAQD_ERROR << flush;
		else
		  *yyout << "Failed: Bad IP address" << endl;
	   } else {
	     	if (((my_lexer *)lexer) -> strict)
		  *yyout << S_DAQD_OK << flush;
		else
		  *yyout << "OK" << endl;
	   }
	}
 	| SET CONTROLLER_DCU '=' INTNUM {
		AUTH_CHECK(((my_lexer *)lexer));
		daqd.controller_dcu = $4;
	}
 	| SET DO_DIRECTIO '=' INTNUM {
		AUTH_CHECK(((my_lexer *)lexer));
		daqd.do_directio = $4;
	}
 	| SET DO_FSYNC '=' INTNUM {
		AUTH_CHECK(((my_lexer *)lexer));
		daqd.do_fsync = $4;
	}
	| SET GPS_LEAPS '=' INTNUM OptionalIntnum OptionalIntnum OptionalIntnum OptionalIntnum OptionalIntnum OptionalIntnum OptionalIntnum OptionalIntnum OptionalIntnum OptionalIntnum OptionalIntnum OptionalIntnum OptionalIntnum OptionalIntnum OptionalIntnum {
		// Assign all leapseconds; 0 is assigned if unspecified
		daqd.gps_leaps[0] = $4;
		daqd.gps_leaps[1] = $5;
		daqd.gps_leaps[2] = $6;
		daqd.gps_leaps[3] = $7;
		daqd.gps_leaps[4] = $8;
		daqd.gps_leaps[5] = $9;
		daqd.gps_leaps[6] = $10;
		daqd.gps_leaps[7] = $11;
		daqd.gps_leaps[8] = $12;
		daqd.gps_leaps[9] = $13;
		daqd.gps_leaps[10] = $14;
		daqd.gps_leaps[11] = $15;
		daqd.gps_leaps[12] = $16;
		daqd.gps_leaps[13] = $17;
		daqd.gps_leaps[14] = $18;
		daqd.gps_leaps[15] = $19;

		for (daqd.nleaps = 0;
		     daqd.nleaps < 16 && daqd.gps_leaps[daqd.nleaps];
		     daqd.nleaps++);
	}

	| SET PARALLEL_PRODUCERS '=' INTNUM {
		AUTH_CHECK(((my_lexer *)lexer));
		daqd.producer1.parallel = $4;
	}
	| SET CIT_40M '=' INTNUM {
		/* Enable or disable 40M related stuff in the frame builder */
		AUTH_CHECK(((my_lexer *)lexer));
		daqd.cit_40m = $4;
	}
	| SET CYCLE_INPUT '=' INTNUM {
		AUTH_CHECK(((my_lexer *)lexer));
		daqd.producer1.cycle_input = $4;
	}
	| MALLOC INTNUM {
	  char *foo;
	  ostream *yyout = ((my_lexer *)lexer)->get_yyout ();
	  if ((foo = (char *)malloc($2)) == 0) {
	    *yyout << "Couldn't malloc " << $2 << "bytes" << endl;
	  } else {
	    for (int i = 0; i < $2; i++) foo[i] = 0xff;
	    *yyout << "Malloced " << $2 << "bytes fine" << endl;
	  }
	}
	| SET BR_PACKET_BURST '=' INTNUM {
		AUTH_CHECK(((my_lexer *)lexer));
		diag::packetBurst = $4;
	}
	| SET DCU_STATUS_CHECK '=' INTNUM {
		AUTH_CHECK(((my_lexer *)lexer));
		daqd.dcu_status_check = $4;
	}
	| STATUS CRC_T {
		AUTH_CHECK(((my_lexer *)lexer));
	        ostream *yyout = ((my_lexer *)lexer)->get_yyout ();
		for (int i = 0; i < daqd.data_feeds; i++)
		  for (int j = 0; j < DCU_COUNT; j++) {
	      	    if (daqd.dcuSize[i][j] == 0) continue;
		    char *name = daqd.dcuName[j];
	      	    *yyout << name << "\tdcu=" << j << " crc_errs=" << daqd.dcuCrcErrCnt[i][j];
		    if (daqd.producer1.rcvr_stats[j].getN()) {
		    	*yyout << " ";
		    	daqd.producer1.rcvr_stats[j].print(*yyout);
		    }
		    *yyout << endl;
		  }
		daqd.producer1.print(*yyout);
	}
	| CLEAR CRC_T {
		AUTH_CHECK(((my_lexer *)lexer));
		for (int i = 0; i < daqd.data_feeds; i++)
		  for (int j = 0; j < DCU_COUNT; j++) {
		    daqd.dcuCrcErrCnt[i][j] = 0;
		    
		  }
		daqd.producer1.clearStats();
	}
	| SET CRC_T DEBUG_T '=' INTNUM {
		AUTH_CHECK(((my_lexer *)lexer));
	  	daqd.crc_debug = $5;
	}
	| START EPICS SERVER TextExpression  TextExpression  OptionalTextExpression {
#if EPICS_EDCU == 1
	        AUTH_CHECK(((my_lexer *)lexer));
	        ostream *yyout = ((my_lexer *)lexer)->get_yyout ();
	   	if (! daqd.start_epics_server (yyout, $4, $5, $6)) {
			system_log(1, "epics server started");
			free($4);
			free($5);
			if ($6) free ($6);
	   	} else exit (1);
#endif
#if 0
	        AUTH_CHECK(((my_lexer *)lexer));
	        ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

/* TODO: Generate daqd.db file from this code, based on frame builder number */
/* */
		if (dbLoadDatabase("daqd.dbd",0,0)) {
			*yyout << "Couldn't load daqd.dbd file" << endl;
		} else {
		   softIoc_registerRecordDeviceDriver(pdbbase);
		   dbLoadRecords("daqd.db",0);
		   iocInit();
		   //iocsh($4);
		}
		//free($4);
#endif
	}
	| STATUS EPICS DCU OptionalIntnum {
#if EPICS_EDCU == 1
	   AUTH_CHECK(((my_lexer *)lexer));
	   ostream *yyout = ((my_lexer *)lexer)->get_yyout ();
	   if (daqd.edcu1.running) {
	      *yyout << daqd.edcu1.num_chans << " channels" << endl;
	      *yyout << daqd.edcu1.con_chans << " connected" << endl; 
	      *yyout << daqd.edcu1.num_chans - daqd.edcu1.con_chans << " disconnected" << endl; 
	      *yyout << daqd.edcu1.con_events << " connection events processed" << endl; 
	      *yyout << daqd.edcu1.val_events << " value change events processed" << endl; 
	      if ($4) {
		int total_disco = 0;
		int total_bad_bad = 0;
	        for (int i = 0 ; i < daqd.edcu1.num_chans; i++) {
		  if (daqd.edcu1.channel_status[i]) {
		    total_disco++;
		    *yyout << daqd.channels[daqd.edcu1.fidx + i]. name << endl;
		    if (daqd.edcu1.channel_status[i] != 0xbad) {
			*yyout << "ERROR: status value is " << daqd.edcu1.channel_status[i] << endl;
			total_bad_bad++;
		    }
		  }
	        }
		*yyout << "Total " << total_disco << " disconnected." << endl;
		if (total_bad_bad) *yyout << "Total " << total_bad_bad << " have invalid (not 0xbad) status value" << endl;
	      }
	   }
#endif
	}
	| START EPICS DCU {
#if EPICS_EDCU == 1
	   AUTH_CHECK(((my_lexer *)lexer));
	   ostream *yyout = ((my_lexer *)lexer)->get_yyout ();
	   if (daqd.edcu1.running) {
	     *yyout << "EDCU already running" << endl;
	   } else {
	     int num_epics_channels = 0;
	     for (int i = 0; i < daqd.num_channels; i++) 
		if (IS_EPICS_DCU(daqd.channels [i].dcu_id)) {
		  if (!num_epics_channels) daqd.edcu1.fidx = i;
		  num_epics_channels++;
	        }
	     if (!num_epics_channels) {
		*yyout << "`start epics dcu': there are no epics channels configured" << endl;
		system_log(1, "edcu was not started; no channels configured");
	     } else {
	        daqd.edcu1.num_chans = num_epics_channels;
	   	if (! daqd.start_edcu (yyout)) {
		  system_log(1, "edcu started");
	   	} else exit (1);
	     }
	   }
#endif
	}
	| STATUS DCU {
	  ostream *yyout = ((my_lexer *) lexer)->get_yyout ();
	  for (int i = 0; i < daqd.data_feeds; i++) 
	  {
	    for (int j = 0; j < DCU_COUNT; j++) 
	      if (j < 4 || daqd.dcuSize[i][j]) {
		volatile struct rmIpcStr *ipc = daqd.dcuIpc[i][j];
		*yyout << "<ifo: " << i << ";dcu: " << j << "> " << daqd.dcuName[j]
		       << " size=" << daqd.dcuDAQsize[i][j]
		       << " status=0x" << hex << daqd.dcuStatus[i][j]
		       << " cycle=" << dec << daqd.dcuCycle[i][j]
		       << endl << flush;
	      }
	  }
	}
	| SET BROADCAST_CONFIG '=' TextExpression {
		AUTH_CHECK((my_lexer *) lexer);
		daqd.broadcast_config  = $4;
		free($4);
	}
	| SET MASTER_CONFIG '=' TextExpression {
		AUTH_CHECK((my_lexer *) lexer);
		daqd.master_config  = $4;
		free($4);
	}
	| UPDATE ALL ARCHIVES {
		int sr;
		AUTH_CHECK((my_lexer *) lexer);
		
		if ((sr = system("/usr/local/bin/update_dmt_archives.pl")) < 0)
			system_log(1, "Update all archives: system() returned %d\n", sr);
	}
	/*		    Directory		    ConfigFname    OldConfigFname */
	| CONFIGURE ARCHIVE TextExpression CHANNELS TextExpression OptionalTextExpression {
		unsigned int res;
		AUTH_CHECK((my_lexer *) lexer);
		ostream *yyout = ((my_lexer *) lexer)->get_yyout ();
		res = daqd.configure_archive_channels($3, $5, $6);
		if (((my_lexer *)lexer) -> strict) {
		  *yyout << setw(4) << setfill ('0') << hex << res << dec <<flush;
		} else {
		  if (res)
		     *yyout << "Failed; error=" << res << endl;
		  else
		     *yyout << "OK" << endl;
		}
		free($3); free($5); free($6);
	}
	| STATUS ARCHIVES {
		AUTH_CHECK((my_lexer *) lexer);
		ostream *yyout = ((my_lexer *) lexer)->get_yyout ();
		if (((my_lexer *)lexer) -> strict) {
#if 0
	:TODO: send the size of this archive in seconds and origin in GPS seconds

			// send one comm block
			*yyout << S_DAQD_OK << "00000000" << flush; // an ACK first, then dummy net-writer id
			unsigned long header [6];
			// block is empty, just the header
			header [0] = htonl (1);
			header [1] = htonl (4 * sizeof (unsigned long));
			// number of seconds of data
			header [2] = htonl (daqd.fsd.get_max()-daqd.fsd.get_min()); // period
			header [3] = htonl (daqd.fsd.get_min()); // time
			header [4] = htonl (0); // nanoresidual
			header [5] = htonl (0); // sequence number
			if (basic_io::writen (((my_lexer *)lexer) -> ofd,
			    (char *) header, sizeof (header)) != sizeof (header)) {
				YYABORT;
			}
#endif

		} else {
			for (s_link *clink = daqd.archive.first (); clink; clink = clink -> next ()) {
			    archive_c *a = (archive_c *) clink;
			    a -> lock();
			    *yyout << "Archive: " << a -> fsd.get_path() << endl;
			    *yyout << "Type: ";
			    switch (a -> data_type) {
				case archive_c::full: *yyout << "full"; break;
				case archive_c::secondtrend: *yyout << "secondtrend"; break;
				case archive_c::minutetrend: *yyout << "minutetrend"; break;
				default: *yyout << "unknown"; break;
			    }
			    *yyout << endl;
			    *yyout << "Signals: (" << a -> nchannels << ")" << endl;
			    for (unsigned int i = 0; i < a -> nchannels; i++) {
				*yyout << a -> channels [i].name << " ";
				switch (a -> channels [i].type) {
					case _16bit_integer: *yyout << "_16bit_integer"; break;
					case _32bit_integer: *yyout << "_32bit_integer"; break;
						case _32bit_float:   *yyout << "_32bit_float"; break;
					case _64bit_double:  *yyout << "_64bit_double"; break;
					default: *yyout << "unknown"; break;
			    	}
				*yyout << " old=" << a -> channels [i].old;
				*yyout << " rate=" << a -> channels [i].rate;
				*yyout << endl;
			    }
			    *yyout << endl;
			    a -> fsd.print_stats (yyout, 0);
			    *yyout << endl;
			    a -> unlock();
			}
		}
	}

	/*               Directory          GPS        Dt         DirNum                        */
	| UPDATE ARCHIVE TextExpression ',' INTNUM ',' INTNUM ',' INTNUM {	
		unsigned int res;
		AUTH_CHECK((my_lexer *) lexer);
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();
		res = daqd.update_archive($3, $5, $7, $9);
		if (((my_lexer *)lexer) -> strict) {
		  *yyout << setw(4) << setfill ('0') << hex << res << dec <<flush;
		} else {
		  if (res)
		     *yyout << "Failed; error=" << res << endl;
		  else
		     *yyout << "OK" << endl;
		}
		free($3);      	
	}
	/*             Directory	  Prefix	     Suffix 	   	NDirs		*/
	| SCAN ARCHIVE TextExpression ',' TextExpression ',' TextExpression ',' INTNUM {
		unsigned int res;
		AUTH_CHECK((my_lexer *) lexer);
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();
		res = daqd.scan_archive($3, $5, $7, $9);
		if (((my_lexer *)lexer) -> strict) {
		  *yyout << setw(4) << setfill ('0') << hex << res << dec <<flush;
		} else {
		  if (res)
		     *yyout << "Failed; error=" << res << endl;
		  else
		     *yyout << "OK" << endl;
		}
		free($3); free($5); free($7);
	}
	/*		 Directory		*/
	| DELETE ARCHIVE TextExpression  {
		unsigned int res;
		AUTH_CHECK((my_lexer *) lexer);
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();
		res = daqd.delete_archive($3);
		if (((my_lexer *)lexer) -> strict) {
		  *yyout << setw(4) << setfill ('0') << hex << res << dec <<flush;
		} else {
		  if (res)
		     *yyout << "Archive was not found; error=" << res << endl;
		  else
		     *yyout << "OK" << endl;
		}
		free ($3);
	}
	| CONDITIONAL_RFM_REFRESH {
		AUTH_CHECK((my_lexer *) lexer);
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();
		if (system("test `/usr/sbin/eeprom | grep power-cycles | head -1` = `head -1 /etc/power-cycles` > /dev/null 2>&1")) {
		  system_log(1, "RFM refresh is needed; power was cycled");
		  rfmUpdate();
		  if (system("/usr/sbin/eeprom | grep power-cycles > /etc/power-cycles")) {
		    system_log(1,"Could not write /etc/power-cycles or /usr/sbin/eeprom not found");
		    exit(1);
		  }
		} else {
		  system_log(1, "RFM refresh is not required");
		}
	}
       	| RFM_REFRESH {
		AUTH_CHECK((my_lexer *) lexer);
		rfmUpdate();
	}
	| SET ZERO_BAD_DATA '=' INTNUM {
		AUTH_CHECK((my_lexer *) lexer);
		daqd.zero_bad_data = $4;
	}
       	| SET CKSUM_FNAME '=' TextExpression {
		AUTH_CHECK((my_lexer *) lexer);
		daqd.cksum_file = $4;
		free ($4);
	}
	| SET RAW_MINUTE_TREND_SAVING_PERIOD '=' INTNUM {
		AUTH_CHECK((my_lexer *) lexer);
                daqd.trender.raw_minute_trend_saving_period = $4;
	}
	| SET FULL_FRAMES_PER_FILE '=' INTNUM {
                AUTH_CHECK((my_lexer *) lexer);
		daqd.frames_per_file = $4;
	}
	| SET FULL_FRAMES_BLOCKS_PER_FRAME '=' INTNUM {
                AUTH_CHECK((my_lexer *) lexer);
		daqd.blocks_per_frame = $4;
	}


       	| SET DETECTOR_NAME '=' TextExpression {
		AUTH_CHECK((my_lexer *) lexer);
		daqd.detector_name = $4;
		free ($4);
	}
       	| SET DETECTOR_PREFIX '=' TextExpression {
		AUTH_CHECK((my_lexer *) lexer);
		daqd.detector_prefix = $4;
		free ($4);
	}
       	| SET DETECTOR_LONGITUDE '=' REALNUM {
		AUTH_CHECK((my_lexer *) lexer);
		daqd.detector_longitude = $4;
	}
       	| SET DETECTOR_LATITUDE '=' REALNUM {
		AUTH_CHECK((my_lexer *) lexer);
		daqd.detector_latitude = $4;
	}
       	| SET DETECTOR_ELEVATION '=' REALNUM {
		AUTH_CHECK((my_lexer *) lexer);
		daqd.detector_elevation = $4;
	}
       	| SET DETECTOR_ALTITUDES '=' REALNUM ',' REALNUM {
		AUTH_CHECK((my_lexer *) lexer);
		daqd.detector_arm_x_altitude = $4;
		daqd.detector_arm_y_altitude = $6;
	}
       	| SET DETECTOR_MIDPOINTS '=' REALNUM ',' REALNUM {
		AUTH_CHECK((my_lexer *) lexer);
		daqd.detector_arm_x_midpoint = $4;
		daqd.detector_arm_y_midpoint = $6;
	}
       	| SET DETECTOR_AZIMUTHS '=' REALNUM ',' REALNUM {
		AUTH_CHECK((my_lexer *) lexer);
		daqd.detector_arm_x_azimuth = $4;
		daqd.detector_arm_y_azimuth = $6;
	}



       	| SET DETECTOR_NAME1 '=' TextExpression {
		AUTH_CHECK((my_lexer *) lexer);
		daqd.detector_name1 = $4;
		free ($4);
	}
       	| SET DETECTOR_PREFIX1 '=' TextExpression {
		AUTH_CHECK((my_lexer *) lexer);
		daqd.detector_prefix1 = $4;
		free ($4);
	}
       	| SET DETECTOR_LONGITUDE1 '=' REALNUM {
		AUTH_CHECK((my_lexer *) lexer);
		daqd.detector_longitude1 = $4;
	}
       	| SET DETECTOR_LATITUDE1 '=' REALNUM {
		AUTH_CHECK((my_lexer *) lexer);
		daqd.detector_latitude1 = $4;
	}
       	| SET DETECTOR_ELEVATION1 '=' REALNUM {
		AUTH_CHECK((my_lexer *) lexer);
		daqd.detector_elevation1 = $4;
	}
       	| SET DETECTOR_ALTITUDES1 '=' REALNUM ',' REALNUM {
		AUTH_CHECK((my_lexer *) lexer);
		daqd.detector_arm_x_altitude1 = $4;
		daqd.detector_arm_y_altitude1 = $6;
	}
       	| SET DETECTOR_MIDPOINTS1 '=' REALNUM ',' REALNUM {
		AUTH_CHECK((my_lexer *) lexer);
		daqd.detector_arm_x_midpoint1 = $4;
		daqd.detector_arm_y_midpoint1 = $6;
	}
       	| SET DETECTOR_AZIMUTHS1 '=' REALNUM ',' REALNUM {
		AUTH_CHECK((my_lexer *) lexer);
		daqd.detector_arm_x_azimuth1 = $4;
		daqd.detector_arm_y_azimuth1 = $6;
	}


       	| SET NDS_JOBS_DIR '=' TextExpression {
		AUTH_CHECK((my_lexer *) lexer);
		daqd.nds_jobs_dir = $4;
		free ($4);
	}
	| FLOCK TextExpression {
		int fd = open ($2, O_RDWR);
		if (fd == -1) {
			system_log(1, "FATAL: Can't open `%s' for locking", $2);
			exit (1);
		}
		int lock_res = lockf (fd, F_TLOCK, 0);
		if (lock_res == -1) {
			system_log(1, "FATAL: Can not put a lock on `%s'", $2);	
			system_log(1, "Is there another copy already started?");
			exit (1);
		}
		free ($2);
	}
	| SET DO_SCAN_FRAME_READS '=' INTNUM {
          AUTH_CHECK(((my_lexer *)lexer));
          daqd.do_scan_frame_reads = $4;
	}
	| CLEAR FAULT {
	  AUTH_CHECK(((my_lexer *)lexer));
	  daqd.clear_fault ();
	}
	| SET IPC_OFFSET '=' INTNUM {
	  AUTH_CHECK(((my_lexer *)lexer));
	}
	| WORD_GPS WORD_STRING  {
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();
		
		// which block is the current one
		int bnum = ((my_lexer *)lexer) -> cb -> next_block_in - 1;
		if (bnum < 0)
			bnum = ((my_lexer *)lexer) -> cb -> blocks - 1;

		unsigned long gps = ((my_lexer *)lexer) -> cb -> block [bnum].prop.gps;

		*yyout << setw(8) << setfill ('0') << hex << gps << flush;
	}

	| WORD_GPS {
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();
		
		unsigned long gps, gps_n;

		gps = gps_n = 0;
 		if (((my_lexer *)lexer) -> cb != 0) {

		  // which block is the current one
		  int bnum = ((my_lexer *)lexer) -> cb -> next_block_in - 1;
		  if (bnum < 0)
			bnum = ((my_lexer *)lexer) -> cb -> blocks - 1;
		  int bnum16th = ((my_lexer *)lexer) -> cb -> next_block_in_16th - 1;
		  if (bnum16th < 0)
			bnum16th = 15;

		  gps = ((my_lexer *)lexer) -> cb -> block [bnum].prop.gps;
		  gps_n = ((my_lexer *)lexer) -> cb -> block [bnum].prop16th [bnum16th].gps_n;
		}
		// If the producer is not running get the time from the system
		//if (gps == 0)  {
		  //gps = time(0) - 315964819 + 32;
		//}

		if (((my_lexer *)lexer) -> strict) {
		    // send one comm block
		    *yyout << S_DAQD_OK << "00000000" << flush; // an ACK first, then dummy net-writer id
		    unsigned int header [6];
		    // block is empty, just the header
		    header [0] = htonl (1);
		    header [1] = htonl (4 * sizeof (unsigned int));
		    // number of seconds of data
		    header [2] = htonl (1); // period
		    header [3] = htonl (gps); // time
		    header [4] = htonl (gps_n); // nanoresidual
		    header [5] = htonl (0); // sequence number
		    if (basic_io::writen (((my_lexer *)lexer) -> ofd,
			(char *) header, sizeof (header)) != sizeof (header)) {
			YYABORT;
		    }
		} else
		  *yyout << gps << endl;
	}
	| TPCONFIG TextExpression {
#ifdef GDS_TESTPOINTS
		AUTH_CHECK(((my_lexer *)lexer));
		daqd.gds.set_gds_server ($2);
		if (daqd.gds.gds_initialize ()) {
			sleep(2);
			exit(-1);
		}
#else
		;
#endif
	}
	| UPLWP {
		AUTH_CHECK(((my_lexer *)lexer));
	}
	| SET PROFILING_PERIOD '=' INTNUM {
		AUTH_CHECK(((my_lexer *)lexer));
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

		if ($4 > 0) {
			*yyout << "profilling period for all profilers set to "<< $4 << " seconds" << endl;
			daqd.profile.set_profiling_period ($4);
			daqd.trender.profile.set_profiling_period ($4);
		} else {
			*yyout << "profilling period must be greater than 1" << endl;
		}
	}
	| SET PROFILING_CORE_DUMP {
		daqd.profile.coredump_enable ();
		daqd.trender.profile.coredump_enable ();		
	}
	| START PROFILER {
		AUTH_CHECK(((my_lexer *)lexer));
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

		if (! daqd.b1)
			*yyout << "`start profiler': start main before starting profiler" << endl;
		else
			daqd.profile.start_profiler(daqd.b1);
	}
	| STATUS PROFILER {
		AUTH_CHECK(((my_lexer *)lexer));
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

		daqd.profile.print_status(yyout);
	}
	| START TREND PROFILER {
		AUTH_CHECK(((my_lexer *)lexer));
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

		if (! daqd.trender.tb)
			*yyout << "`start trend profiler': start trend before starting profiler" << endl;
		else
			daqd.trender.profile.start_profiler(daqd.trender.tb);
		if (daqd.trender.mtb)
			daqd.trender.profile_mt.start_profiler(daqd.trender.mtb);
	}
	| STATUS TREND PROFILER {
		AUTH_CHECK(((my_lexer *)lexer));
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

		daqd.trender.profile.print_status(yyout);
		daqd.trender.profile_mt.print_status(yyout);
		*yyout << "Minute trend saving period stats (" <<
			daqd.trender.num_channels << " channels/files )" << endl;
		daqd.trender.mt_stats.println(*yyout);
		*yyout << "Minute trend file saving stats (per file)" << endl;
		daqd.trender.mt_file_stats.println(*yyout);
	}

	| PSRINFO {
		AUTH_CHECK(((my_lexer *)lexer));
		}
	| PROCESS_LOCK allOrNothing {
		AUTH_CHECK(((my_lexer *)lexer));
                ostream *yyout = ((my_lexer *)lexer)->get_yyout ();
                seteuid (0); // Try to switch to superuser effective uid
                if (! geteuid ()) {
                    if (mlockall (MCL_CURRENT)) {
                      *yyout << "mlockall (MCL_CURRENT) failed: errno=" << errno << endl;
                      system_log(1, "mlockall (MCL_CURRENT)) failed: errno=%d", errno);
                    } else {
                      //*yyout << "current process pages are locked in memory" << endl;
                      system_log(1, "current process pages are locked in memory");
                    }
                  seteuid(getuid());
                } else {
                        *yyout << "process memory pages lock impossible: not a superuser" << endl;
                }
	}
	| PROCESS_UNLOCK {
		AUTH_CHECK(((my_lexer *)lexer));
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();
		seteuid (0); // Try to switch to superuser effective uid
		if (! geteuid ()) {
			if (munlockall ()) 
				*yyout << "munlockall () failed: errno=" << errno << endl;
			else
				*yyout << "process pages are unlocked from memory" << endl;
			seteuid(getuid());
		} else {
			*yyout << "process memory pages unlock impossible: not a superuser" << endl;
		}
	}
       	| SET RAW_MINUTE_TREND_DIR '=' TextExpression {
		AUTH_CHECK((my_lexer *) lexer);
		daqd.trender.raw_minute_fsd.set_filename_attrs ($4);
		free ($4);
	}
       	| SET OLD_RAW_MINUTE_TREND_DIRS '=' TextExpression {
		AUTH_CHECK((my_lexer *) lexer);
		daqd.old_raw_minute_trend_dirs = $4;
		free ($4);
	}
	| SET MINUTE_TREND_NUM_DIRS '=' INTNUM {
		AUTH_CHECK(((my_lexer *)lexer));
		daqd.trender.minute_fsd.set_num_dirs ($4);
		DEBUG1(cerr << "Number of directories in `daqd.trender.minute_fsd' set to " << $4 << endl);
	}
	| SET MINUTE_TREND_FRAME_DIR '=' TextExpression ',' TextExpression ',' TextExpression {
		AUTH_CHECK(((my_lexer *)lexer));
		daqd.trender.minute_fsd.set_filename_attrs ($8, $6, $4);
		free ($4); free ($6); free ($8);
	}
	| SET MINUTE_TREND_FRAMES_PER_DIR '=' INTNUM {
		AUTH_CHECK(((my_lexer *)lexer));
		daqd.trender.minute_fsd.set_files_per_dir ($4);
	}
	| SCAN MINUTE_TREND_FRAMES  {
		AUTH_CHECK(((my_lexer *)lexer));
		daqd.trender.minute_fsd.scan ();
	}
	| ENABLE MINUTE_TREND_FRAME_WIPER {
		AUTH_CHECK(((my_lexer *)lexer));
		daqd.trender.minute_fsd.enable_wiper ();
		system_log(1, "minute trend frame wiper enabled");
	}
	| DISABLE MINUTE_TREND_FRAME_WIPER {
		AUTH_CHECK(((my_lexer *)lexer));
		daqd.trender.minute_fsd.disable_wiper ();
		system_log(1, "minute trend frame wiper disabled");
	}
	| DISABLE OFFLINE {
		AUTH_CHECK(((my_lexer *)lexer));		
		daqd.offline_disabled = 1;
	}
	| ENABLE OFFLINE {
		AUTH_CHECK(((my_lexer *)lexer));
		daqd.offline_disabled = 0;
	}
	| HELP {
		print_command_help (((my_lexer *)lexer)->get_yyout ());
	}
	| PASSWORD TextExpression {
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

		if (! strncmp (daqd.password, $2, strlen (daqd.password))) {
			((my_lexer *)lexer) -> auth_ok = 1;
			if (((my_lexer *)lexer) -> strict)
			  *yyout << S_DAQD_OK << flush;
			else
			  *yyout << "OK" << endl;
		} else {
		  if (((my_lexer *)lexer) -> strict)
		    *yyout << S_DAQD_ERROR << flush;
		  else
		    *yyout << "invalid password" << endl;
		}
		free ($2);
	}
        | SET SWEPTSINE_FILENAME '=' TextExpression {
	  strncpy (daqd.sweptsine_filename, $4, filesys_c::filename_max);
	  daqd.sweptsine_filename [filesys_c::filename_max] = 0;
	  free ($4);
	}
	| SET THREAD_STACK_SIZE '=' INTNUM {
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

		if ($4 < 512) {
			*yyout << "less than 512K is not enough for the stack" << endl;
			system_log(1, "less than 512K is not enough for the stack");
		}
		daqd.thread_stack_size = $4 * 1024;
		//*yyout << "new threads will be created with the stack of size " << $4 <<  "K" << endl;
		system_log(1, "new threads will be created with the stack of size %dK", $4);
	}
	| SET PASSWORD '=' TextExpression {
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

		AUTH_CHECK(((my_lexer *)lexer));

		strncpy (daqd.password, $4, daqd_c::max_password_len);
		daqd.password [daqd_c::max_password_len] = 0;
		free ($4);
	}
	| SET LOG_T '=' INTNUM {
		AUTH_CHECK(((my_lexer *)lexer));
		_log_level = $4;
	}
	| SET DEBUG_T '=' INTNUM {
		AUTH_CHECK(((my_lexer *)lexer));
#ifndef NDEBUG
	  _debug = $4;
#else
	  ostream *yyout = ((my_lexer *)lexer)->get_yyout ();
	  system_log(1, "No debugging compiled in.");
#endif
	}
	| ECHO_ECHO TextExpression {
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

	//	*yyout << $2 << endl;
	  	system_log(1, "%s", (char *)$2);
		free ($2);
	}
	| SLEEP INTNUM {
		sleep ($2);
	}
	| SET NUM_DIRS '=' INTNUM {
		AUTH_CHECK(((my_lexer *)lexer));
		daqd.fsd.set_num_dirs ($4);
		DEBUG1(cerr << "Number of directories in `daqd.fsd' set to " << $4 << endl);
	}
	| SET FRAME_DIR '=' TextExpression ',' TextExpression ',' TextExpression {
		AUTH_CHECK(((my_lexer *)lexer));
		daqd.fsd.set_filename_attrs ($8, $6, $4);
		free ($4); free ($6); free ($8);
	}
	| SET FRAMES_PER_DIR '=' INTNUM {
		AUTH_CHECK(((my_lexer *)lexer));
		daqd.fsd.set_files_per_dir ($4);
	}
	| SET TREND_NUM_DIRS '=' INTNUM {
		AUTH_CHECK(((my_lexer *)lexer));
		daqd.trender.fsd.set_num_dirs ($4);
		DEBUG1(cerr << "Number of directories in `daqd.trender.fsd' set to " << $4 << endl);
	}
	| SET TREND_FRAME_DIR '=' TextExpression ',' TextExpression ',' TextExpression {
		AUTH_CHECK(((my_lexer *)lexer));
		daqd.trender.fsd.set_filename_attrs ($8, $6, $4);
		free ($4); free ($6); free ($8);
	}
	| SET TREND_FRAMES_PER_DIR '=' INTNUM {
		AUTH_CHECK(((my_lexer *)lexer));
		daqd.trender.fsd.set_files_per_dir ($4);
	}
	| SET TREND_ASCII_OUTPUT '=' INTNUM {
		AUTH_CHECK(((my_lexer *)lexer));
		daqd.trender.ascii_output = 1;
	}
	| SET FILESYS_CB_BLOCKS '=' INTNUM {
		AUTH_CHECK(((my_lexer *)lexer));
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();
		if ($4 < 1) {
			*yyout << "invalid block count" << endl;
		} else {
			int res;
			if (res = daqd.fsd.construct_cb ($4)) {
				*yyout << "failed; errno=" << res << endl;
			} else {
				DEBUG1(cerr << "Constructed a circular buffer with " << $4 << " blocks for main filesys map" << endl);
			}
		}
	}
	| PSCAN FRAMES {
		AUTH_CHECK(((my_lexer *)lexer));
		filesys_c fsmap (&daqd.fsd,1);
	
		fsmap.scan (); // run a scan on the local `fsmap'
//		daqd.fsd += fsmap; // merge local map into the main map
		daqd.fsd = fsmap;
	}
	| SCAN FRAMES  {
		AUTH_CHECK(((my_lexer *)lexer));
		daqd.fsd.scan ();
	}
	| SCAN TREND_FRAMES  {
		AUTH_CHECK(((my_lexer *)lexer));
		daqd.trender.fsd.scan ();
	}
	| PSCAN TREND_FRAMES {
		AUTH_CHECK(((my_lexer *)lexer));
		filesys_c fsmap (&daqd.trender.fsd,60);
	
		fsmap.scan (); // run a scan on the local `fsmap'
//		daqd.trender.fsd += fsmap; // merge local map into the main map
		daqd.trender.fsd = fsmap;
	}
	| DAQD_VERSION {
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

		if (((my_lexer *)lexer) -> strict) {
			*yyout << S_DAQD_OK << setw(4) << setfill ('0') << hex << DAQD_PROTOCOL_VERSION << dec << flush;
		} else {
			*yyout << DAQD_PROTOCOL_VERSION << "." << DAQD_PROTOCOL_REVISION << endl;
		}
	}
	| REVISION {
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

		if (((my_lexer *)lexer) -> strict) {
			*yyout << S_DAQD_OK << setw(4) << setfill ('0') << hex << DAQD_PROTOCOL_REVISION << dec << flush;
		} else {
			*yyout << DAQD_PROTOCOL_VERSION << "." << DAQD_PROTOCOL_REVISION << endl;
		}
	}
	| CONFIGURE CHANNEL_GROUPS {AUTH_CHECK(((my_lexer *)lexer));} ConfigureChannelGroupsBody {}
	| STATUS CHANNEL_GROUPS {
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

		if (((my_lexer *)lexer) -> strict) {
		  *yyout << S_DAQD_OK;
		  *yyout << setw(4) << setfill ('0') << hex << daqd.num_channel_groups;
		  for (int i = 0; i < daqd.num_channel_groups; i++) {
		    yyout -> setf (ios::left, ios::adjustfield);
		    *yyout << setw (channel_group_t::channel_group_name_max_len)
			   << setfill (' ') << daqd.channel_groups [i].name;
		    yyout -> setf (ios::right, ios::adjustfield);
		    *yyout << setw (4) << setfill ('0') << hex << daqd.channel_groups [i].num  << flush;
		  }
		} else {
		  *yyout << daqd.num_channel_groups << " channel groups" << endl;
		  *yyout << "|num\t|name" << endl;

		  for (int i = 0; i < daqd.num_channel_groups; i++) {
		    *yyout << daqd.channel_groups [i].num << "\t"
			   << daqd.channel_groups [i].name << endl;
		  }
		}
	}
	| CONFIGURE CHANNELS {AUTH_CHECK(((my_lexer *)lexer));} ConfigureChannelsBody {}
	| STATUS UNITS {
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

		if (((my_lexer *)lexer) -> strict) {
		  *yyout << S_DAQD_OK;
		} else {
		  *yyout << daqd.num_channels << " channels total" << endl;
#ifdef GDS_TESTPOINTS
		  *yyout << daqd.num_gds_channels << " gds channels" << endl;
		  *yyout << daqd.num_gds_channel_aliases << " gds channels aliases" << endl;
#endif
		  *yyout << daqd.trender.num_channels << " trend channels" << endl;
		  *yyout << "name\t\t\t\t\t\tunits\t\t\t\t\t\tgain\toffset\tslope" << endl;

		  for (int i = 0; i < daqd.num_channels; i++) {
		    yyout -> setf (ios::left, ios::adjustfield);
		    *yyout << setw (channel_t::channel_name_max_len) << setfill (' ') 
			   << daqd.channels [i].name << "\t";
		    *yyout << setw (channel_t::engr_unit_max_len) << setfill (' ') 
			   << daqd.channels [i].signal_units << "\t";

		    *yyout << daqd.channels [i].signal_gain << "\t"
			   << daqd.channels [i].signal_offset << "\t"
			   << daqd.channels [i].signal_slope << endl;

		      
		    yyout -> setf (ios::right, ios::adjustfield);
		  }
		}
	}
	| STATUS CHANNELS OptionalIntnum {((my_lexer *) lexer) -> trend_channels = 0; ((my_lexer *) lexer) -> num_channels = 0; ((my_lexer *)lexer) -> error_code = 0;((my_lexer *) lexer) -> n_archive_channel_names = 0;} ChannelNames1 {
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();
		unsigned int nchannels = daqd.num_channels;
		int schan = ($5 == 0); // specific channels only request

		int num_channels = daqd.num_channels;
		channel_t *c = daqd.channels;
		if (schan) {
		  num_channels = ((my_lexer *) lexer) -> num_channels;
		  c = ((my_lexer *) lexer) -> channels;
		}

 		// Check errors in channel names here
 		if (schan && ((my_lexer *)lexer) -> error_code) {
			if (((my_lexer *)lexer) -> strict)
				*yyout << setw(4) << setfill ('0') << hex << ((my_lexer *) lexer) -> error_code << dec << flush;
			goto status_channels_bailout;
		}

		if ($3 == 3) {
		  if (!schan) {
		    // Sending nicely formated list of channels
		    // Add archive channel numbers
		    for (s_link *clink = daqd.archive.first (); clink; clink = clink -> next ()) {
		      archive_c *a = (archive_c *) clink;
		      a -> lock();
		      nchannels += a -> nchannels;
		      a -> unlock();
		    }
		    // ACK field and the total number of channels
		    //
		    *yyout << S_DAQD_OK;
		    *yyout << dec << nchannels << endl;
		  }

		  // DAQ channels
		  //
		  for (int i = 0; i < num_channels; i++) {
		    if (schan) {
		      *yyout << ((my_lexer *) lexer) -> channels [i].name << endl;
		    } else {
		      *yyout << c [i].name << endl;
		    }
		    *yyout << c [i].sample_rate << endl;
		    *yyout << c [i].data_type << endl;
#ifdef GDS_TESTPOINTS
		    if (IS_GDS_ALIAS(c [i])) {
		    	*yyout << c [i].chNum << endl;
		    } else 
#endif
		    	*yyout << 0 << endl;
		    *yyout << c [i].group_num << endl;
		    *yyout << c [i].signal_units << endl;
		    *yyout << c [i].signal_gain << endl;
		    *yyout << c [i].signal_slope << endl;
		    *yyout << c [i].signal_offset << endl;
		  }

		  if (!schan) {
		    // Send channel names from the archives (DMT)
		    //
		    for (s_link *clink = daqd.archive.first (); clink; clink = clink -> next ()) {
		      archive_c *a = (archive_c *) clink;
		      a -> lock();
		    
		      int trend = a -> data_type == archive_c::secondtrend || a -> data_type == archive_c::minutetrend;
		      for (unsigned int i = 0; i < a -> nchannels; i++) {
		    	*yyout <<  a -> channels [i].name << endl;
		    	*yyout <<  a -> channels [i].rate << endl;
		    	*yyout <<  a -> channels [i].type << endl;
		    	*yyout <<  0 << endl;
			if (!strcasecmp(a->fsd.get_path(), "obsolete")
			    || a -> channels [i].old) {
			  *yyout << channel_t::obsolete_arc_groupn << endl;
			} else {
			  *yyout << channel_t::arc_groupn <<endl;
			}
		    	*yyout << " " << endl;
		    	*yyout << 1 << endl;
		    	*yyout << 1 << endl;
		    	*yyout << 0 << endl;
		      }
		      a -> unlock();
		    }
		  }
		} else if (((my_lexer *)lexer) -> strict) {

		  if (!schan) {
		    // Add archive channel numbers
		    for (s_link *clink = daqd.archive.first (); clink; clink = clink -> next ()) {
		      archive_c *a = (archive_c *) clink;
		      a -> lock();
		      for (unsigned int i = 0; i < a -> nchannels; i++) {
			// Do not count very long DMT channel names; they will not be sent
			// User must do 'status channels 3' to get long names
			if (strlen(a -> channels [i].name) <= channel_t::channel_name_max_len)
		    		nchannels++;
		      }
		      a -> unlock();
		    }

		    *yyout << S_DAQD_OK;
		    if ($3 > 1) {
		      *yyout << setw(8) << setfill ('0') << hex << nchannels;
		    } else {
		      *yyout << setw(4) << setfill ('0') << hex << nchannels;
		    }
		    if (!$3) {
		      *yyout << setw(4) << setfill ('0') << hex << 1000000/daqd.writer_sleep_usec; // Clock rate (Hz)
 		    }
		  } else {
		    *yyout << S_DAQD_OK;
		    if ($3 > 1) {
		      *yyout << setw(8) << setfill ('0') << hex << num_channels;
		    } else {
		      *yyout << setw(4) << setfill ('0') << hex << num_channels;
		    }
		    if (!$3) {
		      *yyout << setw(4) << setfill ('0') << hex << 1000000/daqd.writer_sleep_usec; // Clock rate (Hz)
 		    }
		  }
		  for (int i = 0; i < num_channels; i++) {
		    yyout -> setf (ios::left, ios::adjustfield);
		    if (schan) {
		      *yyout << setw (channel_t::channel_name_max_len) << setfill (' ') 
			   << ((my_lexer *) lexer) -> channels [i].name << "\t";
		    } else {
		      *yyout << setw (channel_t::channel_name_max_len) << setfill (' ') << daqd.channels [i].name;
		    }
		    yyout -> setf (ios::right, ios::adjustfield);
		    if ($3) {
		      *yyout << setw (8) << setfill ('0') << hex << c [i].sample_rate;
		    } else {
		      /* Limit older clients to 16K rate */
		      *yyout << setw (4) << setfill ('0') << hex << ((c [i].sample_rate > (16*1024))? (16*1024): c[i].sample_rate);
		    }
		    if ($3 > 1) {
#ifdef GDS_TESTPOINTS
		      if (IS_GDS_ALIAS(c[i])) {
		        *yyout << setw (8) << setfill ('0') << hex << c[i].chNum;
		      } else
#endif
		      {
		        *yyout << setw (8) << setfill ('0') << hex << 0;
		      }
	 	    } else {
		      *yyout << setw (4) << setfill ('0') << hex << c[i].chNum;
		    }
		    if (IS_GDS_ALIAS(c[i])) {
		    	*yyout << setw (4) << setfill ('0') << hex << c[i].tp_node;
		    } else {
		    	*yyout << setw (4) << setfill ('0') << hex << c[i].group_num;
		    }
		    if ($3 < 2) {
		      *yyout << setw (4) << setfill ('0') << hex << (c[i].bps > 0xffff? 0:  c[i].bps);
		    }
		    *yyout << setw (4) << setfill ('0') << hex << c[i].data_type;
		    // send conversion data
		    *yyout << setw (8) << setfill ('0') << hex
			   << *((unsigned int*)&c[i].signal_gain);
		    *yyout << setw (8) << setfill ('0') << hex
			   << *((unsigned int*)&c[i].signal_slope);
		    *yyout << setw (8) << setfill ('0') << hex
			   << *((unsigned int*)&c[i].signal_offset);
		    yyout -> setf (ios::left, ios::adjustfield);
		    *yyout << setw (channel_t::engr_unit_max_len) << setfill (' ')
			   << c[i].signal_units;
		    yyout -> setf (ios::right, ios::adjustfield);
		  }

		  if (!schan) {
		    // Send channel names from the archives
		    for (s_link *clink = daqd.archive.first (); clink; clink = clink -> next ()) {
		      archive_c *a = (archive_c *) clink;
		      a -> lock();
		    
		      int trend = a -> data_type == archive_c::secondtrend || a -> data_type == archive_c::minutetrend;
		      for (unsigned int i = 0; i < a -> nchannels; i++) {
		    	yyout -> setf (ios::left, ios::adjustfield);
			// Skip very long DMT channel names
			if (strlen(a -> channels [i].name) > channel_t::channel_name_max_len)
			  continue;
		    	*yyout << setw (channel_t::channel_name_max_len) << setfill (' ') << a -> channels [i].name;
		    	yyout -> setf (ios::right, ios::adjustfield);
		        if ($3) {
			  *yyout << setw (8) << setfill ('0') << hex << a -> channels [i].rate;
			} else {
			  *yyout << setw (4) << setfill ('0') << hex << a -> channels [i].rate;
			}
		        if ($3 > 1) {
		          *yyout << setw (8) << setfill ('0') << hex << 0;
			} else {
		    	  *yyout << setw (4) << setfill ('0') << hex << trend;
			}
			if (!strcasecmp(a->fsd.get_path(), "obsolete")
			    || a -> channels [i].old) {
			  *yyout << setw (4) << setfill ('0') << hex << channel_t::obsolete_arc_groupn; // group number
			} else {
			  *yyout << setw (4) << setfill ('0') << hex << channel_t::arc_groupn; // group number
			}
			if ($3 < 2) {
		    	  *yyout << setw (4) << setfill ('0') << hex << daqd_c::data_type_size (a -> channels [i].type);
			}
		    	*yyout << setw (4) << setfill ('0') << hex << a -> channels [i].type;

		    	// send conversion data -- there is no conversion data available
		    	*yyout << setw (8) << setfill ('0') << hex << 0;
		    	*yyout << setw (8) << setfill ('0') << hex << 0;
		    	*yyout << setw (8) << setfill ('0') << hex << 0;
		    	yyout -> setf (ios::left, ios::adjustfield);
		    	*yyout << setw (channel_t::engr_unit_max_len) << setfill (' ') << "";
		    	yyout -> setf (ios::right, ios::adjustfield);
		      }

		      a -> unlock();
		    }
		  }

		  *yyout << flush;
		} else {
		  if (!schan) {
		    *yyout << daqd.num_channels << " channels total" << endl;
#ifdef GDS_TESTPOINTS
		    *yyout << daqd.num_gds_channels << " gds channels" << endl;
		    *yyout << daqd.num_gds_channel_aliases << " gds channels aliases" << endl;
#endif

		    *yyout << daqd.trender.num_channels << " trend channels" << endl;
		    *yyout << 1000000/daqd.writer_sleep_usec << "Hz clock" << endl;
		  }
		  *yyout << "chnum\tslow\t|name\t\t\t\t|rate\t|trend\t|group\t|bps\t|bytes\t|offset\t|type\t|active" << endl;

		  for (int i = 0; i < num_channels; i++) {
		    *yyout
		           << c [i].chNum << "\t"
#ifndef NO_SLOW_CHANNELS
		           << c [i].slow << "\t";
#else
			   << 0 << "\t";
#endif

		    yyout -> setf (ios::left, ios::adjustfield);
		if (schan) {
		    *yyout << setw (channel_t::channel_name_max_len) << setfill (' ') 
			   << ((my_lexer *) lexer) -> channels [i].name << "\t";
		} else {
		    *yyout << setw (channel_t::channel_name_max_len) << setfill (' ') 
			   << c [i].name << "\t";
		}

		    *yyout 

			   << c [i].sample_rate << "\t"
			   << c [i].trend << "\t"
			   << c [i].group_num << "\t"
			   << c [i].bps << "\t"
			   << c [i].bytes << "\t"
			   << c [i].offset << "\t"
			   << c [i].data_type << "\t"
			   << c [i].active << endl;
		    yyout -> setf (ios::right, ios::adjustfield);
		  }
		}
status_channels_bailout:
	}
	| STATUS INPUT TREND CHANNELS {
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

		for (int i = 0; i < daqd.trender.num_channels; i++) {
			*yyout 
#ifndef NO_SLOW_CHANNELS
			  << daqd.trender.channels [i].slow << "\t"
#else
			  << 0 << "\t"
#endif
			  << daqd.trender.channels [i].name << "\t"
			  << daqd.trender.channels [i].sample_rate << "\t"
			  << daqd.trender.channels [i].trend << "\t"
			  << daqd.trender.channels [i].bps << "\t"
			  << daqd.trender.channels [i].bytes << "\t"
			  << daqd.trender.channels [i].offset << "\t"
			  << daqd.channels [i].data_type << endl;
		}
	}
	| STATUS TREND CHANNELS {
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

		*yyout << daqd.num_channels << " daqd channels" << endl;
		*yyout << daqd.trender.num_channels << " input trend channels" << endl;
		*yyout << "|p\t|id\t|slow\t|name\t\t\t\t|rate\t|trend\t|bps\t|bytes\t|offset" << endl;

#if 0
		int i;
		for (i = 0; i < daqd.trender.num_channels; i++) {
			*yyout
#ifndef NO_SLOW_CHANNELS
			  << daqd.trender.channels [i].slow << "\t";
#else
			  << 0 << "\t";
#endif
		        yyout -> setf (ios::left, ios::adjustfield);
		        *yyout << setw (channel_t::channel_name_max_len) << setfill (' ') 
			       << daqd.trender.channels [i].name << "\t";

			*yyout << daqd.trender.channels [i].sample_rate << "\t"
			  << daqd.trender.channels [i].trend << "\t"
			  << daqd.trender.channels [i].bps << "\t"
			  << daqd.trender.channels [i].bytes << "\t"
			  << daqd.trender.channels [i].offset << endl;
		        yyout -> setf (ios::right, ios::adjustfield);
		}
#endif

		*yyout << daqd.trender.num_trend_channels << " output trend channels" << endl;
		*yyout << "|p\t|id\t|slow\t|name\t\t\t\t|rate\t|trend\t|bps\t|bytes\t|offset" << endl;

#if 0
		for (i = 0; i < daqd.trender.num_trend_channels; i++) {
			*yyout
#ifndef NO_SLOW_CHANNELS
			  << daqd.trender.trend_channels [i].slow << "\t";
#else
			  << 0 << "\t";
#endif
		        yyout -> setf (ios::left, ios::adjustfield);
		        *yyout << setw (channel_t::channel_name_max_len) << setfill (' ') 
			       << daqd.trender.trend_channels [i].name << "\t";
			*yyout << daqd.trender.trend_channels [i].sample_rate << "\t"
			  << daqd.trender.trend_channels [i].trend << "\t"
			  << daqd.trender.trend_channels [i].bps << "\t"
			  << daqd.trender.trend_channels [i].bytes << "\t"
			  << daqd.trender.trend_channels [i].offset << endl;
		        yyout -> setf (ios::right, ios::adjustfield);
		}
#endif
	}
	| QUIT {
	  system_log(5, "->%d: quit", ((my_lexer *)lexer)->ifd); // `quit' is not logged from ((my_lexer *)lexer)->LexerInput()
	  YYABORT;
	}
	| ABORT {
		AUTH_CHECK(((my_lexer *)lexer));
		abort();
	}
	| SHUTDOWN {
		AUTH_CHECK(((my_lexer *)lexer));
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();
		if (((my_lexer *)lexer) -> strict)
		  *yyout << S_DAQD_OK << flush;
		else
		  *yyout << "OK" << endl;
		system_log(1, "shutdown");
		shutdown_server ();
		YYABORT;
	}
	| BUFFER_SIZE {
		AUTH_CHECK(((my_lexer *)lexer));
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

		*yyout << ((my_lexer *)lexer) -> cb -> blocks << endl;
		((my_lexer *)lexer)->outsync ();
	}
	| UPTIME {
		AUTH_CHECK(((my_lexer *)lexer));
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

		*yyout << ((my_lexer *)lexer) -> cb -> puts << endl;
		((my_lexer *)lexer)->outsync ();
	}
	| STATUS MAIN {
		AUTH_CHECK(((my_lexer *)lexer));
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

		yyout -> setf (ios::right, ios::adjustfield);

		if (! ((my_lexer *)lexer) -> strict) {
		  if (! ((my_lexer *)lexer)->cb) {
		    *yyout << "no main buffer" << endl;
		  } else {
		    *yyout << "min_time=" << daqd.fsd.get_min () << endl;
		    *yyout << "max_time=" << daqd.fsd.get_max () << endl;
		    print_block_stats (((my_lexer *)lexer)->get_yyout (), ((my_lexer *)lexer) -> cb);
		    //		    *yyout << flush;
		  }
		}
		else {
		  time_t dummy, mummy;
		  char tmpf [filesys_c::filename_max + 10];

		  *yyout << S_DAQD_OK;
		  yyout -> setf (ios::left, ios::adjustfield);

		  // Main framer
		  if (daqd.fsd.is_empty())
		    *yyout << setw (80) << setfill (' ')
			   << "Main frame saver is not running."
			   << flush;
		  else {
		    // FIXME: get_max() - 1
		    //                 ^^^^^^^
		    *yyout << setw (80) << setfill (' ')
			   << "Last Frame Filename:" << flush;
		    *yyout << setw (80) << setfill (' ')
			   << (daqd.fsd.filename (daqd.fsd.get_max () - 1, tmpf, &dummy, &mummy) < 0
			       ? "-- couldn't determine a filename --"
			       : tmpf)
			   << flush;

		    *yyout << setw (80) << setfill (' ')
			   << "Total Frames Saved:" << flush;
		    *yyout << setw (80) << setfill (' ')
			   << daqd.fsd.get_frames_saved () << flush;
		    
		    *yyout << setw (80) << setfill (' ')
			   << "Frames Lost:" << flush;
		    *yyout << setw (80) << setfill (' ')
			   << daqd.fsd.get_frames_lost () << flush;

		  }
		  
		  *yyout << setw (80) << setfill (' ') << " " << flush;

		  // Trend framer
		  if (daqd.trender.fsd.is_empty ())
		    *yyout << setw (80) << setfill (' ')
			   << "Trend frame saver is not running."
			   << flush;
		  else {
		    *yyout << setw (80) << setfill (' ')
			   << "Last Trend Frame Filename:" << flush;
		    *yyout << setw (80) << setfill (' ')
			   << (daqd.trender.fsd.filename (daqd.trender.fsd.get_max () - 1, tmpf, &dummy, &mummy) < 0
			       ? "-- couldn't determine a filename --"
			       : tmpf)
			   << flush;

		    *yyout << setw (80) << setfill (' ')
			   << "Total Trend Frames Saved:" << flush;
		    *yyout << setw (80) << setfill (' ')
			   << daqd.trender.fsd.get_frames_saved () << flush;
		    
		    *yyout << setw (80) << setfill (' ')
			   << "Trend Frames Lost:" << flush;
		    *yyout << setw (80) << setfill (' ')
			   << daqd.trender.fsd.get_frames_lost () << flush;
		  }

		  *yyout << setw (80) << setfill (' ') << " " << flush;

		  // Trend framer
		  if (daqd.trender.minute_fsd.is_empty ())
		    *yyout << setw (80) << setfill (' ')
			   << "Minute trend frame saver is not running."
			   << flush;
		  else {
		    *yyout << setw (80) << setfill (' ')
			   << "Last Minute Trend Frame Filename:" << flush;
		    *yyout << setw (80) << setfill (' ')
			   << (daqd.trender.minute_fsd.filename (daqd.trender.minute_fsd.get_max () - 1, tmpf, &dummy, &mummy) < 0
			       ? "-- couldn't determine a filename --"
			       : tmpf)
			   << flush;
		    *yyout << setw (80) << setfill (' ')
			   << "Total Minute Trend Frames Saved:" << flush;
		    *yyout << setw (80) << setfill (' ')
			   << daqd.trender.minute_fsd.get_frames_saved () << flush;
		    
		    *yyout << setw (80) << setfill (' ')
			   << "Minute Trend Frames Lost:" << flush;
		    *yyout << setw (80) << setfill (' ')
			   << daqd.trender.minute_fsd.get_frames_lost () << flush;
		  }

		  *yyout << setw (80) << setfill (' ')
			 << "--EOF--" << flush;
		  yyout -> setf (ios::right, ios::adjustfield);

		}
		//yyout->rdbuf()->pubsync();
		((my_lexer *)lexer)->outsync ();
	}
	| STATUS TREND { 
		AUTH_CHECK(((my_lexer *)lexer));
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

		if (! daqd.trender.tb) {
			*yyout << "no trend buffer" << endl;
		} else {
			*yyout << "min_time=" << daqd.trender.fsd.get_min () << endl;
			*yyout << "max_time=" << daqd.trender.fsd.get_max () << endl;
			*yyout << "minute trend min_time=" << daqd.trender.minute_fsd.get_min () << endl;
			*yyout << "minute trend max_time=" << daqd.trender.minute_fsd.get_max () << endl;

			print_block_stats (((my_lexer *)lexer)->get_yyout (), daqd.trender.tb -> buffer_ptr ());
			print_block_stats (((my_lexer *)lexer)->get_yyout (), daqd.trender.mtb -> buffer_ptr ());
		}
	}
	| STATUS NET_WRITER { 
		AUTH_CHECK(((my_lexer *)lexer));
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

		// This loop is not race free
		// `status net-writer' command should be used only for debugging
		for (s_link *p = daqd.net_writers.first(); p; p = p -> next ()) {
			net_writer_c *w = (net_writer_c *)p;

//			pthread_mutex_lock (&w -> lock);
			*yyout << "Net-writer " << w << " " << dec << (unsigned long) w << endl;
			if (w -> writer_type == net_writer_c::slow_writer || w -> writer_type == net_writer_c::fast_writer) {
			  print_block_stats (yyout, w -> buffptr -> buffer_ptr ()); 
			}
			*yyout << "---" << endl;
//			pthread_mutex_unlock (&w -> lock);
		}
	}
	| STATUS MAIN FILESYS OptionalIntnum {
		AUTH_CHECK(((my_lexer *)lexer));
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();
		if (((my_lexer *)lexer) -> strict) {
			// send one comm block
			*yyout << S_DAQD_OK << "00000000" << flush; // an ACK first, then dummy net-writer id
			unsigned int header [6];
			// block is empty, just the header
			header [0] = htonl (1);
			header [1] = htonl (4 * sizeof (unsigned int));
			// number of seconds of data
			header [2] = htonl (daqd.fsd.get_max()-daqd.fsd.get_min()); // period
			header [3] = htonl (daqd.fsd.get_min()); // time
			header [4] = htonl (0); // nanoresidual
			header [5] = htonl (0); // sequence number
			if (basic_io::writen (((my_lexer *)lexer) -> ofd,
			    (char *) header, sizeof (header)) != sizeof (header)) {
				YYABORT;
			}
		} else {
			daqd.fsd.print_stats (yyout, $4);
		}
	}
	| STATUS TREND FILESYS OptionalIntnum {
		AUTH_CHECK(((my_lexer *)lexer));
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();
		if (((my_lexer *)lexer) -> strict) {
			// send one comm block
			*yyout << S_DAQD_OK << "00000000" << flush; // an ACK first, then dummy net-writer id
			unsigned int header [6];
			// block is empty, just the header
			header [0] = htonl (1);
			header [1] = htonl (4 * sizeof (unsigned int));
			// number of seconds of data
			header [2] = htonl (daqd.trender.fsd.get_max()-daqd.trender.fsd.get_min()); // period
			header [3] = htonl (daqd.trender.fsd.get_min()); // time
			header [4] = htonl (0); // nanoresidual
			header [5] = htonl (0); // sequence number
			if (basic_io::writen (((my_lexer *)lexer) -> ofd,
			    (char *) header, sizeof (header)) != sizeof (header)) {
				YYABORT;
			}
		} else {
			daqd.trender.fsd.print_stats (yyout, $4);
		}
	}

	| STATUS MINUTE_TREND FILESYS OptionalIntnum {
		AUTH_CHECK(((my_lexer *)lexer));
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();
		if (((my_lexer *)lexer) -> strict) {
			// send one comm block
			*yyout << S_DAQD_OK << "00000000" << flush; // an ACK first, then dummy net-writer id
			unsigned int header [6];
			// block is empty, just the header
			header [0] = htonl (1);
			header [1] = htonl (4 * sizeof (unsigned int));
			// number of seconds of data
			header [2] = htonl (daqd.trender.minute_fsd.get_max()
					    -daqd.trender.minute_fsd.get_min()); // period
			header [3] = htonl (daqd.trender.minute_fsd.get_min()); // time
			header [4] = htonl (0); // nanoresidual
			header [5] = htonl (0); // sequence number
			if (basic_io::writen (((my_lexer *)lexer) -> ofd,
			    (char *) header, sizeof (header)) != sizeof (header)) {
				YYABORT;
			}
		} else {
			daqd.trender.minute_fsd.print_stats (yyout, $4);
		}
	}
	| BLOCKS MAIN {
		AUTH_CHECK(((my_lexer *)lexer));
		int i;
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

		if (! ((my_lexer *)lexer)->cb) {
			*yyout << "no main buffer" << endl;
		}

		*yyout << "number\t|busy\t|bytes\t|gps\t\t|gps_n\t\t|run" << endl;
		for (i = 0; i < ((my_lexer *)lexer)->cb -> blocks; i++) {
			*yyout << i << "\t" << ((my_lexer *)lexer)->cb -> block [i].busy
				<< "\t" << ((my_lexer *)lexer)->cb -> block [i].bytes 
				<< "\t" << ((my_lexer *)lexer)->cb -> block [i].prop.gps
				<< "\t" << ((my_lexer *)lexer)->cb -> block [i].prop.gps_n
				<< "\t" << ((my_lexer *)lexer)->cb -> block [i].prop.run << endl;
		}
	}
	| START MAIN OptionalIntnum{
		AUTH_CHECK(((my_lexer *)lexer));
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

		if (! daqd.num_channels) {
			*yyout << "`start main': there are no channels configured" << endl;
			exit (1);
		}
		if ($3 > MAX_BLOCKS) {
		  *yyout << "`start main': over the limit on circ buffer blocks" << endl;
		sleep(1);
		  exit (1);
		} else {
		  if (! daqd.start_main ($3? $3: 16, yyout)) {
		    system_log(1, "main started");
		    assert (daqd.b1);
		    ((my_lexer *)lexer) -> cb = daqd.b1 -> buffer_ptr ();
		  } else
		    exit (1);
		}
	}
	| START PRODUCER {
		AUTH_CHECK(((my_lexer *)lexer));
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

		if (! daqd.b1) {
			*yyout << "`start producer': start main before starting producer" << endl;
			exit (1);
		} else {
			if (! daqd.start_producer (yyout)) {
				system_log(1, "producer started");
			} else
				exit (1);
		}		
	}
	| ENABLE FRAME_WIPER {
		AUTH_CHECK(((my_lexer *)lexer));
		daqd.fsd.enable_wiper ();
		system_log(1, "frame wiper enabled");
	}
	| DISABLE FRAME_WIPER {
		AUTH_CHECK(((my_lexer *)lexer));
		daqd.fsd.disable_wiper ();
		system_log(1, "frame wiper disabled");
	}
	| SYNC  FRAME_SAVER {
#if EPICS_EDCU == 1
	   extern unsigned int pvValue[1000];
	   pvValue[5] = 0;
	   pvValue[0] = 0;
#endif
	   // Command is used to sync with the `start frame-saver'
	   for (;sem_trywait (&daqd.frame_saver_sem);) {
#if EPICS_EDCU == 1
	     //pvValue[5]++;
	     pvValue[0]++;
#endif
	     sleep(1);
	   }
	   sem_post (&daqd.frame_saver_sem);
	}
	| START FRAME_SAVER {
		AUTH_CHECK(((my_lexer *)lexer));
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

		if (! daqd.b1) {
			*yyout << "`start main' before starting frame saver" << endl;
			exit (1);
		} else {
			if (! daqd.start_frame_saver (yyout)) {
				system_log(1, "frame saver started");
			} else
				exit (1);
		}		
	}
/*
	| SET TREND_FRAMES_PER_FILE INTNUM {
		daqd.trender.frames_per_file = $3;
	}
	| SET TREND_BUF_BLOCKS INTNUM {
		daqd.trender.trend_buffeR_blocks = $3;
	}
*/
	| ENABLE TREND_FRAME_WIPER {
		AUTH_CHECK(((my_lexer *)lexer));
		daqd.trender.fsd.enable_wiper ();
		system_log(1, "trend frame wiper enabled");
	}
	| DISABLE TREND_FRAME_WIPER {
		AUTH_CHECK(((my_lexer *)lexer));
		daqd.trender.fsd.disable_wiper ();
		system_log(1, "trend frame wiper disabled");
	}
	| SYNC  TREND_FRAME_SAVER {
#if EPICS_EDCU == 1
	   extern unsigned int pvValue[1000];
	   //pvValue[5] = 1000;
#endif
	   // Command is used to sync with the `start trend-frame-saver'
	   for (;sem_trywait (&daqd.trender.frame_saver_sem);) {
#if EPICS_EDCU == 1
	     //pvValue[5]++;
	     pvValue[0]++;
#endif
	     sleep(1);
	   }
	   sem_post (&daqd.trender.frame_saver_sem);
	}
	| SYNC  MINUTE_TREND_FRAME_SAVER {
#if EPICS_EDCU == 1
	   extern unsigned int pvValue[1000];
	   //pvValue[5] = 2000;
#endif
	   // Command is used to sync with the `start minute-trend-frame-saver'
	   for (;sem_trywait (&daqd.trender.minute_frame_saver_sem);) {
#if EPICS_EDCU == 1
	     //pvValue[5]++;
	     pvValue[0]++;
#endif
	     sleep(1);
	   }
	   sem_post (&daqd.trender.minute_frame_saver_sem);
	}
	| START RAW_MINUTE_TREND_SAVER {
		AUTH_CHECK(((my_lexer *)lexer));
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

		if (! daqd.trender.tb) {
			*yyout << "`start trend' before starting minute trend frame saver" << endl;
			system_log(1, "`start trend' before starting minute trend frame saver");
			exit (1);
		} else {
			if (! daqd.trender.start_raw_minute_trend_saver (yyout)) {
				system_log(1, "raw minute trend frame saver started");
			} else
				exit (1);
		}
	}
	| START MINUTE_TREND_FRAME_SAVER {
		AUTH_CHECK(((my_lexer *)lexer));
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

		if (! daqd.trender.tb) {
			*yyout << "`start trend' before starting minute trend frame saver" << endl;
			system_log(1, "`start trend' before starting minute trend frame saver");
			exit (1);
		} else {
			if (! daqd.trender.start_minute_trend_saver (yyout)) {
				system_log(1, "minute trend frame saver started");
			} else
				exit (1);
		}
	}
	| START TREND_FRAME_SAVER {
		AUTH_CHECK(((my_lexer *)lexer));
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

		if (! daqd.trender.tb) {
			*yyout << "`start trend' before starting trend frame saver" << endl;
			system_log(1, "`start trend' before starting trend frame saver");
			exit (1);
		} else {
			if (! daqd.trender.start_trend_saver (yyout)) {
				system_log(1, "trend frame saver started");
			} else
				exit (1);
		}
	}

/*			Trend buffer
		       	size (in blocks) */

	| SYNC  TREND {
	   // Command is used to sync with the `start trend'
	   sem_wait (&daqd.trender.trender_sem);
	   sem_post (&daqd.trender.trender_sem);
	}
	| START TREND OptionalIntnum {
		AUTH_CHECK(((my_lexer *)lexer));
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

		if (! ((my_lexer *)lexer)->cb) {
		  *yyout << "no main buffer" << endl;
		}
		else if (! daqd.trender.num_channels)
		  *yyout << "there are no trend channels configured" << endl;
		else {
		  if (!daqd.trender.start_trend (yyout, 1, 1, 60, 60, $3)) {
		    system_log(1, "trender started");
		  }
		}
	}
	| START LISTENER INTNUM OptionalIntnum {
		AUTH_CHECK(((my_lexer *)lexer));
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

		// Check if there are no more than allowed number of listeners
		if (daqd.num_listeners >= daqd_c::max_listeners)
			*yyout << "no more listeners" << endl;
		else {
			daqd.listeners [daqd.num_listeners].start_listener (yyout, $3, $4);
			daqd.num_listeners++;
		}
	}

/*
  All the KILL commands should be thought out later on if required.
  This kill not working properly all the time.

	| KILL TREND {
		AUTH_CHECK(((my_lexer *)lexer));
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

		if (!daqd.trender.kill_trend (yyout))
			*yyout << "killed" << endl;
	}
*/
	/* 		avg period seconds			IP address:port	       	gps_seconds_start	period_seconds */
	| START TREND	OptionalIntnum 		NET_WRITER	OptionalTextExpression	OptionalIntnum		OptionalIntnum	 {((my_lexer *) lexer) -> trend_channels = 1; /* Only allow minute trend archive (for now) */;  ((my_lexer *) lexer) -> num_channels = 0; ((my_lexer *)lexer) -> error_code = 0; ((my_lexer *) lexer) -> n_archive_channel_names = 0;} ChannelNames {
		int no_data_connection;

		// Do not try to obtain any data from the online data buffers
		// when minute trend is requested.
		int no_online = ($3) > 1;

		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

		// Check errors in channel names here
		if (((my_lexer *)lexer) -> error_code) {
			if (((my_lexer *)lexer) -> strict)
				*yyout << setw(4) << setfill ('0') << hex << ((my_lexer *) lexer) -> error_code << dec << flush;
			else
				*yyout << "Network writer is not started; lexer errno=" << ((my_lexer *)lexer) -> error_code << endl;
		} else

		// Allow averaging over a minute 
		if ($3 != 0 && $3 != 60) {
			if (((my_lexer *)lexer) -> strict)
				*yyout << S_DAQD_NOT_SUPPORTED << flush;
			else
				*yyout << "Invalid trend averaging period (only 60 second allowed)" << endl;
		} else

		// Check if no trend buffer
		if (! daqd.trender.tb) {
			if (((my_lexer *)lexer) -> strict)
				*yyout << S_DAQD_NO_TRENDER << flush;
			else
				*yyout << "Trender is not started" << endl;
		} else if (daqd.offline_disabled && ($6 || $7)) { // Check if off-line data request is disabled
			if (((my_lexer *)lexer) -> strict)
				*yyout << S_DAQD_NO_OFFLINE << flush;
			else
				*yyout << "Off-line data request disabled" << endl;
		} else {
			void *mptr = malloc (sizeof(net_writer_c));
			if (!mptr) {
				if (((my_lexer *)lexer) -> strict)
					*yyout << S_DAQD_MALLOC << flush;
				else
					*yyout << "Virtual memory exhausted" << endl;

				goto start_trend_bailout;
			}
		    
			net_writer_c *nw = new (mptr) net_writer_c(0);
			if (!nw -> alloc_vars(my_lexer::max_channels)) {
				if (((my_lexer *)lexer) -> strict) *yyout << S_DAQD_MALLOC << flush;
				else *yyout << "Virtual memory exhausted" << endl;

				nw->~net_writer_c ();
				free ((void *) nw);
				goto start_trend_bailout;
 			}
			int errc;

			// Check if any of the given channel names are not found in the current DAQ configuration
			// in which case I need to point this out to the net_writer which shouldn't and will not
			// try to obtain any data from the online buffers. NDS will process the request. The net-writer
			// will prepare job configuration file with all relevant information and then pass the job on to
			// the NDS server.
			//
			if (((my_lexer *)lexer) -> n_archive_channel_names > 0)
				no_online = 1;

			// $5 is the string in the format `127.0.0.1:9090'
			// If the string is not present -- use control connection for data transport
			// If there is no IP address, use the port and extract IP address
			// from the request and open data connection to this address using specified port.
			if ($5) {
				long ip_addr = -1;

				if (! daqd_c::is_valid_dec_number ($5))
					ip_addr = net_writer_c::ip_str ($5);

				// Extract IP address
				if (ip_addr == -1) {

					if ((ip_addr = net_writer_c::ip_fd (((my_lexer *)lexer) -> ifd)) == -1) {
						if (((my_lexer *)lexer) -> strict)
							*yyout << setw(4) << setfill ('0') << hex << DAQD_GETPEERNAME << dec << flush;
						else {
							*yyout << "Couldn't do getpeername(); errno=" << errno << endl;
						}
						nw->~net_writer_c ();
						free ((void *) nw);
						goto start_trend_bailout;
					}
				}
				nw -> srvr_addr.sin_addr.s_addr = (u_long) ip_addr;
				nw -> srvr_addr.sin_family = AF_INET;
				nw -> srvr_addr.sin_port = net_writer_c::port_str ($5);
				no_data_connection = 0;
			} else
				no_data_connection = 1;

			// calculate block size
			/*

Wed Oct 14 16:42:36 PDT 1998, avi: can't ever use all channels option on trend buffers, since there are holes in data.


			if ($9) {
			        DEBUG1(cerr << "all channels selected" << endl);
				nw -> transmission_block_size
				  = nw -> block_size = daqd.trender.tb -> block_size ();
				memcpy (nw -> channels,
					daqd.trender.trend_channels,
					sizeof (nw -> channels [0]) * daqd.trender.num_trend_channels);
				nw -> num_channels = daqd.trender.num_trend_channels;
			} else 
			*/

			{
			     if ($9) {
			        DEBUG1(cerr << "all channels selected" << endl);
				memcpy (nw -> channels,
					daqd.trender.trend_channels,
					sizeof (nw -> channels [0]) * daqd.trender.num_trend_channels);
				nw -> num_channels = daqd.trender.num_trend_channels;
			     } else {
				memcpy (nw -> channels,
					((my_lexer *) lexer) -> channels,
					sizeof (nw -> channels [0]) * ((my_lexer *) lexer) -> num_channels);
				nw -> num_channels = ((my_lexer *) lexer) -> num_channels;
			     }
				nw -> block_size = 0;
				DEBUG1(cerr << "num_channels=" << nw -> num_channels << endl);
				for (int i = 0; i < nw -> num_channels; i++) {
					nw -> block_size += nw -> channels [i].bytes;

					// Construct put vector array
					if (!nw -> pvec_len) { // Start vector
						nw -> pvec [0].vec_idx = nw -> channels [i].offset;
						nw -> pvec [0].vec_len = nw -> channels [i].bytes;
						nw -> pvec_len++;
					} else if (nw -> pvec [nw -> pvec_len -1].vec_idx + nw -> pvec [nw -> pvec_len -1].vec_len == nw -> channels [i].offset) {
						nw -> pvec [nw -> pvec_len -1].vec_len += nw -> channels [i].bytes; // Increase vector element length for contiguous channels
					} else { // Start new vector if there is a gap in channel sequence
						nw -> pvec [nw -> pvec_len].vec_idx = nw -> channels [i].offset;
						nw -> pvec [nw -> pvec_len].vec_len = nw -> channels [i].bytes;
						nw -> pvec_len++;
					}
				}
				// No data decimation on the trend could be possibly done, since the trend is coming
				// with the lowest rate possible
				nw -> transmission_block_size = nw -> block_size;

				DEBUG1(cerr << "vec_idx\tvec_len" << endl);
				DEBUG1(for(int j=0; j < nw -> pvec_len; j++) cerr << nw -> pvec [j].vec_idx << '\t' << nw -> pvec [j].vec_len << endl);
			}
			
			if (!(errc = nw -> start_net_writer (yyout, ((my_lexer *)lexer) -> ofd,	no_data_connection,
							     $3 > 1? daqd.trender.mtb: daqd.trender.tb,
							     $3 > 1? &daqd.trender.minute_fsd:&daqd.trender.fsd, $6, $7, no_online)))
                        {

				if (! no_data_connection) {
					if (((my_lexer *)lexer) -> strict) {
						*yyout << S_DAQD_OK << flush;
						// send writer ID
						*yyout << setfill ('0') << setw (sizeof (unsigned long) * 2) << hex
							<< (unsigned long) nw << dec << flush;
					} else {
						*yyout << "started" << endl;
						*yyout << "writer id: " << (unsigned long) nw << endl;
					}
				}
			} else {						 // a problem
				nw->~net_writer_c ();
				free ((void *) nw);
				if (((my_lexer *)lexer) -> strict)
					*yyout << setw(4) << setfill ('0') << hex << errc << dec << flush;
				else
					*yyout << "error " << errc << endl;
			}
		}

start_trend_bailout:
		if ($5)
			free ($5);
	}

	/* 			IP address:port		gps_seconds_start	period_seconds */
	| START WriterType	OptionalTextExpression	OptionalIntnum		OptionalIntnum  BroadcastOption	DecimateOption {((my_lexer *) lexer) -> trend_channels = 0; ((my_lexer *) lexer) -> num_channels = 0; ((my_lexer *)lexer) -> error_code = 0;((my_lexer *) lexer) -> n_archive_channel_names = 0;} ChannelNames {
	 int no_data_connection;
	 ostream *yyout = ((my_lexer *)lexer)->get_yyout ();
	 int ifd = ((my_lexer *)lexer)->get_ifd ();

	 // Check if no main buffer
	 if (! daqd.b1) {
		if (((my_lexer *)lexer) -> strict)
			*yyout << S_DAQD_NO_MAIN << flush;
		else
			*yyout << "Main is not started" << endl;
	 } else if (daqd.offline_disabled && ($4 || $5)) { // Check if off-line data request is disabled
		if (((my_lexer *)lexer) -> strict)
			*yyout << S_DAQD_NO_OFFLINE << flush;
		else
			*yyout << "Off-line data request disabled" << endl;
	 } else

	 // Check errors in channel names here
	 if (((my_lexer *)lexer) -> error_code) {
		if (((my_lexer *)lexer) -> strict)
			*yyout << setw(4) << setfill ('0') << hex << ((my_lexer *) lexer) -> error_code << dec << flush;
		else
			*yyout << "Network writer is not started" << endl;
		goto start_writer_bailout;
	 } else {
	 void *mptr = malloc (sizeof(net_writer_c));
	 if (!mptr) {
		if (((my_lexer *)lexer) -> strict)
			*yyout << S_DAQD_MALLOC << flush;
		else
			*yyout << "Virtual memory exhausted" << endl;
		goto start_writer_bailout;
	 }
		    
  	 DEBUG1(cerr << "Creating net_writer tagged " << ifd << endl);
	 net_writer_c *nw = new (mptr) net_writer_c(ifd);
	 if (!nw -> alloc_vars($9? MAX_CHANNELS: my_lexer::max_channels)) {
		if (((my_lexer *)lexer) -> strict) *yyout << S_DAQD_MALLOC << flush;
		else *yyout << "Virtual memory exhausted" << endl;

		nw->~net_writer_c ();
		free ((void *) nw);
		goto start_writer_bailout;
 	 }
	 int errc;

#ifndef NO_BROADCAST
	 nw -> broadcast = (long) $6;
	 nw -> mcast_interface = ((long) $6 > 1)? strdup ($6): 0;
#endif
	 if ((unsigned long) $6 > 1)
		free ($6);

         nw -> no_averaging = ( $7 == DOWNSAMPLE );
	 system_log(1, "no_average=%d\n", nw -> no_averaging);
   	 

	 if (($2) == NAME_WRITER) {
		// Bail out if frame saver is not running, no point to start it hanging around, i guess (???)
		// Bail out on a request with any channel specs (only `all' is valid)
		// Bail out on a request with any periods
		// Bail out if there is no circular buffer, ie, if !daqd.fsd.cb

		if (! daqd.frame_saver_tid) {
		    if (((my_lexer *)lexer) -> strict)
		      *yyout << S_DAQD_NO_OFFLINE << flush;
		    else
		      *yyout << "name writer is not started, since frame saver is not running" << endl;
		}

		if (($4) || ($5) || ! ($9)) {
		    if (((my_lexer *)lexer) -> strict)
		      *yyout << S_DAQD_ERROR << flush;
		    else
		      *yyout << "invalid command" << endl;
		}

		//FIXME
		// Should I try to construct this circular buffer on the first request?
		if (! daqd.fsd.cb) {
		    if (((my_lexer *)lexer) -> strict)
		      *yyout << S_DAQD_NOT_SUPPORTED << flush;
		    else
		      *yyout << "main filesys map doesn't have a circular buffer constructed" << endl;
		}

		if (! daqd.frame_saver_tid || ($4) || ($5) || !($9)|| !daqd.fsd.cb) {
		    nw -> ~net_writer_c ();
		    free ((void *) nw);
		    goto start_writer_bailout;
		}
		nw -> writer_type = net_writer_c::name_writer;
	 } else if (($2) == FRAME_WRITER) {
		  int not_supported = ($4) ^ ($5)
			|| (($4) && ($5) && ! ($9)); // only format `start frame-writer 123123 10 all' supported

		  // Decimation is not supported
		  for (int i = 0; i < ((my_lexer *) lexer) -> num_channels; i++)
		  	not_supported |= ((my_lexer *) lexer) -> channels [i].req_rate;
		  if (not_supported) {
		    if (((my_lexer *)lexer) -> strict)
		      *yyout << S_DAQD_NOT_SUPPORTED << flush;
		    else
		      *yyout << "frame write call format is not supported" << endl;
		    nw -> ~net_writer_c ();
		    free ((void *) nw);
		    goto start_writer_bailout;
		  }
		  nw -> writer_type = net_writer_c::frame_writer;
		} else if (($2) == FAST_WRITER) {
		  if (($4) || ($5) ) {
		    if (((my_lexer *)lexer) -> strict)
		      *yyout << S_DAQD_ERROR << flush;
		    else
		      *yyout << "invalid command" << endl;

		    nw -> ~net_writer_c ();
		    free ((void *) nw);
		    goto start_writer_bailout;
		  }
		  nw -> writer_type = net_writer_c::fast_writer;
		} else {
			  nw -> writer_type = net_writer_c::slow_writer;
		}


		// $3 is the string in the format `127.0.0.1:9090'
		// If the string is not present -- use control connection for data transport
		// If there is no IP address, usethe port and extract IP address
		// from the request and open data connection to this address using specified port.
		if ($3) {
			long ip_addr = -1;

			if (! daqd_c::is_valid_dec_number ($3))
				ip_addr = net_writer_c::ip_str ($3);

			// Extract IP address
			if (ip_addr == -1) {
				if ((ip_addr = net_writer_c::ip_fd (((my_lexer *)lexer) -> ifd)) == -1) {
					if (((my_lexer *)lexer) -> strict)
						*yyout << setw(4) << setfill ('0') << hex << DAQD_GETPEERNAME << dec << flush;
					else {
						*yyout << "Couldn't do getpeername(); errno=" << errno << endl;
					}
					nw -> ~net_writer_c ();
					free ((void *) nw);
					goto start_writer_bailout;
				}
			}
			nw -> srvr_addr.sin_addr.s_addr = (u_long) ip_addr;
			nw -> srvr_addr.sin_family = AF_INET;
			nw -> srvr_addr.sin_port = net_writer_c::port_str ($3);
			no_data_connection = 0;
		} else
			no_data_connection = 1;

		if (nw -> writer_type == net_writer_c::name_writer) {
			DEBUG1(cerr << "name writer on the frame files" << endl);
			nw -> transmission_block_size = nw -> block_size = daqd.fsd.cb -> block_size ();
			nw -> num_channels = 0;
			nw -> pvec [0].vec_idx = 0;
			nw -> pvec [0].vec_len = daqd.fsd.cb -> block_size ();
			nw -> pvec_len = 1;
		} else {
			int fast_net_writer = nw -> writer_type == net_writer_c::fast_writer;

			if ($9) {
			  DEBUG1(cerr << "all channels selected" << endl);
#if 0
			  memcpy (nw -> channels, daqd.channels,
				  sizeof (nw -> channels [0]) * daqd.num_channels);
#endif // 0

			  nw -> num_channels = daqd.num_channels
#ifdef GDS_TESTPOINTS
// Do not send gds channels to broadcaster for now
			    - daqd.num_gds_channels - daqd.num_gds_channel_aliases
#endif
				    ;
			  unsigned int j = 0;
			  for (unsigned int i=0; i < daqd.num_channels; i++) {
#if 0
				if (nw -> broadcast) {
					if (daqd.channels [i].sample_rate > 16384
					    || daqd.channels [i].data_type == _32bit_complex) {
						nw -> num_channels--;
						continue;
					}
				}
#endif

#ifdef GDS_TESTPOINTS
				if ((!IS_GDS_ALIAS (daqd.channels [i]))
				   && (!IS_GDS_SIGNAL (daqd.channels [i]))) 	
#endif
			        	nw -> channels [j++] = daqd.channels [i];
			  }
			} else {
				memcpy (nw -> channels, ((my_lexer *) lexer) -> channels, sizeof (nw -> channels [0]) * ((my_lexer *) lexer) -> num_channels);
				nw -> num_channels = ((my_lexer *) lexer) -> num_channels;
			}

			nw -> block_size = 0;
			DEBUG1(cerr << "num_channels=" << nw -> num_channels << endl);

			if (fast_net_writer) {
				// Check if any channel rate is less than 16
				for (int i = 0; i < nw -> num_channels; i++)
					if (nw -> channels -> req_rate && nw -> channels -> req_rate < 16) {
						if (((my_lexer *)lexer) -> strict)
							*yyout << S_DAQD_INVALID_CHANNEL_DATA_RATE << flush;
		  				else
		    					*yyout << "invalid channel data rate -- minimum rate for fast net-writer is 16" << endl;

						nw -> ~net_writer_c ();
						free ((void *) nw);
						goto start_writer_bailout;
					}
			}
#ifdef GDS_TESTPOINTS
			// See if there are any GDS alias channels
			// If there are any, need to find real GDS
			// channels carrying the data, then update
			// the offset
			{
				  int na = 0;
				  //char *alias[nw -> num_channels];
				  //unsigned int tpnum[nw -> num_channels];
				  long_channel_t *ac [nw -> num_channels];
				  channel_t *gds [nw -> num_channels];

				  for (int i = 0; i < nw -> num_channels; i++) {
				    //printf("channel %d tp_node=%d\n", i, nw -> channels [i].tp_node);
				    if (IS_GDS_ALIAS(nw -> channels [i])) {
				      ac [na] = nw -> channels + i;
				      //tpnum [na] = nw -> channels [i].chNum;
				      //alias [na] = nw -> channels [i].name;
				      na++;
				    }
				  }
				  if (na) {
				    int res = -1;
				    //res = daqd.gds.req_names (alias, tpnum, gds,na);
				    if (daqd.tp_allowed(net_writer_c::ip_fd(((my_lexer *)lexer) -> ifd)))
				      res = daqd.gds.req_tps(ac, gds, na);

				    if (res) {
				      //if (daqd.tp_allowed(net_writer_c::ip_fd(((my_lexer *)lexer) -> ifd)))
				        //daqd.gds.clear_tps(ac, na);
				      if (((my_lexer *)lexer) -> strict)
				 	*yyout << S_DAQD_NOT_FOUND << flush;
			  	       else
			    		*yyout << "failed to find GDS signal for one or more aliases" << endl;

					nw -> ~net_writer_c ();
					free ((void *) nw);
					goto start_writer_bailout;
				    }
				  }
				  nw -> clear_testpoints = 1;
			}
#endif // #ifdef GDS_TESTPOINTS

			for (int i = 0; i < nw -> num_channels; i++) {
				if (fast_net_writer) {
					if (nw -> channels [i].req_rate)
					  nw -> transmission_block_size += nw -> channels [i].bps * nw -> channels [i].req_rate/16;
					else
					  nw -> transmission_block_size += nw -> channels [i].bytes/16;

					nw -> block_size += nw -> channels [i].bytes / 16;
					for (int j = 0; j < 16; j++) {
						nw -> pvec16th [j][nw -> pvec_len].vec_idx
							= nw -> channels [i].offset + nw -> channels [i].bytes / 16 * j;
						nw -> pvec16th [j][nw -> pvec_len].vec_len = nw -> channels [i].bytes / 16;
					}
					nw -> pvec_len++;


					// Contruct another scattered data array to be used for data decimation
					// in the net_writer_c::consumer()
					// `dec_vec [].vec_idx' is the offset in the net-writer circ buffer data block
					if (!nw -> dec_vec_len) { // Start the vector
							nw -> dec_vec [0].vec_idx = 0;
							nw -> dec_vec [0].vec_len = nw -> channels [i].bytes/16;
							nw -> dec_vec [0].vec_rate = nw -> channels [i].req_rate/16;
							nw -> dec_vec [0].vec_bps = nw -> channels [i].bps;
							nw -> dec_vec_len++;
					} else if (nw -> channels [i -1].req_rate == nw -> channels [i].req_rate
						   && nw -> channels [i -1].bytes == nw -> channels [i].bytes
						   && nw -> channels [i -1].bps == nw -> channels [i].bps) {

							// Increase data element length for contiguous channels with the same rate, same requested rate	
							// and the same number bytes per sample.
							// We should be able to treat these channels as one channel with the
							// increased data rate.

							nw -> dec_vec [nw -> dec_vec_len -1].vec_len += nw -> channels [i].bytes/16;
							nw -> dec_vec [nw -> dec_vec_len -1].vec_rate += nw -> channels [i].req_rate/16;
					} else {

							// Start new vector if rate or BPS changes

							nw -> dec_vec [nw -> dec_vec_len].vec_idx
							 = nw -> dec_vec [nw -> dec_vec_len -1].vec_idx
							   + nw -> dec_vec [nw -> dec_vec_len -1].vec_len;
							nw -> dec_vec [nw -> dec_vec_len].vec_len = nw -> channels [i].bytes/16;
							nw -> dec_vec [nw -> dec_vec_len].vec_rate = nw -> channels [i].req_rate/16;
							nw -> dec_vec [nw -> dec_vec_len].vec_bps = nw -> channels [i].bps;
							nw -> dec_vec_len++;						
					}
				} else {
					if (nw -> channels [i].req_rate)
					  	nw -> transmission_block_size += nw -> channels [i].bps * nw -> channels [i].req_rate;
					else
						nw -> transmission_block_size += nw -> channels [i].bytes;

					nw -> block_size += nw -> channels [i].bytes;
					// Construct put vector array
					if (!nw -> pvec_len) { // Start vector
							nw -> pvec [0].vec_idx = nw -> channels [i].offset;
							nw -> pvec [0].vec_len = nw -> channels [i].bytes;
							nw -> pvec_len++;
					} else if (nw -> pvec [nw -> pvec_len -1].vec_idx + nw -> pvec [nw -> pvec_len -1].vec_len
							   == nw -> channels [i].offset) {
							// Increase vector element length for contiguous channels
							nw -> pvec [nw -> pvec_len -1].vec_len += nw -> channels [i].bytes;
					} else { // Start new vector if there is a gap in channel sequence
							nw -> pvec [nw -> pvec_len].vec_idx = nw -> channels [i].offset;
							nw -> pvec [nw -> pvec_len].vec_len = nw -> channels [i].bytes;
							nw -> pvec_len++;
					}

					// Contruct another scattered data array to be used for data decimation
					// in the net_writer_c::consumer()
					// `dec_vec [].vec_idx' is the offset in the net-writer circ buffer data block
					if (!nw -> dec_vec_len) { // Start the vector
							nw -> dec_vec [0].vec_idx = 0;
							nw -> dec_vec [0].vec_len = nw -> channels [i].bytes;
							nw -> dec_vec [0].vec_rate = nw -> channels [i].req_rate;
							nw -> dec_vec [0].vec_bps = nw -> channels [i].bps;
							nw -> dec_vec_len++;
					} else if (nw -> channels [i -1].req_rate == nw -> channels [i].req_rate
							   && nw -> channels [i -1].bytes == nw -> channels [i].bytes
							   && nw -> channels [i -1].bps == nw -> channels [i].bps) {

							// Increase data element length for contiguous channels with the same rate	
							// and the same number bytes per sample.
							// We should be able to treat these channels as one channel with the
							// increased data rate.

							nw -> dec_vec [nw -> dec_vec_len -1].vec_len += nw -> channels [i].bytes;
							nw -> dec_vec [nw -> dec_vec_len -1].vec_rate += nw -> channels [i].req_rate;
					} else {

							// Start new vector if rate or BPS changes

							nw -> dec_vec [nw -> dec_vec_len].vec_idx
							 = nw -> dec_vec [nw -> dec_vec_len -1].vec_idx
							   + nw -> dec_vec [nw -> dec_vec_len -1].vec_len;
							nw -> dec_vec [nw -> dec_vec_len].vec_len = nw -> channels [i].bytes;
							nw -> dec_vec [nw -> dec_vec_len].vec_rate = nw -> channels [i].req_rate;
							nw -> dec_vec [nw -> dec_vec_len].vec_bps = nw -> channels [i].bps;
							nw -> dec_vec_len++;						
					}
				}
			}

			nw -> decimation_requested = !nw -> broadcast;
			printf("decimation flag=%d\n", nw -> decimation_requested);
			nw -> block_size += nw -> num_channels * sizeof(int);
			if (fast_net_writer) {
			  if (!nw -> broadcast) {
			    unsigned long  status_ptr = sizeof(int) + daqd.block_size
							- 17 * sizeof(int) * daqd.num_channels;
			    for (int i = 0; i < nw -> num_channels; i++) {
			      for (int j = 0; j < 16; j++) {
				nw -> pvec16th [j][nw -> pvec_len].vec_idx
				  = j*sizeof(int) + status_ptr + 17 * sizeof(int) * nw -> channels [i].seq_num;
			        nw -> pvec16th [j][nw -> pvec_len].vec_len = sizeof(int);
			      }
			      nw -> pvec_len++;
			    }
			  }
			} else {
			  // append one element per channel to save the status word
			  //
			  unsigned long  status_ptr = daqd.block_size - 17 * sizeof(int) * daqd.num_channels;
			  for (int i = 0; i < nw -> num_channels; i++) {
			      nw -> pvec [nw -> pvec_len].vec_idx = status_ptr + 17 * sizeof(int) * nw -> channels [i].seq_num;
			      nw -> pvec [nw -> pvec_len].vec_len = sizeof(int);
			      nw -> pvec_len++;					
			  }				
			}
			if (!fast_net_writer) {
				DEBUG1(cerr << "pvec->vec_idx\tpvec->vec_len" << endl);
				DEBUG1(for(int j=0; j < nw -> pvec_len; j++) cerr << nw -> pvec [j].vec_idx << '\t' << nw -> pvec [j].vec_len << endl);
			}
			DEBUG1(cerr << "dec_vec->vec_idx\tdec_vec->vec_len\tdec_vec->vec_rate\tdec_vec->vec_bps" << endl);
			DEBUG1(for(int k=0; k < nw -> dec_vec_len; k++) cerr << nw -> dec_vec [k].vec_idx << '\t' << nw -> dec_vec [k].vec_len << '\t' << nw -> dec_vec [k].vec_rate << '\t' << nw -> dec_vec [k].vec_bps << endl);
		} // not name writer
		circ_buffer *this_cb;

		if (nw -> writer_type == net_writer_c::name_writer)
		  this_cb = daqd.fsd.cb;
		else {
		  this_cb = daqd.b1;
		  nw -> need_send_reconfig_block = true;
		}

		if (!(errc = nw -> start_net_writer (yyout, ((my_lexer *)lexer) -> ofd,	no_data_connection,
							this_cb, &daqd.fsd, $4, $5, 0))) {
			if (! no_data_connection) {
				if (((my_lexer *)lexer) -> strict) {
					*yyout << S_DAQD_OK << flush;
					// send writer ID
					*yyout << setfill ('0') << setw (sizeof (unsigned long) * 2) << hex
						<< (unsigned long) nw << dec << flush;
				} else {
					*yyout << "started" << endl;
					*yyout << "writer id: " << (unsigned long) nw << endl;
				}
			}
		} else {						 // a problem
			nw -> ~net_writer_c ();
			free ((void *) nw);

			if (((my_lexer *)lexer) -> strict)
				*yyout << setw(4) << setfill ('0') << hex << errc << dec << flush;
			else
				*yyout << "error " << errc << endl;
		}
        }
start_writer_bailout:
		if ($3)
			free ($3);
	}

	| KILL NET_WRITER INTNUM {
		int errc;
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();
		int ifd = ((my_lexer *)lexer)->get_ifd ();
		net_writer_c *nw;

		nw = (net_writer_c *) daqd.net_writers.find_id (ifd);

		if (nw) {
			if (!(errc = nw -> kill_net_writer ())) {
				if (((my_lexer *)lexer) -> strict) {
					*yyout << S_DAQD_OK << flush;
				} else
					*yyout << "killed" << endl;
			} else {
				if (((my_lexer *)lexer) -> strict)
					*yyout << setw(4) << setfill ('0') << hex << errc << dec << flush;
				else
					*yyout << "error " << errc << endl;
			}
		} else {
			if (((my_lexer *)lexer) -> strict)
				*yyout << setw(4) << setfill ('0') << hex << DAQD_NO_SUCH_NET_WRITER << dec << flush;
			else
				*yyout << "no such network writer" << endl;
		}

/*
		if (!(errc = daqd.net_writer.kill_net_writer (yyout, 0))) {	// succesfully killed
			if (((my_lexer *)lexer) -> strict) {
				*yyout << S_DAQD_OK << flush;
			} else
				*yyout << "killed" << endl;
		} else {						 // a problem
			if (((my_lexer *)lexer) -> strict)
				*yyout << setw(4) << setfill ('0') << hex << errc << dec << flush;
			else
				*yyout << "error " << errc << endl;
		}
*/

	}
	;

WriterType: NET_WRITER { $$=NET_WRITER; }
	| FRAME_WRITER { $$=FRAME_WRITER; }
	| FAST_WRITER { $$=FAST_WRITER; }
	| NAME_WRITER { $$=NAME_WRITER; }
	;

BroadcastOption: /* Nothing */ { $$ = (char *)0; }
	| BROADCAST {$$ = (char *)1; }
	| BROADCAST '=' TextExpression {$$ = $3;}
	;

DecimateOption: /* Nothing */ { $$ = DOWNSAMPLE; }
	| DOWNSAMPLE { $$ = DOWNSAMPLE; }
	| AVERAGE { $$ = AVERAGE; }
	;

OptionalIntnum: /* Nothing */ { $$ = 0; }
	| INTNUM 
	;

ConfigureChannelGroupsBody: BEGIN_BEGIN { daqd.num_channel_groups = 0; } ConfigureChannelGroupsBodyLines END {

	}

ConfigureChannelGroupsBodyLines: /* NOTHING */
	| ConfigureChannelGroupsBodyLine ConfigureChannelGroupsBodyLines
	;

ConfigureChannelGroupsBodyLine: INTNUM TextExpression {
	ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

	if (strlen ($2) >= channel_group_t::channel_group_name_max_len) {
		*yyout << "channel group name is too long. Maximum " << channel_group_t::channel_group_name_max_len - 1 << " characters" << endl;
	} else if (daqd.num_channel_groups >= daqd_c::max_channel_groups) {
		*yyout << "too many channel groups. Channel group`" << $2 << "' ignored" << endl;
	} else {
		int cur_channel_group;

		cur_channel_group = daqd.num_channel_groups++;
		daqd.channel_groups [cur_channel_group].num = $1;
		strcpy (daqd.channel_groups [cur_channel_group].name, $2);
	}
	free ($2);
	}

ConfigureChannelsBody: BEGIN_BEGIN {
  daqd.num_channels = daqd.trender.num_channels = 0;} ConfigureChannelsBodyLines END {
	int i, j, k, offs;
	ostream *yyout = ((my_lexer *)lexer)->get_yyout ();
	int rm_offs = 0;

	// Configure channels from files
	if (daqd.configure_channels_files ()) exit(1);
	daqd.num_active_channels = 0;
	int cur_dcu = -1;

	// Save channel offsets and assign trend channel data struct
	for (i = 0, offs = 0; i < daqd.num_channels + 1; i++) {

		int t = 0;
		if (i == daqd.num_channels) t = 1;
		else t = cur_dcu != -1 && cur_dcu != daqd.channels[i].dcu_id;

		if (IS_TP_DCU(daqd.channels[i].dcu_id) && t) {
		   // Remember the offset to start of TP/EXC DCU data
		   // in main buffer
		   daqd.dcuTPOffset[daqd.channels[i].ifoid][daqd.channels[i].dcu_id] = offs;
		}

		// Finished with cur_dcu: channels sorted by dcu_id
		if (IS_MYRINET_DCU(cur_dcu) && t) {
		   int dcu_size = daqd.dcuSize[daqd.channels[i-1].ifoid][cur_dcu];
		   daqd.dcuDAQsize[daqd.channels[i-1].ifoid][cur_dcu] = dcu_size;

#ifndef USE_BROADCAST
		   // When compiled with USE_BROADCAST we do not allocate contigious memory
		   // in the move buffer for the test points. We allocate this memory later
		   // at the end of the buffer.

		   // Save testpoint data offset
		   daqd.dcuTPOffset[daqd.channels[i-1].ifoid][cur_dcu] = offs;
		   daqd.dcuTPOffsetRmem[daqd.channels[i-1].ifoid][cur_dcu] = rm_offs;

		   // Allocate testpoint data buffer for this DCU
		   int tp_buf_size = 2*DAQ_DCU_BLOCK_SIZE*DAQ_NUM_DATA_BLOCKS_PER_SECOND - dcu_size*DAQ_NUM_DATA_BLOCKS_PER_SECOND;
		   offs += tp_buf_size;
		   daqd.block_size += tp_buf_size;
		   rm_offs += 2*DAQ_DCU_BLOCK_SIZE - dcu_size;
		   DEBUG1(cerr << "Configured MYRINET DCU, block size=" << tp_buf_size << endl);
		   DEBUG1(cerr << "Myrinet DCU size " << dcu_size << endl);
		   daqd.dcuSize[daqd.channels[i-1].ifoid][cur_dcu] = 2*DAQ_DCU_BLOCK_SIZE;
#endif
		}
		if (!IS_MYRINET_DCU(cur_dcu) && t) {
		   daqd.dcuDAQsize[daqd.channels[i-1].ifoid][cur_dcu] = daqd.dcuSize[daqd.channels[i-1].ifoid][cur_dcu];
		}
		if (i == daqd.num_channels) continue;

		cur_dcu = daqd.channels[i].dcu_id;

		daqd.channels [i].bytes = daqd.channels [i].sample_rate * daqd.channels [i].bps;

#ifdef USE_BROADCAST

#ifndef GDS_TESTPOINTS
		// Skip testpoints
		if (IS_TP_DCU(cur_dcu)) continue;
#else
		// We want to keep TP/EXC DCU data at the very end of the buffer
		// so we an receive the broadcast into the move buffer directly
		if (IS_TP_DCU(cur_dcu) && !IS_GDS_ALIAS (daqd.channels [i])) {
		  daqd.dcuSize[daqd.channels[i].ifoid][cur_dcu]
			      += daqd.channels [i].bytes / DAQ_NUM_DATA_BLOCKS_PER_SECOND;
		  printf("ifo %d dcu %d channel %d size %d\n",
			daqd.channels[i].ifoid, cur_dcu, i, daqd.channels [i].bytes);
		  if (IS_EXC_DCU(cur_dcu))
			  daqd.dcuSize[daqd.channels[i].ifoid][cur_dcu] += 4;
		  continue;
		}
#endif
#endif


#ifdef GDS_TESTPOINTS
		if (IS_GDS_ALIAS (daqd.channels [i])) {	
			daqd.channels [i].offset = 0;
		} else {
#endif
			daqd.channels [i].offset = offs;

			offs += daqd.channels [i].bytes;
			daqd.block_size += daqd.channels [i].bytes;

			daqd.dcuSize[daqd.channels[i].ifoid][daqd.channels[i].dcu_id]
			      += daqd.channels [i].bytes / DAQ_NUM_DATA_BLOCKS_PER_SECOND;
			daqd.channels [i].rm_offset = rm_offs;
			rm_offs += daqd.channels [i].bytes / DAQ_NUM_DATA_BLOCKS_PER_SECOND;
			/* For the EXC DCUs: add the status word length */
			if (IS_EXC_DCU(daqd.channels[i].dcu_id)) {
			  daqd.dcuSize[daqd.channels[i].ifoid][daqd.channels[i].dcu_id] += 4;
			  rm_offs += 4;
			  daqd.channels [i].rm_offset += 4;
			}

#ifdef GDS_TESTPOINTS
		}
#endif

		if (daqd.broadcast_set.empty()
			? daqd.channels [i].active
			: daqd.broadcast_set.count(daqd.channels [i].name)) {
				daqd.active_channels [daqd.num_active_channels++] = daqd.channels [i];
		}

		if (daqd.channels [i].trend) {
			if (daqd.trender.num_channels >= trender_c::max_trend_channels) {
				system_log(1, "too many trend channels. No trend on `%s' channel", daqd.channels [i].name);
			} else {
				daqd.trender.channels [daqd.trender.num_channels] = daqd.channels [i];
				daqd.trender.block_size += sizeof (trend_block_t);
				if (daqd.trender.channels[daqd.trender.num_channels].data_type == _32bit_complex) {
				  /* Change data type from complex to float but leave bps field at 8 */
				  /* Trend loop function checks bps variable to detect complex data */
				  daqd.trender.channels[daqd.trender.num_channels].data_type = _32bit_float;
#if 0
				  daqd.trender.channels[daqd.trender.num_channels].bps /= 2;
				  daqd.trender.channels[daqd.trender.num_channels].bytes /= 2;
#endif
				}

				daqd.trender.num_channels++;
			}
		  }
	}


#ifdef USE_BROADCAST
	for (int ifo = 0; ifo < daqd.data_feeds; ifo++) {
	 for (int mdcu = 0; mdcu < DCU_COUNT; mdcu++) {
	  printf("ifo %d dcu %d size %d\n", ifo, mdcu, daqd.dcuSize[ifo][mdcu]);
	  if (0 == daqd.dcuSize[ifo][mdcu]) continue;
	  if (IS_TP_DCU(mdcu) || IS_MYRINET_DCU(mdcu)) {

	    // Save testpoint data offset, at the end of the buffer
	    daqd.dcuTPOffset[ifo][mdcu] = daqd.block_size;

	    // Allocate memory for the test points' buffers at the end of a move buffer
	    daqd.dcuTPOffsetRmem[ifo][mdcu] = rm_offs;

	    // One test point data size
	    int tp_size = daqd.dcuRate[ifo][mdcu] * 4 / DAQ_NUM_DATA_BLOCKS_PER_SECOND;
	    int tp_buf_size = 0;
	    if (IS_TP_DCU(mdcu)) {
#if 0
	      int tp_index = mdcu - DCU_ID_FIRST_GDS;
              int tp_table_len = daqGdsTpNum[tp_index];
              if (tp_table_len > DAQ_GDS_MAX_TP_ALLOWED)  {
                	tp_table_len = DAQ_GDS_MAX_TP_ALLOWED;
              }
	      tp_buf_size = tp_size * tp_table_len;
#endif
	      tp_buf_size = daqd.dcuSize[ifo][mdcu];
	      cerr << "Legacy TP DCU " << mdcu <<" tp data size " << tp_buf_size << endl;

	      // Now set the correct offsets for each channel within this DCU
	      int dcu_chnum = 0;
	      for (int chnum = 0; chnum < daqd.num_channels; chnum++) {
		if (daqd.channels[chnum].dcu_id == mdcu && daqd.channels[chnum].ifoid == ifo) {
		   daqd.channels [chnum].offset = daqd.block_size
			+ dcu_chnum * tp_size * DAQ_NUM_DATA_BLOCKS_PER_SECOND;
		   daqd.channels [chnum].rm_offset = rm_offs
			+ dcu_chnum * tp_size;
		   dcu_chnum++;
		}
	      }
	    } else {
	      // Allocate testpoint data buffer for this DCU
	      tp_buf_size = DAQ_GDS_MAX_TP_ALLOWED * tp_size;
	      if (tp_buf_size > 2*DAQ_DCU_BLOCK_SIZE) {
		// Limit the number of testpoints for this DCU
		tp_buf_size = 2*DAQ_DCU_BLOCK_SIZE;
	      }
	      cerr << "Myrinet DCU " << mdcu <<" tp data size " << tp_buf_size << endl;
	    }
	    rm_offs += tp_buf_size;
	    daqd.block_size += tp_buf_size * DAQ_NUM_DATA_BLOCKS_PER_SECOND;
	  }
	 }
	}
#endif


	// Calculate memory size needed for channels status storage in the main buffer
	// and increment main block size 
	daqd.block_size += 17 * sizeof(int) * daqd.num_channels;

	daqd.trender.num_trend_channels = 0;
	// Assign trend output channels
	for (i = 0, offs = 0; i < daqd.trender.num_channels; i++) {
		int l = strlen(daqd.trender.channels[i].name);
		if (l > (MAX_CHANNEL_NAME_LENGTH - 6)) {
			printf("Channel %s length %d over the limit of %d\n", 
					daqd.trender.channels[i].name,
					(int)strlen(daqd.trender.channels[i].name),
					MAX_CHANNEL_NAME_LENGTH - 6);
			exit(1);
		}
		for (int j = 0; j < trender_c::num_trend_suffixes; j++) {
			daqd.trender.trend_channels [daqd.trender.num_trend_channels]
				= daqd.trender.channels [i];

			// Add a suffix to the channel name
			strcat (daqd.trender.trend_channels [daqd.trender.num_trend_channels].name,
				daqd.trender.sufxs [j]);

			// Set the sample rate always to one, since
			// we have fixed trend calculation period equal to one second
			daqd.trender.trend_channels [daqd.trender.num_trend_channels].sample_rate = 1;

			switch (j) { 
			  case 0:
			  case 1:
      				if (daqd.trender.channels [i].data_type != _64bit_double && daqd.trender.channels [i].data_type != _32bit_float)
			  		daqd.trender.trend_channels [daqd.trender.num_trend_channels].data_type = _32bit_integer;
				else
			  		daqd.trender.trend_channels [daqd.trender.num_trend_channels].data_type = daqd.trender.channels [i].data_type;
				break;
			  case 2: // `n'
				daqd.trender.trend_channels [daqd.trender.num_trend_channels].data_type = _32bit_integer;
				break;
			  case 3: // `mean'
			  case 4: // `rms'
				daqd.trender.trend_channels [daqd.trender.num_trend_channels].data_type = _64bit_double;
				break;
		        }

			daqd.trender.trend_channels [daqd.trender.num_trend_channels].bps
				= daqd.trender.bps [daqd.trender.channels [i].data_type] [j];
			daqd.trender.trend_channels [daqd.trender.num_trend_channels].bytes
				= daqd.trender.trend_channels [daqd.trender.num_trend_channels].bps 
				  * daqd.trender.trend_channels [daqd.trender.num_trend_channels].sample_rate;
			daqd.trender.trend_channels [daqd.trender.num_trend_channels].offset = daqd.trender.toffs [j] + offs;
			daqd.trender.num_trend_channels++;
		}
		offs += sizeof (trend_block_t);
	}

	DEBUG1(cerr << "Configured " << daqd.num_channels << " channels" << endl);
	DEBUG1(cerr << "Configured " << daqd.trender.num_channels << " trend channels" << endl);
	DEBUG1(cerr << "Configured " << daqd.trender.num_trend_channels << " output trend channels" << endl);
	DEBUG1(cerr << "comm.y: daqd block_size=" << daqd.block_size << endl);

	if (((my_lexer *)lexer) -> strict)
		*yyout << S_DAQD_OK << flush;
}
;

ConfigureChannelsBodyLines: /* Nothing */
	| ConfigureChannelsBodyLine  ConfigureChannelsBodyLines
	;

// net-producer:                  type   dcuid  name           rate             active offset group
// scramnet/dummy:                type          name           rate   precision active trend  group
//
ConfigureChannelsBodyLine: INTNUM INTNUM INTNUM TextExpression INTNUM INTNUM    INTNUM INTNUM INTNUM {

	free ($4);	
}

/*

ConfigureNetWriterBody: BEGIN_BEGIN { daqd.net_writer.num_channels = daqd.net_writer.block_size = 0; } ConfigureNetWriterBodyLines END {
		int errc;
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();

		if (((my_lexer *)lexer) -> strict) {
			if (errc = ((my_lexer *)lexer) -> error_code)
				*yyout << setw(4) << setfill ('0') << hex << errc << dec << flush;
			else
				*yyout << S_DAQD_OK << flush;
		} else
		  *yyout << "configured " << daqd.net_writer.num_channels << endl << flush;
	}
	;

ConfigureNetWriterBodyLines:
	| ConfigureNetWriterBodyLine  ConfigureNetWriterBodyLines
	;

*/
/*
ChannelNames: ALL OptionalIntnum { $$ = $2 > 0? $2: 1; }
	| '{' DistinctChannelNames '}'  { $$ = 0;}
        ;
*/

ChannelNames: ALL  { $$ = 1; }
	| '{' DistinctChannelNames '}'  { $$ = 0;}
	;

ChannelNames1: { $$ = 1; }
	| '{' DistinctChannelNames '}'  { $$ = 0;}
	;


DistinctChannelNames:
	| ChannelName DistinctChannelNames

ChannelName: TextExpression OptionalIntnum {
		ostream *yyout = ((my_lexer *)lexer)->get_yyout ();
		int trend_channels =((my_lexer *) lexer) -> trend_channels;
		channel_t *channels_ptr = trend_channels? daqd.trender.trend_channels: daqd.channels;
		int chnum =  trend_channels? daqd.trender.num_trend_channels: daqd.num_channels;

		// Look this channel up by name
		int chidx = -1;
		for (int i = 0; i < chnum; i++)
		    if (! strcmp (channels_ptr [i].name, $1)) {
			  chidx = i;
			  int req_rate = $2;
			  // Requested rate must lie between 1 and `sample_rate' and should be a power of `2'
			  if (req_rate != 0) {
				if (req_rate < 0 || req_rate > channels_ptr [chidx].sample_rate || ! daqd_c::power_of (req_rate, 2)) {
					((my_lexer *)lexer) -> error_code = DAQD_INVALID_CHANNEL_DATA_RATE;
					if (!((my_lexer *)lexer) -> strict)
						*yyout << "Invalid rate `" << req_rate <<"' given for channel `" << $1 << "'" << endl;
				}
			  }
			  if (((my_lexer *) lexer) -> num_channels < my_lexer::max_channels) {
			    ((my_lexer *) lexer) -> channels [((my_lexer *) lexer) -> num_channels] = channels_ptr [chidx];
			    ((my_lexer *) lexer) -> channels [((my_lexer *) lexer) -> num_channels].seq_num = chidx;
			    ((my_lexer *) lexer) -> channels [((my_lexer *) lexer) -> num_channels++].req_rate = req_rate;
			  } else {
			    ((my_lexer *)lexer) -> error_code = DAQD_TOO_MANY_CHANNELS;			    
			    if (!((my_lexer *)lexer) -> strict)
			      *yyout << "Too many channel names in the request" << endl;
			  }
			  break;
		    }

		if (chidx < 0) {
			int fini = 0;
			// Try to find this channel in the archives
			for (s_link *clink = daqd.archive.first (); clink; clink = clink -> next ()) {
		    		archive_c *a = (archive_c *) clink;
		    		a -> lock();
		    
		    		for (unsigned int i = 0; i < a -> nchannels; i++) {
				  int len = strlen (a -> channels [i].name);
				  if (! strncmp ($1, a -> channels [i].name,  len)) {
					int full_res = 0;
				        int j;

				    	// See if name matches exactly (in case this is not
				        // a trend channel)
				        if (!strcmp($1, a -> channels [i].name)) {
					   full_res = 1;
				        } else {
					   // Check channel name trend-suffix
					   for (j = 0; j < trender_c::num_trend_suffixes; j++)
						if (!strcmp($1 + len, daqd.trender.sufxs[j]))
							break;
					   if (j == trender_c::num_trend_suffixes)
						continue; // check next channel
					}
					chidx = i;

					long_channel_t *chan = ((my_lexer *) lexer) -> channels + ((my_lexer *) lexer) -> num_channels;
					memset(chan, 0, sizeof(long_channel_t));
					strncpy(chan->name, $1, long_channel_t::channel_name_max_len);
					chan -> name [long_channel_t::channel_name_max_len-1] = 0;
					chan -> seq_num = chidx;
					chan -> group_num = long_channel_t::arc_groupn;
					// Store archive memory address so it can be easily found
					// NOTE: this address will need to be checked in the archive list before its usage begins
					chan -> id = (void *) a;
					
					if (full_res) {
						chan -> data_type = a -> channels [i].type;
						chan -> sample_rate = a -> channels [i].rate;
						chan -> req_rate = $2? $2: a -> channels [i].rate;
					} else {
					  // Determine and assign the data type
					  switch (j) {
			  			case 0:
			  			case 1:
      							if (a -> channels [i].type != _64bit_double
							    && a -> channels [i].type != _32bit_float)
			  					chan -> data_type = _32bit_integer;
							else
			  					chan -> data_type = a -> channels [i].type;
							break;
			  			case 2: // `n'
							chan -> data_type = _32bit_integer;
							break;
			  			case 3: // `mean'
			  			case 4: // `rms'
							chan -> data_type = _64bit_double;
							break;
		        		  }
					}

					((my_lexer *) lexer) -> num_channels++;
					((my_lexer *) lexer) -> n_archive_channel_names++;

					fini = 1;
					break;
				  }
				}
				a -> unlock();
				if (fini) break;
			}
		}

		// Channel was not found, bad channel name specified
		if (chidx < 0) {
			((my_lexer *)lexer) -> error_code = DAQD_INVALID_CHANNEL_NAME;
			if (!((my_lexer *)lexer) -> strict)
				*yyout << "Channel `" << $1 << "' not found" << endl;
		}
		free ($1);
	}


/*
//
//                          writer name    IP             port   <- channels config will be here
//
ConfigureNetWriterBodyLine: TextExpression TextExpression INTNUM ChannelNames{
		if ((daqd.net_writer.srvr_addr.sin_addr.s_addr = inet_addr ($2)) <= 0) {
			if (((my_lexer *)lexer) -> strict) {
			        ((my_lexer *)lexer) -> error_code = DAQD_INVALID_IP_ADDRESS;
			} else {
				*((my_lexer *)lexer)->get_yyout () << "invalid IP address" << endl;
			}
		} else {
			daqd.net_writer.srvr_addr.sin_family = AF_INET;
			daqd.net_writer.srvr_addr.sin_port = htons ($3);

			// calculate block size
			for (int i = 0; i < daqd.net_writer.num_channels; i++)
				daqd.net_writer.block_size += daqd.net_writer.channels [i].bytes;
		}

		free ($1);
		free ($2);
	}
*/

OptionalTextExpression: /* nothing */ { $$ = 0; }
	| TextExpression
	;

TextExpression:    TEXT
		 | TOKREF {
                        $$ = $1;
			/*
			  $$ = strdup (get_token_value ($1));
			  free ($1);
			*/
		   }
		 | CAT '(' TextExpression ',' TextExpression ')'
			{
			  $$ = strcat (strcpy ((char *) malloc (strlen ($3) + strlen ($5) + 1), $3), $5);
			  free ($3); free ($5);
			}

		  /* 
			substr (string, start [, count]) clips out a piece of a
			string beginning at `start' and going for `count' characters.
			If `count' is not specified, the string is clipped from `start'
			and goes to the end of the string.
		  */

		 | SUBSTR '(' TextExpression ',' INTNUM ')'
			{
			  if ($5 <= 0)
			    yyerror ("substr(): invalid second argument value");

			  $5--;
			  if (strlen ($3) > $5)
			    $$ = strdup ($3 + $5);
			  else
			    $$ = strdup ("");
			  free ($3);
			}
		 | SUBSTR '(' TextExpression ',' INTNUM ',' INTNUM ')'
			{
			  int slen = strlen ($3);

			  if ($5 <= 0)
			    yyerror ("substr(): invalid second argument value");

			  if ($7 <= 0)
			    yyerror ("substr(): invalid third argument value");

			  if ($7 < $5)
			    yyerror ("substr(): arguments are invalid");

			  $5--;
			  $7;
			  if (slen > $5)
			    if (slen > $7)
			      {	
				$3 [$7] = '\000';
				$$ = strdup ($3 + $5);
			      }
			    else
			      $$ = strdup ($3 + $5);
			  else
			    $$ = strdup ("");
			  free ($3);
			}
		 /* 
		  Strip all fluff, leave only digits in the number
		 */
                 | MKNUMBER '(' TextExpression ')'
			{
			  char *s;
			  char *d;

			  $$ = (char *) malloc (strlen ($3) + 1);
			  for (s = $3, d = $$; *s; s++)
#if __GNUC__ >= 3
			    if (std::isdigit(*s))
#else
                            if (isdigit(*s))
#endif
			      *d++ = *s;
			  *d = '\000';
			}
/*
  To do: make decode() to support variable number of argument conditions, like Oracle does
  */

                 | DECODE '(' TextExpression ',' TextExpression ',' TextExpression ',' TextExpression ')'
			{ $$ = ! strcmp ($3, $5)? $7: $9; }
                 ;

allOrNothing: {  $$=0;} /* Nothing */
	| ALL { $$=1;}
	;
%%



#ifdef COMM_MAIN
int
main (argc, argv)
     int argc;
     char *argv [];
{

  if (argc < 2)
    yyin = stdin;
  else if (! (yyin = fopen (argv [1], "r")))
    {
      fprintf (stderr, "cant' open %s; errno=%d\n", argv [1], errno);
      yyin = stdin;
    }
  rewind (yyin);

  while (1) {
    printf ("initializing\n");
    yyparse ();
  }

  fclose (yyin);
  exit (0);
}

#endif

void
print_block_stats (ostream *yyout, circ_buffer_t *cb)
{
  int i;

  assert (cb);
  *yyout << "puts=" << cb -> puts << endl;
  *yyout << "drops=" << cb -> drops << endl;
  *yyout << "blocks=" << cb -> blocks << endl;
  *yyout << "block_size=" << cb -> block_size << endl;
  *yyout << "producers=" << cb -> producers << endl;
  *yyout << "consumers=" << cb -> consumers << endl;
  *yyout << "transient_consumers=" << cb -> transient_consumers << endl;
  *yyout << "fast_consumers=" << cb -> fast_consumers << endl;
  *yyout << "cmask=0x" << setw(4) << setfill ('0') << hex << cb -> cmask << dec << endl;
  *yyout << "tcmask=0x" << setw(4) << setfill ('0') << hex << cb -> tcmask << dec << endl;
  *yyout << "cmask16th=0x" << setw(4) << setfill ('0') << hex << cb -> cmask16th << dec << endl;
  *yyout << dec << "next_block_in=" << cb -> next_block_in << endl;
  *yyout << dec << "next_block_in_16th=" << cb -> next_block_in_16th << endl;

  *yyout << "consumer\t|next_block_out\t|next_block_out_16th" << endl;
  for (i = 0; i < MAX_CONSUMERS; i++)
    if (cb -> cmask & 1 << i)
      *yyout << i << "\t\t|" << cb -> next_block_out [i] << "\t\t|" << cb -> next_block_out_16th [i] << endl;

  //  yyout -> flush ();
}

void
print_command_help (ostream *yyout)
{
	char *lines[] = {

"User commands:",
"help;                           - get these messages",
"password \"pswd\";                - try to become 'superuser'",
"echo \"Hi!\";                     - echo parameter string",
"version;                        - get comm protocol version",
"revision;                       - get comm protocol revision",
"status channels;                - see channel configuration",
"status input trend channels;    - see input trend channel config",
"status trend channels;          - see output trend channels config",
"status channel-groups;          - see channel group configuration",
"quit;                           - disconnect",
"start trend [avg_period] net-writer [\"IP:port\"] [GPS] [period]",
"      all | { \"chname\" [chrate] ... };",
"                                - start trend network writer",
"start net-writer [\"IP:port\"] [GPS] [period]",
"      all | { \"chname\" [chrate] ... };",
"                                - start network writer",
"kill net-writer ID;             - kill certain network writer",
"gps;                            - get GPS time",
"---",
"Privileged commands:",
"set thread_stack_size = 1024;   - set threads stack size to 1024K",
"set password = \"pswd\";          - change the password",
"set log = 1;                    - set logging level",
"set debug = 10;                 - set debugging level",
"set num_dirs = 8;               - set the number of full frame data directories",
"set frame_dir = \"/spa1/Data\", \"C1-\", \".F\";",
"                                - set full frame data directories filesystem path, file prefix and suffix",
"set frames_per_dir = 60;        - set the number of frames saved in a data directory",
"scan frames;                    - scan full frame directories",
"set trend_num_dirs = 1;         - set the number of trend frame data directories",
"set trend_frame_dir = \"/foo\", \"C1-\", \".T\";",
"                                - set trend frame data directories filesystem path file prefix and suffix",
"set trend_frames_per_dir = 60;  - set the number of trend frames saved in a single data directory",
"scan trend-frames;              - scan trend frame directories",
"set minute_trend_num_dirs = 1;  - set the number of minute trend frames data directories",
"set minute_trend_frame_dir = \"/foo\", \"C1-\", \".T\";",
"                                - set trend frame data directories filesystem path file prefix and suffix",
"set minute_trend_frames_per_dir = 60;",
"                                - set the number of minute trend frame files per data directory",
"scan minute_trend_frames;       - scan minute trend frame directories",
"shutdown;                       - kill the server gracefully",
"status main;                    - debugging printout of the state of main circular buffer",
"status trend;                   - debugging printout of the stats of trend circular buffer",
"status net-writer;              - get debug info about all network writers",
"                                  NOTE: data races with net-writer disconnect",
"                                  could cause server crash on this command",
"status main filesys;            - get data from the main filesystem map",
"status trend filesys;           - data from the trend writer filesystem map",
"status minute-trend filesys;    - data from the minute trend writer filesystem map",
"blocks main;                    - get debugging data about the blocks in main circular buffer",
"start main 16;                  - contruct main circular buffer; 16 blocks",
"start producer;                 - start main circular buffer producer",
"start frame-saver;              - start full frame saver thread",
"sync frame-saver;               - synchronize with the frame saver startup",
"enable frame-wiper;             - enable frame directory cleaning",
"disable frame-wiper;            - disable frame directory cleaning",
"start trend;                    - start trend calculation treads",
"sync trend;                     - synchronize with the trender thread",
"start trend-frame-saver;        - start saving trend frames",
"sync trend-frame-saver;         - synchronize with the trend frame saver thread startup",
"enable trend-frame-wiper;       - enable trend frame directory cleaning",
"disable trend-frame-wiper;      - disable trend frame directory cleaning",
"enabled offline;                - enable off-line data request",
"disable offline;                - disable off-line data request",
"start listener 8088 1;          - start strict network listener on port 8088",
"start listener 8087;            - start network listener on port 8087",
"kill trend;                     - kill all trend calculation threads",
"configure channels begin",
"  1 1 1 \"Name\" rate active trend group...",
"end;                            - configure data channels",
"configure channel-groups begin",
"  N  \"group name\" ...",
"end;                            - configure data channel groups",
"start profiler;                 - creates a thread to gather main circular buffer stats",
"status profiler;                - check main circular buffer stats, gathered with the profiler thread",
"start trend profiler;           - creates a thread to gather trend circular buffer buffer stats",
"status trend profiler;          - check the stats on the trend circular buffer, gathered with the trend profiler thread",
"set profiling_period = N;       - set an interval for profiler threads checking",
"set profiling_core_dump;        - abort the process and dump core if full buffer detected by a profiler",
"process_lock;                   - do mlockall(MCL_CURRENT), to disable process image swapping",
"process_unlock;                 - unlock process pages from memory",
"set sweptsine_filename = \"fname\";",
"                                - set sweptsine data filename",
"set filesys_cb_blocks = 10;     - enable filename net-writer command by constructing a filename circular buffer in main filesystem map",

"---",
"Miscellaneous commands (debugging mostly, other oddities and things):",
"uplwp;                          - do thr_setconcurrency (sc=thr_getconcurrency ()+1), which ups thread concurrency",
"psrinfo;                        - display system processor status, similar to psrinfo command",
"set trend_ascii_output = 1;     - save trend data into the text file instead of creating trend frames",
"disable offline;                - disable offline data net-writer request",
"enable offline;                 - enable offline data net-writer request",
"sleep 10;                       - sleep for ten seconds",
"pscan frames;                   - not entirely finished yet parallelized filesystem scan",
"pscan trend_frames;             - not entirely finished yet parallelized trend filesystem scan",
"abort;                          - dump core and die",
0
        };

	for (int i = 0; lines [i]; i++) {
		*yyout << lines [i] << endl;
	}
}
