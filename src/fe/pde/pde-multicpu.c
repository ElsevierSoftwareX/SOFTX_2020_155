// ******* This is a computer generated file *******
// ******* DO NOT HAND EDIT ************************

volatile int go = 0;
volatile int done = 0;
volatile int go1 = 0;
volatile int done1 = 0;

#if 1
//double dWord[1][32] = {{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
double sus_etmx_ascp;
double sus_etmx_ascy;
static float sus_etmx_ground;
static float sus_etmx_ground1;
double sus_etmx_inmtrx[3][4];
double sus_etmx_llout = 0.0;
double sus_etmx_llpit;
double sus_etmx_llpos;
double sus_etmx_llsen;
double sus_etmx_llyaw;
double sus_etmx_lrout = 0.0;
double sus_etmx_lrpit;
double sus_etmx_lrpos;
double sus_etmx_lrsen;
double sus_etmx_lryaw;
double sus_etmx_lsc;
double sus_etmx_pit;
double sus_etmx_pos;
double sus_etmx_side = 0.0;
double sus_etmx_sum;
double sus_etmx_sum1;
double sus_etmx_sum2;
double sus_etmx_sum3;
double sus_etmx_sum4;
double sus_etmx_sum5;
double sus_etmx_sum6;
double sus_etmx_ulout = 0.0;
double sus_etmx_ulpit;
double sus_etmx_ulpos;
double sus_etmx_ulsen;
double sus_etmx_ulyaw;
double sus_etmx_urout = 0.0;
double sus_etmx_urpit;
double sus_etmx_urpos;
double sus_etmx_ursen;
double sus_etmx_uryaw;
double sus_etmx_yaw;
double sus_etmy_ascp;
double sus_etmy_ascy;
static float sus_etmy_ground1;
static float sus_etmy_ground2;
double sus_etmy_inmtrx[3][4];
double sus_etmy_llout = 0.0;
double sus_etmy_llpit;
double sus_etmy_llpos;
double sus_etmy_llsen;
double sus_etmy_llyaw;
double sus_etmy_lrout = 0.0;
double sus_etmy_lrpit;
double sus_etmy_lrpos;
double sus_etmy_lrsen;
double sus_etmy_lryaw;
double sus_etmy_lsc;
double sus_etmy_pit;
double sus_etmy_pos;
double sus_etmy_side = 0.0;
double sus_etmy_sum;
double sus_etmy_sum1;
double sus_etmy_sum2;
double sus_etmy_sum3;
double sus_etmy_sum4;
double sus_etmy_sum5;
double sus_etmy_sum6;
double sus_etmy_ulout = 0.0;
double sus_etmy_ulpit;
double sus_etmy_ulpos;
double sus_etmy_ulsen;
double sus_etmy_ulyaw;
double sus_etmy_urout = 0.0;
double sus_etmy_urpit;
double sus_etmy_urpos;
double sus_etmy_ursen;
double sus_etmy_uryaw;
double sus_etmy_yaw;
#endif
#if 1
double sus_itmx_ascp;
double sus_itmx_ascy;
double sus_itmx_inmtrx[3][4];
double sus_itmx_llout = 0.0;
double sus_itmx_llpit;
double sus_itmx_llpos;
double sus_itmx_llsen;
double sus_itmx_llyaw;
double sus_itmx_lrout = 0.0;
double sus_itmx_lrpit;
double sus_itmx_lrpos;
double sus_itmx_lrsen;
double sus_itmx_lryaw;
double sus_itmx_lsc;
double sus_itmx_pit;
double sus_itmx_pos;
double sus_itmx_side;
double sus_itmx_sum;
double sus_itmx_sum1;
double sus_itmx_sum2;
double sus_itmx_sum3;
double sus_itmx_sum4;
double sus_itmx_sum5;
double sus_itmx_sum6;
double sus_itmx_ulout = 0.0;
double sus_itmx_ulpit;
double sus_itmx_ulpos;
double sus_itmx_ulsen;
double sus_itmx_ulyaw;
double sus_itmx_urout = 0.0;
double sus_itmx_urpit;
double sus_itmx_urpos;
double sus_itmx_ursen;
double sus_itmx_uryaw;
double sus_itmx_yaw;
double sus_itmy_ascp;
double sus_itmy_ascy;
double sus_itmy_inmtrx[3][4];
double sus_itmy_llout = 0.0;
double sus_itmy_llpit;
double sus_itmy_llpos;
double sus_itmy_llsen;
double sus_itmy_llyaw;
double sus_itmy_lrout = 0.0;
double sus_itmy_lrpit;
double sus_itmy_lrpos;
double sus_itmy_lrsen;
double sus_itmy_lryaw;
double sus_itmy_lsc;
double sus_itmy_pit;
double sus_itmy_pos;
double sus_itmy_side = 0.0;
double sus_itmy_sum;
double sus_itmy_sum1;
double sus_itmy_sum2;
double sus_itmy_sum3;
double sus_itmy_sum4;
double sus_itmy_sum5;
double sus_itmy_sum6;
double sus_itmy_ulout = 0.0;
double sus_itmy_ulpit;
double sus_itmy_ulpos;
double sus_itmy_ulsen;
double sus_itmy_ulyaw;
double sus_itmy_urout = 0.0;
double sus_itmy_urpit;
double sus_itmy_urpos;
double sus_itmy_ursen;
double sus_itmy_uryaw;
double sus_itmy_yaw;
#endif

void cpu2_start()
{
double asc_outmtrx[4][4] = {{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1}};
double lsc_outmtrx[6][3] = {{1,1,1},{1,1,1},{1,1,1},{1,1,1},{1,1,1},{1,1,1}};
int ii;
	while(!stop_working_threads) {
		while(!go && !stop_working_threads);
		go = 0;
//Start of subsystem **************************************************

#if 0
// FILTER MODULE
sus_etmx_ulsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_ULSEN,dWord[0][0],0);

// FILTER MODULE
sus_etmx_llsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_LLSEN,dWord[0][1],0);

// FILTER MODULE
sus_etmx_ursen = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_URSEN,dWord[0][2],0);

// FILTER MODULE
sus_etmx_lrsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_LRSEN,dWord[0][3],0);

// FILTER MODULE
sus_etmx_side = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_SIDE,dWord[0][4],0);

// FILTER MODULE
sus_etmx_lsc = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_LSC,lsc_outmtrx[1][0],0);

// FILTER MODULE
sus_etmx_ascy = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_ASCY,sus_etmx_ground,0);

// FILTER MODULE
sus_etmx_ascp = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_ASCP,sus_etmx_ground1,0);

// MATRIX CALC
for(ii=0;ii<3;ii++)
{
sus_etmx_inmtrx[1][ii] = 
	pLocalEpics->pde.SUS_ETMX_INMTRX[ii][0] * sus_etmx_ulsen +
	pLocalEpics->pde.SUS_ETMX_INMTRX[ii][1] * sus_etmx_llsen +
	pLocalEpics->pde.SUS_ETMX_INMTRX[ii][2] * sus_etmx_ursen +
	pLocalEpics->pde.SUS_ETMX_INMTRX[ii][3] * sus_etmx_lrsen;
}

// FILTER MODULE
sus_etmx_pos = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_POS,sus_etmx_inmtrx[1][0],0);

// FILTER MODULE
sus_etmx_pit = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_PIT,sus_etmx_inmtrx[1][1],0);

// FILTER MODULE
sus_etmx_yaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_YAW,sus_etmx_inmtrx[1][2],0);

// SUM
sus_etmx_sum4 = sus_etmx_lsc + sus_etmx_pos;

// SUM
sus_etmx_sum5 = sus_etmx_pit + sus_etmx_ascp;

// SUM
sus_etmx_sum6 = sus_etmx_yaw + sus_etmx_ascy;

// FILTER MODULE
sus_etmx_ulpos = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_ULPOS,sus_etmx_sum4,0);

// FILTER MODULE
sus_etmx_llpos = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_LLPOS,sus_etmx_sum4,0);

// FILTER MODULE
sus_etmx_urpos = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_URPOS,sus_etmx_sum4,0);

// FILTER MODULE
sus_etmx_lrpos = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_LRPOS,sus_etmx_sum4,0);

// FILTER MODULE
sus_etmx_ulpit = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_ULPIT,sus_etmx_sum5,0);

// FILTER MODULE
sus_etmx_llpit = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_LLPIT,sus_etmx_sum5,0);

// FILTER MODULE
sus_etmx_urpit = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_URPIT,sus_etmx_sum5,0);

// FILTER MODULE
sus_etmx_lrpit = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_LRPIT,sus_etmx_sum5,0);

// FILTER MODULE
sus_etmx_ulyaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_ULYAW,sus_etmx_sum6,0);

// FILTER MODULE
sus_etmx_llyaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_LLYAW,sus_etmx_sum6,0);

// FILTER MODULE
sus_etmx_uryaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_URYAW,sus_etmx_sum6,0);

// FILTER MODULE
sus_etmx_lryaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_LRYAW,sus_etmx_sum6,0);

// SUM
sus_etmx_sum = sus_etmx_ulpos + sus_etmx_ulpit + sus_etmx_ulyaw;

// SUM
sus_etmx_sum1 = sus_etmx_llpos + sus_etmx_llpit + sus_etmx_llyaw;

// SUM
sus_etmx_sum2 = sus_etmx_urpos + sus_etmx_urpit + sus_etmx_uryaw;

// SUM
sus_etmx_sum3 = sus_etmx_lrpos + sus_etmx_lrpit + sus_etmx_lryaw;

// FILTER MODULE
sus_etmx_ulout = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_ULOUT,sus_etmx_sum,0);

// FILTER MODULE
sus_etmx_llout = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_LLOUT,sus_etmx_sum1,0);

// FILTER MODULE
sus_etmx_urout = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_UROUT,sus_etmx_sum2,0);

// FILTER MODULE
sus_etmx_lrout = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_LROUT,sus_etmx_sum3,0);


//End of subsystem **************************************************



//Start of subsystem **************************************************

// FILTER MODULE
sus_etmy_ulsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_ULSEN,dWord[0][5],0);

// FILTER MODULE
sus_etmy_llsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_LLSEN,dWord[0][6],0);

// FILTER MODULE
sus_etmy_ursen = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_URSEN,dWord[0][7],0);

// FILTER MODULE
sus_etmy_lrsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_LRSEN,dWord[0][8],0);

// FILTER MODULE
sus_etmy_side = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_SIDE,dWord[0][9],0);

// FILTER MODULE
sus_etmy_lsc = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_LSC,lsc_outmtrx[1][1],0);

// FILTER MODULE
sus_etmy_ascp = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_ASCP,sus_etmy_ground1,0);

// FILTER MODULE
sus_etmy_ascy = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_ASCY,sus_etmy_ground2,0);

