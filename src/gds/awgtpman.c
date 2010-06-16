
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "dtt/gdsmain.h"
#include "dtt/gdstask.h"
#include "dtt/testpoint_server.h"
#include "dtt/awg.h"
#include "dtt/awg_server.h"
#include "dtt/gdsprm.h"

#ifdef OS_VXWORKS
#define _PRIORITY_TPMAN		40
#define _PRIORITY_AWG		40
#else
#define _PRIORITY_TPMAN		60
#define _PRIORITY_AWG		60
#endif
#define _TPMAN_NAME		"tTPmgr"
#define _AWG_NAME		"tAWGmgr"

char *ifo_prefix = "G1";
char *site_prefix ="G";
char *archive = "/cvs/cds/geo/target/gds/";
char myParFile[128];

/* How many times over 16 kHz is the front-end system? */
int sys_freq_mult = 1;

/* Control system name */
char system_name[PARAM_ENTRY_LEN];

// Set to one when we are in sync with master (x00)
int inSyncMaster = 0;

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


#if defined(OS_SOLARIS)
      run_awg = 0;
#endif
   
      system_name[0] = 0;
      while ((c = getopt (argc, argv, "h?ta248s:")) != EOF) {
         switch (c) {
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
	    case '2':
	    case '4':
	    case '8':
	       {
		 sys_freq_mult *= c - '0';
		 break;
	       }
         }
      }
      printf("%d kHz system\n", 16 * sys_freq_mult);
   
      /* help */
      if (errflag) {
         printf ("Usage: awgtpman\n"
	        "	Starts awg and tpman on a unix machine\n"
                "	-h : help\n"
		"	-s system_name : specify control system name\n"
		"	-t : run tpman, no awg\n"
		"	-a : run awg, no tpman\n"
		"	-2 : run awg at 32 kHz\n"
		"	-4 : run awg at 64 kHz\n"
		"	-8 : run awg at 128 kHz\n"
		"	-8 -2 : run awg at 256 kHz\n");
         return 1;
      }
   
      if (!run_awg && !run_tpman) exit(0);

#ifdef __linux__
      initReflectiveMemory();
#endif
      if (run_awg) {
        if (geteuid() != 0) {
	  printf ("Must be a superuser to run awgtpman\n");
	  return 1;
        } else {
	  nice(-20);
        }
      }

      sprintf(myParFile, "%s/param/tpchn_%s.par", archive, system_name);
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
        sleep (1);
     
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
      

      /* go to sleep */
      for (;;) {
         sleep (1000);
      }
      /* Never reached */
   #endif
   }
