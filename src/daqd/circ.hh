/*
  $Id: circ.hh,v 1.5 2008/07/08 18:52:19 aivanov Exp $
  
  Circular buffer class
*/

#ifndef __CIRC_PLUS_PLUS_H__
#define __CIRC_PLUS_PLUS_H__

#include "circ.h"
#include "debug.h"

#define nvl(a,b) ((a)?(a):(b))

/// Do not mark immediate block on the producer path
/// for the transient consumer
/// Don't set this equal to zero
const int circ_buffer_transient_thresh = 1;

/// Defines parameter to `put_nowait_scattered'
struct put_vec {
  unsigned long vec_idx;
  unsigned long vec_len;
};

/// Defines parameter to `put_pscattered'
struct put_pvec {
  char *pvec_addr;
  unsigned long pvec_len;
};

/// Defines parameter to `put16th_dpscattered'
struct put_dpvec {
  unsigned char *src_pvec_addr;
  unsigned long dest_vec_idx;
  unsigned long vec_len;
  unsigned int *src_status_addr;
  unsigned long dest_status_idx;
  unsigned int  bsw; ///< byteswap or not; if not zero then the number of bytes in a group to byteswap
};

/// Circular buffer class
class circ_buffer {
 public:
  enum mem_choice {flag_malloc=0, ptr=1, shmem_ftok=2, shmem_shmid=3, shmem_shmkey=4}; ///< how to allocate circular buffer 
 private:
  /*
    Locking on the instance of the class can be done with the
    scoped locking.

    void
    circ_buffer::foo()
     {
       locker mon (this);   // lock is held as long as the mon exists
       ...
     }
  */
  void lock (void) {pthread_mutex_lock (&pbuffer -> lock);}
  void unlock (void) {pthread_mutex_unlock (&pbuffer -> lock);}
  class locker;
  friend class circ_buffer::locker;
  class locker {
    circ_buffer *dp;
  public:
    locker (circ_buffer *objp) {(dp = objp) -> lock ();}
    ~locker () {dp -> unlock ();}
  };
 private:

  mem_choice mem_flag;

  /// Variables are factored out to the `circ_buffer_t' struct 
  circ_buffer_t *pbuffer;

  int buffer_malloc (int consumers, int blocks, long block_size, time_t buffer_period); ///< This is a part of the constructor, really

public:
  circ_buffer (int consumers = 1, int blocks = 100, long block_size = 10240, time_t block_period = 1, mem_choice mem_flagp=flag_malloc, char *param1 = NULL);
  ~circ_buffer ();

  int put16th_dpscattered_lost (struct put_dpvec *, int, circ_buffer_block_prop_t *a = 0);
  int put16th_dpscattered (struct put_dpvec *, int, circ_buffer_block_prop_t *a = 0);
  int get16th (int);
  int put (char *, int, circ_buffer_block_prop_t *a = 0);
  int put_pscattered (struct put_pvec *, int, circ_buffer_block_prop_t *a = 0);
  int put_nowait (char *, int, circ_buffer_block_prop_t *a = 0);
  int put_nowait_scattered (char *, struct put_vec *, int, circ_buffer_block_prop_t *a = 0);
  int get (int);
  int get_nowait (int);
  void unlock (int);
  void unlock16th (int);
  int noop (int);

  int block_period () {
    assert (pbuffer);
    return pbuffer -> block_period;
  }

  long block_size () { 
    assert (pbuffer);
    return pbuffer -> block_size;
  }

  /// Get the number of free blocks in the buffer
  int bfree () {
    int free_blocks = blocks ();
    int fb;
    assert (pbuffer);
    locker mon (this);

    for (int i = 0; i < MAX_CONSUMERS; i++)
      if (pbuffer -> cmask & 1 << i)
	{
	  int b = pbuffer -> next_block_in - pbuffer -> next_block_out [i];
	  if (b < 0)
	    fb = -b;
	  else if (b == 0)
	    fb = pbuffer -> block [pbuffer -> next_block_out [i]].busy? 0: blocks ();
	  else
	    fb = blocks() - b;

	  if (fb < free_blocks)
	    free_blocks = fb;
	}

    return free_blocks;
  }

