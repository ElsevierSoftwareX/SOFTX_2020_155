//  $Id: circ.h,v 1.6 2009/02/06 17:49:40 aivanov Exp $

#ifndef __CIRC_H__
#define __CIRC_H__

#include <limits.h>
#include <assert.h>
#include "config.h"

#define MAX_BLOCKS 200

/*
  There is one bit allocated for each consumer
*/
#define MAX_CONSUMERS (sizeof (unsigned int) * CHAR_BIT - 1)
#define MAX_PRODUCERS 2

/// Circ buffer properties
typedef struct {
  unsigned int run;      ///< Run number
  time_t gps, gps_n;     ///< GPS system timestamp is seconds and nanosec residual
  unsigned long seq_num; ///< Set to `puts' value when created
  int leap_seconds;      ///< TAI-UTC for this data block
  int altzone;           ///< Local seasonal time minus UTC (negative for USA)
#ifndef NO_BROADCAST
  int gds_signal_refresh;
#endif
  unsigned int cycle;    ///< current cycle counter (data block number within this run)
#ifdef DATA_CONCENTRATOR
  /// Concentrator needs to broadcast the following information for each block
  /// We need to keep it in the main circular buffer for each 16Hz data block
  /// for each DCU
  struct dcu_data {
    unsigned int crc;	///< Data block CRC sum
    unsigned int status;	///< DCU status 
    unsigned int cycle;
  } dcu_data[/*DCU_COUNT*/ 128 * 2]; ///< To support the H2
#endif
} circ_buffer_block_prop_t;

///  Circ buffer block
typedef struct {

  circ_buffer_block_prop_t prop16th [16];

  /// This structure is filled by the producer along with the data block,
  ///  and the contents are synchronized in the same way the data blocks in
  /// the buffer are 
  circ_buffer_block_prop_t prop;

  int bytes;                            ///< Number of bytes in this data block 
  //  int over_range;     /* This field is set if data is suspicious or not valid */
  //  int status;
  
  //  long datai;                    /* Index into the circ_buffer_t.data_space[] */

  pthread_mutex_t lock;
  unsigned int busy;                        ///< Bitmask of comsumer busy flags 
  unsigned int busy16th [16];
  pthread_cond_t notfull;                   ///< consumer will signal notfull after it gets the block 
  pthread_cond_t notempty;                ///< producer broadcasts this after it has filled in new block 
  pthread_cond_t notempty16th;           ///< producer broadcasts this after it has filled in new 1/16 of a block 
} circ_buffer_block_t;

///  Circular buffer
///
///  Using C structure instead of C++ class here to be sure of the memory layout
///  and to be compatible with C programs, which could access the buffer  if it
///  is in the shared memory.
///
///  There is a number of locks in the buffer.
///  These locks ought to be locked in order :-
///
///    circ_buffer_t::lock
///    circ_buffer_t::blocks [0].lock
///    .
///    .
///    .
///    circ_buffer_t::blocks [<last>].lock
///
///  These locks should be unlocked in the opposite order.
///
///  All said above shouldn't prevent you from just locking one type of the lock --
///  locking just `circ_buffer_t::block [i].lock' without locking `circ_buffer_t::lock'
///  is perfectly fine.
///
typedef struct {
  unsigned long puts;                   /* Number of put operations performed */
  long drops;                 /* Number of times `put_nowait()' dropped block */
  int blocks;          /* Number of data blocks in this buffer's data_space[] */

  /*
    Blocks are of equal size -- `block_size' bytes big. Each block can be
    addressed by either this -> data_space[block_num * blocks_size] or by
    this -> data_space[this -> block [block_num].datai].
  */
  long block_size;

  /*
    Time period, which data block spans, in seconds.
   */
  time_t block_period;

  int next_block_out [MAX_CONSUMERS];
  int next_block_out_16th [MAX_CONSUMERS];

  pthread_mutex_t lock;            /* Locked to protect this buffer integrity */
  int next_block_in;  /* index of the next blocks to be filled in by producer */ 
  int next_block_in_16th;  /* index of the next 16th part of a block to be
			      filled in by producer                           */ 

  int producers;
  int consumers; // Number of consumers (ordinary and transient)
  int transient_consumers;
  int fast_consumers; // Number of concumers using get16th() call

  /*
    Bitmask, has bit set foreach consumer
  */
  unsigned int cmask;
  unsigned int cmask16th;

  /*
    This bitmask is `cmask' | <transient consumers>
  */
  unsigned int tcmask;

  circ_buffer_block_t block [MAX_BLOCKS];

  /*
    These actual space for the blocks is here - the array extends pass 1.
    Each block can be addressed by this -> data_space[block_num * blocks_size]
    or this -> data_space[this -> block [block_num].datai].
  */
  char data_space [1];
} circ_buffer_t;

#endif
