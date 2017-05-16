#include <unistd.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "../drv/crc.c"
#include <zmq.h>
#include <assert.h>
#include "zmq_daq.h"

static volatile int keepRunning = 1;
void intHandler(int dummy) {
        keepRunning = 0;
}

int readinifile(char *filename,int chcount,channel_t ndsdata[])
{
	int lft = 0;
	int ii;
	FILE *fr;
	char line[80];
	char tmpname[60];
	int tmpdatarate = 0;
	int totalchans = chcount;
	int totalrate = 0;
	int epicstotal = 0;

	fr = fopen(filename,"r");
	if(fr == NULL) return(-1);
	while(fgets(line,80,fr) != NULL) {
		if(strstr(line,"X2") != NULL && strstr(line,"#") == NULL) { 
			int sl = strlen(line) - 2;
			memmove(line, line+1, sl);
			line[sl-1] = 0;
			// printf("%s\n",line);
			sprintf(tmpname,"%s",line);
			lft = 1;
		}
		if(strstr(line,"datarate") != NULL && strstr(line,"#") == NULL && lft) { 
			if(strstr(line,"16") != NULL && strstr(line,"16384") == NULL)  {
				tmpdatarate = 16;
				epicstotal ++;
			}
			if(strstr(line,"64") != NULL) tmpdatarate = 64;
			if(strstr(line,"128") != NULL) tmpdatarate = 128;
			if(strstr(line,"256") != NULL) tmpdatarate = 256;
			if(strstr(line,"512") != NULL) tmpdatarate = 512;
			if(strstr(line,"1024") != NULL) tmpdatarate = 1024;
			if(strstr(line,"2048") != NULL) tmpdatarate = 2048;
			if(strstr(line,"4096") != NULL) tmpdatarate = 4096;
			if(strstr(line,"8192") != NULL) tmpdatarate = 8192;
			if(strstr(line,"16384") != NULL) tmpdatarate = 16384;
			if(strstr(line,"32768") != NULL) tmpdatarate = 32768;
			if(strstr(line,"65536") != NULL) tmpdatarate = 65536;

		}
		if(strstr(line,"datatype") != NULL && strstr(line,"#") == NULL && lft) { 
			if(strstr(line,"2") != NULL)  {
				ndsdata[totalchans].type = 2;
				ndsdata[totalchans].datasize = (4 * tmpdatarate) /16;;

				totalrate += (4 * tmpdatarate);
			}
			if(strstr(line,"4") != NULL)  {
				ndsdata[totalchans].type = 4;
				ndsdata[totalchans].datasize = (4 * tmpdatarate) /16;;
				totalrate += (4 * tmpdatarate);
			}
			if(strstr(line,"5") != NULL)  {
				ndsdata[totalchans].type = 5;
				ndsdata[totalchans].datasize = (8 * tmpdatarate) /16;;
				totalrate += (8 * tmpdatarate);
			}
			if(strstr(line,"7") != NULL)  {
				ndsdata[totalchans].type = 7;
				ndsdata[totalchans].datasize = (4 * tmpdatarate) /16;;
				totalrate += (4 * tmpdatarate);
			}
			sprintf(ndsdata[totalchans].name,"%s",tmpname);
			ndsdata[totalchans].datarate = tmpdatarate;
			totalchans ++;
			lft = 0;
		}
	}
	for(ii = 0;ii<totalchans;ii++) 
		printf("%s\t%d\t%d\t%d\n",ndsdata[ii].name,ndsdata[ii].type,ndsdata[ii].datarate,
						ndsdata[ii].datasize);
	int fastchans = totalchans - epicstotal;
	printf("Total chans = %d\nTotal Epics = %d\nTotalrate = %d\n",fastchans,epicstotal,totalrate);
	return(totalchans);
}
void
usage()
{
        fprintf(stderr, "Usage: zmq_multi_rcvr [args] -s server name\n");
        fprintf(stderr, "-l filename - log file name\n");
        fprintf(stderr, "-s - server name eg x1lsc0, x1susex, etc.\n");
        fprintf(stderr, "-v - verbose prints cpu_meter test data\n");
        fprintf(stderr, "-h - help\n");
}

