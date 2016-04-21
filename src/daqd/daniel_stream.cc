//
// Daniel UDP broadcast DAQ data sender
//
// USAGE:./daniel_stream -v  -d scipe12

#include <unistd.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#ifdef NO_RTL
#include "../drv/mbuf/mbuf.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#define USE_UDP 1
#include "drv/fb.h"
#include "daqmap.h"
#include "framesend.hh"
#include <iostream>

#define BUFLEN 512
#define NPACK 10
#define PORT 9930

namespace diag {
int packetBurst = 1;
}

typedef  struct {
    bool   inUse;
    char*  data;
} radio_buffer;

// 192.168.1.255" broadcast="192.168.1.0
int mcast_port = 7000;
//char *mcast_interface = "192.168.1.0";
//char *mcast_addr = "192.168.1.255";
char *mcast_interface = "10.12.0.0";
char *mcast_addr = "10.12.0.255";

          //inet addr:10.11.0.12  Bcast:10.11.0.255  Mask:255.255.255.0

diag::frameSend radio;

static const int radio_buf_len = 512 * 1024;
static const int radio_buf_num = 100;

// Broadcast buffers
radio_buffer radio_bufs [radio_buf_num];

void diep(char *s) {
    perror(s);
    exit(1);
}

int Verify;
int do_verbose;
int num_threads;
volatile int threads_running;
unsigned char *dcu_addr;


void
usage()
{
	fprintf(stderr, "Usage: udp_stream  -s sys_name \n");
	fprintf(stderr, "sys_name - name of a control system model, e.g. isiham6\n");
	//fprintf(stderr, "hostname - destination host name, e.g. dtsdc0\n");
}

struct daqMXdata {
	struct rmIpcStr mxIpcData;
	cdsDaqNetGdsTpNum mxTpTable;
	char mxDataBlock[DAQ_DCU_BLOCK_SIZE];
};
static struct rmIpcStr *shmIpcPtr;
static char *shmDataPtr;
static struct cdsDaqNetGdsTpNum *shmTpTable;
static const int buf_size = DAQ_DCU_BLOCK_SIZE * 2;
static const int header_size = sizeof(struct rmIpcStr) + sizeof(struct cdsDaqNetGdsTpNum);

static inline void
sender(char *hostname)
{
  int cur_req;
  struct daqMXdata mxDataBlock;
  int lastCycle = 0;
  char *dataBuff;
  int sendLength = 0;
  for (;;) {
    // Connect
    struct sockaddr_in si_other;
    int s, i, slen=sizeof(si_other);
    //char buf[BUFLEN];

/*
    if ((s = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
          diep("socket");

#define SRV_IP "10.11.0.3"

    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(PORT);
    if (inet_aton(SRV_IP, &si_other.sin_addr)==0) {
          fprintf(stderr, "inet_aton() failed\n");
          exit(1);
    }

*/
    cur_req = 0;
    usleep(1000);
    for (;;) {
	// Wait for cycle count update from FE
	do {
		usleep(1000);
	} while(shmIpcPtr->cycle == lastCycle);

	mxDataBlock.mxTpTable = shmTpTable[0];

	// Copy values from shmmem to MX buffer
	lastCycle = shmIpcPtr->cycle;
	if (lastCycle == 0) shmIpcPtr->status ^= 1;
	cur_req = (cur_req + 1) % 8;
	mxDataBlock.mxIpcData.cycle = lastCycle;
	mxDataBlock.mxIpcData.crc = shmIpcPtr->crc;
	mxDataBlock.mxIpcData.dcuId = shmIpcPtr->dcuId;
	//printf("offs=%d\n", ((char *)&mxDataBlock) - (char *)&(mxDataBlock.mxIpcData.dcuId));
	mxDataBlock.mxIpcData.dataBlockSize = shmIpcPtr->dataBlockSize;
	mxDataBlock.mxIpcData.bp[lastCycle].timeSec = shmIpcPtr->bp[lastCycle].timeSec;;
	mxDataBlock.mxIpcData.bp[lastCycle].timeNSec = shmIpcPtr->bp[lastCycle].timeNSec;;
	mxDataBlock.mxIpcData.bp[lastCycle].cycle = shmIpcPtr->bp[lastCycle].cycle;;
	mxDataBlock.mxIpcData.bp[lastCycle].crc = shmIpcPtr->bp[lastCycle].crc;;

	dataBuff = (char *)(shmDataPtr + lastCycle * buf_size);
	memcpy((void *)&mxDataBlock.mxDataBlock[0],dataBuff,mxDataBlock.mxIpcData.dataBlockSize);
	sendLength = header_size + mxDataBlock.mxIpcData.dataBlockSize;
 ///printf("send length = %d  total length = %ld\n",sendLength,sizeof(struct daqMXdata));
#if 0
	char *dataBuff;
struct daqMXdata {
	struct rmIpcStr mxIpcData;
	cdsDaqNetGdsTpNum mxTpTable;
	char mxDataBlock[DAQ_DCU_BLOCK_SIZE];
		shmDataPtr = (char *)(dcu_addr + CDS_DAQ_NET_DATA_OFFSET);
};
#endif

	// Send
	//if (sendto(s, &mxDataBlock, sendLength, 0, &si_other, slen)==-1)   diep("sendto()");
	radio_buffer*        buf = 0;

	for (radio_buffer* b = radio_bufs;
             b < radio_bufs + radio_buf_num; b++) {
          if (!radio.isUsed(b->inUse)) {
            buf = b;
            break;
          }
        }

        if (buf == 0) {
          fprintf(stderr, "cannot find unused radio buffer; data was lost");
	  exit(1);
        }

      	if (sendLength > radio_buf_len) {
          fprintf(stderr, "radio buffers too small");
	  exit(1);
        }

	memcpy (buf -> data, &mxDataBlock, sendLength);

	radio.send (buf -> data, sendLength, &buf -> inUse, false, 
			lastCycle, 1);
//	printf("%d\n", lastCycle);
    }
  }
}

