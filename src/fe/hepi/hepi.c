// ******* This is a computer generated file *******
// ******* DO NOT HAND EDIT ************************


void feCode(double dWord[][32],	/* ADC inputs */
		int dacOut[][16],	/* DAC outputs */
		FILT_MOD *dspPtr,	/* Filter Mod variables */
		COEF *dspCoeff,		/* Filter Mod coeffs */
		CDS_EPICS *pLocalEpics)	/* EPICS variables */
{

int ii,jj;

double bsc_hp;
double bsc_loop_gain[8];
float BSC_LOOP_GAIN_CALC;
double bsc_loop_sw[16];
double bsc_mtrx[8][8];
double bsc_rx;
double bsc_ry;
double bsc_rz;
double bsc_sum;
double bsc_sum1;
double bsc_sum2;
double bsc_sum3;
double bsc_sum4;
double bsc_sum5;
double bsc_sum6;
double bsc_sum7;
double bsc_tiltcorr_gain[8];
float BSC_TILTCORR_GAIN_CALC;
double bsc_tiltcorr_h1;
double bsc_tiltcorr_h2;
double bsc_tiltcorr_h3;
double bsc_tiltcorr_h4;
double bsc_tiltcorr_sw[8];
double bsc_tiltcorr_v1;
double bsc_tiltcorr_v2;
double bsc_tiltcorr_v3;
double bsc_tiltcorr_v4;
double bsc_vp;
double bsc_x;
double bsc_y;
double bsc_z;
double bsc_act_gain[8];
float BSC_ACT_GAIN_CALC;
double bsc_act_h1;
double bsc_act_h2;
double bsc_act_h3;
double bsc_act_h4;
double bsc_act_sw[8];
double bsc_act_sum;
double bsc_act_sum1;
double bsc_act_sum2;
double bsc_act_sum3;
double bsc_act_sum4;
double bsc_act_sum5;
double bsc_act_sum6;
double bsc_act_sum7;
double bsc_act_v1;
double bsc_act_v2;
double bsc_act_v3;
double bsc_act_v4;
double bsc_dc_bias_mtrx[8][6];
double bsc_dc_bias_sw[8];
double bsc_fir_mtrx[2][4];
double bsc_fir_x_out;
double bsc_fir_y_out;
double bsc_geo_h1;
double bsc_geo_h1e;
double bsc_geo_h2;
double bsc_geo_h2e;
double bsc_geo_h3;
double bsc_geo_h3e;
double bsc_geo_h4;
double bsc_geo_h4e;
double bsc_geo_hp;
double bsc_geo_mtrx[8][8];
double bsc_geo_rx;
double bsc_geo_ry;
double bsc_geo_rz;
double bsc_geo_sw[8];
double bsc_geo_sum;
double bsc_geo_sum1;
double bsc_geo_sum2;
double bsc_geo_sum3;
double bsc_geo_sum4;
double bsc_geo_sum5;
double bsc_geo_sum6;
double bsc_geo_sum7;
double bsc_geo_v1;
double bsc_geo_v1e;
double bsc_geo_v2;
double bsc_geo_v2e;
double bsc_geo_v3;
double bsc_geo_v3e;
double bsc_geo_v4;
double bsc_geo_v4e;
double bsc_geo_vp;
double bsc_geo_x;
double bsc_geo_y;
double bsc_geo_z;
double bsc_pos_h1;
double bsc_pos_h1e;
double bsc_pos_h2;
double bsc_pos_h2e;
double bsc_pos_h3;
double bsc_pos_h3e;
double bsc_pos_h4;
double bsc_pos_h4e;
double bsc_pos_hp;
double bsc_pos_mtrx[8][8];
double bsc_pos_norm[16];
double bsc_pos_rx;
double bsc_pos_ry;
double bsc_pos_rz;
double bsc_pos_sw[8];
double bsc_pos_sum;
double bsc_pos_sum1;
double bsc_pos_v1;
double bsc_pos_v1e;
double bsc_pos_v2;
double bsc_pos_v2e;
double bsc_pos_v3;
double bsc_pos_v3e;
double bsc_pos_v4;
double bsc_pos_v4e;
double bsc_pos_vp;
double bsc_pos_x;
double bsc_pos_y;
double bsc_pos_z;
double bsc_spare_fp1;
double bsc_spare_fp2;
double bsc_spare_rp1;
double bsc_spare_rp2;
double bsc_spare_rp3;
double bsc_sts_in_mtrx[3][6];
double bsc_sts_mtrx[8][3];
double bsc_sts_ramp_sw[4];
double bsc_sts_x;
double bsc_sts_xe;
double bsc_sts_x_rx;
double bsc_sts_x_ry;
double bsc_sts_y;
double bsc_sts_ye;
double bsc_sts_z;
double bsc_sts_ze;
static double delay1;
static double delay2;
static double delay3;
static double delay4;
static double delay5;
static double delay6;
static double delay7;
static double delay8;
double sts1_fir_x;
double sts1_fir_y;
double sts2_fir_x;
double sts2_fir_y;



//Start of subsystem **************************************************

// MATRIX CALC
for(ii=0;ii<8;ii++)
{
bsc_dc_bias_mtrx[1][ii] = 
	pLocalEpics->sei.BSC_DC_BIAS_MTRX[ii][0] * pLocalEpics->sei.BSC_DC_BIAS_1 +
	pLocalEpics->sei.BSC_DC_BIAS_MTRX[ii][1] * pLocalEpics->sei.BSC_DC_BIAS_2 +
	pLocalEpics->sei.BSC_DC_BIAS_MTRX[ii][2] * pLocalEpics->sei.BSC_DC_BIAS_3 +
	pLocalEpics->sei.BSC_DC_BIAS_MTRX[ii][3] * pLocalEpics->sei.BSC_DC_BIAS_4 +
	pLocalEpics->sei.BSC_DC_BIAS_MTRX[ii][4] * pLocalEpics->sei.BSC_DC_BIAS_5 +
	pLocalEpics->sei.BSC_DC_BIAS_MTRX[ii][5] * pLocalEpics->sei.BSC_DC_BIAS_6;
}

// MULTI_SW
bsc_dc_bias_sw[0] = bsc_dc_bias_mtrx[1][0];
bsc_dc_bias_sw[1] = bsc_dc_bias_mtrx[1][1];
bsc_dc_bias_sw[2] = bsc_dc_bias_mtrx[1][2];
bsc_dc_bias_sw[3] = bsc_dc_bias_mtrx[1][3];
bsc_dc_bias_sw[4] = bsc_dc_bias_mtrx[1][4];
bsc_dc_bias_sw[5] = bsc_dc_bias_mtrx[1][5];
bsc_dc_bias_sw[6] = bsc_dc_bias_mtrx[1][6];
bsc_dc_bias_sw[7] = bsc_dc_bias_mtrx[1][7];
if(pLocalEpics->sei.BSC_DC_BIAS_SW == 0)
{
	for(ii=0;ii< 8;ii++) bsc_dc_bias_sw[ii] = 0.0;
}



//End of subsystem **************************************************



//Start of subsystem **************************************************

// FILTER MODULE
bsc_spare_fp1 = filterModuleD(dspPtr,dspCoeff,BSC_SPARE_FP1,dWord[0][26],0);

// FILTER MODULE
bsc_spare_fp2 = filterModuleD(dspPtr,dspCoeff,BSC_SPARE_FP2,dWord[0][27],0);

// FILTER MODULE
bsc_spare_rp1 = filterModuleD(dspPtr,dspCoeff,BSC_SPARE_RP1,dWord[0][28],0);

// FILTER MODULE
bsc_spare_rp2 = filterModuleD(dspPtr,dspCoeff,BSC_SPARE_RP2,dWord[0][29],0);

// FILTER MODULE
bsc_spare_rp3 = filterModuleD(dspPtr,dspCoeff,BSC_SPARE_RP3,dWord[0][30],0);

// EPICS_OUTPUT
pLocalEpics->sei.BSC_WIT_1_SENSOR = dWord[0][16];

// EPICS_OUTPUT
pLocalEpics->sei.BSC_WIT_2_SENSOR = dWord[0][17];

// EPICS_OUTPUT
pLocalEpics->sei.BSC_WIT_3_SENSOR = dWord[0][18];

// EPICS_OUTPUT
pLocalEpics->sei.BSC_WIT_4_SENSOR = dWord[0][19];



// EPICS_OUTPUT
pLocalEpics->sei.STS1_X = dWord[0][23];

// EPICS_OUTPUT
pLocalEpics->sei.STS1_Y = dWord[0][24];

// EPICS_OUTPUT
pLocalEpics->sei.STS1_Z = dWord[0][25];



// EPICS_OUTPUT
pLocalEpics->sei.STS2_X = dWord[1][23];

// EPICS_OUTPUT
pLocalEpics->sei.STS2_Y = dWord[1][24];

// EPICS_OUTPUT
pLocalEpics->sei.STS2_Z = dWord[1][25];


//End of subsystem **************************************************



//Start of subsystem **************************************************

// MATRIX CALC
for(ii=0;ii<2;ii++)
{
bsc_fir_mtrx[1][ii] = 
	pLocalEpics->sei.BSC_FIR_MTRX[ii][0] * sts1_fir_x +
	pLocalEpics->sei.BSC_FIR_MTRX[ii][1] * sts1_fir_y +
	pLocalEpics->sei.BSC_FIR_MTRX[ii][2] * sts2_fir_x +
	pLocalEpics->sei.BSC_FIR_MTRX[ii][3] * sts2_fir_y;
}

// FILTER MODULE
bsc_fir_x_out = filterModuleD(dspPtr,dspCoeff,BSC_FIR_X_OUT,bsc_fir_mtrx[1][0],0);

// FILTER MODULE
bsc_fir_y_out = filterModuleD(dspPtr,dspCoeff,BSC_FIR_Y_OUT,bsc_fir_mtrx[1][1],0);


//End of subsystem **************************************************



//Start of subsystem **************************************************

// EPICS_OUTPUT
pLocalEpics->sei.BSC_GEO_SV1 = dWord[0][4];

// EPICS_OUTPUT
pLocalEpics->sei.BSC_GEO_SV2 = dWord[0][5];

// EPICS_OUTPUT
pLocalEpics->sei.BSC_GEO_SV3 = dWord[0][6];

// EPICS_OUTPUT
pLocalEpics->sei.BSC_GEO_SV4 = dWord[0][7];

// EPICS_OUTPUT
pLocalEpics->sei.BSC_GEO_SH1 = dWord[0][0];

// EPICS_OUTPUT
pLocalEpics->sei.BSC_GEO_SH2 = dWord[0][1];

// EPICS_OUTPUT
pLocalEpics->sei.BSC_GEO_SH3 = dWord[0][2];

// EPICS_OUTPUT
pLocalEpics->sei.BSC_GEO_SH4 = dWord[0][3];

// SUM
bsc_geo_sum = pLocalEpics->sei.BSC_GEO_SV1 + delay1;

// SUM
bsc_geo_sum1 = pLocalEpics->sei.BSC_GEO_SV2 + delay2;

// SUM
bsc_geo_sum2 = pLocalEpics->sei.BSC_GEO_SV3 + delay3;

// SUM
bsc_geo_sum3 = pLocalEpics->sei.BSC_GEO_SV4 + delay4;

// SUM
bsc_geo_sum4 = pLocalEpics->sei.BSC_GEO_SH1 + delay5;

// SUM
bsc_geo_sum5 = pLocalEpics->sei.BSC_GEO_SH2 + delay6;

// SUM
bsc_geo_sum6 = pLocalEpics->sei.BSC_GEO_SH3 + delay7;

// SUM
bsc_geo_sum7 = pLocalEpics->sei.BSC_GEO_SH4 + delay8;

// FILTER MODULE
bsc_geo_v1 = filterModuleD(dspPtr,dspCoeff,BSC_GEO_V1,bsc_geo_sum,0);

// FILTER MODULE
bsc_geo_v1e = filterModuleD(dspPtr,dspCoeff,BSC_GEO_V1E,bsc_geo_sum,0);

// FILTER MODULE
bsc_geo_v2 = filterModuleD(dspPtr,dspCoeff,BSC_GEO_V2,bsc_geo_sum1,0);

// FILTER MODULE
bsc_geo_v2e = filterModuleD(dspPtr,dspCoeff,BSC_GEO_V2E,bsc_geo_sum1,0);

// FILTER MODULE
bsc_geo_v3 = filterModuleD(dspPtr,dspCoeff,BSC_GEO_V3,bsc_geo_sum2,0);

// FILTER MODULE
bsc_geo_v3e = filterModuleD(dspPtr,dspCoeff,BSC_GEO_V3E,bsc_geo_sum2,0);

// FILTER MODULE
bsc_geo_v4 = filterModuleD(dspPtr,dspCoeff,BSC_GEO_V4,bsc_geo_sum3,0);

// FILTER MODULE
bsc_geo_v4e = filterModuleD(dspPtr,dspCoeff,BSC_GEO_V4E,bsc_geo_sum3,0);

// FILTER MODULE
bsc_geo_h1 = filterModuleD(dspPtr,dspCoeff,BSC_GEO_H1,bsc_geo_sum4,0);

// FILTER MODULE
bsc_geo_h1e = filterModuleD(dspPtr,dspCoeff,BSC_GEO_H1E,bsc_geo_sum4,0);

// FILTER MODULE
bsc_geo_h2 = filterModuleD(dspPtr,dspCoeff,BSC_GEO_H2,bsc_geo_sum5,0);

// FILTER MODULE
bsc_geo_h2e = filterModuleD(dspPtr,dspCoeff,BSC_GEO_H2E,bsc_geo_sum5,0);

// FILTER MODULE
bsc_geo_h3 = filterModuleD(dspPtr,dspCoeff,BSC_GEO_H3,bsc_geo_sum6,0);

// FILTER MODULE
bsc_geo_h3e = filterModuleD(dspPtr,dspCoeff,BSC_GEO_H3E,bsc_geo_sum6,0);

// FILTER MODULE
bsc_geo_h4 = filterModuleD(dspPtr,dspCoeff,BSC_GEO_H4,bsc_geo_sum7,0);

// FILTER MODULE
bsc_geo_h4e = filterModuleD(dspPtr,dspCoeff,BSC_GEO_H4E,bsc_geo_sum7,0);

// MATRIX CALC
for(ii=0;ii<8;ii++)
{
bsc_geo_mtrx[1][ii] = 
	pLocalEpics->sei.BSC_GEO_MTRX[ii][0] * bsc_geo_v1 +
	pLocalEpics->sei.BSC_GEO_MTRX[ii][1] * bsc_geo_v2 +
	pLocalEpics->sei.BSC_GEO_MTRX[ii][2] * bsc_geo_v3 +
	pLocalEpics->sei.BSC_GEO_MTRX[ii][3] * bsc_geo_v4 +
	pLocalEpics->sei.BSC_GEO_MTRX[ii][4] * bsc_geo_h1 +
	pLocalEpics->sei.BSC_GEO_MTRX[ii][5] * bsc_geo_h2 +
	pLocalEpics->sei.BSC_GEO_MTRX[ii][6] * bsc_geo_h3 +
	pLocalEpics->sei.BSC_GEO_MTRX[ii][7] * bsc_geo_h4;
}

// FILTER MODULE
bsc_geo_x = filterModuleD(dspPtr,dspCoeff,BSC_GEO_X,bsc_geo_mtrx[1][0],0);

// FILTER MODULE
bsc_geo_y = filterModuleD(dspPtr,dspCoeff,BSC_GEO_Y,bsc_geo_mtrx[1][1],0);

// FILTER MODULE
bsc_geo_z = filterModuleD(dspPtr,dspCoeff,BSC_GEO_Z,bsc_geo_mtrx[1][2],0);

// FILTER MODULE
bsc_geo_rz = filterModuleD(dspPtr,dspCoeff,BSC_GEO_RZ,bsc_geo_mtrx[1][5],0);

// FILTER MODULE
bsc_geo_vp = filterModuleD(dspPtr,dspCoeff,BSC_GEO_VP,bsc_geo_mtrx[1][6],0);

// FILTER MODULE
bsc_geo_hp = filterModuleD(dspPtr,dspCoeff,BSC_GEO_HP,bsc_geo_mtrx[1][7],0);

// FILTER MODULE
bsc_geo_rx = filterModuleD(dspPtr,dspCoeff,BSC_GEO_RX,bsc_geo_mtrx[1][3],0);

// FILTER MODULE
bsc_geo_ry = filterModuleD(dspPtr,dspCoeff,BSC_GEO_RY,bsc_geo_mtrx[1][4],0);

// MULTI_SW
bsc_geo_sw[0] = bsc_geo_x;
bsc_geo_sw[1] = bsc_geo_y;
bsc_geo_sw[2] = bsc_geo_z;
bsc_geo_sw[3] = bsc_geo_rx;
bsc_geo_sw[4] = bsc_geo_ry;
bsc_geo_sw[5] = bsc_geo_rz;
bsc_geo_sw[6] = bsc_geo_vp;
bsc_geo_sw[7] = bsc_geo_hp;
if(pLocalEpics->sei.BSC_GEO_SW == 0)
{
	for(ii=0;ii< 8;ii++) bsc_geo_sw[ii] = 0.0;
}



//End of subsystem **************************************************



//Start of subsystem **************************************************

// MATRIX CALC
for(ii=0;ii<3;ii++)
{
bsc_sts_in_mtrx[1][ii] = 
	pLocalEpics->sei.BSC_STS_IN_MTRX[ii][0] * pLocalEpics->sei.STS1_X +
	pLocalEpics->sei.BSC_STS_IN_MTRX[ii][1] * pLocalEpics->sei.STS1_Y +
	pLocalEpics->sei.BSC_STS_IN_MTRX[ii][2] * pLocalEpics->sei.STS1_Z +
	pLocalEpics->sei.BSC_STS_IN_MTRX[ii][3] * dWord[1][23] +
	pLocalEpics->sei.BSC_STS_IN_MTRX[ii][4] * dWord[1][24] +
	pLocalEpics->sei.BSC_STS_IN_MTRX[ii][5] * dWord[1][25];
}

// FILTER MODULE
bsc_sts_y = filterModuleD(dspPtr,dspCoeff,BSC_STS_Y,bsc_sts_in_mtrx[1][1],0);

// FILTER MODULE
bsc_sts_ye = filterModuleD(dspPtr,dspCoeff,BSC_STS_YE,bsc_sts_in_mtrx[1][1],0);

// FILTER MODULE
bsc_sts_x = filterModuleD(dspPtr,dspCoeff,BSC_STS_X,bsc_sts_in_mtrx[1][0],0);

// FILTER MODULE
bsc_sts_xe = filterModuleD(dspPtr,dspCoeff,BSC_STS_XE,bsc_sts_in_mtrx[1][0],0);

// FILTER MODULE
bsc_sts_z = filterModuleD(dspPtr,dspCoeff,BSC_STS_Z,bsc_sts_in_mtrx[1][2],0);

// FILTER MODULE
bsc_sts_ze = filterModuleD(dspPtr,dspCoeff,BSC_STS_ZE,bsc_sts_in_mtrx[1][2],0);

// RAMP_SW
bsc_sts_ramp_sw[0] = bsc_sts_x;
bsc_sts_ramp_sw[1] = bsc_fir_x_out;
bsc_sts_ramp_sw[2] = bsc_sts_y;
bsc_sts_ramp_sw[3] = bsc_fir_y_out;
if(pLocalEpics->sei.BSC_STS_RAMP_SW == 0)
{
	bsc_sts_ramp_sw[1] = bsc_sts_ramp_sw[2];
}
else
{
	bsc_sts_ramp_sw[0] = bsc_sts_ramp_sw[1];
	bsc_sts_ramp_sw[1] = bsc_sts_ramp_sw[3];
}


// FILTER MODULE
bsc_sts_x_ry = filterModuleD(dspPtr,dspCoeff,BSC_STS_X_RY,bsc_sts_ramp_sw[1],0);

// MATRIX CALC
for(ii=0;ii<8;ii++)
{
bsc_sts_mtrx[1][ii] = 
	pLocalEpics->sei.BSC_STS_MTRX[ii][0] * bsc_sts_ramp_sw[0] +
	pLocalEpics->sei.BSC_STS_MTRX[ii][1] * bsc_sts_ramp_sw[1] +
	pLocalEpics->sei.BSC_STS_MTRX[ii][2] * bsc_sts_z;
}

// FILTER MODULE
bsc_sts_x_rx = filterModuleD(dspPtr,dspCoeff,BSC_STS_X_RX,bsc_sts_ramp_sw[0],0);


//End of subsystem **************************************************



//Start of subsystem **************************************************

// FILTER MODULE
bsc_pos_v1 = filterModuleD(dspPtr,dspCoeff,BSC_POS_V1,dWord[0][12],0);

// FILTER MODULE
bsc_pos_v1e = filterModuleD(dspPtr,dspCoeff,BSC_POS_V1E,dWord[0][12],0);

// FILTER MODULE
bsc_pos_v2 = filterModuleD(dspPtr,dspCoeff,BSC_POS_V2,dWord[0][13],0);

// FILTER MODULE
bsc_pos_v2e = filterModuleD(dspPtr,dspCoeff,BSC_POS_V2E,dWord[0][13],0);

// FILTER MODULE
bsc_pos_v3 = filterModuleD(dspPtr,dspCoeff,BSC_POS_V3,dWord[0][14],0);

// FILTER MODULE
bsc_pos_v3e = filterModuleD(dspPtr,dspCoeff,BSC_POS_V3E,dWord[0][14],0);

// FILTER MODULE
bsc_pos_v4 = filterModuleD(dspPtr,dspCoeff,BSC_POS_V4,dWord[0][15],0);

// FILTER MODULE
bsc_pos_v4e = filterModuleD(dspPtr,dspCoeff,BSC_POS_V4E,dWord[0][15],0);

// FILTER MODULE
bsc_pos_h1 = filterModuleD(dspPtr,dspCoeff,BSC_POS_H1,dWord[0][8],0);

// FILTER MODULE
bsc_pos_h1e = filterModuleD(dspPtr,dspCoeff,BSC_POS_H1E,dWord[0][8],0);

// FILTER MODULE
bsc_pos_h2 = filterModuleD(dspPtr,dspCoeff,BSC_POS_H2,dWord[0][9],0);

// FILTER MODULE
bsc_pos_h2e = filterModuleD(dspPtr,dspCoeff,BSC_POS_H2E,dWord[0][9],0);

// FILTER MODULE
bsc_pos_h3 = filterModuleD(dspPtr,dspCoeff,BSC_POS_H3,dWord[0][10],0);

// FILTER MODULE
bsc_pos_h3e = filterModuleD(dspPtr,dspCoeff,BSC_POS_H3E,dWord[0][10],0);

// FILTER MODULE
bsc_pos_h4 = filterModuleD(dspPtr,dspCoeff,BSC_POS_H4,dWord[0][11],0);

// FILTER MODULE
bsc_pos_h4e = filterModuleD(dspPtr,dspCoeff,BSC_POS_H4E,dWord[0][11],0);

// DIFF_JUNC
bsc_pos_norm[0] = bsc_pos_v1 - bsc_sts_mtrx[1][0];
bsc_pos_norm[1] = bsc_pos_v2 - bsc_sts_mtrx[1][1];
bsc_pos_norm[2] = bsc_pos_v3 - bsc_sts_mtrx[1][2];
bsc_pos_norm[3] = bsc_pos_v4 - bsc_sts_mtrx[1][3];
bsc_pos_norm[4] = bsc_pos_h1 - bsc_sts_mtrx[1][4];
bsc_pos_norm[5] = bsc_pos_h2 - bsc_sts_mtrx[1][5];
bsc_pos_norm[6] = bsc_pos_h3 - bsc_sts_mtrx[1][6];
bsc_pos_norm[7] = bsc_pos_h4 - bsc_sts_mtrx[1][7];

// SEI WD GOES HERE ***

// bsc_pos_watchdogFilt[0] = bsc_pos_v1e;
// bsc_pos_watchdogRaw[0] = dWord[0][12];
// bsc_pos_watchdogFilt[1] = bsc_pos_v2e;
// bsc_pos_watchdogRaw[1] = dWord[0][13];
// bsc_pos_watchdogFilt[2] = bsc_pos_v3e;
// bsc_pos_watchdogRaw[2] = dWord[0][14];
// bsc_pos_watchdogFilt[3] = bsc_pos_v4e;
// bsc_pos_watchdogRaw[3] = dWord[0][15];
// bsc_pos_watchdogFilt[4] = bsc_pos_h1e;
// bsc_pos_watchdogRaw[4] = dWord[0][8];
// bsc_pos_watchdogFilt[5] = bsc_pos_h2e;
// bsc_pos_watchdogRaw[5] = dWord[0][9];
// bsc_pos_watchdogFilt[6] = bsc_pos_h3e;
// bsc_pos_watchdogRaw[6] = dWord[0][10];
// bsc_pos_watchdogFilt[7] = bsc_pos_h4e;
// bsc_pos_watchdogRaw[7] = dWord[0][11];
// seiWd1(bsc_pos_watchdogRaw, bsc_pos_watchdogFilt,&pLocalEpics->BSC_POS_WATCHDOG)

// MATRIX CALC
for(ii=0;ii<8;ii++)
{
bsc_pos_mtrx[1][ii] = 
	pLocalEpics->sei.BSC_POS_MTRX[ii][0] * bsc_pos_norm[0] +
	pLocalEpics->sei.BSC_POS_MTRX[ii][1] * bsc_pos_norm[1] +
	pLocalEpics->sei.BSC_POS_MTRX[ii][2] * bsc_pos_norm[2] +
	pLocalEpics->sei.BSC_POS_MTRX[ii][3] * bsc_pos_norm[3] +
	pLocalEpics->sei.BSC_POS_MTRX[ii][4] * bsc_pos_norm[4] +
	pLocalEpics->sei.BSC_POS_MTRX[ii][5] * bsc_pos_norm[5] +
	pLocalEpics->sei.BSC_POS_MTRX[ii][6] * bsc_pos_norm[6] +
	pLocalEpics->sei.BSC_POS_MTRX[ii][7] * bsc_pos_norm[7];
}

// FILTER MODULE
bsc_pos_x = filterModuleD(dspPtr,dspCoeff,BSC_POS_X,bsc_pos_mtrx[1][0],0);

// FILTER MODULE
bsc_pos_y = filterModuleD(dspPtr,dspCoeff,BSC_POS_Y,bsc_pos_mtrx[1][1],0);

// FILTER MODULE
bsc_pos_z = filterModuleD(dspPtr,dspCoeff,BSC_POS_Z,bsc_pos_mtrx[1][2],0);

// FILTER MODULE
bsc_pos_rz = filterModuleD(dspPtr,dspCoeff,BSC_POS_RZ,bsc_pos_mtrx[1][5],0);

// FILTER MODULE
bsc_pos_vp = filterModuleD(dspPtr,dspCoeff,BSC_POS_VP,bsc_pos_mtrx[1][6],0);

// FILTER MODULE
bsc_pos_hp = filterModuleD(dspPtr,dspCoeff,BSC_POS_HP,bsc_pos_mtrx[1][7],0);

// SUM
bsc_pos_sum = bsc_pos_mtrx[1][3] + bsc_sts_x_rx;

// SUM
bsc_pos_sum1 = bsc_pos_mtrx[1][4] + bsc_sts_x_ry;

// FILTER MODULE
bsc_pos_rx = filterModuleD(dspPtr,dspCoeff,BSC_POS_RX,bsc_pos_sum,0);

// FILTER MODULE
bsc_pos_ry = filterModuleD(dspPtr,dspCoeff,BSC_POS_RY,bsc_pos_sum1,0);

// MULTI_SW
bsc_pos_sw[0] = bsc_pos_x;
bsc_pos_sw[1] = bsc_pos_y;
bsc_pos_sw[2] = bsc_pos_z;
bsc_pos_sw[3] = bsc_pos_rx;
bsc_pos_sw[4] = bsc_pos_ry;
bsc_pos_sw[5] = bsc_pos_rz;
bsc_pos_sw[6] = bsc_pos_vp;
bsc_pos_sw[7] = bsc_pos_hp;
if(pLocalEpics->sei.BSC_POS_SW == 0)
{
	for(ii=0;ii< 8;ii++) bsc_pos_sw[ii] = 0.0;
}



//End of subsystem **************************************************



//Start of subsystem **************************************************

// SUM
bsc_sum = bsc_pos_sw[0] + bsc_geo_sw[0];

// SUM
bsc_sum1 = bsc_pos_sw[1] + bsc_geo_sw[1];

// SUM
bsc_sum2 = bsc_pos_sw[2] + bsc_geo_sw[2];

// SUM
bsc_sum3 = bsc_pos_sw[3] + bsc_geo_sw[3];

// SUM
bsc_sum4 = bsc_pos_sw[4] + bsc_geo_sw[4];

// SUM
bsc_sum5 = bsc_pos_sw[5] + bsc_geo_sw[5];

// SUM
bsc_sum6 = bsc_pos_sw[6] + bsc_geo_sw[6];

// SUM
bsc_sum7 = bsc_pos_sw[7] + bsc_geo_sw[7];

// FILTER MODULE
bsc_x = filterModuleD(dspPtr,dspCoeff,BSC_X,bsc_sum,0);

// FILTER MODULE
bsc_y = filterModuleD(dspPtr,dspCoeff,BSC_Y,bsc_sum1,0);

// FILTER MODULE
bsc_z = filterModuleD(dspPtr,dspCoeff,BSC_Z,bsc_sum2,0);

// FILTER MODULE
bsc_rx = filterModuleD(dspPtr,dspCoeff,BSC_RX,bsc_sum3,0);

// FILTER MODULE
bsc_ry = filterModuleD(dspPtr,dspCoeff,BSC_RY,bsc_sum4,0);

// FILTER MODULE
bsc_rz = filterModuleD(dspPtr,dspCoeff,BSC_RZ,bsc_sum5,0);

// FILTER MODULE
bsc_vp = filterModuleD(dspPtr,dspCoeff,BSC_VP,bsc_sum6,0);

// FILTER MODULE
bsc_hp = filterModuleD(dspPtr,dspCoeff,BSC_HP,bsc_sum7,0);

// MATRIX CALC
for(ii=0;ii<8;ii++)
{
bsc_mtrx[1][ii] = 
	pLocalEpics->sei.BSC_MTRX[ii][0] * bsc_x +
	pLocalEpics->sei.BSC_MTRX[ii][1] * bsc_y +
	pLocalEpics->sei.BSC_MTRX[ii][2] * bsc_z +
	pLocalEpics->sei.BSC_MTRX[ii][3] * bsc_rx +
	pLocalEpics->sei.BSC_MTRX[ii][4] * bsc_ry +
	pLocalEpics->sei.BSC_MTRX[ii][5] * bsc_rz +
	pLocalEpics->sei.BSC_MTRX[ii][6] * bsc_vp +
	pLocalEpics->sei.BSC_MTRX[ii][7] * bsc_hp;
}

// PRODUCT
pLocalEpics->sei.BSC_LOOP_GAIN_RMON = 
	gainRamp(pLocalEpics->sei.BSC_LOOP_GAIN,pLocalEpics->sei.BSC_LOOP_GAIN_TRAMP,0,&BSC_LOOP_GAIN_CALC);

bsc_loop_gain[0] = BSC_LOOP_GAIN_CALC * bsc_mtrx[1][0];
bsc_loop_gain[1] = BSC_LOOP_GAIN_CALC * bsc_mtrx[1][1];
bsc_loop_gain[2] = BSC_LOOP_GAIN_CALC * bsc_mtrx[1][2];
bsc_loop_gain[3] = BSC_LOOP_GAIN_CALC * bsc_mtrx[1][3];
bsc_loop_gain[4] = BSC_LOOP_GAIN_CALC * bsc_mtrx[1][4];
bsc_loop_gain[5] = BSC_LOOP_GAIN_CALC * bsc_mtrx[1][5];
bsc_loop_gain[6] = BSC_LOOP_GAIN_CALC * bsc_mtrx[1][6];
bsc_loop_gain[7] = BSC_LOOP_GAIN_CALC * bsc_mtrx[1][7];

// MULTI_SW
bsc_loop_sw[0] = bsc_loop_gain[0];
bsc_loop_sw[1] = bsc_loop_gain[1];
bsc_loop_sw[2] = bsc_loop_gain[2];
bsc_loop_sw[3] = bsc_loop_gain[3];
bsc_loop_sw[4] = bsc_loop_gain[4];
bsc_loop_sw[5] = bsc_loop_gain[5];
bsc_loop_sw[6] = bsc_loop_gain[6];
bsc_loop_sw[7] = bsc_loop_gain[7];
if(pLocalEpics->sei.BSC_LOOP_SW == 0)
{
	for(ii=0;ii< 16;ii++) bsc_loop_sw[ii] = 0.0;
}


// FILTER MODULE
bsc_tiltcorr_v1 = filterModuleD(dspPtr,dspCoeff,BSC_TILTCORR_V1,bsc_loop_sw[0],0);

// FILTER MODULE
bsc_tiltcorr_v2 = filterModuleD(dspPtr,dspCoeff,BSC_TILTCORR_V2,bsc_loop_sw[1],0);

// FILTER MODULE
bsc_tiltcorr_v3 = filterModuleD(dspPtr,dspCoeff,BSC_TILTCORR_V3,bsc_loop_sw[2],0);

// FILTER MODULE
bsc_tiltcorr_v4 = filterModuleD(dspPtr,dspCoeff,BSC_TILTCORR_V4,bsc_loop_sw[3],0);

// FILTER MODULE
bsc_tiltcorr_h1 = filterModuleD(dspPtr,dspCoeff,BSC_TILTCORR_H1,bsc_loop_sw[4],0);

// FILTER MODULE
bsc_tiltcorr_h2 = filterModuleD(dspPtr,dspCoeff,BSC_TILTCORR_H2,bsc_loop_sw[5],0);

// FILTER MODULE
bsc_tiltcorr_h3 = filterModuleD(dspPtr,dspCoeff,BSC_TILTCORR_H3,bsc_loop_sw[6],0);

// FILTER MODULE
bsc_tiltcorr_h4 = filterModuleD(dspPtr,dspCoeff,BSC_TILTCORR_H4,bsc_loop_sw[7],0);

// PRODUCT
pLocalEpics->sei.BSC_TILTCORR_GAIN_RMON = 
	gainRamp(pLocalEpics->sei.BSC_TILTCORR_GAIN,pLocalEpics->sei.BSC_TILTCORR_GAIN_TRAMP,0,&BSC_TILTCORR_GAIN_CALC);

bsc_tiltcorr_gain[0] = BSC_TILTCORR_GAIN_CALC * bsc_tiltcorr_v1;
bsc_tiltcorr_gain[1] = BSC_TILTCORR_GAIN_CALC * bsc_tiltcorr_v2;
bsc_tiltcorr_gain[2] = BSC_TILTCORR_GAIN_CALC * bsc_tiltcorr_v3;
bsc_tiltcorr_gain[3] = BSC_TILTCORR_GAIN_CALC * bsc_tiltcorr_v4;
bsc_tiltcorr_gain[4] = BSC_TILTCORR_GAIN_CALC * bsc_tiltcorr_h1;
bsc_tiltcorr_gain[5] = BSC_TILTCORR_GAIN_CALC * bsc_tiltcorr_h2;
bsc_tiltcorr_gain[6] = BSC_TILTCORR_GAIN_CALC * bsc_tiltcorr_h3;
bsc_tiltcorr_gain[7] = BSC_TILTCORR_GAIN_CALC * bsc_tiltcorr_h4;

// MULTI_SW
bsc_tiltcorr_sw[0] = bsc_tiltcorr_gain[0];
bsc_tiltcorr_sw[1] = bsc_tiltcorr_gain[1];
bsc_tiltcorr_sw[2] = bsc_tiltcorr_gain[2];
bsc_tiltcorr_sw[3] = bsc_tiltcorr_gain[3];
bsc_tiltcorr_sw[4] = bsc_tiltcorr_gain[4];
bsc_tiltcorr_sw[5] = bsc_tiltcorr_gain[5];
bsc_tiltcorr_sw[6] = bsc_tiltcorr_gain[6];
bsc_tiltcorr_sw[7] = bsc_tiltcorr_gain[7];
if(pLocalEpics->sei.BSC_TILTCORR_SW == 0)
{
	for(ii=0;ii< 8;ii++) bsc_tiltcorr_sw[ii] = 0.0;
}



//End of subsystem **************************************************



//Start of subsystem **************************************************

// SUM
bsc_act_sum = bsc_loop_sw[0] + bsc_dc_bias_sw[0];

// SUM
bsc_act_sum1 = bsc_loop_sw[1] + bsc_dc_bias_sw[1];

// SUM
bsc_act_sum2 = bsc_loop_sw[2] + bsc_dc_bias_sw[2];

// SUM
bsc_act_sum3 = bsc_loop_sw[3] + bsc_dc_bias_sw[3];

// SUM
bsc_act_sum4 = bsc_loop_sw[4] + bsc_dc_bias_sw[4];

// SUM
bsc_act_sum5 = bsc_loop_sw[5] + bsc_dc_bias_sw[5];

// SUM
bsc_act_sum6 = bsc_loop_sw[6] + bsc_dc_bias_sw[6];

// SUM
bsc_act_sum7 = bsc_loop_sw[7] + bsc_dc_bias_sw[7];

// FILTER MODULE
bsc_act_v1 = filterModuleD(dspPtr,dspCoeff,BSC_ACT_V1,bsc_act_sum,0);

// FILTER MODULE
bsc_act_v2 = filterModuleD(dspPtr,dspCoeff,BSC_ACT_V2,bsc_act_sum1,0);

// FILTER MODULE
bsc_act_v3 = filterModuleD(dspPtr,dspCoeff,BSC_ACT_V3,bsc_act_sum2,0);

// FILTER MODULE
bsc_act_v4 = filterModuleD(dspPtr,dspCoeff,BSC_ACT_V4,bsc_act_sum3,0);

// FILTER MODULE
bsc_act_h1 = filterModuleD(dspPtr,dspCoeff,BSC_ACT_H1,bsc_act_sum4,0);

// FILTER MODULE
bsc_act_h2 = filterModuleD(dspPtr,dspCoeff,BSC_ACT_H2,bsc_act_sum5,0);

// FILTER MODULE
bsc_act_h3 = filterModuleD(dspPtr,dspCoeff,BSC_ACT_H3,bsc_act_sum6,0);

// FILTER MODULE
bsc_act_h4 = filterModuleD(dspPtr,dspCoeff,BSC_ACT_H4,bsc_act_sum7,0);

// PRODUCT
pLocalEpics->sei.BSC_ACT_GAIN_RMON = 
	gainRamp(pLocalEpics->sei.BSC_ACT_GAIN,pLocalEpics->sei.BSC_ACT_GAIN_TRAMP,0,&BSC_ACT_GAIN_CALC);

bsc_act_gain[0] = BSC_ACT_GAIN_CALC * bsc_act_v1;
bsc_act_gain[1] = BSC_ACT_GAIN_CALC * bsc_act_v2;
bsc_act_gain[2] = BSC_ACT_GAIN_CALC * bsc_act_v3;
bsc_act_gain[3] = BSC_ACT_GAIN_CALC * bsc_act_v4;
bsc_act_gain[4] = BSC_ACT_GAIN_CALC * bsc_act_h1;
bsc_act_gain[5] = BSC_ACT_GAIN_CALC * bsc_act_h2;
bsc_act_gain[6] = BSC_ACT_GAIN_CALC * bsc_act_h3;
bsc_act_gain[7] = BSC_ACT_GAIN_CALC * bsc_act_h4;

// MULTI_SW
bsc_act_sw[0] = bsc_act_gain[0];
bsc_act_sw[1] = bsc_act_gain[1];
bsc_act_sw[2] = bsc_act_gain[2];
bsc_act_sw[3] = bsc_act_gain[3];
bsc_act_sw[4] = bsc_act_gain[4];
bsc_act_sw[5] = bsc_act_gain[5];
bsc_act_sw[6] = bsc_act_gain[6];
bsc_act_sw[7] = bsc_act_gain[7];
if(pLocalEpics->sei.BSC_ACT_SW == 0)
{
	for(ii=0;ii< 8;ii++) bsc_act_sw[ii] = 0.0;
}


// EPICS_OUTPUT
pLocalEpics->sei.BSC_DAC_OUTPUT_H1 = bsc_act_sw[4];

// EPICS_OUTPUT
pLocalEpics->sei.BSC_DAC_OUTPUT_H2 = bsc_act_sw[5];

// EPICS_OUTPUT
pLocalEpics->sei.BSC_DAC_OUTPUT_H3 = bsc_act_sw[6];

// EPICS_OUTPUT
pLocalEpics->sei.BSC_DAC_OUTPUT_H4 = bsc_act_sw[7];

// EPICS_OUTPUT
pLocalEpics->sei.BSC_DAC_OUTPUT_V1 = bsc_act_sw[0];

// EPICS_OUTPUT
pLocalEpics->sei.BSC_DAC_OUTPUT_V2 = bsc_act_sw[1];

// EPICS_OUTPUT
pLocalEpics->sei.BSC_DAC_OUTPUT_V3 = bsc_act_sw[2];

// EPICS_OUTPUT
pLocalEpics->sei.BSC_DAC_OUTPUT_V4 = bsc_act_sw[3];

// DELAY
delay1 = bsc_tiltcorr_sw[0];

// DELAY
delay2 = bsc_tiltcorr_sw[1];

// DELAY
delay3 = bsc_tiltcorr_sw[2];

// DELAY
delay4 = bsc_tiltcorr_sw[3];

// DELAY
delay5 = bsc_tiltcorr_sw[4];

// DELAY
delay6 = bsc_tiltcorr_sw[5];

// DELAY
delay7 = bsc_tiltcorr_sw[6];

// DELAY
delay8 = bsc_tiltcorr_sw[7];

}

