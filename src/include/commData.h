/*----------------------------------------------------------------------
   File: commData.h
   Date: October 2009
   Author: Matthew Evans & Tobin Fricke
 
   These functions are used to communicated between real-time front-end
   machines.  A cycle counter, maintained externally, is used to move
   between 2 entries in a double buffer.  A checksum based on the 
   cycle counter and on the data is included to verify that the data
   block is intact and is from the correct cycle.

   No attempt is made to deal with endianness issues, nor to correct
   any errors which are detected.
----------------------------------------------------------------------*/

#ifndef __COMMDATA_H__
#define __COMMDATA_H__

// type used for cycle counter
//   all FEs must agree on its size
typedef unsigned int cycle_cd;

// type used for CRC checksum
//   this type must be at least 4 bytes,
//   and all FEs must agree on its size
typedef unsigned int crc_cd;

// type used for copying data one word at a time
//   this may be different from one FE to the next
//   but must divide 8 evenly (e.g., 8, 4, 2 or 1)
typedef unsigned long word_cd;

// cycle counter rate
//   this is the maximum rate for all FEs
//   and the maximum rate of all communication links
//   this must be a power of 2
#ifdef FE_MAX_RATE
#  define COMMDATA_MAX_RATE FE_MAX_RATE
#else
#  define COMMDATA_MAX_RATE 65536
#endif

// number of bytes in the commData header
//   this is used for the CRC
//   it must be >= 2 * sizeof(crc_cd)
//   and it must an integer number of words
#define COMMDATA_HEADER_BYTES 8

// structure that maintains internal state of communication data
struct CommDataState
{
  // sample rate of this communication link
  //   data is updated at this data rate
  //   the data rate must be a power of 2
  //   and it must be less than COMMDATA_MAX_RATE
  int dataRate;

  // data rate as a shift relative to the clock rate
  //   rateShift = log2(COMMDATA_MAX_RATE) - log2(dataRate)
  int rateShift;

  // number of bytes to send or receive
  //   this is the length of the localData buffer
  //   it excludes the 8 bytes reserved for the checksum in shared memory
  //   and must be an even multiple of 8 bytes
  int dataBytes;

  // data length in words
  //   dataWords = dataLength / sizeof(word_cd)
  int dataWords;

  // pointer to first checksum in the header
  //   the second checksum follows
  crc_cd* sharedCRC;

  // pointer to first data block in the shared double buffer
  //   the second data block follows
  word_cd* sharedData;

  // pointer to the local copy of the data
  //   this data is copied to/from shared memory on send/recv
  word_cd* localData;

  // error counters, running and last second
  int errorCountRunning;
  int errorCountOneSec;
};
typedef struct CommDataState CommDataState;

// decide between inline or not for commData functions
#ifdef COMMDATA_INLINE
#  include "../fe/commData.c"
#else
  // initialize the CommDataState struct
  void commDataInit(CommDataState* state, int dataRate, int dataBytes,
                         void* localData, void* sharedBlock);
  // send data
  //   the cycle counter is included in the checksum,
  //   and is used to index the ring buffer
  void commDataSend(CommDataState* state, cycle_cd cycleCounter);

  // receive data
  int commDataRecv(CommDataState* state, cycle_cd cycleCounter);
#endif

#endif // __COMMDATA_H__
