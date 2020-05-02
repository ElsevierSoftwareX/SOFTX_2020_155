static char *versionId = "Version $Id$" ;

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include "dtt/gdsmain.h"
#include "dtt/gdstask.h"
#include "dtt/testpoint_server.h"
#include "dtt/awg.h"
#include "dtt/awg_server.h"
#include "dtt/gdsprm.h"
#include "rmapi.h"
#include "drv/cdsHardware.h"
#include "modelrate.h"

#ifdef OS_VXWORKS
#define _PRIORITY_TPMAN		40
#define _PRIORITY_AWG		40
#else
#define _PRIORITY_TPMAN		60
#define _PRIORITY_AWG		60
#endif
#define _TPMAN_NAME		"tTPmgr"
#define _AWG_NAME		"tAWGmgr"

char ifo_prefix_storage[3];
char site_prefix_storage[2];
char archive_storage[256];
char *ifo_prefix = ifo_prefix_storage; // G1
char *site_prefix = site_prefix_storage; // G
char *archive = archive_storage; // /opt/rtcds/geo/g1/target/gds/
char site_name_lower[16]; // geo
char ifo_prefix_lower[3];  // g1
char myParFile[256];

/* How many times over 16 kHz is the front-end system? */
int sys_freq_mult = 1;

/* Control system name */
char system_name[PARAM_ENTRY_LEN];

// Set to one when we are in sync with master (x00)
int inSyncMaster = 0;

// Pointers into the shared memory for the cycle and time (coming from the IOP (e.g. x00))
volatile unsigned int *ioMemDataCycle;
volatile unsigned int *ioMemDataGPS;
volatile IO_MEM_DATA *ioMemData;

