#include <stdio.h>
#include "daqmap.h"
#include "drv/fb.h"

#ifndef NO_DAQ

int cdsNetStatus = 0;

#if !defined(USE_GM)
int cdsDaqNetDrop()
{
  return(0);
}


int cdsDaqNetInit(int fbId)
{
  //printf("cdsDaqNetInit\n");
  return(1);
}

int cdsDaqNetClose()
{
  //printf("cdsDaqNetClose\n");
  return(0);
}

int cdsDaqNetCheckCallback()
{
  //return(expected_callbacks);
  //printf("cdsDaqNetCheckCallback\n");
  return 0;
}

int cdsDaqNetReconnect(int dcuId)
{
  //return(expected_callbacks);
  //printf("cdsDaqNetReconnect\n");
  return 0;
}

int cdsDaqNetCheckReconnect()
{
  //return(eMessage);
  //printf("cdsDaqNetCheckReconnect\n");
  return 0;
}


int cdsDaqNetDaqSend(	int dcuId, 
			int cycle, 
			int subCycle, 
			unsigned int fileCrc, 
			unsigned int blockCrc,
			int crcSize, /* Data count */
			int tpCount,
			int tpNum[],
			int xferSize,
			char *dataBuffer)
{
#if defined(SHMEM_DAQ)
  // Mapped shared memory pointer
  extern char *_daq_shm;
  // IPC area to the frame builder pointer
  struct rmIpcStr *ipc = (struct rmIpcStr *)(_daq_shm + CDS_DAQ_NET_IPC_OFFSET);
  // Data buffers (DAQ) to the frame builder
  // "buf" point to the first buffer
  char *buf = _daq_shm + CDS_DAQ_NET_DATA_OFFSET;
  static const int buf_size = DAQ_DCU_BLOCK_SIZE*2;
  // GDS test point table in the shared memory 
  struct cdsDaqNetGdsTpNum *tp = (struct cdsDaqNetGdsTpNum *)(_daq_shm + CDS_DAQ_NET_GDS_TP_TABLE_OFFSET);

  //printf("cdsDaqNetDaqSend cycle=%d subCycle=%d size=%d file_crc=%x\n", cycle, subCycle, xferSize, fileCrc);
  int mycycle = cycle? cycle-1 : 15;

  // Copy data into the buffer
  buf += buf_size *cycle + subCycle * xferSize;
  dataBuffer += subCycle * xferSize;
  memcpy(buf, dataBuffer, xferSize);

  // End of current cycle, all data filled in
  if (subCycle == 15) {
	// Assign global parameters
  	ipc->dcuId = fileCrc; // DCU id of this system
  	ipc->crc = fileCrc; // Checksum of the configuration file

	// Assign current block parameters
	ipc->bp[mycycle].cycle = mycycle;
	ipc->bp[mycycle].crc = blockCrc;
	//ipc->bp[mycycle].status = 0;
  	ipc->bp[mycycle].timeSec = (unsigned int) cycle_gps_time;
  	ipc->bp[mycycle].timeNSec = (unsigned int) cycle_gps_ns + (unsigned int) (1000000000. * (cycle_gps_time - (unsigned int) cycle_gps_time));
	
	// Assign the test points table
	tp->count = tpCount;
	memcpy(tp->tpNum, tpNum, sizeof(tpNum[0]) * tpCount);

	// As the last step set the cycle counter
 	// Frame builder is looking for cycle change
	ipc->cycle =mycycle; // Ready cycle (16 Hz)
  }
#endif
  return(0);
}
#endif
#endif
