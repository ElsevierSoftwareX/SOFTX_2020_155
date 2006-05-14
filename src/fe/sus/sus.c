// ******* This is a computer generated file *******
// ******* DO NOT HAND EDIT ************************


void feCode(int cycle, double dWord[][32],	/* ADC inputs */
		int dacOut[][16],	/* DAC outputs */
		FILT_MOD *dspPtr,	/* Filter Mod variables */
		COEF *dspCoeff,		/* Filter Mod coeffs */
		CDS_EPICS *pLocalEpics,	/* EPICS variables */
		int feInit)	/* Initialization flag */
{

int ii,jj;

double q1_esd_ascp;
double q1_esd_ascy;
double q1_esd_bias;
double q1_esd_inmtrx[5][3];
double q1_esd_lsc;
double q1_esd_q1;
double q1_esd_q2;
double q1_esd_q3;
double q1_esd_q4;
double q1_l1_ascp;
double q1_l1_ascy;
static float q1_l1_ground;
static float q1_l1_ground1;
static float q1_l1_ground2;
double q1_l1_inmtrx[3][4];
double q1_l1_llout;
double q1_l1_llpit;
double q1_l1_llpos;
double q1_l1_llsen;
double q1_l1_llyaw;
double q1_l1_lrout;
double q1_l1_lrpit;
double q1_l1_lrpos;
double q1_l1_lrsen;
double q1_l1_lryaw;
double q1_l1_lsc;
double q1_l1_pit;
double q1_l1_pos;
double q1_l1_sum;
double q1_l1_sum1;
double q1_l1_sum2;
double q1_l1_sum3;
double q1_l1_sum4;
double q1_l1_sum5;
double q1_l1_sum6;
double q1_l1_ulout;
double q1_l1_ulpit;
double q1_l1_ulpos;
double q1_l1_ulsen;
double q1_l1_ulyaw;
double q1_l1_urout;
double q1_l1_urpit;
double q1_l1_urpos;
double q1_l1_ursen;
double q1_l1_uryaw;
double q1_l1_yaw;
double q1_l2_ascp;
double q1_l2_ascy;
static float q1_l2_ground;
static float q1_l2_ground1;
static float q1_l2_ground2;
double q1_l2_inmtrx[3][4];
double q1_l2_llout;
double q1_l2_llpit;
double q1_l2_llpos;
double q1_l2_llsen;
double q1_l2_llyaw;
double q1_l2_lrout;
double q1_l2_lrpit;
double q1_l2_lrpos;
double q1_l2_lrsen;
double q1_l2_lryaw;
double q1_l2_lsc;
double q1_l2_pit;
double q1_l2_pos;
double q1_l2_sum;
double q1_l2_sum1;
double q1_l2_sum2;
double q1_l2_sum3;
double q1_l2_sum4;
double q1_l2_sum5;
double q1_l2_sum6;
double q1_l2_ulout;
double q1_l2_ulpit;
double q1_l2_ulpos;
double q1_l2_ulsen;
double q1_l2_ulyaw;
double q1_l2_urout;
double q1_l2_urpit;
double q1_l2_urpos;
double q1_l2_ursen;
double q1_l2_uryaw;
double q1_l2_yaw;
double q1_m0_dof1;
double q1_m0_dof2;
double q1_m0_dof3;
double q1_m0_dof4;
double q1_m0_dof5;
double q1_m0_dof6;
double q1_m0_f1_act;
double q1_m0_f2_act;
double q1_m0_f3_act;
double q1_m0_face1;
double q1_m0_face2;
double q1_m0_face3;
double q1_m0_inmtrx[6][6];
double q1_m0_left;
double q1_m0_l_act;
double q1_m0_outmtrx[6][6];
double q1_m0_right;
double q1_m0_r_act;
double q1_m0_side;
double q1_m0_s_act;
double q1_r0_dof1;
double q1_r0_dof2;
double q1_r0_dof3;
double q1_r0_dof4;
double q1_r0_dof5;
double q1_r0_dof6;
double q1_r0_f1_act;
double q1_r0_f2_act;
double q1_r0_f3_act;
double q1_r0_face1;
double q1_r0_face2;
double q1_r0_face3;
double q1_r0_inmtrx[6][6];
double q1_r0_left;
double q1_r0_l_act;
double q1_r0_outmtrx[6][6];
double q1_r0_right;
double q1_r0_r_act;
double q1_r0_side;
double q1_r0_s_act;
float q1_wd;
static float q1_wd_avg[20];
static float q1_wd_var[20];
float vabs;
double q1_wd_sw[25];


if(feInit)
{
q1_l1_ground = 0.0;
q1_l1_ground1 = 0.0;
q1_l1_ground2 = 0.0;
q1_l2_ground = 0.0;
q1_l2_ground1 = 0.0;
q1_l2_ground2 = 0.0;
for(ii=0;ii<20;ii++) {
	q1_wd_avg[ii] = 0.0;
	q1_wd_var[ii] = 0.0;
}
} else {

//Start of subsystem **************************************************

// FILTER MODULE
q1_esd_lsc = filterModuleD(dspPtr,dspCoeff,Q1_ESD_LSC,dWord[0][24],0);

// FILTER MODULE
q1_esd_ascp = filterModuleD(dspPtr,dspCoeff,Q1_ESD_ASCP,dWord[0][25],0);

// FILTER MODULE
q1_esd_ascy = filterModuleD(dspPtr,dspCoeff,Q1_ESD_ASCY,dWord[0][26],0);

// MATRIX CALC
for(ii=0;ii<5;ii++)
{
q1_esd_inmtrx[1][ii] = 
	pLocalEpics->sus.Q1_ESD_INMTRX[ii][0] * q1_esd_lsc +
	pLocalEpics->sus.Q1_ESD_INMTRX[ii][1] * q1_esd_ascp +
	pLocalEpics->sus.Q1_ESD_INMTRX[ii][2] * q1_esd_ascy;
}

// FILTER MODULE
q1_esd_bias = filterModuleD(dspPtr,dspCoeff,Q1_ESD_BIAS,q1_esd_inmtrx[1][0],0);

// FILTER MODULE
q1_esd_q1 = filterModuleD(dspPtr,dspCoeff,Q1_ESD_Q1,q1_esd_inmtrx[1][1],0);

// FILTER MODULE
q1_esd_q2 = filterModuleD(dspPtr,dspCoeff,Q1_ESD_Q2,q1_esd_inmtrx[1][2],0);

// FILTER MODULE
q1_esd_q3 = filterModuleD(dspPtr,dspCoeff,Q1_ESD_Q3,q1_esd_inmtrx[1][3],0);

// FILTER MODULE
q1_esd_q4 = filterModuleD(dspPtr,dspCoeff,Q1_ESD_Q4,q1_esd_inmtrx[1][4],0);


//End of subsystem **************************************************



//Start of subsystem **************************************************

// FILTER MODULE
q1_l1_ulsen = filterModuleD(dspPtr,dspCoeff,Q1_L1_ULSEN,dWord[0][12],0);

// FILTER MODULE
q1_l1_llsen = filterModuleD(dspPtr,dspCoeff,Q1_L1_LLSEN,dWord[0][13],0);

// FILTER MODULE
q1_l1_ursen = filterModuleD(dspPtr,dspCoeff,Q1_L1_URSEN,dWord[0][14],0);

// FILTER MODULE
q1_l1_lrsen = filterModuleD(dspPtr,dspCoeff,Q1_L1_LRSEN,dWord[0][15],0);

// FILTER MODULE
q1_l1_lsc = filterModuleD(dspPtr,dspCoeff,Q1_L1_LSC,q1_l1_ground,0);

// FILTER MODULE
q1_l1_ascp = filterModuleD(dspPtr,dspCoeff,Q1_L1_ASCP,q1_l1_ground1,0);

// FILTER MODULE
q1_l1_ascy = filterModuleD(dspPtr,dspCoeff,Q1_L1_ASCY,q1_l1_ground2,0);

// MATRIX CALC
for(ii=0;ii<3;ii++)
{
q1_l1_inmtrx[1][ii] = 
	pLocalEpics->sus.Q1_L1_INMTRX[ii][0] * q1_l1_ulsen +
	pLocalEpics->sus.Q1_L1_INMTRX[ii][1] * q1_l1_llsen +
	pLocalEpics->sus.Q1_L1_INMTRX[ii][2] * q1_l1_ursen +
	pLocalEpics->sus.Q1_L1_INMTRX[ii][3] * q1_l1_lrsen;
}

// FILTER MODULE
q1_l1_pos = filterModuleD(dspPtr,dspCoeff,Q1_L1_POS,q1_l1_inmtrx[1][0],0);

// FILTER MODULE
q1_l1_pit = filterModuleD(dspPtr,dspCoeff,Q1_L1_PIT,q1_l1_inmtrx[1][1],0);

// FILTER MODULE
q1_l1_yaw = filterModuleD(dspPtr,dspCoeff,Q1_L1_YAW,q1_l1_inmtrx[1][2],0);

// SUM
q1_l1_sum4 = q1_l1_lsc + q1_l1_pos;

// SUM
q1_l1_sum5 = q1_l1_pit + q1_l1_ascp;

// SUM
q1_l1_sum6 = q1_l1_yaw + q1_l1_ascy;

// FILTER MODULE
q1_l1_ulpos = filterModuleD(dspPtr,dspCoeff,Q1_L1_ULPOS,q1_l1_sum4,0);

// FILTER MODULE
q1_l1_llpos = filterModuleD(dspPtr,dspCoeff,Q1_L1_LLPOS,q1_l1_sum4,0);

// FILTER MODULE
q1_l1_urpos = filterModuleD(dspPtr,dspCoeff,Q1_L1_URPOS,q1_l1_sum4,0);

// FILTER MODULE
q1_l1_lrpos = filterModuleD(dspPtr,dspCoeff,Q1_L1_LRPOS,q1_l1_sum4,0);

// FILTER MODULE
q1_l1_ulpit = filterModuleD(dspPtr,dspCoeff,Q1_L1_ULPIT,q1_l1_sum5,0);

// FILTER MODULE
q1_l1_llpit = filterModuleD(dspPtr,dspCoeff,Q1_L1_LLPIT,q1_l1_sum5,0);

// FILTER MODULE
q1_l1_urpit = filterModuleD(dspPtr,dspCoeff,Q1_L1_URPIT,q1_l1_sum5,0);

// FILTER MODULE
q1_l1_lrpit = filterModuleD(dspPtr,dspCoeff,Q1_L1_LRPIT,q1_l1_sum5,0);

// FILTER MODULE
q1_l1_ulyaw = filterModuleD(dspPtr,dspCoeff,Q1_L1_ULYAW,q1_l1_sum6,0);

// FILTER MODULE
q1_l1_llyaw = filterModuleD(dspPtr,dspCoeff,Q1_L1_LLYAW,q1_l1_sum6,0);

// FILTER MODULE
q1_l1_uryaw = filterModuleD(dspPtr,dspCoeff,Q1_L1_URYAW,q1_l1_sum6,0);

// FILTER MODULE
q1_l1_lryaw = filterModuleD(dspPtr,dspCoeff,Q1_L1_LRYAW,q1_l1_sum6,0);

// SUM
q1_l1_sum = q1_l1_ulpos + q1_l1_ulpit + q1_l1_ulyaw;

// SUM
q1_l1_sum1 = q1_l1_llpos + q1_l1_llpit + q1_l1_llyaw;

// SUM
q1_l1_sum2 = q1_l1_urpos + q1_l1_urpit + q1_l1_uryaw;

// SUM
q1_l1_sum3 = q1_l1_lrpos + q1_l1_lrpit + q1_l1_lryaw;

// FILTER MODULE
q1_l1_ulout = filterModuleD(dspPtr,dspCoeff,Q1_L1_ULOUT,q1_l1_sum,0);

// FILTER MODULE
q1_l1_llout = filterModuleD(dspPtr,dspCoeff,Q1_L1_LLOUT,q1_l1_sum1,0);

// FILTER MODULE
q1_l1_urout = filterModuleD(dspPtr,dspCoeff,Q1_L1_UROUT,q1_l1_sum2,0);

// FILTER MODULE
q1_l1_lrout = filterModuleD(dspPtr,dspCoeff,Q1_L1_LROUT,q1_l1_sum3,0);


//End of subsystem **************************************************



//Start of subsystem **************************************************

// FILTER MODULE
q1_l2_ulsen = filterModuleD(dspPtr,dspCoeff,Q1_L2_ULSEN,dWord[0][18],0);

// FILTER MODULE
q1_l2_llsen = filterModuleD(dspPtr,dspCoeff,Q1_L2_LLSEN,dWord[0][19],0);

// FILTER MODULE
q1_l2_ursen = filterModuleD(dspPtr,dspCoeff,Q1_L2_URSEN,dWord[0][20],0);

// FILTER MODULE
q1_l2_lrsen = filterModuleD(dspPtr,dspCoeff,Q1_L2_LRSEN,dWord[0][21],0);

// FILTER MODULE
q1_l2_lsc = filterModuleD(dspPtr,dspCoeff,Q1_L2_LSC,q1_l2_ground,0);

// FILTER MODULE
q1_l2_ascp = filterModuleD(dspPtr,dspCoeff,Q1_L2_ASCP,q1_l2_ground1,0);

// FILTER MODULE
q1_l2_ascy = filterModuleD(dspPtr,dspCoeff,Q1_L2_ASCY,q1_l2_ground2,0);

// MATRIX CALC
for(ii=0;ii<3;ii++)
{
q1_l2_inmtrx[1][ii] = 
	pLocalEpics->sus.Q1_L2_INMTRX[ii][0] * q1_l2_ulsen +
	pLocalEpics->sus.Q1_L2_INMTRX[ii][1] * q1_l2_llsen +
	pLocalEpics->sus.Q1_L2_INMTRX[ii][2] * q1_l2_ursen +
	pLocalEpics->sus.Q1_L2_INMTRX[ii][3] * q1_l2_lrsen;
}

// FILTER MODULE
q1_l2_pos = filterModuleD(dspPtr,dspCoeff,Q1_L2_POS,q1_l2_inmtrx[1][0],0);

// FILTER MODULE
q1_l2_pit = filterModuleD(dspPtr,dspCoeff,Q1_L2_PIT,q1_l2_inmtrx[1][1],0);

// FILTER MODULE
q1_l2_yaw = filterModuleD(dspPtr,dspCoeff,Q1_L2_YAW,q1_l2_inmtrx[1][2],0);

// SUM
q1_l2_sum4 = q1_l2_lsc + q1_l2_pos;

// SUM
q1_l2_sum5 = q1_l2_pit + q1_l2_ascp;

// SUM
q1_l2_sum6 = q1_l2_yaw + q1_l2_ascy;

// FILTER MODULE
q1_l2_ulpos = filterModuleD(dspPtr,dspCoeff,Q1_L2_ULPOS,q1_l2_sum4,0);

// FILTER MODULE
q1_l2_llpos = filterModuleD(dspPtr,dspCoeff,Q1_L2_LLPOS,q1_l2_sum4,0);

// FILTER MODULE
q1_l2_urpos = filterModuleD(dspPtr,dspCoeff,Q1_L2_URPOS,q1_l2_sum4,0);

// FILTER MODULE
q1_l2_lrpos = filterModuleD(dspPtr,dspCoeff,Q1_L2_LRPOS,q1_l2_sum4,0);

// FILTER MODULE
q1_l2_ulpit = filterModuleD(dspPtr,dspCoeff,Q1_L2_ULPIT,q1_l2_sum5,0);

// FILTER MODULE
q1_l2_llpit = filterModuleD(dspPtr,dspCoeff,Q1_L2_LLPIT,q1_l2_sum5,0);

// FILTER MODULE
q1_l2_urpit = filterModuleD(dspPtr,dspCoeff,Q1_L2_URPIT,q1_l2_sum5,0);

// FILTER MODULE
q1_l2_lrpit = filterModuleD(dspPtr,dspCoeff,Q1_L2_LRPIT,q1_l2_sum5,0);

// FILTER MODULE
q1_l2_ulyaw = filterModuleD(dspPtr,dspCoeff,Q1_L2_ULYAW,q1_l2_sum6,0);

// FILTER MODULE
q1_l2_llyaw = filterModuleD(dspPtr,dspCoeff,Q1_L2_LLYAW,q1_l2_sum6,0);

// FILTER MODULE
q1_l2_uryaw = filterModuleD(dspPtr,dspCoeff,Q1_L2_URYAW,q1_l2_sum6,0);

// FILTER MODULE
q1_l2_lryaw = filterModuleD(dspPtr,dspCoeff,Q1_L2_LRYAW,q1_l2_sum6,0);

// SUM
q1_l2_sum = q1_l2_ulpos + q1_l2_ulpit + q1_l2_ulyaw;

// SUM
q1_l2_sum1 = q1_l2_llpos + q1_l2_llpit + q1_l2_llyaw;

// SUM
q1_l2_sum2 = q1_l2_urpos + q1_l2_urpit + q1_l2_uryaw;

// SUM
q1_l2_sum3 = q1_l2_lrpos + q1_l2_lrpit + q1_l2_lryaw;

// FILTER MODULE
q1_l2_ulout = filterModuleD(dspPtr,dspCoeff,Q1_L2_ULOUT,q1_l2_sum,0);

// FILTER MODULE
q1_l2_llout = filterModuleD(dspPtr,dspCoeff,Q1_L2_LLOUT,q1_l2_sum1,0);

// FILTER MODULE
q1_l2_urout = filterModuleD(dspPtr,dspCoeff,Q1_L2_UROUT,q1_l2_sum2,0);

// FILTER MODULE
q1_l2_lrout = filterModuleD(dspPtr,dspCoeff,Q1_L2_LROUT,q1_l2_sum3,0);


//End of subsystem **************************************************



//Start of subsystem **************************************************

// FILTER MODULE
q1_m0_face1 = filterModuleD(dspPtr,dspCoeff,Q1_M0_FACE1,dWord[0][0],0);

// FILTER MODULE
q1_m0_face2 = filterModuleD(dspPtr,dspCoeff,Q1_M0_FACE2,dWord[0][1],0);

// FILTER MODULE
q1_m0_face3 = filterModuleD(dspPtr,dspCoeff,Q1_M0_FACE3,dWord[0][2],0);

// FILTER MODULE
q1_m0_left = filterModuleD(dspPtr,dspCoeff,Q1_M0_LEFT,dWord[0][3],0);

// FILTER MODULE
q1_m0_right = filterModuleD(dspPtr,dspCoeff,Q1_M0_RIGHT,dWord[0][4],0);

// FILTER MODULE
q1_m0_side = filterModuleD(dspPtr,dspCoeff,Q1_M0_SIDE,dWord[0][5],0);

// MATRIX CALC
for(ii=0;ii<6;ii++)
{
q1_m0_inmtrx[1][ii] = 
	pLocalEpics->sus.Q1_M0_INMTRX[ii][0] * q1_m0_face1 +
	pLocalEpics->sus.Q1_M0_INMTRX[ii][1] * q1_m0_face2 +
	pLocalEpics->sus.Q1_M0_INMTRX[ii][2] * q1_m0_face3 +
	pLocalEpics->sus.Q1_M0_INMTRX[ii][3] * q1_m0_left +
	pLocalEpics->sus.Q1_M0_INMTRX[ii][4] * q1_m0_right +
	pLocalEpics->sus.Q1_M0_INMTRX[ii][5] * q1_m0_side;
}

// FILTER MODULE
q1_m0_dof6 = filterModuleD(dspPtr,dspCoeff,Q1_M0_DOF6,q1_m0_inmtrx[1][5],0);

// FILTER MODULE
q1_m0_dof5 = filterModuleD(dspPtr,dspCoeff,Q1_M0_DOF5,q1_m0_inmtrx[1][4],0);

// FILTER MODULE
q1_m0_dof4 = filterModuleD(dspPtr,dspCoeff,Q1_M0_DOF4,q1_m0_inmtrx[1][3],0);

// FILTER MODULE
q1_m0_dof3 = filterModuleD(dspPtr,dspCoeff,Q1_M0_DOF3,q1_m0_inmtrx[1][2],0);

// FILTER MODULE
q1_m0_dof2 = filterModuleD(dspPtr,dspCoeff,Q1_M0_DOF2,q1_m0_inmtrx[1][1],0);

// FILTER MODULE
q1_m0_dof1 = filterModuleD(dspPtr,dspCoeff,Q1_M0_DOF1,q1_m0_inmtrx[1][0],0);

// MATRIX CALC
for(ii=0;ii<6;ii++)
{
q1_m0_outmtrx[1][ii] = 
	pLocalEpics->sus.Q1_M0_OUTMTRX[ii][0] * q1_m0_dof1 +
	pLocalEpics->sus.Q1_M0_OUTMTRX[ii][1] * q1_m0_dof2 +
	pLocalEpics->sus.Q1_M0_OUTMTRX[ii][2] * q1_m0_dof3 +
	pLocalEpics->sus.Q1_M0_OUTMTRX[ii][3] * q1_m0_dof4 +
	pLocalEpics->sus.Q1_M0_OUTMTRX[ii][4] * q1_m0_dof5 +
	pLocalEpics->sus.Q1_M0_OUTMTRX[ii][5] * q1_m0_dof6;
}

// FILTER MODULE
q1_m0_s_act = filterModuleD(dspPtr,dspCoeff,Q1_M0_S_ACT,q1_m0_outmtrx[1][5],0);

// FILTER MODULE
q1_m0_r_act = filterModuleD(dspPtr,dspCoeff,Q1_M0_R_ACT,q1_m0_outmtrx[1][4],0);

// FILTER MODULE
q1_m0_l_act = filterModuleD(dspPtr,dspCoeff,Q1_M0_L_ACT,q1_m0_outmtrx[1][3],0);

// FILTER MODULE
q1_m0_f3_act = filterModuleD(dspPtr,dspCoeff,Q1_M0_F3_ACT,q1_m0_outmtrx[1][2],0);

// FILTER MODULE
q1_m0_f2_act = filterModuleD(dspPtr,dspCoeff,Q1_M0_F2_ACT,q1_m0_outmtrx[1][1],0);

// FILTER MODULE
q1_m0_f1_act = filterModuleD(dspPtr,dspCoeff,Q1_M0_F1_ACT,q1_m0_outmtrx[1][0],0);


//End of subsystem **************************************************



//Start of subsystem **************************************************

// FILTER MODULE
q1_r0_face1 = filterModuleD(dspPtr,dspCoeff,Q1_R0_FACE1,dWord[0][6],0);

// FILTER MODULE
q1_r0_face2 = filterModuleD(dspPtr,dspCoeff,Q1_R0_FACE2,dWord[0][7],0);

// FILTER MODULE
q1_r0_face3 = filterModuleD(dspPtr,dspCoeff,Q1_R0_FACE3,dWord[0][8],0);

// FILTER MODULE
q1_r0_left = filterModuleD(dspPtr,dspCoeff,Q1_R0_LEFT,dWord[0][9],0);

// FILTER MODULE
q1_r0_right = filterModuleD(dspPtr,dspCoeff,Q1_R0_RIGHT,dWord[0][10],0);

// FILTER MODULE
q1_r0_side = filterModuleD(dspPtr,dspCoeff,Q1_R0_SIDE,dWord[0][11],0);

// MATRIX CALC
for(ii=0;ii<6;ii++)
{
q1_r0_inmtrx[1][ii] = 
	pLocalEpics->sus.Q1_R0_INMTRX[ii][0] * q1_r0_face1 +
	pLocalEpics->sus.Q1_R0_INMTRX[ii][1] * q1_r0_face2 +
	pLocalEpics->sus.Q1_R0_INMTRX[ii][2] * q1_r0_face3 +
	pLocalEpics->sus.Q1_R0_INMTRX[ii][3] * q1_r0_left +
	pLocalEpics->sus.Q1_R0_INMTRX[ii][4] * q1_r0_right +
	pLocalEpics->sus.Q1_R0_INMTRX[ii][5] * q1_r0_side;
}

// FILTER MODULE
q1_r0_dof6 = filterModuleD(dspPtr,dspCoeff,Q1_R0_DOF6,q1_r0_inmtrx[1][5],0);

// FILTER MODULE
q1_r0_dof5 = filterModuleD(dspPtr,dspCoeff,Q1_R0_DOF5,q1_r0_inmtrx[1][4],0);

// FILTER MODULE
q1_r0_dof4 = filterModuleD(dspPtr,dspCoeff,Q1_R0_DOF4,q1_r0_inmtrx[1][3],0);

// FILTER MODULE
q1_r0_dof3 = filterModuleD(dspPtr,dspCoeff,Q1_R0_DOF3,q1_r0_inmtrx[1][2],0);

// FILTER MODULE
q1_r0_dof2 = filterModuleD(dspPtr,dspCoeff,Q1_R0_DOF2,q1_r0_inmtrx[1][1],0);

// FILTER MODULE
q1_r0_dof1 = filterModuleD(dspPtr,dspCoeff,Q1_R0_DOF1,q1_r0_inmtrx[1][0],0);

// MATRIX CALC
for(ii=0;ii<6;ii++)
{
q1_r0_outmtrx[1][ii] = 
	pLocalEpics->sus.Q1_R0_OUTMTRX[ii][0] * q1_r0_dof1 +
	pLocalEpics->sus.Q1_R0_OUTMTRX[ii][1] * q1_r0_dof2 +
	pLocalEpics->sus.Q1_R0_OUTMTRX[ii][2] * q1_r0_dof3 +
	pLocalEpics->sus.Q1_R0_OUTMTRX[ii][3] * q1_r0_dof4 +
	pLocalEpics->sus.Q1_R0_OUTMTRX[ii][4] * q1_r0_dof5 +
	pLocalEpics->sus.Q1_R0_OUTMTRX[ii][5] * q1_r0_dof6;
}

// FILTER MODULE
q1_r0_s_act = filterModuleD(dspPtr,dspCoeff,Q1_R0_S_ACT,q1_r0_outmtrx[1][5],0);

// FILTER MODULE
q1_r0_r_act = filterModuleD(dspPtr,dspCoeff,Q1_R0_R_ACT,q1_r0_outmtrx[1][4],0);

// FILTER MODULE
q1_r0_l_act = filterModuleD(dspPtr,dspCoeff,Q1_R0_L_ACT,q1_r0_outmtrx[1][3],0);

// FILTER MODULE
q1_r0_f3_act = filterModuleD(dspPtr,dspCoeff,Q1_R0_F3_ACT,q1_r0_outmtrx[1][2],0);

// FILTER MODULE
q1_r0_f2_act = filterModuleD(dspPtr,dspCoeff,Q1_R0_F2_ACT,q1_r0_outmtrx[1][1],0);

// FILTER MODULE
q1_r0_f1_act = filterModuleD(dspPtr,dspCoeff,Q1_R0_F1_ACT,q1_r0_outmtrx[1][0],0);

// SUS_WD MODULE
if((cycle % 16) == 0) {
q1_wd = 16384;
   for(ii=0;ii<20;ii++) {
	if(ii<16) jj = ii;
	else jj = ii+2;
	q1_wd_avg[ii] = dWord[0][jj] * .00005 + q1_wd_avg[ii] * 0.99995;
	vabs = dWord[0][jj] - q1_wd_avg[ii];
	if(vabs < 0) vabs *= -1.0;
	q1_wd_var[ii] = vabs * .00005 + q1_wd_var[ii] * 0.99995;
	pLocalEpics->sus.Q1_WD_VAR[ii] = q1_wd_var[ii];
	if(q1_wd_var[ii] > pLocalEpics->sus.Q1_WD_MAX) q1_wd = 0;
   }
	pLocalEpics->sus.Q1_WD = q1_wd / 16384;
}

// MULTI_SW
q1_wd_sw[0] = q1_m0_f1_act;
q1_wd_sw[1] = q1_m0_f2_act;
q1_wd_sw[2] = q1_m0_f3_act;
q1_wd_sw[3] = q1_m0_l_act;
q1_wd_sw[4] = q1_m0_r_act;
q1_wd_sw[5] = q1_m0_s_act;
q1_wd_sw[6] = q1_r0_f1_act;
q1_wd_sw[7] = q1_r0_f2_act;
q1_wd_sw[8] = q1_r0_f3_act;
q1_wd_sw[9] = q1_r0_l_act;
q1_wd_sw[10] = q1_r0_r_act;
q1_wd_sw[11] = q1_r0_s_act;
q1_wd_sw[12] = q1_l1_ulout;
q1_wd_sw[13] = q1_l1_llout;
q1_wd_sw[14] = q1_l1_urout;
q1_wd_sw[15] = q1_l1_lrout;
q1_wd_sw[16] = q1_l2_ulout;
q1_wd_sw[17] = q1_l2_llout;
q1_wd_sw[18] = q1_l2_urout;
q1_wd_sw[19] = q1_l2_lrout;
q1_wd_sw[20] = q1_esd_bias;
q1_wd_sw[21] = q1_esd_q1;
q1_wd_sw[22] = q1_esd_q2;
q1_wd_sw[23] = q1_esd_q3;
q1_wd_sw[24] = q1_esd_q4;
if(pLocalEpics->sus.Q1_WD_SW == 0)
{
	for(ii=0;ii< 25;ii++) q1_wd_sw[ii] = 0.0;
}


// DAC number is 0
dacOut[0][0] = q1_wd_sw[0];
dacOut[0][1] = q1_wd_sw[1];
dacOut[0][2] = q1_wd_sw[2];
dacOut[0][3] = q1_wd_sw[3];
dacOut[0][4] = q1_wd_sw[4];
dacOut[0][5] = q1_wd_sw[5];
dacOut[0][6] = q1_wd_sw[6];
dacOut[0][7] = q1_wd_sw[7];
dacOut[0][8] = q1_wd_sw[8];
dacOut[0][9] = q1_wd_sw[9];
dacOut[0][10] = q1_wd_sw[10];
dacOut[0][11] = q1_wd_sw[11];
dacOut[0][12] = q1_wd_sw[12];
dacOut[0][13] = q1_wd_sw[13];
dacOut[0][14] = q1_wd_sw[14];
dacOut[0][15] = q1_wd_sw[15];

// DAC number is 1
dacOut[1][14] = q1_wd;
dacOut[1][0] = q1_wd_sw[16];
dacOut[1][1] = q1_wd_sw[17];
dacOut[1][2] = q1_wd_sw[18];
dacOut[1][3] = q1_wd_sw[19];
dacOut[1][6] = q1_wd_sw[20];
dacOut[1][7] = q1_wd_sw[21];
dacOut[1][8] = q1_wd_sw[22];
dacOut[1][9] = q1_wd_sw[23];
dacOut[1][10] = q1_wd_sw[24];

  }
}

