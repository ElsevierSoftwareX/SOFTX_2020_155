/* ---------------------------------------------------
   File: commData.c
   Date: Tuesday, October 06 2009
   Author: Matthew Evans & Tobin Fricke
 
   This function is used to communicated between real-time front-end
   machines.  A cycle counter is maintained to verify that the sender
   and receiver remain synchronized, and a checksum is maintained to
   verify that the entire block of data transmitted is
   consistent. 

   No attempt is made to deal with endianness issues, nor to correct
   any errors which are detected.
---------------------------------------------------*/

#include <stdio.h>
#include "commData.h"

static const unsigned int CYCLE_MASK = 0x0000ffff; // bits used for cycle number
static const unsigned int CSUM_MASK  = 0xffff0000;  
static const unsigned int CSUM_SHIFT = 16;         // bit shift for check sum folding

unsigned int crc_ptr(char*, unsigned int, unsigned int);
unsigned int crc_len(unsigned int, unsigned int);

// init function
void commDataInit(CommState* lcl, CommType commType,
		  int Nsamp, unsigned int* rfmCounter,
		  double* rfmData, int dataLength) {
  lcl->commType = commType;
  lcl->Nsamp = Nsamp;
  lcl->rfmCounter = rfmCounter;
  lcl->rfmData = rfmData;
  lcl->dataLength = dataLength;

  // init counters
  lcl->waitCounter = 0;
  lcl->cycleCounter = 0;
  lcl->errCounter = 0;
}

int commDataRecv(CommState* state, double* data) {
  //assert(state->commType == COMM_RECV);
  int ii; // counter used in for-loops

  // Wait for the correct cycle
  if (state->waitCounter) {
    state->waitCounter --;
    return 0;
  }

  // Perform the read 
  unsigned int rfmCounter = *(state->rfmCounter);
  for (ii=0; ii<(state->dataLength); ii++) {
    data[ii] = state->rfmData[ii];
  }
  
  // Unpack
  unsigned int cycle = (rfmCounter & CYCLE_MASK);
  unsigned int checksum = (rfmCounter & CSUM_MASK) >> CSUM_SHIFT;

  // Compute the checksum 
  unsigned int calculatedChecksum = 0;
  calculatedChecksum = crc_ptr((char *)&cycle, sizeof(cycle), calculatedChecksum);
  calculatedChecksum = crc_ptr((char *)data, state->dataLength * sizeof(double), calculatedChecksum);
  calculatedChecksum = crc_len(state->dataLength * sizeof(double) + sizeof(cycle), calculatedChecksum);

  // Verify the checksum and counter
  int isError = 0;
  if (cycle != ((state->cycleCounter + 1) & CYCLE_MASK)) {
    isError = 1;
  }

  if (checksum != (calculatedChecksum & (CSUM_MASK >> CSUM_SHIFT))) {
    isError = 1;
  }

  // Update the stored state
  state->cycleCounter = cycle;
  state->waitCounter = state->Nsamp - 1;
  if (isError) 
    state->errCounter++;

  return isError;
}


int commDataSend(CommState* state, double* data) {
  //assert(state->commType == COMM_SEND);
  int ii; // counter used in for-loops

  // Wait for the correct cycle
  if (state->waitCounter) {
    state->waitCounter --;
    return 0;
  }

  // increment the cycle counter
  state->cycleCounter = (state->cycleCounter + 1) & CYCLE_MASK;

  // Compute the checksum 
  unsigned int calculatedChecksum = 0;
  calculatedChecksum = crc_ptr((char *)&(state->cycleCounter), sizeof(state->cycleCounter), calculatedChecksum);
  calculatedChecksum = crc_ptr((char *)data, state->dataLength * sizeof(double), calculatedChecksum);
  calculatedChecksum = crc_len(state->dataLength * sizeof(double) + sizeof(state->cycleCounter), calculatedChecksum);

  // Pack the cycle number and checksum together
  unsigned int rfmCounter = ((calculatedChecksum << CSUM_SHIFT) & CSUM_MASK) | (state->cycleCounter & CYCLE_MASK);

  // Perform the write
  *(state->rfmCounter) = rfmCounter;
  for (ii=0; ii<(state->dataLength); ii++) {
    state->rfmData[ii] = data[ii];
  }
  
  // We have no way to know whether this succeeds...
  return 0;
}
