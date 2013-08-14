///	@file daqLib.c                                                	
///	@brief	This module provides the generic routines for: \n
///<		- Writing DAQ, GDS TP and GDS EXC signals to the
///<		  Framebuilder. \n
///<		- Reading GDS EXC signals from shmem and writing them
///<		  to the requested front end code exc channels.

volatile DAQ_INFO_BLOCK *pInfo;		///< Ptr to DAQ config in shmem.

extern volatile char *_epics_shm;	///< Ptr to EPICS shmem block
extern long daqBuffer;			///< Address of daqLib swing buffers.
extern char *_daq_shm;			///< Pointer to DAQ base address in shared memory.
struct rmIpcStr *dipc;			///< Pointer to DAQ IPC data in shared memory.
struct cdsDaqNetGdsTpNum *tpPtr;	///< Pointer to TP table in shared memory.
char *mcPtr;				///< Pointer to current DAQ data in shared memory.
float *testPtr;				///< Pointer to current DAQ data in shared memory.
int *testPtrI;				///< Pointer to current DAQ data in shared memory.
int *readPtrI;
char *lmPtr;				///< Pointer to current DAQ data in local memory data buffer.
char *daqShmPtr;			///< Pointer to DAQ data in shared memory.
int fillSize;				///< Amount of data to copy local to shared memory.
char *pEpicsIntData;
double *pEpicsDblData;
unsigned int curDaqBlockSize;


/* ******************************************************************** */
/* Routine to connect and write to LIGO DAQ system       		*/
/* ******************************************************************** */
///	@brief This function provides for reading GDS TP/EXC and writing DAQ data.
///<	For DAQ data, this routine also provides all necessary decimation filtering.

///	For additional information in LIGO DCC, see
/// <a href="https://dcc.ligo.org/cgi-bin/private/DocDB/ShowDocument?docid=8037">T0900638 CDS Real-time DAQ Software</a>
///	@param[in] flag Initialization flag 
///	@param[in] dcuId		DAQ Data unit ID - unique within a control system
///	@param[in] daqRange		Struct defining fron end valid test point and exc ranges.
///	@param[in] sysRate		Data rate of the code / 16
///	@param[in] *pFloatData[]	Pointer to TP data not associated with filter modules.
///	@param[in] *dspPtr		Pointer to array of filter module data.
///	@param[in] netStatus		Status of DAQ network
///	@param[out] gdsMonitor[]	Array to return values of GDS TP/EXC selections.
///	@param[out] excSignals[]	Array to write EXC signals not associated with filter modules.
///	@return	Total size of data transmitted in KB/sec.

