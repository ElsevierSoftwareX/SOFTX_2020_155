/*!	\file daqLib.c
 *	\brief File contains routines to support DAQ on realtime systems. \n
 *	\author R.Bork, A. Ivanov
*/

volatile DAQ_INFO_BLOCK *pInfo;   ///< Ptr to DAQ config in shmem.
extern volatile char *_epics_shm; ///< Ptr to EPICS shmem block
extern long daqBuffer;            ///< Address of daqLib swing buffers.
extern char *_daq_shm; ///< Pointer to DAQ base address in shared memory.
struct rmIpcStr *dipc; ///< Pointer to DAQ IPC data in shared memory.
struct cdsDaqNetGdsTpNum *tpPtr; ///< Pointer to TP table in shared memory.
char *daqShmPtr;                 ///< Pointer to DAQ data in shared memory.
char *pEpicsIntData; ///< Pointer to EPICS integer type data in shared memory.
char *pEpicsDblData; ///< Pointer to EPICS double type data in shared memory.
unsigned int curDaqBlockSize; ///< Total DAQ data rate diag
// Added to get EPICS data for RCG V2.8
char *pEpicsInt; // Pointer to current DAQ data in shared memory.
char *pEpicsInt1;
float *pEpicsFloat; // Pointer to current DAQ data in shared memory.
double *pEpicsDblData1;

int daqConfig(struct DAQ_INFO_BLOCK *, struct DAQ_INFO_BLOCK *, char *);
int loadLocalTable(DAQ_XFER_INFO *, DAQ_LKUP_TABLE[], int, DAQ_INFO_BLOCK *,
                   DAQ_RANGE *);
int daqWrite(int, int, struct DAQ_RANGE, int, double *[], struct FILT_MOD *,
             int, int[], double[], char *);

inline double htond(double in) {
  double retVal;
  char *p = (char *)&retVal;
  char *i = (char *)&in;
  p[0] = i[7];
  p[1] = i[6];
  p[2] = i[5];
  p[3] = i[4];

  p[4] = i[3];
  p[5] = i[2];
  p[6] = i[1];
  p[7] = i[0];

  return retVal;
}

/* ******************************************************************** */
/* Routine to connect and write to LIGO DAQ system       		*/
/* ******************************************************************** */
///	@author R.Bork, A. Ivanov\n
///	@brief This function provides for reading GDS TP/EXC and writing DAQ
///data. \n
///	@detail For additional information in LIGO DCC, see <a
///href="https://dcc.ligo.org/cgi-bin/private/DocDB/ShowDocument?docid=8037">T0900638
///CDS Real-time DAQ Software</a>
///	@param[in] flag 		Initialization flag (1=Init call, 0 =
///run)
///	@param[in] dcuId		DAQ Data unit ID - unique within a control
///system
///	@param[in] daqRange		Struct defining fron end valid test point
///and exc ranges.
///	@param[in] sysRate		Data rate of the code / 16
///	@param[in] *pFloatData[]	Pointer to TP data not associated with
///filter modules.
///	@param[in] *dspPtr		Pointer to array of filter module data.
///	@param[in] netStatus		Status of DAQ network
///	@param[out] gdsMonitor[]	Array to return values of GDS TP/EXC
///selections.
///	@param[out] excSignals[]	Array to write EXC signals not associated
///with filter modules.
///	@return	Total size of data transmitted in KB/sec.

