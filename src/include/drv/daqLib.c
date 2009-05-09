/*----------------------------------------------------------------------------- */
/*                                                                      	*/
/* Module Name: daqLib.c                                                	*/
/*                                                                      	*/
/* Module Description: 								*/
/*
	This module provides the generic routines for:
		- Writing DAQ, GDS TP and GDS EXC signals to the
		  Framebuilder.
		- Reading GDS EXC signals from shmem and writing them
		  to the requested front end code exc channels.
										*/
/*                                                                      	*/
/* Module Arguments:    							*/
/*
		
	daqWrite(int flag - 0 to initialize or 1 to write data.
		 int dcuId - DAQ node ID of this front end.
		 DAQ_RANGE daqRange - struct defining front end valid
				      test point and exc ranges.
		 int sysRate - Data rate of front end CPU / 16.
		 float *pFloatData[] - Pointer to TP data not associated
				       with filter modules.
		 FILT_MOD *dspPtr - Point to array of FM data.
		 int netStatus - Status of myrinet
		 int gdsMonitor[] - Array to return values of GDS
				    TP/EXC selections.
		 float excSignal[] - Array to write EXC signals not
				     associated with filter modules.
										*/
/*                                                                      	*/
/*                                                                      	*/
/*                      -------------------                             	*/
/*                                                                      	*/
/*                             LIGO                                     	*/
/*                                                                      	*/
/*        THE LASER INTERFEROMETER GRAVITATIONAL WAVE OBSERVATORY.      	*/
/*                                                                      	*/
/*                     (C) The LIGO Project, 2005.                      	*/
/*                                                                      	*/
/*                                                                      	*/
/*                                                                      	*/
/* 		California Institute of Technology                             	*/
/* 		LIGO Project MS 18-34                                          	*/
/* 		Pasadena CA 91125                                              	*/
/*                                                                      	*/
/* 		Massachusetts Institute of Technology                          	*/
/* 		LIGO Project MIT NW17-161                                     	*/
/* 		Cambridge MA 02139                                             	*/
/*                                                                      	*/
/*----------------------------------------------------------------------------- */

char *daqLib5565_cvs_id = "$Id: daqLib.c,v 1.37 2009/05/09 20:07:02 aivanov Exp $";

#define DAQ_16K_SAMPLE_SIZE	1024	/* Num values for 16K system in 1/16 second 	*/
#define DAQ_2K_SAMPLE_SIZE	128	/* Num values for 2K system in 1/16 second	*/
#define DAQ_CONNECT		0
#define DAQ_WRITE		1

#define DTAPS	3	/* Num SOS in decimation filters.	*/


/* Structure for maintaining DAQ channel information */
typedef struct DAQ_LKUP_TABLE {
	int type;	/* 0=SFM, 1=nonSFM TP, 2= SFM EXC, 3=nonSFM EXC 	*/	
	int sysNum;	/* If multi-dim SFM array, which one to use.		*/
	int fmNum;	/* Filter module with signal of interest.		*/
	int sigNum;	/* Which signal within a filter.			*/
	int decFactor;	/* Decimation factor to use.				*/
	int offset;	/* Offset to beginning of next channel in local buff.	*/
}DAQ_LKUP_TABLE;

// Structure for maintaining TP channel number ranges
// which are valid for this front end. These typically
// come from gdsLib.h.
typedef struct DAQ_RANGE {
	int filtTpMin;
	int filtTpMax;
	int filtTpSize;
	int xTpMin;
	int xTpMax;
	int filtExMin;
	int filtExMax;
	int filtExSize;
	int xExMin;
	int xExMax;
} DAQ_RANGE;


volatile DAQ_INFO_BLOCK *pInfo;		/* Ptr to DAQ config in shmem.	*/

/* Fast Pentium FPU SQRT command */
// inline double lsqrt (double __x) { register double __result; __asm __volatile__ ("fsqrt" : "=t" (__result) : "0" (__x)); return __result; }

