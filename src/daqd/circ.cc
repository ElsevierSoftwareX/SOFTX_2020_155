#include <config.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <iostream>
using namespace std;
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include "circ.hh"

#ifndef SCOPE_PROGRAM
#include "daqd.hh"

extern daqd_c daqd;
#endif

int
circ_buffer::buffer_malloc( int    consumers,
                            int    blocks,
                            long   block_size,
                            time_t block_period )
{
    long bufs;

    // adjust the block size to be the multiple of 8
    // this must be done to avoid sigbus errors when accessing double data

    if ( block_size % 8 )
        block_size += 8 - block_size % 8;

    pbuffer = (circ_buffer_t*)malloc( bufs = blocks * block_size +
                                          sizeof( circ_buffer_t ) - 1 );
    if ( !pbuffer )
        return -1;

    memset( pbuffer, 0, bufs );

    pthread_mutex_init( &pbuffer->lock, NULL );
    pbuffer->blocks = blocks;
    pbuffer->block_size = block_size;
    pbuffer->block_period = block_period;

    for ( int i = 0; i < blocks; i++ )
    {
        //      pbuffer -> block [i].datai = i * block_size;
        pthread_mutex_init( &pbuffer->block[ i ].lock, NULL );
        pbuffer->block[ i ].busy.clear_all( );
        for ( int j = 0; j < 16; j++ )
            pbuffer->block[ i ].busy16th[ j ].clear_all( );
        pthread_cond_init( &pbuffer->block[ i ].notfull, NULL );
        pthread_cond_init( &pbuffer->block[ i ].notempty, NULL );
        pthread_cond_init( &pbuffer->block[ i ].notempty16th, NULL );
    }
    pbuffer->producers = 1;
    pbuffer->consumers = consumers;
    pbuffer->tcmask.clear_all( );
    pbuffer->cmask.clear_all( );
    if ( consumers )
    {
        for ( int j = 0; j < consumers; ++j )
        {
            pbuffer->tcmask.set( j );
        }
        pbuffer->cmask = pbuffer->tcmask;
    }

    pbuffer->cmask16th.clear_all( );
    DEBUG1( cerr << "circ buffer constructed; blocks=" << blocks
                 << " block_size=" << block_size << "\n" );
    return 0;
}

// circ_buffer::circ_buffer (int consumers = 1, int blocks = 100, long
// block_size = 10240, time_t block_period = 1, mem_choice mem_flagp=flag_malloc,
// char *param1 = NULL)
circ_buffer::circ_buffer( int        consumers,
                          int        blocks,
                          long       block_size,
                          time_t     block_period,
                          mem_choice mem_flagp,
                          char*      param1 )
{
    if ( blocks > MAX_BLOCKS )
    {
        // too many blocks
        assert( 0 );
    }

    this->mem_flag = mem_flagp;

    switch ( mem_flagp )
    {
    case flag_malloc:
        buffer_malloc( consumers, blocks, block_size, block_period );
        break;
    case ptr:
        assert( param1 );
        pbuffer = (circ_buffer_t*)param1;
        break;
    default:
        // not implemented
        assert( 0 );
    }
}

circ_buffer::~circ_buffer( )
{
    if ( pbuffer )
    {
        assert( invariant( 1 ) );

        pthread_mutex_destroy( &pbuffer->lock );

        for ( int i = 0; i < pbuffer->blocks; i++ )
        {
            pthread_mutex_destroy( &pbuffer->block[ i ].lock );
            pthread_cond_destroy( &pbuffer->block[ i ].notfull );
            pthread_cond_destroy( &pbuffer->block[ i ].notempty );
            pthread_cond_destroy( &pbuffer->block[ i ].notempty16th );
        }

        switch ( mem_flag )
        {
        case flag_malloc:
            assert( pbuffer );
            free( (void*)pbuffer );
            DEBUG1( cerr << "circ buffer deleted\n" );
            break;
        default:
            break;
        }
    }
}

// FIXME: do not broadcast on `notempty16th' if there are no fast consumers