int daqWrite(int flag, int dcuId, DAQ_RANGE daqRange, int sysRate,
             double *pFloatData[], FILT_MOD *dspPtr, int netStatus,
             int gdsMonitor[], double excSignal[], char *pEpics) {
  int ii, jj, kk; /* Loop counters.			*/
  int status;     /* Return value from called routines.	*/
  unsigned int mydatatype;
  double dWord;               /* Temp value for storage of DAQ values */
  static int daqBlockNum;     /* 1-16, tracks DAQ cycle.		*/
  static int daqXmitBlockNum; /* 1-16, tracks shmem DAQ block to write to.
                                 */
  static int excBlockNum;     /* 1-16, tracks EXC block to read from.	*/
  static int excDataSize;
  static DAQ_XFER_INFO xferInfo;
  static DAQ_LKUP_TABLE localTable[DCU_MAX_CHANNELS];
  static DAQ_LKUP_TABLE excTable[DCU_MAX_CHANNELS];
  static volatile char *pWriteBuffer; /* Ptr to swing buff to write data
                                         */
  static int phase;                   /* 0-1, switches swing buffers.		*/
  static int daqSlot;                 /* 0-sysRate, data slot to write data	*/
  static int excSlot;                 /* 0-sysRate, slot to read exc data	*/
  static DAQ_INFO_BLOCK dataInfo;     /* Local DAQ config info buffer.	*/
  static int tpStart;                 /* Marks address of first TP data	*/
  static volatile GDS_CNTRL_BLOCK
      *gdsPtr;                         /* Ptr to shm to pass TP info to DAQ */
  static volatile char *exciteDataPtr; /* Ptr to EXC data in shmem.	*/
  static int validTp = 0;              /* Number of valid GDS sigs selected.	*/
  static int validTpNet = 0;           /* Number of valid GDS sigs selected.	*/
  static int validEx;                  /* EXC signal set status indicator.	*/
  static int
      tpNum[DAQ_GDS_MAX_TP_ALLOWED]; /* TP/EXC selects to send to FB.	*/
  static int tpNumNet
      [DAQ_GDS_MAX_TP_ALLOWED]; /* TP/EXC selects to send to FB.	*/
  static int totalChans;        /* DAQ + TP + EXC chans selected.	*/
  int *statusPtr;
  volatile float *dataPtr; /* Ptr to excitation chan data.		*/
  int exChanOffset;        /* shmem offset to next EXC value.	*/
  int tpx;
  static int buf_size;
  int i;
  int ltSlot;
  unsigned int exc;
  unsigned int tpn;
  int slot;
  int num_tps;
  unsigned int tpnum[DAQ_GDS_MAX_TP_ALLOWED];  // Current TP nums
  unsigned int excnum[DAQ_GDS_MAX_TP_ALLOWED]; // Current EXC nums

#ifdef CORE_BIQUAD
  // BIQUAD Decimation filter coefficient definitions.
  // dCOEFF 2x
  // *************************************************************************
  static double dCoeff2x[13] = {
      0.02717257186578, -0.1159055409088,  -0.40753832312918, 2.66236735378793,
      3.37073457156755, -0.49505157452475, -1.10461941102831, 1.40184470311617,
      1.79227686661261, -0.74143396593712, -1.62740819248313, 0.72188475979666,
      0.83591053325065};

  // dCOEFF 4x
  // *************************************************************************
  static double dCoeff4x[13] = {
      0.00426219526013, 0.46640482430571, -0.10620935923005, 2.50932081620118,
      2.93670663266542, 0.43602772908265, -0.31016854747127, 0.75143527373544,
      1.00523899718152, 0.44571894955428, -0.47692045639835, 0.36664098129003,
      0.4440015753374};

  // dCOEFF 8x
  // *************************************************************************
  static double dCoeff8x[13] = {
      0.00162185538923, 0.73342532779703, -0.02862365091314, 1.44110125961504,
      1.67905228090487, 0.77657563380963, -0.08304311675394, 0.18851328163424,
      0.32889453107067, 0.83213081484618, -0.12573495191273, 0.0940911979108501,
      0.13622543115194};

  // dCOEFF 16x
  // *************************************************************************
  static double dCoeff16x[13] = {
      0.00112590539483,    0.86616831686611,   -0.00753654634986012,
      0.48586805026482,    0.61216318704885,   0.90508101474565,
      -0.0215349711544799, 0.0149886842581499, 0.08837269835802,
      0.94631370100442,    -0.0320417955561,   0.0141281606027401,
      0.0357726640422202};

  // dCOEFF 32x
  // *************************************************************************
  static double dCoeff32x[13] = {
      0.00102945292275,     0.93288074072411,     -0.00194092797014001,
      0.10751900551591,     0.17269733682166,     0.9570539169953,
      -0.00548573773340011, -0.0149997987966302,  0.0224605464746699,
      0.98100244642901,     -0.00807148639261013, -0.00189235941040011,
      0.00903370776797985};

  // dCOEFF 64x
  // *************************************************************************
  static double dCoeff64x[13] = {
      0.00101894798776,     0.96638168022541,    -0.000492974627960052,
      0.01147570619135,     0.04460105133798,    0.97969775930388,
      -0.00138449271550001, -0.0132857101503898, 0.00563203783023014,
      0.99249184543014,     -0.0020244997813601, -0.00322227927422025,
      0.00226137551427974};

  // dCOEFF 128x
  // *************************************************************************
  static double dCoeff128x[13] = {
      0.00102359688929,      0.98317523053482,      -0.000124254191099959,
      -0.00545789721985002,  0.01124261805423,      0.9901470788001,
      -0.000347773996469902, -0.00809690612593994,  0.00140824107749005,
      0.9967468102523,       -0.000506888877139899, -0.00218112074794985,
      0.000565180122610309};

  // dCOEFF 256x
  // *************************************************************************
  static double dCoeff256x[13] = {
      0.00102849104272,     0.99158359864769,      -3.11926170200039e-05,
      -0.00556878740432998, 0.00281642133096005,   0.99514878857652,
      -8.7150981279982e-05, -0.00441208984599983,  0.000351970596200069,
      0.99849900371282,     -0.000126813612729926, -0.00123294150072994,
      0.000141241173720053};
#else
  // SOS Decimation filter coefficient definitions.
  // dCOEFF 2x
  // *************************************************************************
  static double dCoeff2x[13] = {0.02717257186578,
                                -0.8840944590912,
                                0.29163278222038,
                                1.77827289469673,
                                1,
                                -0.50494842547525,
                                0.60956783650356,
                                0.89689627764092,
                                1,
                                -0.25856603406288,
                                0.88597422654601,
                                0.46331872573378,
                                1};

  // dCOEFF 4x
  // *************************************************************************
  static double dCoeff4x[13] = {0.00426219526013,
                                -1.46640482430571,
                                0.57261418353576,
                                1.04291599189547,
                                1,
                                -1.43602772908265,
                                0.74619627655392,
                                -0.68459245534721,
                                1,
                                -1.44571894955428,
                                0.92263940595263,
                                -1.07907796826425,
                                1};

  // dCOEFF 8x
  // *************************************************************************
  static double dCoeff8x[13] = {0.00162185538923,
                                -1.73342532779703,
                                0.76204897871017,
                                -0.29232406818199,
                                1,
                                -1.77657563380963,
                                0.85961875056357,
                                -1.58806235217539,
                                1,
                                -1.83213081484618,
                                0.95786576675891,
                                -1.73803961693533,
                                1};

  // dCOEFF 16x
  // *************************************************************************
  static double dCoeff16x[13] = {0.00112590539483,
                                 -1.86616831686611,
                                 0.87370486321597,
                                 -1.38030026660129,
                                 1,
                                 -1.90508101474565,
                                 0.92661598590013,
                                 -1.8900923304875,
                                 1,
                                 -1.94631370100442,
                                 0.97835549656052,
                                 -1.93218554040168,
                                 1};

  // dCOEFF 32x
  // *************************************************************************
  static double dCoeff32x[13] = {0.00102945292275,
                                 -1.93288074072411,
                                 0.93482166869425,
                                 -1.8253617352082,
                                 1,
                                 -1.9570539169953,
                                 0.9625396547287,
                                 -1.97205371579193,
                                 1,
                                 -1.98100244642901,
                                 0.98907393282162,
                                 -1.98289480583941,
                                 1};

  // dCOEFF 64x
  // *************************************************************************
  static double dCoeff64x[13] = {0.00101894798776,
                                 -1.96638168022541,
                                 0.96687465485337,
                                 -1.95490597403406,
                                 1,
                                 -1.97969775930388,
                                 0.98108225201938,
                                 -1.99298346945427,
                                 1,
                                 -1.99249184543014,
                                 0.9945163452115,
                                 -1.99571412470436,
                                 1};

  // dCOEFF 128x
  // *************************************************************************
  static double dCoeff128x[13] = {0.00102359688929,
                                  -1.98317523053482,
                                  0.98329948472592,
                                  -1.98863312775467,
                                  1,
                                  -1.9901470788001,
                                  0.99049485279657,
                                  -1.99824398492604,
                                  1,
                                  -1.9967468102523,
                                  0.99725369912944,
                                  -1.99892793100025,
                                  1};

  // dCOEFF 256x
  // *************************************************************************
  static double dCoeff256x[13] = {0.00102849104272,
                                  -1.99158359864769,
                                  0.99161479126471,
                                  -1.99715238605202,
                                  1,
                                  -1.99514878857652,
                                  0.9952359395578,
                                  -1.99956087842252,
                                  1,
                                  -1.99849900371282,
                                  0.99862581732555,
                                  -1.99973194521355,
                                  1};
#endif

  // History buffers for decimation IIR filters
  static double dHistory[DCU_MAX_CHANNELS][MAX_HISTRY];

  // **************************************************************************************
  /// If flag input is 1, then this is a startup initialization request from
  /// controller code.
  if (flag == DAQ_CONNECT) /* Initialize DAQ connection */
  {

    /* First block to write out is last from previous second */
    phase = 0;
    daqBlockNum = (DAQ_NUM_DATA_BLOCKS - 1);
    daqXmitBlockNum = 0;
    excBlockNum = 0;

    /// ** INITIALIZATION **************\n
    daqSlot = -1;
    excSlot = 0;

    ///	\> CDS standard IIR filters will be used for decimation filtering from
    /// the native application rate down to DAQ sample rate. \n
    /// \>\> Need to clear the decimation filter histories.
    for (ii = 0; ii < DCU_MAX_CHANNELS; ii++)
      for (jj = 0; jj < MAX_HISTRY; jj++)
        dHistory[ii][jj] = 0.0;

    /// \> Setup Pointers to the various shared memories:\n
    /// ----  Assign Ptr to data shared memory to network driver (mx_stream) \n
    daqShmPtr = _daq_shm + CDS_DAQ_NET_DATA_OFFSET;
    buf_size = DAQ_DCU_BLOCK_SIZE * 2;
    pWriteBuffer = (volatile char *)daqShmPtr;
    pWriteBuffer += buf_size * 15;
    /// ----  Setup Ptr to interprocess comms with network driver
    dipc = (struct rmIpcStr *)(_daq_shm + CDS_DAQ_NET_IPC_OFFSET);
    /// ----  Setup Ptr to awgtpman shared memory (TP number table)
    tpPtr = (struct cdsDaqNetGdsTpNum *)(_daq_shm +
                                         CDS_DAQ_NET_GDS_TP_TABLE_OFFSET);

    /// ----  Set up pointer to DAQ configuration information in shmem */
    pInfo = (DAQ_INFO_BLOCK *)(_epics_shm + DAQ_INFO_ADDRESS);
    /// ----  Set pointer to shared mem to pass GDS info to DAQ
    gdsPtr = (GDS_CNTRL_BLOCK *)(_epics_shm + DAQ_GDS_BLOCK_ADD);
    // Set pointer to EXC data in shmem.
    if (sysRate < DAQ_16K_SAMPLE_SIZE) {
      exciteDataPtr = (char *)(_epics_shm + DATA_OFFSET_DCU(DCU_ID_EX_2K));
    } else {
      exciteDataPtr = (char *)(_epics_shm + DATA_OFFSET_DCU(DCU_ID_EX_16K));
    }
    excDataSize = 4 + 4 * sysRate;

    // Clear the reconfiguration flag in shmem.
    pInfo->reconfig = 0;

    // Configure data channels
    // *****************************************************
    // Return error if configuration is incorrect.
    /// \> Load DAQ configuration info from memory shared with EPICS
    if ((xferInfo.crcLength = daqConfig(&dataInfo, pInfo, pEpics)) == -1)
      return (-1);

    /// \> Load local table information with channel info
    if ((status = loadLocalTable(&xferInfo, localTable, sysRate, &dataInfo,
                                 &daqRange)) == -1)
      return (-1);

    // Set the start of TP data after DAQ data.
    tpStart = xferInfo.offsetAccum;
    totalChans = dataInfo.numChans;

    /// \> Clear out the GDS TP selections.
    if (sysRate < DAQ_16K_SAMPLE_SIZE)
      tpx = 3;
    else
      tpx = 2;
    for (ii = 0; ii < DAQ_GDS_MAX_TP_ALLOWED; ii++)
      gdsPtr->tp[tpx][0][ii] = 0;
    if (sysRate < DAQ_16K_SAMPLE_SIZE)
      tpx = 1;
    else
      tpx = 0;
    for (ii = 0; ii < DAQ_GDS_MAX_TP_ALLOWED; ii++)
      gdsPtr->tp[tpx][0][ii] = 0;

    /// \> Clear the GDS TP lookup table
    for (i = 0; i < DAQ_GDS_MAX_TP_ALLOWED; i++) {
      tpNum[i] = 0;
      tpNumNet[i] = 0;
    }
    validTp = 0;
    validTpNet = 0;

    // printf("at connect TPnum[0]=%d\n", tpNum[0]);
  } ///  End DAQ CONNECT INITIALIZATION ******************************

  /* ********************************************************************************
   */
  /* Write Data to FB 			*******************************************
   */
  /* ********************************************************************************
   */
  /// If flag=0, data is to be acquired. This is called every code cycle by
  /// controller.c
  /// ** Data Acquisition Mode
  /// ********************************************************
  if (flag == DAQ_WRITE) {
    /// \> Calc data offset into current write swing buffer
    daqSlot = (daqSlot + 1) % sysRate;

    /// \> Write data into local swing buffer
    for (ii = 0; ii < totalChans; ii++) {

      dWord = 0;
      /// \> Read data to a local variable, either from a FM TP or other TP */
      if (localTable[ii].type == DAQ_SRC_FM_TP)
      /* Data if from filter module testpoint */
      {
        switch (localTable[ii].sigNum) {
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
      } else if (localTable[ii].type == DAQ_SRC_NFM_TP)
      /* Data is from non filter module  testpoint */
      {
        dWord = *(pFloatData[localTable[ii].sigNum]);
      } else if (localTable[ii].type == DAQ_SRC_FM_EXC)
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
      if (dataInfo.tp[ii].dataType != DAQ_DATATYPE_32BIT_UINT) {
        if (localTable[ii].decFactor == 2)
          dWord = iir_filter(dWord, &dCoeff2x[0], DTAPS, &dHistory[ii][0]);
        if (localTable[ii].decFactor == 4)
          dWord = iir_filter(dWord, &dCoeff4x[0], DTAPS, &dHistory[ii][0]);
        if (localTable[ii].decFactor == 8)
          dWord = iir_filter(dWord, &dCoeff8x[0], DTAPS, &dHistory[ii][0]);
        if (localTable[ii].decFactor == 16)
          dWord = iir_filter(dWord, &dCoeff16x[0], DTAPS, &dHistory[ii][0]);
        if (localTable[ii].decFactor == 32)
          dWord = iir_filter(dWord, &dCoeff32x[0], DTAPS, &dHistory[ii][0]);
        if (localTable[ii].decFactor == 64)
          dWord = iir_filter(dWord, &dCoeff64x[0], DTAPS, &dHistory[ii][0]);
        if (localTable[ii].decFactor == 128)
          dWord = iir_filter(dWord, &dCoeff128x[0], DTAPS, &dHistory[ii][0]);
        if (localTable[ii].decFactor == 256)
          dWord = iir_filter(dWord, &dCoeff256x[0], DTAPS, &dHistory[ii][0]);
      }
#ifdef CORE_BIQUAD
#undef iir_filter
#endif

      /// \> Write fast data into the swing buffer.
      if ((daqSlot % localTable[ii].decFactor) == 0) {
        mydatatype = dataInfo.tp[ii].dataType;
        switch (mydatatype) {
        case DAQ_DATATYPE_16BIT_INT:
          // Write short data; (XOR 1) here provides sample swapping
          ((short *)(pWriteBuffer +
                     localTable[ii]
                         .offset))[(daqSlot / localTable[ii].decFactor) ^ 1] =
              (short)dWord;
          break;
        case DAQ_DATATYPE_DOUBLE:
          ((double *)(pWriteBuffer +
                      localTable[ii]
                          .offset))[daqSlot / localTable[ii].decFactor] = dWord;
          break;
        case DAQ_DATATYPE_32BIT_UINT:
          // Write a 32-bit int (downcast from the double passed)
          if (localTable[ii].decFactor == 1)
            ((unsigned int *)(pWriteBuffer + localTable[ii].offset))
                [daqSlot / localTable[ii].decFactor] = ((unsigned int)dWord);
          else
            ((unsigned int *)(pWriteBuffer + localTable[ii].offset))
                [daqSlot / localTable[ii].decFactor] =
                    ((unsigned int)dWord) & *((unsigned int *)(dHistory[ii]));
          break;
        case DAQ_DATATYPE_32BIT_INT:
          ((int *)(pWriteBuffer + localTable[ii].offset))[daqSlot] = (int)dWord;
          break;
        default:
          // Write a 32-bit float (downcast from the double passed)
          ((float *)(pWriteBuffer +
                     localTable[ii]
                         .offset))[daqSlot / localTable[ii].decFactor] =
              (float)dWord;
          break;
        }
      } else if (dataInfo.tp[ii].dataType == DAQ_DATATYPE_32BIT_UINT) {
        if ((daqSlot % localTable[ii].decFactor) == 1)
          *((unsigned int *)(dHistory[ii])) = (unsigned int)dWord;
        else
          *((unsigned int *)(dHistory[ii])) &= (unsigned int)dWord;
      }
    } /* end swing buffer write loop */

    /// \> Write EPICS data into swing buffer at 16Hz.
    if (daqSlot == DAQ_XFER_CYCLE_INT) {
      /// \>\> On 16Hz boundary: \n
      /// - ----  Write EPICS integer values to beginning of local write buffer

      if (dataInfo.cpyepics2times) {
        memcpy((void *)pWriteBuffer, pEpicsIntData, dataInfo.cpyIntSize[0]);
        pEpicsInt = (char *)pWriteBuffer;
        pEpicsInt += dataInfo.cpyIntSize[0];
        pEpicsInt1 = pEpicsIntData + dataInfo.cpyIntSize[0] + 4;
        memcpy(pEpicsInt, pEpicsInt1, dataInfo.cpyIntSize[1]);
      } else {
        memcpy((void *)pWriteBuffer, pEpicsIntData, dataInfo.cpyIntSize[0]);
      }
    }
    if (daqSlot == DAQ_XFER_CYCLE_DBL) {
      /// - ---- Write EPICS double values as float values after EPICS integer
      /// type data.
      pEpicsDblData1 = (double *)pEpicsDblData;
      pEpicsFloat = (float *)pWriteBuffer;
      pEpicsFloat += dataInfo.numEpicsInts;
      for (ii = 0; ii < dataInfo.numEpicsFloats; ii++) {
        *pEpicsFloat = (float)*pEpicsDblData1;
        pEpicsFloat++;
        pEpicsDblData1++;
      }
    }
    if ((daqSlot >= DAQ_XFER_CYCLE_FMD) &&
        (daqSlot < dataInfo.numEpicsFiltXfers)) {
      /// \>\> On 16Hz boundary + 1 (or more) cycle(s): \n
      /// - ----  Write filter module EPICS values as floats
      jj = DAQ_XFER_FMD_PER_CYCLE * (daqSlot - DAQ_XFER_CYCLE_FMD);
      if (daqSlot == (dataInfo.numEpicsFiltXfers - 1)) {
        kk = jj + dataInfo.numEpicsFiltsLast;
      } else {
        kk = jj + DAQ_XFER_FMD_PER_CYCLE;
      }
      // printf("Cycle = %d jj = %d kk = %d\n",daqSlot,jj,kk);
      for (ii = jj; ii < kk; ii++) {
        *pEpicsFloat = (float)dspPtr->inputs[ii].offset;
        pEpicsFloat++;
        *pEpicsFloat = (float)dspPtr->inputs[ii].outgain;
        pEpicsFloat++;
        *pEpicsFloat = (float)dspPtr->inputs[ii].limiter;
        pEpicsFloat++;
        *pEpicsFloat = (float)dspPtr->inputs[ii].gain_ramp_time;
        pEpicsFloat++;
        *pEpicsFloat = (float)dspPtr->inputs[ii].swReq;
        pEpicsFloat++;
        *pEpicsFloat = (float)dspPtr->inputs[ii].swMask;
        pEpicsFloat++;
        *pEpicsFloat = (float)dspPtr->data[ii].filterInput;
        pEpicsFloat++;
        *pEpicsFloat = (float)dspPtr->data[ii].exciteInput;
        pEpicsFloat++;
        *pEpicsFloat = (float)dspPtr->data[ii].output16Hz;
        pEpicsFloat++;
        *pEpicsFloat = (float)dspPtr->data[ii].output;
        pEpicsFloat++;
        *pEpicsFloat = (float)dspPtr->data[ii].swStatus;
        pEpicsFloat++;
      }
    }

    /// \> Read in any selected EXC signals. \n
    /// --- NOTE: EXC signals have to be read and loaded in advance by 1 cycle
    /// ie must be loaded and
    /// available to the realtime application when the next code cycle is
    /// initiated.
    excSlot = (excSlot + 1) % sysRate;
    // if(validEx)
    validEx = 0;
    {
      // Go through all test points
      for (ii = dataInfo.numChans; ii < totalChans; ii++) {
        // Do not pickup any testpoints (type 0 or 1)
        if (localTable[ii].type < DAQ_SRC_FM_EXC)
          continue;

        exChanOffset = localTable[ii].sigNum * excDataSize;
        statusPtr = (int *)(exciteDataPtr + excBlockNum * DAQ_DCU_BLOCK_SIZE +
                            exChanOffset);
        if (*statusPtr == 0) {
          validEx = FE_ERROR_EXC_SET;
          dataPtr = (float *)(exciteDataPtr + excBlockNum * DAQ_DCU_BLOCK_SIZE +
                              exChanOffset + excSlot * 4 + 4);
          if (localTable[ii].type == DAQ_SRC_FM_EXC) {
            dspPtr->data[localTable[ii].fmNum].exciteInput = *dataPtr;
          } else if (localTable[ii].type == DAQ_SRC_NFM_EXC) {
            // extra excitation
            excSignal[localTable[ii].fmNum] = *dataPtr;
          }
        }
        // else dspPtr->data[localTable[ii].fmNum].exciteInput = 0.0;
        else {
          if (localTable[ii].type == DAQ_SRC_FM_EXC) {
            dspPtr->data[localTable[ii].fmNum].exciteInput = 0.0;
          } else if (localTable[ii].type == DAQ_SRC_NFM_EXC) {
            // extra excitation
            excSignal[localTable[ii].fmNum] = 0.0;
          }
        }
      }
    }

    /// \> Move to the next 1/16 EXC signal data block if end of 16Hz block
    if (excSlot == (sysRate - 1))
      excBlockNum = (excBlockNum + 1) % DAQ_NUM_DATA_BLOCKS;

    /// \> If last cycle of a 16Hz block:
    if (daqSlot == (sysRate - 1))
    /* Done with 1/16 second DAQ data block */
    {

      /// - -- Fill in the IPC table for DAQ network driver (mx_stream) \n
      dipc->dcuId = dcuId; /// -   ------ DCU id of this system
      dipc->crc =
          xferInfo.fileCrc; /// -   ------ Checksum of the configuration file
      dipc->dataBlockSize =
          xferInfo.totalSizeNet; /// -   ------ Actual data size
      /// -   ------  Data block number
      dipc->bp[daqXmitBlockNum].cycle = daqXmitBlockNum;
      /// -   ------  Data block CRC
      dipc->bp[daqXmitBlockNum].crc = xferInfo.crcLength;
      /// -   ------ Timestamp GPS Second
      dipc->bp[daqXmitBlockNum].timeSec = (unsigned int)cycle_gps_time;
      /// -   ------ Timestamp GPS nanoSecond - Actually cycle number
      dipc->bp[daqXmitBlockNum].timeNSec = (unsigned int)daqXmitBlockNum;

      /// - ------ Write test point info to DAQ net shared memory
      tpPtr->count = validTpNet | validEx;
      memcpy(tpPtr->tpNum, tpNumNet, sizeof(tpNumNet[0]) * validTp);

      // As the last step set the cycle counter
      // Frame builder is looking for cycle change
      /// - ------ Write IPC cycle number. This will trigger DAQ network driver
      /// to send data to DAQ
      dipc->cycle = daqXmitBlockNum; // Ready cycle (16 Hz)

      /// - -- Increment the 1/16 sec block counter
      daqBlockNum = (daqBlockNum + 1) % DAQ_NUM_DATA_BLOCKS_PER_SECOND;
      daqXmitBlockNum = (daqXmitBlockNum + 1) % DAQ_NUM_DATA_BLOCKS_PER_SECOND;

      /// - -- Set data write ptr to next block in shmem
      pWriteBuffer = (char *)daqShmPtr;
      pWriteBuffer += buf_size * daqXmitBlockNum;

      //  - -- Check for reconfig request at start of each second
      if ((pInfo->reconfig == 1) && (daqBlockNum == 0)) {
        // printf("New daq config\n");
        pInfo->reconfig = 0;
        // Configure EPICS data channels
        xferInfo.crcLength = daqConfig(&dataInfo, pInfo, pEpics);
        if (xferInfo.crcLength) {
          status = loadLocalTable(&xferInfo, localTable, sysRate, &dataInfo,
                                  &daqRange);
          // Clear decimation filter history
          for (ii = 0; ii < dataInfo.numChans; ii++) {
            for (jj = 0; jj < MAX_HISTRY; jj++)
              dHistory[ii][jj] = 0.0;
          }

          tpStart = xferInfo.offsetAccum;
          totalChans = dataInfo.numChans;
        }
      }
      /// - -- If last cycle of 1 sec time frame, check for new TP and load
      /// info.
      // This will cause new TP to be written to local memory at start of 1 sec
      // block.

      if (daqBlockNum == 15) {
        // Offset by one into the TP/EXC tables for the 2K systems
        unsigned int _2k_sys_offs = sysRate < DAQ_16K_SAMPLE_SIZE;

        // Helper function to search the lists
        // Clears the found number from the lists
        // tpnum and excnum lists of numbers do not intersect
        inline int in_the_lists(unsigned int tp, unsigned int slot) {
          int i;
          for (i = 0; i < DAQ_GDS_MAX_TP_ALLOWED; i++) {
            if (tpnum[i] == tp)
              return (tpnum[i] = 0, 1);
            if (excnum[i] == tp) {
              // Check if the excitation is still in the same slot
              if (i != excTable[slot].offset)
                return 0;
              return (excnum[i] = 0, 1);
            }
          }
          return 0;
        }

        // Helper function to find an empty slot in the localTable
        inline unsigned int empty_slot(void) {
          int i;
          for (i = 0; i < DAQ_GDS_MAX_TP_ALLOWED; i++) {
            if (tpNum[i] == 0)
              return i;
          }
          return -1;
        }

        // Copy TP/EXC tables into my local memory
        // Had to change from memcpy to for loop for Debian 10.
        for (ii = 0; ii < DAQ_GDS_MAX_TP_ALLOWED; ii++)
          excnum[ii] = gdsPtr->tp[_2k_sys_offs][0][ii];
        for (ii = 0; ii < DAQ_GDS_MAX_TP_ALLOWED; ii++)
          tpnum[ii] = gdsPtr->tp[(2 + _2k_sys_offs)][0][ii];

        /// - ------ Search and clear deselected test points
        for (i = 0; i < DAQ_GDS_MAX_TP_ALLOWED; i++) {
          if (tpNum[i] == 0)
            continue;
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
            if (tpnum[i] == 0)
              continue;
            tpn = tpnum[i];
          } else {
            if (excnum[i - DAQ_GDS_MAX_TP_ALLOWED] == 0)
              continue;
            tpn = excnum[i - DAQ_GDS_MAX_TP_ALLOWED];
            exc = 1;
            ii = i - DAQ_GDS_MAX_TP_ALLOWED;
          }

          // printf("tpn=%d at %d\n", tpn, i);
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

              // if (slot < 24) gdsMonitor[slot] = tpn;
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
              localTable[ltSlot].sysNum =
                  jj / daqRange.filtExSize; // filtExSize = MAX_MODULES
              localTable[ltSlot].fmNum = jj % daqRange.filtExSize;
              localTable[ltSlot].sigNum = ii;
              localTable[ltSlot].decFactor = 1;

              excTable[slot].sigNum = tpn;
              excTable[slot].sysNum = localTable[ltSlot].sysNum;
              excTable[slot].fmNum = localTable[ltSlot].fmNum;
              excTable[slot].offset = i - DAQ_GDS_MAX_TP_ALLOWED;

              // Save the index into the TPman table
              dataInfo.tp[ltSlot].dataType = DAQ_DATATYPE_FLOAT;

              gdsPtr->tp[_2k_sys_offs][1][slot] = 0;
              tpNum[slot] = tpn;

            } else if (tpn >= daqRange.xExMin && tpn < daqRange.xExMax) {
              jj = tpn - daqRange.xExMin;
              localTable[ltSlot].type = DAQ_SRC_NFM_EXC;
              localTable[ltSlot].sysNum = 0; // filtExSize = MAX_MODULES
              localTable[ltSlot].fmNum = jj;
              // localTable[ltSlot].sigNum = slot;
              localTable[ltSlot].sigNum = ii;
              localTable[ltSlot].decFactor = 1;

              excTable[slot].sigNum = tpn;
              excTable[slot].sysNum = localTable[ltSlot].sysNum;
              excTable[slot].fmNum = localTable[ltSlot].fmNum;
              excTable[slot].offset = i - DAQ_GDS_MAX_TP_ALLOWED;

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
        for (i = DAQ_GDS_MAX_TP_ALLOWED - 1; i >= 0; i--) {
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
          if (i < 32)
            gdsMonitor[i] = tpNum[i];
        }

        for (i = validTp; i < 32; i++)
          gdsMonitor[i] = 0;

      } /* End normal check for new TP numbers */

      // Network write is one cycle behind memory write, so now update tp nums
      // for FB xmission
      if (daqBlockNum == 0) {
        for (ii = 0; ii < validTp; ii++)
          tpNumNet[ii] = tpNum[ii];
        validTpNet = validTp;
        xferInfo.totalSizeNet = xferInfo.totalSize;
      }

    } /* End done 16Hz Cycle */

  } /* End case write */

  /// \> Return the FE total DAQ data rate */
  return ((xferInfo.totalSize * DAQ_NUM_DATA_BLOCKS_PER_SECOND) / 1000);
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
int daqConfig(DAQ_INFO_BLOCK *dataInfo, DAQ_INFO_BLOCK *pInfo, char *pEpics) {
  int ii, jj;               // Loop counters
  int epicsIntXferSize = 0; // Size, in bytes, of EPICS integer type data.
  int dataLength = 0;       // Total size, in bytes, of data to be sent
  int status = 0;
  int mydatatype;

  /// \> Verify correct channel count before proceeding. \n
  /// - ---- Required to be at least one and not more than DCU_MAX_CHANNELS.
  status = pInfo->numChans;
  // if((status < 1) || (status > DCU_MAX_CHANNELS))
  if (status > DCU_MAX_CHANNELS) {
    // printf("Invalid num daq chans = %d\n",status);
    return (-1);
  }

  /// \> Get the number of fast channels to be acquired
  dataInfo->numChans = pInfo->numChans;

  /// \> Setup EPICS channel information/pointers
  dataInfo->numEpicsInts = pInfo->numEpicsInts;
  dataInfo->numEpicsFloats = pInfo->numEpicsFloats;
  dataInfo->numEpicsFilts = pInfo->numEpicsFilts;
  dataInfo->numEpicsTotal = pInfo->numEpicsTotal;
  /// \> Determine how many filter modules are to have their data transferred
  /// per cycle.
  /// - ---- This is to balance load (CPU time) across several cycles if number
  /// of filter modules > 100
  dataInfo->numEpicsFiltXfers = MAX_MODULES / DAQ_XFER_FMD_PER_CYCLE;
  dataInfo->numEpicsFiltsLast = DAQ_XFER_FMD_PER_CYCLE;
  if (MAX_MODULES % DAQ_XFER_FMD_PER_CYCLE) {
    dataInfo->numEpicsFiltXfers++;
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
  // printf("Have %d CDS epics integer and %d USR epics integer
  // channels\n",ii,jj);
  ii *= 4;
  jj *= 4;

  /// \>  Look for memory holes ie integer types not ending on 8 byte boundary.
  /// Need to check both standard CDS struct and User channels
  /// - ---- If either doesn't end on 8 byte boundary, then a 4 byte hole will
  /// appear
  /// before the double type data in shared memory.
  if (ii % 8) {
    // printf("Have int mem hole after CDS %d %d \n",ii,jj);
    dataInfo->epicsdblDataOffset += 4;
  }
  if (jj % 8) {
    // printf("Have int mem hole after user %d %d \n",ii,jj);
    dataInfo->epicsdblDataOffset += 4;
  }
  if ((ii % 8) && (jj > 0)) {
    /// - ---- If standard CDS struct doesn't end on 8 byte boundary, then
    /// have 4 byte mem hole after CDS data
    /// This will require 2 memcpys of integer data to get 8 byte alignment for
    /// xfer to DAQ buffer..
    dataInfo->cpyIntSize[0] = ii;
    dataInfo->cpyIntSize[1] = jj;
    dataInfo->cpyepics2times = 1;
    // dataInfo->epicsdblDataOffset += 4;
    // printf("Have mem holes after CDS  %d %d \nNeed to cpy ints twice - size 1
    // = %d size 2 = %d
    // \n",ii,jj,dataInfo->cpyIntSize[0],dataInfo->cpyIntSize[1]);
  }

  /// \> Set the pointer to start of EPICS double type data in shared memory. \n
  /// - ---- Ptr to double type data is at EPICS integer start + EPICS integer
  /// size + Memory holes
  pEpicsDblData =
      (pEpicsIntData + epicsIntXferSize + dataInfo->epicsdblDataOffset);

  // Send EPICS data diags to dmesg
  // printf("DAQ EPICS INT DATA is at 0x%lx with size
  // %d\n",(long)pEpicsIntData,epicsIntXferSize);
  // printf("DAQ EPICS FLT DATA is at 0x%lx\n",(long)pEpicsDblData);
  // printf("DAQ EPICS: Int = %d  Flt = %d Filters = %d Total = %d Fast =
  // %d\n",dataInfo->numEpicsInts,dataInfo->numEpicsFloats,dataInfo->numEpicsFilts,
  // dataInfo->numEpicsTotal, dataInfo->numChans);
  // printf("DAQ EPICS: Number of Filter Module Xfers = %d last =
  // %d\n",dataInfo->numEpicsFiltXfers,dataInfo->numEpicsFiltsLast);
  /// \> Initialize CRC length with EPICS data size.
  dataLength = 4 * dataInfo->numEpicsTotal;
  // printf("crc length epics = %d\n",dataLength);

  /// \>  Get the DAQ configuration information for all fast DAQ channels and
  /// calc a crc checksum length
  for (ii = 0; ii < dataInfo->numChans; ii++) {
    dataInfo->tp[ii].tpnum = pInfo->tp[ii].tpnum;
    dataInfo->tp[ii].dataType = pInfo->tp[ii].dataType;
    dataInfo->tp[ii].dataRate = pInfo->tp[ii].dataRate;
    mydatatype = dataInfo->tp[ii].dataType;
    dataLength += DAQ_DATA_TYPE_SIZE(mydatatype) * dataInfo->tp[ii].dataRate /
                  DAQ_NUM_DATA_BLOCKS;
    // if(mydatatype == 5) printf("Found double
    // %d\n",DAQ_DATA_TYPE_SIZE(mydatatype));
  }
  /// \> Set DAQ bytes/sec global, which is output to EPICS by controller.c
  curDaqBlockSize = dataLength * DAQ_NUM_DATA_BLOCKS_PER_SECOND;

  /// \> RETURN dataLength, used in other code for CRC checksum byte count
  return (dataLength);
}

// **************************************************************************************
///	@author R.Bork\n
///	@brief This function populates the local DAQ/TP tables.
///	@param *pDxi	Pointer to struct with data transfer size information.
///	@param localTable[]	Table to be populated with data pointer
///information
///	@param sysRate			Number of code cycles in 1/16 second.
///	@param *dataInfo		DAQ configuration information
///	@param *daqRange		Info on GDS TP number ranges which provides
///type information
///	@return	0=OK or -1=FAIL
// **************************************************************************************
int loadLocalTable(DAQ_XFER_INFO *pDxi, DAQ_LKUP_TABLE localTable[],
                   int sysRate, DAQ_INFO_BLOCK *dataInfo, DAQ_RANGE *daqRange) {
  int ii, jj;
  int mydatatype;

  /// \> Get the .INI file crc checksum to pass to DAQ Framebuilders for config
  /// checking */
  pDxi->fileCrc = pInfo->configFileCRC;
  /// \> Calculate the number of bytes to xfer on each call, based on total
  /// number
  ///   of bytes to write each 1/16sec and the front end data rate
  ///   (2048/16384Hz)
  pDxi->xferSize1 = pDxi->crcLength / sysRate;
  pDxi->totalSize = pDxi->crcLength;
  pDxi->totalSizeNet = pDxi->crcLength;

  /// \> Find first memory location for fast data in read/write swing buffers.
  pDxi->offsetAccum = 4 * dataInfo->numEpicsTotal;
  localTable[0].offset = pDxi->offsetAccum;

  /// \> Fill in the local lookup table for finding data.
  /// - (Need to develop a table of offset pointers to load data into swing
  /// buffers)  \n
  /// - (This is based on decimation factors and data size.)
  for (ii = 0; ii < dataInfo->numChans; ii++) {
    if ((dataInfo->tp[ii].dataRate / DAQ_NUM_DATA_BLOCKS) > sysRate) {
      /* Channel data rate is greater than system rate */
      // printf("Channels %d has bad data rate %d\n", ii,
      // dataInfo->tp[ii].dataRate);
      return (-1);
    } else {
      /// - ---- Load decimation factor
      localTable[ii].decFactor =
          sysRate / (dataInfo->tp[ii].dataRate / DAQ_NUM_DATA_BLOCKS);
    }

    /// - ---- Calc offset into swing buffer for writing data
    mydatatype = dataInfo->tp[ii].dataType;
    pDxi->offsetAccum +=
        (sysRate / localTable[ii].decFactor * DAQ_DATA_TYPE_SIZE(mydatatype));

    localTable[ii + 1].offset = pDxi->offsetAccum;
    /// - ----  Need to determine if data is from a filter module TP or non-FM
    /// TP and tag accordingly
    if ((dataInfo->tp[ii].tpnum >= daqRange->filtTpMin) &&
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
    } else if ((dataInfo->tp[ii].tpnum >= daqRange->filtExMin) &&
               (dataInfo->tp[ii].tpnum < daqRange->filtExMax))
    /* This is a filter module excitation input */
    {
      /* Mark as coming from a filter module excitation input */
      localTable[ii].type = DAQ_SRC_FM_EXC;
      /* Mark filter module number */
      localTable[ii].fmNum = dataInfo->tp[ii].tpnum - daqRange->filtExMin;
    } else if ((dataInfo->tp[ii].tpnum >= daqRange->xTpMin) &&
               (dataInfo->tp[ii].tpnum < daqRange->xTpMax))
    /* This testpoint is not part of a filter module */
    {
      jj = dataInfo->tp[ii].tpnum - daqRange->xTpMin;
      /* Mark as a non filter module testpoint */
      localTable[ii].type = DAQ_SRC_NFM_TP;
      /* Mark the offset into the local data buffer */
      localTable[ii].sigNum = jj;
    }
    else if ((dataInfo->tp[ii].tpnum >= daqRange->xExMin) &&
             (dataInfo->tp[ii].tpnum < daqRange->xExMax))
    /* This exc testpoint is not part of a filter module */
    {
      jj = dataInfo->tp[ii].tpnum - daqRange->xExMin;
      /* Mark as a non filter module testpoint */
      localTable[ii].type = DAQ_SRC_NFM_EXC;
      /* Mark the offset into the local data buffer */
      localTable[ii].fmNum = jj;
      localTable[ii].sigNum = jj;
    } else {
      // printf("Invalid chan num found %d = %d\n",ii,dataInfo->tp[ii].tpnum);
      return (-1);
    }
    // printf("Table %d Offset = %d  Type =
    // %d\n",ii,localTable[ii].offset,dataInfo->tp[ii].dataType);
  }
  return (0);
  /// \> RETURN 0=OK or -1=FAIL
}
