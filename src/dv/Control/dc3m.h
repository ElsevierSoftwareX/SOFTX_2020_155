
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>
#include <stdio.h>
#include <signal.h>

#include "../Th/datasrv.h"

#define MSQSIZE 64
#define MAX_FILELEN 1024
/*#define MAX_FILENUM 1024*/
#define GRAPHTIME       0
#define GRAPHTREND      1
#define GMODESTAND	0
#define GMODEMULTI	1
#define MMEXIT          0
#define MMNEW           1
#define MMSAVE          2
#define MMRESTORE       3
#define MMSIG           4
#define MMLONG          5
#define MMPRINT         6
#define MMCONF          7
#define MMFRAME         8
#define MMTOC           9
#define MMMAKETOC       10
#define SFAST           1
#define SSLOW           0
#define SDMT            2
#define SOBS            3

struct mymsgbuf {
        long mtype;
        char mtext[MSQSIZE];
};
struct mymsgbuf mb;

struct mytime {
	int days;
	int hours;
	int mins;
	int secs;
};
struct mytime totalPlay;

int  startyr,startmo,startda,starthr,startmn,startsc;
long startgps=0, durgps=0, fgps0, fgps1, fgpsdur;
int  startstop = 1; /* stoptime-1, starttime-0 */

int msqid;
char *args[4];
int fproc,fproc1;

int mmSelect;
int chSelected=0, windowNum=0, chanTrig=1;
int graphOption=GRAPHTIME, graphMode=GMODESTAND; 
int resolution=1, /* time 128 */ refreshrate=1;
int trigHow=0; 
double trigLev=0.0;  
int chMarked[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int xyType[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, tempItem;
int eunit[16] =  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int autoon[16] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
int gColor[16] = {2,3,4,5,6,8,9,10,11,12,13,14,15,2,3,4};
int g0Color[16] = {2,3,4,5,6,8,9,10,11,12,13,14,15,2,3,4};
char colortext[17][24] = 
{"White","Black","Red","Green","Blue","Yellow","Brown","Gray","Violet","Cyan","Magenta","Orange","Indigo","Maroon","Turquoise","Darkgreen","Default"};


char xAxisScale[16][16] = {"8","8","8","8","8","8","8","8","8","8","8","8","8","8","8","8"};
char xAxisDelay[16][16] = {"0","0","0","0","0","0","0","0","0","0","0","0","0","0","0","0"};
/*char yAxisMin[16][16] = {"-4000","-4000","-4000","-4000","-4000","-4000","-4000","-4000","-4000","-4000","-4000","-4000","-4000","-4000","-4000","-4000"};
  char yAxisMax[16][16] = {"4000","4000","4000","4000","4000","4000","4000","4000","4000","4000","4000","4000","4000","4000","4000","4000"};*/
char yAxisMin[16][16] = {"-5.00","-5.00","-5.00","-5.00","-5.00","-5.00","-5.00","-5.00","-5.00","-5.00","-5.00","-5.00","-5.00","-5.00","-5.00","-5.00"};
char yAxisMax[16][16] = {"5.00","5.00","5.00","5.00","5.00","5.00","5.00","5.00","5.00","5.00","5.00","5.00","5.00","5.00","5.00","5.00"};
char xShowText[16];
char xUnit[5] = {" Sec"};
char xyText[4][24] = {"Lin","Log","Ln","Exp"};
char typetext[2][24] = {"Full", "Trend"};
char modetext[2][24] = {"Standard", "Multiple"};
char chChange[3] = "1";
int  globOn=1, textchange=0; /* used for textfields */

int  saveCalled=0;
char *saveFileName;
char currentDir[MAX_FILELEN];
char dirListItem[MAX_CHANNELS+100][MAX_FILELEN], fileListItem[MAX_CHANNELS+100][MAX_FILELEN];
int  ndirItem, nfileItem, nseldir=-1;
int  xml_only=1;

int  totalchan;
int  topTotal, openTop[MAX_CHANNEL_GROUPS], /* #-number in list, 0-close */
     openSec[MAX_CHANNEL_GROUPS]; 
int  totalgroup, selGroup, selSig, sigCounter;
char topGroup[MAX_CHANNEL_GROUPS][MAX_LONG_CHANNEL_NAME_LENGTH+1];
char secGroup[MAX_CHANNEL_GROUPS][MAX_LONG_CHANNEL_NAME_LENGTH+1];
/*char allGroup[MAX_CHANNEL_GROUPS][MAX_LONG_CHANNEL_NAME_LENGTH+1];*/
char groupList[2*MAX_CHANNEL_GROUPS][MAX_LONG_CHANNEL_NAME_LENGTH+10];
char sigListItem[MAX_CHANNELS+1][MAX_LONG_CHANNEL_NAME_LENGTH+80];
XmString sigListItemString[MAX_CHANNELS+1];
int  sigListRate[MAX_CHANNELS+1];
short showSig = 0, firstTime = 1, fastslow = SFAST;
short xaxisformat = 0, multiple = 0, linestyle = 0, decimation, first = 1;
char unitsel[80];

/* trigger long playback  */
int chanTrig1=1, trigHow1=0, chanTrig2=1, trigHow2=0, trigOp=0;
double trigLev1=0.0, trigLev2=0.0;  

/* drag - sig selection panel */
int drop_ch;

char serverLongIP[80];
int  serverLongPort, trend = -1;

int debug = 1;
int i;

void Ashowlist();
void AcreatList();
void sortArray1(char listArray[MAX_CHANNELS+100][MAX_FILELEN], int arraySize);
void sortArray(char listArray[MAX_CHANNEL_GROUPS][MAX_LONG_CHANNEL_NAME_LENGTH+1], int arraySize);
void setString();
void initialSetting(int firsttime);
void get_a_line(FILE* read_file, char s[]);
int  test_substring (char sub[], char s[]);
int  chop_string (char inout[], char sep);
void resetSigList(int abc);
void setDecimation();
void ShowGroupchList(int reload); /* 1-reload all, 2-reload right, 0-none */
void writeChannelset(int reload);
void startXmgr(int play);
void setXtext();
void printMessage(char msg[], int debug);
void showAbout();
void channelMarked();
void restore_File(char restorefilename[]);
void get_save_line(FILE* read_file, char s[]); /* save-restore */
int string_chop(char sout[], char sin[], char a, char b); /* save/restore */
void longDisplay(int trigger);
int signalVarify(int chanNo, char chanV[]);
void TransferProc( Widget w, XtPointer closure, Atom *seltype, Atom *type, XtPointer value, unsigned long *length, int format );
int signalVerify(int chanDrag, char chanV[]);
