static const char *versionId = "Version $Id$" ;
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdsstring						*/
/*                                                         		*/
/* Module Description: implements functions for dealing with strings	*/
/* and the LIGO channel naming convention				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

#include "dtt/gdsstringcc.hh"
#include <cstdio>

namespace diag {
   using std::string;

   tempFilename::tempFilename () : string ("") {
      char		filename [L_tmpnam];
      this->assign (tmpnam (filename));
   }

}


