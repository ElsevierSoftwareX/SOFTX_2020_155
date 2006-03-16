	/* ADC Mapping: *************************************************

		Module 1

		00 = ETMX UL		16 = ETMY UL
		01 = ETMX UR  		17 = ETMY UR
		02 = ETMX LR 		18 = ETMY LR
		03 = ETMX LL 		19 = ETMY LL
		04 = ETMX Side 		20 = ETMY Side
		05 = ITMX UL 		21 = ITMY UL
		06 = ITMX UR		22 = ITMY UR
		07 = ITMX LR		23 = ITMY LR
		08 = ITMX LL		24 = ITMY LL
		09 = ITMX Side		25 = ITMY Side
		10 = BS   UL
		11 = BS	  UR
		12 = BS	  LR
		13 = BS   LL
		14 = BS   Side
		15 = 			31 = One PPS Timing Signal

		Module 2

		00 = WFS I1		16 = ASPD1 I
		01 = WFS I2   		17 = ASPD1 Q
		02 = WFS I3  		18 = ASPD1 DC
		03 = WFS I4  		19 =
		04 = WFS Q1    		20 = REFL PD I
		05 = WFS Q2  		21 = REFL PD Q
		06 = WFS Q3 		22 = REFL PD DC
		07 = WFS Q4 		23 = 
		08 =        		24 = ASPD2 DC
		09 =          		25 = ASPD3 DC
		10 =       		26 = Xarm Trans 
		11 =        		27 = Yarm Trans
		12 =        
		13 =        
		14 =          
		15 = 			
	*/
	/* DAC Mapping: *************************************************
		Module 0
		---------
		0,1,2,3,4	= ETMX	UL, UR, LR, LL, Side
		5,6,7,8,9 	= ETMY	UL, UR, LR, LL, Side
		11,12,13,14,15	= BS	UL, UR, LR, LL, Side

		Module 1
		---------
		0,1,2,3,4	= ITMX	UL, UR, LR, LL, Side
		5,6,7,8,9 	= ITMY	UL, UR, LR, LL, Side
		10,11		= M1 PZT Pitch, M1 PZT Yaw
		12,13		= M2 PZT Pitch, M2 PZT Yaw
		14		= Lazer Control
		15 		= M3 LSC
	*/


pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t go1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t go2 = PTHREAD_COND_INITIALIZER;
pthread_cond_t done = PTHREAD_COND_INITIALIZER;
volatile unsigned int waiting1 = 1;
volatile unsigned int waiting2 = 1;
volatile unsigned int done1 = 0;
volatile unsigned int done2 = 0;

#include <pnm_lsc.h>

void feCodeLSC(double dWord[][32],int dacOut[][16],FILT_MOD *dspPtr,COEF *dspCoeff,CDS_EPICS *pLocalEpics, int optic)
{
  int ii, jj;
  double senIn[10];
  double senOut[10];

  senIn[0] = dWord[1][26]; /* Xarm Trans */
  senIn[1] = dWord[1][27]; /* Yarm Trans */
  senIn[2] = dWord[1][20]; /* Refl I */
  senIn[3] = dWord[1][21]; /* Refl Q */
  senIn[4] = dWord[1][16]; /* ASPD1 I */
  senIn[5] = dWord[1][17]; /* ASPD1 Q */
  senIn[6] = dWord[1][18]; /* ASPD1 DC */
  senIn[7] = dWord[1][24]; /* ASPD2 DC */
  senIn[8] = dWord[1][25]; /* ASPD3 DC */
  senIn[9] = dWord[1][28]; /* Lock in DC ??? */

  /* Do sensor filters */
  for (ii=0; ii < 10; ii++) {
      senOut[ii] = filterModuleD(dspPtr, dspCoeff, XARM_TRANS + ii, senIn[ii], 0);
  }

  double matInput[4];

  /* Phase rotation */
  /* Refl I matrix input */
  matInput[0] = senOut[REFL_I]* pLocalEpics->epicsInput.lscReflPhase[1]  /* cos */
                + senOut[REFL_Q]* pLocalEpics->epicsInput.lscReflPhase[0];  /* sin */
  /* Refl Q matrix input */
  matInput[1] = senOut[REFL_Q]* pLocalEpics->epicsInput.lscReflPhase[1]  /* cos */
                - senOut[REFL_I]* pLocalEpics->epicsInput.lscReflPhase[0];  /* sin */
  /* ASRF1 I matrix input */
  matInput[2] = senOut[ASRF_I]* pLocalEpics->epicsInput.lscASRFPhase[1]  /* cos */
                + senOut[ASRF_Q]* pLocalEpics->epicsInput.lscASRFPhase[0];  /* sin */
  /* ASRF1 Q matrix input */
  matInput[3] = senOut[ASRF_Q]* pLocalEpics->epicsInput.lscASRFPhase[1]  /* cos */
                - senOut[ASRF_I]* pLocalEpics->epicsInput.lscASRFPhase[0];  /* sin */

  /* Do strange DIV filter ??? */
  double divOut = filterModuleD(dspPtr, dspCoeff, DIV, senOut[XARM_TRANS] + senOut[YARM_TRANS], 0);
  double dofOut[3];
 
  /* Calculate Input matrix and DOF filters */
  for (ii=0; ii < 3; ii++) {
    double fmIn =  0.0;
    for (jj=0; jj < 4; jj++) fmIn += senOut[jj] * pLocalEpics->epicsInput.lscInputMatrix[jj][ii];
    dofOut[ii] = filterModuleD(dspPtr, dspCoeff, LSC_DOF1 + ii, fmIn + divOut, 0);
  }

  double lscOut[8];

  /* Calculate output matrix */
  for (ii = 0; ii < 6; ii++) {
    lscOut[ii] = 0.0;
    for (jj = 0; jj < 3; jj++) {
      lscOut[ii] += dofOut[jj] * pLocalEpics->epicsInput.lscOutputMatrix[jj][ii];
    }
  }
  /* TODO: outputs */
}

