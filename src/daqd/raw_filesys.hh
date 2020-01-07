#ifndef RAW_FILESYS_H
#define RAW_FILESYS_H

#include "config.h"
#include "debug.h"

/*
  Raw filesystem map class
  Rudimentary class now, just stores the location of the files (directory)
*/

class raw_filesys_c
{
public:
    enum
    {
        filename_max = FILENAME_MAX
    };

private:
    pthread_mutex_t bm;
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
    class locker;
    friend class raw_filesys_c::locker;
    class locker
    {
        raw_filesys_c* dp;

    public:
        locker( raw_filesys_c* objp )
        {
            ( dp = objp )->lock( );
        }
        ~locker( )
        {
            dp->unlock( );
        }
    };

    // Filename path
    char path[ filename_max + 1 ];

public:
    ~raw_filesys_c( )
    {
        pthread_mutex_destroy( &bm );
    }

    raw_filesys_c( )
    {
        pthread_mutex_init( &bm, NULL );
        path[ 0 ] = '\000';
    }

    // Set the bits to construct the filename from: path
    int
    set_filename_attrs( char* pth )
    {
        locker mon( this );
        if ( strlen( pth ) > filename_max )
            return -1;
        strcpy( path, pth );
        return 0;
    }

    char*
    get_path( )
    {
        return path;
    }
};

#endif
