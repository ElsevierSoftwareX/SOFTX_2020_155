// ---------------------------------------------------
// File: commData.h
// Date: Tuesday, October 06 2009
// Author: Matthew Evans
//
// see commData.c
// ---------------------------------------------------

#ifndef __COMMDATA_H__
#define __COMMDATA_H__

enum CommType
{
  COMM_SEND = 0,
  COMM_RECV = 1
};
typedef enum CommType CommType;

struct CommState
{
  CommType commType;          // communication type
  int Nsamp;                  // up/down-sample ratio = 2 for OMC to LSC

  unsigned int* rfmCounter;   // pointer to RFM counter
  double* rfmData;            // pointer to RFM data
  int dataLength;             // number of doubles to send

  unsigned int waitCounter;   // up/down-sample wait cycle counter
  unsigned int cycleCounter;   // next expected value of RFM counter

  unsigned int errCounter;    // running error counter (epics reset?)
};
typedef struct CommState CommState;

// this union is used to make the checksum
// by assigning doubles to val, and then doing
// bitwise operations on bits[0] and bits[1]
union CommDataCheckSum
{
  double val;
  unsigned int bits[2];
};
typedef union CommDataCheckSum CommDataCheckSum;

// init function
void commDataInit(CommState* lcl, CommType commType,
		  int Nsamp, unsigned int* rfmCounter,
		  double* rfmData, int dataLength);

// decide between inline or not for commData
#ifndef COMMDATA_INLINE
int commData(CommState* lcl, double* data);
int commDataRecv(CommState* lcl, double* data);
int commDataSend(CommState* lcl, double* data);
#else
#include "../fe/commData.c"
#endif

#endif // __COMMDATA_H__