#include <pnm_asc.h>

void feCodeASC(double dWord[][32],int dacOut[][16],FILT_MOD *dspPtr,COEF *dspCoeff,CDS_EPICS *pLocalEpics, int optic)
{
  int ii, jj;

  double senOut[8];

  /* Do sensor filters */
  for (ii=0; ii < 8; ii++) {
      senOut[ii] = filterModuleD(dspPtr, dspCoeff, WFS_I1 + ii, dWord[1][ii], 0);
  }

  double matInputI[4];
  double matInputQ[4];

  for (ii = 0; ii < 4; ii++) { 
    /* WFS I matrix input */
    matInputI[ii] = senOut[WFS_I1 + ii]* pLocalEpics->epicsInput.ascPhase[ii][1]  /* cos */
                + senOut[WFS_Q1 + ii]* pLocalEpics->epicsInput.ascPhase[ii][0];  /* sin */
    /* WFS Q matrix input */
    matInputQ[ii] = senOut[WFS_Q1 + ii]* pLocalEpics->epicsInput.ascPhase[ii][1]  /* cos */
                - senOut[WFS_I1 + ii]* pLocalEpics->epicsInput.ascPhase[ii][0];  /* sin */
  }

  double matOutputI[3];
  double matOutputQ[3];
  /* Calculate I & Q Matrices */
  for (ii=0; ii < 3; ii++) {
    matOutputI[ii] = 0.0;
    matOutputQ[ii] = 0.0;
    for (jj=0; jj < 4; jj++) matOutputI[ii] += matInputI[jj] * pLocalEpics->epicsInput.wfsInputMatrixI[jj][ii];
    for (jj=0; jj < 4; jj++) matOutputQ[ii] += matInputQ[jj] * pLocalEpics->epicsInput.wfsInputMatrixQ[jj][ii];
  }


  double dofOut[4];
  double matInput[4];
  matInput[0] = matOutputI[0];
  matInput[1] = matOutputI[1];
  matInput[2] = matOutputQ[0];
  matInput[3] = matOutputQ[1];

  /* Calculate Input matrix and DOF filters */
  for (ii=0; ii < 4; ii++) {
    double fmIn =  0.0;
    for (jj=0; jj < 4; jj++) fmIn += matInput[jj] * pLocalEpics->epicsInput.wfsInputMatrix[jj][ii];
    dofOut[ii] = filterModuleD(dspPtr, dspCoeff, WFS_P + ii, fmIn, 0);
  }

   
  double ascOut[4];

  /* Calculate output matrix */
  for (ii = 0; ii < 4; ii++) {
    ascOut[ii] = 0.0;
    for (jj = 0; jj < 4; jj++) {
      ascOut[ii] += dofOut[jj] * pLocalEpics->epicsInput.wfsOutputMatrix[jj][ii];
    }
  }
}