int daqWrite(int flag,
	     int dcuId,
	     DAQ_RANGE daqRange,
	     int sysRate,
	     double *pFloatData[],
	     FILT_MOD *dspPtr,
	     int netStatus,
	     int gdsMonitor[],
	     double excSignal[],
	     char *pEpics)
{
int ii,jj;			/* Loop counters.			*/
int status;			/* Return value from called routines.	*/
double dWord;			/* Temp value for storage of DAQ values */
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
unsigned int *pInteger = 0;	/* Temp ptr to write unsigned int data. */
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
static int validTp = 0;		/* Number of valid GDS sigs selected.	*/
static int validTpNet = 0;		/* Number of valid GDS sigs selected.	*/
static int validEx;		/* EXC signal set status indicator.	*/
static int tpNum[DAQ_GDS_MAX_TP_ALLOWED]; 	/* TP/EXC selects to send to FB.	*/
static int tpNumNet[DAQ_GDS_MAX_TP_ALLOWED]; 	/* TP/EXC selects to send to FB.	*/
static int totalChans;		/* DAQ + TP + EXC chans selected.	*/
static int totalSize;		/* DAQ + TP + EXC chans size in bytes.	*/
static int totalSizeNet;        /* DAQ + TP + EXC chans size in bytes sent to network driver.  */
int *statusPtr;
volatile float *dataPtr;	/* Ptr to excitation chan data.		*/
int exChanOffset;		/* shmem offset to next EXC value.	*/
int tpx;
static int buf_size;
int i;
int ltSlot;
unsigned int exc;
unsigned int tpn;
int slot;
int num_tps;
static int epicsIntXferSize;

#ifdef CORE_BIQUAD
// Decimation filter coefficient definitions.		
static double dCoeff2x[13] =
	{0.014605318489015,
	0.0061330534633199, -0.3067718521410701, 1.0061297523961799, 1.6898970546512500,
	-0.1416634327119900, -0.7218542081231900, 1.1639392962956800, 1.5641813375750999,
	-0.2223002987598800, -1.1002072247518702, 2.4322961493728101, 2.5543892233808201};
static double dCoeff4x[13] =
	{0.0032897561126272,
	0.5262606025434300, -0.0761411615790100, 0.1130468884239699, 0.5092319101840799,
	0.5730930806734700, -0.1812069602474000, 0.4535162982982299, 0.6837579627174200,
	0.6560226277436600, -0.2732748286521299, 1.9218491283142201, 1.9903219392643201};
static double dCoeff8x[13] =
	{0.0019451746049824,
	0.7581968768203300, -0.0208065724495400, -0.0885007357744900, 0.1313472736383900,
	0.8177667403664499, -0.0484926355391700, 0.0804638221493899, 0.1881713856561398,
	0.8916285940607900, -0.0710046059171400, 1.0789961400704899, 0.9517899355931499};
static double dCoeff16x[13] =
	{0.0010292496296221,
	0.8821795273490400, -0.0050811632845900, -0.0793910648843901, 0.0333482444819799,
	0.9213891226266799, -0.0116267556958600, -0.0147463176669200, 0.0522378040105400,
	0.9631231120650900, -0.0166459584776699, 0.3524249070977401, 0.3726558365549801};
static double dCoeff32x[13] =
	{0.00099066651652901,
	0.9407723671890900, -0.0013021996687700, -0.0495970976842000, 0.0083283354579400,
	0.9629941014830901, -0.0029486095232200, -0.0209238527730700, 0.0131334362206199,
	0.9856499106827501, -0.0041756491626799, 0.0901459629393901, 0.1003204030939602};
static double dCoeff64x[13] =
	{9.117708813402705e-05,  
	0.9884411147668981, -0.0002966277054097, -0.0097345801570645, 0.0015276773706276,
	0.9958880030565560, -0.0005974359311846, 0.0001273101399277, 0.0036418711521875,
	-0.0079580196372524, -0.0079580196372524, 1.9920419803627476, 1.9920419803627476};
static double dCoeff128x[13] =
	{4.580254440937838e-05, 
	0.9942785038368447, -0.0000743648654229, -0.0052652981867765, 0.0003818331109557,
	0.9980915472248755, -0.0001494958409671, -0.0008478627563595, 0.0009110941777979,
	-0.0039867920479109, -0.0039867920479109, 1.9960132079520891, 1.9960132079520891};
static double dCoeff256x[13] =
	{2.296084727953743e-05, 
	0.9971538121386385, -0.0000186174099586, -0.0027321307580492, 0.0000954396933539,
	0.9990827274780281, -0.0000373907471043, -0.0006520772714058, 0.0002278045034618,
	-0.0019953660573223, -0.0019953660573223, 1.9980046339426778, 1.9980046339426778};
#else
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
static double dCoeff64x[13] =
	{9.117708813402705e-05,  
	-1.9884411147668981,  0.9887377424723078, -1.9981756949239626,  1.0000000000000000,
        -1.9958880030565560,  0.9964854389877406, -1.9957606929166283,  1.0000000000000002,
        -0.9920419803627476,  0.0000000000000000,  1.0000000000000000,  0.0000000000000000};
static double dCoeff128x[13] =
	{4.580254440937838e-05, 
	-1.9942785038368447,  0.9943528687022676, -1.9995438020236211,  0.9999999999999998,
        -1.9980915472248755,  0.9982410430658426, -1.9989394099812350,  1.0000000000000000,
        -0.9960132079520891,  0.0000000000000000,  1.0000000000000000,  0.0000000000000000};
static double dCoeff256x[13] =
	{2.296084727953743e-05, 
	-1.9971538121386385,  0.9971724295485971, -1.9998859428966878,  1.0000000000000002,
        -1.9990827274780281,  0.9991201182251324, -1.9997348047494339,  0.9999999999999999,
        -0.9980046339426777,  0.0000000000000000,  1.0000000000000000,  0.0000000000000000};
#endif

// History buffers for decimation IIR filters
static double dHistory[DCU_MAX_CHANNELS][MAX_HISTRY];




// ************************************************************************************** 
  if(flag == DAQ_CONNECT)	/* Initialize DAQ connection */
  {

    /* First block to write out is last from previous second */
    phase = 0;
    daqBlockNum = (DAQ_NUM_DATA_BLOCKS - 1);
    excBlockNum = 0;

    /* Allocate memory for two local data swing buffers */
    for(ii=0;ii<2;ii++) 
    {
      pDaqBuffer[ii] = (char *)daqBuffer;
      pDaqBuffer[ii] += DAQ_DCU_SIZE * ii;
      // printf("DAQ buffer %d is at 0x%x\n",ii,(long long)pDaqBuffer[ii]);
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

    daqShmPtr = _daq_shm + CDS_DAQ_NET_DATA_OFFSET;
    buf_size = DAQ_DCU_BLOCK_SIZE*2;
    dipc = (struct rmIpcStr *)(_daq_shm + CDS_DAQ_NET_IPC_OFFSET);
    tpPtr = (struct cdsDaqNetGdsTpNum *)(_daq_shm + CDS_DAQ_NET_GDS_TP_TABLE_OFFSET);
    mcPtr = daqShmPtr;
    lmPtr = pReadBuffer;
    fillSize = DAQ_DCU_BLOCK_SIZE / sysRate;
    printf("DIRECT MEMORY MODE of size %d\n",fillSize);


    crcLength = 0;
    printf("daqLib DCU_ID = %d\n",dcuId);

    /* Set up pointer to DAQ configuration information in shmem */
    pInfo = (DAQ_INFO_BLOCK *)(_epics_shm + DAQ_INFO_ADDRESS);

    // Setup EPICS channel information/pointers
    dataInfo.numEpicsInts = pInfo->numEpicsInts;
    dataInfo.numEpicsFloats = pInfo->numEpicsFloats;
    dataInfo.numEpicsFilts = pInfo->numEpicsFilts;
    dataInfo.numEpicsTotal = pInfo->numEpicsTotal;
    pEpicsIntData = pEpics;
    epicsIntXferSize = dataInfo.numEpicsInts * 4;
    pEpicsDblData = (pEpicsIntData + epicsIntXferSize);

    printf("DAQ DATA INFO is at 0x%x\n",(long)pInfo);
    printf("DAQ EPICS INT DATA is at 0x%x with size %d\n",(long)pEpicsIntData,epicsIntXferSize);
    printf("DAQ EPICS FLT DATA is at 0x%x\n",(long)pEpicsDblData);


    /* Get the .INI file crc checksum to pass to DAQ Framebuilders for config checking */
    fileCrc = pInfo->configFileCRC;
    // Clear the reconfiguration flag in shmem.
    pInfo->reconfig = 0;
    /* Get the number of channels to be acquired */
    dataInfo.numChans = pInfo->numChans;
    printf("EPICS: Int = %d  Flt = %d Filters = %d Total = %d Fast = %d\n",dataInfo.numEpicsInts,dataInfo.numEpicsFloats,dataInfo.numEpicsFilts, dataInfo.numEpicsTotal, dataInfo.numChans);
    // Verify number of channels is legal
    if((dataInfo.numChans < 1) || (dataInfo.numChans > DCU_MAX_CHANNELS))
    {
	printf("Invalid num daq chans = %d\n",dataInfo.numChans);
	return(-1);
    }

    // Initialize CRC length with EPICS data size.
    crcLength = 4 * dataInfo.numEpicsTotal;
printf("crc length epics = %d\n",crcLength);

    /* Get the DAQ configuration information for all channels and calc a crc checksum length */
    for(ii=0;ii<dataInfo.numChans;ii++)
    {
      dataInfo.tp[ii].tpnum = pInfo->tp[ii].tpnum;
      dataInfo.tp[ii].dataType = pInfo->tp[ii].dataType;
      dataInfo.tp[ii].dataRate = pInfo->tp[ii].dataRate;
      if(dataInfo.tp[ii].dataType == DAQ_DATATYPE_16BIT_INT)
        crcLength += 2 * dataInfo.tp[ii].dataRate / DAQ_NUM_DATA_BLOCKS;
      else
        crcLength += 4 * dataInfo.tp[ii].dataRate / DAQ_NUM_DATA_BLOCKS;
    }

    /* Calculate the number of bytes to xfer on each call, based on total number
       of bytes to write each 1/16sec and the front end data rate (2048/16384Hz) */
    // Note, this is left over from RFM net. Now used only to calc CRC
    xferSize1 = crcLength/sysRate;
    // mnDaqSize = crcLength/DAQ_NUM_DATA_BLOCKS;
    mnDaqSize = crcLength;
    // curDaqBlockSize = mnDaqSize * 256;
    curDaqBlockSize = crcLength * DAQ_NUM_DATA_BLOCKS_PER_SECOND;
    totalSize = crcLength;
    totalSizeNet = crcLength;

printf (" xfer sizes = %d %d %d %d \n",sysRate,xferSize1,totalSize,crcLength);
    
    if (xferSize1 == 0) {
	printf("DAQ size too small\n");
	return -1;
    }
/*    if (xferSize1 == 0) xferSize1 = 8;  ???? */

    /* 	Maintain 8 byte data boundaries for writing data, particularly important
        when DMA xfers are used on 5565 RFM modules. Note that this usually results
	in data not being written on every 2048/16384 cycle and last data xfer
	in a 1/16 sec block well may be shorter than the rest.			*/
 	xferSize1 = ((xferSize1/8) + 1) * 8;
	printf("DAQ resized %d\n", xferSize1);



    // Fill in the local lookup table for finding data.
    // offsetAccum = 0;
    offsetAccum = 4 * dataInfo.numEpicsTotal;
    localTable[0].offset = offsetAccum;

printf("Fast data offset = %d \n",offsetAccum);

    for(ii=0;ii<dataInfo.numChans;ii++)
    {
      /* Need to develop a table of offset pointers to load data into swing buffers 	*/
      /* This is based on decimation factors and data size				*/
      if ((dataInfo.tp[ii].dataRate / DAQ_NUM_DATA_BLOCKS) > sysRate) {
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
      // :TODO: this is broken, needs some work to make extra EXC work.
      else if((dataInfo.tp[ii].tpnum >= daqRange.xExMin) &&
	 (dataInfo.tp[ii].tpnum < daqRange.xExMax))
      /* This exc testpoint is not part of a filter module */
      {
	jj = dataInfo.tp[ii].tpnum - daqRange.xExMin;
	/* Mark as a non filter module testpoint */
	localTable[ii].type = 3;
	/* Mark the offset into the local data buffer */
	localTable[ii].fmNum = jj;
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
    // Clear out the GDS TP selections.
    if (sysRate < DAQ_16K_SAMPLE_SIZE) tpx = 3; else tpx = 2;
    for (ii=0; ii < DAQ_GDS_MAX_TP_ALLOWED; ii++) gdsPtr->tp[tpx][0][ii] = 0;
    if(sysRate < DAQ_16K_SAMPLE_SIZE) tpx = 1; else tpx = 0;
    for (ii=0; ii < DAQ_GDS_MAX_TP_ALLOWED; ii++) gdsPtr->tp[tpx][0][ii] = 0;
    // Following can be uncommented for testing.
    // gdsPtr->tp[3][0][0] = 31250;
    // gdsPtr->tp[3][0][1] = 11002;
    // gdsPtr->tp[0][0][0] = 5000;

    // Set pointer to EXC data in shmem.
    if(sysRate < DAQ_16K_SAMPLE_SIZE)
	{
    	exciteDataPtr = (char *)(_epics_shm + DATA_OFFSET_DCU(DCU_ID_EX_2K));
	}
    else {
    	exciteDataPtr = (char *)(_epics_shm + DATA_OFFSET_DCU(DCU_ID_EX_16K));
    }
    excDataSize = 4 + 4 * sysRate;

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


    for (i = 0; i < DAQ_GDS_MAX_TP_ALLOWED; i++) {
	tpNum[i] = 0;
	tpNumNet[i] = 0;
    }
    validTp = 0;
    validTpNet = 0;

    //printf("at connect TPnum[0]=%d\n", tpNum[0]);
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
        //printf("TPnum[0]=%d\n", tpNum[0]);
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
      if(localTable[ii].type == DAQ_SRC_FM_TP)
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
      else if(localTable[ii].type == DAQ_SRC_NFM_TP)
      /* Data is from non filter module  testpoint */
      {
	    dWord = *(pFloatData[localTable[ii].sigNum]);
      }
      else if(localTable[ii].type == DAQ_SRC_FM_EXC)
      /* Data is from filter module excitation */
      {
	    dWord = dspPtr->data[localTable[ii].fmNum].exciteInput;
      } else if (localTable[ii].type == DAQ_SRC_NFM_EXC) {
	      // Extra excitation
	      dWord = excSignal[localTable[ii].fmNum];
      }

      // Perform decimation filtering, if required.
#ifdef CORE_BIQUAD
#define iir_filter iir_filter_biquad
#endif
      if(dataInfo.tp[ii].dataType != DAQ_DATATYPE_32BIT_INT)
      {
	      if(localTable[ii].decFactor == 2) dWord = iir_filter(dWord,&dCoeff2x[0],DTAPS,&dHistory[ii][0]);
	      if(localTable[ii].decFactor == 4) dWord = iir_filter(dWord,&dCoeff4x[0],DTAPS,&dHistory[ii][0]);
	      if(localTable[ii].decFactor == 8) dWord = iir_filter(dWord,&dCoeff8x[0],DTAPS,&dHistory[ii][0]);
	      if(localTable[ii].decFactor == 16) dWord = iir_filter(dWord,&dCoeff16x[0],DTAPS,&dHistory[ii][0]);
	      if(localTable[ii].decFactor == 32) dWord = iir_filter(dWord,&dCoeff32x[0],DTAPS,&dHistory[ii][0]);
	      if(localTable[ii].decFactor == 64) dWord = iir_filter(dWord,&dCoeff64x[0],DTAPS,&dHistory[ii][0]);
	      if(localTable[ii].decFactor == 128) dWord = iir_filter(dWord,&dCoeff128x[0],DTAPS,&dHistory[ii][0]);
	      if(localTable[ii].decFactor == 256) dWord = iir_filter(dWord,&dCoeff256x[0],DTAPS,&dHistory[ii][0]);
      }
#ifdef CORE_BIQUAD
#undef iir_filter
#endif

      // Write the data into the swing buffer.
      if ((daqSlot % localTable[ii].decFactor) == 0) {
      	if (dataInfo.tp[ii].dataType == DAQ_DATATYPE_16BIT_INT) {
	  // Write short data; (XOR 1) here provides sample swapping
	  ((short *)(pWriteBuffer + localTable[ii].offset))[(daqSlot/localTable[ii].decFactor)^1] = (short)dWord;

        } else if (dataInfo.tp[ii].dataType == DAQ_DATATYPE_32BIT_INT) {
	  if (localTable[ii].decFactor == 1)
	    ((unsigned int *)(pWriteBuffer + localTable[ii].offset))[daqSlot] = ((unsigned int)dWord);
	  else 
	    ((unsigned int *)(pWriteBuffer + localTable[ii].offset))[daqSlot/localTable[ii].decFactor]
		= ((unsigned int)dWord) & *((unsigned int *)(dHistory[ii]));
	} else {
	  // Write a 32-bit float (downcast from the double passed)
	  ((float *)(pWriteBuffer + localTable[ii].offset))[daqSlot/localTable[ii].decFactor] = (float)dWord;
      	}
      } else if (dataInfo.tp[ii].dataType == DAQ_DATATYPE_32BIT_INT) {
        if ((daqSlot % localTable[ii].decFactor) == 1)
	  *((unsigned int *)(dHistory[ii])) = (unsigned int)dWord;
 	else
	  *((unsigned int *)(dHistory[ii])) &= (unsigned int)dWord;
      }
    } /* end swing buffer write loop */

    // Copy data from read buffer to shared memory
    memcpy(mcPtr,lmPtr,fillSize);
    mcPtr += fillSize;
    lmPtr += fillSize;

if(daqSlot == 0)
{
// Write EPICS integer values
      memcpy(pWriteBuffer,pEpicsIntData,epicsIntXferSize);
}
if(daqSlot == 41)
{
// Write EPICS double values as float values
    pEpicsDblData = (pEpicsIntData + epicsIntXferSize);
    	// testPtr = (pWriteBuffer + epicsIntXferSize); 
    	testPtr = (float *)pWriteBuffer; 
    	testPtr += dataInfo.numEpicsInts; 
 	for(ii=0;ii<dataInfo.numEpicsFloats;ii++)
	{
		*testPtr = (float)*pEpicsDblData;
		testPtr ++;
		pEpicsDblData ++;
	}
}
if(daqSlot == 42)
{
// Write filter module EPICS values as floats
 	for(ii=0;ii<MAX_MODULES;ii++)
	{
		*testPtr = (float)dspPtr->data[ii].filterInput;;
		testPtr ++;
		// *testPtr = (float)dspPtr->data[ii].exciteInput;;
		// testPtr ++;
		*testPtr = (float)dspPtr->inputs[ii].offset;;
		testPtr ++;
		*testPtr = (float)dspPtr->inputs[ii].outgain;;
		testPtr ++;
		*testPtr = (float)dspPtr->inputs[ii].limiter;;
		testPtr ++;
		*testPtr = (float)dspPtr->data[ii].output16Hz;;
		testPtr ++;
		*testPtr = (float)dspPtr->data[ii].output;;
		testPtr ++;
		*testPtr = (float)dspPtr->data[ii].swStatus;;
		testPtr ++;
		*testPtr = (float)dspPtr->inputs[ii].swReq;;
		testPtr ++;
		*testPtr = (float)dspPtr->inputs[ii].swMask;;
		testPtr ++;
	}
}
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

  // Read in any selected EXC signals.
  excSlot = (excSlot + 1) % sysRate;
  //if(validEx)
  validEx = 0;
  {
	// Go through all test points
  	for(ii=dataInfo.numChans;ii<totalChans;ii++)
  	{
		// Do not pickup any testpoints (type 0 or 1)
		if (localTable[ii].type < 2) continue;

		exChanOffset = localTable[ii].sigNum * excDataSize;
		statusPtr = (int *)(exciteDataPtr + excBlockNum * DAQ_DCU_BLOCK_SIZE + exChanOffset);
		if(*statusPtr == 0)
		{
			validEx = FE_ERROR_EXC_SET;
			dataPtr = (float *)(exciteDataPtr + excBlockNum * DAQ_DCU_BLOCK_SIZE + exChanOffset +
					    excSlot * 4 + 4);
			if(localTable[ii].type == DAQ_SRC_FM_EXC)
			{
				dspPtr->data[localTable[ii].fmNum].exciteInput = *dataPtr;
			} else if (localTable[ii].type == DAQ_SRC_NFM_EXC) {
				// extra excitation
				excSignal[localTable[ii].fmNum] = *dataPtr;
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

// Assign global parameters
        dipc->dcuId = dcuId; // DCU id of this system
        dipc->crc = fileCrc; // Checksum of the configuration file
        // dipc->dataBlockSize = crcLength; // actual data size
        dipc->dataBlockSize = totalSizeNet * DAQ_NUM_DATA_BLOCKS_PER_SECOND; // actual data size
        // Assign current block parameters
        dipc->bp[daqBlockNum].cycle = daqBlockNum;
        dipc->bp[daqBlockNum].crc = crcSend;
        //ipc->bp[daqBlockNum].status = 0;
	if (daqBlockNum == DAQ_NUM_DATA_BLOCKS_PER_SECOND - 1) {
        	dipc->bp[daqBlockNum].timeSec = ((unsigned int) cycle_gps_time - 1);
	} else {
        	dipc->bp[daqBlockNum].timeSec = (unsigned int) cycle_gps_time;
	}
        dipc->bp[daqBlockNum].timeNSec = (unsigned int)daqBlockNum;

        // Assign the test points table
        tpPtr->count = validTpNet | validEx;
        memcpy(tpPtr->tpNum, tpNumNet, sizeof(tpNumNet[0]) * validTp);

        // As the last step set the cycle counter
        // Frame builder is looking for cycle change
        dipc->cycle =daqBlockNum; // Ready cycle (16 Hz)


      /* Increment the 1/16 sec block counter */
      daqBlockNum = (daqBlockNum + 1) % DAQ_NUM_DATA_BLOCKS_PER_SECOND;

      mcPtr = daqShmPtr;
      mcPtr += buf_size * daqBlockNum;
      lmPtr = pReadBuffer;

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

		    // Setup EPICS channel information/pointers
    dataInfo.numEpicsInts = pInfo->numEpicsInts;
    dataInfo.numEpicsFloats = pInfo->numEpicsFloats;
    dataInfo.numEpicsFilts = pInfo->numEpicsFilts;
    dataInfo.numEpicsTotal = pInfo->numEpicsTotal;
    pEpicsIntData = pEpics;
    epicsIntXferSize = dataInfo.numEpicsInts * 4;
    pEpicsDblData = (pEpicsIntData + epicsIntXferSize);

    printf("DAQ DATA INFO is at 0x%x\n",(long)pInfo);
    printf("DAQ EPICS INT DATA is at 0x%x with size %d\n",(long)pEpicsIntData,epicsIntXferSize);
    printf("DAQ EPICS FLT DATA is at 0x%x\n",(long)pEpicsDblData);
    printf("EPICS: Int = %d  Flt = %d Filters = %d Total = %d Fast = %d\n",dataInfo.numEpicsInts,dataInfo.numEpicsFloats,dataInfo.numEpicsFilts, dataInfo.numEpicsTotal, dataInfo.numChans);

	    // Initialize CRC length with EPICS data size.
    crcLength = 4 * dataInfo.numEpicsTotal;
	printf("crc length epics = %d\n",crcLength);


		    /* Get the DAQ configuration information for all channels and calc a crc checksum length */
		    for(ii=0;ii<dataInfo.numChans;ii++)
		    {
		      dataInfo.tp[ii].tpnum = pInfo->tp[ii].tpnum;
		      dataInfo.tp[ii].dataType = pInfo->tp[ii].dataType;
		      dataInfo.tp[ii].dataRate = pInfo->tp[ii].dataRate;
		      if(dataInfo.tp[ii].dataType == DAQ_DATATYPE_16BIT_INT)
			crcLength += 2 * dataInfo.tp[ii].dataRate / DAQ_NUM_DATA_BLOCKS_PER_SECOND;
		      else
			crcLength += 4 * dataInfo.tp[ii].dataRate / DAQ_NUM_DATA_BLOCKS_PER_SECOND;
if(dataInfo.tp[ii].dataType == DAQ_DATATYPE_32BIT_INT) printf("Found int32 type \n");
		    }
		    /* Calculate the number of bytes to xfer on each call, based on total number
		       of bytes to write each 1/16sec and the front end data rate (2048/16384Hz) */
		    xferSize1 = crcLength/sysRate;
    		    // mnDaqSize = crcLength/16;
    		    mnDaqSize = crcLength;
    		    totalSize = crcLength;

		    /*  Maintain 8 byte data boundaries for writing data, particularly important
			when DMA xfers are used on 5565 RFM modules. Note that this usually results
			in data not being written on every 2048/16384 cycle and last data xfer
			in a 1/16 sec block well may be shorter than the rest.                  */
		    xferSize1 = ((xferSize1/8) + 1) * 8;

		    offsetAccum = 4 * pInfo->numEpicsTotal;
		    localTable[0].offset = offsetAccum;

		    for(ii=0;ii<dataInfo.numChans;ii++)
		    {
		      /* Need to develop a table of offset pointers to load data into swing buffers     */
		      /* This is based on decimation factors and data size                              */
		      localTable[ii].decFactor = sysRate/(dataInfo.tp[ii].dataRate / DAQ_NUM_DATA_BLOCKS_PER_SECOND);
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
			localTable[ii].fmNum = jj / DAQ_NUM_FM_TP;
			/* Mark which of three testpoints to store */
			localTable[ii].sigNum = jj % DAQ_NUM_FM_TP;
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
	unsigned int tpnum[DAQ_GDS_MAX_TP_ALLOWED];		// Current TP nums
	unsigned int excnum[DAQ_GDS_MAX_TP_ALLOWED];	// Current EXC nums
	// Offset by one into the TP/EXC tables for the 2K systems
	unsigned int _2k_sys_offs = sysRate < DAQ_16K_SAMPLE_SIZE;
	
	// Helper function to search the lists
	// Clears the found number from the lists
	// tpnum and excnum lists of numbers do not intersect
	inline int in_the_lists(unsigned int tp, unsigned int slot) {
		int i;
		for (i = 0; i < DAQ_GDS_MAX_TP_ALLOWED; i++){
			if (tpnum[i] == tp) return (tpnum[i] = 0, 1);
                        if (excnum[i] == tp) {
                                // Check if the excitation is still in the same slot
                                if (i != excTable[slot].offset) return 0;
                                return (excnum[i] = 0, 1);
                        }
		}
		return 0;
	}

	// Helper function to find an empty slot in the localTable
	inline unsigned int empty_slot(void) {
		int i;
		for (i = 0; i < DAQ_GDS_MAX_TP_ALLOWED; i++) {
			if (tpNum[i] == 0) return i;	
		}
		return -1;
	}

	// Copy TP/EXC tables into my local memory
	memcpy(excnum, (const void *)(gdsPtr->tp[_2k_sys_offs][0]), sizeof(excnum));
	memcpy(tpnum, (const void *)(gdsPtr->tp[2 + _2k_sys_offs][0]), sizeof(tpnum));

        //printf("TPnum[0]=%d\n", tpNum[0]);
        //printf("excnum[0]=%d\n", excnum[0]);
        //printf("tpnum[0]=%d\n", tpnum[0]);

	// Search and clear deselected test points
	for (i = 0; i < DAQ_GDS_MAX_TP_ALLOWED; i++) {
		if (tpNum[i] == 0) continue;
		if (!in_the_lists(tpNum[i], i)) {
		  tpNum[i] = 0; // Removed test point is cleared now
		  ltSlot = dataInfo.numChans + i;

		  // If we are clearing an EXC signal, reset filter module input
		  if (localTable[ltSlot].type == DAQ_SRC_FM_EXC) {
		    dspPtr->data[excTable[i].fmNum].exciteInput = 0.0;
		    excTable[i].sigNum = 0;
		  } else if (localTable[ltSlot].type == DAQ_SRC_NFM_EXC) {
		    // Extra excitation
		    excSignal[excTable[i].fmNum] = 0.0;
		    excTable[i].sigNum = 0;
		  }

		  localTable[ltSlot].type = 0;
          	  localTable[ltSlot].sysNum = 0;
          	  localTable[ltSlot].fmNum = 0;
          	  localTable[ltSlot].sigNum = 0;
	  	  localTable[ltSlot].decFactor = 1;
      		  dataInfo.tp[ltSlot].dataType = DAQ_DATATYPE_FLOAT;
		}
	}
	
	// tpnum and excnum lists now have only the new test points
	// Insert these new numbers into empty localTable slots
	for (i = 0; i < (2 * DAQ_GDS_MAX_TP_ALLOWED); i++) {
		exc = 0;
		// Do test points first
		if (i < DAQ_GDS_MAX_TP_ALLOWED) {
			if (tpnum[i] == 0) continue;
			tpn = tpnum[i];
		} else {
			if (excnum[i - DAQ_GDS_MAX_TP_ALLOWED] == 0) continue;
			tpn = excnum[i - DAQ_GDS_MAX_TP_ALLOWED];
			exc = 1;
			ii = i - DAQ_GDS_MAX_TP_ALLOWED;
		}

        	//printf("tpn=%d at %d\n", tpn, i);
		slot = empty_slot();
		if (slot < 0) {
			// No more slots left, table's full
			break;
		}

		// localTable slot (shifted by the number of DAQ channels)
		ltSlot = dataInfo.numChans + slot;

		// Populate the slot with the information
		if (!exc) {
       		  if (tpn >= daqRange.filtTpMin && tpn < daqRange.filtTpMax) {
		    jj = tpn - daqRange.filtTpMin;
		    localTable[ltSlot].type = DAQ_SRC_FM_TP;
          	    localTable[ltSlot].sysNum = jj / daqRange.filtTpSize;
		    jj -= localTable[ltSlot].sysNum * daqRange.filtTpSize;
          	    localTable[ltSlot].fmNum = jj / DAQ_NUM_FM_TP;
          	    localTable[ltSlot].sigNum = jj % DAQ_NUM_FM_TP; 
	  	    localTable[ltSlot].decFactor = 1;
      		    dataInfo.tp[ltSlot].dataType = DAQ_DATATYPE_FLOAT;

		    // Need to recalculate offsets later
		    //offsetAccum += sysRate * 4;
		    //localTable[totalChans+1].offset = offsetAccum;

		    //if (slot < 24) gdsMonitor[slot] = tpn;
          	    gdsPtr->tp[2 + _2k_sys_offs][1][slot] = 0;
		    tpNum[slot] = tpn;

        	  } else if (tpn >= daqRange.xTpMin && tpn < daqRange.xTpMax) {
	 	    jj = tpn - daqRange.xTpMin;
		    localTable[ltSlot].type = DAQ_SRC_NFM_TP;
		    localTable[ltSlot].sigNum = jj;
	  	    localTable[ltSlot].decFactor = 1;
      		    dataInfo.tp[ltSlot].dataType = DAQ_DATATYPE_FLOAT;
		    gdsPtr->tp[2 + _2k_sys_offs][1][slot] = 0;
		    tpNum[slot] = tpn;
		  
		  }
	        } else {

        	  if (tpn >= daqRange.filtExMin && tpn < daqRange.filtExMax) {
		    jj = tpn - daqRange.filtExMin;
		    localTable[ltSlot].type = DAQ_SRC_FM_EXC;
          	    localTable[ltSlot].sysNum = jj / daqRange.filtExSize;
          	    localTable[ltSlot].fmNum = jj % daqRange.filtExSize;
          	    localTable[ltSlot].sigNum = ii;
	  	    localTable[ltSlot].decFactor = 1;
		    excTable[slot].sigNum = tpn;
		    excTable[slot].sysNum = localTable[ltSlot].sysNum;
		    excTable[slot].fmNum = localTable[ltSlot].fmNum;
                    // Save the index into the TPman table
                    excTable[slot].offset = i - DAQ_GDS_MAX_TP_ALLOWED;
      		    dataInfo.tp[ltSlot].dataType = DAQ_DATATYPE_FLOAT;

          	    gdsPtr->tp[_2k_sys_offs][1][slot] = 0;
		    tpNum[slot] = tpn;

        	  } else if (tpn >= daqRange.xExMin && tpn < daqRange.xExMax) {
	 	    jj = tpn - daqRange.xExMin;
		    localTable[ltSlot].type = DAQ_SRC_NFM_EXC;
          	    localTable[ltSlot].fmNum = jj;
          	    localTable[ltSlot].sigNum = slot;
		    //offsetAccum += sysRate * 4;
		    //localTable[totalChans+1].offset = offsetAccum;
	  	    localTable[ltSlot].decFactor = 1;
		    excTable[slot].sigNum = tpn;
		    excTable[slot].sysNum = localTable[ltSlot].sysNum;
		    excTable[slot].fmNum = localTable[ltSlot].fmNum;
      		    dataInfo.tp[ltSlot].dataType = DAQ_DATATYPE_FLOAT;
		   // if (slot < 8) gdsMonitor[slot + 24] = tpn;
          	    gdsPtr->tp[_2k_sys_offs][1][slot] = 0;
		    tpNum[slot] = tpn;
		  }
		}
	}

	// Calculate total number of test points to transmit
	totalChans = dataInfo.numChans; // Set to the DAQ channels number
	validTp = 0;
	// totalSize = mnDaqSize;
	num_tps = 0;

	// Skip empty slots at the end
	for (i = DAQ_GDS_MAX_TP_ALLOWED-1; i >= 0; i--) {
		if (tpNum[i]) {
			num_tps = i + 1;
			break;
		}
	}
	totalChans += num_tps;
	validTp = num_tps;

	curDaqBlockSize = crcLength * DAQ_NUM_DATA_BLOCKS_PER_SECOND;
	
	// Calculate the total transmission size
	totalSize = crcLength + validTp * sysRate * 4;

	//printf("totalSize=%d; totalChans=%d; validTp=%d\n", totalSize, totalChans, validTp);

	// Assign offsets into the localTable
	offsetAccum = tpStart;
	for (i = 0; i < validTp; i++) {
	    localTable[dataInfo.numChans + i].offset = offsetAccum;
	    offsetAccum += sysRate * 4;
	    if (i < 32) gdsMonitor[i] = tpNum[i];
	}

	for (i = validTp; i < 32; i++) gdsMonitor[i] = 0;

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
  return((totalSize*DAQ_NUM_DATA_BLOCKS_PER_SECOND)/1000);

}
