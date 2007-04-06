#ifndef FB_H_INCLUDED
#define FB_H_INCLUDED

int cdsDaqNetInit(int);	  /* Initialize DAQ network		*/
int cdsDaqNetClose(void);  /* Close CDS network connection	*/
int cdsDaqNetCheckCallback(void);/* Check for messages on 	*/
int cdsDaqNetReconnect(int); /* Make connects to FB.		*/
int cdsDaqNetCheckReconnect(void);/* Check FB net connected	*/
int cdsDaqNetDrop(void);
extern int cdsNetStatus;

#endif