extern char *_epics_shm;		/* Ptr to EPICS shmem block		*/
extern long daqBuffer;			/* Address of daqLib swing buffers.	*/


/* ******************************************************************** */
/* Routine to connect and write to LIGO DAQ system       		*/
/* ******************************************************************** */
int daqWrite(int flag,
	     int dcuId,
	     DAQ_RANGE daqRange,
	     int sysRate,
	     float *pFloatData[],
	     FILT_MOD *dspPtr,
	     int netStatus,
	     int gdsMonitor[],
	     float excSignal[])
{
int ii,jj;			/* Loop counters.			*/
int status;			/* Return value from called routines.	*/
float dWord;			/* Temp value for storage of DAQ values */
static int daqBlockNum;		/* 1-16, tracks DAQ block to write to.	*/
static int excBlockNum;		/* 1-16, tracks EXC block to read from.	*/
static int excDataSize;
static int xferSize;		/* Tracks remaining xfer size for crc	*/
static int xferSize1;
static int mnDaqSize;
static int xferLength;
static int xferDone;
static int crcLength;		/* Number of bytes in 1/16 sec data 	*/
static char *pDaqBuffer[2];	/* Pointers to local swing buffers.	*/
static DAQ_LKUP_TABLE localTable[DCU_MAX_CHANNELS];
static DAQ_LKUP_TABLE excTable[DCU_MAX_CHANNELS];
static char *pWriteBuffer;	/* Ptr to swing buff to write data	*/
static char *pReadBuffer;	/* Ptr to swing buff to xmit data to FB */
static int phase;		/* 0-1, switches swing buffers.		*/
static int daqSlot;		/* 0-sysRate, data slot to write data	*/
static int excSlot;		/* 0-sysRate, slot to read exc data	*/
static int daqWaitCycle;	/* If 0, write to FB (256Hz)		*/
static int daqWriteTime;	/* Num daq cycles between writes.	*/
static int daqWriteCycle;	/* Cycle count to xmit to FB.		*/
float *pFloat = 0;		/* Temp ptr to write float data.	*/
short *pShort = 0;		/* Temp ptr to write short data.	*/
char *pData;			/* Ptr to start of data set in swing	*/
char *bufPtr;			/* Ptr to data for crc calculation.	*/
static unsigned int crcTest;	/* Continuous calc of CRC.		*/
static unsigned int crcSend;	/* CRC sent to FB.			*/
static DAQ_INFO_BLOCK dataInfo; /* Local DAQ config info buffer.	*/
static UINT32 fileCrc;		/* .ini file CRC, sent to FB.		*/
int decSlot;			/* Tracks decimated data positions.	*/
int offsetAccum;		/* Used to set localTable.offset	*/
static int tpStart;		/* Marks address of first TP data	*/
static volatile GDS_CNTRL_BLOCK *gdsPtr;  /* Ptr to GDS table in shmem.	*/
static volatile char *exciteDataPtr;	  /* Ptr to EXC data in shmem.	*/
int testVal;			/* Temp TP value for valid check.	*/
static int validTp;		/* Number of valid GDS sigs selected.	*/
static int validTpNet;		/* Number of valid GDS sigs selected.	*/
static int validEx;		/* Local chan number of 1st EXC signal.	*/
static int tpNum[GM_DAQ_MAX_TPS]; /* TP/EXC selects to send to FB.	*/
static int tpNumNet[GM_DAQ_MAX_TPS]; /* TP/EXC selects to send to FB.	*/
static int totalChans;		/* DAQ + TP + EXC chans selected.	*/
static int totalSize;		/* DAQ + TP + EXC chans size in bytes.	*/
static int totalSizeNet;	/* DAQ + TP + EXC chans size in bytes.	*/
int *statusPtr;
volatile float *dataPtr;	/* Ptr to excitation chan data.		*/
int exChanOffset;		/* shmem offset to next EXC value.	*/
int tpx;
int tpAdd;

#if DCU_MAX_CHANNELS > DAQ_GDS_MAX_TP_NUM
#warning DCU_MAX_CHANNELS greater than DAQ_GDS_MAX_TP_NUM
#endif

// Decimation filter coefficient definitions.		
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
static double dCoeff64x[9] =
        {0.0033803482406795,
        -1.97723317288580,    0.97769671166054,   -1.99036945334439,    1.00000000000000,
        -1.99094221621559,    0.99261635146438,   -1.97616224554465,    1.00000000000000};
static double dHistory[DCU_MAX_CHANNELS][MAX_HISTRY];




// ************************************************************************************** 
  if(flag == DAQ_CONNECT)	/* Initialize DAQ connection */
  {

    /* First block to write out is last from previous second */
    phase = 0;
    daqBlockNum = 15;
    excBlockNum = 0;

    /* Allocate memory for two local data swing buffers */
    for(ii=0;ii<2;ii++) 
    {
      pDaqBuffer[ii] = (char *)daqBuffer;
      pDaqBuffer[ii] += DAQ_DCU_SIZE * ii;
      printf("DAQ buffer %ld is at 0x%x\n",ii,(long long)pDaqBuffer[ii]);
    }

    // Clear the decimation filter histories
    for(ii=0;ii<DCU_MAX_CHANNELS;ii++)
	for(jj=0;jj<MAX_HISTRY;jj++)
	     dHistory[ii][jj] = 0.0;

    /* Set pointers to two swing buffers */
    pWriteBuffer = (char *)pDaqBuffer[phase^1];
    pReadBuffer = (char *)pDaqBuffer[phase];
    daqSlot = -1;
    excSlot = 0;



    crcLength = 0;
    printf("daqLib DCU_ID = %d\n",dcuId);

    /* Set up pointer to DAQ configuration information in shmem */
    pInfo = (DAQ_INFO_BLOCK *)(_epics_shm + DAQ_INFO_ADDRESS + (dcuId * sizeof(DAQ_INFO_BLOCK)));
    printf("DAQ DATA INFO is at 0x%x\n",(long)pInfo);

    /* Get the .INI file crc checksum to pass to DAQ Framebuilders for config checking */
    fileCrc = pInfo->configFileCRC;
    // Clear the reconfiguration flag in shmem.
    pInfo->reconfig = 0;
    /* Get the number of channels to be acquired */
    dataInfo.numChans = pInfo->numChans;
    // Verify number of channels is legal
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

    /* Calculate the number of bytes to xfer on each call, based on total number
       of bytes to write each 1/16sec and the front end data rate (2048/16384Hz) */
    // Note, this is left over from RFM net. Now used only to calc CRC
    xferSize1 = crcLength/sysRate;
    mnDaqSize = crcLength/16;
    totalSize = mnDaqSize;
    totalSizeNet = mnDaqSize;
    
    if (xferSize1 == 0) {
	printf("DAQ size too small\n");
	return -1;
    }
/*    if (xferSize1 == 0) xferSize1 = 8;  ???? */

    /* 	Maintain 8 byte data boundaries for writing data, particularly important
        when DMA xfers are used on 5565 RFM modules. Note that this usually results
	in data not being written on every 2048/16384 cycle and last data xfer
	in a 1/16 sec block well may be shorter than the rest.			*/
    if(((crcLength/xferSize1) > sysRate) || ((xferSize1 % 8) > 0)) 
	xferSize1 = ((xferSize1/8) + 1) * 8;


    printf("Daq chan count = %d\nBlockSize = 0x%x\n",dataInfo.numChans,crcLength);
    printf("Daq XferSize = %d\n",xferSize1);
    printf("Daq xfer should complete in %d cycles\n",(crcLength/xferSize1));
    printf("Daq xmit Size = %d\n",mnDaqSize);


    // Fill in the local lookup table for finding data.
    localTable[0].offset = 0;
    offsetAccum = 0;

    for(ii=0;ii<dataInfo.numChans;ii++)
    {
      /* Need to develop a table of offset pointers to load data into swing buffers 	*/
      /* This is based on decimation factors and data size				*/
      if ((dataInfo.tp[ii].dataRate / 16) > sysRate) {
	/* Channel data rate is greater than system rate */
	printf("Channels %d has bad data rate %d\n", ii, dataInfo.tp[ii].dataRate);
	return(-1);
      } else {
      localTable[ii].decFactor = sysRate/(dataInfo.tp[ii].dataRate / 16);
      }
      if(dataInfo.tp[ii].dataType == DAQ_DATATYPE_16BIT_INT)
	offsetAccum += (sysRate/localTable[ii].decFactor * 2);
      else
	offsetAccum += (sysRate/localTable[ii].decFactor * 4);
      localTable[ii+1].offset = offsetAccum;
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
      }
      else if((dataInfo.tp[ii].tpnum >= daqRange.filtExMin) &&
	 (dataInfo.tp[ii].tpnum < daqRange.filtExMax))
      /* This is a filter module excitation input */
      {
	/* Mark as coming from a filter module excitation input */
	localTable[ii].type = 2;
	/* Mark filter module number */
	localTable[ii].fmNum =  dataInfo.tp[ii].tpnum - daqRange.filtExMin;
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

    // Set the start of TP data after DAQ data.
    tpStart = offsetAccum;
    totalChans = dataInfo.numChans;
    // Set pointer to GDS table in shmem
    gdsPtr = (GDS_CNTRL_BLOCK *)(_epics_shm + DAQ_GDS_BLOCK_ADD);
    printf("gdsCntrl block is at 0x%lx\n",(long)gdsPtr);
    // Clear out the GDS TP selections.
    if(sysRate == DAQ_2K_SAMPLE_SIZE) tpx = 3;
    else tpx = 2;
    for(ii=0;ii<24;ii++) gdsPtr->tp[tpx][0][ii] = 0;
    if(sysRate == DAQ_2K_SAMPLE_SIZE) tpx = 1;
    else tpx = 0;
    for(ii=0;ii<8;ii++) gdsPtr->tp[tpx][0][ii] = 0;
    // Following can be uncommented for testing.
    // gdsPtr->tp[3][0][0] = 31250;
    // gdsPtr->tp[3][0][1] = 11002;
    // gdsPtr->tp[0][0][0] = 5000;

    // Set pointer to EXC data in shmem.
    if(sysRate == DAQ_2K_SAMPLE_SIZE)
	{
    	exciteDataPtr = (char *)(_epics_shm + DATA_OFFSET_DCU(DCU_ID_EX_2K));
	excDataSize = 0x204;
	}
    else {
    	exciteDataPtr = (char *)(_epics_shm + DATA_OFFSET_DCU(DCU_ID_EX_16K));
	excDataSize = 4 + 4 * sysRate;
    }

    // Following just sets in some dummy data for testing.
    for(ii=0;ii<16;ii++)
    {
	statusPtr = (int *)(exciteDataPtr + ii * DAQ_DCU_BLOCK_SIZE);
	*statusPtr = 0;
	dataPtr = (float *)(exciteDataPtr + ii * DAQ_DCU_BLOCK_SIZE + 4);
	for(jj=0;jj<1024;jj++)
	{
		*dataPtr = (float)(ii * 1000 + jj);
		dataPtr ++;
	}
    }


    // Initialize network variables.
    daqWaitCycle = -1;
    daqWriteCycle = 0;
    daqWriteTime = sysRate / 16;

  } /* End DAQ CONNECT */

/* ******************************************************************************** */
/* Write Data to FB 			******************************************* */
/* ******************************************************************************** */
  if(flag == DAQ_WRITE)
  {
    /* Calc data offset into swing buffer */
    daqSlot = (daqSlot + 1) % sysRate;
    daqWaitCycle = (daqWaitCycle + 1) % daqWriteTime;

    /* At start of 1/16 sec. data block, reset the xfer sizes and done bit */
    if(daqSlot == 0)
    {
	xferLength = crcLength;
	xferSize = xferSize1;
	xferDone = 0;
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

      dWord = 0;
      /* Read in the data to a local variable, either from a FM TP or other TP */
      if(localTable[ii].type == 0)
      /* Data if from filter module testpoint */
      {
	switch(localTable[ii].sigNum)
	{
	  case 0:
	    dWord = dspPtr->data[localTable[ii].fmNum].filterInput;
	    break;
	  case 1:
	    dWord = dspPtr->data[localTable[ii].fmNum].inputTestpoint;
	    break;
	  case 2:
	    dWord = dspPtr->data[localTable[ii].fmNum].testpoint;
	    break;
 	  default:
	    dWord = 0.0;
	    break;
	}
      }
      else if(localTable[ii].type == 1)
      /* Data is from non filter module  testpoint */
      {
	    dWord = *(pFloatData[localTable[ii].sigNum]);
      }
      else if(localTable[ii].type == 2)
      /* Data is from filter module excitation */
      {
	    dWord = dspPtr->data[localTable[ii].fmNum].exciteInput;
      }

      // Perform decimation filtering, if required.
      if(localTable[ii].decFactor == 2) dWord = iir_filter(dWord,&dCoeff2x[0],DTAPS,&dHistory[ii][0]);
      if(localTable[ii].decFactor == 4) dWord = iir_filter(dWord,&dCoeff4x[0],DTAPS,&dHistory[ii][0]);
      if(localTable[ii].decFactor == 8) dWord = iir_filter(dWord,&dCoeff8x[0],DTAPS,&dHistory[ii][0]);
      if(localTable[ii].decFactor == 16) dWord = iir_filter(dWord,&dCoeff16x[0],DTAPS,&dHistory[ii][0]);
      if(localTable[ii].decFactor == 32) dWord = iir_filter(dWord,&dCoeff32x[0],DTAPS,&dHistory[ii][0]);
      if(localTable[ii].decFactor == 64) dWord = iir_filter(dWord,&dCoeff64x[0],2,&dHistory[ii][0]);

      // Write the data into the swing buffer.
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
//#if defined(COMPAT_INITIAL_LIGO)
#if 0
	#define byteswap(a,b) ((char *)&a)[0] = ((char *)&b)[3]; ((char *)&a)[1] = ((char *)&b)[2];((char *)&a)[2] = ((char *)&b)[1];((char *)&a)[3] = ((char *)&b)[0];
#else	
	#define byteswap(a,b) a
#endif

	/* Write the data into the local swing buffer */
      	if(dataInfo.tp[ii].dataType == DAQ_DATATYPE_16BIT_INT)
      		/* Write data as short into swing buffer */
	  	*pShort = (short)dWord;
      	else {
      		/* Write data as float into swing buffer */
		float t = dWord;
		byteswap(dWord, t);
		*pFloat = dWord;
	}
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
      /* Complete CRC checksum calc */
      crcTest = crc_len(crcLength, crcTest);
      crcSend = crcTest;
  }

  /* Write DAQ data to the Framebuilder 256 times per second */
  if(!daqWaitCycle)
  {
	if(!netStatus) status = cdsDaqNetDaqSend(dcuId,daqBlockNum, daqWriteCycle, fileCrc, 
						crcSend,crcLength,validTpNet,tpNumNet,totalSizeNet,pReadBuffer);
	daqWriteCycle = (daqWriteCycle + 1) % 16;
  }

  // Read in any selected EXC signals.
  excSlot = (excSlot + 1) % sysRate;
  if(validEx)
  {
  	for(ii=validEx;ii<totalChans;ii++)
  	{
		exChanOffset = localTable[ii].sigNum * excDataSize;
		statusPtr = (int *)(exciteDataPtr + excBlockNum * DAQ_DCU_BLOCK_SIZE + exChanOffset);
		if(*statusPtr == 0)
		{
			dataPtr = (float *)(exciteDataPtr + excBlockNum * DAQ_DCU_BLOCK_SIZE + exChanOffset +
					    excSlot * 4 + 4);
			if(localTable[ii].type == 2)
			{
				dspPtr->data[localTable[ii].fmNum].exciteInput = *dataPtr;
#if 0
				if((excSlot == 0) && (ii == validEx)) printf("%ld %f\n",(int)dataPtr,*dataPtr);
#endif
			}
		}
		else dspPtr->data[localTable[ii].fmNum].exciteInput = 0.0;
  	}
  }

  // Move to the next 1/16 EXC signal data block
  if(excSlot == (sysRate - 1)) excBlockNum = (excBlockNum + 1) % 16;

    if(daqSlot == (sysRate - 1))
    /* Done with 1/16 second DAQ data block */
    {
      /* Swap swing buffers */
      phase = (phase + 1) % 2;
      pReadBuffer = (char *)pDaqBuffer[phase];
      pWriteBuffer = (char *)pDaqBuffer[(phase^1)];

      /* Increment the 1/16 sec block counter */
      daqBlockNum = (daqBlockNum + 1) % 16;

      // Check for reconfig request at start of each second
      if((pInfo->reconfig == 1) && (daqBlockNum == 0))
      {
	    printf("New daq config\n");
	    status = pInfo->numChans;
	    pInfo->reconfig = 0;
	    if((status > 0) && (status <= DCU_MAX_CHANNELS))
	    {
		    /* Get the .INI file crc checksum to pass to DAQ Framebuilders for config checking */
		    fileCrc = pInfo->configFileCRC;
		    /* Get the number of channels to be acquired */
		    dataInfo.numChans = pInfo->numChans;
		    crcLength = 0;
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
		    /* Calculate the number of bytes to xfer on each call, based on total number
		       of bytes to write each 1/16sec and the front end data rate (2048/16384Hz) */
		    xferSize1 = crcLength/sysRate;
    		    mnDaqSize = crcLength/16;
    		    totalSize = mnDaqSize;

		    /*  Maintain 8 byte data boundaries for writing data, particularly important
			when DMA xfers are used on 5565 RFM modules. Note that this usually results
			in data not being written on every 2048/16384 cycle and last data xfer
			in a 1/16 sec block well may be shorter than the rest.                  */
		    if(((crcLength/xferSize1) > sysRate) || ((xferSize1 % 8) > 0))
			xferSize1 = ((xferSize1/8) + 1) * 8;

		    localTable[0].offset = 0;
		    offsetAccum = 0;

		    for(ii=0;ii<dataInfo.numChans;ii++)
		    {
		      /* Need to develop a table of offset pointers to load data into swing buffers     */
		      /* This is based on decimation factors and data size                              */
		      localTable[ii].decFactor = sysRate/(dataInfo.tp[ii].dataRate / 16);
		      if(dataInfo.tp[ii].dataType == DAQ_DATATYPE_16BIT_INT)
			offsetAccum += (sysRate/localTable[ii].decFactor * 2);
		      else
			offsetAccum += (sysRate/localTable[ii].decFactor * 4);
		      localTable[ii+1].offset = offsetAccum;
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
		      }
		      else if((dataInfo.tp[ii].tpnum >= daqRange.filtExMin) &&
			 (dataInfo.tp[ii].tpnum < daqRange.filtExMax))
		      /* This is a filter module excitation input */
		      {
			/* Mark as coming from a filter module excitation input */
			localTable[ii].type = 2;
			/* Mark filter module number */
			localTable[ii].fmNum =  dataInfo.tp[ii].tpnum - daqRange.filtExMin;
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
		      for(jj=0;jj<MAX_HISTRY;jj++) dHistory[ii][jj] = 0.0;
		    }

		    tpStart = offsetAccum;
		    totalChans = dataInfo.numChans;

	    }
	}
      // Check for new TP
      // This will cause new TP to be written to local memory at start of 1 sec block.
      if(daqBlockNum == 15)
      {
 	totalChans = dataInfo.numChans;
	totalSize = mnDaqSize;
	offsetAccum = tpStart;
	validTp = 0;
	if(sysRate == DAQ_2K_SAMPLE_SIZE)
	{
		tpx = 3;
		tpAdd = 32;
	}
	else 
	{
		tpx = 2;
		tpAdd = sysRate / 4;
	}

	/* See how many testpoints we've got in the table (with the gaps) */
	/* I.e. find the last non-zero testpoint number */
	unsigned tplimit;
	for(tplimit = GM_DAQ_MAX_TPS; tplimit; tplimit--) {
	  // See if find the last test point number
	  if (gdsPtr->tp[tpx][0][tplimit - 1]) break;
	  // Clear tp monitoring slot 
	  if ((tplimit - 1) < 24) gdsMonitor[tplimit - 1] = 0;
	}

	for(ii=0; validTp < GM_DAQ_MAX_TPS && ii < tplimit; ii++)
	{
		/* Get TP number from shared memory */
		testVal = gdsPtr->tp[tpx][0][ii];

		/* Check if the TP selection is valid for this front end's filter module TP
		   If it is, load the local table with the proper lookup values to find the
		   signal and clear the TP status in shared mem.                           */
        	if(((testVal >= daqRange.filtTpMin) &&
           	    (testVal < daqRange.filtTpMax)) || testVal == 0)
        	{
		  if (testVal == 0) jj = 0;
		  else jj = testVal - daqRange.filtTpMin;
		  localTable[totalChans].type = 0;
          	  localTable[totalChans].sysNum = jj / daqRange.filtTpSize;
		  jj -= localTable[ii].sysNum * daqRange.filtTpSize;
          	  localTable[totalChans].fmNum = jj / 3;
          	  localTable[totalChans].sigNum = jj % 3; 
	  	  localTable[totalChans].decFactor = 1;
      		  dataInfo.tp[totalChans].dataType = 4;
		  offsetAccum += sysRate * 4;
		  localTable[totalChans+1].offset = offsetAccum;
		  if (ii < 24) {
          	    gdsMonitor[ii] = testVal;
		  }
          	  gdsPtr->tp[tpx][1][ii] = 0;
		  tpNum[validTp] = testVal;
		  validTp ++;
		  totalChans ++;
		  totalSize += tpAdd;
        	}

		/* Check if the TP selection is valid for other front end TP signals.
		   If it is, load the local table with the proper lookup values to find the
		   signal and clear the TP status in RFM.                                       */
		else if( (testVal >= daqRange.xTpMin) &&
			 (testVal < daqRange.xTpMax))
		{
	 	  jj = testVal - daqRange.xTpMin;
		  localTable[totalChans].type = 1;
		  localTable[totalChans].sigNum = jj;
		  offsetAccum += sysRate * 4;
		  localTable[totalChans+1].offset = offsetAccum;
	  	  localTable[totalChans].decFactor = 1;
      		  dataInfo.tp[totalChans].dataType = 4;
		  if (ii < 24) {
		    gdsMonitor[ii] = testVal;
		  }
		  gdsPtr->tp[tpx][1][ii] = 0;
		  tpNum[validTp] = testVal;
		  validTp ++;
		  totalChans ++;
		  totalSize += tpAdd;
		}
		/* If this TP select is not valid for this front end, deselect it in the local
		   lookup table.                                                                */
		else
		{
		  if (ii < 24) {
		    gdsMonitor[ii] = 0;
		  }
		}

	}  /* End for loop */

	// Check list of EXC channels
	validEx = 0;
	if(sysRate == DAQ_2K_SAMPLE_SIZE)
		tpx = 1;
	else tpx = 0;

        for(tplimit = GM_DAQ_MAX_TPS; tplimit; tplimit--) {
          // See if find the last test point number
          if (gdsPtr->tp[tpx][0][tplimit - 1]) break;
          // Clear tp monitoring slot 
          if ((tplimit - 1) < 8) gdsMonitor[24 + tplimit - 1] = 0;
        }

	for(ii=0; validTp < GM_DAQ_MAX_TPS && ii < tplimit; ii++)
	{
		/* Get EXC number from shared memory */
		testVal = gdsPtr->tp[tpx][0][ii];

		// Have to clear an exc if it goes away
		if((testVal != excTable[ii].sigNum) && (excTable[ii].sigNum != 0))
		{
		  dspPtr->data[excTable[ii].fmNum].exciteInput = 0.0;
		  excTable[ii].sigNum = 0;
		}

		/* Check if the EXC selection is valid for this front end's filter module EXC 
		   If it is, load the local table with the proper lookup values to find the
		   signal and clear the EXC status in shared mem.                           */
        	if((testVal >= daqRange.filtExMin) &&
           	   (testVal < daqRange.filtExMax))
        	{
		  jj = testVal - daqRange.filtExMin;
		  localTable[totalChans].type = 2;
          	  localTable[totalChans].sysNum = jj / daqRange.filtExSize;
          	  localTable[totalChans].fmNum = jj % daqRange.filtExSize;
          	  localTable[totalChans].sigNum = ii; 
	  	  localTable[totalChans].decFactor = 1;
		  excTable[ii].sigNum = testVal;
		  excTable[ii].sysNum = localTable[totalChans].sysNum;
		  excTable[ii].fmNum = localTable[totalChans].fmNum;
      		  dataInfo.tp[totalChans].dataType = 4;
		  offsetAccum += sysRate * 4;
		  localTable[totalChans+1].offset = offsetAccum;
		  if (ii < 8) {
          	    gdsMonitor[ii+24] = testVal;
		  }
          	  gdsPtr->tp[tpx][1][ii] = 0;
		  tpNum[validTp] = testVal;
		  validTp ++;
		  if(!validEx) validEx = totalChans;
		  totalChans ++;
		  totalSize += tpAdd;
        	}

		/* Check if the EXC selection is valid for other front end EXC signals.
		   If it is, load the local table with the proper lookup values to find the
		   signal and clear the EXC status in shmem.                               */
		else if( (testVal >= daqRange.xExMin) &&
			 (testVal < daqRange.xExMax))
		{
	 	  jj = testVal - daqRange.xExMin;
		  localTable[totalChans].type = 3;
          	  localTable[totalChans].fmNum = jj;
		  localTable[totalChans].sigNum = ii;
		  offsetAccum += sysRate * 4;
		  localTable[totalChans+1].offset = offsetAccum;
	  	  localTable[totalChans].decFactor = 1;
      		  dataInfo.tp[totalChans].dataType = 4;
		  if (ii < 8) {
		    gdsMonitor[ii+24] = testVal;
		  }
		  gdsPtr->tp[tpx][1][ii] = 0;
		  tpNum[validTp] = testVal;
		  validTp ++;
		  if(!validEx) validEx = totalChans;
		  totalChans ++;
		  totalSize += tpAdd;
		}

		/* If this EXC select is not valid for this front end, deselect it in the local
		   lookup table.                                                                */
		else
		{
		  if (ii < 8) {
		    gdsMonitor[ii+24] = 0;
		  }
		}

	 }  /* End for loop */
      } /* End normal check for new TP numbers */

      // Network write is one cycle behind memory write, so now update tp nums for FB xmission
      if(daqBlockNum == 0)
      {
	for(ii=0;ii<validTp;ii++)
		tpNumNet[ii] = tpNum[ii];
	validTpNet = validTp;
	totalSizeNet = totalSize;
      }

    } /* End done 16Hz Cycle */

  } /* End case write */
  /* Return the FE total DAQ data rate */
  return((totalSize*256)/1000);

}
