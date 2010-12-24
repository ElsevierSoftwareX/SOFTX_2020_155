static const char *versionId = "Version $Id$" ;
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: testchn							*/
/*                                                         		*/
/* Module Description: name handling for diagnostics test channels	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

/* Header File List: */

#include <time.h>
#include "dtt/testchn.hh"

namespace diag {
   using namespace std;


   bool channelHandler::setSiteIfo (char SiteDefault, char SiteForce, 
                     char IfoDefault, char IfoForce)
   {
      siteDefault = siteDefault;
      siteForce = SiteForce;
      ifoDefault = IfoDefault;
      ifoForce = IfoForce;
      return true;
   }


   bool channelHandler::channelInfo (const string& name, 
                     gdsChnInfo_t& info) const
   {
      return (gdsChannelInfo (channelName (name).c_str(), 
                             &info) == 0);
   }


   string channelHandler::channelName (const string& name) const
   {
      return name;
   }

}
