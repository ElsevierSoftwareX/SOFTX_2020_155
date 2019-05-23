/*----------------------------------------------------------------------*/
/*                                                                      */
/* Module Name: framer4.h                                               */
/* Compile with                                                         */
/*  gcc -o framer4 framer4.c datasrv.o daqc_access.o                    */
/*     -L/home/hding/Cds/Frame/Lib/Grace -lgrace_np                     */
/*     -L/home/hding/Cds/Frame/Lib/UTC_GPS -ltai                        */
/*     -lm -lnsl -lsocket -lpthread                                     */
/*                                                                      */
/* Module Description: 40m Data Acquisition System                      */
/*                     Data Reader and Displayer.                       */
/*     This code is used in conjunction with DaDsp to read and display  */
/*     data channels acquired by the 40m prototype data acquisition     */
/*     system.                                                          */
/*                                                                      */
/* Module Arguments:                                                    */
/*                                                                      */
/* Revision History:                                                    */
/* Rel   Date    Engineer   Comments                                    */
/* 00    14Mar97 R. Bork    First Release.                              */
/* 01    03Dec97 H. Ding                 .                              */
/* 02    02Mar98 H. Ding    Using Data library for access of data.      */
/* 03    10May00 H. Ding    Version 3.0: cancel A/B, FvsT, Diag, etc.   */
/* 04    29Jan01 H. Ding    Version 4.0: group channels by names        */
/* 05    22May01 H. Ding    Version 5.0: Moving Average Filter          */
/*                                                                      */
/* Documentation References:                                            */
/*      Man Pages:                                                      */
/*      References:                                                     */
/*                                                                      */
/* Author Information:                                                  */
/*      Name          Telephone    Fax          e-mail                  */
/*      Rolf Bork   . (626)3953182 (626)5440424 rolf@ligo.caltech.edu   */
/*      Hongyu Ding . (626)3952042 (626)5440424 hding@ligo.caltech.edu  */
/*                                                                      */
/* Code Compilation and Runtime Specifications:                         */
/*      Code Compiled on: Sun Ultra Enterprise 2 running Solaris2.5.1   */
/*      Compiler Used: sparcworks pro                                   */
/*      Runtime environment: Sun UltraSparc 2200 / Solaris 2.5.1        */
/*                                                                      */
/* Code Standards Conformance:                                          */
/*      Code Conforms to: LIGO standards.       OK                      */
/*                        Lint.                 TBD                     */
/*                        ANSI                  TBD                     */
/*                        POSIX                 TBD                     */
/*                                                                      */
/* Known Bugs, Limitations, Caveats:                                    */
/*                      -------------------                             */
/*                                                                      */
/*                             LIGO                                     */
/*                                                                      */
/*        THE LASER INTERFEROMETER GRAVITATIONAL WAVE OBSERVATORY.      */
/*                                                                      */
/*                     (C) The LIGO Project, 1996.                      */
/*                                                                      */
/*                                                                      */
/* California Institute of Technology                                   */
/* LIGO Project MS 51-33                                                */
/* Pasadena CA 91125                                                    */
/*                                                                      */
/* Massachusetts Institute of Technology                                */
/* LIGO Project MS 20B-145                                              */
/* Cambridge MA 01239                                                   */
/*                                                                      */
/*----------------------------------------------------------------------*/

#include "datasrv.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define FILTER_SIZE 9
#define FILTER_WIDTH 4

#define ADC_BUFF_SIZE 2048 * 4
#define NCF 8192 * 4
#define MSQSIZE 64

#define STOPMODE 0
#define PAUSEMODE 1
#define REALTIME 2

#define GRAPHTIME 0
#define GRAPHTREND 1
#define GMODESTAND 0
#define GMODEMULTI 1

#define TRIGABOVE 0
#define TRIGBELOW 1

#define TRANSLIMIT 200

#define ZEROLOG 1.0e-20
#define BIGNUM 1.0e8
#define XSHIFT 0.04