// MATRIX CALC
for(ii=0;ii<3;ii++)
{
sus_etmy_inmtrx[1][ii] = 
	pLocalEpics->pde.SUS_ETMY_INMTRX[ii][0] * sus_etmy_ulsen +
	pLocalEpics->pde.SUS_ETMY_INMTRX[ii][1] * sus_etmy_llsen +
	pLocalEpics->pde.SUS_ETMY_INMTRX[ii][2] * sus_etmy_ursen +
	pLocalEpics->pde.SUS_ETMY_INMTRX[ii][3] * sus_etmy_lrsen;
}

// FILTER MODULE
sus_etmy_pos = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_POS,sus_etmy_inmtrx[1][0],0);

// FILTER MODULE
sus_etmy_pit = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_PIT,sus_etmy_inmtrx[1][1],0);

// FILTER MODULE
sus_etmy_yaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_YAW,sus_etmy_inmtrx[1][2],0);

// SUM
sus_etmy_sum4 = sus_etmy_lsc + sus_etmy_pos;

// SUM
sus_etmy_sum5 = sus_etmy_pit + sus_etmy_ascp;

// SUM
sus_etmy_sum6 = sus_etmy_yaw + sus_etmy_ascy;

// FILTER MODULE
sus_etmy_ulpos = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_ULPOS,sus_etmy_sum4,0);

// FILTER MODULE
sus_etmy_llpos = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_LLPOS,sus_etmy_sum4,0);

// FILTER MODULE
sus_etmy_urpos = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_URPOS,sus_etmy_sum4,0);

// FILTER MODULE
sus_etmy_lrpos = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_LRPOS,sus_etmy_sum4,0);

// FILTER MODULE
sus_etmy_ulpit = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_ULPIT,sus_etmy_sum5,0);

// FILTER MODULE
sus_etmy_llpit = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_LLPIT,sus_etmy_sum5,0);

// FILTER MODULE
sus_etmy_urpit = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_URPIT,sus_etmy_sum5,0);

// FILTER MODULE
sus_etmy_lrpit = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_LRPIT,sus_etmy_sum5,0);

// FILTER MODULE
sus_etmy_ulyaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_ULYAW,sus_etmy_sum6,0);

// FILTER MODULE
sus_etmy_llyaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_LLYAW,sus_etmy_sum6,0);

// FILTER MODULE
sus_etmy_uryaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_URYAW,sus_etmy_sum6,0);

// FILTER MODULE
sus_etmy_lryaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_LRYAW,sus_etmy_sum6,0);

// SUM
sus_etmy_sum = sus_etmy_ulpos + sus_etmy_ulpit + sus_etmy_ulyaw;

// SUM
sus_etmy_sum1 = sus_etmy_llpos + sus_etmy_llpit + sus_etmy_llyaw;

// SUM
sus_etmy_sum2 = sus_etmy_urpos + sus_etmy_urpit + sus_etmy_uryaw;

// SUM
sus_etmy_sum3 = sus_etmy_lrpos + sus_etmy_lrpit + sus_etmy_lryaw;

// FILTER MODULE
sus_etmy_ulout = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_ULOUT,sus_etmy_sum,0);

// FILTER MODULE
sus_etmy_llout = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_LLOUT,sus_etmy_sum1,0);

// FILTER MODULE
sus_etmy_urout = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_UROUT,sus_etmy_sum2,0);

// FILTER MODULE
sus_etmy_lrout = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_LROUT,sus_etmy_sum3,0);


//End of subsystem **************************************************
#endif
#if 1
//Start of subsystem **************************************************

// FILTER MODULE
sus_itmx_ulsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_ULSEN,dWord[0][10],0);

// FILTER MODULE
sus_itmx_llsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_LLSEN,dWord[0][11],0);

// FILTER MODULE
sus_itmx_ursen = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_URSEN,dWord[0][12],0);

// FILTER MODULE
sus_itmx_lrsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_LRSEN,dWord[0][13],0);

// FILTER MODULE
sus_itmx_side = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_SIDE,dWord[0][14],0);

// FILTER MODULE
sus_itmx_lsc = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_LSC,lsc_outmtrx[1][2],0);

// FILTER MODULE
sus_itmx_ascp = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_ASCP,asc_outmtrx[1][0],0);

// FILTER MODULE
sus_itmx_ascy = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_ASCY,asc_outmtrx[1][1],0);

// MATRIX CALC
for(ii=0;ii<3;ii++)
{
sus_itmx_inmtrx[1][ii] = 
	pLocalEpics->pde.SUS_ITMX_INMTRX[ii][0] * sus_itmx_ulsen +
	pLocalEpics->pde.SUS_ITMX_INMTRX[ii][1] * sus_itmx_llsen +
	pLocalEpics->pde.SUS_ITMX_INMTRX[ii][2] * sus_itmx_ursen +
	pLocalEpics->pde.SUS_ITMX_INMTRX[ii][3] * sus_itmx_lrsen;
}

// FILTER MODULE
sus_itmx_pos = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_POS,sus_itmx_inmtrx[1][0],0);

// FILTER MODULE
sus_itmx_pit = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_PIT,sus_itmx_inmtrx[1][1],0);

// FILTER MODULE
sus_itmx_yaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_YAW,sus_itmx_inmtrx[1][2],0);

// SUM
sus_itmx_sum4 = sus_itmx_lsc + sus_itmx_pos;

// SUM
sus_itmx_sum5 = sus_itmx_pit + sus_itmx_ascp;

// SUM
sus_itmx_sum6 = sus_itmx_yaw + sus_itmx_ascy;

// FILTER MODULE
sus_itmx_ulpos = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_ULPOS,sus_itmx_sum4,0);

// FILTER MODULE
sus_itmx_llpos = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_LLPOS,sus_itmx_sum4,0);

// FILTER MODULE
sus_itmx_urpos = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_URPOS,sus_itmx_sum4,0);

// FILTER MODULE
sus_itmx_lrpos = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_LRPOS,sus_itmx_sum4,0);

// FILTER MODULE
sus_itmx_ulpit = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_ULPIT,sus_itmx_sum5,0);

// FILTER MODULE
sus_itmx_llpit = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_LLPIT,sus_itmx_sum5,0);

// FILTER MODULE
sus_itmx_urpit = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_URPIT,sus_itmx_sum5,0);

// FILTER MODULE
sus_itmx_lrpit = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_LRPIT,sus_itmx_sum5,0);

// FILTER MODULE
sus_itmx_ulyaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_ULYAW,sus_itmx_sum6,0);

// FILTER MODULE
sus_itmx_llyaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_LLYAW,sus_itmx_sum6,0);

// FILTER MODULE
sus_itmx_uryaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_URYAW,sus_itmx_sum6,0);

// FILTER MODULE
sus_itmx_lryaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_LRYAW,sus_itmx_sum6,0);

// SUM
sus_itmx_sum = sus_itmx_ulpos + sus_itmx_ulpit + sus_itmx_ulyaw;

// SUM
sus_itmx_sum1 = sus_itmx_llpos + sus_itmx_llpit + sus_itmx_llyaw;

// SUM
sus_itmx_sum2 = sus_itmx_urpos + sus_itmx_urpit + sus_itmx_uryaw;

// SUM
sus_itmx_sum3 = sus_itmx_lrpos + sus_itmx_lrpit + sus_itmx_lryaw;

// FILTER MODULE
sus_itmx_ulout = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_ULOUT,sus_itmx_sum,0);

// FILTER MODULE
sus_itmx_llout = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_LLOUT,sus_itmx_sum1,0);

// FILTER MODULE
sus_itmx_urout = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_UROUT,sus_itmx_sum2,0);

// FILTER MODULE
sus_itmx_lrout = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_LROUT,sus_itmx_sum3,0);


//End of subsystem **************************************************



//Start of subsystem **************************************************

// FILTER MODULE
sus_itmy_ulsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_ULSEN,dWord[0][15],0);

// FILTER MODULE
sus_itmy_llsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_LLSEN,dWord[0][16],0);

// FILTER MODULE
sus_itmy_ursen = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_URSEN,dWord[0][17],0);

// FILTER MODULE
sus_itmy_lrsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_LRSEN,dWord[0][18],0);

// FILTER MODULE
sus_itmy_side = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_SIDE,dWord[0][19],0);

// FILTER MODULE
sus_itmy_lsc = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_LSC,lsc_outmtrx[1][3],0);

// FILTER MODULE
sus_itmy_ascp = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_ASCP,asc_outmtrx[1][2],0);

// FILTER MODULE
sus_itmy_ascy = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_ASCY,asc_outmtrx[1][3],0);

// MATRIX CALC
for(ii=0;ii<3;ii++)
{
sus_itmy_inmtrx[1][ii] = 
	pLocalEpics->pde.SUS_ITMY_INMTRX[ii][0] * sus_itmy_ulsen +
	pLocalEpics->pde.SUS_ITMY_INMTRX[ii][1] * sus_itmy_llsen +
	pLocalEpics->pde.SUS_ITMY_INMTRX[ii][2] * sus_itmy_ursen +
	pLocalEpics->pde.SUS_ITMY_INMTRX[ii][3] * sus_itmy_lrsen;
}

// FILTER MODULE
sus_itmy_pos = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_POS,sus_itmy_inmtrx[1][0],0);

// FILTER MODULE
sus_itmy_pit = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_PIT,sus_itmy_inmtrx[1][1],0);

// FILTER MODULE
sus_itmy_yaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_YAW,sus_itmy_inmtrx[1][2],0);

// SUM
sus_itmy_sum4 = sus_itmy_lsc + sus_itmy_pos;

// SUM
sus_itmy_sum5 = sus_itmy_pit + sus_itmy_ascp;

// SUM
sus_itmy_sum6 = sus_itmy_yaw + sus_itmy_ascy;

// FILTER MODULE
sus_itmy_ulpos = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_ULPOS,sus_itmy_sum4,0);

// FILTER MODULE
sus_itmy_llpos = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_LLPOS,sus_itmy_sum4,0);

// FILTER MODULE
sus_itmy_urpos = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_URPOS,sus_itmy_sum4,0);

// FILTER MODULE
sus_itmy_lrpos = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_LRPOS,sus_itmy_sum4,0);

// FILTER MODULE
sus_itmy_ulpit = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_ULPIT,sus_itmy_sum5,0);

// FILTER MODULE
sus_itmy_llpit = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_LLPIT,sus_itmy_sum5,0);

// FILTER MODULE
sus_itmy_urpit = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_URPIT,sus_itmy_sum5,0);

// FILTER MODULE
sus_itmy_lrpit = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_LRPIT,sus_itmy_sum5,0);

// FILTER MODULE
sus_itmy_ulyaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_ULYAW,sus_itmy_sum6,0);

// FILTER MODULE
sus_itmy_llyaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_LLYAW,sus_itmy_sum6,0);

// FILTER MODULE
sus_itmy_uryaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_URYAW,sus_itmy_sum6,0);

// FILTER MODULE
sus_itmy_lryaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_LRYAW,sus_itmy_sum6,0);

// SUM
sus_itmy_sum = sus_itmy_ulpos + sus_itmy_ulpit + sus_itmy_ulyaw;

