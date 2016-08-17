/*
 * This file just holds the basic backing store for the epics pv functions.
 * It is meant to be included in all builds of daqd, so that the epics_pvs.hh
 * file can be included w/o regards to whether or not EPICS is being actually
 * being used.
 */
#include "epics_pvs.hh"

unsigned int pvValue[PV::MAX_PV];
