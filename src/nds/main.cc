#include <stdio.h>
#include <iostream>
#include "nds.hh"
//#include "framecpp/dictionary.hh"

using namespace CDS_NDS;

std::string programname; // Set to the program's executable name during run time

//
// Set log level 2 in the production system
//

int nds_log_level=4; // Controls volume of log messages
#ifndef NDEBUG
int _debug = 4; // Controls volume of the debugging messages that is printed out
#endif

// FrameCPP::Dictionary *dict = FrameCPP::library.getCurrentVersionDictionary();

main (int argc, char *argv[])
{
  if (argc != 2) {
    system_log(1, "nds usage: nds <pipe file name>");
    return 1;
  }

  programname = Nds::basename(argv [0]);
  openlog (programname.c_str(), LOG_PID | LOG_CONS, LOG_USER);
  Nds nds (argv[1]);
  int res = nds.run();
  return res;
}