// SUM
sus_itmy_sum1 = sus_itmy_llpos + sus_itmy_llpit + sus_itmy_llyaw;

// SUM
sus_itmy_sum2 = sus_itmy_urpos + sus_itmy_urpit + sus_itmy_uryaw;

// SUM
sus_itmy_sum3 = sus_itmy_lrpos + sus_itmy_lrpit + sus_itmy_lryaw;

// FILTER MODULE
sus_itmy_ulout = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_ULOUT,sus_itmy_sum,0);

// FILTER MODULE
sus_itmy_llout = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_LLOUT,sus_itmy_sum1,0);

// FILTER MODULE
sus_itmy_urout = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_UROUT,sus_itmy_sum2,0);

// FILTER MODULE
sus_itmy_lrout = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_LROUT,sus_itmy_sum3,0);
#endif
		done = 1;
	}
	done = 1;
}

void cpu3_start()
{
double asc_outmtrx[4][4] = {{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1}};
double lsc_outmtrx[6][3] = {{1,1,1},{1,1,1},{1,1,1},{1,1,1},{1,1,1},{1,1,1}};
int ii;
	while(!stop_working_threads) {
		while(!go1 && !stop_working_threads);
		go1 = 0;
//Start of subsystem **************************************************

#if 1
// FILTER MODULE
sus_etmx_ulsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_ULSEN,dWord[0][0],0);

// FILTER MODULE
sus_etmx_llsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_LLSEN,dWord[0][1],0);

// FILTER MODULE
sus_etmx_ursen = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_URSEN,dWord[0][2],0);

// FILTER MODULE
sus_etmx_lrsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_LRSEN,dWord[0][3],0);

// FILTER MODULE
sus_etmx_side = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_SIDE,dWord[0][4],0);

// FILTER MODULE
sus_etmx_lsc = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_LSC,lsc_outmtrx[1][0],0);

// FILTER MODULE
sus_etmx_ascy = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_ASCY,sus_etmx_ground,0);

// FILTER MODULE
sus_etmx_ascp = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_ASCP,sus_etmx_ground1,0);

// MATRIX CALC
for(ii=0;ii<3;ii++)
{
sus_etmx_inmtrx[1][ii] = 
	pLocalEpics->pde.SUS_ETMX_INMTRX[ii][0] * sus_etmx_ulsen +
	pLocalEpics->pde.SUS_ETMX_INMTRX[ii][1] * sus_etmx_llsen +
	pLocalEpics->pde.SUS_ETMX_INMTRX[ii][2] * sus_etmx_ursen +
	pLocalEpics->pde.SUS_ETMX_INMTRX[ii][3] * sus_etmx_lrsen;
}

// FILTER MODULE
sus_etmx_pos = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_POS,sus_etmx_inmtrx[1][0],0);

// FILTER MODULE
sus_etmx_pit = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_PIT,sus_etmx_inmtrx[1][1],0);

// FILTER MODULE
sus_etmx_yaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_YAW,sus_etmx_inmtrx[1][2],0);

// SUM
sus_etmx_sum4 = sus_etmx_lsc + sus_etmx_pos;

// SUM
sus_etmx_sum5 = sus_etmx_pit + sus_etmx_ascp;

// SUM
sus_etmx_sum6 = sus_etmx_yaw + sus_etmx_ascy;

// FILTER MODULE
sus_etmx_ulpos = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_ULPOS,sus_etmx_sum4,0);

// FILTER MODULE
sus_etmx_llpos = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_LLPOS,sus_etmx_sum4,0);

// FILTER MODULE
sus_etmx_urpos = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_URPOS,sus_etmx_sum4,0);

// FILTER MODULE
sus_etmx_lrpos = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_LRPOS,sus_etmx_sum4,0);

// FILTER MODULE
sus_etmx_ulpit = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_ULPIT,sus_etmx_sum5,0);

// FILTER MODULE
sus_etmx_llpit = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_LLPIT,sus_etmx_sum5,0);

// FILTER MODULE
sus_etmx_urpit = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_URPIT,sus_etmx_sum5,0);

// FILTER MODULE
sus_etmx_lrpit = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_LRPIT,sus_etmx_sum5,0);

// FILTER MODULE
sus_etmx_ulyaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_ULYAW,sus_etmx_sum6,0);

// FILTER MODULE
sus_etmx_llyaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_LLYAW,sus_etmx_sum6,0);

// FILTER MODULE
sus_etmx_uryaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_URYAW,sus_etmx_sum6,0);

// FILTER MODULE
sus_etmx_lryaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_LRYAW,sus_etmx_sum6,0);

// SUM
sus_etmx_sum = sus_etmx_ulpos + sus_etmx_ulpit + sus_etmx_ulyaw;

// SUM
sus_etmx_sum1 = sus_etmx_llpos + sus_etmx_llpit + sus_etmx_llyaw;

// SUM
sus_etmx_sum2 = sus_etmx_urpos + sus_etmx_urpit + sus_etmx_uryaw;

// SUM
sus_etmx_sum3 = sus_etmx_lrpos + sus_etmx_lrpit + sus_etmx_lryaw;

// FILTER MODULE
sus_etmx_ulout = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_ULOUT,sus_etmx_sum,0);

// FILTER MODULE
sus_etmx_llout = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_LLOUT,sus_etmx_sum1,0);

// FILTER MODULE
sus_etmx_urout = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_UROUT,sus_etmx_sum2,0);

// FILTER MODULE
sus_etmx_lrout = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_LROUT,sus_etmx_sum3,0);


//End of subsystem **************************************************



//Start of subsystem **************************************************

// FILTER MODULE
sus_etmy_ulsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_ULSEN,dWord[0][5],0);

// FILTER MODULE
sus_etmy_llsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_LLSEN,dWord[0][6],0);

// FILTER MODULE
sus_etmy_ursen = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_URSEN,dWord[0][7],0);

// FILTER MODULE
sus_etmy_lrsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_LRSEN,dWord[0][8],0);

// FILTER MODULE
sus_etmy_side = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_SIDE,dWord[0][9],0);

// FILTER MODULE
sus_etmy_lsc = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_LSC,lsc_outmtrx[1][1],0);

// FILTER MODULE
sus_etmy_ascp = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_ASCP,sus_etmy_ground1,0);

// FILTER MODULE
sus_etmy_ascy = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_ASCY,sus_etmy_ground2,0);

// MATRIX CALC
for(ii=0;ii<3;ii++)
{
sus_etmy_inmtrx[1][ii] = 
	pLocalEpics->pde.SUS_ETMY_INMTRX[ii][0] * sus_etmy_ulsen +
	pLocalEpics->pde.SUS_ETMY_INMTRX[ii][1] * sus_etmy_llsen +
	pLocalEpics->pde.SUS_ETMY_INMTRX[ii][2] * sus_etmy_ursen +
	pLocalEpics->pde.SUS_ETMY_INMTRX[ii][3] * sus_etmy_lrsen;
}

// FILTER MODULE
sus_etmy_pos = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_POS,sus_etmy_inmtrx[1][0],0);

// FILTER MODULE
sus_etmy_pit = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_PIT,sus_etmy_inmtrx[1][1],0);

// FILTER MODULE
sus_etmy_yaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_YAW,sus_etmy_inmtrx[1][2],0);

// SUM
sus_etmy_sum4 = sus_etmy_lsc + sus_etmy_pos;

// SUM
sus_etmy_sum5 = sus_etmy_pit + sus_etmy_ascp;

// SUM
sus_etmy_sum6 = sus_etmy_yaw + sus_etmy_ascy;

// FILTER MODULE
sus_etmy_ulpos = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_ULPOS,sus_etmy_sum4,0);

// FILTER MODULE
sus_etmy_llpos = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_LLPOS,sus_etmy_sum4,0);

// FILTER MODULE
sus_etmy_urpos = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_URPOS,sus_etmy_sum4,0);

// FILTER MODULE
sus_etmy_lrpos = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_LRPOS,sus_etmy_sum4,0);

// FILTER MODULE
sus_etmy_ulpit = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_ULPIT,sus_etmy_sum5,0);

// FILTER MODULE
sus_etmy_llpit = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_LLPIT,sus_etmy_sum5,0);

// FILTER MODULE
sus_etmy_urpit = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_URPIT,sus_etmy_sum5,0);

// FILTER MODULE
sus_etmy_lrpit = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_LRPIT,sus_etmy_sum5,0);

// FILTER MODULE
sus_etmy_ulyaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_ULYAW,sus_etmy_sum6,0);

// FILTER MODULE
sus_etmy_llyaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_LLYAW,sus_etmy_sum6,0);

// FILTER MODULE
sus_etmy_uryaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_URYAW,sus_etmy_sum6,0);

// FILTER MODULE
sus_etmy_lryaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_LRYAW,sus_etmy_sum6,0);

// SUM
sus_etmy_sum = sus_etmy_ulpos + sus_etmy_ulpit + sus_etmy_ulyaw;

// SUM
sus_etmy_sum1 = sus_etmy_llpos + sus_etmy_llpit + sus_etmy_llyaw;

// SUM
sus_etmy_sum2 = sus_etmy_urpos + sus_etmy_urpit + sus_etmy_uryaw;

// SUM
sus_etmy_sum3 = sus_etmy_lrpos + sus_etmy_lrpit + sus_etmy_lryaw;

// FILTER MODULE
sus_etmy_ulout = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_ULOUT,sus_etmy_sum,0);

// FILTER MODULE
sus_etmy_llout = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_LLOUT,sus_etmy_sum1,0);

// FILTER MODULE
sus_etmy_urout = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_UROUT,sus_etmy_sum2,0);

// FILTER MODULE
sus_etmy_lrout = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_LROUT,sus_etmy_sum3,0);


//End of subsystem **************************************************
#endif
#if 0
//Start of subsystem **************************************************

// FILTER MODULE
sus_itmx_ulsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_ULSEN,dWord[0][10],0);

// FILTER MODULE
sus_itmx_llsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_LLSEN,dWord[0][11],0);

// FILTER MODULE
sus_itmx_ursen = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_URSEN,dWord[0][12],0);

// FILTER MODULE
sus_itmx_lrsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_LRSEN,dWord[0][13],0);

// FILTER MODULE
sus_itmx_side = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_SIDE,dWord[0][14],0);

// FILTER MODULE
sus_itmx_lsc = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_LSC,lsc_outmtrx[1][2],0);

// FILTER MODULE
sus_itmx_ascp = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_ASCP,asc_outmtrx[1][0],0);

// FILTER MODULE
sus_itmx_ascy = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_ASCY,asc_outmtrx[1][1],0);

// MATRIX CALC
for(ii=0;ii<3;ii++)
{
sus_itmx_inmtrx[1][ii] = 
	pLocalEpics->pde.SUS_ITMX_INMTRX[ii][0] * sus_itmx_ulsen +
	pLocalEpics->pde.SUS_ITMX_INMTRX[ii][1] * sus_itmx_llsen +
	pLocalEpics->pde.SUS_ITMX_INMTRX[ii][2] * sus_itmx_ursen +
	pLocalEpics->pde.SUS_ITMX_INMTRX[ii][3] * sus_itmx_lrsen;
}

