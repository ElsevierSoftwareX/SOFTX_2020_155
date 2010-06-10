/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: test channels						*/
/*                                                         		*/
/* Module Description: API for handling channel names			*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 8May99	  D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: testchn.html						*/
/*	References: none						*/
/*                                                         		*/
/* Author Information:							*/
/* Name          Telephone       Fax             e-mail 		*/
/* Daniel Sigg   (509) 372-8336  (509) 372-2178  sigg_d@ligo.mit.edu	*/
/*                                                         		*/
/* Code Compilation and Runtime Specifications:				*/
/*	Code Compiled on: Ultra-Enterprise, Solaris 5.6			*/
/*	Compiler Used: sun workshop C++ 4.2				*/
/*	Runtime environment: sparc/solaris				*/
/*                                                         		*/
/* Code Standards Conformance:						*/
/*	Code Conforms to: LIGO standards.	OK			*/
/*			  Lint.			TBD			*/
/*			  ANSI			TBD			*/
/*			  POSIX			TBD			*/
/*									*/
/* Known Bugs, Limitations, Caveats:					*/
/*								 	*/
/*									*/
/*                                                         		*/
/*                      -------------------                             */
/*                                                         		*/
/*                             LIGO					*/
/*                                                         		*/
/*        THE LASER INTERFEROMETER GRAVITATIONAL WAVE OBSERVATORY.	*/
/*                                                         		*/
/*                     (C) The LIGO Project, 1996.			*/
/*                                                         		*/
/*                                                         		*/
/* California Institute of Technology			   		*/
/* LIGO Project MS 51-33				   		*/
/* Pasadena CA 91125					   		*/
/*                                                         		*/
/* Massachusetts Institute of Technology		   		*/
/* LIGO Project MS 20B-145				   		*/
/* Cambridge MA 01239					   		*/
/*                                                         		*/
/* LIGO Hanford Observatory				   		*/
/* P.O. Box 1970 S9-02					   		*/
/* Richland WA 99352					   		*/
/*                                                         		*/
/* LIGO Livingston Observatory		   				*/
/* 19100 LIGO Lane Rd.					   		*/
/* Livingston, LA 70754					   		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

#ifndef _GDS_TESTCHN_H
#define _GDS_TESTCHN_H

/* Header File List: */
#include <string>
#include "dtt/gdsmain.h"
#include "dtt/gdschannel.h"
#include "dtt/gdsstring.h"

namespace diag {


/** @name Test Channel Name Utilities

    @memo Classes for handling channel names of diagnostics tests
    @author Written June 1998 by Daniel Sigg
    @version 0.1
 ************************************************************************/

/*@{*/

/** Class for handling channel names and channel information. This 
    object maintains default and mandatory identifiers for site and
    interferomter.

    @memo Class for channel handling.
    @author DS, May 99
 ************************************************************************/
   class channelHandler {
   public:
   
      /** Constructs a channel handler.
          @memo Default constructor.
   	  @param SiteDefault default site idenstifier
   	  @param SiteForce mandatory site identifier
   	  @param IfoDefault default ifo identifier
   	  @param IfoForce mandatory ifo identifier
       ******************************************************************/
      explicit channelHandler (char SiteDefault = SITE_PREFIX[0], 
                        char SiteForce = ' ', 
                        char IfoDefault = IFO_PREFIX[0], 
                        char IfoForce = ' ') {
         setSiteIfo (SiteDefault, SiteForce, IfoDefault, IfoForce);
      }
      /** Destructs a channel handler.
          @memo Destructor.
       ******************************************************************/
      virtual ~channelHandler() {
      }
      /** Initializes the default and mandatory site and interferometer
          prefixes.
          @memo Init method.
   	  @param SiteDefault default site idenstifier
   	  @param SiteForce mandatory site identifier
   	  @param IfoDefault default ifo identifier
   	  @param IfoForce mandatory ifo identifier
          @return true if successful
       ******************************************************************/
      bool setSiteIfo (char SiteDefault, char SiteForce,
                      char IfoDefault, char IfoForce);
   
      /** Obtaines channel information. Convinience function which 
          makes sure channel names are expanded correctly.
          @memo info method.
          @param name channel name
   	  @param info channel info
          @return true if successful
       ******************************************************************/
      virtual bool channelInfo (const string& name, 
                        gdsChnInfo_t& info) const;
   
      /** Returns an expanded channel name. The channel name is expanded
          by using the default and mandatory idenfiers for site and
          interferomter.
          @memo channel name method.
          @param name channel name
          @return expanded channel name
       ******************************************************************/
      virtual string channelName (const string& name) const;
   
      /// default site idenstifier
      char		siteDefault;
      /// mandatory site identifier
      char		siteForce;
      /// default ifo identifier
      char		ifoDefault;
      /// mandatory ifo identifier
      char		ifoForce;
   };


/** String class for a channel name. During construction the name
    is automatically expanded with default and mandatory identifiers 
    for site and interferomter.

    @memo String class for channel handling.
    @author DS, May 99
 ************************************************************************/
   class channelname : public string, public channelHandler {
   
   public:
   
      /** Constructs a channel name string.
          @memo Constructor.
   	  @param chnMgr channel handler
   	  @param name channel name
       ******************************************************************/
      channelname (const channelHandler& chnMgr, const char* name) 
      : string (chnMgr.channelName (name)), channelHandler (chnMgr) {
      }
   
      /** Constructs a channel name string.
          @memo Constructor.
   	  @param chnMgr channel handler
   	  @param name channel name
       ******************************************************************/
      channelname (const channelHandler& chnMgr, const string& name)
      : string (chnMgr.channelName (name)), channelHandler (chnMgr) {
      }
   
      /** Copies a normal string into a channel name string.
          @memo Assignment operator.
   	  @param name channel name
          @return reference to this
       ******************************************************************/
      channelname& operator= (const string& name) {
         assign (channelName (name));
         return *this;
      }
   };

/*@}*/

}
#endif /*_GDS_TESTCHN_H */