  int blocks () {
    assert (pbuffer);
    return pbuffer -> blocks;
  }

  int drops () {
    assert (pbuffer);
    return pbuffer -> drops;
  }

  int num_puts () {
    assert (pbuffer);
    return pbuffer -> puts;
  }

  char *block_ptr (int bnum) {
    assert (pbuffer && bnum < pbuffer -> blocks && bnum >= 0);
    return pbuffer -> data_space + bnum * pbuffer -> block_size ;
  };

  circ_buffer_block_t *block_prop (int bnum) {
    assert (pbuffer && bnum >= 0 && bnum < pbuffer -> blocks);
    return pbuffer -> block + bnum;
  };

  int get_cons_num () { 
    int consumers;
    pthread_mutex_lock (&pbuffer -> lock);
    consumers = pbuffer -> consumers;
    pthread_mutex_unlock (&pbuffer -> lock);
    return consumers;
  };

  /// Add new consumer to the circular buffer.  Returns -1 if no more
  /// consumers allowed.  Otherwise returns consumer number.
  /// Set `fast' true if new concumer will be using get16th() call 
  int add_consumer (int fast = 0) {
    int i;
    locker mon (this);

    if (pbuffer -> consumers >= MAX_CONSUMERS)
      return -1;

    // Find next available `0' in `pbuffer -> tcmask', starting from the low end
    for (i = 0; pbuffer -> tcmask & 1 << i; i++)
      ;
    pbuffer -> cmask |= 1 << i;
    pbuffer -> tcmask |= 1 << i;
    pbuffer -> consumers++;

    pbuffer -> next_block_out [i] = pbuffer -> next_block_in;

    if (fast) {
      pbuffer -> cmask16th |= 1 << i;
      pbuffer -> next_block_out_16th [i] = pbuffer -> next_block_in_16th;
      pbuffer -> fast_consumers++;
    }

    assert (invariant (1));

    return i;
  }

  /// Add new transient consumer to the circular buffer.  No bit in `cmask' is
  /// set for new consumer (producer will not be marking new blocks for this
  /// consumer).  Look through the timestamps on the blocks in the circular
  /// buffer and set `busy' bits for the block inside the time period.  Time
  /// period is passed as the GPS time and period length in seconds.
  ///
  /// Returns -1 if no more consumers allowed.  Returns -2 if no data
  /// found. Returns -3 if `gps' is in the future.
  /// 
  /// Otherwise returns consumer number.  
  int add_transient_consumer (time_t gps, time_t delta, time_t *bstart, time_t *blast) {
    int cons_num;
    locker mon (this);

    // too many consumers for one buffer
    if (pbuffer -> consumers >= MAX_CONSUMERS)
      return -1;

    // empty buffer
    if (! num_puts ())
      return -2;

    // Find next available `0' in `pbuffer -> tcmask', starting from low end
    for (cons_num = 0; pbuffer -> tcmask & 1 << cons_num; cons_num++)
      ;

    // If `delta' = 0, `gps' represents playback offset
    if (! delta)
      {
	delta = gps > blocks () * block_period () - 1? blocks () * block_period () - 1: gps;

	// `gps' is used here as the index into the `block[]' array
	gps = (pbuffer -> next_block_in + blocks ()  - delta / block_period ()) % blocks ();
	if (gps > num_puts ())
	  {
	    delta = num_puts () * block_period ();
	    gps = 0;
	  }
	gps = pbuffer -> block [gps].prop.gps;
      }

    DEBUG(2, std::cerr << "Marking circ buffer for: gps=" << gps << " delta=" << delta << std::endl);

    int blocks_marked = 0;
    int first_block = -1;

    // Search for the data and set bits for matching blocks
    for (int i = (pbuffer -> next_block_in + circ_buffer_transient_thresh) % pbuffer -> blocks;
	 i != pbuffer -> next_block_in;
	 ++i %= pbuffer -> blocks)
      {
	pthread_mutex_lock (&pbuffer -> block [i].lock);
	if (gps <= pbuffer -> block [i].prop.gps
	    && pbuffer -> block [i].prop.gps < gps + delta)
	  {
	    DEBUG(2, std::cerr << "block " << i << " marked; gps=" << pbuffer -> block [i].prop.gps << std::endl);
	    blocks_marked++;
	    pbuffer -> block [i].busy |= 1 << cons_num;
	    if (first_block < 0)
	      {
		first_block = i;
		*bstart = pbuffer -> block [i].prop.gps;
	      }
	    *blast = pbuffer -> block [i].prop.gps;
	  }
	pthread_mutex_unlock (&pbuffer -> block [i].lock);
      }

    DEBUG(2, std::cerr << blocks_marked << " marked" << std::endl);
    DEBUG(2, std::cerr << "bstart=" << *bstart << "; blast=" << *blast << std::endl);

    // I don't start new consumer if no blocks were marked
    if (first_block < 0)
      return -2;

    pbuffer -> tcmask |= 1 << cons_num;
    pbuffer -> consumers++;
    pbuffer -> transient_consumers++;

    pbuffer -> next_block_out [cons_num] = first_block;

    assert (invariant(1));

    return cons_num;
  }