// FILTER MODULE
sus_itmx_pos = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_POS,sus_itmx_inmtrx[1][0],0);

// FILTER MODULE
sus_itmx_pit = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_PIT,sus_itmx_inmtrx[1][1],0);

// FILTER MODULE
sus_itmx_yaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_YAW,sus_itmx_inmtrx[1][2],0);

// SUM
sus_itmx_sum4 = sus_itmx_lsc + sus_itmx_pos;

// SUM
sus_itmx_sum5 = sus_itmx_pit + sus_itmx_ascp;

// SUM
sus_itmx_sum6 = sus_itmx_yaw + sus_itmx_ascy;

// FILTER MODULE
sus_itmx_ulpos = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_ULPOS,sus_itmx_sum4,0);

// FILTER MODULE
sus_itmx_llpos = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_LLPOS,sus_itmx_sum4,0);

// FILTER MODULE
sus_itmx_urpos = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_URPOS,sus_itmx_sum4,0);

// FILTER MODULE
sus_itmx_lrpos = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_LRPOS,sus_itmx_sum4,0);

// FILTER MODULE
sus_itmx_ulpit = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_ULPIT,sus_itmx_sum5,0);

// FILTER MODULE
sus_itmx_llpit = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_LLPIT,sus_itmx_sum5,0);

// FILTER MODULE
sus_itmx_urpit = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_URPIT,sus_itmx_sum5,0);

// FILTER MODULE
sus_itmx_lrpit = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_LRPIT,sus_itmx_sum5,0);

// FILTER MODULE
sus_itmx_ulyaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_ULYAW,sus_itmx_sum6,0);

// FILTER MODULE
sus_itmx_llyaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_LLYAW,sus_itmx_sum6,0);

// FILTER MODULE
sus_itmx_uryaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_URYAW,sus_itmx_sum6,0);

// FILTER MODULE
sus_itmx_lryaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_LRYAW,sus_itmx_sum6,0);

// SUM
sus_itmx_sum = sus_itmx_ulpos + sus_itmx_ulpit + sus_itmx_ulyaw;

// SUM
sus_itmx_sum1 = sus_itmx_llpos + sus_itmx_llpit + sus_itmx_llyaw;

// SUM
sus_itmx_sum2 = sus_itmx_urpos + sus_itmx_urpit + sus_itmx_uryaw;

// SUM
sus_itmx_sum3 = sus_itmx_lrpos + sus_itmx_lrpit + sus_itmx_lryaw;

// FILTER MODULE
sus_itmx_ulout = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_ULOUT,sus_itmx_sum,0);

// FILTER MODULE
sus_itmx_llout = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_LLOUT,sus_itmx_sum1,0);

// FILTER MODULE
sus_itmx_urout = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_UROUT,sus_itmx_sum2,0);

// FILTER MODULE
sus_itmx_lrout = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_LROUT,sus_itmx_sum3,0);


//End of subsystem **************************************************



//Start of subsystem **************************************************

// FILTER MODULE
sus_itmy_ulsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_ULSEN,dWord[0][15],0);

// FILTER MODULE
sus_itmy_llsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_LLSEN,dWord[0][16],0);

// FILTER MODULE
sus_itmy_ursen = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_URSEN,dWord[0][17],0);

// FILTER MODULE
sus_itmy_lrsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_LRSEN,dWord[0][18],0);

// FILTER MODULE
sus_itmy_side = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_SIDE,dWord[0][19],0);

// FILTER MODULE
sus_itmy_lsc = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_LSC,lsc_outmtrx[1][3],0);

// FILTER MODULE
sus_itmy_ascp = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_ASCP,asc_outmtrx[1][2],0);

// FILTER MODULE
sus_itmy_ascy = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_ASCY,asc_outmtrx[1][3],0);

// MATRIX CALC
for(ii=0;ii<3;ii++)
{
sus_itmy_inmtrx[1][ii] = 
	pLocalEpics->pde.SUS_ITMY_INMTRX[ii][0] * sus_itmy_ulsen +
	pLocalEpics->pde.SUS_ITMY_INMTRX[ii][1] * sus_itmy_llsen +
	pLocalEpics->pde.SUS_ITMY_INMTRX[ii][2] * sus_itmy_ursen +
	pLocalEpics->pde.SUS_ITMY_INMTRX[ii][3] * sus_itmy_lrsen;
}

// FILTER MODULE
sus_itmy_pos = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_POS,sus_itmy_inmtrx[1][0],0);

// FILTER MODULE
sus_itmy_pit = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_PIT,sus_itmy_inmtrx[1][1],0);

// FILTER MODULE
sus_itmy_yaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_YAW,sus_itmy_inmtrx[1][2],0);

// SUM
sus_itmy_sum4 = sus_itmy_lsc + sus_itmy_pos;

// SUM
sus_itmy_sum5 = sus_itmy_pit + sus_itmy_ascp;

// SUM
sus_itmy_sum6 = sus_itmy_yaw + sus_itmy_ascy;

// FILTER MODULE
sus_itmy_ulpos = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_ULPOS,sus_itmy_sum4,0);

// FILTER MODULE
sus_itmy_llpos = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_LLPOS,sus_itmy_sum4,0);

// FILTER MODULE
sus_itmy_urpos = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_URPOS,sus_itmy_sum4,0);

// FILTER MODULE
sus_itmy_lrpos = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_LRPOS,sus_itmy_sum4,0);

// FILTER MODULE
sus_itmy_ulpit = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_ULPIT,sus_itmy_sum5,0);

// FILTER MODULE
sus_itmy_llpit = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_LLPIT,sus_itmy_sum5,0);

// FILTER MODULE
sus_itmy_urpit = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_URPIT,sus_itmy_sum5,0);

// FILTER MODULE
sus_itmy_lrpit = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_LRPIT,sus_itmy_sum5,0);

// FILTER MODULE
sus_itmy_ulyaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_ULYAW,sus_itmy_sum6,0);

// FILTER MODULE
sus_itmy_llyaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_LLYAW,sus_itmy_sum6,0);

// FILTER MODULE
sus_itmy_uryaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_URYAW,sus_itmy_sum6,0);

// FILTER MODULE
sus_itmy_lryaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_LRYAW,sus_itmy_sum6,0);

// SUM
sus_itmy_sum = sus_itmy_ulpos + sus_itmy_ulpit + sus_itmy_ulyaw;

// SUM
sus_itmy_sum1 = sus_itmy_llpos + sus_itmy_llpit + sus_itmy_llyaw;

// SUM
sus_itmy_sum2 = sus_itmy_urpos + sus_itmy_urpit + sus_itmy_uryaw;

// SUM
sus_itmy_sum3 = sus_itmy_lrpos + sus_itmy_lrpit + sus_itmy_lryaw;

// FILTER MODULE
sus_itmy_ulout = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_ULOUT,sus_itmy_sum,0);

// FILTER MODULE
sus_itmy_llout = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_LLOUT,sus_itmy_sum1,0);

// FILTER MODULE
sus_itmy_urout = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_UROUT,sus_itmy_sum2,0);

// FILTER MODULE
sus_itmy_lrout = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_LROUT,sus_itmy_sum3,0);
#endif
		done1 = 1;
	}
	done1 = 1;
}

