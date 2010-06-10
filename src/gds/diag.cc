/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: diag							*/
/*                                                         		*/
/* Module Description: command line interface to DTT			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

#ifndef DEBUG
#define DEBUG
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Includes: 								*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <iostream>
#include "dtt/gdsmain.h"
#include "dtt/cmdline.hh"
#include "dtt/confinfo.h"

   using namespace std;
   using namespace diag;


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Constants: _argCMD		argument representing the commmand line	*/
/*            _argHelp		argument for displaying help		*/
/*            _argLocal		local diagnostics kernel		*/
/*            _argServer	remote diagnostics kernel		*/
/*            _argScript	read init script from file		*/
/*            _argNDS		NDS server name				*/
/*            _argNDSport	NDS port number				*/
/*            help_text		help text				*/
/*            								*/
/*----------------------------------------------------------------------*/
   const string 	_argCMD ("-c");
   const string 	_argInfo ("-i");
   const string		_argHelp ("-help");
   const string 	_argLocal ("-l");
   const string 	_argServer ("-s");
   const string 	_argScript ("-f");
   const string 	_argNDS ("-n");
   const string 	_argNDSport ("-m");
   const string		help_text 
   ("Usage: diag -g              start diagnostics GUI\n"
   "       diag -c              start command line (obsolete)\n"
   "       diag -i              show configuration information\n"
   "       diag -help           show this help screen\n"
   "Additional arguments\n"
   "       -l                   use local diagnostics kernel\n"
   "       -s 'server'          use remote diagnostics kernel\n"
   "       -f 'filename'        read init script from file\n"
   "       -n 'nds name'        specifies the name of the NDS\n"
   "       -m 'nds port'        specifies the port number of the NDS\n");


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Main Program 							*/
/*                                                         		*/
/* Description: 							*/
/* 									*/
/*----------------------------------------------------------------------*/
   int main (int argc, char *argv[])
   {
      int		i;
   
      /* parse arguments: look for help first */
      for (i = 0; i < argc; i++) {
         if (_argHelp == argv[i]) {
            cout << help_text;
            return 0;
         }
      }
   
      /* parse arguments: look for info */
      for (i = 0; i < argc; i++) {
         if (_argInfo == argv[i]) {
            const char* const* ret = getConfInfo (0, 0);
            if (ret == NULL) {
               cout << "No configuration information available" << endl;
               return 0;
            }
            cout << "Diagnostics configuration:" << endl;
            if (*ret == NULL) {
               cout << "no services available" << endl;
            }
            else {
               while  (*ret != NULL) {
                  cout << *ret << endl;
                  ret++;
               }
            }
            return 0;
         }
      }
   
      commandline		cmdline (argc, argv);
      if (!cmdline) {
         printf ("error starting comamnd line\n");
      }
      while (cmdline()) {
      }
      return 0;
   }