CDS_HARDWARE cdsPciModules;



   int main (int argc, char* argv[])
   {
   #ifdef OS_VXWORKS
      printf ("Not supported\n");
      return 1;
   #else
      char 		c;		/* flag */
      int		errflag = 0;	/* error flag */
      int		attr;	        /* task creation attribute */
      taskID_t		tpmanTID = 0;
      taskID_t		awgTID = 0;
      int		run_awg = 1;
      int		run_tpman = 1;
      int 		lckall = 0;
      int               sys_freq_mult_cmdline = 1;

      /* awgtpman by default tries to read the model rate from the INI files
       * for the model.  failing that, it assumes a 16 kHz (actually 2^14 Hz)
       * rate that is modified by numerical arguments.  -2 would make the assumed
       * rate 32 Khz, -4 is 64 kHz, -8 128 kHz etc.  The numerical args stack
       * multiplicitively so that -2 -4 is also 128 kHz
       * If the flag below is set, the model rate is first read from file.
       * If cleared, the the attempt to read from file is skipped and the
       * rate as set by command line arguments is used.
       *
       * Setting the -r command line option sets this flag */
      int               use_file_model_rate = 1;

#if defined(OS_SOLARIS)
      run_awg = 0;
#endif
      {
	 int i ;
	 char   hostname[128] ;

	 for(i = 0; i < argc; i++)
	 {
	    fprintf(stderr, "%s ", argv[i]) ;
	 }
	 fprintf(stderr, "started ") ;
	 if (gethostname(hostname, sizeof(hostname)) == 0)
	 {
	    fprintf(stderr, "on host %s ", hostname) ;
	 }
	 fprintf(stderr, "hostid %lx ", gethostid()) ;
	 fprintf(stderr, "\nawgtpman %s\n", versionId) ;
      }
   
      system_name[0] = 0;
      while ((c = getopt (argc, argv, "h?tar01248ws:l:")) != EOF) {
         switch (c) {
	    case 'w':
		lckall = 1;
		break;
	    case 's':
		if (strlen(optarg) > (PARAM_ENTRY_LEN-2)) {
			printf("System name is too long\n");
			exit(1);
		}
		strcpy(system_name, optarg);
		break;
            /* help */
            case 'h':
            case '?':
               {
                  errflag = 1;
                  break;
               }
	    case 't':
	       {
		 run_awg = 0;
		 break;
	       }
	    case 'a':
	       {
		 run_tpman = 0;
		 break;
	       }
	    case 'l':
	    	{
			if (0 == freopen(optarg, "w", stdout)) {
				perror("freopen");
				exit(1);
			}
			setvbuf(stdout, NULL, _IOLBF, 0);
			stderr = stdout;
			break;
		}
            case 'r':
                {
                    use_file_model_rate = 0;
                    break;
                }
	    case '1':
	    case '2':
	    case '4':
	    case '8':
	       {
		 sys_freq_mult_cmdline *= c - '0';
		 break;
	       }
	    case '0':
	    	{
			sys_freq_mult_cmdline = 1;
			break;
	 	}
         }
      }

      int rate_hz=0, dcuid=0;
      if(use_file_model_rate)
      {
          getmodelrate(&rate_hz, &dcuid, system_name, NULL);
      }
      if(rate_hz)  //if getmodelrate() fails to get the rate, it'll still be zero.
      {
          sys_freq_mult = rate_hz >> 14;  //rate is in Hz but sys_freq_mult is times 2^14 Hz.
          (sys_freq_mult > 0) ? sys_freq_mult : 1;  //rate can be less than 16kHz
                                                    //but awgtpman still set to 16kHz
      }
      else
      {
          sys_freq_mult = sys_freq_mult_cmdline;
      }

      printf("%d kHz system\n", 16 * sys_freq_mult);
   
      /* help */
      if (errflag) {
         printf ("Usage: awgtpman\n"
	        "	Starts awg and tpman on a unix machine\n"
                "	-h : help\n"
                "	-l file_name : specify log file name\n"
		"	-s system_name : specify control system name\n"
		"	-t : run tpman, no awg\n"
		"	-a : run awg, no tpman\n"
                "	-w : lock all pages in memory\n"
                "\n"
                "awgtpman tries to determine the model rate from files in the\n"
                "    TARGET directory. Failing that, awgtpman uses a 16 kHz rate\n"
                "    unless modified by one or more numerical arguments.\n"
                "\n"
                "       -r : override file model rate.  Use 16 kHz modified by \n"
                "            any numerical arguments\n"
		"	-2 : run awg at 32 kHz\n"
		"	-4 : run awg at 64 kHz\n"
		"	-8 : run awg at 128 kHz\n"
		"	-8 -2 : run awg at 256 kHz\n");
         return 1;
      }
   
      if (!run_awg && !run_tpman) exit(0);

      if (lckall) {
      	// Lock all current and future process pages in memory
      	mlockall(MCL_FUTURE);
      }

#ifdef __linux__
      initReflectiveMemory();
#endif
      if (run_awg) {
        nice(-20);
      }
/*
                                if ($::site =~ /^M/) {
                                        $::location = "mit";
                                } elsif ($::site =~ /^G/) {
                                        $::location = "geo";
                                } elsif ($::site =~ /^H/) {
                                        $::location = "lho";
                                } elsif ($::site =~ /^L/) {
                                        $::location = "llo";
                                } elsif ($::site =~ /^C/) {
                                        $::location = "caltech";
                                } elsif ($::site =~ /^S/) {
                                        $::location = "stn";
                                } elsif ($::site =~ /^K/) {
                                        $::location = "kamioka";
                                } elsif ($::site =~ /^X/) {
                                        $::location = "tst";
                                }

*/
      char st[3]; st[0] = system_name[0]; st[1] = system_name[1]; st[2] = 0;
      switch(st[0]) {
	case 'm':
		strcpy(site_prefix_storage, "M");
		strcpy(site_name_lower, "mit");
		break;
	case 'g':
		strcpy(site_prefix_storage, "G");
		strcpy(site_name_lower, "geo");
		break;
	case 'h':
		strcpy(site_prefix_storage, "H");
		strcpy(site_name_lower, "lho");
		break;
	case 'l':
		strcpy(site_prefix_storage, "L");
		strcpy(site_name_lower, "llo");
		break;
	case 'c':
		strcpy(site_prefix_storage, "C");
		strcpy(site_name_lower, "caltech");
		break;
	case 's':
		strcpy(site_prefix_storage, "S");
		strcpy(site_name_lower, "stn");
		break;
	case 'k':
		strcpy(site_prefix_storage, "K");
		strcpy(site_name_lower, "kamioka");
		break;
	case 'i':
		strcpy(site_prefix_storage, "I");
		strcpy(site_name_lower, "indigo");
		break;
	case 'a':
		strcpy(site_prefix_storage, "A");
		strcpy(site_name_lower, "anu");
		break;
	case 'u':
		strcpy(site_prefix_storage, "U");
		strcpy(site_name_lower, "uwa");
		break;
	case 'w':
		strcpy(site_prefix_storage, "W");
		strcpy(site_name_lower, "cardiff");
		break;
	case 'b':
		strcpy(site_prefix_storage, "B");
		strcpy(site_name_lower, "bham");
		break;
	case 'x':
		strcpy(site_prefix_storage, "X");
		strcpy(site_name_lower, "tst");
		break;
	default:
		fprintf(stderr, "Unknown location: %s\n",  st);
		exit(1);
		break;
      }
      strcpy(ifo_prefix_lower, st);
      strcpy(ifo_prefix_storage, st);
      ifo_prefix_storage[0] = toupper(ifo_prefix_storage[0]);
      sprintf(archive_storage, "/opt/rtcds/%s/%s/target/gds", site_name_lower, ifo_prefix_lower);
      sprintf(myParFile, "%s/param/tpchn_%s.par", archive, system_name);
      printf("My config file is %s\n", myParFile);

      printf("IPC at 0x%p\n", rmBoardAddress(2));
      ioMemData = (IO_MEM_DATA *)(rmBoardAddress(2) + IO_MEM_DATA_OFFSET);

      // Make sure there is no other copy running already
      { 
      	int g,g1;
        rmRead (0, (char*)&g, 0, 4, 0);
	sleep(1);
        rmRead (0, (char*)&g1, 0, 4, 0);
	if (g != g1) {
		fprintf(stderr, "Another copy already running! Will not start a second copy.\n");
		_exit(1);
	}
      }

      // Find the first ADC card
      // Master will map ADC cards first, then DAC and finally DIO
      if (ioMemData -> totalCards == 0) {
	// Wait for the master to come up
	printf("Waiting for the IOP to start\n");
	while (ioMemData -> totalCards == 0) {
		sleep(2);
	}
      }
      printf("Total PCI cards from the master: %d\n", ioMemData -> totalCards);
      sleep(2);
      for (int ii = 0; ii < 1; ii++) {
          printf("Model %d = %d\n",ii,ioMemData->model[ii]);
          switch (ioMemData -> model [ii]) {
            case GSC_16AI64SSA:
              printf("Found ADC at %d\n", ioMemData -> ipc[ii]);
              cdsPciModules.adcType[0] = GSC_16AI64SSA;
              cdsPciModules.adcConfig[0] = ioMemData->ipc[ii];
              cdsPciModules.adcCount = 1;
               break;
          }
      }
      if (!cdsPciModules.adcCount) {
                printf("No ADC cards found - exiting\n");
                _exit(1);
      }

      int ll = cdsPciModules.adcConfig[0];
      ioMemDataCycle = &ioMemData->iodata[ll][0].cycle;
      ioMemDataGPS = &ioMemData->gpsSecond;

      if (run_tpman) {

	if (!run_awg) return testpoint_server();

        /* Start TP Manager */
        printf ("Spawn testpoint manager\n");
   #ifdef OS_VXWORKS
        attr = 0;
   #else
        attr = PTHREAD_CREATE_DETACHED | PTHREAD_SCOPE_SYSTEM;
   #endif
        if (taskCreate (attr, _PRIORITY_TPMAN, &tpmanTID, 
           _TPMAN_NAME, (taskfunc_t) testpoint_server, 0) < 0) {
	   printf ("Error: Unable to spawn testpoint manager\n");
           return 1;
        }
        sleep (5);
     
	{
          extern int testpoint_manager_node;
          if (testpoint_manager_node < 0) {
	    printf("Test point manager startup failed; %d\n", testpoint_manager_node);
	    return 1;
          }
	}
      }

      if (run_awg) {
        /* Start AWG Manager */
        printf ("Spawn arbitrary waveform generator\n");
   #ifdef OS_VXWORKS
        attr = 0;
   #else
        attr = PTHREAD_CREATE_DETACHED | PTHREAD_SCOPE_SYSTEM;
   #endif
        if (taskCreate (attr, _PRIORITY_AWG, &awgTID, 
           _AWG_NAME, (taskfunc_t) awg_server, 0) < 0) {
	   printf ("Error: Unable to spawn arbitrary waveform generator\n");
           return 1;
        }
        sleep (5);

        /* Load AWG paramters here */
       awgLock(1);
      }
      

      fflush(stdout) ;
      fflush(stderr) ;

      /* go to sleep */
      for (;;) {
         sleep (1000);
      }
      /* Never reached */
   #endif
   }
