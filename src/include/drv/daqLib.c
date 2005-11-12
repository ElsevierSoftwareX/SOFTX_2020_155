/*----------------------------------------------------------------------*/
/*                                                                      */
/* Module Name: daqLib.c                                                */
/*                                                                      */
/* Module Description: 							*/
/* 	This code provides two library functions used by many LIGO VME
	front end processors:
	1) rfm_refresh(): Sends requests to EPICS processors to update
	   selected reflected memory locations. This is important on 
	   startup as the FE CPU may have been powered down or otherwise
	   disconnected from the RFM loop.
	2) daqWrite(): Provides the front ends with the capability to
	   receive DAQ configuration information from and write data 
	   from any of its defined test points to the LIGO DAQ system 
	   via the RFM networks.			
									*/
/*                                                                      */
/* Module Arguments:    						*/
/*	rfm_refresh(uint offs - byte offset from beginning of RFM memory
		    uint size - Size in bytes to be refreshed.
		    )
		
	daqWrite(int flag - 0 to initialize or 1 to write data.
		 int dcuId - DAQ node ID of this front end.
		 DAQ_RANGE daqRange - struct defining front end valid
				      test point ranges.
		 int sysRate - Data rate of front end CPU.
		 float *pFloatData[] - Pointer to TP data array.
		 FILT_MOD *dspPtr[] - Point to array of FM data.
									*/
/*                                                                      */
/* Documentation References:                                            */
/*      Man Pages:                                                      */
/*      References:                                                     */
/*                                                                      */
/* Author Information:                                                  */
/*      Name          Telephone    Fax          e-mail                  */
/*      Rolf Bork   . (626)3953182 (626)5770424 rolf@ligo.caltech.edu   */
/*                                                                      */
/* Code Compilation and Runtime Specifications:                         */
/*      Code Compiled on: Sun Ultra60  running Solaris2.8               */
/*      Compiler Used: cc386                                            */
/*      Runtime environment: PentiumIII running VxWorks 5.4.1      	*/
/*                                                                      */
/* Hardware Required:
        1) Pentium CPU w/either VMIC5579 or VMIC5565PMC RFM module	*/
/*                                                                      */
/* Known Bugs, Limitations, Caveats:                                    */
/*                                                                      */
/*                      -------------------                             */
/*                                                                      */
/*                             LIGO                                     */
/*                                                                      */
/*        THE LASER INTERFEROMETER GRAVITATIONAL WAVE OBSERVATORY.      */
/*                                                                      */
/*                     (C) The LIGO Project, 2004.                      */
/*                                                                      */
/*                                                                      */
/*                                                                      */
/* California Institute of Technology                                   */
/* LIGO Project MS 18-34                                                */
/* Pasadena CA 91125                                                    */
/*                                                                      */
/* Massachusetts Institute of Technology                                */
/* LIGO Project MS 20B-145                                              */
/* Cambridge MA 01239                                                   */
/*                                                                      */
/*----------------------------------------------------------------------*/

char *daqLib5565_cvs_id = "$Id: daqLib.c,v 1.6 2005/11/12 01:10:17 rolf Exp $";

#define DAQ_16K_SAMPLE_SIZE	1024
#define DAQ_2K_SAMPLE_SIZE	128
#define DAQ_CONNECT		0
#define DAQ_WRITE		1

#define VMIC5565_NET    1
#define VMIC5579_NET    0

#define DTAPS	3


/* Structure for maintaining DAQ channel information */
typedef struct DAQ_LKUP_TABLE {
	int type;	
	int sysNum;
	int fmNum;
	int sigNum;
	int decFactor;
	int offset;
}DAQ_LKUP_TABLE;

/* Structure for maintaining TP channel number ranges */
typedef struct DAQ_RANGE {
	int filtTpMin;
	int filtTpMax;
	int filtTpSize;
	int xTpMin;
	int xTpMax;
} DAQ_RANGE;


int DAQ_TEST = -1;
volatile DAQ_INFO_BLOCK *pInfo;

/* Fast Pentium FPU SQRT command */
inline double lsqrt (double __x) { register double __result; __asm __volatile__ ("fsqrt" : "=t" (__result) : "0" (__x)); return __result; }

