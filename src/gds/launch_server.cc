/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: launch_server						*/
/*                                                         		*/
/* Module Description: implements server functions for providing  	*/
/* launch information and remote exec capability			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

/* #define RPC_SVC_FG */

/* Header File List: */
#ifndef __EXTENSIONS__
#define __EXTENSIONS__
#endif 
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include "dtt/gdsutil.h"
#include "dtt/rpcinc.h"
#include "gmutex.hh"
#include "dtt/rlaunch.h"
#include "dtt/confserver.h"
#include "dtt/launch_server.hh"


namespace diag {
   using namespace std;
   using namespace thread;

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Constants: _KEEPALIVE_TIMEOUT  timeout for keep alive signal		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#define _KEEPALIVE_TIMEOUT	60

#ifdef RPC_SVC_FG
#define _SVC_FG		1
#else
#define _SVC_FG		1
#endif
#if !defined (OS_VXWORKS) && !defined (PORTMAP)
#define _SVC_MODE	RPC_SVC_MT_AUTO
#else
#define _SVC_MODE	0
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Types: 								*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   struct launch_info_t {
      string		title;
      string		prog;
      string		cmd;
      string		arg;
   };
   typedef vector<launch_info_t>	launchlist;

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Globals: servermux		protects globals			*/
/*          list		program list				*/
/*          shutdownflag	shutdown flag				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static readwritelock		servermux;
   static launchlist		list;
   static int			shutdownflag = 0;


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Utilities: 								*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static char* strcopy (const char* p) 
   {
      if (!p) 
         return 0;
      char* pp = (char*)malloc (strlen(p) + 1);
      if (!pp) 
         return 0;
      strcpy (pp, p);
      return pp;
   }

   static string readnext (string& s)
   {
      string r;
      while (!s.empty() && isspace (s[0])) s.erase (0, 1);
      if (s.empty()) {
         return r;
      }
      int len = 0;
      if (!s.empty() && (s[0] == '"')) {
         len = 1;
         while ((len < (int)s.size()) && (s[len] != '"')) ++len;
         r = s.substr (1, len - 1);
      }
      else {
         while ((len < (int)s.size()) && !isspace (s[len])) ++len;
         r = s.substr (0, len);
      }
      if (len < (int)s.size()) {
         s.erase (0, len + 1);
         while (!s.empty() && isspace (s[0])) s.erase (0, 1);
      }
      else {
         s = "";
      }
      return r;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Forward declarations: 						*/
/*      rlaunchprog_1		rpc dispatch function			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
extern "C" void rlaunchprog_1 (struct svc_req* rqstp, 
                     register SVCXPRT* transp);


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Remote Procedure Name: launchquery_1_svc				*/
/*                                                         		*/
/* Procedure Description: launch information query			*/
/*                                                         		*/
/* Procedure Arguments: launch info list (result), svc request handle	*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if not (status)		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
extern "C" 
   bool_t launchquery_1_svc (resultLaunchInfoQuery_r* result, 
                     struct svc_req* rqstp)
   {
      rpcSetServerBusy (1);
      semlock		lockit (servermux);
   
      // copy list into result
      result->list.launch_infolist_r_len = list.size();
      result->list.launch_infolist_r_val = (launch_info_r*) 
         calloc (list.size(), sizeof (launch_info_r));
      if (result->list.launch_infolist_r_val == 0) {
         result->status = -1;
      }
      else {
         int i = 0;
         for (launchlist::iterator iter = list.begin();
             iter != list.end(); ++iter, ++i) {
            result->list.launch_infolist_r_val[i].title = 
               strcopy (iter->title.c_str());
            result->list.launch_infolist_r_val[i].prog = 
               strcopy (iter->prog.c_str());
         }
         result->status = 0;
      }
      return TRUE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Remote Procedure Name: launch_1_svc					*/
/*                                                         		*/
/* Procedure Description: launch command				*/
/*                                                         		*/
/* Procedure Arguments: launc cmd, svc request handle			*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if not (status)		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
extern "C" 
   bool_t launch_1_svc (char* title, char* prog, char* display, 
                     int* result, struct svc_req* rqstp)
   {
      // valid title/prog?
      if (!title || !prog) {
         *result = -1;
         return TRUE;
      }
      // determine display
      string d = display ? display : "";
      while (!d.empty() && isspace (d[0])) d.erase (0, 1);
      if (d.empty() || (d[0] == ':')) {
         struct in_addr caddr;
         if (rpcGetClientaddress (rqstp->rq_xprt, &caddr) != 0) {
            *result = -1;
            return TRUE;
         }
         d.insert (0, inet_ntoa (caddr));
      }
      if (d.find (':') == string::npos) {
         d += ":0.0";
      }
   
      // look for program command
      for (launchlist::iterator iter = list.begin();
          iter != list.end(); ++iter) {
         if ((iter->title == title) && (iter->prog == prog)) {
            // found it! 
            string cmd = iter->cmd;
            cmd += " -display " + d + " " + iter->arg + " &";
            if (::system (cmd.c_str()) == -1) {
               *result = -2;
            }
            else {
               *result = 0;
            }
            return TRUE;
         }
      }
      *result = -3;
      return TRUE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: rlaunchprog_1_freeresult			*/
/*                                                         		*/
/* Procedure Description: frees memory of rpc call			*/
/*                                                         		*/
/* Procedure Arguments: rpc transport info, xdr routine for result,	*/
/*			pointer to result				*/
/*                                                         		*/
/* Procedure Returns: TRUE if successful, FALSE if failed		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
extern "C"
   int rlaunchprog_1_freeresult (SVCXPRT* transp, 
                     xdrproc_t xdr_result, caddr_t result)
   {
      (void) xdr_free (xdr_result, result);
      return TRUE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: readLaunchFile				*/
/*                                                         		*/
/* Procedure Description: reads launch info from file			*/
/* 			  mutex MUST be owned by caller			*/
/*                                                         		*/
/* Procedure Arguments: filename					*/
/*                                                         		*/
/* Procedure Returns: true if successful, false otherwise.		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static bool readLaunchFile (const string& filename)
   {
      launch_info_t	info;			// launch info record
      ifstream		in (filename.c_str());	// launch config. file
      string		line;
   
      // open config file
      if (!in) {
         return false;
      }
   
      // loop through configuration file
      while (getline (in, line)) {
         // skip leading blanks
         while (!line.empty() && isspace (line[0])) {
            line.erase (0, 1);
         }
         // skip empty lines and comments
         if (line.empty() || (line[0] == '#')) {
            continue;
         }
         // read in parameters
         info.title = readnext (line);
         info.prog = readnext (line);
         info.cmd = readnext (line);
         info.arg = line;
         // now add elements
         if (!info.title.empty() && !info.prog.empty() &&
            !info.cmd.empty()) {
            list.push_back (info);
         }
      }
   
      // close the file
      in.close ();
      return true;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: launch_server				*/
/*                                                         		*/
/* Procedure Description: start rpc service task for launch server	*/
/*                                                         		*/
/* Procedure Arguments: configuration file				*/
/*                                                         		*/
/* Procedure Returns: does not return if successful, <0 otherwise	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   bool launch_server (const string& config)
   {
      int		rpcpmstart;	// port monitor flag
      SVCXPRT*		transp;		// service transport
      int		proto;		// protocol
      struct in_addr	host;		// local host address
   
      // read configuration files
      servermux.writelock();
      if (!readLaunchFile (config)) {
         servermux.unlock();
         gdsError (GDS_ERR_PROG, 
                  "unable to load launch server configuration file");
         return false;
      }
      servermux.unlock();
   
      // get local address
      if (rpcGetLocaladdress (&host) < 0) {
         gdsError (GDS_ERR_PROG, "unable to obtain local address");
         return false;
      }
   
      // init rpc service (will fork!)
      if (rpcInitializeServer (&rpcpmstart, _SVC_FG, _SVC_MODE,
                           &transp, &proto) < 0) {
         gdsError (GDS_ERR_PROG, "unable to start rpc service");
         return false;
      }
   
      // setup configuration server (creates 2nd thread!)
      static confServices	conf[1];
      static char               confbuf[256];
      conf[0].id = 0;
      conf[0].answer = stdAnswer;
      sprintf (confbuf, "launch * * %s %ld %ld",
              inet_ntoa (host), (unsigned long)RPC_PROGNUM_LAUNCH, 
              (unsigned long)RPC_PROGVER_LAUNCH);
      conf[0].user = confbuf;
      if (conf_server (conf, 1, 1) < 0) {
         gdsError (GDS_ERR_PROG, "unable to start configuration service");
         return false;
      }
   
      // register with rpc service
      if (rpcRegisterService (rpcpmstart, transp, proto, 
                           RPC_PROGNUM_LAUNCH, RPC_PROGVER_LAUNCH, 
                           rlaunchprog_1) != 0) {
         gdsError (GDS_ERR_PROG, 
                  "unable to register launch service");
         return false;
      }
   
      // start server
      printf ("Launch server (%x / %i)\n", 
             RPC_PROGNUM_LAUNCH, RPC_PROGVER_LAUNCH);
      rpcStartServer (rpcpmstart, &shutdownflag);
   
      // never reached
      return false;
   }


}
