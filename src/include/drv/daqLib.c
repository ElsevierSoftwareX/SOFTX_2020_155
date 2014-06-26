/*!	\file daqLib.c                                                	
 *	\brief File contains routines to support DAQ on realtime systems. \n
 *	\author R.Bork, A. Ivanov
*/

volatile DAQ_INFO_BLOCK *pInfo;		///< Ptr to DAQ config in shmem.
extern volatile char *_epics_shm;	///< Ptr to EPICS shmem block
extern long daqBuffer;			///< Address of daqLib swing buffers.
extern char *_daq_shm;			///< Pointer to DAQ base address in shared memory.
struct rmIpcStr *dipc;			///< Pointer to DAQ IPC data in shared memory.
struct cdsDaqNetGdsTpNum *tpPtr;	///< Pointer to TP table in shared memory.
char *mcPtr;				///< Pointer to current DAQ data in shared memory.
char *lmPtr;				///< Pointer to current DAQ data in local memory data buffer.
char *daqShmPtr;			///< Pointer to DAQ data in shared memory.
int fillSize;				///< Amount of data to copy local to shared memory.
char *pEpicsIntData;			///< Pointer to EPICS integer type data in shared memory.
char *pEpicsDblData;			///< Pointer to EPICS double type data in shared memory.
unsigned int curDaqBlockSize;		///< Total DAQ data rate diag
// Added to get EPICS data for RCG V2.8
char *pEpicsInt;				// Pointer to current DAQ data in shared memory.
char *pEpicsInt1;
float *pEpicsFloat;				// Pointer to current DAQ data in shared memory.
double *pEpicsDblData1;

int daqConfig(struct DAQ_INFO_BLOCK *, struct DAQ_INFO_BLOCK *, char *);
int loadLocalTable(DAQ_XFER_INFO *, DAQ_LKUP_TABLE [], int , DAQ_INFO_BLOCK *, DAQ_RANGE *);
int daqWrite(int,int,struct DAQ_RANGE,int,double *[],struct FILT_MOD *,int,int [],double [],char *);

