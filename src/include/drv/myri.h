int myriNetInit(int);	  /* Initialize myrinet card.		*/
int myriNetClose(void);	  /* Clean up myrinet on exit.		*/
int myriNetCheckCallback(void);/* Check for messages on myrinet.	*/
int myriNetReconnect(int); /* Make connects to FB.		*/
int myriNetCheckReconnect(void);/* Check FB net connected.		*/
int myriNetDrop(void);	  /* Check FB net connected.		*/
