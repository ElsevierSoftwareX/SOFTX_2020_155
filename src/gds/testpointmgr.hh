/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: testpointmgr.h						*/
/*                                                         		*/
/* Module Description: iteration class for repeating tests		*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 30Apr99  D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: testpointmgr.html					*/
/*	References: none						*/
/*                                                         		*/
/* Author Information:							*/
/* Name          Telephone       Fax             e-mail 		*/
/* Daniel Sigg   (509) 372-8132  (509) 372-8137  sigg_d@ligo.mit.edu	*/
/*                                                         		*/
/* Code Compilation and Runtime Specifications:				*/
/*	Code Compiled on: Ultra-Enterprise, Solaris 5.6			*/
/*	Compiler Used: sun workshop C++ 4.2				*/
/*	Runtime environment: sparc/solaris				*/
/*                                                         		*/
/* Code Standards Conformance:						*/
/*	Code Conforms to: LIGO standards.	OK			*/
/*			  Lint.			TBD			*/
/*			  ANSI			OK			*/
/*			  POSIX			OK			*/
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

#ifndef _GDS_TESTPOINTMGR_H
#define _GDS_TESTPOINTMGR_H

/* Header File List: */
#include <utility>
#include <map>
#include "dtt/gdsstring.h"
#include "dtt/gdstask.h"
#include "gmutex.hh"
#include "dtt/testpointinfo.h"

namespace diag {


/** Test point manager. This object implements a test point handler
    object which can be used to manage a list of test points. It 
    works in two steps: first, test points have to be added to the list,
    second, they can be set active. Test points can be cleared with a
    lazy option, i.e. they are not immediately removed from the internal
    list, so that a successive set can be completed more efficiently.

    Usage: In general, a diagnostics test should interact with the
    test point manager directly, but rather rely on the excitation
    and the RTDD manager to handle test points. On the 
    other hand a diagnostics supervisory task should use set and clear
    to activate and deactivate the test points.
    
    @memo Object for manageing test points
    @author Written December 1998 by Daniel Sigg
    @version 0.1
 ************************************************************************/
   class testpointMgr {
   public:
      /** Constructs a test point management object.
   	  @memo Default constructor
          @param Lazytime Time to wait for cleanup after a lazy clear
       ******************************************************************/
      explicit testpointMgr (double Lazytime = 0);
   
      /** Destructs a test point management object.
          @memo Destructor.
       ******************************************************************/
      ~testpointMgr ();
   
      /** Set state of test point manager. If the state is set inactive
          (false), test points will not be physically selected. 
          Everything else will work identical. If set inactive while
          test points are still active, a clear(false) is performed
          first.
          @memo Set state method.
          @param active set active if true
       ******************************************************************/
      void setState (bool Active);
   
      /** Returns true if all test points are set active.
          @memo Are testpoints set method.
          @return true if set
       ******************************************************************/
      bool areSet () const;
   
      /** Returns true if all test points are in use.
          @memo Are testpoints used method.
          @return true if set
       ******************************************************************/
      bool areUsed () const;
   
      /** Sets test points by sending a request to the test point API.
          @memo Set method.
          @param active time when test points become active
          @return true if successful
       ******************************************************************/
      bool set (tainsec_t* activeTime = 0);
   
      /** Clears test points by sending a request to the test point API.
          @memo Clear method.
          @param lazy lazy clear if true
          @return true if successful
       ******************************************************************/
      bool clear (bool lazy = false);
   
      /** Adds a test points to the internal list.
          @memo Add method.
          @param chnname name of test point
          @return true if successful
       ******************************************************************/
      bool add (const string& chnname);
   
      /** Deletes a test point from the internal list.
          @memo Delete method.
          @param chnname name of test point
          @return true if successful
       ******************************************************************/
      bool del (const string& chnname);
   
      /** Deletes all test point from the internal list.
          @memo Delete method.
          @return true if successful
       ******************************************************************/
      bool del ();
   
   protected:
      /** Test point information object. This object stores information
          associated with a test point.
    
          @memo Object for storing test point information
          @author Written December 1998 by Daniel Sigg
          @version 0.1
       ******************************************************************/
      class testpointinfo {
      public:
         /// name of test point
         string			name;
         /// in use count
         int			inUse;
         /// true if set
         bool 			isSet;
      
         /** Constructs a test point information object.
             @memo Constructor
             @param Name name of test point
          ***************************************************************/
         explicit testpointinfo (const string& Name) : 
         name (Name), inUse (1), isSet (false) {
         }
         /** Constructs a test point information object.
             @memo Copy constructor
             @param Name name of test point
          ***************************************************************/
         testpointinfo (const testpointinfo& tpinfo) {
            *this = tpinfo;
         }
      };
   
      /// Denotes a node/testpoint ID pair
      typedef std::pair <int, testpoint_t> nodetestpoint;
      /// Internal list of test points
      typedef std::map <nodetestpoint, testpointinfo> testpointrecord;
   
      /// mutex to protect object
      mutable thread::recursivemutex		mux;
      /// Active state of manager
      bool				active;
      /// list of test points
      testpointrecord			testpoints;
      /// time to keep test points around after clear/delete
      double				lazytime;
      /// time when a lazy clear occured (0 indicates no clear)
      double				cleartime;
   
      /** Cleanup task. Clears testpoints if lazy delay has expired.
          @memo Cleanup method.
       ******************************************************************/
      void cleanup ();
   
   private:
      // prevent copy
      testpointMgr (const testpointMgr&);
      testpointMgr& operator= (const testpointMgr&);
      // ID of cleanup task
      taskID_t		cleanTID;
      // cleanup task
      static int cleanuptask (testpointMgr& tpMgr);
   };


}
#endif // _GDS_TESTPOINTMGR_H
