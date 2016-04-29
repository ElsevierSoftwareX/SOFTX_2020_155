/// Experimental UDP data receiver
#include "config.h"
#include <unistd.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../include/daqmap.h"
#include "debug.h"
#include "daqd.hh"
#include "framerecv.hh"
#include "udp_rcvr.hh"

extern daqd_c daqd;
extern struct cdsDaqNetGdsTpNum * gdsTpNum[2][DCU_COUNT];

struct daqMXdata {
        struct rmIpcStr mxIpcData;
        cdsDaqNetGdsTpNum mxTpTable;
        char mxDataBlock[DAQ_DCU_BLOCK_SIZE];
};

static const int buf_size = DAQ_DCU_BLOCK_SIZE * 2;

void
receiver_udp(int my_dcu_id) {

      #define BUFLEN 512
      #define PORT 9930
    
     struct sockaddr_in si_me, si_other;
     int s, i;
     char buf[buf_size];

     // Open radio receiver on port 7090
     diag::frameRecv* NDS = new diag::frameRecv(0);
     //NDS->logging(false);
     //if (!NDS->open("225.0.0.1", "192.168.1.0", 7090)) {
     if (!NDS->open("225.0.0.1", "10.12.0.0", 7000 + my_dcu_id)) {
        perror("Multicast receiver open failed.");
        exit(1);
     }
     printf("Multicast receiver opened on port %d\n", 7000 + my_dcu_id);

     for (i=0;;i++) {
	int br;

	unsigned int seq, gps, gps_n;
	char *bufptr = buf;
    	int length = NDS->receive(bufptr, buf_size, &seq, &gps, &gps_n);
    	if (length < 0) {
        	printf("Allocated buffer too small; required %d, size %d\n", -length, buf_size);
        	exit(1);
    	}
    	//printf("%d %d %d %d\n", length, seq, gps, gps_n);


         struct daqMXdata *dataPtr = (struct daqMXdata *) buf;
         int dcu_id = dataPtr->mxIpcData.dcuId;
	 if (dcu_id != my_dcu_id) {

         	printf("Received packet from th foreign dcu_id %d %s:%d bytes=%d, dcu_id=%d\n\n", dcu_id, inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), br, dcu_id);
		continue;
	 }

         int cycle = dataPtr->mxIpcData.cycle;
         int len = dataPtr->mxIpcData.dataBlockSize;

         char *dataSource = (char *)dataPtr->mxDataBlock;
         char *dataDest = (char *)((char *)(directed_receive_buffer[dcu_id]) + buf_size * cycle);

         // Move the block data into the buffer
         memcpy (dataDest, dataSource, len);

         // Assign IPC data
         gmDaqIpc[dcu_id].crc = dataPtr->mxIpcData.crc;
         gmDaqIpc[dcu_id].dcuId = dataPtr->mxIpcData.dcuId;
         gmDaqIpc[dcu_id].bp[cycle].timeSec = dataPtr->mxIpcData.bp[cycle].timeSec;
         gmDaqIpc[dcu_id].bp[cycle].crc = dataPtr->mxIpcData.bp[cycle].crc;
         gmDaqIpc[dcu_id].bp[cycle].cycle = dataPtr->mxIpcData.bp[cycle].cycle;
         gmDaqIpc[dcu_id].dataBlockSize = dataPtr->mxIpcData.dataBlockSize;

         // Assign test points table
         *gdsTpNum[0][dcu_id] = dataPtr->mxTpTable;

         gmDaqIpc[dcu_id].cycle = cycle;
         if (daqd.controller_dcu == dcu_id)  {
              controller_cycle = cycle;
              DEBUG(6, printf("Timing dcu=%d cycle=%d\n", dcu_id, controller_cycle));
         }
    }
    close(s);
}


