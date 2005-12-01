void feCode(double dWord[][32],int dacOut[][16],FILT_MOD *dspPtr,COEF *dspCoeff,CDS_EPICS *pLocalEpics)
{
  double fmIn;
  double m0SenOut[6];
  double m0DofOut[6];
  double m0Out[6];
  double r0SenOut[6];
  double r0DofOut[6];
  double r0Out[6];
  double l1SenOut[6];
  double l1DofOut[6];
  double l1Out[6];
  double l2SenOut[6];
  double l2DofOut[6];
  double l2Out[6];
  double l3DofOut[6];
  double l3Out[6];

  int ii,kk;

        // Do M0 input filtering
        for(ii=0;ii<6;ii++)
                m0SenOut[ii] = filterModuleD(dspPtr,dspCoeff,ii,dWord[0][ii],0);

        // Do M0 input matrix and DOF filtering
        for(ii=0;ii<6;ii++)
        {
                kk = ii + 6;
                fmIn =  m0SenOut[0] * pLocalEpics->epicsInput.m0InputMatrix[ii][0] +
                        m0SenOut[1] * pLocalEpics->epicsInput.m0InputMatrix[ii][1] +
                        m0SenOut[2] * pLocalEpics->epicsInput.m0InputMatrix[ii][2] +
                        m0SenOut[3] * pLocalEpics->epicsInput.m0InputMatrix[ii][3] +
                        m0SenOut[4] * pLocalEpics->epicsInput.m0InputMatrix[ii][4] +
                        m0SenOut[5] * pLocalEpics->epicsInput.m0InputMatrix[ii][5];
                m0DofOut[ii] = filterModuleD(dspPtr,dspCoeff,kk,fmIn,0);
        }

        // Do M0 output matrix and Output filtering
        for(ii=0;ii<6;ii++)
        {
                kk = ii + 12;
                fmIn =
                        m0DofOut[0] * pLocalEpics->epicsInput.m0OutputMatrix[ii][0] +
                        m0DofOut[1] * pLocalEpics->epicsInput.m0OutputMatrix[ii][1] +
                        m0DofOut[2] * pLocalEpics->epicsInput.m0OutputMatrix[ii][2] +
                        m0DofOut[3] * pLocalEpics->epicsInput.m0OutputMatrix[ii][3] +
                        m0DofOut[4] * pLocalEpics->epicsInput.m0OutputMatrix[ii][4] +
                        m0DofOut[5] * pLocalEpics->epicsInput.m0OutputMatrix[ii][5];
                m0Out[ii] = filterModuleD(dspPtr,dspCoeff,kk,fmIn,0);
                dacOut[0][ii] = (int)m0Out[ii];
        }

        // Do R0 input filtering
        for(ii=0;ii<6;ii++)
        {
                kk = ii + 18;
                r0SenOut[ii] = filterModuleD(dspPtr,dspCoeff,kk,dWord[0][ii+6],0);
        }


        // Do R0 input matrix and DOF filtering
        for(ii=0;ii<6;ii++)
        {
                kk = ii + 24;
                fmIn =  r0SenOut[0] * pLocalEpics->epicsInput.r0InputMatrix[ii][0] +
                        r0SenOut[1] * pLocalEpics->epicsInput.r0InputMatrix[ii][1] +
                        r0SenOut[2] * pLocalEpics->epicsInput.r0InputMatrix[ii][2] +
                        r0SenOut[3] * pLocalEpics->epicsInput.r0InputMatrix[ii][3] +
                        r0SenOut[4] * pLocalEpics->epicsInput.r0InputMatrix[ii][4] +
                        r0SenOut[5] * pLocalEpics->epicsInput.r0InputMatrix[ii][5];
                r0DofOut[ii] = filterModuleD(dspPtr,dspCoeff,kk,fmIn,0);
        }

        // Do R0 output matrix and Output filtering
        for(ii=0;ii<6;ii++)
        {
                kk = ii + 30;
                fmIn =
                        r0DofOut[0] * pLocalEpics->epicsInput.r0OutputMatrix[ii][0] +
                        r0DofOut[1] * pLocalEpics->epicsInput.r0OutputMatrix[ii][1] +
                        r0DofOut[2] * pLocalEpics->epicsInput.r0OutputMatrix[ii][2] +
                        r0DofOut[3] * pLocalEpics->epicsInput.r0OutputMatrix[ii][3] +
                        r0DofOut[4] * pLocalEpics->epicsInput.r0OutputMatrix[ii][4] +
                        r0DofOut[5] * pLocalEpics->epicsInput.r0OutputMatrix[ii][5];
                r0Out[ii] = filterModuleD(dspPtr,dspCoeff,kk,fmIn,0);
                dacOut[0][ii+6] = (int)r0Out[ii];
        }

        // Do L1 input filtering
        for(ii=0;ii<4;ii++)
        {
                kk = ii + FILT_L1_ULSEN;
                l1SenOut[ii] = filterModuleD(dspPtr,dspCoeff,kk,dWord[0][ii+12],0);
        }

        // Do L1 input matrix and DOF filtering
        for(ii=0;ii<3;ii++)
        {
                kk = ii + FILT_L1_POS;
                fmIn =  l1SenOut[0] * pLocalEpics->epicsInput.l1InputMatrix[0][ii] +
                        l1SenOut[1] * pLocalEpics->epicsInput.l1InputMatrix[1][ii] +
                        l1SenOut[2] * pLocalEpics->epicsInput.l1InputMatrix[2][ii] +
                        l1SenOut[3] * pLocalEpics->epicsInput.l1InputMatrix[3][ii];
                l1DofOut[ii] = filterModuleD(dspPtr,dspCoeff,kk,fmIn,0);
        }

        // Place for LSC, ASCP, ASCY
        for(ii=0;ii<3;ii++)
        {
                kk = ii + FILT_L1_LSC;
                l1DofOut[ii] += filterModuleD(dspPtr,dspCoeff,kk,0.0,0);
        }

        // L1 Output Filter Matrix
        l1Out[0] =
                filterModuleD(dspPtr,dspCoeff,FILT_L1_ULPOS,l1DofOut[0],0) +
                filterModuleD(dspPtr,dspCoeff,FILT_L1_ULPIT,l1DofOut[1],0) +
                filterModuleD(dspPtr,dspCoeff,FILT_L1_ULYAW,l1DofOut[2],0);

        l1Out[1] =
                filterModuleD(dspPtr,dspCoeff,FILT_L1_LLPOS,l1DofOut[0],0) +
                filterModuleD(dspPtr,dspCoeff,FILT_L1_LLPIT,l1DofOut[1],0) +
                filterModuleD(dspPtr,dspCoeff,FILT_L1_LLYAW,l1DofOut[2],0);

        l1Out[2] =
                filterModuleD(dspPtr,dspCoeff,FILT_L1_URPOS,l1DofOut[0],0) +
                filterModuleD(dspPtr,dspCoeff,FILT_L1_URPIT,l1DofOut[1],0) +
                filterModuleD(dspPtr,dspCoeff,FILT_L1_URYAW,l1DofOut[2],0);

        l1Out[3] =
                filterModuleD(dspPtr,dspCoeff,FILT_L1_LRPOS,l1DofOut[0],0) +
                filterModuleD(dspPtr,dspCoeff,FILT_L1_LRPIT,l1DofOut[1],0) +
                filterModuleD(dspPtr,dspCoeff,FILT_L1_LRYAW,l1DofOut[2],0);

        // L1 Output Filters
        for(ii=0;ii<4;ii++)
        {
                kk = ii + FILT_L1_ULOUT;
                l1Out[ii] += filterModuleD(dspPtr,dspCoeff,kk,l1Out[ii],0);
                dacOut[1][ii] = (int)l1Out[ii];
        }

        // Do L2 input filtering
        for(ii=0;ii<4;ii++)
        {
                kk = ii + FILT_L2_ULSEN;
                l2SenOut[ii] = filterModuleD(dspPtr,dspCoeff,kk,dWord[0][ii+16],0);
        }

        // Do L2 input matrix and DOF filtering
        for(ii=0;ii<3;ii++)
        {
                kk = ii + FILT_L2_POS;
                fmIn =  l2SenOut[0] * pLocalEpics->epicsInput.l2InputMatrix[0][ii] +
                        l2SenOut[1] * pLocalEpics->epicsInput.l2InputMatrix[1][ii] +
                        l2SenOut[2] * pLocalEpics->epicsInput.l2InputMatrix[2][ii] +
                        l2SenOut[3] * pLocalEpics->epicsInput.l2InputMatrix[3][ii];
                l2DofOut[ii] = filterModuleD(dspPtr,dspCoeff,kk,fmIn,0);
        }

        // Place for LSC, ASCP, ASCY
        for(ii=0;ii<3;ii++)
        {
                kk = ii + FILT_L2_LSC;
                l2DofOut[ii] += filterModuleD(dspPtr,dspCoeff,kk,0.0,0);
        }

        // L2 Output Filter Matrix
        l2Out[0] =
                filterModuleD(dspPtr,dspCoeff,FILT_L2_ULPOS,l2DofOut[0],0) +
                filterModuleD(dspPtr,dspCoeff,FILT_L2_ULPIT,l2DofOut[1],0) +
                filterModuleD(dspPtr,dspCoeff,FILT_L2_ULYAW,l2DofOut[2],0);

        l2Out[1] =
                filterModuleD(dspPtr,dspCoeff,FILT_L2_LLPOS,l2DofOut[0],0) +
                filterModuleD(dspPtr,dspCoeff,FILT_L2_LLPIT,l2DofOut[1],0) +
                filterModuleD(dspPtr,dspCoeff,FILT_L2_LLYAW,l2DofOut[2],0);

        l2Out[2] =
                filterModuleD(dspPtr,dspCoeff,FILT_L2_URPOS,l2DofOut[0],0) +
                filterModuleD(dspPtr,dspCoeff,FILT_L2_URPIT,l2DofOut[1],0) +
                filterModuleD(dspPtr,dspCoeff,FILT_L2_URYAW,l2DofOut[2],0);

        l2Out[3] =
                filterModuleD(dspPtr,dspCoeff,FILT_L2_LRPOS,l2DofOut[0],0) +
                filterModuleD(dspPtr,dspCoeff,FILT_L2_LRPIT,l2DofOut[1],0) +
                filterModuleD(dspPtr,dspCoeff,FILT_L2_LRYAW,l2DofOut[2],0);

        // L2 Output Filters
        for(ii=0;ii<4;ii++)
        {
                kk = ii + FILT_L2_ULOUT;
                l2Out[ii] += filterModuleD(dspPtr,dspCoeff,kk,l2Out[ii],0);
                dacOut[1][ii+4] = (int)l2Out[ii];
        }

        // Place for L3 LSC, ASCP, ASCY
        for(ii=0;ii<3;ii++)
        {
                kk = ii + FILT_L3_LSC;
                l3DofOut[ii] = filterModuleD(dspPtr,dspCoeff,kk,0.0,0);
        }

        // Do L3 output matrix and DOF filtering
        for(ii=0;ii<5;ii++)
        {
                kk = ii + FILT_L3_ULOUT;
                fmIn =
                        l3DofOut[0] * pLocalEpics->epicsInput.l3OutputMatrix[0][ii] +
                        l3DofOut[1] * pLocalEpics->epicsInput.l3OutputMatrix[1][ii] +
                        l3DofOut[2] * pLocalEpics->epicsInput.l3OutputMatrix[2][ii];
                l3Out[ii] = filterModuleD(dspPtr,dspCoeff,kk,fmIn,0);
                dacOut[1][ii+8] = (int)l3Out[ii];
        }

}
