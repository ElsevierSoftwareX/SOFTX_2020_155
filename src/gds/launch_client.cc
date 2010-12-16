static char *versionId = "Version $Id$" ;
#ifndef __EXTENSIONS__
#define __EXTENSIONS__
#endif 
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include "dtt/launch_client.hh"
#include "dtt/confinfo.h"
//#include "dtt/rpcinc.h"
#include "dtt/rlaunch.h"


   using namespace std;

   const char* net_id = "tcp";
   const double autoconf_timeout = 1.0;


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// launch_client::item_t                                                //
//                                                                      //
// program item                                                         //
//                                                                      //
//////////////////////////////////////////////////////////////////////////
   bool launch_client::item_t::operator== (const item_t& i) const
   {
      return ((strcasecmp (fTitle.c_str(), i.fTitle.c_str()) == 0) &&
             (strcasecmp (fAddr.c_str(), i.fAddr.c_str()) == 0) &&
             (strcasecmp (fProgram.c_str(), i.fProgram.c_str()) == 0));
   }

//______________________________________________________________________________
   bool launch_client::item_t::operator< (const item_t& i) const
   {
      int cmp1 = strcasecmp (fTitle.c_str(), i.fTitle.c_str());
      if (cmp1 != 0) {
         return cmp1 < 0;
      }
      int cmp2 = strcasecmp (fAddr.c_str(), i.fAddr.c_str());
      if (cmp2 != 0) {
         return cmp2 < 0;
      }
      int cmp3 = strcasecmp (fProgram.c_str(), i.fProgram.c_str());
      return cmp3 < 0;
   }



//////////////////////////////////////////////////////////////////////////
//                                                                      //
// launch_client	                                                //
//                                                                      //
// Remote launch API                                                    //
//                                                                      //
//////////////////////////////////////////////////////////////////////////
   launch_client::launch_client (const char* server)
   {
      AddLocalServers();
      if (server) AddServer (server);
   }

//______________________________________________________________________________
   launch_client::~launch_client()
   {
   }

//______________________________________________________________________________
   bool launch_client::AddLocalServers()
   {
      char* buf = new char[2*1024];
      const char* const* conf =
         getConfInfo_r (0, autoconf_timeout, buf, 2*1024);
      if (conf == 0) {
         delete [] buf;
         return false;
      }
      confinfo_t crec;
      for (const char* const* p = conf; *p; ++p) {
         if ((parseConfInfo (*p, &crec) == 0) &&
            (strcasecmp (crec.interface, CONFIG_SERVICE_LAUNCH) == 0)) {
            AddServer (crec.sender);
         }
      }
   
      delete [] buf;
      return true;
   }

//______________________________________________________________________________
   bool launch_client::AddServer (const char* server)
   {
      if (!server) {
         return false;
      }
      resultLaunchInfoQuery_r	res;		// rpc return
      static CLIENT*		clnt;		// rpc client handle
      struct timeval 		timeout;	// connect timeout
   
      // Call launch server
      timeout.tv_sec = RPC_PROBE_WAIT;
      timeout.tv_usec = 0;
      memset (&res, 0, sizeof (res));
      if (!rpcProbe (server, RPC_PROGNUM_LAUNCH, RPC_PROGVER_LAUNCH,
                    net_id, &timeout, &clnt) ||
         (launchquery_1 (&res, clnt) != RPC_SUCCESS) ||
         (res.status != 0)) {
         return false;
      }
      clnt_destroy (clnt);
   
      // Add returned title/programs to list
      for (int i = 0; i < (int)res.list.launch_infolist_r_len; ++i) {
         item_t item;
         item.fTitle = res.list.launch_infolist_r_val[i].title;
         item.fProgram = res.list.launch_infolist_r_val[i].prog;
         item.fAddr = server;
         list::iterator f = lower_bound (fList.begin(), fList.end(), item);
         if ((f != fList.end()) && (*f == item)) {
            *f = item;
         }
         else {
            fList.insert (f, item);
         }
         //fList.push_back (item);
      }
      xdr_free ((xdrproc_t)xdr_resultLaunchInfoQuery_r, 
               (char*) &res);
   
      return true;
   }

//______________________________________________________________________________
   bool launch_client::Launch (const item_t& item, const char* display)
   {
      int			res;		// result
      static CLIENT*		clnt;		// rpc client handle
      struct timeval 		timeout;	// connect timeout
   
      // get display
      string displ = display ? display : fDisplay.c_str();
      if (displ.empty()) displ = ":0.0";
      // disable xhost
      string xhost = "xhost +" + item.fAddr;
      ::system (xhost.c_str());
      // launch program
      timeout.tv_sec = RPC_PROBE_WAIT;
      timeout.tv_usec = 0;
      if (!rpcProbe (item.fAddr.c_str(), RPC_PROGNUM_LAUNCH, 
                    RPC_PROGVER_LAUNCH, net_id, &timeout, &clnt) ||
         (launch_1 ((char*)item.fTitle.c_str(), 
                   (char*)item.fProgram.c_str(),
                   (char*)displ.c_str(), &res, clnt) != RPC_SUCCESS) ||
         (res != 0)) {
         return false;
      }
      clnt_destroy (clnt);
      return true;
   }
