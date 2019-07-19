///	@file rcguserIop.c
///	@brief File contains startup routines for IOP running in user space.

#include <unistd.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>


// These externs and "16" need to go to a header file (mbuf.h)
extern int fe_start_iop_user();
extern char daqArea[2*DAQ_DCU_SIZE];           // Space allocation for daqLib buffers
extern char *addr;
extern int cycleOffset;

// Scan a double
#if 0
double
simple_strtod(char *start, char **end) {
	int integer;
	if (*start != '.') {
		integer = simple_strtol(start, end, 10);
        	if (*end == start) return 0.0;
		start = *end;
	} else integer = 0;
	if (*start != '.') return integer;
	else {
		start++;
		double frac = simple_strtol(start, end, 10);
        	if (*end == start) return integer;
		int i;
		for (i = 0; i < (*end - start); i++) frac /= 10.0;
		return ((double)integer) + frac;
	}
	// Never reached
}
#endif

void usage()
{
	fprintf(stderr,"Usage: rcgUser -m system_name \n");
	fprintf(stderr, "-h - help\n");
}


/// Startup function for initialization of kernel module.
int main (int argc, char **argv)
{
 	int status;
	int ii,jj,kk,mm;		/// @param ii,jj,kk default loop counters
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
	char *sysname;
	char shm_name[64];
	int c;
	char *modelname;

	cycleOffset = 0;

	while ((c = getopt(argc, argv, "m:t:help")) != EOF) switch(c) {
		case 'm':
			sysname = optarg;
			printf("sysname = %s\n",sysname);
			break;
		case 't':
			cycleOffset = atoi(optarg);
			printf("cycle offset = %d\n",cycleOffset);
			break;
		case 'help':
		default:
			usage();
			exit(1);
	}

	kk = 0;

	jj = 0;
  	int i = 0;
	char *p = strtok (argv[0], "/");
	char *array[5];

    while (p != NULL)
    {
        array[i++] = p;
        p = strtok (NULL, "/");
    }
	modelname = array[i-1];
	sysname = array[i-1];
	
	printf("model name is %s \n",sysname);


	sprintf(shm_name,"%s",sysname);
	findSharedMemory(sysname);
	_epics_shm = (char *)addr;;
        if (_epics_shm < 0) {
                printf("mbuf_allocate_area() failed; ret = %d\n", _epics_shm);
                return -1;
        }
        printf("EPICSM at 0x%lx\n", (long)_epics_shm);

	sprintf(shm_name,"%s","ipc");
	findSharedMemory("ipc");
	_ipc_shm = (char *)addr;
        if (_ipc_shm< 0) {
                printf("mbuf_allocate_area(ipc) failed; ret = %d\n", _ipc_shm);
                return -1;
        }

	printf("IPC    at 0x%lx\n",(long)_ipc_shm);
	ioMemData = (volatile IO_MEM_DATA *)(((char *)_ipc_shm) + 0x4000);
	printf("IOMEM  at 0x%lx size 0x%x\n",(long)ioMemData,sizeof(IO_MEM_DATA));
	printf("%d PCI cards found\n",ioMemData->totalCards);


// If DAQ is via shared memory (Framebuilder code running on same machine or MX networking is used)
// attach DAQ shared memory location.
        sprintf(shm_name, "%s_daq", sysname);
	findSharedMemory(shm_name);
	_daq_shm = (char *)addr;
        if (_daq_shm < 0) {
                printf("mbuf_allocate_area() failed; ret = %d\n", _daq_shm);
                return -1;
        }
        // printf("Allocated daq shmem; set at 0x%x\n", _daq_shm);
	printf("DAQSM at 0x%lx\n",_daq_shm);
 	daqPtr = (struct rmIpcStr *) _daq_shm;


// Open new IO shared memory in support of no hardware I/O
        sprintf(shm_name, "%s_io_space", sysname);
	findSharedMemory(shm_name);
	_io_shm = (char *)addr;
        if (_io_shm < 0) {
                printf("mbuf_allocate_area() failed; ret = %d\n", _io_shm);
                return -1;
        }
	printf("IO SPACE at 0x%lx\n",_io_shm);
	ioMemDataIop = (volatile IO_MEM_DATA_IOP *)(((char *)_io_shm)) ;

	// Find and initialize all PCI I/O modules *******************************************************
	  // Following I/O card info is from feCode
	  cards = sizeof(cards_used)/sizeof(cards_used[0]);
	  printf("configured to use %d cards\n", cards);
	  cdsPciModules.cards = cards;
	  cdsPciModules.cards_used = cards_used;
          //return -1;
	printf("Initializing PCI Modules for IOP\n");
	for(jj=0;jj<cards;jj++)
		printf("Card %d type = %d\n",jj,cdsPciModules.cards_used[jj].type);
	cdsPciModules.adcCount = 0;
	cdsPciModules.dacCount = 0;
	cdsPciModules.dioCount = 0;
	cdsPciModules.doCount = 0;

// If running as a slave process, I/O card information is via ipc shared memory
	printf("%d PCI cards found\n",cards);
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

	ioMemData->totalCards = cards;

	// Have to search thru all cards and find desired instance for application
	// Master will map ADC cards first, then DAC and finally DIO
		for(jj=0;jj<cards;jj++)
		{
			/*
			printf("Model %d = %d, type = %d, instance = %d, dacCnt = %d \n",
				ii,ioMemData->model[ii],
				cdsPciModules.cards_used[jj].type,
				cdsPciModules.cards_used[jj].instance,
 				dacCnt);
				*/
		   switch(cdsPciModules.cards_used[jj].type)
		   {
			case GSC_16AI64SSA:
				kk = cdsPciModules.adcCount;
				cdsPciModules.adcType[kk] = GSC_16AI64SSA;
				cdsPciModules.adcCount ++;
				status ++;
				break;
			case GSC_16AO16:
				kk = cdsPciModules.dacCount;
				cdsPciModules.dacType[kk] = GSC_16AO16;
				cdsPciModules.dacCount ++;
				status ++;
				break;
			case GSC_18AO8:
				kk = cdsPciModules.dacCount;
				cdsPciModules.dacType[kk] = GSC_18AO8;
				cdsPciModules.dacCount ++;
				status ++;
				break;
			case CON_6464DIO:
				kk = cdsPciModules.doCount;
				cdsPciModules.doType[kk] = CON_6464DIO;
				cdsPciModules.doCount ++;
				cdsPciModules.cDio6464lCount ++;
				// cdsPciModules.doInstance[kk] = cDio6464lCount;
				status += 2;
				break;
                        case CON_32DO:
				kk = cdsPciModules.doCount;
				cdsPciModules.doType[kk] =  CON_32DO;
				cdsPciModules.doCount ++; 
				cdsPciModules.cDo32lCount ++; 
				cdsPciModules.doInstance[kk] = do32Cnt;
				status ++;
				 break;
			case ACS_16DIO:
				kk = cdsPciModules.doCount;
				cdsPciModules.doCount ++;
				cdsPciModules.iiroDio1Count ++;
				cdsPciModules.doInstance[kk] = doIIRO16Cnt;
				status ++;
				break;
			case ACS_8DIO:
			       kk = cdsPciModules.doCount;
			       cdsPciModules.doCount ++;
			       cdsPciModules.iiroDioCount ++;
			       cdsPciModules.doInstance[kk] = doIIRO8Cnt;
			       status ++;
		 	       break;
			default:
				break;
		   }
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
	// Master send module counds to SLAVE via ipc shm
	ioMemData->totalCards = status;
	ioMemData->adcCount = cdsPciModules.adcCount;
	ioMemData->dacCount = cdsPciModules.dacCount;
	ioMemData->bioCount = cdsPciModules.doCount;
	// kk will act as ioMem location counter for mapping modules
	kk = cdsPciModules.adcCount;
	printf("%d ADC cards found\n",cdsPciModules.adcCount);
	for(ii=0;ii<cdsPciModules.adcCount;ii++)
        {
		// MASTER maps ADC modules first in ipc shm for SLAVES
		ioMemData->model[ii] = cdsPciModules.adcType[ii];
		ioMemData->ipc[ii] = ii;	// ioData memory buffer location for SLAVE to use
		ioMemDataIop->model[ii] = cdsPciModules.adcType[ii];
		ioMemDataIop->ipc[ii] = ii;	// ioData memory buffer location for SLAVE to use
		for(jj=0;jj<65536;jj++) {
			for(mm=0;mm<32;mm++) {
				ioMemDataIop->iodata[ii][jj].data[mm] = jj - 32767;
			}
		}
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
		// Pass DAC info to SLAVE processes
		ioMemData->model[kk] = cdsPciModules.dacType[ii];
		ioMemData->ipc[kk] = kk;
		// Following used by MASTER to point to ipc memory for inputting DAC data from SLAVES
                cdsPciModules.dacConfig[ii]  = kk;
printf("MASTER DAC SLOT %d %d\n",ii,cdsPciModules.dacConfig[ii]);
		kk ++;
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
	// MASTER sends DIO module information to SLAVES
	// Note that for DIO, SLAVE modules will perform the I/O directly and therefore need to
	// know the PCIe address of these modules.
	ioMemData->bioCount = cdsPciModules.doCount;
	for(ii=0;ii<cdsPciModules.doCount;ii++)
        {
		// MASTER needs to find Contec 1616 I/O card to control timing slave.
		if(cdsPciModules.doType[ii] == CON_1616DIO)
		{
			tdsControl[tdsCount] = ii;
			printf("TDS controller %d is at %d\n",tdsCount,ii);
			tdsCount ++;
		}
		ioMemData->model[kk] = cdsPciModules.doType[ii];
		// Unlike ADC and DAC, where a memory buffer number is passed, a PCIe address is passed
		// for DIO cards.
		ioMemData->ipc[kk] = kk;
		kk ++;
	}
	printf("Total of %d I/O modules found and mapped\n",kk);
        printf("***************************************************************************\n");
// Following section maps Reflected Memory, both VMIC hardware style and Dolphin PCIe network style.
// Slave units will perform I/O transactions with RFM directly ie MASTER does not do RFM I/O.
// Master unit only maps the RFM I/O space and passes pointers to SLAVES.

	printf("%d RFM cards found\n",cdsPciModules.rfmCount);
	ioMemData->rfmCount = cdsPciModules.rfmCount;
	for(ii=0;ii<cdsPciModules.rfmCount;ii++)
        {
                 printf("\tRFM %d is a VMIC_%x module with Node ID %d\n", ii, cdsPciModules.rfmType[ii], cdsPciModules.rfmConfig[ii]);
		printf("address is 0x%lx\n",cdsPciModules.pci_rfm[ii]);
		// Master sends RFM memory pointers to SLAVES
		ioMemData->pci_rfm[ii] = cdsPciModules.pci_rfm[ii];
		ioMemData->pci_rfm_dma[ii] = cdsPciModules.pci_rfm_dma[ii];
	}
	// ioMemData->dolphinCount = 0;
#ifdef DOLPHIN_TEST
	ioMemData->dolphinCount = cdsPciModules.dolphinCount;
	ioMemData->dolphinRead[0] = cdsPciModules.dolphinRead[0];
	ioMemData->dolphinWrite[0] = cdsPciModules.dolphinWrite[0];

#else
// Clear Dolphin pointers so the slave sees NULLs
	ioMemData->dolphinCount = 0;
        ioMemData->dolphinRead[0] = 0;
        ioMemData->dolphinWrite[0] = 0;
#endif
        printf("***************************************************************************\n");
  	if (cdsPciModules.gps) {
	printf("IRIG-B card found %d\n",cdsPciModules.gpsType);
        printf("***************************************************************************\n");
  	}


	// Initialize buffer for daqLib.c code
	printf("Initializing space for daqLib buffers\n");
	daqBuffer = (long)&daqArea[0];
 
        pLocalEpics = (CDS_EPICS *)&((RFM_FE_COMMS *)_epics_shm)->epicsSpace;
	for (cnt = 0;  cnt < 10 && pLocalEpics->epicsInput.burtRestore == 0; cnt++) {
        	printf("Epics burt restore is %d\n", pLocalEpics->epicsInput.burtRestore);
        	usleep(1000000);
	}
	if (cnt == 10) {
		return -1;
	}

        pLocalEpics->epicsInput.vmeReset = 0;
	fe_start_iop_user();

        return 0;
}