extern char *_epics_shm;
extern long _pci_rfm;



/* ******************************************************************** */
/* Routine to connect and write to LIGO DAQ system       		*/
/* ******************************************************************** */
int daqWrite(int flag,
		     int dcuId,
		     DAQ_RANGE daqRange,
		     int sysRate,
		     float *pFloatData[],
		     FILT_MOD *dspPtr[],
		     int netStatus,
		     int gdsMonitor[])
{
int ii,jj;
int status;
float dWord;
static long daqDataAddress[DAQ_NUM_DATA_BLOCKS];
static int daqBlockNum;
static int xferSize;
static int xferSize1;
static int xferLength;
static int xferDone;
static int crcLength;
static char *pDaqBuffer[2];
static char pdspace[2][1000000];
static DAQ_LKUP_TABLE localTable[DCU_MAX_CHANNELS];
static char *pWriteBuffer;
static char *pReadBuffer;
static int phase;
static int daqSlot;
static int daqWaitCycle;
static int daqWriteTime;
static int daqWriteCycle;
float *pFloat;
short *pShort;
char *pData;
char *bufPtr;
static unsigned int crcTest;
static unsigned int crcSend;
static int crcComplete;
static DAQ_INFO_BLOCK dataInfo;
volatile char *pRfmData;
volatile char *pLocalData;
static UINT32 fileCrc;
static int secondCounter;
int decSlot;
int offsetAccum;
static int tpStart;
static int daqReconfig;
static int configInProgress;
unsigned int *pXfer[2];
static volatile GDS_CNTRL_BLOCK *gdsPtr;
int testVal;
static int validTp;
static int tpNum[20];
static int totalChans;

#ifdef DAQ_DEC_FILTER
/* 6th order eliptic cutoff at 128 Hz, 80 db 
static double dCoeff[13] =
	{0.00028490832406584,
	-1.79846205926774, 0.81981757589904, -1.52610762089641, 1.00000000000000,
	-1.79404316559046, 0.88268926712295, -1.24646518445109, 1.00000000000000,
	-1.81292897340225, 0.96129731435968,  0.46049665114190, 1.00000000000000};
4th order eliptic corner at 90 Hz, 80 db 
static double dCoeff[9] =
	{0.020434809427619,
	-1.84093680779922, 0.86745935572456, -1.84775906502257, 1.0,
	-1.84957110082436, 0.91429341707980, -1.33211653915933, 0.88389819700609};
*/
static double dCoeff2x[13] =
	{0.014605318489015,
	-1.00613305346332,    0.31290490560439,   -0.00000330106714,    0.99667220785946,
	-0.85833656728801,    0.58019077541120,    0.30560272900767,    0.98043281669062,
	-0.77769970124012,    0.87790692599199,    1.65459644813269,    1.00000000000000};
static double dCoeff4x[13] =
	{0.0032897561126272,
	-1.52626060254343,    0.60240176412244,   -1.41321371411946,    0.99858678588255,
        -1.57309308067347,    0.75430004092087,   -1.11957678237524,    0.98454170534006,
        -1.65602262774366,    0.92929745639579,    0.26582650057056,    0.99777026734589};
static double dCoeff8x[13] =
	{0.0019451746049824,
	-1.75819687682033,    0.77900344926987,   -1.84669761259482,    0.99885145868275,
        -1.81776674036645,    0.86625937590562,   -1.73730291821706,    0.97396693941237,
        -1.89162859406079,    0.96263319997793,   -0.81263245399030,    0.83542699550059};
static double dCoeff16x[13] =
	{0.0010292496296221,
        -1.88217952734904,    0.88726069063363,   -1.96157059223343,    1.00000000000000,
        -1.92138912262668,    0.93301587832254,   -1.93613544029360,    1.00000000000000,
        -1.96312311206509,    0.97976907054276,   -1.61069820496735,    1.00000000000000};
static double dCoeff32x[13] =
	{0.00099066651652901,
        -1.94077236718909,    0.94207456685786,   -1.99036946487329,    1.00000000000000,
        -1.96299410148309,    0.96594271100631,   -1.98391795425616,    1.00000000000000,
        -1.98564991068275,    0.98982555984543,   -1.89550394774336,    1.00000000000000};
static double dHistory[DCU_MAX_CHANNELS][MAX_HISTRY];
#endif




 
  if(flag == DAQ_CONNECT)	/* Initialize DAQ connection */
  {

    /* First block to write out is last from previous second */
    phase = 0;
    daqBlockNum = 15;
    secondCounter = 0;
    configInProgress = -1;
    crcComplete = 0;

    /* Allocate memory for two local data swing buffers */
    for(ii=0;ii<2;ii++) 
    {
      pDaqBuffer[ii] = (char *)_pci_rfm;
      pDaqBuffer[ii] += 0x100000 * ii;
      printf("DAQ buffer %ld is at 0x%x\n",ii,(long long)pDaqBuffer[ii]);
    }

#ifdef DAQ_DEC_FILTER
    for(ii=0;ii<DCU_MAX_CHANNELS;ii++)
	for(jj=0;jj<MAX_HISTRY;jj++)
	     dHistory[ii][jj] = 0.0;
#endif

    /* Set pointers to two swing buffers */
    pWriteBuffer = (char *)pDaqBuffer[phase^1];
    pReadBuffer = (char *)pDaqBuffer[phase];
    daqSlot = -1;


    /* Set up pointer to DAQ IPC area for passing status info to DAQ */

    crcLength = 0;
printf("daqLib DCU_ID = %d\n",dcuId);

    /* Set up pointer to DAQ configuration information on RFM network */
    pInfo = (DAQ_INFO_BLOCK *)(_epics_shm + DAQ_INFO_ADDRESS + (dcuId * sizeof(DAQ_INFO_BLOCK)));
    printf("DAQ DATA INFO is at 0x%x\n",(long)pInfo);

    /* Get the .INI file crc checksum to pass to DAQ Framebuilders for config checking */
    fileCrc = pInfo->configFileCRC;
    pInfo->reconfig = 0;
    /* Get the number of channels to be acquired */
    dataInfo.numChans = pInfo->numChans;
    if((dataInfo.numChans < 1) || (dataInfo.numChans > DCU_MAX_CHANNELS))
    {
	printf("Invalid num daq chans = %d\n",dataInfo.numChans);
	return(-1);
    }

    /* Get the DAQ configuration information for all channels and calc a crc checksum length */
    for(ii=0;ii<dataInfo.numChans;ii++)
    {
      dataInfo.tp[ii].tpnum = pInfo->tp[ii].tpnum;
      dataInfo.tp[ii].dataType = pInfo->tp[ii].dataType;
      dataInfo.tp[ii].dataRate = pInfo->tp[ii].dataRate;
      if(dataInfo.tp[ii].dataType == DAQ_DATATYPE_16BIT_INT)
        crcLength += 2 * dataInfo.tp[ii].dataRate / 16;
      else
        crcLength += 4 * dataInfo.tp[ii].dataRate / 16;
    }

    /* Communicate DAQ channel count and file crc chksum to Framebuilders */
    printf("FILE CRC = 0x%x\n",fileCrc);


    /* Calculate the number of bytes to xfer on each call, based on total number
       of bytes to write each 1/16sec and the front end data rate (2048/16384Hz) */
    xferSize1 = crcLength/sysRate;
    
    /* 	Maintain 8 byte data boundaries for writing data, particularly important
        when DMA xfers are used on 5565 RFM modules. Note that this usually results
	in data not being written on every 2048/16384 cycle and last data xfer
	in a 1/16 sec block well may be shorter than the rest.			*/
    if(((crcLength/xferSize1) > sysRate) || ((xferSize1 % 8) > 0)) 
	xferSize1 = ((xferSize1/8) + 1) * 8;


    printf("Daq chan count = %d\nBlockSize = 0x%x\n",dataInfo.numChans,crcLength);
    printf("Daq XferSize = %d\n",xferSize1);
    printf("Daq xfer should complete in %d cycles\n",(crcLength/xferSize1));

    /* Set addresses for all 1/16 sec rfm data blocks */
    for(ii=0;ii<DAQ_NUM_DATA_BLOCKS;ii++) 
    {
      daqDataAddress[ii] = (DATA_OFFSET_DCU(dcuId) + ii * DAQ_DCU_BLOCK_SIZE);
      if(ii<2) printf("Daq Block %d = 0x%x\n",ii,daqDataAddress[ii]);
    }

    localTable[0].offset = 0;
    offsetAccum = 0;

    for(ii=0;ii<dataInfo.numChans;ii++)
    {
      /* Need to develop a table of offset pointers to load data into swing buffers 	*/
      /* This is based on decimation factors and data size				*/
      localTable[ii].decFactor = sysRate/(dataInfo.tp[ii].dataRate / 16);
      if(dataInfo.tp[ii].dataType == DAQ_DATATYPE_16BIT_INT)
	offsetAccum += (sysRate/localTable[ii].decFactor * 2);
      else
	offsetAccum += (sysRate/localTable[ii].decFactor * 4);
      localTable[ii+1].offset = offsetAccum;
#ifdef DEBUG_ON
      printf("ch %d = %d   DecFactor = %d   Offset = %d\n",ii,dataInfo.tp[ii].tpnum,localTable[ii].decFactor,localTable[ii].offset);
#endif
      /* Need to determine if data is from a filter module TP or non-FM TP */
      if((dataInfo.tp[ii].tpnum >= daqRange.filtTpMin) &&
	 (dataInfo.tp[ii].tpnum < daqRange.filtTpMax))
      /* This is a filter module testpoint */
      {
	jj = dataInfo.tp[ii].tpnum - daqRange.filtTpMin;
	/* Mark as coming from a filter module testpoint */
	localTable[ii].type = 0;
	/* Mark which system filter module is in */
	localTable[ii].sysNum = jj / daqRange.filtTpSize;
	jj -= localTable[ii].sysNum * daqRange.filtTpSize; 
	/* Mark which filter module within a system */
	localTable[ii].fmNum = jj / 3;
	/* Mark which of three testpoints to store */
	localTable[ii].sigNum = jj % 3;
#ifdef DEBUG_ON
	printf("type = %d  system = %d fmnum = %d  signum = %d\n",
		localTable[ii].type,localTable[ii].sysNum,localTable[ii].fmNum, localTable[ii].sigNum);
#endif
      }
      else if((dataInfo.tp[ii].tpnum >= daqRange.xTpMin) &&
	 (dataInfo.tp[ii].tpnum < daqRange.xTpMax))
      /* This testpoint is not part of a filter module */
      {
	jj = dataInfo.tp[ii].tpnum - daqRange.xTpMin;
	/* Mark as a non filter module testpoint */
	localTable[ii].type = 1;
	/* Mark the offset into the local data buffer */
	localTable[ii].sigNum = jj;
#ifdef DEBUG_ON
        dWord = *(pFloatData[localTable[ii].sigNum]);
	printf("XTP %d =  %d = %f\n",ii,localTable[ii].sigNum,dWord);
#endif
      }
      else
      {
	printf("Invalid chan num found %d = %d\n",ii,dataInfo.tp[ii].tpnum);
	return(-1);
      }
    }

    tpStart = offsetAccum;
    totalChans = dataInfo.numChans;
    printf("Start of TP data is at offset 0x%x\n",tpStart);
    gdsPtr = (GDS_CNTRL_BLOCK *)(_epics_shm + DAQ_GDS_BLOCK_ADD);
    printf("gdsCntrl block is at 0x%lx\n",(long)gdsPtr);
    for(ii=0;ii<32;ii++) gdsPtr->tp[2][0][ii] = 0;
    // gdsPtr->tp[3][0][0] = 11001;
    // gdsPtr->tp[3][0][1] = 11002;


    /* Set rfm net address to first data block to be written */
   daqWaitCycle = -1;
   daqWriteCycle = -1;
   daqWriteTime = sysRate / 16;

  } /* End DAQ CONNECT */

/* ******************************************************************************** */
/* Write Data to RFM 			******************************************* */
/* ******************************************************************************** */
  if(flag == DAQ_WRITE)
  {
    /* Calc data offset into swing buffer */
    daqSlot = (daqSlot + 1) % sysRate;
    daqWaitCycle = (daqWaitCycle + 1) % daqWriteTime;

    /* This section provides for on the fly reconfiguration of DAQ channels. It is
	initiated by a command from DAQ system. It essentially replacates what was
	done in the DAQ connect section of the software above.			 */
    if(daqReconfig)
    {
      if((daqSlot + configInProgress * sysRate)  == 0)
      {
  	crcLength = 0;
	fileCrc = pInfo->configFileCRC;
	dataInfo.numChans = pInfo->numChans;
      }
      if((daqSlot + configInProgress * sysRate) < dataInfo.numChans)
      {
	ii = daqSlot + configInProgress * sysRate;
	dataInfo.tp[ii].tpnum = pInfo->tp[ii].tpnum;
        dataInfo.tp[ii].dataType = pInfo->tp[ii].dataType;
        dataInfo.tp[ii].dataRate = pInfo->tp[ii].dataRate;
        if(dataInfo.tp[ii].dataType == DAQ_DATATYPE_16BIT_INT)
          crcLength += 2 * dataInfo.tp[ii].dataRate / 16;
        else
          crcLength += 4 * dataInfo.tp[ii].dataRate / 16;
      }
      if((daqSlot + configInProgress * sysRate) == dataInfo.numChans)
      {
	xferSize1 = crcLength/sysRate;
    	if(((crcLength/xferSize1) > sysRate) || ((xferSize1 % 8) > 0))
            xferSize1 = ((xferSize1/8) + 1) * 8;
      }
      if((daqSlot + configInProgress * sysRate) == (dataInfo.numChans + 1))
      {
    	localTable[0].offset = 0;
    	offsetAccum = 0;
	pInfo->reconfig = 0;

    	for(ii=0;ii<dataInfo.numChans;ii++)
    	{
#ifdef DAQ_DEC_FILTER
	      for(jj=0;jj<MAX_HISTRY;jj++) dHistory[ii][jj] = 0.0;
#endif
	      localTable[ii].decFactor = sysRate/(dataInfo.tp[ii].dataRate / 16);
	      if(dataInfo.tp[ii].dataType == DAQ_DATATYPE_16BIT_INT)
		offsetAccum += (sysRate/localTable[ii].decFactor * 2);
	      else
		offsetAccum += (sysRate/localTable[ii].decFactor * 4);
	      localTable[ii+1].offset = offsetAccum;
	      if((dataInfo.tp[ii].tpnum >= daqRange.filtTpMin) &&
		 (dataInfo.tp[ii].tpnum < daqRange.filtTpMax))
	      /* This is a filter module testpoint */
	      {
		jj = dataInfo.tp[ii].tpnum - daqRange.filtTpMin;
		/* Mark as coming from a filter module testpoint */
		localTable[ii].type = 0;
		/* Mark which system filter module is in */
		localTable[ii].sysNum = jj / daqRange.filtTpSize;
		/* Mark which filter module within a system */
		localTable[ii].fmNum = jj / 3;
		/* Mark which of three testpoints to store */
		localTable[ii].sigNum = jj % 3;
	      }
	      else if((dataInfo.tp[ii].tpnum >= daqRange.xTpMin) &&
		 (dataInfo.tp[ii].tpnum < daqRange.xTpMax))
	      /* This testpoint is not part of a filter module */
	      {
		jj = dataInfo.tp[ii].tpnum - daqRange.xTpMin;
		/* Mark as a non filter module testpoint */
		localTable[ii].type = 1;
		/* Mark the offset into the local data buffer */
		localTable[ii].sigNum = jj;
	      }
	      else
	      {
		printf("Invalid chan num found %d = %d\n",ii,dataInfo.tp[ii].tpnum);
		return(-1);
	      }

      	}
      }
    }
/* End DAQ Reconfiguration */

    /* This section of code sends data to the DAQ system */
    else
    {

    /* At start of 1/16 sec. data block, reset the xfer sizes and done bit */
    if(daqSlot == 0)
    {
	xferLength = crcLength;
	xferSize = xferSize1;
	xferDone = 0;
	crcComplete = 0;
    }

    /* If size of data remaining to be sent is less than calc xfer size, reduce xfer size
       to that of data remaining and mark that data xfer is complete for this 1/16 sec block */
    if((xferLength < xferSize1) && (!xferDone))
    {
	xferSize = xferLength;
	if(xferSize <= 0)  xferDone = 1;
    }


    /* Write data into local swing buffer */
    for(ii=0;ii<totalChans;ii++)
    {

      /* Read in the data to a local variable, either from a FM TP or other TP */
      if(localTable[ii].type == 0)
      /* Data if from filter module testpoint */
      {
	switch(localTable[ii].sigNum)
	{
	  case 0:
	    dWord = dspPtr[localTable[ii].sysNum]->data[localTable[ii].fmNum].filterInput;
	    break;
	  case 1:
	    dWord = dspPtr[localTable[ii].sysNum]->data[localTable[ii].fmNum].inputTestpoint;
	    break;
	  case 2:
	    dWord = dspPtr[localTable[ii].sysNum]->data[localTable[ii].fmNum].testpoint;
	    break;
 	  default:
	    dWord = 0.0;
	    break;
	}
      }
      else
      /* Data is from non filter module  testpoint */
      {
        dWord = *(pFloatData[localTable[ii].sigNum]);
      }

#ifdef DAQ_DEC_FILTER
      if(localTable[ii].decFactor == 2) dWord = iir_filter(dWord,&dCoeff2x[0],DTAPS,&dHistory[ii][0]);
      if(localTable[ii].decFactor == 4) dWord = iir_filter(dWord,&dCoeff4x[0],DTAPS,&dHistory[ii][0]);
      if(localTable[ii].decFactor == 8) dWord = iir_filter(dWord,&dCoeff8x[0],DTAPS,&dHistory[ii][0]);
      if(localTable[ii].decFactor == 16) dWord = iir_filter(dWord,&dCoeff16x[0],DTAPS,&dHistory[ii][0]);
      if(localTable[ii].decFactor == 32) dWord = iir_filter(dWord,&dCoeff32x[0],DTAPS,&dHistory[ii][0]);
#endif

      if((daqSlot % localTable[ii].decFactor) == 0)
      {
	/* Set the buffer pointer to the start of this channels data */
      	pData = (char *)pWriteBuffer;
      	pData += localTable[ii].offset;

	/* Set either a short pointer or float pointer to the next data location in the 
	   local swing buffer, as based on dataType */
      	if(dataInfo.tp[ii].dataType == DAQ_DATATYPE_16BIT_INT)
      	{
          /* Point to local swing buffer */
      	  pShort = (short *)pData;
	  /* Determine offset from start of 1/16 sec data for this channel given the DAQ cycle
	     and the decimation factor */
	  decSlot = daqSlot/localTable[ii].decFactor;
      	  /* add this data offset to the data pointer */
      	  pShort += decSlot;
      	  /* Need to sample swap short data */
      	  if((decSlot % 2) == 0) pShort ++;
      	  else pShort --;
	}
      	else	/* Floating point number is to be written */
      	{
          /* Point to local swing buffer */
      	  pFloat = (float *)pData;
	  /* Determine offset from start of 1/16 sec data for this channel given the DAQ cycle
	     and the decimation factor */
	  decSlot = daqSlot/localTable[ii].decFactor;
      	  /* add this data offset to the data pointer */
      	  pFloat += decSlot;
      	}

	/* Write the data into the local swing buffer */
      	if(dataInfo.tp[ii].dataType == DAQ_DATATYPE_16BIT_INT)
      		/* Write data as short into swing buffer */
	  	*pShort = (short)dWord;
      	else
      		/* Write data as float into swing buffer */
		*pFloat = dWord;
      }
    } /* end swing buffer write loop */


  if(!xferDone)
  {
    /* Do CRC check sum calculation */
    bufPtr = (char *)pReadBuffer + daqSlot * xferSize1;
    if(daqSlot == 0) crcTest = crc_ptr(bufPtr, xferSize, 0);
    else crcTest = crc_ptr(bufPtr, xferSize, crcTest);

    xferLength -= xferSize;
  }
    if(daqSlot == (sysRate - 1))
  /* Done with 1/16 second data block */
  {

      /* Complete CRC checksum calc and send to ipc area */
      crcTest = crc_len(crcLength, crcTest);
      crcComplete = 1;
      crcSend = crcTest;
  }

  /* Write DAQ data to the RFM network */
  if(!daqWaitCycle)
  {
	if(!netStatus) status = myriNetDaqSend(dcuId,daqBlockNum, daqWriteCycle, fileCrc, 
						crcSend,crcLength,validTp,tpNum,pReadBuffer);
	daqWriteCycle = (daqWriteCycle + 1) % 16;
  }
}

    if(daqSlot == (sysRate - 1))
    /* Done with 1/16 second data block */
    {
      /* Swap swing buffers */
      phase = (phase + 1) % 2;
      pReadBuffer = (char *)pDaqBuffer[phase];
      pWriteBuffer = (char *)pDaqBuffer[(phase^1)];

      /* Move to next RFM buffer */
      daqBlockNum = (daqBlockNum + 1) % 16;
      daqReconfig = 0;
      if(pInfo->reconfig == 1)
      {
	daqReconfig = 1;
	configInProgress ++;
      }
      else
      {
	configInProgress = -1;
      }
      // Check for new TP
      if(daqBlockNum == 0)
      {
 	totalChans = dataInfo.numChans;
	offsetAccum = tpStart;
	validTp = 0;
	for(ii=0;ii<20;ii++)
	{
		/* Get TP number from shared memory */
		testVal = gdsPtr->tp[2][0][ii];

		/* Check if the TP selection is valid for this front end's filter module TP
		   If it is, load the local table with the proper lookup values to find the
		   signal and clear the TP status in shared mem.                           */
        	if((testVal >= daqRange.filtTpMin) &&
           	   (testVal < daqRange.filtTpMax))
        	{
		  jj = dataInfo.tp[ii].tpnum - daqRange.filtTpMin;
		  localTable[totalChans].type = 0;
          	  localTable[totalChans].sysNum = jj / daqRange.filtTpSize;
		  jj -= localTable[ii].sysNum * daqRange.filtTpSize;
          	  localTable[totalChans].fmNum = jj / 3;
          	  localTable[totalChans].sigNum = jj % 3; 
	  	  localTable[totalChans].decFactor = 1;
      		  dataInfo.tp[totalChans].dataType = 4;
		  offsetAccum += sysRate * 4;
		  localTable[totalChans+1].offset = offsetAccum;
          	  gdsMonitor[ii] = testVal;
          	  gdsPtr->tp[2][1][ii] = 0;
		  tpNum[validTp] = testVal;
		  validTp ++;
		  totalChans ++;
        	}

		/* Check if the TP selection is valid for other front end TP signals.
		   If it is, load the local table with the proper lookup values to find the
		   signal and clear the TP status in RFM.                                       */
		else if( (testVal >= daqRange.xTpMin) &&
			 (testVal < daqRange.xTpMax))
		{
	 	  jj = dataInfo.tp[ii].tpnum - daqRange.xTpMin;
		  localTable[totalChans].type = 1;
		  localTable[totalChans].sigNum = jj;
		  offsetAccum += sysRate * 4;
		  localTable[totalChans+1].offset = offsetAccum;
	  	  localTable[totalChans].decFactor = 1;
      		  dataInfo.tp[totalChans].dataType = 4;
		  gdsMonitor[ii] = testVal;
		  gdsPtr->tp[2][1][ii] = 0;
		  tpNum[validTp] = testVal;
		  validTp ++;
		  totalChans ++;
		}

		/* If this TP select is not valid for this front end, deselect it in the local
		   lookup table.                                                                */
		else
		{
		  gdsMonitor[ii] = testVal;
		}

	    }  /* End for loop */
      } /* End normal check for new TP numbers */

    } /* End done 16Hz Cycle */

  } /* End case write */
  return(dataInfo.numChans);

}