/* ******************************************************************** */
/* Routine to connect and write to LIGO DAQ system       		*/
/* ******************************************************************** */
///	@author R.Bork, A. Ivanov\n
///	@brief This function provides for reading GDS TP/EXC and writing DAQ data. \n
///	@detail For additional information in LIGO DCC, see <a href="https://dcc.ligo.org/cgi-bin/private/DocDB/ShowDocument?docid=8037">T0900638 CDS Real-time DAQ Software</a>
///	@param[in] flag 		Initialization flag (1=Init call, 0 = run)
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
int ii,jj,kk;			/* Loop counters.			*/
int status;			/* Return value from called routines.	*/
double dWord;			/* Temp value for storage of DAQ values */
static int daqBlockNum;		/* 1-16, tracks DAQ block to write to.	*/
static int excBlockNum;		/* 1-16, tracks EXC block to read from.	*/
static int excDataSize;
static DAQ_XFER_INFO xferInfo;
static int xferDone;
static char *pDaqBuffer[DAQ_NUM_SWING_BUFFERS];	/* Pointers to local swing buffers.	*/
static DAQ_LKUP_TABLE localTable[DCU_MAX_CHANNELS];
static DAQ_LKUP_TABLE excTable[DCU_MAX_CHANNELS];
static char *pWriteBuffer;	/* Ptr to swing buff to write data	*/
static char *pReadBuffer;	/* Ptr to swing buff to xmit data to FB */
static int phase;		/* 0-1, switches swing buffers.		*/
static int daqSlot;		/* 0-sysRate, data slot to write data	*/
static int excSlot;		/* 0-sysRate, slot to read exc data	*/
char *bufPtr;			/* Ptr to data for crc calculation.	*/
static unsigned int crcTest;	/* Continuous calc of CRC.		*/
static unsigned int crcSend;	/* CRC sent to FB.			*/
static DAQ_INFO_BLOCK dataInfo; /* Local DAQ config info buffer.	*/
static int tpStart;		/* Marks address of first TP data	*/
static volatile GDS_CNTRL_BLOCK *gdsPtr;  /* Ptr to shm to pass TP info to DAQ */
static volatile char *exciteDataPtr;	  /* Ptr to EXC data in shmem.	*/
static int validTp = 0;		/* Number of valid GDS sigs selected.	*/
static int validTpNet = 0;		/* Number of valid GDS sigs selected.	*/
static int validEx;		/* EXC signal set status indicator.	*/
static int tpNum[DAQ_GDS_MAX_TP_ALLOWED]; 	/* TP/EXC selects to send to FB.	*/
static int tpNumNet[DAQ_GDS_MAX_TP_ALLOWED]; 	/* TP/EXC selects to send to FB.	*/
static int totalChans;		/* DAQ + TP + EXC chans selected.	*/
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
unsigned int tpnum[DAQ_GDS_MAX_TP_ALLOWED];		// Current TP nums
unsigned int excnum[DAQ_GDS_MAX_TP_ALLOWED];	// Current EXC nums

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
/// If flag input is 1, then this is a startup initialization request from controller code.
  if(flag == DAQ_CONNECT)	/* Initialize DAQ connection */
  {

    /* First block to write out is last from previous second */
    phase = 0;
    daqBlockNum = (DAQ_NUM_DATA_BLOCKS - 1);
    excBlockNum = 0;

    /// ** INITIALIZATION **************\n
    /// \> Two local memory swing buffers will be used to store acquired data, each containing
    /// 1/16 sec of data.\n
    /// ----  One for writing ie copying/filtering data from shared memory on each cycle. \n
    /// ----  One for reading data ie copying data from local memory to memory shared with 
    /// DAQ network communication software. \n
    /// \> Buffers switch roles every 1/16 sec. \n
    /// \> Introduces 1/16 sec delay on delivery of data to network. However, this is done for
    /// performance reasons ie it takes too long to perform CRC checksum on full data set and
    /// to get the data out. With this method, the computing load can be spread out on a per
    /// per cycle basis. \n
    /// \>\> Allocate memory for two local data swing buffers
    for(ii=0;ii<DAQ_NUM_SWING_BUFFERS;ii++) 
    {
      pDaqBuffer[ii] = (char *)daqBuffer;
      pDaqBuffer[ii] += DAQ_DCU_SIZE * ii;
    }
    /// \>\> Set pointers to two swing buffers
    pWriteBuffer = (char *)pDaqBuffer[phase^1];
    pReadBuffer = (char *)pDaqBuffer[phase];
    daqSlot = -1;
    excSlot = 0;

    ///	\> CDS standard IIR filters will be used for decimation filtering from
    /// the native application rate down to DAQ sample rate. \n 
    /// \>\> Need to clear the decimation filter histories.
    for(ii=0;ii<DCU_MAX_CHANNELS;ii++)
	for(jj=0;jj<MAX_HISTRY;jj++)
	     dHistory[ii][jj] = 0.0;

    /// \> Setup Pointers to the various shared memories:\n
    /// ----  Assign Ptr to data shared memory to network driver (mx_stream) \n
    daqShmPtr = _daq_shm + CDS_DAQ_NET_DATA_OFFSET;
    buf_size = DAQ_DCU_BLOCK_SIZE*DAQ_NUM_SWING_BUFFERS;
    /// ----  Setup Ptr to interprocess comms with network driver
    dipc = (struct rmIpcStr *)(_daq_shm + CDS_DAQ_NET_IPC_OFFSET);
    /// ----  Setup Ptr to awgtpman shared memory (TP number table)
    tpPtr = (struct cdsDaqNetGdsTpNum *)(_daq_shm + CDS_DAQ_NET_GDS_TP_TABLE_OFFSET);
    mcPtr = daqShmPtr;		// Set daq2net data ptr to start of daq2net shared memory.
    lmPtr = pReadBuffer;	// Set read buffer ptr to start of read buffer..

    // Set mem cpy size evenly over all code cycles.
    fillSize = DAQ_DCU_BLOCK_SIZE / sysRate;
    /// ----  Set up pointer to DAQ configuration information in shmem */
    pInfo = (DAQ_INFO_BLOCK *)(_epics_shm + DAQ_INFO_ADDRESS);
    /// ----  Set pointer to shared mem to pass GDS info to DAQ
    gdsPtr = (GDS_CNTRL_BLOCK *)(_epics_shm + DAQ_GDS_BLOCK_ADD);
    // Set pointer to EXC data in shmem.
    if(sysRate < DAQ_16K_SAMPLE_SIZE)
	{
    	exciteDataPtr = (char *)(_epics_shm + DATA_OFFSET_DCU(DCU_ID_EX_2K));
	}
    else {
    	exciteDataPtr = (char *)(_epics_shm + DATA_OFFSET_DCU(DCU_ID_EX_16K));
    }
    excDataSize = 4 + 4 * sysRate;


    // Clear the reconfiguration flag in shmem.
    pInfo->reconfig = 0;

    // Configure data channels *****************************************************
    // Return error if configuration is incorrect.
    /// \> Load DAQ configuration info from memory shared with EPICS
    if((xferInfo.crcLength = daqConfig(&dataInfo,pInfo,pEpics)) == -1) return(-1);

    /// \> Load local table information with channel info
    if((status = loadLocalTable(&xferInfo,localTable,sysRate,&dataInfo,&daqRange)) == -1) return(-1);

    // Set the start of TP data after DAQ data.
    tpStart = xferInfo.offsetAccum;
    totalChans = dataInfo.numChans;

    /// \> Clear out the GDS TP selections.
    if (sysRate < DAQ_16K_SAMPLE_SIZE) tpx = 3; else tpx = 2;
    for (ii=0; ii < DAQ_GDS_MAX_TP_ALLOWED; ii++) gdsPtr->tp[tpx][0][ii] = 0;
    if(sysRate < DAQ_16K_SAMPLE_SIZE) tpx = 1; else tpx = 0;
    for (ii=0; ii < DAQ_GDS_MAX_TP_ALLOWED; ii++) gdsPtr->tp[tpx][0][ii] = 0;

    /// \> Clear the GDS TP lookup table
    for (i = 0; i < DAQ_GDS_MAX_TP_ALLOWED; i++) {
	tpNum[i] = 0;
	tpNumNet[i] = 0;
    }
    validTp = 0;
    validTpNet = 0;

    //printf("at connect TPnum[0]=%d\n", tpNum[0]);
  } ///  End DAQ CONNECT INITIALIZATION ******************************

