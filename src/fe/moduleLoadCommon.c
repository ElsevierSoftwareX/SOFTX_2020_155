///     @file moduleLoadCommon.c
///     @brief File contains common routines for moduleLoadIop.c and moduleLoadApp.c.`

int print_io_info(CDS_HARDWARE *cdsp) {
  int ii,jj,kk;
	printf("THIS IS A TEST \n");
#ifndef USER_SPACE
	printf("startup time is %ld\n", current_time());
	printf("cpu clock %u\n",cpu_khz);
#endif
	printf("EPICSM at 0x%x\n", _epics_shm);
	printf("IPC    at 0x%x\n",_ipc_shm);
	printf("IOMEM  at 0x%x size 0x%x\n",(_ipc_shm + 0x4000),sizeof(IO_MEM_DATA));
	printf("DAQSM at 0x%x\n",_daq_shm);
	printf("configured to use %d cards\n", cdsp->cards);
	kk = 0;
	printf("***************************************************************************\n");
	printf("%d ADC cards found\n",cdsp->adcCount);
	for(ii=0;ii<cdsp->adcCount;ii++)
        {
		kk ++;
		if(cdsp->adcType[ii] == GSC_18AISS6C)
                {
                        printf("\tADC %d is a GSC_18AISS6C module\n",ii);
                        printf("\t\tChannels = 6 \n");
                        printf("\t\tFirmware Rev = %d \n\n",(cdsp->adcConfig[ii] & 0xfff));
                }
                if(cdsp->adcType[ii] == GSC_16AI64SSA)
                {
                        printf("\tADC %d is a GSC_16AI64SSA module\n",ii);
                        if((cdsp->adcConfig[ii] & 0x10000) > 0) jj = 32;
                        else jj = 64;
                        printf("\t\tChannels = %d \n",jj);
                        printf("\t\tFirmware Rev = %d \n\n",(cdsp->adcConfig[ii] & 0xfff));
                }
	}
	printf("***************************************************************************\n");
	printf("%d DAC cards found\n",cdsp->dacCount);
        for(ii=0;ii<cdsp->dacCount;ii++)
        {
		kk ++;
                if(cdsp->dacType[ii] == GSC_18AO8)
                {
                        printf("\tDAC %d is a GSC_18AO8 module\n",ii);
                }
                if(cdsp->dacType[ii] == GSC_16AO16)
                {
                        printf("\tDAC %d is a GSC_16AO16 module\n",ii);
                        if((cdsp->dacConfig[ii] & 0x10000) == 0x10000) jj = 8;
                        if((cdsp->dacConfig[ii] & 0x20000) == 0x20000) jj = 12;
                        if((cdsp->dacConfig[ii] & 0x30000) == 0x30000) jj = 16;
                        printf("\t\tChannels = %d \n",jj);
                        if((cdsp->dacConfig[ii] & 0xC0000) == 0x0000)
                        {
                                printf("\t\tFilters = None\n");
                        }
                        if((cdsp->dacConfig[ii] & 0xC0000) == 0x40000)
                        {
                                printf("\t\tFilters = 10kHz\n");
                        }
                        if((cdsp->dacConfig[ii] & 0xC0000) == 0x80000)
                        {
                                printf("\t\tFilters = 100kHz\n");
                        }
                        if((cdsp->dacConfig[ii] & 0x100000) == 0x100000)
                        {
                                printf("\t\tOutput Type = Differential\n");
                        }
                        printf("\t\tFirmware Rev = %d \n\n",(cdsp->dacConfig[ii] & 0xfff));
                }
	}
	kk += cdsp->dioCount;
	printf("***************************************************************************\n");
        printf("%d DIO cards found\n",cdsp->dioCount);
        printf("***************************************************************************\n");
        printf("%d IIRO-8 Isolated DIO cards found\n",cdsp->iiroDioCount);
        printf("***************************************************************************\n");
        printf("%d IIRO-16 Isolated DIO cards found\n",cdsp->iiroDio1Count);
        printf("***************************************************************************\n");
        printf("%d Contec 32ch PCIe DO cards found\n",cdsp->cDo32lCount);
        printf("%d Contec PCIe DIO1616 cards found\n",cdsp->cDio1616lCount);
        printf("%d Contec PCIe DIO6464 cards found\n",cdsp->cDio6464lCount);
        printf("%d DO cards found\n",cdsp->doCount);
        printf("Total of %d I/O modules found and mapped\n",kk);
        printf("***************************************************************************\n");
	printf("%d RFM cards found\n",cdsp->rfmCount);
	for(ii=0;ii<cdsp->rfmCount;ii++)
        {
                 printf("\tRFM %d is a VMIC_%x module with Node ID %d\n", ii, cdsp->rfmType[ii], cdsp->rfmConfig[ii]);
		printf("address is 0x%lx\n",cdsp->pci_rfm[ii]);
	}
        printf("***************************************************************************\n");
  	if (cdsp->gps) {
	printf("IRIG-B card found %d\n",cdsp->gpsType);
        printf("***************************************************************************\n");
  	}

}
