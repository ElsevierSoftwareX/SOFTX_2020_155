//
//    Print a list of channels
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include "DAQSocket.hh"
using namespace std;

int 
main(int argc, const char *argv[]) {
    const char*   ndsname  = getenv("LIGONDSIP");
    if (!ndsname) ndsname  = "131.215.115.67";
    bool debug = false;

    //-------------------------------------  Parse the arguments.
    for (int i=1 ; i<argc ; i++) {
        if (!strcmp(argv[i], "nds")) {
	    ndsname = argv[++i];
	} else if (!strcmp(argv[i], "debug")) {
	    debug = true;
	} else {
	    cerr << "Unrecognized argument: " << argv[i] << endl;
	    cerr << "The command syntax is: " << endl;
	    cerr << "  prndsvsn [nds <server-name>] [debug]" << endl;
	    return 1;
	}
    }

    //-------------------------------------  Open a socket to the NDS
    if (debug) cout << "Opening socket to " << ndsname << "..." << endl;
    DAQSocket nds;
    int ocode = nds.open(ndsname);
    if (ocode) {
        if (debug) {
	    cerr << "Error " << ocode << " occurred while opening socket." 
		 << endl;
	    perror("Error");
	}
	return 1;
    }
    nds.setDebug(debug);
    cout << "The software version for the " << ndsname 
	 << " server is: " << nds.Version() << endl;
    return 0;
}