int main(int argc, char **argv)
{

char *sysname;
char *modname;
char *sname[20];
extern char *optarg;
sysname = NULL;
modname = NULL;
channel_t mydata[8000];
int c;
int ii;
char filename[256];
char basedir[128];
int num_chans = 0;

daq_multi_dcu_data_t mxDataBlock;
char *daqbuffer = (char *)&mxDataBlock;
char msgbuffer[10000];
nds_data_t ndsbuffer;
char *ndsptr = (char *) &ndsbuffer;

void *daq_context;
void *daq_subscriber;
void *nds_context;
void *nds_publisher;
int rc;
int size;
zmq_msg_t message;

sprintf(basedir,"%s","/opt/rtcds/tst/x2/chans/daq/");

while ((c = getopt(argc, argv, "hd:s:l:d:Vvw:x")) != EOF) switch(c) {
        case 's':
               sysname = optarg;
	       printf("sysname = %s\n",sysname);
               break;
        case 'd':
               modname = optarg;
	       printf("modname = %s\n",modname);
               break;
        case 'h':
        default:
               usage();
               exit(1);
       }

	int nsys = 1;
       if (sysname == NULL || modname == NULL) { usage(); exit(1); }

       for(ii=0;modname[ii] != '\0';ii++) {
       	if(islower(modname[ii])) modname[ii] = toupper(modname[ii]);
       }

	sname[0] = strtok(modname, " ");
        for(;;) {
                printf("%s\n", sname[nsys - 1]);
                char *s = strtok(0, " ");
                if (!s) break;
                sname[nsys] = s;
                nsys++;
        }

													           printf("nsys = %d\n",nsys);
        for(ii=0;ii<nsys;ii++) {
        	printf("sys %d = %s\n",ii,sname[ii]);
	       sprintf(filename,"%s%s%s",basedir,sname[ii],".ini");
	       printf("reading %s\n",filename);
	       num_chans += readinifile(filename,num_chans,mydata);
        }


       signal(SIGINT,intHandler);



       // Set up to rcv
       daq_context = zmq_ctx_new();
       daq_subscriber = zmq_socket (daq_context,ZMQ_SUB);
       char loc[32];
       sprintf(loc,"%s%s%s%d","tcp://",sysname,":",DAQ_DATA_PORT);
       rc = zmq_connect (daq_subscriber, loc);
       assert (rc == 0);
       rc = zmq_setsockopt(daq_subscriber,ZMQ_SUBSCRIBE,"",0);
       assert (rc == 0);
       printf("Rcv data on %s\n",loc);

	nds_context = zmq_ctx_new();
        nds_publisher = zmq_socket (nds_context,ZMQ_PUB);
        sprintf(loc,"%s%d","tcp://eth2:",DAQ_DATA_PORT);
        rc = zmq_bind (nds_publisher,loc);
        assert (rc == 0);
        printf("send data on %s\n",loc);


	do {
		// Initialize 0MQ message buffer
		zmq_msg_init(&message);
		// Get data when message size > 0
                size = zmq_msg_recv(&message,daq_subscriber,0);
                if(size >= 0) {
			// Get pointer to message data
			char *string = (char *)zmq_msg_data(&message);
			// Copy data out of 0MQ message buffer to local memory buffer
			memcpy(daqbuffer,string,size);
			// Destroy the received message buffer
			zmq_msg_close(&message);
			// printf("message rcvd\n");
			char *dptr = (char *)&mxDataBlock.zmqDataBlock[0];
			for(ii=0;ii<num_chans;ii++) {
				sprintf(ndsbuffer.ndschan.name,"%s",mydata[ii].name);
				ndsbuffer.ndschan.type = mydata[ii].type;
				ndsbuffer.ndschan.datarate = mydata[ii].datarate;
				ndsbuffer.ndschan.datasize = mydata[ii].datasize;
				char *ndptr = (char *)&ndsbuffer.ndsdata[0];
				memcpy(ndptr,dptr,mydata[ii].datasize);
				dptr += mydata[ii].datasize;
				int xsize = sizeof(channel_t) + mydata[ii].datasize;
				memcpy(msgbuffer,ndsptr,xsize);
				zmq_send(nds_publisher,msgbuffer,xsize,0);
			}
		}
	}while(keepRunning);

	printf("closing out zmq\n");
	zmq_close(daq_subscriber);
        zmq_ctx_destroy(daq_context);
        zmq_close(nds_publisher);
        zmq_ctx_destroy(nds_context);


       return(0);


}