void feCodeOptic(double dWord[][32],int dacOut[][16],FILT_MOD *dspPtr,COEF *dspCoeff,CDS_EPICS *pLocalEpics, int optic)
{
  double fmIn;
  double senOut[5];
  double dofOut[3];
  double cOut[4];
  int ii, jj;
        // Input filtering, corners and side
        for (ii=0; ii < 5; ii++) {
                senOut[ii] = filterModuleD(dspPtr, dspCoeff, ULSEN + ii, dWord[0][ii], 0);
	}

        // Do input matrix and DOF filtering
        for (ii=0; ii < 3; ii++) {
                fmIn =  senOut[0] * pLocalEpics->epicsInput.inputMatrix[optic][0][ii] +
                        senOut[1] * pLocalEpics->epicsInput.inputMatrix[optic][1][ii] +
                        senOut[2] * pLocalEpics->epicsInput.inputMatrix[optic][2][ii] +
                        senOut[3] * pLocalEpics->epicsInput.inputMatrix[optic][3][ii];
                dofOut[ii] = filterModuleD(dspPtr,dspCoeff, SUSPOS + ii,fmIn,0);
        }

	// Add LSC and ASC (inputs are 0.0 for now) corrections
        dofOut[0] += filterModuleD(dspPtr, dspCoeff, LSC , 0.0, 0);
        dofOut[1] += filterModuleD(dspPtr, dspCoeff, ASCPIT , 0.0, 0);
        dofOut[2] += filterModuleD(dspPtr, dspCoeff, ASCYAW , 0.0, 0);

        // Do output matrix filters
        for (ii=0; ii < 4; ii++) {
	  cOut[ii] = 0.0;
          for (jj=0; jj < 3; jj++) {
                cOut[ii] += filterModuleD(dspPtr, dspCoeff, ULPOS + 3*ii + jj, dofOut[jj], 0);
	  }
        }

        // Do output filters
        for (ii=0; ii < 4; ii++) {
                dacOut[0][ii] = filterModuleD(dspPtr, dspCoeff, ULCOIL + ii, cOut[ii], 0);
        }

	/* :TODO: outputs must be routed to correct D/A channels for each optic */
}

volatile unsigned int cpu2go = 0;
volatile unsigned int cpu3go = 0;

void feCode(double dWord[][32],int dacOut[][16],FILT_MOD *dspPtrArg,COEF *dspCoeff,CDS_EPICS *pLocalEpics)
{
  int system;
#if 0
  pthread_mutex_lock(&lock);
  waiting1 = 0; waiting2 = 0; done1 = 0; done2 = 0;
  pthread_cond_signal(&go1);
  pthread_cond_signal(&go2);
  pthread_mutex_unlock(&lock);
#endif

  feCodeASC(dWord,dacOut,dspPtr[system],dspCoeff + system,pLocalEpics, system);
  feCodeLSC(dWord,dacOut,dspPtr[system],dspCoeff + system,pLocalEpics, system);
  cpu2go = cpu3go = 1;
  for (system = 0; system < 3; system++)
    feCodeOptic(dWord,dacOut,dspPtr[system],dspCoeff + system,pLocalEpics, system);
  for(;cpu2go || cpu3go;);
#if 0
  pthread_mutex_lock(&lock);
  while (!done1 || !done2) pthread_cond_wait(&done, &lock);
  pthread_mutex_unlock(&lock);
#endif
}

void
cpu2_start() {
  for(;!vmeDone;) {
  int system;
  for(;cpu2go == 0;);
#if 0
  pthread_mutex_lock(&lock);
  while (waiting1) pthread_cond_wait(&go1, &lock);
  pthread_mutex_unlock(&lock);
#endif
  for (system = 3; system < 4; system++)
    feCodeOptic(dWord,dacOut,dspPtr[system],dspCoeff + system,pLocalEpics, system);
  cpu2go = 0;
#if 0
  pthread_mutex_lock(&lock);
  done1 = 1; waiting1 = 1;
  pthread_cond_signal(&done);
  pthread_mutex_unlock(&lock);
#endif
  }
}

void
cpu3_start() {
  for(;!vmeDone;) {
  int system;
#if 0
  pthread_mutex_lock(&lock);
  while (waiting2) pthread_cond_wait(&go2, &lock);
  pthread_mutex_unlock(&lock);
#endif
  for(;cpu3go == 0;);
  for (system = 4; system < 5; system++)
    feCodeOptic(dWord,dacOut,dspPtr[system],dspCoeff + system,pLocalEpics, system);
  cpu3go = 0;
#if 0
  pthread_mutex_lock(&lock);
  done2 = 1; waiting2 = 1;
  pthread_cond_signal(&done);
  pthread_mutex_unlock(&lock);
#endif
  }
}


/* Signal threads to proceed */
feDone() {
#if 0
  waiting1 = 0; waiting2 = 0; done1 = 0; done2 = 0;
  pthread_cond_signal(&go1);
  pthread_cond_signal(&go2);
#endif
  cpu2go = cpu3go = 1;
}
