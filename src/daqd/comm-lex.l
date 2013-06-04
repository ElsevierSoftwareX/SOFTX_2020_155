%{

#include <iostream>
#include <fstream>
#include <iomanip>
using namespace std;

#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "y.tab.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "circ.h"

#include "circ.hh"
#include "channel.h"
#include "daqc.h"
#include "daqd.hh"
#include "MyLexer.hh"

#if 0
#undef yywrap

extern "C" {
int
yywrap ()
{
  DEBUG1(cerr << "yywrap() called" << endl);
  return 1;
}
}

#endif
%}

%option yylineno
%option noyywrap




%%
"cycle_delay" { return CYCLE_DELAY; }
"symm_gps_offset" { return SYMM_GPS_OFFSET; }
"no_compression" { return NO_COMPRESSION; }
"allow_tpman_connect_fail" { return ALLOW_TPMAN_CONNECT_FAILURE; }
"buffersize" { return BUFFER_SIZE; }
"uptime" { return UPTIME; }
"periodic_main_filesys_scan" { return PERIODIC_MAIN_FILESYS_SCAN; }
"periodic_trend_filesys_scan" { return PERIODIC_TREND_FILESYS_SCAN; }
"avoid_reconnect" { return AVOID_RECONNECT; }
"tp_allow" { return TP_ALLOW; }
"controller_dcu" { return CONTROLLER_DCU; }
"do_directio" { return DO_DIRECTIO; }
"do_fsync" { return DO_FSYNC; }
"gps_leaps" { return GPS_LEAPS; }
"parallel_producers" { return PARALLEL_PRODUCERS; }
"cit_40m" { return CIT_40M; }
"cycle_input" { return CYCLE_INPUT; }
"malloc" { return MALLOC; }
"broadcaster_packet_burst" { return BR_PACKET_BURST; }
"dcu_status_check" { return DCU_STATUS_CHECK; }
"crc" { return CRC_T; }
"server" { return SERVER; }
"epics" { return EPICS; }
"dcu" { return DCU; }
"master_config" { return MASTER_CONFIG; }
"broadcast_config" { return BROADCAST_CONFIG; }
"system" { return SYSTEM; }
"zero_bad_data" { return ZERO_BAD_DATA; }
"update" { return UPDATE; }
"archive" { return ARCHIVE; }
"archives" { return ARCHIVES; }
"add" { return ADD; }
"delete" { return DELETE; }
"conditional_rfm_refresh" { return CONDITIONAL_RFM_REFRESH; }
"rfm_refresh" { return RFM_REFRESH; }
"cksum_fname" { return CKSUM_FNAME; }
"raw_minute_trend_saving_period" { return RAW_MINUTE_TREND_SAVING_PERIOD; }
"full_frames_per_file" { return FULL_FRAMES_PER_FILE; }
"full_frames_blocks_per_frame" { return FULL_FRAMES_BLOCKS_PER_FRAME; }

"detector-name" { return DETECTOR_NAME; }
"detector_name" { return DETECTOR_NAME; }
"detector-prefix" { return DETECTOR_PREFIX; }
"detector_prefix" { return DETECTOR_PREFIX; }
"detector-longitude" { return DETECTOR_LONGITUDE; }
"detector_longitude" { return DETECTOR_LONGITUDE; }
"detector-latitude" { return DETECTOR_LATITUDE; }
"detector_latitude" { return DETECTOR_LATITUDE; }
"detector-elevation" { return DETECTOR_ELEVATION; }
"detector_elevation" { return DETECTOR_ELEVATION; }
"detector-azimuths" { return DETECTOR_AZIMUTHS; }
"detector_azimuths" { return DETECTOR_AZIMUTHS; }
"detector-altitudes" { return DETECTOR_ALTITUDES; }
"detector_altitudes" { return DETECTOR_ALTITUDES; }
"detector-midpoints" { return DETECTOR_MIDPOINTS; }
"detector_midpoints" { return DETECTOR_MIDPOINTS; }

"detector-name1" { return DETECTOR_NAME1; }
"detector_name1" { return DETECTOR_NAME1; }
"detector-prefix1" { return DETECTOR_PREFIX1; }
"detector_prefix1" { return DETECTOR_PREFIX1; }
"detector-longitude1" { return DETECTOR_LONGITUDE1; }
"detector_longitude1" { return DETECTOR_LONGITUDE1; }
"detector-latitude1" { return DETECTOR_LATITUDE1; }
"detector_latitude1" { return DETECTOR_LATITUDE1; }
"detector-elevation1" { return DETECTOR_ELEVATION1; }
"detector_elevation1" { return DETECTOR_ELEVATION1; }
"detector-azimuths1" { return DETECTOR_AZIMUTHS1; }
"detector_azimuths1" { return DETECTOR_AZIMUTHS1; }
"detector-altitudes1" { return DETECTOR_ALTITUDES1; }
"detector_altitudes1" { return DETECTOR_ALTITUDES1; }
"detector-midpoints1" { return DETECTOR_MIDPOINTS1; }
"detector_midpoints1" { return DETECTOR_MIDPOINTS1; }