int 
main(int argc, char **argv)
{
	char *rem_host;
	char *sysname;
	int len;
	int iter;
	int c;
	extern char *optarg;
	int fd;
	char *shmem_fname_format = "/rtl_mem_%s_daq";
	char shmem_fname[256];
	//int status;


	//printf("%d\n", sizeof(struct daqMXdata));

	/* set up defaults */
	rem_host = NULL;
	sysname = NULL;

	//printf("file = %s %s %s\n",shmem1,shmem2,shmem3);

	while ((c = getopt(argc, argv, "hd:s:")) != EOF) switch(c) {
	case 'd':
		rem_host = optarg;
		break;
	case 's':
		sysname = optarg;
		printf ("sysname = %s\n",sysname);
		break;
	case 'h':
	default:
		usage();
		exit(1);
	}

	if (sysname == NULL) { usage(); exit(1); }
	//if (rem_host == NULL) { usage(); exit(1); }

	// Make it lower case;
	int i;

	printf("System name: %s\n", sysname);
	sprintf(shmem_fname, shmem_fname_format, sysname);

#ifdef NO_RTL
		const int ss = 64*1024*1024;
       		if ((fd = open ("/dev/mbuf", O_RDWR | O_SYNC)) < 0) {
                	fprintf(stderr, "Couldn't open /dev/mbuf read/write\n");
			return 0;
       		}
		struct mbuf_request_struct req;
       		req.size = ss;
       		strcpy(req.name, sysname);
		strcat(req.name, "_daq");
       		ioctl (fd, IOCTL_MBUF_ALLOCATE, &req);
       		ioctl (fd, IOCTL_MBUF_INFO, &req);
#else
		// Open shared memory to FE DAQ
		if ((fd = open(shmem_fname, O_RDWR))<0) {
			fprintf(stderr, "Can't open shmem\n");
			exit(1);
	        }
#endif

		// Map shared memory
		dcu_addr = (unsigned char *)mmap(0, 64*1024*1024-5000, PROT_READ | PROT_WRITE, MAP_SHARED, fd , 0);
		if (dcu_addr <= 0) {
			fprintf(stderr, "Can't map shmem\n");
			exit(1);
		}
		else printf("mapped at 0x%lx\n",(unsigned long)dcu_addr);

		shmIpcPtr = (struct rmIpcStr *)(dcu_addr + CDS_DAQ_NET_IPC_OFFSET);
		shmDataPtr = (char *)(dcu_addr + CDS_DAQ_NET_DATA_OFFSET);
		shmTpTable = (struct cdsDaqNetGdsTpNum *)(dcu_addr + CDS_DAQ_NET_GDS_TP_TABLE_OFFSET);

	len = sizeof(struct daqMXdata);
	printf("send len = %d\n",len);

	printf("my dcu id id %d\n", shmIpcPtr->dcuId);

	// Open multicast port
 	if (!radio.open (mcast_addr, mcast_interface, mcast_port + shmIpcPtr->dcuId)) {
      		fprintf(stderr, "framexmit open failed");
		exit(1);
	}

    	std::cerr << "opened framexmit addr=" << mcast_addr << " iface=" << (mcast_interface? mcast_interface: "") << std::endl;

	// Create broadcast buffers
        for (int i = 0; i < radio_buf_num; i++) {
      		radio_bufs [i].inUse = false;
      		if (! radio_bufs [i].data) {
        	  radio_bufs [i].data = new char [radio_buf_len];
        	  if (radio_bufs [i].data == 0) {
          		fprintf(stderr, "cannot create framexmit buffers");
          		exit(1);
        	  }
      		}
    	}

	sender(rem_host); 
  
	exit(0);
}
