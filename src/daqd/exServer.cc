/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
//
// fileDescriptorManager.process(delay);
// (the name of the global symbol has leaked in here)
//

//
// Example EPICS CA server
//
#include "exServer.h"
#include "config.h"
#include "../../src/include/daqmap.h"
//#include "daqd.hh"
//extern daqd_c daqd;

char epicsDcuName[DCU_COUNT][40];

unsigned int pvValue[128] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
				0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

// First subscript is the variable index:
// DCU status is the first element
// CRC error counter is the second element.
// CRC accumulated error counter is the third element.
// Second subscript is the ifo number.
unsigned int epicsDcuStatus[3][2][DCU_COUNT];

//
// static list of pre-created PVs
//
pvInfo exServer::pvList[] = {
    pvInfo (1, "CYCLE", 0xffffffff, 0, excasIoSync, 1, pvValue),
    pvInfo (1, "TOTAL_CHANS", 0xffffffff, 0, excasIoSync, 1, pvValue+1),
    pvInfo (1, "DATA_RATE", 0xffffffff, 0, excasIoSync, 1, pvValue+2),
    pvInfo (1, "EDCU_CHANS", 0xffffffff, 0, excasIoSync, 1, pvValue+3),
    pvInfo (1, "EDCU_CONN_CHANS", 0xffffffff, 0, excasIoSync, 1, pvValue+4),
    pvInfo (1, "UPTIME_SECONDS", 0xffffffff, 0, excasIoSync, 1, pvValue+5),
    pvInfo (1, "LOOKBACK_RAM",  0xffffffff, 0, excasIoSync, 1, pvValue+6),
    pvInfo (1, "LOOKBACK_FULL",  0xffffffff, 0, excasIoSync, 1, pvValue+7),
    pvInfo (1, "LOOKBACK_DIR",  0xffffffff, 0, excasIoSync, 1, pvValue+8),
    pvInfo (1, "LOOKBACK_STREND",  0xffffffff, 0, excasIoSync, 1, pvValue+9),
    pvInfo (1, "LOOKBACK_STREND_DIR",  0xffffffff, 0, excasIoSync, 1, pvValue+10),
    pvInfo (1, "LOOKBACK_MTREND",  0xffffffff, 0, excasIoSync, 1, pvValue+11),
    pvInfo (1, "LOOKBACK_MTREND_DIR",  0xffffffff, 0, excasIoSync, 1, pvValue+12),
    pvInfo (1, "FAST_DATA_CRC",  0xffffffff, 0, excasIoSync, 1, pvValue+13),
    pvInfo (1, "FAULT",  0xffffffff, 0, excasIoSync, 1, pvValue+14),
    pvInfo (1, "BCAST_RETR",  0xffffffff, 0, excasIoSync, 1, pvValue+15),
    pvInfo (1, "BCAST_FAILED_RETR",  0xffffffff, 0, excasIoSync, 1, pvValue+16),
    pvInfo (0.5, "GPS",  0xffffffff, 0, excasIoSync, 1, pvValue+17),
    pvInfo (1, "CHANS_SAVED", 0xffffffff, 0, excasIoSync, 1, pvValue+18),
    pvInfo (1, "FRAME_SIZE", 0xffffffff, 0, excasIoSync, 1, pvValue+19),
    pvInfo (1, "SCIENCE_FRAME_SIZE", 0xffffffff, 0, excasIoSync, 1, pvValue+20),
    pvInfo (1, "SCIENCE_TOTAL_CHANS", 0xffffffff, 0, excasIoSync, 1, pvValue+21),
    pvInfo (1, "SCIENCE_CHANS_SAVED", 0xffffffff, 0, excasIoSync, 1, pvValue+22),
};

const unsigned exServer::pvListNElem = NELEMENTS (exServer::pvList);

//
// exServer::exServer()
//
exServer::exServer ( const char * const pvPrefix,
		     const char * const pvPrefix1,
		     const char * const pvPrefix2,
		     unsigned aliasCount, bool scanOnIn ) : 
    simultAsychIOCount (0u),
    scanOn (scanOnIn)
{
    unsigned i, j;
    exPV *pPV;
    pvInfo *pPVI;
    pvInfo *pPVAfter = &exServer::pvList[pvListNElem];
    char pvAlias[256];
    const char * const pNameFmtStr = "%.100s%.40s";
    const char * const pDcuNameFmtStr[3] = { "%.100s%.40s_STATUS", 
    					     "%.100s%.40s_CRC_CPS",
    					     "%.100s%.40s_CRC_SUM" };
    const char * const pAliasFmtStr = "%.100s%.40s%u";

    exPV::initFT();

    //
    // pre-create all of the simple PVs that this server will export
    //
    for (pPVI = exServer::pvList; pPVI < pPVAfter; pPVI++) {
        pPV = pPVI->createPV (*this, true, scanOnIn);
        if (!pPV) {
            fprintf(stderr, "Unable to create new PV \"%s\"\n",
                pPVI->getName());
        }


        //
        // Install canonical (root) name
        //
        sprintf(pvAlias, pNameFmtStr, pvPrefix, pPVI->getName());
        this->installAliasName(*pPVI, pvAlias);
    }


    // Create DCU status channels
    for (i = 0; i < DCU_COUNT; i++) {
      for (j = 0; j < 3; j++) {
	if (epicsDcuName[i][0] == 0) continue;
        sprintf(pvAlias, pDcuNameFmtStr[j], pvPrefix1, epicsDcuName[i]);
        pvInfo *pPVI = new pvInfo (1, pvAlias, 0xffffffff, 0, excasIoSync, 1, epicsDcuStatus[j][0] + i);
        pPV = pPVI->createPV (*this, true, scanOnIn);
        if (!pPV) {
            fprintf(stderr, "Unable to create new PV \"%s\"\n",
                pPVI->getName());
	    continue;
        }
	this->installAliasName(*pPVI, pvAlias);
	printf("Creating %s\n", pvAlias);
      }
    }
    
    if (strcmp(pvPrefix2,"")) {
      for (i = 0; i < DCU_COUNT; i++) {
	for (j = 0; j < 3; j++) {
	  if (epicsDcuName[i][0] == 0) continue;
	  sprintf(pvAlias, pDcuNameFmtStr[j], pvPrefix2, epicsDcuName[i]);
	  pvInfo *pPVI = new pvInfo (1, pvAlias, 0xffffffff, 0, excasIoSync, 1, epicsDcuStatus[j][1] + i);
	  pPV = pPVI->createPV (*this, true, scanOnIn);
	  if (!pPV) {
            fprintf(stderr, "Unable to create new PV \"%s\"\n",
		    pPVI->getName());
	    continue;
	  }
	  this->installAliasName(*pPVI, pvAlias);
	}
      }
    }
}