"nds-jobs-dir" { return NDS_JOBS_DIR; }
"nds_jobs_dir" { return NDS_JOBS_DIR; }
"units" { return UNITS; }
"flock" { return FLOCK; }
"do-scan-frame-reads" { return DO_SCAN_FRAME_READS; }
"do_scan_frame_reads" { return DO_SCAN_FRAME_READS; }
"clear" { return CLEAR;}
"fault" { return FAULT;}
"broadcast" { return BROADCAST; }
"nofilter" { return DOWNSAMPLE; }
"average" { return AVERAGE; }
"ipc-offset" { return IPC_OFFSET; }
"string" { return WORD_STRING; }
"gps" { return WORD_GPS; }
"profiling_period" { return PROFILING_PERIOD; }
"profiling-period" { return PROFILING_PERIOD; }
"profiling_core_dump" { return PROFILING_CORE_DUMP; }
"profiling-core-dump" { return PROFILING_CORE_DUMP; }
"transmission" { return TRANSMISSION; }
"uplwp" { return UPLWP; }
"filesys_cb_blocks"  { return FILESYS_CB_BLOCKS; }
"filesys-cb-blocks"  { return FILESYS_CB_BLOCKS; }
"profiler"  { return PROFILER; }
"sync"  { return SYNC; }
"psrinfo"  { return PSRINFO; }
"process_lock" { return PROCESS_LOCK; }
"process_unlock" { return PROCESS_UNLOCK; }
"minute_trend_frames_per_dir"    { return MINUTE_TREND_FRAMES_PER_DIR; }
"minute-trend-frames-per-dir"    { return MINUTE_TREND_FRAMES_PER_DIR; }
"minute_trend_frame_wiper" { return MINUTE_TREND_FRAME_WIPER; }
"minute-trend-frame-wiper" { return MINUTE_TREND_FRAME_WIPER; }
"minute_trend_num_dirs" { return MINUTE_TREND_NUM_DIRS; }
"minute-trend-num-dirs" { return MINUTE_TREND_NUM_DIRS; }
"minute_trend_frame_dir" { return MINUTE_TREND_FRAME_DIR; }
"minute-trend-frame-dir" { return MINUTE_TREND_FRAME_DIR; }
"raw_minute_trend_dir" { return RAW_MINUTE_TREND_DIR; }
"raw-minute-trend-dir" { return RAW_MINUTE_TREND_DIR; }
"old_raw_minute_trend_dirs" { return OLD_RAW_MINUTE_TREND_DIRS; }
"old-raw-minute-trend-dirs" { return OLD_RAW_MINUTE_TREND_DIRS; }
"minute_trend_frames" { return MINUTE_TREND_FRAMES; }
"minute-trend-frames" { return MINUTE_TREND_FRAMES; }
"minute_trend"  { return MINUTE_TREND; }
"minute-trend"  { return MINUTE_TREND; }
"minute_trender"  { return MINUTE_TREND; }
"minute-trender"  { return MINUTE_TREND; }

