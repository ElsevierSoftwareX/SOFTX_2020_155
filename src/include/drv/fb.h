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

#endif
extern int cdsNetStatus;

#endif
