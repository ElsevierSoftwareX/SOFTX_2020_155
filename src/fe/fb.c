#include <linux/types.h>
#include <linux/pci.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#ifndef NO_DAQ
#include <drv/gmnet.h>
#include <drv/cdsHardware.h>
#include <drv/fb.h>

#if defined(USE_GM)
#include <drv/myri.h>
#endif

int cdsNetStatus = 0;

#if !defined(USE_GM)
int cdsDaqNetDrop()
{
  return(0);
}


int cdsDaqNetInit(int fbId)
{
  return(1);
}

int cdsDaqNetClose()
{
  return(0);
}

int cdsDaqNetCheckCallback()
{
  //return(expected_callbacks);
  return 0;
}

int cdsDaqNetReconnect(int dcuId)
{
  //return(expected_callbacks);
  return 0;
}

int cdsDaqNetCheckReconnect()
{
  //return(eMessage);
  return 0;
}


int cdsDaqNetDaqSend(	int dcuId, 
			int cycle, 
			int subCycle, 
			unsigned int fileCrc, 
			unsigned int blockCrc,
			int crcSize,
			int tpCount,
			int tpNum[],
			int xferSize,
			char *dataBuffer)
{
  return(0);
}
#endif
#endif
