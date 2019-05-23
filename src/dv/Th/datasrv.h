#ifndef TH_DATASRV_H
#define TH_DATASRV_H

/* datasrv.h  */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <arpa/inet.h>

/*
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <assert.h>
#include <pthread.h>
#include <math.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>
*/
#include "channel.h"
#include "daqc.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DChList {
    char    name[MAX_LONG_CHANNEL_NAME_LENGTH+1]; /* Channel name */
    char    units [MAX_CHANNEL_NAME_LENGTH]; /* Engineering units */
    int     rate; /* Sampling rate */
    int     group_num; /* Channel group number */
    int     data_type; /* Sample data type for the channel */
    int     tpnum;     /* Test point number */
    int     dcu_id;
};

struct DChGroup {
    char    name[MAX_LONG_CHANNEL_NAME_LENGTH+1]; /* Channel group name */
    int     group_num; /* Channel group number */
    int     total_chan; /* Number of channels in the group */
};

struct DTrend {
    double  min;
    double  max;
    double  mean;
};


/** DataConnect: connect to Data server **/
int    DataConnect(char* serverName, int serverPort, int listenerPort, void* read_data());

/** DataSimpleConnect: a simple connection to Data server which will not
    receive data -- used for getting group or channel information **/
int    DataSimpleConnect(char* serverName, int serverPort);

/** DataReConnect: re-connect to Data server **/
int    DataReConnect(char* serverName, int serverPort);

/** DataQuit: quit the connection **/
void   DataQuit();

/** DataChanAdd: input channel name or "all"; data rate (must be power of 2)
    or 0 (default, all data will be sent) 
    returns total channels added or -1 if fails**/
int    DataChanAdd(const char* chName, int dataRate ); 

/** DataChanDelede: input channel name or "all" **/
int    DataChanDelede(const char* chName); 

/** DataWriteRealtime: request on-line data; 
    returns process ID or -1 if fails **/
unsigned long    DataWriteRealtime();

/** DataWriteRealtimeFast: request on-line data in fast pace (16 Hz); 
    returns process ID or -1 if fails **/
unsigned long    DataWriteRealtimeFast();

/** DataWrite: request off-line data;
    Example: DataWrite("98-02-05-14-05-30", 5, 0, 0) requests 5 second of data 
    starting from time 98-02-05-14-05-30;
    returns process ID or -1 if fails **/
unsigned long    DataWrite(char* starttime, int duration, int isgps);

/** DataWriteTrendRealtime: request on-line trend data; 
    returns process ID or -1 if fails **/
unsigned long    DataWriteTrendRealtime();

/** DataWriteTrend: request off-line trend data;
    it sends trend data every trendlength seconds -- currently only 1 or 60
    if starttime= "0" or "00-00-00-00-00-00" it gives off-line
          trend data up to the present time; 
    returns process ID or -1 if fails **/
unsigned long    DataWriteTrend(char* starttime, int duration, int trendlength, int isgps); 

/** DataWriteStop **/
void   DataWriteStop(unsigned long processID);

/** DataTimeNow **/
int    DataTimeNow();


/***** the following three functions should be used in the function 
       read_data  *****/
void   DataReadStart();
int    DataRead();
void   DataReadStop();

/** DataGetCh: get data by channel name; output data[]; and the
    position in the block;
    returns rate of the channel received or -1 if fails **/
int    DataGetCh(const char* chName, double data[], int bpos, int);

/** DataTrendGetCh: get trend data by channel name; output trend;
    returns number of seconds of data received,
        or -1 if fails **/
int    DataTrendGetCh(const char* chName, struct DTrend *trend);

/** DataTrendLength: returns number of seconds of data received
       used in conjunction with DataTrendGetCh **/
int    DataTrendLength();

/** DataGetChSlope: get slop, offset, and status of the given chan
       returns the number of the array elements, should be same as chan num
        or -1 if fails **/       
int    DataGetSlope(const char* chName, float *slope, float *offset, int *status);

/** DataTimeGps: returns the current GPS time in seconds**/
time_t DataTimeGps();

/** DataTimensec: returns the nanoseconds residual **/
time_t DataTimensec();

/** DataTimestamp: get current time in yy-mm-dd-hh-mm-ss format **/
void   DataTimestamp(char* timestamp);

/** DataSequenceNum: returns the block sequence number which indicates any dropped blocks **/
unsigned long DataSequenceNum();

/*** Utility Functions ***/

/** transfer GPS time to UTC time **/
void DataGPStoUTC(long gpsin, char* utcout);

/* transfer UTC time to GPS time  */
long DataUTCtoGPS(char* utctime);
long  DataUTCtoGPS1(int yr, int mo, int da, int hr, int mn, int sc);

/***** the following are information functions  *****/
/** DataChanList: show the list of all configured channels **/
int    DataChanList(struct DChList allChan[], size_t dest_size);

/** DataGroup: get channel group information
    returns number of channel groups or -1 if fails **/
int    DataGroup(struct DChGroup allGroup[]);

/** DataChanRate: returns the sample rate of the channel or -1 if fails **/
int    DataChanRate(const char* chName);

/** DataChanType: returns the sample type of the channel 
      1(16bit integer), 2(32bit integer), 3(64bit integer),
      4(32bit float),   5(64bit double),  6(32byte_string)  
    or error:
      0:  cann't find the channel
      -1: trend data is not available for the channel **/
int    DataChanType(const char* chName);

#ifdef __cplusplus
}
#endif

#endif
