static const char *versionId = "Version $Id$" ;
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Daemon Name: remote program launcher					*/
/*                                                         		*/
/* Module Description: launcher daemon 					*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 22Apr02  D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: rlauncher.html					*/
/*	References: none						*/
/*                                                         		*/
/* Author Information:							*/
/* Name          Telephone       Fax             e-mail 		*/
/* Daniel Sigg   (509) 372-8132  (509) 372-8137  sigg_d@ligo.mit.edu	*/
/*                                                         		*/
/* Code Compilation and Runtime Specifications:				*/
/*	Code Compiled on: Ultra-Enterprise, Solaris 5.5.1		*/
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


/* Header File List: */
#include "dtt/launch_server.hh"
#include <iostream>

   using namespace diag;
   using namespace std;

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Main Program								*/
/*                                                         		*/
/* Description: calls channel_server					*/
/* 									*/
/*----------------------------------------------------------------------*/
   int main (int argc, char *argv[])
   {
      string		 conf;
   
      // get configuration information
      if ((argc > 1) && (strlen (argv[1]) > 0) && (argv[1][0] != '-')) {
         conf = string (argv[1]);
      }
      else {
         cerr << "usage: rlauncher 'config. file'" << endl;
         cerr << "Configuration file format:" << endl;
         cerr << "  'Title'  'Program'  'Command'  'Arguments'" << endl;
         cerr << "   Title: Program category (used by GUI)" << endl;
         cerr << "   Program: Program name (used by GUI)" << endl;
         cerr << "   Command: Program start command (used by launcher)" << endl;
         cerr << "   Arguments: Program arguments (used by launcher)" << endl;
         cerr << "   Use quotes when using muliple words in first three columns" << endl;
         cerr << "Configuration file format:" << endl;
         exit (0);
      }
   
   
      // install server
      launch_server (conf);
      return 0;
   }