void feCode(int cycle, double dWord[][32],	/* ADC inputs */
		int dacOut[][16],	/* DAC outputs */
		FILT_MOD *dspPtr,	/* Filter Mod variables */
		COEF *dspCoeff,		/* Filter Mod coeffs */
		CDS_EPICS *pLocalEpics,	/* EPICS variables */
		int feInit)	/* Initialization flag */
{


int ii,jj;

double asc_inmtrx[4][4];
double asc_i_mtrx[3][4];
double asc_i_mtrx1[3][4];
double asc_outmtrx[4][4];
double asc_wfs1_i1;
double asc_wfs1_i2;
double asc_wfs1_i3;
double asc_wfs1_i4;
double asc_wfs1_pit;
double asc_wfs1_q1;
double asc_wfs1_q2;
double asc_wfs1_q3;
double asc_wfs1_q4;
double asc_wfs1_yaw;
double asc_wfs2_pit;
double asc_wfs2_yaw;
double lsc_as1dc;
double lsc_as1i;
double lsc_as1q;
double lsc_as2dc;
double lsc_as3dc;
double lsc_carm;
double lsc_darm;
double lsc_inmtrx[3][4];
double lsc_lock_dc;
double lsc_outmtrx[6][3];
double lsc_prc;
double lsc_refli;
double lsc_reflq;
double lsc_x_trans;
double lsc_y_trans;
double sus_bs_ascp;
double sus_bs_ascy;
static float sus_bs_ground1;
static float sus_bs_ground2;
double sus_bs_inmtrx[3][4];
double sus_bs_llout;
double sus_bs_llpit;
double sus_bs_llpos;
double sus_bs_llsen;
double sus_bs_llyaw;
double sus_bs_lrout;
double sus_bs_lrpit;
double sus_bs_lrpos;
double sus_bs_lrsen;
double sus_bs_lryaw;
double sus_bs_lsc;
double sus_bs_pit;
double sus_bs_pos;
double sus_bs_side;
double sus_bs_sum;
double sus_bs_sum1;
double sus_bs_sum2;
double sus_bs_sum3;
double sus_bs_sum4;
double sus_bs_sum5;
double sus_bs_sum6;
double sus_bs_ulout;
double sus_bs_ulpit;
double sus_bs_ulpos;
double sus_bs_ulsen;
double sus_bs_ulyaw;
double sus_bs_urout;
double sus_bs_urpit;
double sus_bs_urpos;
double sus_bs_ursen;
double sus_bs_uryaw;
double sus_bs_yaw;
#if 0
double sus_etmx_ascp;
double sus_etmx_ascy;
static float sus_etmx_ground;
static float sus_etmx_ground1;
double sus_etmx_inmtrx[3][4];
double sus_etmx_llout;
double sus_etmx_llpit;
double sus_etmx_llpos;
double sus_etmx_llsen;
double sus_etmx_llyaw;
double sus_etmx_lrout;
double sus_etmx_lrpit;
double sus_etmx_lrpos;
double sus_etmx_lrsen;
double sus_etmx_lryaw;
double sus_etmx_lsc;
double sus_etmx_pit;
double sus_etmx_pos;
double sus_etmx_side;
double sus_etmx_sum;
double sus_etmx_sum1;
double sus_etmx_sum2;
double sus_etmx_sum3;
double sus_etmx_sum4;
double sus_etmx_sum5;
double sus_etmx_sum6;
double sus_etmx_ulout;
double sus_etmx_ulpit;
double sus_etmx_ulpos;
double sus_etmx_ulsen;
double sus_etmx_ulyaw;
double sus_etmx_urout;
double sus_etmx_urpit;
double sus_etmx_urpos;
double sus_etmx_ursen;
double sus_etmx_uryaw;
double sus_etmx_yaw;
double sus_etmy_ascp;
double sus_etmy_ascy;
static float sus_etmy_ground1;
static float sus_etmy_ground2;
double sus_etmy_inmtrx[3][4];
double sus_etmy_llout;
double sus_etmy_llpit;
double sus_etmy_llpos;
double sus_etmy_llsen;
double sus_etmy_llyaw;
double sus_etmy_lrout;
double sus_etmy_lrpit;
double sus_etmy_lrpos;
double sus_etmy_lrsen;
double sus_etmy_lryaw;
double sus_etmy_lsc;
double sus_etmy_pit;
double sus_etmy_pos;
double sus_etmy_side;
double sus_etmy_sum;
double sus_etmy_sum1;
double sus_etmy_sum2;
double sus_etmy_sum3;
double sus_etmy_sum4;
double sus_etmy_sum5;
double sus_etmy_sum6;
double sus_etmy_ulout;
double sus_etmy_ulpit;
double sus_etmy_ulpos;
double sus_etmy_ulsen;
double sus_etmy_ulyaw;
double sus_etmy_urout;
double sus_etmy_urpit;
double sus_etmy_urpos;
double sus_etmy_ursen;
double sus_etmy_uryaw;
double sus_etmy_yaw;
#endif
#if 0
double sus_itmx_ascp;
double sus_itmx_ascy;
double sus_itmx_inmtrx[3][4];
double sus_itmx_llout;
double sus_itmx_llpit;
double sus_itmx_llpos;
double sus_itmx_llsen;
double sus_itmx_llyaw;
double sus_itmx_lrout;
double sus_itmx_lrpit;
double sus_itmx_lrpos;
double sus_itmx_lrsen;
double sus_itmx_lryaw;
double sus_itmx_lsc;
double sus_itmx_pit;
double sus_itmx_pos;
double sus_itmx_side;
double sus_itmx_sum;
double sus_itmx_sum1;
double sus_itmx_sum2;
double sus_itmx_sum3;
double sus_itmx_sum4;
double sus_itmx_sum5;
double sus_itmx_sum6;
double sus_itmx_ulout;
double sus_itmx_ulpit;
double sus_itmx_ulpos;
double sus_itmx_ulsen;
double sus_itmx_ulyaw;
double sus_itmx_urout;
double sus_itmx_urpit;
double sus_itmx_urpos;
double sus_itmx_ursen;
double sus_itmx_uryaw;
double sus_itmx_yaw;
double sus_itmy_ascp;
double sus_itmy_ascy;
double sus_itmy_inmtrx[3][4];
double sus_itmy_llout;
double sus_itmy_llpit;
double sus_itmy_llpos;
double sus_itmy_llsen;
double sus_itmy_llyaw;
double sus_itmy_lrout;
double sus_itmy_lrpit;
double sus_itmy_lrpos;
double sus_itmy_lrsen;
double sus_itmy_lryaw;
double sus_itmy_lsc;
double sus_itmy_pit;
double sus_itmy_pos;
double sus_itmy_side;
double sus_itmy_sum;
double sus_itmy_sum1;
double sus_itmy_sum2;
double sus_itmy_sum3;
double sus_itmy_sum4;
double sus_itmy_sum5;
double sus_itmy_sum6;
double sus_itmy_ulout;
double sus_itmy_ulpit;
double sus_itmy_ulpos;
double sus_itmy_ulsen;
double sus_itmy_ulyaw;
double sus_itmy_urout;
double sus_itmy_urpit;
double sus_itmy_urpos;
double sus_itmy_ursen;
double sus_itmy_uryaw;
double sus_itmy_yaw;
#endif

if(feInit)
{
sus_bs_ground1 = 0.0;
sus_bs_ground2 = 0.0;
sus_etmx_ground = 0.0;
sus_etmx_ground1 = 0.0;
sus_etmy_ground1 = 0.0;
sus_etmy_ground2 = 0.0;
} else {

//Start of subsystem **************************************************

// FILTER MODULE
asc_wfs1_i1 = filterModuleD(dspPtr,dspCoeff,ASC_WFS1_I1,dWord[1][16],0);

// FILTER MODULE
asc_wfs1_i2 = filterModuleD(dspPtr,dspCoeff,ASC_WFS1_I2,dWord[1][17],0);

// FILTER MODULE
asc_wfs1_i3 = filterModuleD(dspPtr,dspCoeff,ASC_WFS1_I3,dWord[1][18],0);

// FILTER MODULE
asc_wfs1_i4 = filterModuleD(dspPtr,dspCoeff,ASC_WFS1_I4,dWord[1][19],0);

// FILTER MODULE
asc_wfs1_q1 = filterModuleD(dspPtr,dspCoeff,ASC_WFS1_Q1,dWord[1][20],0);

// FILTER MODULE
asc_wfs1_q2 = filterModuleD(dspPtr,dspCoeff,ASC_WFS1_Q2,dWord[1][21],0);

// FILTER MODULE
asc_wfs1_q3 = filterModuleD(dspPtr,dspCoeff,ASC_WFS1_Q3,dWord[1][22],0);

// FILTER MODULE
asc_wfs1_q4 = filterModuleD(dspPtr,dspCoeff,ASC_WFS1_Q4,dWord[1][23],0);

// MATRIX CALC
for(ii=0;ii<3;ii++)
{
asc_i_mtrx[1][ii] = 
	pLocalEpics->pde.ASC_I_MTRX[ii][0] * asc_wfs1_i1 +
	pLocalEpics->pde.ASC_I_MTRX[ii][1] * asc_wfs1_i2 +
	pLocalEpics->pde.ASC_I_MTRX[ii][2] * asc_wfs1_i3 +
	pLocalEpics->pde.ASC_I_MTRX[ii][3] * asc_wfs1_i4;
}

// MATRIX CALC
for(ii=0;ii<3;ii++)
{
asc_i_mtrx1[1][ii] = 
	pLocalEpics->pde.ASC_I_MTRX1[ii][0] * asc_wfs1_q1 +
	pLocalEpics->pde.ASC_I_MTRX1[ii][1] * asc_wfs1_q2 +
	pLocalEpics->pde.ASC_I_MTRX1[ii][2] * asc_wfs1_q3 +
	pLocalEpics->pde.ASC_I_MTRX1[ii][3] * asc_wfs1_q4;
}


// EPICS_OUTPUT
pLocalEpics->pde.ASC_I_PIT = asc_i_mtrx[1][0];

// EPICS_OUTPUT
pLocalEpics->pde.ASC_I_YAW = asc_i_mtrx[1][1];

// EPICS_OUTPUT
pLocalEpics->pde.ASC_I_SUM = asc_i_mtrx[1][2];

// EPICS_OUTPUT
pLocalEpics->pde.ASC_Q_PIT = asc_i_mtrx1[1][0];

// EPICS_OUTPUT
pLocalEpics->pde.ASC_Q_YAW = asc_i_mtrx1[1][1];

// EPICS_OUTPUT
pLocalEpics->pde.ASC_Q_SUM = asc_i_mtrx1[1][2];

// MATRIX CALC
for(ii=0;ii<4;ii++)
{
asc_inmtrx[1][ii] = 
	pLocalEpics->pde.ASC_INMTRX[ii][0] * pLocalEpics->pde.ASC_I_PIT +
	pLocalEpics->pde.ASC_INMTRX[ii][1] * pLocalEpics->pde.ASC_I_YAW +
	pLocalEpics->pde.ASC_INMTRX[ii][2] * pLocalEpics->pde.ASC_Q_PIT +
	pLocalEpics->pde.ASC_INMTRX[ii][3] * pLocalEpics->pde.ASC_Q_YAW;
}

// FILTER MODULE
asc_wfs1_pit = filterModuleD(dspPtr,dspCoeff,ASC_WFS1_PIT,asc_inmtrx[1][0],0);

// FILTER MODULE
asc_wfs1_yaw = filterModuleD(dspPtr,dspCoeff,ASC_WFS1_YAW,asc_inmtrx[1][1],0);

// FILTER MODULE
asc_wfs2_pit = filterModuleD(dspPtr,dspCoeff,ASC_WFS2_PIT,asc_inmtrx[1][2],0);

// FILTER MODULE
asc_wfs2_yaw = filterModuleD(dspPtr,dspCoeff,ASC_WFS2_YAW,asc_inmtrx[1][3],0);

// MATRIX CALC
for(ii=0;ii<4;ii++)
{
asc_outmtrx[1][ii] = 
	pLocalEpics->pde.ASC_OUTMTRX[ii][0] * asc_wfs1_pit +
	pLocalEpics->pde.ASC_OUTMTRX[ii][1] * asc_wfs1_yaw +
	pLocalEpics->pde.ASC_OUTMTRX[ii][2] * asc_wfs2_pit +
	pLocalEpics->pde.ASC_OUTMTRX[ii][3] * asc_wfs2_yaw;
}


//End of subsystem **************************************************



//Start of subsystem **************************************************

// FILTER MODULE
lsc_refli = filterModuleD(dspPtr,dspCoeff,LSC_REFLI,dWord[1][0],0);

// FILTER MODULE
lsc_reflq = filterModuleD(dspPtr,dspCoeff,LSC_REFLQ,dWord[1][1],0);

// FILTER MODULE
lsc_as1i = filterModuleD(dspPtr,dspCoeff,LSC_AS1I,dWord[1][2],0);

// FILTER MODULE
lsc_as1q = filterModuleD(dspPtr,dspCoeff,LSC_AS1Q,dWord[1][3],0);

// FILTER MODULE
lsc_as1dc = filterModuleD(dspPtr,dspCoeff,LSC_AS1DC,dWord[1][4],0);

// FILTER MODULE
lsc_as2dc = filterModuleD(dspPtr,dspCoeff,LSC_AS2DC,dWord[1][5],0);

// FILTER MODULE
lsc_as3dc = filterModuleD(dspPtr,dspCoeff,LSC_AS3DC,dWord[1][6],0);

// FILTER MODULE
lsc_lock_dc = filterModuleD(dspPtr,dspCoeff,LSC_LOCK_DC,dWord[1][7],0);

// FILTER MODULE
lsc_x_trans = filterModuleD(dspPtr,dspCoeff,LSC_X_TRANS,dWord[1][8],0);

// FILTER MODULE
lsc_y_trans = filterModuleD(dspPtr,dspCoeff,LSC_Y_TRANS,dWord[1][9],0);

// MATRIX CALC
for(ii=0;ii<3;ii++)
{
lsc_inmtrx[1][ii] = 
	pLocalEpics->pde.LSC_INMTRX[ii][0] * lsc_refli +
	pLocalEpics->pde.LSC_INMTRX[ii][1] * lsc_reflq +
	pLocalEpics->pde.LSC_INMTRX[ii][2] * lsc_as1i +
	pLocalEpics->pde.LSC_INMTRX[ii][3] * lsc_as1q;
}

// FILTER MODULE
lsc_darm = filterModuleD(dspPtr,dspCoeff,LSC_DARM,lsc_inmtrx[1][0],0);

// FILTER MODULE
lsc_carm = filterModuleD(dspPtr,dspCoeff,LSC_CARM,lsc_inmtrx[1][1],0);

// FILTER MODULE
lsc_prc = filterModuleD(dspPtr,dspCoeff,LSC_PRC,lsc_inmtrx[1][2],0);

// MATRIX CALC
for(ii=0;ii<6;ii++)
{
lsc_outmtrx[1][ii] = 
	pLocalEpics->pde.LSC_OUTMTRX[ii][0] * lsc_darm +
	pLocalEpics->pde.LSC_OUTMTRX[ii][1] * lsc_carm +
	pLocalEpics->pde.LSC_OUTMTRX[ii][2] * lsc_prc;
}


go = 1;
go1 = 1;




//End of subsystem **************************************************



//Start of subsystem **************************************************

// FILTER MODULE
sus_bs_ulsen = filterModuleD(dspPtr,dspCoeff,SUS_BS_ULSEN,dWord[0][20],0);

// FILTER MODULE
sus_bs_llsen = filterModuleD(dspPtr,dspCoeff,SUS_BS_LLSEN,dWord[0][21],0);

// FILTER MODULE
sus_bs_ursen = filterModuleD(dspPtr,dspCoeff,SUS_BS_URSEN,dWord[0][22],0);

// FILTER MODULE
sus_bs_lrsen = filterModuleD(dspPtr,dspCoeff,SUS_BS_LRSEN,dWord[0][23],0);

// FILTER MODULE
sus_bs_side = filterModuleD(dspPtr,dspCoeff,SUS_BS_SIDE,dWord[0][24],0);

// FILTER MODULE
sus_bs_lsc = filterModuleD(dspPtr,dspCoeff,SUS_BS_LSC,lsc_outmtrx[1][4],0);

// FILTER MODULE
sus_bs_ascp = filterModuleD(dspPtr,dspCoeff,SUS_BS_ASCP,sus_bs_ground1,0);

// FILTER MODULE
sus_bs_ascy = filterModuleD(dspPtr,dspCoeff,SUS_BS_ASCY,sus_bs_ground2,0);

// MATRIX CALC
for(ii=0;ii<3;ii++)
{
sus_bs_inmtrx[1][ii] = 
	pLocalEpics->pde.SUS_BS_INMTRX[ii][0] * sus_bs_ulsen +
	pLocalEpics->pde.SUS_BS_INMTRX[ii][1] * sus_bs_llsen +
	pLocalEpics->pde.SUS_BS_INMTRX[ii][2] * sus_bs_ursen +
	pLocalEpics->pde.SUS_BS_INMTRX[ii][3] * sus_bs_lrsen;
}

// FILTER MODULE
sus_bs_pos = filterModuleD(dspPtr,dspCoeff,SUS_BS_POS,sus_bs_inmtrx[1][0],0);

// FILTER MODULE
sus_bs_pit = filterModuleD(dspPtr,dspCoeff,SUS_BS_PIT,sus_bs_inmtrx[1][1],0);

// FILTER MODULE
sus_bs_yaw = filterModuleD(dspPtr,dspCoeff,SUS_BS_YAW,sus_bs_inmtrx[1][2],0);

// SUM
sus_bs_sum4 = sus_bs_lsc + sus_bs_pos;

// SUM
sus_bs_sum5 = sus_bs_pit + sus_bs_ascp;

// SUM
sus_bs_sum6 = sus_bs_yaw + sus_bs_ascy;

// FILTER MODULE
sus_bs_ulpos = filterModuleD(dspPtr,dspCoeff,SUS_BS_ULPOS,sus_bs_sum4,0);

// FILTER MODULE
sus_bs_llpos = filterModuleD(dspPtr,dspCoeff,SUS_BS_LLPOS,sus_bs_sum4,0);

// FILTER MODULE
sus_bs_urpos = filterModuleD(dspPtr,dspCoeff,SUS_BS_URPOS,sus_bs_sum4,0);

// FILTER MODULE
sus_bs_lrpos = filterModuleD(dspPtr,dspCoeff,SUS_BS_LRPOS,sus_bs_sum4,0);

// FILTER MODULE
sus_bs_ulpit = filterModuleD(dspPtr,dspCoeff,SUS_BS_ULPIT,sus_bs_sum5,0);

// FILTER MODULE
sus_bs_llpit = filterModuleD(dspPtr,dspCoeff,SUS_BS_LLPIT,sus_bs_sum5,0);

// FILTER MODULE
sus_bs_urpit = filterModuleD(dspPtr,dspCoeff,SUS_BS_URPIT,sus_bs_sum5,0);

// FILTER MODULE
sus_bs_lrpit = filterModuleD(dspPtr,dspCoeff,SUS_BS_LRPIT,sus_bs_sum5,0);

// FILTER MODULE
sus_bs_ulyaw = filterModuleD(dspPtr,dspCoeff,SUS_BS_ULYAW,sus_bs_sum6,0);

// FILTER MODULE
sus_bs_llyaw = filterModuleD(dspPtr,dspCoeff,SUS_BS_LLYAW,sus_bs_sum6,0);

// FILTER MODULE
sus_bs_uryaw = filterModuleD(dspPtr,dspCoeff,SUS_BS_URYAW,sus_bs_sum6,0);

// FILTER MODULE
sus_bs_lryaw = filterModuleD(dspPtr,dspCoeff,SUS_BS_LRYAW,sus_bs_sum6,0);

// SUM
sus_bs_sum = sus_bs_ulpos + sus_bs_ulpit + sus_bs_ulyaw;

// SUM
sus_bs_sum1 = sus_bs_llpos + sus_bs_llpit + sus_bs_llyaw;

// SUM
sus_bs_sum2 = sus_bs_urpos + sus_bs_urpit + sus_bs_uryaw;

// SUM
sus_bs_sum3 = sus_bs_lrpos + sus_bs_lrpit + sus_bs_lryaw;

// FILTER MODULE
sus_bs_ulout = filterModuleD(dspPtr,dspCoeff,SUS_BS_ULOUT,sus_bs_sum,0);

// FILTER MODULE
sus_bs_llout = filterModuleD(dspPtr,dspCoeff,SUS_BS_LLOUT,sus_bs_sum1,0);

// FILTER MODULE
sus_bs_urout = filterModuleD(dspPtr,dspCoeff,SUS_BS_UROUT,sus_bs_sum2,0);

// FILTER MODULE
sus_bs_lrout = filterModuleD(dspPtr,dspCoeff,SUS_BS_LROUT,sus_bs_sum3,0);


//End of subsystem **************************************************


#if 0
//Start of subsystem **************************************************

// FILTER MODULE
sus_etmx_ulsen = filterModuleD(dspPtr,dspCoeff,SUS_ETMX_ULSEN,dWord[0][0],0);

// FILTER MODULE
sus_etmx_llsen = filterModuleD(dspPtr,dspCoeff,SUS_ETMX_LLSEN,dWord[0][1],0);

// FILTER MODULE
sus_etmx_ursen = filterModuleD(dspPtr,dspCoeff,SUS_ETMX_URSEN,dWord[0][2],0);

// FILTER MODULE
sus_etmx_lrsen = filterModuleD(dspPtr,dspCoeff,SUS_ETMX_LRSEN,dWord[0][3],0);

// FILTER MODULE
sus_etmx_side = filterModuleD(dspPtr,dspCoeff,SUS_ETMX_SIDE,dWord[0][4],0);

// FILTER MODULE
sus_etmx_lsc = filterModuleD(dspPtr,dspCoeff,SUS_ETMX_LSC,lsc_outmtrx[1][0],0);

// FILTER MODULE
sus_etmx_ascy = filterModuleD(dspPtr,dspCoeff,SUS_ETMX_ASCY,sus_etmx_ground,0);

// FILTER MODULE
sus_etmx_ascp = filterModuleD(dspPtr,dspCoeff,SUS_ETMX_ASCP,sus_etmx_ground1,0);

// MATRIX CALC
for(ii=0;ii<3;ii++)
{
sus_etmx_inmtrx[1][ii] = 
	pLocalEpics->pde.SUS_ETMX_INMTRX[ii][0] * sus_etmx_ulsen +
	pLocalEpics->pde.SUS_ETMX_INMTRX[ii][1] * sus_etmx_llsen +
	pLocalEpics->pde.SUS_ETMX_INMTRX[ii][2] * sus_etmx_ursen +
	pLocalEpics->pde.SUS_ETMX_INMTRX[ii][3] * sus_etmx_lrsen;
}

// FILTER MODULE
sus_etmx_pos = filterModuleD(dspPtr,dspCoeff,SUS_ETMX_POS,sus_etmx_inmtrx[1][0],0);

// FILTER MODULE
sus_etmx_pit = filterModuleD(dspPtr,dspCoeff,SUS_ETMX_PIT,sus_etmx_inmtrx[1][1],0);

// FILTER MODULE
sus_etmx_yaw = filterModuleD(dspPtr,dspCoeff,SUS_ETMX_YAW,sus_etmx_inmtrx[1][2],0);

// SUM
sus_etmx_sum4 = sus_etmx_lsc + sus_etmx_pos;

// SUM
sus_etmx_sum5 = sus_etmx_pit + sus_etmx_ascp;

// SUM
sus_etmx_sum6 = sus_etmx_yaw + sus_etmx_ascy;

// FILTER MODULE
sus_etmx_ulpos = filterModuleD(dspPtr,dspCoeff,SUS_ETMX_ULPOS,sus_etmx_sum4,0);

// FILTER MODULE
sus_etmx_llpos = filterModuleD(dspPtr,dspCoeff,SUS_ETMX_LLPOS,sus_etmx_sum4,0);

// FILTER MODULE
sus_etmx_urpos = filterModuleD(dspPtr,dspCoeff,SUS_ETMX_URPOS,sus_etmx_sum4,0);

// FILTER MODULE
sus_etmx_lrpos = filterModuleD(dspPtr,dspCoeff,SUS_ETMX_LRPOS,sus_etmx_sum4,0);

// FILTER MODULE
sus_etmx_ulpit = filterModuleD(dspPtr,dspCoeff,SUS_ETMX_ULPIT,sus_etmx_sum5,0);

// FILTER MODULE
sus_etmx_llpit = filterModuleD(dspPtr,dspCoeff,SUS_ETMX_LLPIT,sus_etmx_sum5,0);

// FILTER MODULE
sus_etmx_urpit = filterModuleD(dspPtr,dspCoeff,SUS_ETMX_URPIT,sus_etmx_sum5,0);

// FILTER MODULE
sus_etmx_lrpit = filterModuleD(dspPtr,dspCoeff,SUS_ETMX_LRPIT,sus_etmx_sum5,0);

// FILTER MODULE
sus_etmx_ulyaw = filterModuleD(dspPtr,dspCoeff,SUS_ETMX_ULYAW,sus_etmx_sum6,0);

// FILTER MODULE
sus_etmx_llyaw = filterModuleD(dspPtr,dspCoeff,SUS_ETMX_LLYAW,sus_etmx_sum6,0);

// FILTER MODULE
sus_etmx_uryaw = filterModuleD(dspPtr,dspCoeff,SUS_ETMX_URYAW,sus_etmx_sum6,0);

// FILTER MODULE
sus_etmx_lryaw = filterModuleD(dspPtr,dspCoeff,SUS_ETMX_LRYAW,sus_etmx_sum6,0);

// SUM
sus_etmx_sum = sus_etmx_ulpos + sus_etmx_ulpit + sus_etmx_ulyaw;

// SUM
sus_etmx_sum1 = sus_etmx_llpos + sus_etmx_llpit + sus_etmx_llyaw;

// SUM
sus_etmx_sum2 = sus_etmx_urpos + sus_etmx_urpit + sus_etmx_uryaw;

// SUM
sus_etmx_sum3 = sus_etmx_lrpos + sus_etmx_lrpit + sus_etmx_lryaw;

// FILTER MODULE
sus_etmx_ulout = filterModuleD(dspPtr,dspCoeff,SUS_ETMX_ULOUT,sus_etmx_sum,0);

// FILTER MODULE
sus_etmx_llout = filterModuleD(dspPtr,dspCoeff,SUS_ETMX_LLOUT,sus_etmx_sum1,0);

// FILTER MODULE
sus_etmx_urout = filterModuleD(dspPtr,dspCoeff,SUS_ETMX_UROUT,sus_etmx_sum2,0);

// FILTER MODULE
sus_etmx_lrout = filterModuleD(dspPtr,dspCoeff,SUS_ETMX_LROUT,sus_etmx_sum3,0);


//End of subsystem **************************************************



//Start of subsystem **************************************************

// FILTER MODULE
sus_etmy_ulsen = filterModuleD(dspPtr,dspCoeff,SUS_ETMY_ULSEN,dWord[0][5],0);

// FILTER MODULE
sus_etmy_llsen = filterModuleD(dspPtr,dspCoeff,SUS_ETMY_LLSEN,dWord[0][6],0);

// FILTER MODULE
sus_etmy_ursen = filterModuleD(dspPtr,dspCoeff,SUS_ETMY_URSEN,dWord[0][7],0);

// FILTER MODULE
sus_etmy_lrsen = filterModuleD(dspPtr,dspCoeff,SUS_ETMY_LRSEN,dWord[0][8],0);

// FILTER MODULE
sus_etmy_side = filterModuleD(dspPtr,dspCoeff,SUS_ETMY_SIDE,dWord[0][9],0);

// FILTER MODULE
sus_etmy_lsc = filterModuleD(dspPtr,dspCoeff,SUS_ETMY_LSC,lsc_outmtrx[1][1],0);

// FILTER MODULE
sus_etmy_ascp = filterModuleD(dspPtr,dspCoeff,SUS_ETMY_ASCP,sus_etmy_ground1,0);

// FILTER MODULE
sus_etmy_ascy = filterModuleD(dspPtr,dspCoeff,SUS_ETMY_ASCY,sus_etmy_ground2,0);

// MATRIX CALC
for(ii=0;ii<3;ii++)
{
sus_etmy_inmtrx[1][ii] = 
	pLocalEpics->pde.SUS_ETMY_INMTRX[ii][0] * sus_etmy_ulsen +
	pLocalEpics->pde.SUS_ETMY_INMTRX[ii][1] * sus_etmy_llsen +
	pLocalEpics->pde.SUS_ETMY_INMTRX[ii][2] * sus_etmy_ursen +
	pLocalEpics->pde.SUS_ETMY_INMTRX[ii][3] * sus_etmy_lrsen;
}

// FILTER MODULE
sus_etmy_pos = filterModuleD(dspPtr,dspCoeff,SUS_ETMY_POS,sus_etmy_inmtrx[1][0],0);

// FILTER MODULE
sus_etmy_pit = filterModuleD(dspPtr,dspCoeff,SUS_ETMY_PIT,sus_etmy_inmtrx[1][1],0);

// FILTER MODULE
sus_etmy_yaw = filterModuleD(dspPtr,dspCoeff,SUS_ETMY_YAW,sus_etmy_inmtrx[1][2],0);

// SUM
sus_etmy_sum4 = sus_etmy_lsc + sus_etmy_pos;

// SUM
sus_etmy_sum5 = sus_etmy_pit + sus_etmy_ascp;

// SUM
sus_etmy_sum6 = sus_etmy_yaw + sus_etmy_ascy;

// FILTER MODULE
sus_etmy_ulpos = filterModuleD(dspPtr,dspCoeff,SUS_ETMY_ULPOS,sus_etmy_sum4,0);

// FILTER MODULE
sus_etmy_llpos = filterModuleD(dspPtr,dspCoeff,SUS_ETMY_LLPOS,sus_etmy_sum4,0);

// FILTER MODULE
sus_etmy_urpos = filterModuleD(dspPtr,dspCoeff,SUS_ETMY_URPOS,sus_etmy_sum4,0);

// FILTER MODULE
sus_etmy_lrpos = filterModuleD(dspPtr,dspCoeff,SUS_ETMY_LRPOS,sus_etmy_sum4,0);

// FILTER MODULE
sus_etmy_ulpit = filterModuleD(dspPtr,dspCoeff,SUS_ETMY_ULPIT,sus_etmy_sum5,0);

// FILTER MODULE
sus_etmy_llpit = filterModuleD(dspPtr,dspCoeff,SUS_ETMY_LLPIT,sus_etmy_sum5,0);

// FILTER MODULE
sus_etmy_urpit = filterModuleD(dspPtr,dspCoeff,SUS_ETMY_URPIT,sus_etmy_sum5,0);

// FILTER MODULE
sus_etmy_lrpit = filterModuleD(dspPtr,dspCoeff,SUS_ETMY_LRPIT,sus_etmy_sum5,0);

// FILTER MODULE
sus_etmy_ulyaw = filterModuleD(dspPtr,dspCoeff,SUS_ETMY_ULYAW,sus_etmy_sum6,0);

// FILTER MODULE
sus_etmy_llyaw = filterModuleD(dspPtr,dspCoeff,SUS_ETMY_LLYAW,sus_etmy_sum6,0);

// FILTER MODULE
sus_etmy_uryaw = filterModuleD(dspPtr,dspCoeff,SUS_ETMY_URYAW,sus_etmy_sum6,0);

// FILTER MODULE
sus_etmy_lryaw = filterModuleD(dspPtr,dspCoeff,SUS_ETMY_LRYAW,sus_etmy_sum6,0);

// SUM
sus_etmy_sum = sus_etmy_ulpos + sus_etmy_ulpit + sus_etmy_ulyaw;

// SUM
sus_etmy_sum1 = sus_etmy_llpos + sus_etmy_llpit + sus_etmy_llyaw;

// SUM
sus_etmy_sum2 = sus_etmy_urpos + sus_etmy_urpit + sus_etmy_uryaw;

// SUM
sus_etmy_sum3 = sus_etmy_lrpos + sus_etmy_lrpit + sus_etmy_lryaw;

// FILTER MODULE
sus_etmy_ulout = filterModuleD(dspPtr,dspCoeff,SUS_ETMY_ULOUT,sus_etmy_sum,0);

// FILTER MODULE
sus_etmy_llout = filterModuleD(dspPtr,dspCoeff,SUS_ETMY_LLOUT,sus_etmy_sum1,0);

// FILTER MODULE
sus_etmy_urout = filterModuleD(dspPtr,dspCoeff,SUS_ETMY_UROUT,sus_etmy_sum2,0);

// FILTER MODULE
sus_etmy_lrout = filterModuleD(dspPtr,dspCoeff,SUS_ETMY_LROUT,sus_etmy_sum3,0);


//End of subsystem **************************************************
#endif



#if 0
//Start of subsystem **************************************************

// FILTER MODULE
sus_itmx_ulsen = filterModuleD(dspPtr,dspCoeff,SUS_ITMX_ULSEN,dWord[0][10],0);

// FILTER MODULE
sus_itmx_llsen = filterModuleD(dspPtr,dspCoeff,SUS_ITMX_LLSEN,dWord[0][11],0);

// FILTER MODULE
sus_itmx_ursen = filterModuleD(dspPtr,dspCoeff,SUS_ITMX_URSEN,dWord[0][12],0);

// FILTER MODULE
sus_itmx_lrsen = filterModuleD(dspPtr,dspCoeff,SUS_ITMX_LRSEN,dWord[0][13],0);

// FILTER MODULE
sus_itmx_side = filterModuleD(dspPtr,dspCoeff,SUS_ITMX_SIDE,dWord[0][14],0);

// FILTER MODULE
sus_itmx_lsc = filterModuleD(dspPtr,dspCoeff,SUS_ITMX_LSC,lsc_outmtrx[1][2],0);

// FILTER MODULE
sus_itmx_ascp = filterModuleD(dspPtr,dspCoeff,SUS_ITMX_ASCP,asc_outmtrx[1][0],0);

// FILTER MODULE
sus_itmx_ascy = filterModuleD(dspPtr,dspCoeff,SUS_ITMX_ASCY,asc_outmtrx[1][1],0);

// MATRIX CALC
for(ii=0;ii<3;ii++)
{
sus_itmx_inmtrx[1][ii] = 
	pLocalEpics->pde.SUS_ITMX_INMTRX[ii][0] * sus_itmx_ulsen +
	pLocalEpics->pde.SUS_ITMX_INMTRX[ii][1] * sus_itmx_llsen +
	pLocalEpics->pde.SUS_ITMX_INMTRX[ii][2] * sus_itmx_ursen +
	pLocalEpics->pde.SUS_ITMX_INMTRX[ii][3] * sus_itmx_lrsen;
}

// FILTER MODULE
sus_itmx_pos = filterModuleD(dspPtr,dspCoeff,SUS_ITMX_POS,sus_itmx_inmtrx[1][0],0);

// FILTER MODULE
sus_itmx_pit = filterModuleD(dspPtr,dspCoeff,SUS_ITMX_PIT,sus_itmx_inmtrx[1][1],0);

// FILTER MODULE
sus_itmx_yaw = filterModuleD(dspPtr,dspCoeff,SUS_ITMX_YAW,sus_itmx_inmtrx[1][2],0);

// SUM
sus_itmx_sum4 = sus_itmx_lsc + sus_itmx_pos;

// SUM
sus_itmx_sum5 = sus_itmx_pit + sus_itmx_ascp;

// SUM
sus_itmx_sum6 = sus_itmx_yaw + sus_itmx_ascy;

// FILTER MODULE
sus_itmx_ulpos = filterModuleD(dspPtr,dspCoeff,SUS_ITMX_ULPOS,sus_itmx_sum4,0);

// FILTER MODULE
sus_itmx_llpos = filterModuleD(dspPtr,dspCoeff,SUS_ITMX_LLPOS,sus_itmx_sum4,0);

// FILTER MODULE
sus_itmx_urpos = filterModuleD(dspPtr,dspCoeff,SUS_ITMX_URPOS,sus_itmx_sum4,0);

// FILTER MODULE
sus_itmx_lrpos = filterModuleD(dspPtr,dspCoeff,SUS_ITMX_LRPOS,sus_itmx_sum4,0);

// FILTER MODULE
sus_itmx_ulpit = filterModuleD(dspPtr,dspCoeff,SUS_ITMX_ULPIT,sus_itmx_sum5,0);

// FILTER MODULE
sus_itmx_llpit = filterModuleD(dspPtr,dspCoeff,SUS_ITMX_LLPIT,sus_itmx_sum5,0);

// FILTER MODULE
sus_itmx_urpit = filterModuleD(dspPtr,dspCoeff,SUS_ITMX_URPIT,sus_itmx_sum5,0);

// FILTER MODULE
sus_itmx_lrpit = filterModuleD(dspPtr,dspCoeff,SUS_ITMX_LRPIT,sus_itmx_sum5,0);

// FILTER MODULE
sus_itmx_ulyaw = filterModuleD(dspPtr,dspCoeff,SUS_ITMX_ULYAW,sus_itmx_sum6,0);

// FILTER MODULE
sus_itmx_llyaw = filterModuleD(dspPtr,dspCoeff,SUS_ITMX_LLYAW,sus_itmx_sum6,0);

// FILTER MODULE
sus_itmx_uryaw = filterModuleD(dspPtr,dspCoeff,SUS_ITMX_URYAW,sus_itmx_sum6,0);

// FILTER MODULE
sus_itmx_lryaw = filterModuleD(dspPtr,dspCoeff,SUS_ITMX_LRYAW,sus_itmx_sum6,0);

// SUM
sus_itmx_sum = sus_itmx_ulpos + sus_itmx_ulpit + sus_itmx_ulyaw;

// SUM
sus_itmx_sum1 = sus_itmx_llpos + sus_itmx_llpit + sus_itmx_llyaw;

// SUM
sus_itmx_sum2 = sus_itmx_urpos + sus_itmx_urpit + sus_itmx_uryaw;

// SUM
sus_itmx_sum3 = sus_itmx_lrpos + sus_itmx_lrpit + sus_itmx_lryaw;

// FILTER MODULE
sus_itmx_ulout = filterModuleD(dspPtr,dspCoeff,SUS_ITMX_ULOUT,sus_itmx_sum,0);

// FILTER MODULE
sus_itmx_llout = filterModuleD(dspPtr,dspCoeff,SUS_ITMX_LLOUT,sus_itmx_sum1,0);

// FILTER MODULE
sus_itmx_urout = filterModuleD(dspPtr,dspCoeff,SUS_ITMX_UROUT,sus_itmx_sum2,0);

// FILTER MODULE
sus_itmx_lrout = filterModuleD(dspPtr,dspCoeff,SUS_ITMX_LROUT,sus_itmx_sum3,0);


//End of subsystem **************************************************



//Start of subsystem **************************************************

// FILTER MODULE
sus_itmy_ulsen = filterModuleD(dspPtr,dspCoeff,SUS_ITMY_ULSEN,dWord[0][15],0);

// FILTER MODULE
sus_itmy_llsen = filterModuleD(dspPtr,dspCoeff,SUS_ITMY_LLSEN,dWord[0][16],0);

// FILTER MODULE
sus_itmy_ursen = filterModuleD(dspPtr,dspCoeff,SUS_ITMY_URSEN,dWord[0][17],0);

// FILTER MODULE
sus_itmy_lrsen = filterModuleD(dspPtr,dspCoeff,SUS_ITMY_LRSEN,dWord[0][18],0);

// FILTER MODULE
sus_itmy_side = filterModuleD(dspPtr,dspCoeff,SUS_ITMY_SIDE,dWord[0][19],0);

// FILTER MODULE
sus_itmy_lsc = filterModuleD(dspPtr,dspCoeff,SUS_ITMY_LSC,lsc_outmtrx[1][3],0);

// FILTER MODULE
sus_itmy_ascp = filterModuleD(dspPtr,dspCoeff,SUS_ITMY_ASCP,asc_outmtrx[1][2],0);

// FILTER MODULE
sus_itmy_ascy = filterModuleD(dspPtr,dspCoeff,SUS_ITMY_ASCY,asc_outmtrx[1][3],0);

// MATRIX CALC
for(ii=0;ii<3;ii++)
{
sus_itmy_inmtrx[1][ii] = 
	pLocalEpics->pde.SUS_ITMY_INMTRX[ii][0] * sus_itmy_ulsen +
	pLocalEpics->pde.SUS_ITMY_INMTRX[ii][1] * sus_itmy_llsen +
	pLocalEpics->pde.SUS_ITMY_INMTRX[ii][2] * sus_itmy_ursen +
	pLocalEpics->pde.SUS_ITMY_INMTRX[ii][3] * sus_itmy_lrsen;
}

// FILTER MODULE
sus_itmy_pos = filterModuleD(dspPtr,dspCoeff,SUS_ITMY_POS,sus_itmy_inmtrx[1][0],0);

// FILTER MODULE
sus_itmy_pit = filterModuleD(dspPtr,dspCoeff,SUS_ITMY_PIT,sus_itmy_inmtrx[1][1],0);

// FILTER MODULE
sus_itmy_yaw = filterModuleD(dspPtr,dspCoeff,SUS_ITMY_YAW,sus_itmy_inmtrx[1][2],0);

// SUM
sus_itmy_sum4 = sus_itmy_lsc + sus_itmy_pos;

// SUM
sus_itmy_sum5 = sus_itmy_pit + sus_itmy_ascp;

// SUM
sus_itmy_sum6 = sus_itmy_yaw + sus_itmy_ascy;

// FILTER MODULE
sus_itmy_ulpos = filterModuleD(dspPtr,dspCoeff,SUS_ITMY_ULPOS,sus_itmy_sum4,0);

// FILTER MODULE
sus_itmy_llpos = filterModuleD(dspPtr,dspCoeff,SUS_ITMY_LLPOS,sus_itmy_sum4,0);

// FILTER MODULE
sus_itmy_urpos = filterModuleD(dspPtr,dspCoeff,SUS_ITMY_URPOS,sus_itmy_sum4,0);

// FILTER MODULE
sus_itmy_lrpos = filterModuleD(dspPtr,dspCoeff,SUS_ITMY_LRPOS,sus_itmy_sum4,0);

// FILTER MODULE
sus_itmy_ulpit = filterModuleD(dspPtr,dspCoeff,SUS_ITMY_ULPIT,sus_itmy_sum5,0);

// FILTER MODULE
sus_itmy_llpit = filterModuleD(dspPtr,dspCoeff,SUS_ITMY_LLPIT,sus_itmy_sum5,0);

// FILTER MODULE
sus_itmy_urpit = filterModuleD(dspPtr,dspCoeff,SUS_ITMY_URPIT,sus_itmy_sum5,0);

// FILTER MODULE
sus_itmy_lrpit = filterModuleD(dspPtr,dspCoeff,SUS_ITMY_LRPIT,sus_itmy_sum5,0);

// FILTER MODULE
sus_itmy_ulyaw = filterModuleD(dspPtr,dspCoeff,SUS_ITMY_ULYAW,sus_itmy_sum6,0);

// FILTER MODULE
sus_itmy_llyaw = filterModuleD(dspPtr,dspCoeff,SUS_ITMY_LLYAW,sus_itmy_sum6,0);

// FILTER MODULE
sus_itmy_uryaw = filterModuleD(dspPtr,dspCoeff,SUS_ITMY_URYAW,sus_itmy_sum6,0);

// FILTER MODULE
sus_itmy_lryaw = filterModuleD(dspPtr,dspCoeff,SUS_ITMY_LRYAW,sus_itmy_sum6,0);

// SUM
sus_itmy_sum = sus_itmy_ulpos + sus_itmy_ulpit + sus_itmy_ulyaw;

// SUM
sus_itmy_sum1 = sus_itmy_llpos + sus_itmy_llpit + sus_itmy_llyaw;

// SUM
sus_itmy_sum2 = sus_itmy_urpos + sus_itmy_urpit + sus_itmy_uryaw;

// SUM
sus_itmy_sum3 = sus_itmy_lrpos + sus_itmy_lrpit + sus_itmy_lryaw;

// FILTER MODULE
sus_itmy_ulout = filterModuleD(dspPtr,dspCoeff,SUS_ITMY_ULOUT,sus_itmy_sum,0);

// FILTER MODULE
sus_itmy_llout = filterModuleD(dspPtr,dspCoeff,SUS_ITMY_LLOUT,sus_itmy_sum1,0);

// FILTER MODULE
sus_itmy_urout = filterModuleD(dspPtr,dspCoeff,SUS_ITMY_UROUT,sus_itmy_sum2,0);

// FILTER MODULE
sus_itmy_lrout = filterModuleD(dspPtr,dspCoeff,SUS_ITMY_LROUT,sus_itmy_sum3,0);
#endif

while(!done && !stop_working_threads);
done = 0;
#ifdef RESERVE_CPU3
while(!done1 && !stop_working_threads);
done1 = 0;
#endif

// DAC number is 0
dacOut[0][0] = sus_etmx_ulout;
dacOut[0][1] = sus_etmx_llout;
dacOut[0][2] = sus_etmx_urout;
dacOut[0][3] = sus_etmx_lrout;
dacOut[0][4] = sus_etmx_side;
dacOut[0][5] = sus_etmy_ulout;
dacOut[0][6] = sus_etmy_llout;
dacOut[0][7] = sus_etmy_urout;
dacOut[0][8] = sus_etmy_lrout;
dacOut[0][9] = sus_etmy_side;
dacOut[0][10] = sus_itmx_ulout;
dacOut[0][11] = sus_itmx_llout;
dacOut[0][12] = sus_itmx_urout;
dacOut[0][13] = sus_itmx_lrout;
dacOut[0][14] = sus_itmx_side;

// DAC number is 1
dacOut[1][0] = sus_itmy_ulout;
dacOut[1][1] = sus_itmy_llout;
dacOut[1][2] = sus_itmy_urout;
dacOut[1][3] = sus_itmy_lrout;
dacOut[1][4] = sus_itmy_side;
dacOut[1][5] = sus_bs_ulout;
dacOut[1][6] = sus_bs_llout;
dacOut[1][7] = sus_bs_urout;
dacOut[1][8] = sus_bs_lrout;
dacOut[1][9] = sus_bs_side;
dacOut[1][10] = lsc_outmtrx[1][5];
dacOut[1][11] = lsc_lock_dc;
dacOut[1][12] = pLocalEpics->pde.PZT_M1_PIT;
dacOut[1][13] = pLocalEpics->pde.PZT_M1_YAW;
dacOut[1][14] = pLocalEpics->pde.PZT_M2_PIT;
dacOut[1][15] = pLocalEpics->pde.PZT_M2_YAW;

  }
}