//
// exServer::~exServer()
//
exServer::~exServer()
{
    pvInfo *pPVI;
    pvInfo *pPVAfter = 
        &exServer::pvList[NELEMENTS(exServer::pvList)];

    //
    // delete all pre-created PVs (eliminate bounds-checker warnings)
    //
    for (pPVI = exServer::pvList; pPVI < pPVAfter; pPVI++) {
        pPVI->deletePV ();
    }
    
    this->stringResTbl.traverse ( &pvEntry::destroy );
}

//
// exServer::installAliasName()
//
void exServer::installAliasName(pvInfo &info, const char *pAliasName)
{
    pvEntry *pEntry;

    pEntry = new pvEntry(info, *this, pAliasName);
    if (pEntry) {
        int resLibStatus;
        resLibStatus = this->stringResTbl.add(*pEntry);
        if (resLibStatus==0) {
            return;
        }
        else {
            delete pEntry;
        }
    }
    fprintf(stderr, 
"Unable to enter PV=\"%s\" Alias=\"%s\" in PV name alias hash table\n",
        info.getName(), pAliasName);
}

//
// exServer::pvExistTest()
//
pvExistReturn exServer::pvExistTest // X aCC 361
    (const casCtx& ctxIn, const char *pPVName)
{
    //
    // lifetime of id is shorter than lifetime of pName
    //
    stringId id(pPVName, stringId::refString);
    pvEntry *pPVE;

    //
    // Look in hash table for PV name (or PV alias name)
    //
    pPVE = this->stringResTbl.lookup(id);
    if (!pPVE) {
        return pverDoesNotExistHere;
    }

    pvInfo &pvi = pPVE->getInfo();

    //
    // Initiate async IO if this is an async PV
    //
    if (pvi.getIOType() == excasIoSync) {
        return pverExistsHere;
    }
    return pverDoesNotExistHere;
}

//
// exServer::pvAttach()
//
pvAttachReturn exServer::pvAttach // X aCC 361
    (const casCtx &ctx, const char *pName)
{
    //
    // lifetime of id is shorter than lifetime of pName
    //
    stringId id(pName, stringId::refString); 
    exPV *pPV;
    pvEntry *pPVE;

    pPVE = this->stringResTbl.lookup(id);
    if (!pPVE) {
        return S_casApp_pvNotFound;
    }

    pvInfo &pvi = pPVE->getInfo();

    //
    // If this is a synchronous PV create the PV now 
    //
    if (pvi.getIOType() == excasIoSync) {
        pPV = pvi.createPV(*this, false, this->scanOn);
        if (pPV) {
            return *pPV;
        }
        else {
            return S_casApp_noMemory;
        }
    }
}

//
// pvInfo::createPV()
//
exPV *pvInfo::createPV ( exServer & /*cas*/,
                         bool preCreateFlag, bool scanOn )
{
    if (this->pPV) {
        return this->pPV;
    }

    exPV *pNewPV;

    //
    // create an instance of the appropriate class
    // depending on the io type and the number
    // of elements
    //
    if (this->elementCount==1u) {
        switch (this->ioType){
        case excasIoSync:
            pNewPV = new exScalarPV ( *this, preCreateFlag, scanOn );
            break;
        default:
            pNewPV = NULL;
            break;
        }
    }
    else {
        if ( this->ioType == excasIoSync ) {
            pNewPV = new exVectorPV ( *this, preCreateFlag, scanOn );
        }
        else {
            pNewPV = NULL;
        }
    }
    
    //
    // load initial value (this is not done in
    // the constructor because the base class's
    // pure virtual function would be called)
    //
    // We always perform this step even if
    // scanning is disable so that there will
    // always be an initial value
    //
    if (pNewPV) {
        this->pPV = pNewPV;
        pNewPV->scan();
    }

    return pNewPV;
}

//
// exServer::show() 
//
void exServer::show (unsigned level) const
{
    //
    // server tool specific show code goes here
    //
    this->stringResTbl.show(level);

    //
    // print information about ca server libarary
    // internals
    //
    this->caServer::show(level);
}