/*
  Zero out all the data in the current 16th part of a current block, indicating
  lost data block.
  FIXME: has to set `lost data' status on a block. May also indicate with
  bit-mask which parts of a block are lost (16 bits for 16 parts).
*/
int
circ_buffer::put16th_dpscattered_lost( struct put_dpvec*         pv,
                                       int                       pvlen,
                                       circ_buffer_block_prop_t* prop )
{
    int nbi, nbi16th;

    pthread_mutex_lock( &pbuffer->block[ pbuffer->next_block_in ].lock );
    {
        nbi = pbuffer->next_block_in;
        nbi16th = pbuffer->next_block_in_16th;

#ifndef NDEBUG
        if ( !nbi16th )
            assert( invariant( ) );
#endif

        while ( !pbuffer->block[ nbi ].busy.empty( ) )
            pthread_cond_wait( &pbuffer->block[ nbi ].notfull,
                               &pbuffer->block[ nbi ].lock );

        char*         dst = pbuffer->data_space + nbi * pbuffer->block_size;
        unsigned long dlen = 0;

        for ( int i = 0; i < pvlen; i++ )
        {
            memset( dst + pv[ i ].dest_vec_idx + pv[ i ].vec_len * nbi16th,
                    0,
                    pv[ i ].vec_len );

            dlen += pv[ i ].vec_len;
        }

        if ( prop )
            pbuffer->block[ nbi ].prop16th[ nbi16th ] = *prop;

        pbuffer->block[ nbi ].prop16th[ nbi16th ].seq_num = pbuffer->puts;
        if ( !nbi16th )
        {
            pbuffer->block[ nbi ].bytes = dlen;
            if ( prop )
            {
                pbuffer->block[ nbi ].prop = *prop;
            }
            pbuffer->block[ nbi ].prop.seq_num = pbuffer->puts;
        }
        else
            pbuffer->block[ nbi ].bytes += dlen;
    }
    pthread_mutex_unlock( &pbuffer->block[ nbi ].lock );
    pthread_mutex_lock( &pbuffer->lock );
    {

        if ( nbi16th == 15 )
        {
            pbuffer->block[ nbi ].busy = pbuffer->cmask;
            ++pbuffer->next_block_in %= pbuffer->blocks;
            pbuffer->puts++;
        }

        ++pbuffer->next_block_in_16th %= 16;
        pbuffer->block[ nbi ].busy16th[ nbi16th ] = pbuffer->cmask16th;
    }
    pthread_mutex_unlock( &pbuffer->lock );

    if ( nbi16th == 15 )
        pthread_cond_broadcast( &pbuffer->block[ nbi ].notempty );

    pthread_cond_broadcast( &pbuffer->block[ nbi ].notempty16th );
    return nbi;
}

