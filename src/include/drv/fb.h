#ifndef FB_H_INCLUDED
#define FB_H_INCLUDED

#include <drv/gmnet.h>
#if defined(USE_GM)
#define cdsDaqNetInit myriNetInit
#define cdsDaqNetClose myriNetClose
#define cdsDaqNetCheckCallback myriNetCheckCallback
#define cdsDaqNetReconnect myriNetReconnect
#define cdsDaqNetCheckReconnect myriNetCheckReconnect
#define cdsDaqNetDrop myriNetDrop
#define cdsDaqNetDaqSend myriNetDaqSend
#else
int cdsDaqNetInit(int);	  /* Initialize DAQ network		*/
int cdsDaqNetClose(void);  /* Close CDS network connection	*/
int cdsDaqNetCheckCallback(void);/* Check for messages on 	*/
int cdsDaqNetReconnect(int); /* Make connects to FB.		*/
int cdsDaqNetCheckReconnect(void);/* Check FB net connected	*/
int cdsDaqNetDrop(void);
int cdsDaqNetDaqSend(   int dcuId,
                        int cycle,
                        int subCycle,
                        unsigned int fileCrc,
                        unsigned int blockCrc,
                        int crcSize,
                        int tpCount,
                        int tpNum[],
                        int xferSize,
                        char *dataBuffer);

/* Offset in the shared memory to the beginning of data buffer */
#define CDS_DAQ_NET_DATA_OFFSET 0x2000
/* Offset to GDS test point table (struct cdsDaqNetGdsTpNum, defined in daqmap.h) */
#define CDS_DAQ_NET_GDS_TP_TABLE_OFFSET 0x1000
/* Offset to the IPC structure (struct rmIpcStr, defined in daqmap.h) */
#define CDS_DAQ_NET_IPC_OFFSET 0x0
#endif

extern int cdsNetStatus;
extern double cycle_gps_time;
extern unsigned int cycle_gps_ns;

#endif
