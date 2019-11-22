#ifndef ARCHIVE_HH
#define ARCHIVE_HH

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "config.h"
#include "sing_list.hh"
#include "channel.h"
#include "filesys.hh"

class archive_c : public s_link
{
public:
    archive_c( ) : fsd( 1 ), data_type( unknown ), channels( 0 ), nchannels( 0 )
    {
        pthread_mutex_init( &bm, NULL );
    }

    ~archive_c( )
    {
        free_channel_config( );
        pthread_mutex_destroy( &bm );
    }

    /* Definition of abstract function for the superclass */
    void
    destroy( )
    {
        delete this;
    };

    int scan( char* name, char* prefix, char* suffix, int ndirs );
    int load_config( char* fname );
    int load_old_config( char* fname );

    void
    lock( void )
    {
        pthread_mutex_lock( &bm );
    }
    void
    unlock( void )
    {
        pthread_mutex_unlock( &bm );
    }

    filesys_c fsd;

    /* Frame file data types.
       Single archive can hold data of one type only */
    enum
    {
        unknown,
        full,
        secondtrend,
        minutetrend
    } data_type;

    struct channel
    {
        char*        name;
        daq_data_t   type;
        bool         old; // Is this old channel name?
        unsigned int rate;
    } * channels;

    unsigned int nchannels;

private:
    /* Read configuration file and load config data */
    int scan( FILE*, bool old = 0 );

    /* Free allocated channel config data if any */
    void
    free_channel_config( )
    {
        for ( int i = 0; i < nchannels; i++ )
            free( channels[ i ].name );
        free( (void*)channels );
        nchannels = 0;
        channels = 0;
        data_type = unknown;
    }

    /* New channels are allocated in chunks */
    const static unsigned channel_alloc_chunk = 100;

    /* Allocate new channel */
    struct channel*
    add_channel( )
    {
        if ( nchannels % channel_alloc_chunk == 0 )
        {
            channels =
                (struct channel*)realloc( channels,
                                          ( nchannels + channel_alloc_chunk ) *
                                              sizeof( struct channel ) );
        }
        return &channels[ nchannels++ ];
    }

    /* Locking for shared access */
    pthread_mutex_t bm;
    class locker;
    friend class archive_c::locker;
    class locker
    {
        archive_c* dp;

    public:
        locker( archive_c* objp )
        {
            ( dp = objp )->lock( );
        }
        ~locker( )
        {
            dp->unlock( );
        }
    };
};

#endif