/* ******************************************************************************** */
/* Write Data to FB 			******************************************* */
/* ******************************************************************************** */
/// If flag=0, data is to be acquired. This is called every code cycle by controller.c
/// ** Data Acquisition Mode ********************************************************
  if(flag == DAQ_WRITE)
  {
    /// \> Calc data offset into current write swing buffer 
    daqSlot = (daqSlot + 1) % sysRate;

    /// \> At start of 1/16 sec. data block, reset the xfer sizes and done bit */
    if(daqSlot == 0)
    {
	xferInfo.xferLength = xferInfo.crcLength;
	xferInfo.xferSize = xferInfo.xferSize1;
	xferDone = 0;
    }

    /// \> If size of data remaining to be sent is less than calc xfer size, reduce xfer size
    ///   to that of data remaining and mark that data xfer is complete for this 1/16 sec block 
    if((xferInfo.xferLength < xferInfo.xferSize1) && (!xferDone))
    {
	xferInfo.xferSize = xferInfo.xferLength;
	if(xferInfo.xferSize <= 0)  xferDone = 1;

    }


    /// \> Write data into local swing buffer 
    for(ii=0;ii<totalChans;ii++)
    {

      dWord = 0;
      /// \> Read data to a local variable, either from a FM TP or other TP */
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

      /// \> Perform decimation filtering, if required.
#ifdef CORE_BIQUAD
#define iir_filter iir_filter_biquad
#endif
      if(dataInfo.tp[ii].dataType != DAQ_DATATYPE_32BIT_UINT)
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

      /// \> Write fast data into the swing buffer.
      if ((daqSlot % localTable[ii].decFactor) == 0) {
      	if (dataInfo.tp[ii].dataType == DAQ_DATATYPE_16BIT_INT) {
	  // Write short data; (XOR 1) here provides sample swapping
	  ((short *)(pWriteBuffer + localTable[ii].offset))[(daqSlot/localTable[ii].decFactor)^1] = (short)dWord;

        } else if (dataInfo.tp[ii].dataType == DAQ_DATATYPE_32BIT_INT) {
	  // Write a 32-bit int (downcast from the double passed)
	  ((int *)(pWriteBuffer + localTable[ii].offset))[daqSlot/localTable[ii].decFactor] = (int)dWord;
        } else if (dataInfo.tp[ii].dataType == DAQ_DATATYPE_32BIT_UINT) {
	  if (localTable[ii].decFactor == 1)
	    ((unsigned int *)(pWriteBuffer + localTable[ii].offset))[daqSlot] = ((unsigned int)dWord);
	  else 
	    ((unsigned int *)(pWriteBuffer + localTable[ii].offset))[daqSlot/localTable[ii].decFactor]
		= ((unsigned int)dWord) & *((unsigned int *)(dHistory[ii]));
	} else {
	  // Write a 32-bit float (downcast from the double passed)
	  ((float *)(pWriteBuffer + localTable[ii].offset))[daqSlot/localTable[ii].decFactor] = (float)dWord;
      	}
      } else if (dataInfo.tp[ii].dataType == DAQ_DATATYPE_32BIT_UINT) {
        if ((daqSlot % localTable[ii].decFactor) == 1)
	  *((unsigned int *)(dHistory[ii])) = (unsigned int)dWord;
 	else
	  *((unsigned int *)(dHistory[ii])) &= (unsigned int)dWord;
      }
    } /* end swing buffer write loop */

/// \> Write EPICS data into swing buffer at 16Hz.
if(daqSlot == DAQ_XFER_CYCLE_INT)
{
/// \>\> On 16Hz boundary: \n
/// - ----  Write EPICS integer values to beginning of local write buffer

      if(dataInfo.cpyepics2times)
	{
	      memcpy(pWriteBuffer,pEpicsIntData,dataInfo.cpyIntSize[0]);
	      pEpicsInt = pWriteBuffer;
	      pEpicsInt += dataInfo.cpyIntSize[0];
	      pEpicsInt1 = pEpicsIntData + dataInfo.cpyIntSize[0] + 4;
	      memcpy(pEpicsInt,pEpicsInt1,dataInfo.cpyIntSize[1]);
	} else {
	      memcpy(pWriteBuffer,pEpicsIntData,dataInfo.cpyIntSize[0]);
	}

}
if(daqSlot == DAQ_XFER_CYCLE_DBL)
{
/// - ---- Write EPICS double values as float values after EPICS integer type data.
    	pEpicsDblData1 = (double *)pEpicsDblData;
    	pEpicsFloat = (float *)pWriteBuffer; 
    	pEpicsFloat += dataInfo.numEpicsInts; 
 	for(ii=0;ii<dataInfo.numEpicsFloats;ii++)
	{
		*pEpicsFloat = (float)*pEpicsDblData1;
		pEpicsFloat ++;
		pEpicsDblData1 ++;
	}
}
if((daqSlot >= DAQ_XFER_CYCLE_FMD) && (daqSlot < dataInfo.numEpicsFiltXfers))
{
/// \>\> On 16Hz boundary + 1 (or more) cycle(s): \n
/// - ----  Write filter module EPICS values as floats
	jj = DAQ_XFER_FMD_PER_CYCLE * (daqSlot - DAQ_XFER_CYCLE_FMD);
	if(daqSlot == (dataInfo.numEpicsFiltXfers - 1))
	{
		kk = jj + dataInfo.numEpicsFiltsLast;
	} else {
		kk = jj + DAQ_XFER_FMD_PER_CYCLE;
	}
	// printf("Cycle = %d jj = %d kk = %d\n",daqSlot,jj,kk);
 	for(ii=jj;ii<kk;ii++)
	{
		*pEpicsFloat = (float)dspPtr->inputs[ii].offset;
		pEpicsFloat ++;
		*pEpicsFloat = (float)dspPtr->inputs[ii].outgain;;
		pEpicsFloat ++;
		*pEpicsFloat = (float)dspPtr->inputs[ii].limiter;;
		pEpicsFloat ++;
		*pEpicsFloat = (float)dspPtr->inputs[ii].gain_ramp_time;;
		pEpicsFloat ++;
		*pEpicsFloat = (float)dspPtr->inputs[ii].swReq;;
		pEpicsFloat ++;
		*pEpicsFloat = (float)dspPtr->inputs[ii].swMask;;
		pEpicsFloat ++;
		*pEpicsFloat = (float)dspPtr->data[ii].filterInput;;
		pEpicsFloat ++;
		*pEpicsFloat = (float)dspPtr->data[ii].exciteInput;;
		pEpicsFloat ++;
		*pEpicsFloat = (float)dspPtr->data[ii].output16Hz;;
		pEpicsFloat ++;
		*pEpicsFloat = (float)dspPtr->data[ii].output;;
		pEpicsFloat ++;
		*pEpicsFloat = (float)dspPtr->data[ii].swStatus;;
		pEpicsFloat ++;
	}
}

    /// \> Copy data from read buffer to DAQ network shared memory on each code cycle.
    /// - ---- Entire buffer is moved during 1/16sec period, in equal amounts each cycle, even though
    /// entire buffer is not full of data.
    memcpy(mcPtr,lmPtr,fillSize);
    mcPtr += fillSize;
    lmPtr += fillSize;

  /// - ----  Perform CRC checksum on data moved from read buffer to DAQ network shared memory.
  if(!xferDone)
  {
    /* Do CRC check sum calculation */
    bufPtr = (char *)pReadBuffer + daqSlot * xferInfo.xferSize1;
    if(daqSlot == 0) crcTest = crc_ptr(bufPtr, xferInfo.xferSize, 0);
    else crcTest = crc_ptr(bufPtr, xferInfo.xferSize, crcTest);
    xferInfo.xferLength -= xferInfo.xferSize;
  }
  if(daqSlot == (sysRate - 1))
  /* Done with 1/16 second data block */
  {
      /* Complete CRC checksum calc */
      crcTest = crc_len(xferInfo.crcLength, crcTest);
      crcSend = crcTest;
  }

  /// \> Read in any selected EXC signals. \n
  /// --- NOTE: EXC signals have to be read and loaded in advance by 1 cycle ie must be loaded and
  /// available to the realtime application when the next code cycle is initiated.
  excSlot = (excSlot + 1) % sysRate;
  //if(validEx)
  validEx = 0;
  {
	// Go through all test points
  	for(ii=dataInfo.numChans;ii<totalChans;ii++)
  	{
		// Do not pickup any testpoints (type 0 or 1)
		if (localTable[ii].type < DAQ_SRC_FM_EXC) continue;

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

  /// \> Move to the next 1/16 EXC signal data block if end of 16Hz block
  if(excSlot == (sysRate - 1)) excBlockNum = (excBlockNum + 1) % DAQ_NUM_DATA_BLOCKS;

    /// \> If last cycle of a 16Hz block:
    if(daqSlot == (sysRate - 1))
    /* Done with 1/16 second DAQ data block */
    {

      /// - -- Swap swing buffers 
      phase = (phase + 1) % DAQ_NUM_SWING_BUFFERS;
      pReadBuffer = (char *)pDaqBuffer[phase];
      pWriteBuffer = (char *)pDaqBuffer[(phase^1)];

	/// - -- Fill in the IPC table for DAQ network driver (mx_stream) \n
        dipc->dcuId = dcuId; /// -   ------ DCU id of this system
        dipc->crc = xferInfo.fileCrc; /// -   ------ Checksum of the configuration file
        dipc->dataBlockSize = xferInfo.totalSizeNet; /// -   ------ Actual data size
        /// -   ------  Data block number
        dipc->bp[daqBlockNum].cycle = daqBlockNum;
        /// -   ------  Data block CRC
        dipc->bp[daqBlockNum].crc = crcSend;
	/// -   ------ Timestamp GPS Second
	if (daqBlockNum == DAQ_NUM_DATA_BLOCKS_PER_SECOND - 1) {
        	dipc->bp[daqBlockNum].timeSec = ((unsigned int) cycle_gps_time - 1);
	} else {
        	dipc->bp[daqBlockNum].timeSec = (unsigned int) cycle_gps_time;
	}
	/// -   ------ Timestamp GPS nanoSecond
        dipc->bp[daqBlockNum].timeNSec = (unsigned int)daqBlockNum;

        /// - ------ Write test point info to DAQ net shared memory
        tpPtr->count = validTpNet | validEx;
        memcpy(tpPtr->tpNum, tpNumNet, sizeof(tpNumNet[0]) * validTp);

        // As the last step set the cycle counter
        // Frame builder is looking for cycle change
	/// - ------ Write IPC cycle number. This will trigger DAQ network driver to send data to DAQ
        dipc->cycle =daqBlockNum; // Ready cycle (16 Hz)


      /// - -- Increment the 1/16 sec block counter
      daqBlockNum = (daqBlockNum + 1) % DAQ_NUM_DATA_BLOCKS_PER_SECOND;

      /// - -- Reset pointers to DAQ net shared memory and local read buffer.
      mcPtr = daqShmPtr;
      mcPtr += buf_size * daqBlockNum;
      lmPtr = pReadBuffer;

      /// - -- Check for reconfig request at start of each second
      if((pInfo->reconfig == 1) && (daqBlockNum == 0))
      {
	    printf("New daq config\n");
	    pInfo->reconfig = 0;
	    // Configure EPICS data channels
	    xferInfo.crcLength = daqConfig(&dataInfo,pInfo,pEpics);
	    if(xferInfo.crcLength)
	    {
		    status = loadLocalTable(&xferInfo,localTable,sysRate,&dataInfo,&daqRange);
		    // Clear decimation filter history
		    for(ii=0;ii<dataInfo.numChans;ii++)
		    {
		      for(jj=0;jj<MAX_HISTRY;jj++) dHistory[ii][jj] = 0.0;
		    }

		    tpStart = xferInfo.offsetAccum;
		    totalChans = dataInfo.numChans;

	    }
      }
      /// - -- If last cycle of 1 sec time frame, check for new TP and load info.
      // This will cause new TP to be written to local memory at start of 1 sec block.

      if(daqBlockNum == 15)
      {
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

	/// - ------ Search and clear deselected test points
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

	/// - ------ Calculate total number of test points to transmit
	totalChans = dataInfo.numChans; // Set to the DAQ channels number
	validTp = 0;
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

	/// - ------ Calculate the total transmission size (DAQ +TP)
	xferInfo.totalSize = xferInfo.crcLength + validTp * sysRate * 4;


	// Assign offsets into the localTable
	xferInfo.offsetAccum = tpStart;
	for (i = 0; i < validTp; i++) {
	    localTable[dataInfo.numChans + i].offset = xferInfo.offsetAccum;
	    xferInfo.offsetAccum += sysRate * 4;
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
	xferInfo.totalSizeNet = xferInfo.totalSize;
      }

    } /* End done 16Hz Cycle */

  } /* End case write */

  /// \> Return the FE total DAQ data rate */
  return((xferInfo.totalSize*DAQ_NUM_DATA_BLOCKS_PER_SECOND)/1000);

}

// **************************************************************************************
///	@author R.Bork\n
///	@brief This function updates the DAQ configuration from information
///< 		loaded from EPICS by the RCG EPICS sequencer.\n
///	@param[out] dataInfo	Pointer to DAQ local configuration table
///	@param[in] pInfo	Pointer to DAQ config info in shared memory
///	@param[in] pEpics	Pointer to beginning of EPICS data.
///	@return	Size, in bytes, of DAQ data.
// **************************************************************************************
int daqConfig( DAQ_INFO_BLOCK *dataInfo,  	
                DAQ_INFO_BLOCK *pInfo,	
		char *pEpics
              )
{
int ii,jj;      		// Loop counters
int epicsIntXferSize = 0;	// Size, in bytes, of EPICS integer type data.
int dataLength = 0;		// Total size, in bytes, of data to be sent
int status = 0;

/// \> Verify correct channel count before proceeding. \n
/// - ---- Required to be at least one and not more than DCU_MAX_CHANNELS.
    status = pInfo->numChans;
    if((status < 1) || (status > DCU_MAX_CHANNELS))
    {   
	printf("Invalid num daq chans = %d\n",status);
	return(-1);
    }

    /// \> Get the number of fast channels to be acquired 
    dataInfo->numChans = pInfo->numChans;

/// \> Setup EPICS channel information/pointers
    dataInfo->numEpicsInts = pInfo->numEpicsInts;
    dataInfo->numEpicsFloats = pInfo->numEpicsFloats;
    dataInfo->numEpicsFilts = pInfo->numEpicsFilts;
    dataInfo->numEpicsTotal = pInfo->numEpicsTotal;
/// \> Determine how many filter modules are to have their data transferred per cycle.
/// - ---- This is to balance load (CPU time) across several cycles if number of filter modules > 100
    dataInfo->numEpicsFiltXfers = MAX_MODULES/DAQ_XFER_FMD_PER_CYCLE;
    dataInfo->numEpicsFiltsLast = DAQ_XFER_FMD_PER_CYCLE;
    if(MAX_MODULES % DAQ_XFER_FMD_PER_CYCLE) 
    {
	dataInfo->numEpicsFiltXfers ++;
	dataInfo->numEpicsFiltsLast = (MAX_MODULES % DAQ_XFER_FMD_PER_CYCLE);
    }
    dataInfo->numEpicsFiltXfers += DAQ_XFER_CYCLE_FMD;
    pEpicsIntData = pEpics;
    epicsIntXferSize = dataInfo->numEpicsInts * 4;

    dataInfo->epicsdblDataOffset = 0;
    dataInfo->cpyepics2times = 0;
    dataInfo->cpyIntSize[0] = dataInfo->numEpicsInts * 4;
    dataInfo->cpyIntSize[1] = 0;

    ii = (sizeof(CDS_EPICS_OUT) / 4);
    jj = dataInfo->numEpicsInts - ii;
    printf("Have %d CDS epics integer and %d USR epics integer channels\n",ii,jj);
    ii *= 4;
    jj *= 4;

    /// \>  Look for memory holes ie integer types not ending on 8 byte boundary.
    /// Need to check both standard CDS struct and User channels
    /// - ---- If either doesn't end on 8 byte boundary, then a 4 byte hole will appear
    /// before the double type data in shared memory.
    if(ii % 8) {
        printf("Have int mem hole after CDS %d %d \n",ii,jj);
        dataInfo->epicsdblDataOffset += 4;
    }
    if(jj % 8) {
        printf("Have int mem hole after user %d %d \n",ii,jj);
        dataInfo->epicsdblDataOffset += 4;
    }
    if ((ii%8) &&(jj>0)) { 
        /// - ---- If standard CDS struct doesn't end on 8 byte boundary, then
        /// have 4 byte mem hole after CDS data
        /// This will require 2 memcpys of integer data to get 8 byte alignment for xfer to DAQ buffer..
        dataInfo->cpyIntSize[0] = ii;
        dataInfo->cpyIntSize[1] = jj;        
	dataInfo->cpyepics2times = 1;
        // dataInfo->epicsdblDataOffset += 4;
        printf("Have mem holes after CDS  %d %d \nNeed to cpy ints twice - size 1 = %d size 2 = %d \n",ii,jj,dataInfo->cpyIntSize[0],dataInfo->cpyIntSize[1]);
    }

    /// \> Set the pointer to start of EPICS double type data in shared memory. \n
    /// - ---- Ptr to double type data is at EPICS integer start + EPICS integer size + Memory holes
    pEpicsDblData = (pEpicsIntData + epicsIntXferSize + dataInfo->epicsdblDataOffset);

    // Send EPICS data diags to dmesg
    printf("DAQ EPICS INT DATA is at 0x%x with size %d\n",(long)pEpicsIntData,epicsIntXferSize);
    printf("DAQ EPICS FLT DATA is at 0x%x\n",(long)pEpicsDblData);
    printf("DAQ EPICS: Int = %d  Flt = %d Filters = %d Total = %d Fast = %d\n",dataInfo->numEpicsInts,dataInfo->numEpicsFloats,dataInfo->numEpicsFilts, dataInfo->numEpicsTotal, dataInfo->numChans);
    printf("DAQ EPICS: Number of Filter Module Xfers = %d last = %d\n",dataInfo->numEpicsFiltXfers,dataInfo->numEpicsFiltsLast);
    /// \> Initialize CRC length with EPICS data size.
    dataLength = 4 * dataInfo->numEpicsTotal;
    printf("crc length epics = %d\n",dataLength);

    /// \>  Get the DAQ configuration information for all fast DAQ channels and calc a crc checksum length
    for(ii=0;ii<dataInfo->numChans;ii++)
    {
      dataInfo->tp[ii].tpnum = pInfo->tp[ii].tpnum;
      dataInfo->tp[ii].dataType = pInfo->tp[ii].dataType;
      dataInfo->tp[ii].dataRate = pInfo->tp[ii].dataRate;
      if(dataInfo->tp[ii].dataType == DAQ_DATATYPE_16BIT_INT)
        dataLength += 2 * dataInfo->tp[ii].dataRate / DAQ_NUM_DATA_BLOCKS;
      else
        dataLength += 4 * dataInfo->tp[ii].dataRate / DAQ_NUM_DATA_BLOCKS;
    }
    /// \> Set DAQ bytes/sec global, which is output to EPICS by controller.c
    curDaqBlockSize = dataLength * DAQ_NUM_DATA_BLOCKS_PER_SECOND;

    /// \> RETURN dataLength, used in other code for CRC checksum byte count
    return(dataLength);

}

// **************************************************************************************
///	@author R.Bork\n
///	@brief This function populates the local DAQ/TP tables.
///	@param *pDxi	Pointer to struct with data transfer size information.
///	@param localTable[]	Table to be populated with data pointer information
///	@param sysRate			Number of code cycles in 1/16 second.
///	@param *dataInfo		DAQ configuration information
///	@param *daqRange		Info on GDS TP number ranges which provides type information 
///	@return	0=OK or -1=FAIL
// **************************************************************************************
int loadLocalTable(DAQ_XFER_INFO *pDxi,
		   DAQ_LKUP_TABLE localTable[],
		   int sysRate,
		   DAQ_INFO_BLOCK *dataInfo,
	     	   DAQ_RANGE *daqRange
		  )
{
int ii,jj;


    /// \> Get the .INI file crc checksum to pass to DAQ Framebuilders for config checking */
    pDxi->fileCrc = pInfo->configFileCRC;
    /// \> Calculate the number of bytes to xfer on each call, based on total number
    ///   of bytes to write each 1/16sec and the front end data rate (2048/16384Hz)
    pDxi->xferSize1 = pDxi->crcLength/sysRate;
    pDxi->totalSize = pDxi->crcLength;
    pDxi->totalSizeNet = pDxi->crcLength;

    printf (" xfer sizes = %d %d %d %d \n",sysRate,pDxi->xferSize1,pDxi->totalSize,pDxi->crcLength);
    
    if (pDxi->xferSize1 == 0) {
	printf("DAQ size too small\n");
	return -1;
    }

    /// \> Maintain 8 byte data boundaries for writing data, particularly important
    ///    when DMA xfers are used on 5565 RFM modules. Note that this usually results
    ///    in data not being written on every 2048/16384 cycle and last data xfer
    ///    in a 1/16 sec block well may be shorter than the rest.	
 	pDxi->xferSize1 = ((pDxi->xferSize1/8) + 1) * 8;
	printf("DAQ resized %d\n", pDxi->xferSize1);

   /// \> Find first memory location for fast data in read/write swing buffers.
    pDxi->offsetAccum = 4 * dataInfo->numEpicsTotal;
    localTable[0].offset = pDxi->offsetAccum;

   /// \> Fill in the local lookup table for finding data.
   /// - (Need to develop a table of offset pointers to load data into swing buffers)  \n 
   /// - (This is based on decimation factors and data size.)
for(ii=0;ii<dataInfo->numChans;ii++)
    {
      if ((dataInfo->tp[ii].dataRate / DAQ_NUM_DATA_BLOCKS) > sysRate) {
        /* Channel data rate is greater than system rate */
        printf("Channels %d has bad data rate %d\n", ii, dataInfo->tp[ii].dataRate);
        return(-1);
      } else {
      /// - ---- Load decimation factor
      localTable[ii].decFactor = sysRate/(dataInfo->tp[ii].dataRate / DAQ_NUM_DATA_BLOCKS);
      }

      /// - ---- Calc offset into swing buffer for writing data
      if(dataInfo->tp[ii].dataType == DAQ_DATATYPE_16BIT_INT)
        pDxi->offsetAccum += (sysRate/localTable[ii].decFactor * 2);
      else
        pDxi->offsetAccum += (sysRate/localTable[ii].decFactor * 4);

      localTable[ii+1].offset = pDxi->offsetAccum;
      /// - ----  Need to determine if data is from a filter module TP or non-FM TP and tag accordingly
      if((dataInfo->tp[ii].tpnum >= daqRange->filtTpMin) &&
         (dataInfo->tp[ii].tpnum < daqRange->filtTpMax))
      /* This is a filter module testpoint */
      {
        jj = dataInfo->tp[ii].tpnum - daqRange->filtTpMin;
        /* Mark as coming from a filter module testpoint */
        localTable[ii].type = DAQ_SRC_FM_TP;
        /* Mark which system filter module is in */
        localTable[ii].sysNum = jj / daqRange->filtTpSize;
        jj -= localTable[ii].sysNum * daqRange->filtTpSize;
        /* Mark which filter module within a system */
        localTable[ii].fmNum = jj / DAQ_NUM_FM_TP;
        /* Mark which of three testpoints to store */
        localTable[ii].sigNum = jj % DAQ_NUM_FM_TP;
      }
      else if((dataInfo->tp[ii].tpnum >= daqRange->filtExMin) &&
         (dataInfo->tp[ii].tpnum < daqRange->filtExMax))
      /* This is a filter module excitation input */
      {
        /* Mark as coming from a filter module excitation input */
        localTable[ii].type = DAQ_SRC_FM_EXC;
        /* Mark filter module number */
        localTable[ii].fmNum =  dataInfo->tp[ii].tpnum - daqRange->filtExMin;
      }
      else if((dataInfo->tp[ii].tpnum >= daqRange->xTpMin) &&
         (dataInfo->tp[ii].tpnum < daqRange->xTpMax))
      /* This testpoint is not part of a filter module */
      {
        jj = dataInfo->tp[ii].tpnum - daqRange->xTpMin;
        /* Mark as a non filter module testpoint */
        localTable[ii].type = DAQ_SRC_NFM_TP;
        /* Mark the offset into the local data buffer */
        localTable[ii].sigNum = jj;
      }
      // :TODO: this is broken, needs some work to make extra EXC work.
      else if((dataInfo->tp[ii].tpnum >= daqRange->xExMin) &&
         (dataInfo->tp[ii].tpnum < daqRange->xExMax))
      /* This exc testpoint is not part of a filter module */
      {
	jj = dataInfo->tp[ii].tpnum - daqRange->xExMin;
        /* Mark as a non filter module testpoint */
        localTable[ii].type = DAQ_SRC_NFM_EXC;
        /* Mark the offset into the local data buffer */
        localTable[ii].fmNum = jj;
        localTable[ii].sigNum = jj;
      }
      else
      {
        printf("Invalid chan num found %d = %d\n",ii,dataInfo->tp[ii].tpnum);
        return(-1);
      }
}
return(0);
/// \> RETURN 0=OK or -1=FAIL

}
