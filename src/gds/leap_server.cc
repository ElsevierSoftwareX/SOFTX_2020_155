static char *versionId = "Version $Id$" ;
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: channel_server						*/
/*                                                         		*/
/* Module Description: implements server functions for providing  	*/
/* channel information for the diagnostics system			*/
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
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <iostream>
#include <fstream>
#include "dtt/gdsutil.h"
#include "dtt/rpcinc.h"
#include "gmutex.hh"
#include "dtt/rleap.h"
#include "dtt/leap_server.hh"


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
#define _SVC_FG		0
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
   typedef vector<leap_r>	leaplist;

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Globals: servermux		protects globals			*/
/*          list		channel list				*/
/*          shutdownflag	shutdown flag				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static readwritelock		servermux;
   static leaplist		list;
   static int			shutdownflag = 0;


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Forward declarations: 						*/
/*      rleap_1			rpc dispatch function			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
extern "C" void rleapprog_1 (struct svc_req* rqstp, 
                     register SVCXPRT* transp);


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Remote Procedure Name: leapquery_1_svc				*/
/*                                                         		*/
/* Procedure Description: leap second query				*/
/*                                                         		*/
/* Procedure Arguments: leap second list (result), svc request handle	*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if not (status)		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
extern "C" 
   bool_t leapquery_1_svc (resultLeapQuery_r* result, 
                     struct svc_req* rqstp)
   {
      rpcSetServerBusy (1);
      semlock		lockit (servermux);
   
      // copy list into result
      result->leaplist.leaplist_r_len = list.size();
      result->leaplist.leaplist_r_val = (leap_r*) 
         calloc (list.size(), sizeof (leap_r));
      if (result->leaplist.leaplist_r_val == 0) {
         result->status = -1;
      }
      else {
         int i = 0;
         for (leaplist::iterator iter = list.begin();
             iter != list.end(); iter++, i++) {
            result->leaplist.leaplist_r_val[i] = *iter;
         }
         result->status = 0;
      }
      return TRUE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: rleap_1_freeresult				*/
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
   int rleapprog_1_freeresult (SVCXPRT* transp, 
                     xdrproc_t xdr_result, caddr_t result)
   {
      (void) xdr_free (xdr_result, result);
      return TRUE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: readLeapFile				*/
/*                                                         		*/
/* Procedure Description: reads leap info from file			*/
/* 			  mutex MUST be owned by caller			*/
/*                                                         		*/
/* Procedure Arguments: filename					*/
/*                                                         		*/
/* Procedure Returns: true if successful, false otherwise.		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static bool readLeapFile (const string& filename)
   {
      leap_r		info;		// leap second
      ifstream		in (filename.c_str());	// leap info file
      string		line;
   
      // open config file
      if (!in) {
         return false;
      }
   
      // loop through configuration file
      while (getline (in, line)) {
         // skip leading blanks
         while (!line.empty() && 
               ((line[0] == ' ') || (line[0] == '\t'))) {
            line.erase (0, 1);
         }
         // skip empty lines and comments
         if (line.empty() || (line[0] == '#')) {
            continue;
         }
         // read in parameters
         if (!(istringstream (line) >> info.transition >> info.change)) { 
            in.close ();
            return false;
         }
         // now add elements 
         list.push_back (info);
      }
   
      // close the file
      in.close ();
      return true;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: leap_server					*/
/*                                                         		*/
/* Procedure Description: start rpc service task for leap second server	*/
/*                                                         		*/
/* Procedure Arguments: configuration file				*/
/*                                                         		*/
/* Procedure Returns: does not return if successful, <0 otherwise	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   bool leap_server (const string& config)
   {
      int		rpcpmstart;	// port monitor flag
      SVCXPRT*		transp;		// service transport
      int		proto;		// protocol
   
      // read configuration files
      servermux.writelock();
      if (!readLeapFile (config)) {
         servermux.unlock();
         gdsError (GDS_ERR_PROG, 
                  "unable to load leap second configuration file");
         return false;
      }
      servermux.unlock();
   
      // init rpc service
      if (rpcInitializeServer (&rpcpmstart, _SVC_FG, _SVC_MODE,
                           &transp, &proto) < 0) {
         gdsError (GDS_ERR_PROG, "unable to start rpc service");
         return false;
      }
   
      // register with rpc service
      if (rpcRegisterService (rpcpmstart, transp, proto, 
                           RPC_PROGNUM_GDSLEAP, RPC_PROGVER_GDSLEAP, 
                           rleapprog_1) != 0) {
         gdsError (GDS_ERR_PROG, 
                  "unable to register leap second service");
         return false;
      }
      printf ("Leap second infromation server (%x / %i)\n", 
             RPC_PROGNUM_GDSLEAP, RPC_PROGVER_GDSLEAP);
   
      // start server
      rpcStartServer (rpcpmstart, &shutdownflag);
   
      // never reached
      return false;
   
   }


}
