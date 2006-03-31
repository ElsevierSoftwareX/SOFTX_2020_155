// ******* This is a computer generated file *******
// ******* DO NOT HAND EDIT ************************


void feCode(double dWord[][32],	/* ADC inputs */
		int dacOut[][16],	/* DAC outputs */
		FILT_MOD *dspPtr,	/* Filter Mod variables */
		COEF *dspCoeff,		/* Filter Mod coeffs */
		CDS_EPICS *pLocalEpics)	/* EPICS variables */
{

int ii,jj;

double Q1_ESD_INMTRX[5][3];
double Q1_L1_INMTRX[3][4];
double Q1_L1_Sum;
double Q1_L1_Sum1;
double Q1_L1_Sum2;
double Q1_L1_Sum3;
double Q1_L1_Sum4;
double Q1_L1_Sum5;
double Q1_L1_Sum6;
double Q1_L2_INMTRX[3][4];
double Q1_L2_Sum;
double Q1_L2_Sum1;
double Q1_L2_Sum2;
double Q1_L2_Sum3;
double Q1_L2_Sum4;
double Q1_L2_Sum5;
double Q1_L2_Sum6;
double Q1_M0_INMTRX[6][6];
double Q1_M0_OUTMTRX[6][6];
double Q1_R0_INMTRX[6][6];
double Q1_R0_OUTMTRX[6][6];


Q1_L1_Sum = 0.0;
Q1_L1_Sum1 = 0.0;
Q1_L1_Sum2 = 0.0;
Q1_L1_Sum3 = 0.0;
Q1_L1_Sum4 = 0.0;
Q1_L1_Sum5 = 0.0;
Q1_L1_Sum6 = 0.0;
Q1_L2_Sum = 0.0;
Q1_L2_Sum1 = 0.0;
Q1_L2_Sum2 = 0.0;
Q1_L2_Sum3 = 0.0;
Q1_L2_Sum4 = 0.0;
Q1_L2_Sum5 = 0.0;
Q1_L2_Sum6 = 0.0;

//Start of subsystem **************************************************

Q1_ESD_INMTRX[0][0] = filterModuleD(dspPtr,dspCoeff,Q1_ESD_LSC,dWord[0][24],0); 

Q1_ESD_INMTRX[0][1] = filterModuleD(dspPtr,dspCoeff,Q1_ESD_ASCP,dWord[0][25],0); 

Q1_ESD_INMTRX[0][2] = filterModuleD(dspPtr,dspCoeff,Q1_ESD_ASCY,dWord[0][26],0); 


// Perform Matrix Calc **********************

for(ii=0;ii<5;ii++)
{
Q1_ESD_INMTRX[1][ii] = 
	pLocalEpics->sus.Q1_ESD_INMTRX[ii][0] * Q1_ESD_INMTRX[0][0] +
	pLocalEpics->sus.Q1_ESD_INMTRX[ii][1] * Q1_ESD_INMTRX[0][1] +
	pLocalEpics->sus.Q1_ESD_INMTRX[ii][2] * Q1_ESD_INMTRX[0][2];
}
// End Matrix Calc ***************************



dacOut[1][6] = filterModuleD(dspPtr,dspCoeff,Q1_ESD_BIAS,Q1_ESD_INMTRX[1][0],0);

dacOut[1][7] = filterModuleD(dspPtr,dspCoeff,Q1_ESD_Q1,Q1_ESD_INMTRX[1][1],0);

dacOut[1][8] = filterModuleD(dspPtr,dspCoeff,Q1_ESD_Q2,Q1_ESD_INMTRX[1][2],0);

dacOut[1][9] = filterModuleD(dspPtr,dspCoeff,Q1_ESD_Q3,Q1_ESD_INMTRX[1][3],0);

dacOut[1][10] = filterModuleD(dspPtr,dspCoeff,Q1_ESD_Q4,Q1_ESD_INMTRX[1][4],0);


//End of subsystem **************************************************



//Start of subsystem **************************************************

Q1_L1_INMTRX[0][0] = filterModuleD(dspPtr,dspCoeff,Q1_L1_ULSEN,dWord[0][12],0); 

Q1_L1_INMTRX[0][1] = filterModuleD(dspPtr,dspCoeff,Q1_L1_LLSEN,dWord[0][13],0); 

Q1_L1_INMTRX[0][2] = filterModuleD(dspPtr,dspCoeff,Q1_L1_URSEN,dWord[0][14],0); 

Q1_L1_INMTRX[0][3] = filterModuleD(dspPtr,dspCoeff,Q1_L1_LRSEN,dWord[0][15],0); 

Q1_L1_Sum4 += filterModuleD(dspPtr,dspCoeff,Q1_L1_LSC,0.0,0);

Q1_L1_Sum5 += filterModuleD(dspPtr,dspCoeff,Q1_L1_ASCP,0.0,0);

Q1_L1_Sum6 += filterModuleD(dspPtr,dspCoeff,Q1_L1_ASCY,0.0,0);


// Perform Matrix Calc **********************

for(ii=0;ii<3;ii++)
{
Q1_L1_INMTRX[1][ii] = 
	pLocalEpics->sus.Q1_L1_INMTRX[ii][0] * Q1_L1_INMTRX[0][0] +
	pLocalEpics->sus.Q1_L1_INMTRX[ii][1] * Q1_L1_INMTRX[0][1] +
	pLocalEpics->sus.Q1_L1_INMTRX[ii][2] * Q1_L1_INMTRX[0][2] +
	pLocalEpics->sus.Q1_L1_INMTRX[ii][3] * Q1_L1_INMTRX[0][3];
}
// End Matrix Calc ***************************



Q1_L1_Sum4 += filterModuleD(dspPtr,dspCoeff,Q1_L1_POS,Q1_L1_INMTRX[1][0],0);

Q1_L1_Sum5 += filterModuleD(dspPtr,dspCoeff,Q1_L1_PIT,Q1_L1_INMTRX[1][1],0);

Q1_L1_Sum6 += filterModuleD(dspPtr,dspCoeff,Q1_L1_YAW,Q1_L1_INMTRX[1][2],0);




Q1_L1_Sum += filterModuleD(dspPtr,dspCoeff,Q1_L1_ULPOS,Q1_L1_Sum4,0);

Q1_L1_Sum1 += filterModuleD(dspPtr,dspCoeff,Q1_L1_LLPOS,Q1_L1_Sum4,0);

Q1_L1_Sum2 += filterModuleD(dspPtr,dspCoeff,Q1_L1_URPOS,Q1_L1_Sum4,0);

Q1_L1_Sum3 += filterModuleD(dspPtr,dspCoeff,Q1_L1_LRPOS,Q1_L1_Sum4,0);

Q1_L1_Sum += filterModuleD(dspPtr,dspCoeff,Q1_L1_ULPIT,Q1_L1_Sum5,0);

Q1_L1_Sum1 += filterModuleD(dspPtr,dspCoeff,Q1_L1_LLPIT,Q1_L1_Sum5,0);

Q1_L1_Sum2 += filterModuleD(dspPtr,dspCoeff,Q1_L1_URPIT,Q1_L1_Sum5,0);

Q1_L1_Sum3 += filterModuleD(dspPtr,dspCoeff,Q1_L1_LRPIT,Q1_L1_Sum5,0);

Q1_L1_Sum += filterModuleD(dspPtr,dspCoeff,Q1_L1_ULYAW,Q1_L1_Sum6,0);

Q1_L1_Sum1 += filterModuleD(dspPtr,dspCoeff,Q1_L1_LLYAW,Q1_L1_Sum6,0);

Q1_L1_Sum2 += filterModuleD(dspPtr,dspCoeff,Q1_L1_URYAW,Q1_L1_Sum6,0);

Q1_L1_Sum3 += filterModuleD(dspPtr,dspCoeff,Q1_L1_LRYAW,Q1_L1_Sum6,0);





dacOut[0][12] = filterModuleD(dspPtr,dspCoeff,Q1_L1_ULOUT,Q1_L1_Sum,0);

dacOut[0][13] = filterModuleD(dspPtr,dspCoeff,Q1_L1_LLOUT,Q1_L1_Sum1,0);

dacOut[0][14] = filterModuleD(dspPtr,dspCoeff,Q1_L1_UROUT,Q1_L1_Sum2,0);

dacOut[0][15] = filterModuleD(dspPtr,dspCoeff,Q1_L1_LROUT,Q1_L1_Sum3,0);


//End of subsystem **************************************************



//Start of subsystem **************************************************

Q1_L2_INMTRX[0][0] = filterModuleD(dspPtr,dspCoeff,Q1_L2_ULSEN,dWord[0][18],0); 

Q1_L2_INMTRX[0][1] = filterModuleD(dspPtr,dspCoeff,Q1_L2_LLSEN,dWord[0][19],0); 

Q1_L2_INMTRX[0][2] = filterModuleD(dspPtr,dspCoeff,Q1_L2_URSEN,dWord[0][20],0); 

Q1_L2_INMTRX[0][3] = filterModuleD(dspPtr,dspCoeff,Q1_L2_LRSEN,dWord[0][21],0); 

Q1_L2_Sum4 += filterModuleD(dspPtr,dspCoeff,Q1_L2_LSC,0.0,0);

Q1_L2_Sum5 += filterModuleD(dspPtr,dspCoeff,Q1_L2_ASCP,0.0,0);

Q1_L2_Sum6 += filterModuleD(dspPtr,dspCoeff,Q1_L2_ASCY,0.0,0);


// Perform Matrix Calc **********************

for(ii=0;ii<3;ii++)
{
Q1_L2_INMTRX[1][ii] = 
	pLocalEpics->sus.Q1_L2_INMTRX[ii][0] * Q1_L2_INMTRX[0][0] +
	pLocalEpics->sus.Q1_L2_INMTRX[ii][1] * Q1_L2_INMTRX[0][1] +
	pLocalEpics->sus.Q1_L2_INMTRX[ii][2] * Q1_L2_INMTRX[0][2] +
	pLocalEpics->sus.Q1_L2_INMTRX[ii][3] * Q1_L2_INMTRX[0][3];
}
// End Matrix Calc ***************************



Q1_L2_Sum4 += filterModuleD(dspPtr,dspCoeff,Q1_L2_POS,Q1_L2_INMTRX[1][0],0);

Q1_L2_Sum5 += filterModuleD(dspPtr,dspCoeff,Q1_L2_PIT,Q1_L2_INMTRX[1][1],0);

Q1_L2_Sum6 += filterModuleD(dspPtr,dspCoeff,Q1_L2_YAW,Q1_L2_INMTRX[1][2],0);




Q1_L2_Sum += filterModuleD(dspPtr,dspCoeff,Q1_L2_ULPOS,Q1_L2_Sum4,0);

Q1_L2_Sum1 += filterModuleD(dspPtr,dspCoeff,Q1_L2_LLPOS,Q1_L2_Sum4,0);

Q1_L2_Sum2 += filterModuleD(dspPtr,dspCoeff,Q1_L2_URPOS,Q1_L2_Sum4,0);

Q1_L2_Sum3 += filterModuleD(dspPtr,dspCoeff,Q1_L2_LRPOS,Q1_L2_Sum4,0);

Q1_L2_Sum += filterModuleD(dspPtr,dspCoeff,Q1_L2_ULPIT,Q1_L2_Sum5,0);

Q1_L2_Sum1 += filterModuleD(dspPtr,dspCoeff,Q1_L2_LLPIT,Q1_L2_Sum5,0);

Q1_L2_Sum2 += filterModuleD(dspPtr,dspCoeff,Q1_L2_URPIT,Q1_L2_Sum5,0);

Q1_L2_Sum3 += filterModuleD(dspPtr,dspCoeff,Q1_L2_LRPIT,Q1_L2_Sum5,0);

Q1_L2_Sum += filterModuleD(dspPtr,dspCoeff,Q1_L2_ULYAW,Q1_L2_Sum6,0);

Q1_L2_Sum1 += filterModuleD(dspPtr,dspCoeff,Q1_L2_LLYAW,Q1_L2_Sum6,0);

Q1_L2_Sum2 += filterModuleD(dspPtr,dspCoeff,Q1_L2_URYAW,Q1_L2_Sum6,0);

Q1_L2_Sum3 += filterModuleD(dspPtr,dspCoeff,Q1_L2_LRYAW,Q1_L2_Sum6,0);





dacOut[1][0] = filterModuleD(dspPtr,dspCoeff,Q1_L2_ULOUT,Q1_L2_Sum,0);

dacOut[1][1] = filterModuleD(dspPtr,dspCoeff,Q1_L2_LLOUT,Q1_L2_Sum1,0);

dacOut[1][2] = filterModuleD(dspPtr,dspCoeff,Q1_L2_UROUT,Q1_L2_Sum2,0);

dacOut[1][3] = filterModuleD(dspPtr,dspCoeff,Q1_L2_LROUT,Q1_L2_Sum3,0);


//End of subsystem **************************************************



//Start of subsystem **************************************************

Q1_M0_INMTRX[0][0] = filterModuleD(dspPtr,dspCoeff,Q1_M0_FACE1,dWord[0][0],0); 

Q1_M0_INMTRX[0][1] = filterModuleD(dspPtr,dspCoeff,Q1_M0_FACE2,dWord[0][1],0); 

Q1_M0_INMTRX[0][2] = filterModuleD(dspPtr,dspCoeff,Q1_M0_FACE3,dWord[0][2],0); 

Q1_M0_INMTRX[0][3] = filterModuleD(dspPtr,dspCoeff,Q1_M0_LEFT,dWord[0][3],0); 

Q1_M0_INMTRX[0][4] = filterModuleD(dspPtr,dspCoeff,Q1_M0_RIGHT,dWord[0][4],0); 

Q1_M0_INMTRX[0][5] = filterModuleD(dspPtr,dspCoeff,Q1_M0_SIDE,dWord[0][5],0); 


// Perform Matrix Calc **********************

for(ii=0;ii<6;ii++)
{
Q1_M0_INMTRX[1][ii] = 
	pLocalEpics->sus.Q1_M0_INMTRX[ii][0] * Q1_M0_INMTRX[0][0] +
	pLocalEpics->sus.Q1_M0_INMTRX[ii][1] * Q1_M0_INMTRX[0][1] +
	pLocalEpics->sus.Q1_M0_INMTRX[ii][2] * Q1_M0_INMTRX[0][2] +
	pLocalEpics->sus.Q1_M0_INMTRX[ii][3] * Q1_M0_INMTRX[0][3] +
	pLocalEpics->sus.Q1_M0_INMTRX[ii][4] * Q1_M0_INMTRX[0][4] +
	pLocalEpics->sus.Q1_M0_INMTRX[ii][5] * Q1_M0_INMTRX[0][5];
}
// End Matrix Calc ***************************



Q1_M0_OUTMTRX[0][5] = filterModuleD(dspPtr,dspCoeff,Q1_M0_DOF6,Q1_M0_INMTRX[1][5],0);

Q1_M0_OUTMTRX[0][4] = filterModuleD(dspPtr,dspCoeff,Q1_M0_DOF5,Q1_M0_INMTRX[1][4],0);

Q1_M0_OUTMTRX[0][3] = filterModuleD(dspPtr,dspCoeff,Q1_M0_DOF4,Q1_M0_INMTRX[1][3],0);

Q1_M0_OUTMTRX[0][2] = filterModuleD(dspPtr,dspCoeff,Q1_M0_DOF3,Q1_M0_INMTRX[1][2],0);

Q1_M0_OUTMTRX[0][1] = filterModuleD(dspPtr,dspCoeff,Q1_M0_DOF2,Q1_M0_INMTRX[1][1],0);

Q1_M0_OUTMTRX[0][0] = filterModuleD(dspPtr,dspCoeff,Q1_M0_DOF1,Q1_M0_INMTRX[1][0],0);


// Perform Matrix Calc **********************

for(ii=0;ii<6;ii++)
{
Q1_M0_OUTMTRX[1][ii] = 
	pLocalEpics->sus.Q1_M0_OUTMTRX[ii][0] * Q1_M0_OUTMTRX[0][0] +
	pLocalEpics->sus.Q1_M0_OUTMTRX[ii][1] * Q1_M0_OUTMTRX[0][1] +
	pLocalEpics->sus.Q1_M0_OUTMTRX[ii][2] * Q1_M0_OUTMTRX[0][2] +
	pLocalEpics->sus.Q1_M0_OUTMTRX[ii][3] * Q1_M0_OUTMTRX[0][3] +
	pLocalEpics->sus.Q1_M0_OUTMTRX[ii][4] * Q1_M0_OUTMTRX[0][4] +
	pLocalEpics->sus.Q1_M0_OUTMTRX[ii][5] * Q1_M0_OUTMTRX[0][5];
}
// End Matrix Calc ***************************



dacOut[0][5] = filterModuleD(dspPtr,dspCoeff,Q1_M0_S_ACT,Q1_M0_OUTMTRX[1][5],0);

dacOut[0][4] = filterModuleD(dspPtr,dspCoeff,Q1_M0_R_ACT,Q1_M0_OUTMTRX[1][4],0);

dacOut[0][3] = filterModuleD(dspPtr,dspCoeff,Q1_M0_L_ACT,Q1_M0_OUTMTRX[1][3],0);

dacOut[0][2] = filterModuleD(dspPtr,dspCoeff,Q1_M0_F3_ACT,Q1_M0_OUTMTRX[1][2],0);

dacOut[0][1] = filterModuleD(dspPtr,dspCoeff,Q1_M0_F2_ACT,Q1_M0_OUTMTRX[1][1],0);

dacOut[0][0] = filterModuleD(dspPtr,dspCoeff,Q1_M0_F1_ACT,Q1_M0_OUTMTRX[1][0],0);


//End of subsystem **************************************************



//Start of subsystem **************************************************

Q1_R0_INMTRX[0][0] = filterModuleD(dspPtr,dspCoeff,Q1_R0_FACE1,dWord[0][6],0); 

Q1_R0_INMTRX[0][1] = filterModuleD(dspPtr,dspCoeff,Q1_R0_FACE2,dWord[0][7],0); 

Q1_R0_INMTRX[0][2] = filterModuleD(dspPtr,dspCoeff,Q1_R0_FACE3,dWord[0][8],0); 

Q1_R0_INMTRX[0][3] = filterModuleD(dspPtr,dspCoeff,Q1_R0_LEFT,dWord[0][9],0); 

Q1_R0_INMTRX[0][4] = filterModuleD(dspPtr,dspCoeff,Q1_R0_RIGHT,dWord[0][10],0); 

Q1_R0_INMTRX[0][5] = filterModuleD(dspPtr,dspCoeff,Q1_R0_SIDE,dWord[0][11],0); 


// Perform Matrix Calc **********************

for(ii=0;ii<6;ii++)
{
Q1_R0_INMTRX[1][ii] = 
	pLocalEpics->sus.Q1_R0_INMTRX[ii][0] * Q1_R0_INMTRX[0][0] +
	pLocalEpics->sus.Q1_R0_INMTRX[ii][1] * Q1_R0_INMTRX[0][1] +
	pLocalEpics->sus.Q1_R0_INMTRX[ii][2] * Q1_R0_INMTRX[0][2] +
	pLocalEpics->sus.Q1_R0_INMTRX[ii][3] * Q1_R0_INMTRX[0][3] +
	pLocalEpics->sus.Q1_R0_INMTRX[ii][4] * Q1_R0_INMTRX[0][4] +
	pLocalEpics->sus.Q1_R0_INMTRX[ii][5] * Q1_R0_INMTRX[0][5];
}
// End Matrix Calc ***************************



Q1_R0_OUTMTRX[0][5] = filterModuleD(dspPtr,dspCoeff,Q1_R0_DOF6,Q1_R0_INMTRX[1][5],0);

Q1_R0_OUTMTRX[0][4] = filterModuleD(dspPtr,dspCoeff,Q1_R0_DOF5,Q1_R0_INMTRX[1][4],0);

Q1_R0_OUTMTRX[0][3] = filterModuleD(dspPtr,dspCoeff,Q1_R0_DOF4,Q1_R0_INMTRX[1][3],0);

Q1_R0_OUTMTRX[0][2] = filterModuleD(dspPtr,dspCoeff,Q1_R0_DOF3,Q1_R0_INMTRX[1][2],0);

Q1_R0_OUTMTRX[0][1] = filterModuleD(dspPtr,dspCoeff,Q1_R0_DOF2,Q1_R0_INMTRX[1][1],0);

Q1_R0_OUTMTRX[0][0] = filterModuleD(dspPtr,dspCoeff,Q1_R0_DOF1,Q1_R0_INMTRX[1][0],0);


// Perform Matrix Calc **********************

for(ii=0;ii<6;ii++)
{
Q1_R0_OUTMTRX[1][ii] = 
	pLocalEpics->sus.Q1_R0_OUTMTRX[ii][0] * Q1_R0_OUTMTRX[0][0] +
	pLocalEpics->sus.Q1_R0_OUTMTRX[ii][1] * Q1_R0_OUTMTRX[0][1] +
	pLocalEpics->sus.Q1_R0_OUTMTRX[ii][2] * Q1_R0_OUTMTRX[0][2] +
	pLocalEpics->sus.Q1_R0_OUTMTRX[ii][3] * Q1_R0_OUTMTRX[0][3] +
	pLocalEpics->sus.Q1_R0_OUTMTRX[ii][4] * Q1_R0_OUTMTRX[0][4] +
	pLocalEpics->sus.Q1_R0_OUTMTRX[ii][5] * Q1_R0_OUTMTRX[0][5];
}
// End Matrix Calc ***************************



dacOut[0][11] = filterModuleD(dspPtr,dspCoeff,Q1_R0_S_ACT,Q1_R0_OUTMTRX[1][5],0);

dacOut[0][10] = filterModuleD(dspPtr,dspCoeff,Q1_R0_R_ACT,Q1_R0_OUTMTRX[1][4],0);

dacOut[0][9] = filterModuleD(dspPtr,dspCoeff,Q1_R0_L_ACT,Q1_R0_OUTMTRX[1][3],0);

dacOut[0][8] = filterModuleD(dspPtr,dspCoeff,Q1_R0_F3_ACT,Q1_R0_OUTMTRX[1][2],0);

dacOut[0][7] = filterModuleD(dspPtr,dspCoeff,Q1_R0_F2_ACT,Q1_R0_OUTMTRX[1][1],0);

dacOut[0][6] = filterModuleD(dspPtr,dspCoeff,Q1_R0_F1_ACT,Q1_R0_OUTMTRX[1][0],0);

}

