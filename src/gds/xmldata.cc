static const char *versionId = "Version $Id$" ;
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: xmldata							*/
/*                                                         		*/
/* Module Description: program to receive on-line data and save it	*/
/*                     in the LIGI-LW data format			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Includes: 								*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#include <time.h>
#include <iostream>
#include <string>
#include <unistd.h>
#include "dtt/gdsutil.h"
#include "dtt/gdschannel.h"
#include "dtt/gdsdatum.hh"
#include "dtt/testpointmgr.hh"
#include "dtt/rtddinput.hh"
#include "dtt/testpointinfo.h"

   using namespace std;
   using namespace diag;

#define DAQD_PORT 8088


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Constants: _argHelp		argument for displaying help		*/
/*            _argFile		save file flag				*/
/*            _argServer	server flag				*/
/*            _argPort		port flag				*/
/*            _argQuite		quite flag				*/
/*            help_text		help text				*/
/*            								*/
/*----------------------------------------------------------------------*/
   const string		_argHelp ("-help");
   const string 	_argFile ("-f");
   const string 	_argServer ("-s");
   const string 	_argPort ("-p");
   const string 	_argQuite ("-q");
   const string		help_text 
   ("Usage: xmldata -flags duration channel1 channel2 ...\n"
   "       channel channel names to be recorded\n"
   "       duration time of recording\n"
   "       -flags control parameters\n"
   "Control parameters\n"
   "       -f 'filename'        file name to write xml\n"
   "       -s 'server'          server name\n"
   "       -p 'port'            server port\n"
   "       -q                   quite\n");
   const int		defaultport = DAQD_PORT;


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Types: 				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   typedef vector<string> namelist;


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Globals: 				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Forward declarations: 						*/
/*			*/
/*      								*/
/*----------------------------------------------------------------------*/


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
      bool		nomsg = false;
      string		servername;
      int		serverport = defaultport;
      double		duration = 1E100;
      namelist		chnnames;
      string		filename = "data.xml";
   
      // no arguments
      if (argc <= 1) {
         cout << help_text;
         return 0;
      }
      cout << "test-1" << endl;
   
      // parse arguments
      for (i = 1; i < argc; i++) {
         // help
         if (_argHelp == argv[i]) {
            cout << help_text;
            return 0;
         }
         // quite flag
         else if (_argQuite == argv[i]) {
            nomsg = true;
         }
         // filename flag
         else if (_argFile == argv[i]) {
            if (i + 1 >= argc) {
               cout << help_text;
               return 1;
            }
            i++;
            filename = argv[i];
         }
         // server name flag
         else if (_argServer == argv[i]) {
            if (i + 1 >= argc) {
               cout << help_text;
               return 1;
            }
            i++;
            servername = argv[i];
         }
         // server port flag
         else if (_argPort == argv[i]) {
            if (i + 1 >= argc) {
               cout << help_text;
               return 1;
            }
            i++;
            if (sscanf (argv[i], "%i", &serverport) != 1) {
               cout << help_text;
               return 1;
            }
         } 
         // duration
         else if (duration > 1E99) {
            if (sscanf (argv[i], "%lf", &duration) != 1) {
               cout << help_text;
               return 1;
            }
         }
         // channel name
         else {
            chnnames.push_back (argv[i]);
         }
      }
      cout << "test0" << endl;
   
      // initialize channel info
      if (!servername.empty()) {
         if (gdsChannelSetHostAddress (servername.c_str(), 
                              serverport) < 0) {
            cout << "Unable to connect to NDS" << endl;
            return 1;
         }
      }
      if (channel_client() < 0) {
         cout << "Unable to connect to NDS" << endl;
         return 1;
      }
      cout << "test1" << endl;
   
      // time
      tainsec_t		now = TAInow ();
      now = _ONESEC * ((now + _ONESEC - 1) / _ONESEC);
      tainsec_t		dT = (tainsec_t) (duration * (double) _ONESEC);
      utc_t		utc;
      char		s[100];
      TAIntoUTC (now, &utc);
      strftime (s, 100, "%Y-%m-%d %H:%M:%S", &utc);
      cout << "test2" << endl;
   
      // create storage object and rtdd manager
      gdsStorage	st ("xmldata", s);
      testpointMgr	tp;
      rtddManager	rtdd (&st, &tp);
      cout << "test3" << endl;
   
      // connect to nds
      const char* pServer = servername.empty() ? 0 : servername.c_str();
      if (!rtdd.connect (pServer, serverport)) {
         cout << "Unable to connect to NDS" << endl;
         return 1;
      }
      cout << "test4 = " << now << endl;
   
      // add channels
      for (namelist::iterator iter = chnnames.begin();
          iter != chnnames.end(); iter++) {
         // channel info
         gdsChnInfo_t	info;
         // test if valid channel
         if (gdsChannelInfo (iter->c_str(), &info) < 0) {
            cout << "Unrecognized channel name 1: " << *iter << endl;
            return 1;
         }
         // subscribe name
         if (!rtdd.add (*iter)) {
            cout << "Unrecognized channel name 2: " << *iter << endl;
            return 1;
         }
         // setup partition
         dataChannel::partition 	p (*iter, now, dT);
         p.setup (1.0 / (double) info.dataRate, 1);
         dataChannel::partitionlist 	plist (1, p);
         // add partition
         rtdd.add (*iter, plist);
      }
      cout << "test5" << endl;
   
      // start measurement
      if (!rtdd.set ()) {
         cout << "Unable to connect to nds" << endl;
         return 1;
      }
      cout << "test6" << endl;
   
      // wait till finished
      while (rtdd.timeStamp() < now + dT) {
         struct timespec tick = {0, 30000000};
         nanosleep (&tick, 0);
         if (TAInow() > now + dT + _ONESEC) {
            cout << "Data not received in time" << endl;
            return 1;
         }
      }
   
      cout << "test7" << endl;
   
      // stop measurement
      rtdd.clear (false);
      cout << "test7.5" << endl;
      rtdd.del ();
      cout << "test8" << endl;
   
      // {
         // gdsDataObject* 	dobj = st.findData (chnnames.front()) ;
         // cout << "3rd" << endl;
         // for (gdsDataObject::gdsParameterList::iterator iter =
             // dobj->parameters.begin(); iter != dobj->parameters.end();
             // iter++) {
            // cout << **iter;
         // }
      // }
      // write XML
      cout << "file name = " << filename << endl;
      st.fsave (filename, gdsStorage::ioExtended);
      cout << "test9" << endl;
      return 0;
   }
