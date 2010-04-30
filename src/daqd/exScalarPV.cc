/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/

#include <math.h>
#include <limits.h>
#include <stdlib.h>

#include "exServer.h"
#include "gddApps.h"

#define myPI 3.14159265358979323846

//
// SUN C++ does not have RAND_MAX yet
//
#if !defined(RAND_MAX)
//
// Apparently SUN C++ is using the SYSV version of rand
//
#if 0
#define RAND_MAX INT_MAX
#else
#define RAND_MAX SHRT_MAX
#endif
#endif

//
// exScalarPV::scan
//
void exScalarPV::scan()
{
    caStatus        status;
    double          radians;
    smartGDDPointer pDD;
    double           newValue;
    float           limit;
    int             gddStatus;

    //
    // update current time (so we are not required to do
    // this every time that we write the PV which impacts
    // throughput under sunos4 because gettimeofday() is
    // slow)
    //
    this->currentTime = epicsTime::getCurrent();

    pDD = new gddScalar (gddAppType_value, aitEnumFloat64);
    if ( ! pDD.valid () ) {
        return;
    }

    //
    // smart pointer class manages reference count after this point
    //
    gddStatus = pDD->unreference();
    assert (!gddStatus);

    //radians = (rand () * 2.0 * myPI)/RAND_MAX;
    if ( this->pValue.valid () ) {
        this->pValue->getConvert(newValue);
    }
    else {
        newValue = 0.0f;
    }
    if (this->info.valPtr) {
      newValue = this->info.valPtr[0];
//	printf("newIntValue=%u\n", this->info.valPtr[0]);
    } else {
	newValue = 0;
//      newValue += (float) (sin (radians) / 10.0);
    }
#if 0
    limit = (float) this->info.getHopr();
    newValue = tsMin (newValue, limit);
    limit = (float) this->info.getLopr();
    newValue = tsMax (newValue, limit);
#endif
    *pDD = newValue;
//printf("newValue=%f\n", newValue);
    aitTimeStamp gddts = this->currentTime;
    pDD->setTimeStamp (&gddts);
    status = this->update (pDD);
    if (status!=S_casApp_success) {
        errMessage (status, "scalar scan update failed\n");
    }
}

//
// exScalarPV::updateValue ()
//
// NOTES:
// 1) This should have a test which verifies that the 
// incoming value in all of its various data types can
// be translated into a real number?
// 2) We prefer to unreference the old PV value here and
// reference the incomming value because this will
// result in each value change events retaining an
// independent value on the event queue.
//
caStatus exScalarPV::updateValue (smartConstGDDPointer pValueIn)
{
    if ( ! pValueIn.valid () ) {
        return S_casApp_undefined;
    }

    //
    // Really no need to perform this check since the
    // server lib verifies that all requests are in range
    //
    if (!pValueIn->isScalar()) {
        return S_casApp_outOfBounds;
    }

    this->pValue = pValueIn;

    return S_casApp_success;
}