static float sqrarg;
#define SQR(a) (sqrarg = (a), sqrarg * sqrarg)
#define SWAP(a, b)                                                             \
    tempr = (a);                                                               \
    (a) = (b);                                                                 \
    (b) = tempr
#define WINDOW(j, a, b) (1.0 - SQR((((j)-1) - (a)) * (b))) /*Welch*/
/*#define WINDOW(j,a,b) (1.0-fabs((((j)-1)-(a))*(b)))*/    /*Bartlett*/
#define maxabs(a, b) (fabs(a) > fabs(b) ? fabs(a) : fabs(b))

struct mymsgbuf {
    long mtype;
    char mtext[MSQSIZE];
};
struct mymsgbuf mb;

key_t key;
int msqid;

FILE *fwarning;

int serverPort, userPort = 7000;
char serverIP[80];

int chNameChange[16];

/* 1 -real; 2-imaginary; 3-magnitude; 4-phase */
int complexDataType[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

int chType[16];    /* 1: fast channel I  rate>=256;
                      2: fast channel II rate<=128
                      3: slow channel    rate=1*/
int chSize[16];    /* half of the data sampling rate */
int xyType[16];    /* 0-linear; 1-Log; 2-Ln; 3-exp */
int graphRate[16]; /* actual graph resolution for fast I channels*/
float slope[16], offset[16];
int uniton[16], chstatus[16], autoon[16];
char chName[16][80], chUnit[16][80];
int chMarked[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
struct DTrend trenddata[TRANSLIMIT];
float fastTime[ADC_BUFF_SIZE];
double *chData, chData_1[NCF * 2], chData_0[NCF * 2];
double tempData[16][ADC_BUFF_SIZE];
double fhist[16][FILTER_WIDTH]; /* for filter history */

/* Global */
short newReadMode;
short graphOption, graphMethod, graphOption0, graphMethod0;
char filename[64];
short firsttime, startnew, initGraph = 1, initdata;
int windowNum = 1;
int chSelect0, chSelect; /* 0...15 */
int resolution = 128;
int maxSec = 16;
int refreshRate = 1;
short chSelect_seq;

double winXScaler[16], winScaler[16], winYMin[16], winYMax[16];
int winDelay[16] /* modified */;
float ytick_maj = 5.0, ytick_min = 10.0;
short sigOn = 1, globOn = 0 /* always set to 0 after use */;
short filterOn = 0;

unsigned long processID = 0;
char timestring[24], trendtimestring[24];
int stopped = 1, paused = 0, trailer = 0, reset = 10, printing = -1,
    quitting = 0;
int timeCount = 0, trendCount = 0;

int blocksize = 16, bindex, tempcycle = 3, shellID;
int dPoints, sPoints;
int dataShift = 128, dataShiftS;
int chChange = 1, screenChange = 0;       /* set-graph changing */
short warning, /* for log0 case */ nslow; /* for slow channel */
int nolimit;

/* Trigger */
short trigOn = 0, trigHow = 0, trigApply = 0;
short chanTrig = 1, chanTrig0 = 1; /* 1...16 */
double trigLevel = 0.0;

/* Trend */
char pausefile[80];
FILE *fppause;
int pausefilecn = 0;
short errorpause = 0;

/* Color */
int monthdays[12] = {31, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30};
int gColor[16] = {2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14, 15, 2, 3, 4};
int gColorDefault[16] = {2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14, 15, 2, 3, 4};

/* Message */
char msgout[200];

char version_n[8];  /* version number */
char version_d[12]; /* version date */

void *read_data();
void changeReadMode();
void changeConfigure();
void graphini();
void graphout();
void graphtrend(int from, int to);
void graphmulti();
void graphtrmulti(int from, int to);
void resumePause();

void switchcolor();
void printgraph();
void quitdisplay(int how);
void printmessage(int stat);

void movAvgFilter(double *inData, double *filtData, double *hist, int inrate,
                  int step);