"sweptsine_filename"  { return SWEPTSINE_FILENAME; }
"offline"  { return OFFLINE; }
"frames_per_dir"    { return FRAMES_PER_DIR; }
"frames-per-dir"    { return FRAMES_PER_DIR; }
"trend_frames_per_dir"    { return TREND_FRAMES_PER_DIR; }
"trend-frames-per-dir"    { return TREND_FRAMES_PER_DIR; }
"thread_stack_size" { return THREAD_STACK_SIZE; }
"thread-stack-size" { return THREAD_STACK_SIZE; }
"help" { return HELP; }
"password" { return PASSWORD; }
"filesys" { return FILESYS; }
"log" { return LOG_T; }
"debug" { return DEBUG_T; }
"input" { return INPUT; }
"echo" { return ECHO_ECHO; }
"sleep" { return SLEEP; }
"trend_ascii_output" { return TREND_ASCII_OUTPUT; }
"trend-ascii-output" { return TREND_ASCII_OUTPUT; }
"frame_saver" { return FRAME_SAVER; }
"frame-saver" { return FRAME_SAVER; }
"science_frame_saver" { return SCIENCE_FRAME_SAVER; }
"science-frame-saver" { return SCIENCE_FRAME_SAVER; }
"frame_wiper" { return FRAME_WIPER; }
"frame-wiper" { return FRAME_WIPER; }
"minute_trend_frame_saver" { return MINUTE_TREND_FRAME_SAVER; }
"minute-trend-frame-saver" { return MINUTE_TREND_FRAME_SAVER; }
"raw_minute_trend_saver" { return RAW_MINUTE_TREND_SAVER; }
"raw-minute-trend-saver" { return RAW_MINUTE_TREND_SAVER; }
"trend_frame_saver" { return TREND_FRAME_SAVER; }
"trend-frame-saver" { return TREND_FRAME_SAVER; }
"trend_frame_wiper" { return TREND_FRAME_WIPER; }
"trend-frame-wiper" { return TREND_FRAME_WIPER; }
"producer"  { return PRODUCER; }
"trend_num_dirs" { return TREND_NUM_DIRS; }
"trend-num-dirs" { return TREND_NUM_DIRS; }
"trend_frame_dir" { return TREND_FRAME_DIR; }
"trend-frame-dir" { return TREND_FRAME_DIR; }
"frame_dir" { return FRAME_DIR; }
"frame-dir" { return FRAME_DIR; }
"science_frame_dir" { return SCIENCE_FRAME_DIR; }
"science_frame-dir" { return SCIENCE_FRAME_DIR; }
"scan" { return SCAN; }
"pscan" { return PSCAN; }
"frames" { return FRAMES; }
"trend_frames" { return TREND_FRAMES; }
"trend-frames" { return TREND_FRAMES; }
"set" { return SET; }
"num_dirs" { return NUM_DIRS;}
"num-dirs" { return NUM_DIRS;}
"listener" { return LISTENER; }
"version" { return DAQD_VERSION; }
"revision" { return REVISION; }
"all" { return ALL; }
"channels" { return CHANNELS; }
"channel-groups" { return CHANNEL_GROUPS; }
"channel_groups" { return CHANNEL_GROUPS; }
"main" { return MAIN; }
"configure" { return CONFIGURE; }
"begin" { return BEGIN_BEGIN; }
"end" { return END; }
"net-writer"   { return NET_WRITER; }
"net_writer"   { return NET_WRITER; }
"frame-writer"   { return FRAME_WRITER; }
"frame_writer"   { return FRAME_WRITER; }
"fast-writer"   { return FAST_WRITER; }
"fast_writer"   { return FAST_WRITER; }
"name-writer"   { return NAME_WRITER; }
"name_writer"   { return NAME_WRITER; }
"trend"  { return TREND; }
"trender"  { return TREND; }
"start"    { return START; }
"enable"    { return ENABLE; }
"disable"    { return DISABLE; }
"kill"    { return KILL; }
"stop"    { return KILL; }
"blocks"   { return BLOCKS; }
"shutdown"   { return SHUTDOWN; }
"abort"   { return ABORT; }
"status" { return STATUS; }
"stat" { return STATUS; }
"quit" { return QUIT; }
"exit" { return QUIT; }
"tpconfig" { return TPCONFIG; }

-?[0-9]+ { 
         lvalp -> y_int = atoi (yytext);
         return INTNUM;
       }

-?[0-9]*\.[0-9]+ { 
         lvalp -> y_real = atof (yytext);
         return REALNUM;
       }

0x[0-9]+ {
         lvalp -> y_int = strtol (yytext,0,0);
         return INTNUM;
      }

["@]	     {
		char buf [BUFSIZ], *p;
		char terminator;

		terminator = yytext [0];
		for (p = buf; *p = yyinput(); p++)
		  {
		    if (*p == terminator && p == buf)
		      break;
		    if (*p == terminator)
		      if (*(p - 1) != '\\')
			break;
		      else
			*(--p) = terminator;

		    if (*p == EOF) {
		      my_yyerror ("syntax error");
                      if (strict) return 0;
                    }
		  }

		*p = '\0';
		lvalp -> y_str = (char *) malloc (strlen (buf) + 1);
		strcpy (lvalp -> y_str, buf);
		switch (terminator) {
		  case '"':
		    return TEXT;
		  case '@':
		    return TOKREF;
		  default:
		    my_yyerror ("syntax error");
                    if (strict) return 0;
		}
	     }

"/*"                    {
  char ch;

  for(;;) {
    while(1) {
      ch = yyinput();
      if(ch == '*') 
	break;
    }
    if(yyinput() != '/') { unput(yytext[yyleng-1]); }
    else break;
  }
                         

             }

=			{ return '='; }
,			{ return ','; }
;                       { return ';'; }
\{                       { return '{'; }
\}                       { return '}'; }
\n                      {if (prompt) { return ';';}}
[ \t\r]+                ;
"--".*                  ;
"//".*                  ;
"#".*                   ;
\004                    { return QUIT; }
.			{ my_yyerror ("invalid character"); if (strict) return 0; }

%%

int
yyFlexLexer::yylex() { return 0; };






