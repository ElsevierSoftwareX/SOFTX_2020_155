pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t go1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t go2 = PTHREAD_COND_INITIALIZER;
pthread_cond_t done = PTHREAD_COND_INITIALIZER;
volatile unsigned int waiting1 = 1;
volatile unsigned int waiting2 = 1;
volatile unsigned int done1 = 0;
volatile unsigned int done2 = 0;


void feCodeOptic(double dWord[][32],int dacOut[][16],FILT_MOD *dspPtr,COEF *dspCoeff,CDS_EPICS *pLocalEpics, int optic)
{
  double fmIn;
  double senOut[5];
  double dofOut[3];
  double cOut[4];
  int ii, jj;

	/* ADC Mapping: *************************************************
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
	*/

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
