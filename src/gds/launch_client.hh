/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: launch_client						*/
/*                                                         		*/
/* Module Description: Remote launch API				*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 28Apr02 D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: confinfo.html					*/
/*	References: none						*/
/*                                                         		*/
/* Author Information:							*/
/* Name          Telephone       Fax             e-mail 		*/
/* Daniel Sigg   (509) 372-8132  (509) 372-8137  sigg_d@ligo.mit.edu	*/
/*                                                         		*/
/* Code Compilation and Runtime Specifications:				*/
/*	Code Compiled on: Ultra-Enterprise, Solaris 5.6			*/
/*	Compiler Used: sun workshop C 4.2				*/
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
/* Caltech				MIT		   		*/
/* LIGO Project MS 51-33		LIGO Project NW-17 161		*/
/* Pasadena CA 91125			Cambridge MA 01239 		*/
/*                                                         		*/
/* LIGO Hanford Observatory		LIGO Livingston Observatory	*/
/* P.O. Box 159				19100 LIGO Lane Rd.		*/
/* Richland WA 99352			Livingston, LA 70754		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#ifndef _GDS_LAUNCH_CLIENT_H
#define _GDS_LAUNCH_CLIENT_H

#include <string>
#include <vector>

/** Launch client interface. The client interface to start programs
    on remote machines. The remote machine must be accessible on the
    local subnet or its address has to be specified explicilty.
   
    @memo Launch client interface
    @author Written April 2002 by Daniel Sigg
    @version 1.0
 ************************************************************************/
   class launch_client {
   public:
      /// Launch item
      struct item_t {
         /// Title/proogram category
         std::string	fTitle;
         /// Server address
         std::string	fAddr;
         /// Program name
         std::string	fProgram;
         /// Equal?
         bool operator== (const item_t& i) const;
         /// Less?
         bool operator< (const item_t& i) const;
      };
      /// Launch list
      typedef std::vector<item_t> list;
      /// Launch list iterator
      typedef list::const_iterator iterator;
   
      /// Create a launch client
      explicit launch_client (const char* server = 0);
      /// Destructs a launch client
      ~launch_client();
   
      /// Get local servers
      bool AddLocalServers();
      /// Add a server
      bool AddServer (const char* server = 0);
   
      /// Launch a program on remote server
      bool Launch (const item_t& item, const char* display = 0);
      /// Launch a program on remote server
      bool Launch (int index, const char* display = 0) {
         return Launch (fList[index], display); }
   
      /// Empty list?
      bool empty() const {
         return fList.empty(); }
      /// Size of list
      int size() const {
         return fList.size(); }
      /// Beginning of list
      iterator begin() const {
         return fList.begin(); }
      /// End of list
      iterator end() const {
         return fList.end(); }
      /// Direct list access
      const item_t& operator[] (int index) const {
         return fList[index]; }
   
      /// Set default display 
      void SetDefaultDisplay (const char* display) {
         fDisplay = display ? display : ""; }
      /// Get default display
      const char* GetDefaultDisplay() const {
         return fDisplay.c_str(); }
   
   protected:
      /// Default display
      std::string	fDisplay;
      /// List of available programs
      list		fList;
   };


#endif // _GDS_LAUNCH_CLIENT_H
