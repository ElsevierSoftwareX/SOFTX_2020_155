//
//    Print a list of channels
//
#include <stdlib.h>
#include <iostream>
#include "DAQSocket.hh"
#include "SigFlag.hh"
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
	    cerr << "  prnames [nds <server-name>] [debug]" << endl;
	    return 1;
	}
    }

    //-------------------------------------  Open a socket to the NDS
    cout << "Opening a socket to ND server on " << ndsname << endl;
    DAQSocket nds(ndsname);
    if (!nds.TestOpen()) {
        cerr << "Unable to open socket to NDS on node " << ndsname << endl;
	return 0;
    }
    nds.setDebug(debug);
    cout << "Request file names" << endl;
    int rc = nds.RequestNames();
    if (rc) {
        cerr << "prnames: File name request failed, rc = " << rc << endl;
	return 0;}

    //-------------------------------------  Get a terminator.
    SigFlag term(SIGTERM);
    term.add(SIGINT);

    //-------------------------------------  Loop over file names
    char filename[1024];
    while (!term) {
        if (nds.WaitforData() > 0) {
	    rc = nds.GetName(filename, sizeof(filename));
	    if (rc > 0) {
	        cout << filename << endl;
	    } else {
	        cerr << "Error receiving frame-file name, rc = " << rc << endl;
	    }
	}
    }

    return 0;
}
