///	@file moduleLoad.c
///	@brief File contains startup routines for real-time code.

#include <asm/uaccess.h>
#include <linux/ctype.h>
#include <linux/spinlock_types.h>

// These externs and "16" need to go to a header file (mbuf.h)
extern void *kmalloc_area[16];
extern int mbuf_allocate_area(char *name, int size, struct file *file);
extern void *fe_start(void *arg);
extern int run_on_timer;
extern char daqArea[2*DAQ_DCU_SIZE];           // Space allocation for daqLib buffers


// MAIN routine: Code starting point ****************************************************************
extern int need_to_load_IOP_first;

extern void set_fe_code_idle(void *(*ptr)(void *), unsigned int cpu);
extern void msleep(unsigned int);

/// Startup function for initialization of kernel module.
int init_module (void)
{
 	int status;
	int ii,jj,kk;		/// @param ii,jj,kk default loop counters
	char fname[128];	/// @param fname[128] Name of shared mem area to allocate for DAQ data
	int cards;		/// @param cards Number of PCIe cards found on bus
	int adcCnt;		/// @param adcCnt Number of ADC cards found by slave model.
	int dacCnt;		/// @param dacCnt Number of 16bit DAC cards found by slave model.
        int dac18Cnt;		/// @param dac18Cnt Number of 18bit DAC cards found by slave model.
	int doCnt;		/// @param doCnt Total number of digital I/O cards found by slave model.
	int do32Cnt;		/// @param do32Cnt Total number of Contec 32 bit DIO cards found by slave model.
	int doIIRO16Cnt;	/// @param doIIRO16Cnt Total number of Acces I/O 16 bit relay cards found by slave model.
	int doIIRO8Cnt;		/// @param doIIRO8Cnt Total number of Acces I/O 8 bit relay cards found by slave model.
	int cdo64Cnt;		/// @param cdo64Cnt Total number of Contec 6464 DIO card 32bit output sections mapped by slave model.
	int cdi64Cnt;		/// @param cdo64Cnt Total number of Contec 6464 DIO card 32bit input sections mapped by slave model.
	int ret;		/// @param ret Return value from various Malloc calls to allocate memory.
	int cnt;
	extern int cpu_down(unsigned int);	/// @param cpu_down CPU shutdown call.
	extern int is_cpu_taken_by_rcg_model(unsigned int cpu);	/// @param is_cpu_taken_by_rcg_model Check to verify CPU availability for shutdown.

	kk = 0;
#ifdef SPECIFIC_CPU
#define CPUID SPECIFIC_CPU
#else 
#define CPUID 1 
#endif

#ifndef NO_CPU_SHUTDOWN
	// See if our CPU core is free
        if (is_cpu_taken_by_rcg_model(CPUID)) {
		printk(KERN_ALERT "Error: CPU %d already taken\n", CPUID);
		return -1;
	}
#endif

	need_to_load_IOP_first = 0;

#ifdef DOLPHIN_TEST
	status = init_dolphin(1);
	if (status != 0) {
		return -1;
	}
#endif


	printf("startup time is %ld\n", current_time());
	jj = 0;
	printf("cpu clock %u\n",cpu_khz);


        ret =  mbuf_allocate_area(SYSTEM_NAME_STRING_LOWER, 64*1024*1024, 0);
        if (ret < 0) {
                printf("mbuf_allocate_area() failed; ret = %d\n", ret);
                return -1;
        }
        _epics_shm = (unsigned char *)(kmalloc_area[ret]);
        printf("EPICSM at 0x%x\n", _epics_shm);
        ret =  mbuf_allocate_area("ipc", 4*1024*1024, 0);
        if (ret < 0) {
                printf("mbuf_allocate_area(ipc) failed; ret = %d\n", ret);
                return -1;
        }
        _ipc_shm = (unsigned char *)(kmalloc_area[ret]);

	printf("IPC    at 0x%x\n",_ipc_shm);
  	ioMemData = (IO_MEM_DATA *)(_ipc_shm+ 0x4000);
	printf("IOMEM  at 0x%x size 0x%x\n",(_ipc_shm + 0x4000),sizeof(IO_MEM_DATA));


// If DAQ is via shared memory (Framebuilder code running on same machine or MX networking is used)
// attach DAQ shared memory location.
        sprintf(fname, "%s_daq", SYSTEM_NAME_STRING_LOWER);
        ret =  mbuf_allocate_area(fname, 64*1024*1024, 0);
        if (ret < 0) {
                printf("mbuf_allocate_area() failed; ret = %d\n", ret);
                return -1;
        }
        _daq_shm = (unsigned char *)(kmalloc_area[ret]);
        // printf("Allocated daq shmem; set at 0x%x\n", _daq_shm);
	printf("DAQSM at 0x%x\n",_daq_shm);
 	daqPtr = (struct rmIpcStr *) _daq_shm;

// Open new GDS TP data shared memory in support of ZMQ
        sprintf(fname, "%s_gds", SYSTEM_NAME_STRING_LOWER);
        ret =  mbuf_allocate_area(fname, 16*1024*1024, 0);
        if (ret < 0) {
                printf("mbuf_allocate_area() failed; ret = %d\n", ret);
                return -1;
        }
        _gds_shm = (unsigned char *)(kmalloc_area[ret]);
	printf("GDSSM at 0x%x\n",_gds_shm);

	// Find and initialize all PCI I/O modules *******************************************************
	  // Following I/O card info is from feCode
	  cards = sizeof(cards_used)/sizeof(cards_used[0]);
	  printf("configured to use %d cards\n", cards);
	  cdsPciModules.cards = cards;
	  cdsPciModules.cards_used = cards_used;
          //return -1;
	printf("Initializing PCI Modules\n");
	cdsPciModules.adcCount = 0;
	cdsPciModules.dacCount = 0;
	cdsPciModules.dioCount = 0;
	cdsPciModules.doCount = 0;

// If running as a slave process, I/O card information is via ipc shared memory
	printf("%d PCI cards found\n",ioMemData->totalCards);
	status = 0;
	adcCnt = 0;
	dacCnt = 0;
	dac18Cnt = 0;
	doCnt = 0;
	do32Cnt = 0;
	cdo64Cnt = 0;
	cdi64Cnt = 0;
	doIIRO16Cnt = 0;
	doIIRO8Cnt = 0;

	// Have to search thru all cards and find desired instance for application
	// Master will map ADC cards first, then DAC and finally DIO
	for(ii=0;ii<ioMemData->totalCards;ii++)
	{
		/*
		printf("Model %d = %d\n",ii,ioMemData->model[ii]);
		*/
		for(jj=0;jj<cards;jj++)
		{
			/*
			printf("Model %d = %d, type = %d, instance = %d, dacCnt = %d \n",
				ii,ioMemData->model[ii],
				cdsPciModules.cards_used[jj].type,
				cdsPciModules.cards_used[jj].instance,
 				dacCnt);
				*/
		   switch(ioMemData->model[ii])
		   {
			case GSC_16AI64SSA:
				if((cdsPciModules.cards_used[jj].type == GSC_16AI64SSA) && 
					(cdsPciModules.cards_used[jj].instance == adcCnt))
				{
					printf("Found ADC at %d %d\n",jj,ioMemData->ipc[ii]);
					kk = cdsPciModules.adcCount;
					cdsPciModules.adcType[kk] = GSC_16AI64SSA;
					cdsPciModules.adcConfig[kk] = ioMemData->ipc[ii];
					cdsPciModules.adcCount ++;
					status ++;
				}
				break;
			case GSC_16AO16:
				if((cdsPciModules.cards_used[jj].type == GSC_16AO16) && 
					(cdsPciModules.cards_used[jj].instance == dacCnt))
				{
					printf("Found DAC at %d %d\n",jj,ioMemData->ipc[ii]);
					kk = cdsPciModules.dacCount;
					cdsPciModules.dacType[kk] = GSC_16AO16;
					cdsPciModules.dacConfig[kk] = ioMemData->ipc[ii];
	   				cdsPciModules.pci_dac[kk] = (long)(ioMemData->iodata[ii]);
					cdsPciModules.dacCount ++;
					status ++;
				}
				break;
			case GSC_18AO8:
				if((cdsPciModules.cards_used[jj].type == GSC_18AO8) && 
					(cdsPciModules.cards_used[jj].instance == dac18Cnt))
				{
					printf("Found DAC at %d %d\n",jj,ioMemData->ipc[ii]);
					kk = cdsPciModules.dacCount;
					cdsPciModules.dacType[kk] = GSC_18AO8;
					cdsPciModules.dacConfig[kk] = ioMemData->ipc[ii];
	   				cdsPciModules.pci_dac[kk] = (long)(ioMemData->iodata[ii]);
					cdsPciModules.dacCount ++;
					status ++;
				}
				break;
			case CON_6464DIO:
				if((cdsPciModules.cards_used[jj].type == CON_6464DIO) && 
					(cdsPciModules.cards_used[jj].instance == doCnt))
				{
					kk = cdsPciModules.doCount;
					printf("Found 6464 DIO CONTEC at %d 0x%x\n",jj,ioMemData->ipc[ii]);
					cdsPciModules.doType[kk] = ioMemData->model[ii];
					cdsPciModules.pci_do[kk] = ioMemData->ipc[ii];
					cdsPciModules.doCount ++;
					cdsPciModules.cDio6464lCount ++;
					cdsPciModules.pci_do[kk] = ioMemData->ipc[ii];
					cdsPciModules.doInstance[kk] = doCnt;
					status += 2;
				}
				if((cdsPciModules.cards_used[jj].type == CDO64) && 
					(cdsPciModules.cards_used[jj].instance == doCnt))
				{
					kk = cdsPciModules.doCount;
					cdsPciModules.doType[kk] = CDO64;
					cdsPciModules.pci_do[kk] = ioMemData->ipc[ii];
					cdsPciModules.doCount ++;
					cdsPciModules.cDio6464lCount ++;
					cdsPciModules.doInstance[kk] = doCnt;
					printf("Found 6464 DOUT CONTEC at %d 0x%x index %d \n",jj,ioMemData->ipc[ii],doCnt);
					cdo64Cnt ++;
					status ++;
				}
				if((cdsPciModules.cards_used[jj].type == CDI64) && 
					(cdsPciModules.cards_used[jj].instance == doCnt))
				{
					kk = cdsPciModules.doCount;
					// printf("Found 6464 DIN CONTEC at %d 0x%x\n",jj,ioMemData->ipc[ii]);
					cdsPciModules.doType[kk] = CDI64;
					cdsPciModules.pci_do[kk] = ioMemData->ipc[ii];
					cdsPciModules.doInstance[kk] = doCnt;
					cdsPciModules.doCount ++;
					printf("Found 6464 DIN CONTEC at %d 0x%x index %d \n",jj,ioMemData->ipc[ii],doCnt);
					cdsPciModules.cDio6464lCount ++;
					cdi64Cnt ++;
					status ++;
				}
				break;
                        case CON_32DO:
		                if((cdsPciModules.cards_used[jj].type == CON_32DO) &&
		                        (cdsPciModules.cards_used[jj].instance == do32Cnt))
		                {
					kk = cdsPciModules.doCount;
               			      	printf("Found 32 DO CONTEC at %d 0x%x\n",jj,ioMemData->ipc[ii]);
		                      	cdsPciModules.doType[kk] = ioMemData->model[ii];
                                      	cdsPciModules.pci_do[kk] = ioMemData->ipc[ii];
                                      	cdsPciModules.doCount ++; 
                                      	cdsPciModules.cDo32lCount ++; 
					cdsPciModules.doInstance[kk] = do32Cnt;
					status ++;
				 }
				 break;
			case ACS_16DIO:
				if((cdsPciModules.cards_used[jj].type == ACS_16DIO) && 
					(cdsPciModules.cards_used[jj].instance == doIIRO16Cnt))
				{
					kk = cdsPciModules.doCount;
					printf("Found Access IIRO-16 at %d 0x%x\n",jj,ioMemData->ipc[ii]);
					cdsPciModules.doType[kk] = ioMemData->model[ii];
					cdsPciModules.pci_do[kk] = ioMemData->ipc[ii];
					cdsPciModules.doCount ++;
					cdsPciModules.iiroDio1Count ++;
					cdsPciModules.doInstance[kk] = doIIRO16Cnt;
					status ++;
				}
				break;
			case ACS_8DIO:
			       if((cdsPciModules.cards_used[jj].type == ACS_8DIO) &&
			               (cdsPciModules.cards_used[jj].instance == doIIRO8Cnt))
			       {
			               kk = cdsPciModules.doCount;
			               printf("Found Access IIRO-8 at %d 0x%x\n",jj,ioMemData->ipc[ii]);
			               cdsPciModules.doType[kk] = ioMemData->model[ii];
			               cdsPciModules.pci_do[kk] = ioMemData->ipc[ii];
			               cdsPciModules.doCount ++;
			               cdsPciModules.iiroDioCount ++;
			               cdsPciModules.doInstance[kk] = doIIRO8Cnt;
			               status ++;
			       }
		 	       break;
			default:
				break;
		   }
		}
		if(ioMemData->model[ii] == GSC_16AI64SSA) adcCnt ++;
		if(ioMemData->model[ii] == GSC_16AO16) dacCnt ++;
		if(ioMemData->model[ii] == GSC_18AO8) dac18Cnt ++;
		if(ioMemData->model[ii] == CON_6464DIO) doCnt ++;
		if(ioMemData->model[ii] == CON_32DO) do32Cnt ++;
		if(ioMemData->model[ii] == ACS_16DIO) doIIRO16Cnt ++;
		if(ioMemData->model[ii] == ACS_8DIO) doIIRO8Cnt ++;
	}
	// If no ADC cards were found, then SLAVE cannot run
	if(!cdsPciModules.adcCount)
	{
		printf("No ADC cards found - exiting\n");
		return -1;
	}
	// This did not quite work for some reason
	// Need to find a way to handle skipped DAC cards in slaves
	//cdsPciModules.dacCount = ioMemData->dacCount;
	printf("%d PCI cards found \n",status);
	if(status < cards)
	{
		printf(" ERROR **** Did not find correct number of cards! Expected %d and Found %d\n",cards,status);
		cardCountErr = 1;
	}

	// Print out all the I/O information
        printf("***************************************************************************\n");
	printf("%d ADC cards found\n",cdsPciModules.adcCount);
	for(ii=0;ii<cdsPciModules.adcCount;ii++)
        {
                if(cdsPciModules.adcType[ii] == GSC_18AISS6C)
                {
                        printf("\tADC %d is a GSC_18AISS6C module\n",ii);
                        printf("\t\tChannels = 6 \n");
                        printf("\t\tFirmware Rev = %d \n\n",(cdsPciModules.adcConfig[ii] & 0xfff));
                }
                if(cdsPciModules.adcType[ii] == GSC_16AI64SSA)
                {
                        printf("\tADC %d is a GSC_16AI64SSA module\n",ii);
                        if((cdsPciModules.adcConfig[ii] & 0x10000) > 0) jj = 32;
                        else jj = 64;
                        printf("\t\tChannels = %d \n",jj);
                        printf("\t\tFirmware Rev = %d \n\n",(cdsPciModules.adcConfig[ii] & 0xfff));
                }
        }
        printf("***************************************************************************\n");
	printf("%d DAC cards found\n",cdsPciModules.dacCount);
	for(ii=0;ii<cdsPciModules.dacCount;ii++)
        {
                if(cdsPciModules.dacType[ii] == GSC_18AO8)
		{
                        printf("\tDAC %d is a GSC_18AO8 module\n",ii);
		}
                if(cdsPciModules.dacType[ii] == GSC_16AO16)
                {
                        printf("\tDAC %d is a GSC_16AO16 module\n",ii);
                        if((cdsPciModules.dacConfig[ii] & 0x10000) == 0x10000) jj = 8;
                        if((cdsPciModules.dacConfig[ii] & 0x20000) == 0x20000) jj = 12;
                        if((cdsPciModules.dacConfig[ii] & 0x30000) == 0x30000) jj = 16;
                        printf("\t\tChannels = %d \n",jj);
                        if((cdsPciModules.dacConfig[ii] & 0xC0000) == 0x0000)
			{
                        	printf("\t\tFilters = None\n");
			}
                        if((cdsPciModules.dacConfig[ii] & 0xC0000) == 0x40000)
			{
                        	printf("\t\tFilters = 10kHz\n");
			}
                        if((cdsPciModules.dacConfig[ii] & 0xC0000) == 0x80000)
			{
                        	printf("\t\tFilters = 100kHz\n");
			}
                        if((cdsPciModules.dacConfig[ii] & 0x100000) == 0x100000)
			{
                        	printf("\t\tOutput Type = Differential\n");
			}
                        printf("\t\tFirmware Rev = %d \n\n",(cdsPciModules.dacConfig[ii] & 0xfff));
                }
	}
        printf("***************************************************************************\n");
	printf("%d DIO cards found\n",cdsPciModules.dioCount);
        printf("***************************************************************************\n");
	printf("%d IIRO-8 Isolated DIO cards found\n",cdsPciModules.iiroDioCount);
        printf("***************************************************************************\n");
	printf("%d IIRO-16 Isolated DIO cards found\n",cdsPciModules.iiroDio1Count);
        printf("***************************************************************************\n");
	printf("%d Contec 32ch PCIe DO cards found\n",cdsPciModules.cDo32lCount);
	printf("%d Contec PCIe DIO1616 cards found\n",cdsPciModules.cDio1616lCount);
	printf("%d Contec PCIe DIO6464 cards found\n",cdsPciModules.cDio6464lCount);
	printf("%d DO cards found\n",cdsPciModules.doCount);
        printf("***************************************************************************\n");
// Following section maps Reflected Memory, both VMIC hardware style and Dolphin PCIe network style.
// Slave units will perform I/O transactions with RFM directly ie MASTER does not do RFM I/O.
// Master unit only maps the RFM I/O space and passes pointers to SLAVES.

	// Slave gets RFM module count from MASTER.
	cdsPciModules.rfmCount = ioMemData->rfmCount;
	cdsPciModules.dolphinCount = ioMemData->dolphinCount;
	cdsPciModules.dolphinRead[0] = ioMemData->dolphinRead[0];
	cdsPciModules.dolphinWrite[0] = ioMemData->dolphinWrite[0];
	printf("%d RFM cards found\n",cdsPciModules.rfmCount);
	for(ii=0;ii<cdsPciModules.rfmCount;ii++)
        {
                 printf("\tRFM %d is a VMIC_%x module with Node ID %d\n", ii, cdsPciModules.rfmType[ii], cdsPciModules.rfmConfig[ii]);
		cdsPciModules.pci_rfm[ii] = ioMemData->pci_rfm[ii];
		cdsPciModules.pci_rfm_dma[ii] = ioMemData->pci_rfm_dma[ii];
		printf("address is 0x%lx\n",cdsPciModules.pci_rfm[ii]);
	}
	// ioMemData->dolphinCount = 0;
	cdsPciModules.gps = 0;
	cdsPciModules.gpsType = 0;
        printf("***************************************************************************\n");
  	if (cdsPciModules.gps) {
	printf("IRIG-B card found %d\n",cdsPciModules.gpsType);
        printf("***************************************************************************\n");
  	}


	// Code will run on internal timer if no ADC modules are found
	if (cdsPciModules.adcCount == 0) {
		printf("No ADC modules found, running on timer\n");
		run_on_timer = 1;
        	//munmap(_epics_shm, MMAP_SIZE);
        	//close(wfd);
        	//return 0;
	}

	// Initialize buffer for daqLib.c code
	printf("Initializing space for daqLib buffers\n");
	daqBuffer = (long)&daqArea[0];
 

        pLocalEpics = (CDS_EPICS *)&((RFM_FE_COMMS *)_epics_shm)->epicsSpace;
	for (cnt = 0;  cnt < 10 && pLocalEpics->epicsInput.burtRestore == 0; cnt++) {
        	printf("Epics burt restore is %d\n", pLocalEpics->epicsInput.burtRestore);
        	msleep(1000);
	}
	if (cnt == 10) {
		// Cleanup
#ifdef DOLPHIN_TEST
		finish_dolphin();
#endif
		return -1;
	}

        pLocalEpics->epicsInput.vmeReset = 0;

#ifdef NO_CPU_SHUTDOWN
        sthread = kthread_create(fe_start, 0, "fe_start/%d", CPUID);
        if (IS_ERR(sthread)){
                printf("Failed to kthread_create()\n");
                return -1;
        }
        kthread_bind(sthread, CPUID);
        wake_up_process(sthread);
#endif


#ifndef NO_CPU_SHUTDOWN
        set_fe_code_idle(fe_start, CPUID);
        msleep(100);

	cpu_down(CPUID);

	// The code runs on the disabled CPU
#endif
        return 0;
}

void cleanup_module (void) {
	int i;
	int ret;
	extern int __cpuinit cpu_up(unsigned int cpu);


#ifndef NO_CPU_SHUTDOWN
	// Unset the code callback
        set_fe_code_idle(0, CPUID);
#endif

	printk("Setting stop_working_threads to 1\n");
	// Stop the code and wait
#ifdef NO_CPU_SHUTDOWN
	ret = kthread_stop(sthread);
#endif
        stop_working_threads = 1;
        msleep(1000);

#ifdef DOLPHIN_TEST
	finish_dolphin();
#endif

#ifndef NO_CPU_SHUTDOWN

	// Unset the code callback
        set_fe_code_idle(0, CPUID);
	printkl("Will bring back CPU %d\n", CPUID);
        msleep(1000);
	// Bring the CPU back up
        cpu_up(CPUID);
        //msleep(1000);
	printk("Brought the CPU back up\n");
#endif
	printk("Just before returning from cleanup_module for " SYSTEM_NAME_STRING_LOWER "\n");

}

MODULE_DESCRIPTION("Control system");
MODULE_AUTHOR("LIGO");
MODULE_LICENSE("Dual BSD/GPL");