  int delete_consumer (int cnumber) {
    locker mon (this);
    int fast_consumer = 0;

    assert (cnumber < MAX_CONSUMERS);
    
    // See if this is fast cincumer
    if (pbuffer -> cmask16th & 1 << cnumber) {
      fast_consumer = 1;
      pbuffer -> fast_consumers--;    
      pbuffer -> cmask16th &= ~(1 << cnumber);
    }

    // See if this is transient consumer
    if (! (pbuffer -> cmask & 1 << cnumber))
      pbuffer -> transient_consumers--;
    else
      pbuffer -> cmask &= ~(1 << cnumber);     // Clear signing off consumer off the consumer mask

    pbuffer -> tcmask &= ~(1 << cnumber);
    pbuffer -> consumers--;

    // Clean `busy' bits in all blocks for the consumer
    for (int i = 0; i < pbuffer -> blocks; i++)
      {
	pthread_mutex_lock (&pbuffer -> block [i].lock);
	{
	  for (int j = 0; j < 16; j++)
	    pbuffer -> block [i].busy16th [j] &= ~(1 << cnumber);	    

	  unsigned int sbusy = pbuffer -> block [i].busy;
	  pbuffer -> block [i].busy &= ~(1 << cnumber);
	  // Signal `notfull' only if we cleared the block
	  if (! pbuffer -> block [i].busy
	      && pbuffer -> block [i].busy != sbusy)
	    pthread_cond_broadcast (&pbuffer -> block [i].notfull);
	}
	pthread_mutex_unlock (&pbuffer -> block [i].lock);
      }
    assert (invariant(1));
    return 0;
  }

  int get_prod_num () { 
    int producers;
    pthread_mutex_lock (&pbuffer -> lock);
    producers = pbuffer -> producers;
    pthread_mutex_unlock (&pbuffer -> lock);
    return producers;
  };
  int set_prod_num (int a) { 
    assert (a >= 0 && a < MAX_PRODUCERS);
    pthread_mutex_lock (&pbuffer -> lock);  
    pbuffer -> producers = a;
    pthread_mutex_unlock (&pbuffer -> lock);
    return a;
  };
  circ_buffer_t *buffer_ptr () { return pbuffer; };
  unsigned long gps() {    
    int bnum = pbuffer -> next_block_in - 1;
    if (bnum < 0)
      bnum = blocks() - 1;
    return block_prop(bnum) -> prop.gps;
  }
#ifndef	NDEBUG
  /*
    This implements invariant for the circular buffer class. Invariant is,
    generally, a function, which returns 0 only if the object it is
    testing is not in the consistent state. Consistency of the object,
    consequentely, is determined by this invariant. This is only the
    syntactical, so to say, type of consistensy for the circular buffer,
    it does not care about the intricacies of the data stored in the
    buffer data blocks.

    `invariant()' is not compiled if NDEBUG is defined. This is for the
    consistensy with the assertions in <assert.h> header file. Assertion
    should be used to call invariant function.

    `level' parameter could be set to indicate check when `circ_buffer_t::lock'
    is locked during invariant call. We can do some more checks in this case.
  */
 