/*
  Place new 16th part of a block in the queue, scattered data interface on the
  source and on the destination
*/
int
circ_buffer::put16th_dpscattered( struct put_dpvec*         pv,
                                  int                       pvlen,
                                  circ_buffer_block_prop_t* prop )
{
    int nbi, nbi16th;

    pthread_mutex_lock( &pbuffer->block[ pbuffer->next_block_in ].lock );
    {
        nbi = pbuffer->next_block_in;
        nbi16th = pbuffer->next_block_in_16th;

#ifndef NDEBUG
        if ( !nbi16th )
            assert( invariant( ) );
#endif

        while ( !pbuffer->block[ nbi ].busy.empty( ) )
            pthread_cond_wait( &pbuffer->block[ nbi ].notfull,
                               &pbuffer->block[ nbi ].lock );

        char*         dst = pbuffer->data_space + nbi * pbuffer->block_size;
        unsigned long dlen = 0;

        for ( int i = 0; i < pvlen; i++ )
        {
            assert( nbi16th < 16 && nbi16th >= 0 );
            assert( pv[ i ].dest_vec_idx + pv[ i ].vec_len * nbi16th +
                        pv[ i ].vec_len <=
                    pbuffer->block_size );

            int status = *( pv[ i ].src_status_addr );

#ifndef SCOPE_PROGRAM
            if ( status && daqd.zero_bad_data )
            {
#else
            if ( status )
            {
#endif
                memset( dst + pv[ i ].dest_vec_idx + pv[ i ].vec_len * nbi16th,
                        0,
                        pv[ i ].vec_len );
            }
            else
            {
                void* dest =
                    dst + pv[ i ].dest_vec_idx + pv[ i ].vec_len * nbi16th;
                void* src = (void*)pv[ i ].src_pvec_addr;
#ifdef DATA_CONCENTRATOR
                int bsw = pv[ i ].bsw;
                switch ( bsw )
                {
                case 2:
                {
                    int n = pv[ i ].vec_len / bsw;
                    for ( int j = 0; j < n; j++ )
                        ( (short*)dest )[ j ] = ntohs( ( (short*)src )[ j ] );
                }
                break;
                case 4:
                {
                    int n = pv[ i ].vec_len / bsw;
                    for ( int j = 0; j < n; j++ )
                        ( (int*)dest )[ j ] = ntohl( ( (int*)src )[ j ] );
                }
                break;
                default:
                    memcpy( dest, src, pv[ i ].vec_len );
                    break;
                }
#else
                memcpy( dest, src, pv[ i ].vec_len );
#endif
            }

#define memor4( dest, tgt )                                                    \
    *( (unsigned char*)dest ) |= *( (unsigned char*)tgt );                     \
    *( ( (unsigned char*)dest ) + 1 ) |= *( ( (unsigned char*)tgt ) + 1 );     \
    *( ( (unsigned char*)dest ) + 2 ) |= *( ( (unsigned char*)tgt ) + 2 );     \
    *( ( (unsigned char*)dest ) + 3 ) |= *( ( (unsigned char*)tgt ) + 3 );

            if ( pv[ i ].dest_status_idx != 0xffffffff )
            {
                if ( !nbi16th )
                {
                    // assign status word for the first 16th of a second
                    //*((int *)(dst + pv [i].dest_status_idx)) = status;
                    memcpy( (char*)( dst + pv[ i ].dest_status_idx ),
                            &status,
                            sizeof( int ) );
                }
                else
                {
                    // OR status word
                    //	  *((int *)(dst + pv [i].dest_status_idx)) |= status;
                    memor4( dst + pv[ i ].dest_status_idx, &status );
                }
                //	*((int *)(dst + pv [i].dest_status_idx) + 1 + nbi16th) =
                //status;
                memcpy( (char*)( dst + pv[ i ].dest_status_idx +
                                 sizeof( int ) * ( 1 + nbi16th ) ),
                        &status,
                        sizeof( int ) );
            }

            dlen += pv[ i ].vec_len;
        }

        if ( prop )
        {
            pbuffer->block[ nbi ].prop16th[ nbi16th ] = *prop;
        }
        pbuffer->block[ nbi ].prop16th[ nbi16th ].seq_num = pbuffer->puts;
        if ( !nbi16th )
        {
            pbuffer->block[ nbi ].bytes = dlen;
            if ( prop )
            {
                pbuffer->block[ nbi ].prop = *prop;
            }
            pbuffer->block[ nbi ].prop.seq_num = pbuffer->puts;
        }
        else
            pbuffer->block[ nbi ].bytes += dlen;
    }
    pthread_mutex_unlock( &pbuffer->block[ nbi ].lock );
    pthread_mutex_lock( &pbuffer->lock );
    {

        if ( nbi16th == 15 )
        {
            pbuffer->block[ nbi ].busy = pbuffer->cmask;
            ++pbuffer->next_block_in %= pbuffer->blocks;
            pbuffer->puts++;
        }

        ++pbuffer->next_block_in_16th %= 16;
        pbuffer->block[ nbi ].busy16th[ nbi16th ] = pbuffer->cmask16th;
    }
    pthread_mutex_unlock( &pbuffer->lock );

    if ( nbi16th == 15 )
        pthread_cond_broadcast( &pbuffer->block[ nbi ].notempty );

    pthread_cond_broadcast( &pbuffer->block[ nbi ].notempty16th );
    return nbi;
}

/*
  Place new block in the queue
*/
int
circ_buffer::put( char* data, int dlen, circ_buffer_block_prop_t* prop )
{
    int nbi;

    /* FIXME: For multiple producers `next_block_in' access should be
     * synchronized */

    pthread_mutex_lock( &pbuffer->block[ pbuffer->next_block_in ].lock );
    {
        assert( dlen <= pbuffer->block_size && dlen >= 0 );
        assert( invariant( ) );

        nbi = pbuffer->next_block_in;
        while ( !pbuffer->block[ nbi ].busy.empty( ) )
            pthread_cond_wait( &pbuffer->block[ nbi ].notfull,
                               &pbuffer->block[ nbi ].lock );

        memcpy( pbuffer->data_space + nbi * pbuffer->block_size, data, dlen );
        pbuffer->block[ nbi ].bytes = dlen;

        if ( prop )
        {
            pbuffer->block[ nbi ].prop = *prop;
        }
        pbuffer->block[ nbi ].prop.seq_num = pbuffer->puts;
    }
    pthread_mutex_unlock( &pbuffer->block[ nbi ].lock );

    pthread_mutex_lock( &pbuffer->lock );
    {
        pbuffer->block[ nbi ].busy = pbuffer->cmask;
        ++pbuffer->next_block_in %= pbuffer->blocks;
        pbuffer->puts++;
    }
    pthread_mutex_unlock( &pbuffer->lock );

    pthread_cond_broadcast( &pbuffer->block[ nbi ].notempty );

    return nbi;
}

/*
  Place new block in the queue, scattered data interface on the source
*/
int
circ_buffer::put_pscattered( struct put_pvec*          pv,
                             int                       pvlen,
                             circ_buffer_block_prop_t* prop )
{
    int nbi;

    /* For multiple producers `next_block_in' access should be synchronized */

    pthread_mutex_lock( &pbuffer->block[ pbuffer->next_block_in ].lock );
    {
        assert( invariant( ) );

        nbi = pbuffer->next_block_in;
        while ( !pbuffer->block[ nbi ].busy.empty( ) )
            pthread_cond_wait( &pbuffer->block[ nbi ].notfull,
                               &pbuffer->block[ nbi ].lock );

        char*         dst = pbuffer->data_space + nbi * pbuffer->block_size;
        unsigned long dlen = 0;

        for ( int i = 0; i < pvlen; i++ )
        {
            memcpy( dst + dlen, pv[ i ].pvec_addr, pv[ i ].pvec_len );
            dlen += pv[ i ].pvec_len;
        }
        pbuffer->block[ nbi ].bytes = dlen;

        if ( prop )
        {
            pbuffer->block[ nbi ].prop = *prop;
        }
        pbuffer->block[ nbi ].prop.seq_num = pbuffer->puts;
    }
    pthread_mutex_unlock( &pbuffer->block[ nbi ].lock );
    pthread_mutex_lock( &pbuffer->lock );
    {
        pbuffer->block[ nbi ].busy = pbuffer->cmask;
        ++pbuffer->next_block_in %= pbuffer->blocks;
        pbuffer->puts++;
    }
    pthread_mutex_unlock( &pbuffer->lock );

    pthread_cond_broadcast( &pbuffer->block[ nbi ].notempty );

    return nbi;
}

/*
  Place new block in the queue
  Drop the block if queue is busy
  Returns -1 if the block was dropped.
*/
int
circ_buffer::put_nowait( char* data, int dlen, circ_buffer_block_prop_t* prop )
{
    int ret;
    int nbi;

    /* For multiple producers `next_block_in' access should be synchronized */

    pthread_mutex_lock( &pbuffer->block[ pbuffer->next_block_in ].lock );

    assert( dlen <= pbuffer->block_size && dlen >= 0 );
    assert( invariant( ) );

    if ( pbuffer->block[ ret = nbi = pbuffer->next_block_in ].busy.empty( ) )
    {
        memcpy( pbuffer->data_space + nbi * pbuffer->block_size, data, dlen );
        pbuffer->block[ nbi ].bytes = dlen;
        if ( prop )
        {
            pbuffer->block[ nbi ].prop = *prop;
        }
        pbuffer->block[ nbi ].prop.seq_num = pbuffer->puts;

        pthread_mutex_unlock( &pbuffer->block[ nbi ].lock );
        pthread_mutex_lock( &pbuffer->lock );
        pbuffer->block[ nbi ].busy = pbuffer->cmask;
        ++pbuffer->next_block_in %= pbuffer->blocks;
    }
    else
    {
        pthread_mutex_unlock( &pbuffer->block[ nbi ].lock );
        pthread_mutex_lock( &pbuffer->lock );

        pbuffer->drops++;
        ret = -1;
    }

    pbuffer->puts++;
    pthread_mutex_unlock( &pbuffer->lock );

    pthread_cond_broadcast( &pbuffer->block[ nbi ].notempty );

    return ret;
}

/*
  Identical to `put_nowait', but takes scattered data.
  It will read

  `data + pv [0].vec_idx' for `pv [0].vec_len' bytes
  .
  .
  .
  `data + pv [pvlen - 1].vec_idx' for `pv [pvlen - 1].vec_len' bytes
*/
int
circ_buffer::put_nowait_scattered( char*                     data,
                                   struct put_vec*           pv,
                                   int                       pvlen,
                                   circ_buffer_block_prop_t* prop )
{
    int ret;
    int nbi;

    /* For multiple producers `next_block_in' access should be synchronized */
    pthread_mutex_lock( &pbuffer->block[ pbuffer->next_block_in ].lock );

    assert( invariant( ) );

    if ( pbuffer->block[ ret = nbi = pbuffer->next_block_in ].busy.empty( ) )
    {
        char*         dst = pbuffer->data_space + nbi * pbuffer->block_size;
        unsigned long dlen = 0;

        for ( int i = 0; i < pvlen; i++ )
        {
            memcpy( dst + dlen, data + pv[ i ].vec_idx, pv[ i ].vec_len );
            dlen += pv[ i ].vec_len;
        }
        pbuffer->block[ nbi ].bytes = dlen;
        if ( prop )
        {
            pbuffer->block[ nbi ].prop = *prop;
        }
        pbuffer->block[ nbi ].prop.seq_num = pbuffer->puts;

        pthread_mutex_unlock( &pbuffer->block[ nbi ].lock );
        pthread_mutex_lock( &pbuffer->lock );

        pbuffer->block[ nbi ].busy = pbuffer->cmask;
        ++pbuffer->next_block_in %= pbuffer->blocks;
    }
    else
    {
        pthread_mutex_unlock( &pbuffer->block[ nbi ].lock );
        pthread_mutex_lock( &pbuffer->lock );

        pbuffer->drops++;
        ret = -1;
    }

    pbuffer->puts++;
    pthread_mutex_unlock( &pbuffer->lock );

    pthread_cond_broadcast( &pbuffer->block[ nbi ].notempty );

    return ret;
}

int
circ_buffer::get16th( int cnum )
{
    int nbo, nbo16th;

    pthread_mutex_lock(
        &pbuffer->block[ pbuffer->next_block_out[ cnum ] ].lock );

    assert( invariant( ) );

    nbo = pbuffer->next_block_out[ cnum ];
    nbo16th = pbuffer->next_block_out_16th[ cnum ];
    while ( !( pbuffer->block[ nbo ].busy16th[ nbo16th ].get( cnum ) ) )
        pthread_cond_wait( &pbuffer->block[ nbo ].notempty16th,
                           &pbuffer->block[ nbo ].lock );
    pbuffer->next_block_out_16th[ cnum ]++;
    pthread_mutex_unlock( &pbuffer->block[ nbo ].lock );

    return nbo << 4 | nbo16th;
}

/*
   Get next data block index for consumer number `cnum'.
   This will hang on while the block is not filled.
   Must be followed as soon as possible by the call to circ_buffer::unlock(),
   or producer will block.

   avi Tue Jan  6 11:05:45 PST 1998
   It seems that the race condition possible in the situation with one slow
   consumer one fast consumer and fast producer -- fast producer will make full
   circle on the buffer and catches up with the slow consumer; it will decrease
   `busy' flag between slow consumer `get' and `unlock' calls. Slow consumer
   then either sets `busy' negative or reads data from the buffer while producer
   puts new block in. This could be managed by having `busy' flags for every
   consumer.
   **** fixed by using bit flags in `busy', one for each consumer ***
*/
int
circ_buffer::get( int cnum )
{
    int nbo;

    pthread_mutex_lock(
        &pbuffer->block[ pbuffer->next_block_out[ cnum ] ].lock );

    assert( invariant( ) );

    nbo = pbuffer->next_block_out[ cnum ];
    while ( !( pbuffer->block[ nbo ].busy.get( cnum ) ) )
        pthread_cond_wait( &pbuffer->block[ nbo ].notempty,
                           &pbuffer->block[ nbo ].lock );
    pthread_mutex_unlock( &pbuffer->block[ nbo ].lock );

    return nbo;
}

/*
  Nonblocking get
*/
int
circ_buffer::get_nowait( int cnum )
{
    int ret;
    int nbo;

    pthread_mutex_lock(
        &pbuffer->block[ pbuffer->next_block_out[ cnum ] ].lock );

    assert( invariant( ) );

    nbo = pbuffer->next_block_out[ cnum ];
    if ( pbuffer->block[ nbo ].busy.get( cnum ) )
        ret = nbo;
    else
        ret = -1;

    pthread_mutex_unlock( &pbuffer->block[ nbo ].lock );

    return ret;
}

/*
   Free block for the consumer. Clean all 1/16 synchronization
*/
void
circ_buffer::unlock16th( int cnum )
{
    pthread_mutex_lock(
        &pbuffer->block[ pbuffer->next_block_out[ cnum ] ].lock );

    assert( invariant( ) );

    int nbo = pbuffer->next_block_out[ cnum ];
    pbuffer->block[ nbo ].busy.clear( cnum );
    bool busy = !pbuffer->block[ nbo ].busy.empty( );

    for ( int i = 0; i < 16; i++ )
        pbuffer->block[ nbo ].busy16th[ i ].clear( cnum );

    pthread_mutex_unlock( &pbuffer->block[ nbo ].lock );

    if ( busy )
        pthread_cond_broadcast( &pbuffer->block[ nbo ].notempty );
    else
        pthread_cond_broadcast( &pbuffer->block[ nbo ].notfull );

    ++pbuffer->next_block_out[ cnum ] %= pbuffer->blocks;
    pbuffer->next_block_out_16th[ cnum ] = 0;
}

/*
   Free block for the consumer
*/
void
circ_buffer::unlock( int cnum )
{
    pthread_mutex_lock(
        &pbuffer->block[ pbuffer->next_block_out[ cnum ] ].lock );

    assert( invariant( ) );

    int nbo = pbuffer->next_block_out[ cnum ];
    pbuffer->block[ nbo ].busy.clear( cnum );
    bool busy = !pbuffer->block[ nbo ].busy.empty( );

    pthread_mutex_unlock( &pbuffer->block[ nbo ].lock );

    if ( busy )
        pthread_cond_broadcast( &pbuffer->block[ nbo ].notempty );
    else
        pthread_cond_broadcast( &pbuffer->block[ nbo ].notfull );

    ++pbuffer->next_block_out[ cnum ] %= pbuffer->blocks;
}

/*
** This is executed by an inactive consumer thread
** to support synchronization
*/
int
circ_buffer::noop( int cnum )
{
    pthread_mutex_lock(
        &pbuffer->block[ pbuffer->next_block_out[ cnum ] ].lock );

    assert( invariant( ) );

    int nbo = pbuffer->next_block_out[ cnum ];
    while ( !( pbuffer->block[ nbo ].busy.get( cnum ) ) )
        pthread_cond_wait( &pbuffer->block[ nbo ].notempty,
                           &pbuffer->block[ nbo ].lock );

    pbuffer->block[ nbo ].busy.clear( cnum );
    bool busy = !pbuffer->block[ nbo ].busy.empty( );

    pthread_mutex_unlock( &pbuffer->block[ nbo ].lock );

    if ( busy )
        pthread_cond_broadcast( &pbuffer->block[ nbo ].notempty );
    else
        pthread_cond_broadcast( &pbuffer->block[ nbo ].notfull );

    ++pbuffer->next_block_out[ cnum ] %= pbuffer->blocks;

    return nbo;
}
