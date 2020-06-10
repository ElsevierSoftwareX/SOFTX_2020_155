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

char myParFile[256];

char archive_storage[256];
char *archive = archive_storage;

char site_prefix_storage[2];
char *site_prefix = site_prefix_storage;

char target[256];

/* How many times over 16 kHz is the front-end system?
 * 2 kHz for slow models */
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
       * for the model.  If the -r agrument is specified,
       * it assumes a 16 kHz (actually 2^14 Hz)
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
                   "    TARGET directory by default. With the '-r' argument,\n"
                   "    awgtpman uses a rate derived from command line arguments\n"
                   "\n"
                   "       -r   : override file model rate.  Use 16 kHz or 2 kHz\n"
                   "              modified by any numerical arguments\n"
                   "       -<n> : a single digit that's a multiple of 2 (1,2,4,8).\n"
                   "              The model rate is a base rate, either 2 or 16 kHz\n"
                   "              multiplied by <n>.  More than one argument can be\n"
                   "              given to reach higher rates.\n");
           return 1;
       }


       //setup the target directory.  Look for TARGET env, first, then failing that, build an
      //old style directory, then just fail.
      if(getenv("TARGET"))
      {
          snprintf(target, sizeof(target), "%s", getenv("TARGET"));
      }
      else if(getenv("IFO") && getenv("SITE"))
      {
          char site_upper[4], site_lower[4];
          char ifo_upper[3], ifo_lower[3];

          printf("WARNING: TARGET environment variable not found\n");
          printf("\tBuilding an old style target path\n");
          snprintf(site_upper, sizeof site_upper, "%s", getenv("SITE"));
          snprintf(ifo_upper, sizeof ifo_upper, "%s", getenv("IFO"));
          printf("SITE=%s, IFO=%s\n", site_upper, ifo_upper);
          int lc=0;
          while(( site_lower[lc] = (char)tolower(site_upper[lc])))
          {
              lc++;
          }
          lc=0;
          while(( ifo_lower[lc] = (char)tolower(ifo_upper[lc])))
          {
              lc++;
          }
          printf("site=%s, ifo=%s\n", site_lower, ifo_lower);
          snprintf( target,
                    sizeof( target ),
                    "/opt/rtcds/%s/%s",
                    site_lower,
                    ifo_lower );
      }
      else
      {
          fprintf(stderr, "ERROR: TARGET environment variable not found.\n"
            "\tTARGET must be set to the base target directory for the IFO\n"
            "\te.g. /opt/rtcds/tst/x2, so that \"$TARGET/target/gds/param\"\n"
            "\tis a directory containing the \".par\" parameter files.\n");
          exit(2);
      }
      printf("Target directory is %s\n", target);

      int rate_hz=0, dcuid=0;
      if(use_file_model_rate)
      {
          char tpdir[256];
          snprintf(tpdir, sizeof tpdir, "%s/target/gds/param", target);
          printf("Looking for model rate in %s\n", tpdir);
          get_model_rate_dcuid( &rate_hz, &dcuid, system_name, tpdir);

          if(!rate_hz)
          {
              fprintf(stderr, "ERROR: model rate for %s could not be read from param file.\n", system_name);
              fprintf(stderr, "\tCheck the spelling of the system name (argument to -s).\n");
              fprintf(stderr, "\tCheck that the TARGET environment variable is set so that the directory\n"
                               "\t\"$TARGET/target/gds/param\" contains the parameter files for the model.\n");
              fprintf(stderr, "\tUse the -r argument to specify the model rate from the command line.\n");
              exit(-1);
          }

          // in high rate models >= 16 kHz, multiplier is x 16 kHz.
          // for slow rate models, it's times 2 kHz (2^11 Hz)
          if(rate_hz >= 1<<14)
          {
              sys_freq_mult = rate_hz >> 14;

          }
          else
          {
              sys_freq_mult = rate_hz >> 11;
          }

          if(sys_freq_mult <= 0)
          {
              fprintf(stderr, "Calculated multiplier of %d from model rate %d Hz. Multiplier must be greater than zero.",
                sys_freq_mult, rate_hz);
          }

      }
      else
      {
          sys_freq_mult = sys_freq_mult_cmdline;
      }

      printf("%d or %d kHz system\n", 2 * sys_freq_mult, 16 * sys_freq_mult);
   
      /* help */

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

      // site_prefix takes on the first letter of the system name (as upper case)
      site_prefix_storage[1] = 0;
      site_prefix_storage[0] = toupper(system_name[0]);

      snprintf(myParFile, sizeof(myParFile),"%s/target/gds/param/tpchn_%s.par", target, system_name);
      printf("My config file is %s\n", myParFile);
      snprintf(archive_storage, sizeof archive_storage, "%s/target/gds", target);
      printf("Archive storage is %s\n", archive);

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