  int invariant (int level = 0) {
    int i;

    if (! pbuffer) {
      std::cerr << "invariant(): `pbuffer' is not set" << std::endl;
      return 0;
    }
    if (mem_flag < flag_malloc || mem_flag > shmem_shmkey) {
      std::cerr << "invariant(): `mem_flag' is invalid" << std::endl;
      return 0;
    }
    if ((pbuffer -> blocks | pbuffer -> block_size) <= 0) {
      std::cerr << "invariant(): invalid block number (" << pbuffer -> blocks 
	   << ") or block size(" << pbuffer -> block_size << ")" << std::endl;
      return 0;
    }
    if (pbuffer -> producers < 0
	|| pbuffer -> producers > MAX_PRODUCERS) {
      std::cerr << "invariant(): invalid number of producers -- " << pbuffer -> producers << std::endl;
      return 0;
    }
    if (pbuffer -> consumers < 0
	|| pbuffer -> consumers > MAX_CONSUMERS) {
      std::cerr << "invariant(): invalid number of consumers -- " << pbuffer -> consumers << std::endl;
      return 0;
    }

    if (level) {
      unsigned int consumers = pbuffer -> consumers;
      for (i = 0; i < MAX_CONSUMERS; i++)
	if (pbuffer -> cmask & 1 << i)
	  {
	    consumers--;
	    if (pbuffer -> next_block_out [i] < 0
		|| pbuffer -> next_block_out [i] > pbuffer -> blocks) {
	      std::cerr << "invariant(): invalid `next_block_out[" << i << "]' == " << pbuffer -> next_block_out [i] << std::endl;
	      return 0;
	    }
	  }

      // Checking if the number of bits set in `pbuffer -> cmask' is equal to
      // the number of consumers, less the number of transient consumers
      if (consumers - pbuffer -> transient_consumers) {
	std::cerr << "invariant(): number of bits set in `pbuffer -> cmask' is not equal to the number of consumers" << std::endl;
	return 0;
      }
      if (pbuffer -> next_block_in < 0
	  || pbuffer -> next_block_in > pbuffer -> blocks) {
	std::cerr << "invariant(): invalid `next_block_in' == " << pbuffer -> next_block_in << std::endl;
	return 0;
      }
    }

    if (pbuffer -> blocks > MAX_BLOCKS) {
      std::cerr << "invariant(): invalid number of `blocks' == " << pbuffer -> blocks << std::endl;
      return 0;
    }

    if (level) {
      for (i = 0; i < pbuffer -> blocks; i++)
	if (pbuffer -> block [i].bytes < 0
	    || pbuffer -> block [i].bytes > pbuffer -> block_size
	    || pbuffer -> block [i].busy & ~ pbuffer -> tcmask) // checking if `busy' has any bits set outside the `tcmask'
	  {
	    std::cerr << "invariant(): invalid `block[" << i << "]' properties; bytes=" << pbuffer -> block [i].bytes
		 << "; busy=" << pbuffer -> block [i].busy << "; transient consumers mask (tcmask) = " << pbuffer -> tcmask
		 << "; block_size=" << pbuffer -> block_size << std::endl;
	    return 0;
	  }
    }
    return 1;
  };
#endif

};

#endif
