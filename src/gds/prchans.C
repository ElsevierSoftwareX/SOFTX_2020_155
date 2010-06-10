//
//    Print a list of channels
//
#include <string.h>
#include <strings.h>
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include "DAQSocket.hh"
using namespace std;
static const int ListSz = 32000;

int 
main(int argc, const char *argv[]) {
    DAQDChannel* List = new DAQDChannel[ListSz];
    int grouplist[32], ngroup=0;

    const char*   ndsname = getenv("LIGONDSIP");
    const char*   format  = 0;
    if (!ndsname) ndsname = "131.215.115.67";
    bool debug = false, testdup = false;
    //---------------------------------  Get the arguments.
    for (int i=1 ; i<argc ; i++) {
        if (!strcmp(argv[i], "nds")) {
	    ndsname = argv[++i];
	} else if (!strcmp(argv[i], "-d")) {
	    testdup = true;
	} else if (!strcmp(argv[i], "-g")) {
	    grouplist[ngroup++] = strtol(argv[++i],0,0);
	} else if (!strcmp(argv[i], "-f")) {
	    format = argv[++i];
	} else if (!strcmp(argv[i], "debug")) {
	    debug = true;
	} else {
	    cerr << "Invalid argument: " << argv[i] << endl;
	    cerr << "Syntax: " << endl;
	    cerr << argv[0] << " [-d] [nds <nds-ip-address>]"
		 <<" [-f <format>] [-g <group>] [debug]" 
		 << endl;
	    return 1;
	}
    }

    //---------------------------------  Open a daqd socket
    cout << "Opening a socket to ND sever on node " << ndsname << endl;
    DAQSocket nds(ndsname);
    if (!nds.TestOpen()) {
        cerr << "Unable to open socket to NDS on node " << ndsname << endl;
	return 0;
    }
    nds.setDebug(debug);

    cout << "Getting available channels" << endl;
    int nc = nds.Available(List, ListSz);
    cout << "Number of channels returned = " << nc << endl;
    if (nc > ListSz) {
        cout << "Warning: List holds only " << ListSz << " entries." << endl;
	nc = ListSz;
    }

    //----------------------------------  Look for duplicates
    if (testdup) {
        for (int i=0 ; i < nc ; i++) {
	    for (int j=i+1 ; j < nc ; j++) {
	        if (!strcasecmp(List[i].mName, List[j].mName)) {
		    cout << "Error duplicate name: " << List[i].mName << endl;
		}
	    }
	}
    }

    //----------------------------------  Print the requested channel names
    if (!format) {
        cout << "Channel                                 "
	     << "Group Num     Rate  NByte  dType" << endl;
    }
    for (int i=0 ; i < nc ; i++) {
        bool select = false;
	if (!ngroup) select = true;
	else {
	    for (int j=0 ; j<ngroup ; j++) {
	        if (List[i].mGroup == grouplist[j]) select = true;
	    }
	}
	if (select) {
	    char line[1024];
	    if (format) sprintf(line, format, List[i].mName);
	    else        sprintf(line, "%-40s %3i %5i %6i %6i %6i", 
				List[i].mName,  List[i].mGroup, 
				List[i].mNum, List[i].mRate, 
				List[i].mBPS, List[i].mDatatype);
	    cout << line << endl;
	}
    }
    return 0;
}

