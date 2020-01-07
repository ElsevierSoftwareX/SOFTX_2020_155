#ifndef FILESYS_H
#define FILESYS_H

/*
  $Id: filesys.hh,v 1.16 2009/05/22 20:42:44 aivanov Exp $
*/
#include <time.h>
#include <stdlib.h>
#include "config.h"
#include "debug.h"
#include "sing_list.hh"
#include "circ.hh"

#include <new>
#include <iostream>
#include <fstream>
#include <iomanip>

#include <sys/stat.h>
#include <sys/types.h>

// Linkable time range block class.
// This is to store time ranges (up to range_block::max_range_blocks).
class range_block : public s_link
{
public:
    enum
    {
        max_range_blocks = 10
    };
    range_block( ) : num_ranges( 0 )
    {
    }
    range_block( range_block* rb ) : num_ranges( 0 )
    {
        num_ranges = rb->num_ranges;
        for ( int i = 0; i < num_ranges; i++ )
            d[ i ] = rb->d[ i ];
    }

    void
    destroy( )
    { /*~range_block();*/
        free( (void*)this );
    }
    range_block( time_t len, time_t time ) : num_ranges( 0 )
    {
        // Assign the first data range
        put( len, time );
    }

    int num_ranges;
    struct
    {
        time_t file_secs; // Length of a file in seconds (true for all files
                          // that fall into this range)
        time_t min_time; // The first second we have data for in this range
        time_t max_time; // The second after the last second we have the data
                         // for in this range
    } d[ range_block::max_range_blocks ];

    // Assign next data range
    void
    put( time_t len, time_t time )
    {
        d[ num_ranges ].file_secs = len;
        d[ num_ranges ].min_time = d[ num_ranges ].max_time = time;
        d[ num_ranges ].max_time += len;
        num_ranges++;
    }

    int
    is_full( )
    {
        return num_ranges >= range_block::max_range_blocks;
    }

    time_t
    current_min_time( )
    {
        return d[ num_ranges - 1 ].min_time;
    }
    time_t
    current_max_time( )
    {
        return d[ num_ranges - 1 ].max_time;
    }
    time_t
    current_file_secs( )
    {
        return d[ num_ranges - 1 ].file_secs;
    }
    time_t
    current_max_time_add( time_t t )
    {
        return d[ num_ranges - 1 ].max_time += t;
    }
    time_t
    current_max_time_sub( time_t t )
    {
        return d[ num_ranges - 1 ].max_time -= t;
    }
};

#ifdef MINI_NDS
#define MAX_FRAME_DIRS 10000
#else
#define MAX_FRAME_DIRS 1000
#endif

///  Filesystem map class
///
///  In this class I want to maintain the consistent map from the GPS time,
///  expressed in seconds, to the directory number (if any) where the
///  corresponding frame file is located.
///
///  There must be a way for the user to access frame files from the program's
///  previous run. For the `filesys_c' class this means that the time period
///  variables for each directory have to be properly initialized on the program
///  startup. This is done with the directory scan to find out time period(s)
///  for the files in directories.
///
///  It is possible to deal with holes in data (some data files missing) for a
///  directory. The list of periods is maintained for a directory.

class filesys_c
{
public:
    /// new style directories; gps time first digits
    static const int gps_time_dirs = 1;

    /// how many digits to keep in a directory
    /// 123456789 will result in directory '123'
    /// IMPORTANT: this is just matched NDS code in scanarchive.c
    /// if this is changed, you have to update NDS code
    static const int digits_in_dir = 5;

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
    friend class filesys_c::locker;
    class locker
    {
        filesys_c* dp;

    public:
        locker( filesys_c* objp )
        {
            ( dp = objp )->lock( );
        }
        ~locker( )
        {
            dp->unlock( );
        }
    };

    int frames_saved;
    int frames_lost;

    /// Filename suffix; no suffix if == \000
    char suffix[ 80 ];

    /// Filename prefix
    char prefix[ 80 ];

    /// Filename path
    char path[ FILENAME_MAX + 1 ];

    /// Time range for all directories
    time_t min_time;
    time_t max_time;

    /// Time ranges for directories
    struct
    {
        int nfiles; ///< The number of files in this directory

        int dir_num; ///< which directory

        /// Time range for the directory
        time_t min_time;
        time_t max_time;

        /// Linked list of blocks of time ranges
        s_list blist;
    } dir[ MAX_FRAME_DIRS ];

    int num_dirs; ///< how many data directories we have

    int cur_dir;

    int cur_update_dir;

    int files_per_dir;

    ///  Map GPS time (seconds) onto the directory number. Returns the timstamp
    ///  in
    ///  `*tr' for which the file exists (`*tr' is in the first second in the
    ///  file; it determines the filename).

    ///  `t' is the requested time.

    ///  `*fsecs' is set to the file length in seconds
    int
    dir_number( time_t t, time_t* tr, time_t* fsecs )
    {
        int fdir;
        int i;

        // Start looping from the current directory. Most of
        // requests will be for the last one, possibly.
        for ( i = 0, fdir = cur_dir; i < num_dirs; i++, ++fdir %= num_dirs )
        {
            if ( dir[ fdir ].min_time <= t && dir[ fdir ].max_time > t )
            {
                // At this point I should find time range for `t'
                // in `dir [fdir].blist' linked list of range blocks,
                // then set `*tr' to min_time + (min_time - t) / file_secs
                for ( s_link* clink = dir[ fdir ].blist.first( ); clink;
                      clink = clink->next( ) ) // over the range blocks
                {
                    range_block* rb = (range_block*)clink;

                    for ( int k = 0; k < rb->num_ranges;
                          k++ ) // over the ranges in the block `rb'
                    {
                        if ( rb->d[ k ].min_time <= t &&
                             rb->d[ k ].max_time > t )
                        {
                            *tr = rb->d[ k ].min_time +
                                ( ( ( t - rb->d[ k ].min_time ) /
                                    rb->d[ k ].file_secs ) *
                                  rb->d[ k ].file_secs );
                            {
                                *fsecs = rb->d[ k ].file_secs;
                                return cur_dir = fdir;
                            }
                        }
                    }
                }
                return -1;
            }
        }
        return -1;
    }

    char* _filename_dir( time_t pt, char* fname, int dnum, int framedt = 1 );

    inline int frame_length_secs( time_t, int );
    inline int valid_fname( char* );
    int        _update_dir( time_t, time_t, time_t, int );

    int wiper_enabled;

    static void* wiper( void* );
    void         start_wiper( int d );

    int period; ///< our file period in seconds

    inline time_t ftosecs( char* filename, time_t* dt );

public:
    char*
    get_path( )
    {
        return path;
    }
    char*
    get_prefix( )
    {
        return prefix;
    }
    char*
    get_suffix( )
    {
        return suffix;
    }

    inline static void secstof( time_t, char* );

    ~filesys_c( )
    {
        pthread_mutex_destroy( &bm );
        destroy_cb( );
    }

    filesys_c( filesys_c* fsmap, int p )
        : min_time( 0 ), max_time( 0 ), cur_dir( 0 ), cur_update_dir( -1 ),
          files_per_dir( 3600 ), wiper_enabled( 0 ), frames_saved( 0 ),
          frames_lost( 0 ), cb( 0 ), period( p )
    {
        //      DEBUG(2, cerr << "filesys_c initialized 1" << endl;);

        pthread_mutex_init( &bm, NULL );

        for ( int i = 0; i < MAX_FRAME_DIRS; i++ )
        {
            dir[ i ].nfiles = 0;
            dir[ i ].dir_num = i;
            dir[ i ].min_time = dir[ i ].max_time = 0;
        }

        num_dirs = fsmap->num_dirs;
        strcpy( prefix, fsmap->prefix );
        strcpy( path, fsmap->path );
        strcpy( suffix, fsmap->suffix );
    }

    filesys_c( int p )
        : min_time( 0 ), max_time( 0 ), cur_dir( 0 ), cur_update_dir( -1 ),
          files_per_dir( 3600 ), wiper_enabled( 0 ), frames_saved( 0 ),
          frames_lost( 0 ), cb( 0 ), period( p )
    {
        //      DEBUG(2, cerr << "filesys_c initialized 2" << endl;);

        pthread_mutex_init( &bm, NULL );

        for ( int i = 0; i < MAX_FRAME_DIRS; i++ )
        {
            dir[ i ].nfiles = 0;
            dir[ i ].dir_num = i;
            dir[ i ].min_time = dir[ i ].max_time = 0;
        }
        num_dirs = 0;
        strcpy( prefix, "C1-" );
        path[ 0 ] = '\000';
        suffix[ 0 ] = '\000';
    }

    int construct_cb( int );
    int destroy_cb( );

    void
    report_lost_frame( )
    {
        frames_lost++;
    }

    void
    enable_wiper( )
    {

        if ( gps_time_dirs )
        {
            system_log( 1,
                        "Frame file wiper cannot be enabled when "
                        "gps_time_dirs==1\nWrite external script to clean up "
                        "old frame files\n" );
        }
        else
        {
            wiper_enabled = 1;
        }
    }
    void
    disable_wiper( )
    {
        wiper_enabled = 0;
    }

    /// Allows to update the map with another map
    /// NOT IMPLEMENTED YET
    void
    operator+=( filesys_c& fmap )
    {
        locker mon( this );
        std::cerr << "`+=' on the filesys_c" << std::endl;
    }

    /// Free current map, make a copy of the map in `fmap'
    /// `fmap' should have the same dir_num as this->dir_num.

    void
    operator=( filesys_c& fmap )
    {
        locker  mon( this );
        s_link* clink;

        for ( int i = 0; i < num_dirs; i++ )
        {
            // Free all ranges
            for ( clink = dir[ i ].blist.first( ); clink;
                  clink = dir[ i ].blist.first( ) )
            {
                dir[ i ].blist.remove( ); // Remove range block from the list
                //	  delete (range_block *) clink; // Free the range block
            }

            // See if there are any range blocks in `fsmap' we need to duplicate
            if ( i < fmap.num_dirs )
            {
                for ( clink = fmap.dir[ i ].blist.first( ); clink;
                      clink = fmap.dir[ i ].blist.next( ) )
                {
                    // FIXME: failure to malloc memory here would probably break
                    // the code that uses this class
                    void* mptr = malloc( sizeof( range_block ) );
                    if ( !mptr )
                    {
                        system_log( 1,
                                    "couldn't construct file map range block, "
                                    "memory exhausted" );
                        break;
                    }
                    dir[ i ].blist.insert(
                        new ( mptr ) range_block( (range_block*)clink ) );
                }
                dir[ i ].nfiles = fmap.dir[ i ].nfiles;
                dir[ i ].min_time = fmap.dir[ i ].min_time;
                dir[ i ].max_time = fmap.dir[ i ].max_time;
            }
            else
            {
                dir[ i ].nfiles = dir[ i ].min_time = dir[ i ].max_time = 0;
            }
        }

        // Update global min and max.
        min_time = fmap.min_time;
        max_time = fmap.max_time;

        // `cur_update_dir' is set in the scan to point to the dir with maximum
        // time
        cur_update_dir = fmap.cur_update_dir;
    }

    /*
      Following functions are the primary interface to the map:
    */
    /// starts a thread to periodically rescan files
    int
    start_periodic_scan( int s )
    {
        return 0;
    }
    /// use scan() to scan the directories with files to create the map
    int scan( );
    /// use update_dir() or new_fname_update() to update the map
    inline int
    update_dir( time_t gps, time_t gps_n, time_t period, int dnum )
    {
        locker mon( this );
        return _update_dir( gps, gps_n, period, dnum );
    }

#if defined( EPICS_IFILE_MONITOR )
    int new_fname_update( char* fname );
#endif

    ///  Maps GPS time (seconds) onto the file name. Returns directory number or
    ///  -1 if failed. Constructs filename in `fname' and sets
    ///  `*file_first_second' and `*file_length_seconds' on success.

    ///  `file_first_second' and `file_length_seconds' shouldn't point
    ///   to the same memory location.
    int
    filename( time_t  t,
              char*   fname,
              time_t* file_first_second,
              time_t* file_length_seconds )
    {
        locker mon( this );
        int    dnum;

        if ( ( dnum = dir_number(
                   t, file_first_second, file_length_seconds ) ) >= 0 )
            _filename_dir( *file_first_second, fname, dnum );

        return dnum;
    }

    ///  return file gps where the passed time is
    time_t
    file_gps( time_t gps )
    {
        time_t file_gps, file_secs;
        dir_number( gps, &file_gps, &file_secs );
        return file_gps;
    }

    ///  Get temporary file name in `*tmpfname' and the filename in `*fname'
    ///  based on the timestamp passed in `t'.
    ///  Returns directory number where the file goes to.
    int
    getDirFileNames(
        time_t t, char* tmpfname, char* fname, int frames = 1, int framedt = 1 )
    {
        locker mon( this );
        int    dnum = 0;

        if ( cur_update_dir > -1 )
        {
            dnum = cur_update_dir;

            // Switch to the next directory if current directory is full
            if ( dir[ cur_update_dir ].nfiles >= files_per_dir )
            {
                if ( num_dirs > 0 )
                {
                    ++dnum %= num_dirs;
                }
                else
                {
                    ++dnum;
                }
            }
        }

        if ( gps_time_dirs )
        {
            char buf[ 128 ];
            sprintf( buf, "%d", (int)t );
            int blen = strlen( buf );
            if ( digits_in_dir < blen )
                buf[ blen - digits_in_dir ] = 0;

            //	printf("%s\n", buf);

            // make the directory first
            char dname[ 128 ];
            sprintf( dname, "%s/%s", path, buf );
            mkdir( dname, 0777 );

            if ( framedt != 1 )
            {
                sprintf( fname,
                         "%s/%s/%s%d-%d%s",
                         path,
                         buf,
                         prefix,
                         (int)t,
                         framedt,
                         suffix );
                sprintf( tmpfname,
                         "%s/%s/.%s%d-%d%s",
                         path,
                         buf,
                         prefix,
                         (int)t,
                         framedt,
                         suffix );
            }
            else
            {
                sprintf(
                    fname, "%s/%s/%s%d%s", path, buf, prefix, (int)t, suffix );
                sprintf( tmpfname,
                         "%s/%s/.%s%d%s",
                         path,
                         buf,
                         prefix,
                         (int)t,
                         suffix );
            }
        }
        else
        {

            DEBUG( 22,
                   std::cerr << "getDirFileNames(): t=" << t << ";dnum=" << dnum
                             << std::endl );

            if ( framedt != 1 )
            {
                sprintf( fname,
                         "%s%d/%s%d-%d%s",
                         path,
                         dnum,
                         prefix,
                         (int)t,
                         framedt,
                         suffix );
                sprintf( tmpfname,
                         "%s%d/.%s%d-%d%s",
                         path,
                         dnum,
                         prefix,
                         (int)t,
                         framedt,
                         suffix );
            }
            else
            {
                sprintf(
                    fname, "%s%d/%s%d%s", path, dnum, prefix, (int)t, suffix );
                sprintf( tmpfname,
                         "%s%d/.%s%d%s",
                         path,
                         dnum,
                         prefix,
                         (int)t,
                         suffix );
            }
        }
        return dnum;
    }

    ///  Get filename in `*fname' based on the timestamp passed
    ///  in `t'. Returns directory number where the file goes to.
    int
    dir_fname( time_t t, char* fname, int framedt = 1 )
    {
        locker mon( this );
        int    dnum = 0;

        if ( cur_update_dir > -1 )
        {
            dnum = cur_update_dir;

            // Switch to the next directory if current directory is full
            if ( dir[ cur_update_dir ].nfiles >= files_per_dir )
            {
                if ( num_dirs > 0 )
                {
                    ++dnum %= num_dirs;
                }
                else
                {
                    ++dnum;
                }
            }
        }

        _filename_dir( t, fname, dnum, framedt );
        return dnum;
    }

    /// Set the bits to construct the filename from: suffix, prefix and path
    int
    set_filename_attrs( char* sfx, char* prx, char* pth )
    {
        locker mon( this );
        if ( strlen( prx ) > 79 )
            return -1;
        if ( strlen( sfx ) > 79 )
            return -1;
        if ( strlen( pth ) > FILENAME_MAX )
            return -1;
        strcpy( prefix, prx );
        strcpy( path, pth );
        strcpy( suffix, sfx );
        return 0;
    }

    /// Set and get the number of directories in the map
    void
    set_num_dirs( int dirs )
    {
        locker mon( this );
        assert( dirs <= MAX_FRAME_DIRS );
        cur_dir = 0;
        num_dirs = dirs;
    }
    int
    get_num_dirs( )
    {
        locker mon( this );
        return num_dirs;
    }

    /// Set and get the number of files per directory
    void
    set_files_per_dir( int nfiles )
    {
        locker mon( this );
        files_per_dir = nfiles;
    }
    int
    get_files_per_dir( )
    {
        locker mon( this );
        return files_per_dir;
    }

    void
    print_times( std::ostream& out )
    {
        for ( int i = 0; i < num_dirs; i++ )
        {
            range_block* rb = (range_block*)dir[ i ].blist.first( );
            if ( !rb )
                out << "0 0 ";
            else
                out << dir[ i ].min_time << " " << dir[ i ].max_time << " ";
        }
    }

    void
    print_stats( std::ostream* yyout, int what )
    {
        locker mon( this );
        char   tmpf[ filesys_c::filename_max + 10 ];

        *yyout << "fundamental min_time=" << min_time << " `"
               << _filename_dir( min_time, tmpf, 0 ) << "'" << std::endl;
        *yyout << "fundamental max_time=" << max_time << " `"
               << _filename_dir( max_time, tmpf, 0 ) << "'" << std::endl;

        if ( what < 0 || what > num_dirs )
            return;

        for ( int i = what ? what - 1 : 0; i < ( what ? what : num_dirs ); i++ )
        {
            range_block* rb = (range_block*)dir[ i ].blist.first( );

            if ( !rb )
                *yyout << "Directory " << i << " is empty" << std::endl;
            else
            {
                *yyout << "Directory " << i << std::endl;
                *yyout << "directory " << i << " nfiles=" << dir[ i ].nfiles
                       << std::endl;
                *yyout << "directory " << i << " minimum=" << dir[ i ].min_time
                       << " `" << _filename_dir( dir[ i ].min_time, tmpf, i )
                       << "'" << std::endl;
                *yyout << "directory " << i << " maximum=" << dir[ i ].max_time
                       << " `" << _filename_dir( dir[ i ].max_time, tmpf, i )
                       << "'" << std::endl;
                for ( s_link* clink = rb; clink; clink = clink->next( ) )
                {
                    range_block* rb = (range_block*)clink;

                    *yyout << "num_ranges=" << rb->num_ranges << std::endl;
                    for ( int k = 0; k < rb->num_ranges; k++ )
                        *yyout << "range " << k
                               << " file_secs=" << rb->d[ k ].file_secs
                               << "\n\tmin_time=" << rb->d[ k ].min_time << " `"
                               << _filename_dir( rb->d[ k ].min_time, tmpf, i )
                               << "'"
                               << "\n\tmax_time=" << rb->d[ k ].max_time << " `"
                               << _filename_dir( rb->d[ k ].max_time, tmpf, i )
                               << "'" << std::endl;
                }
            }
        }
    }

    time_t
    get_frames_saved( )
    {
        locker mon( this );
        return frames_saved;
    }
    time_t
    get_frames_lost( )
    {
        locker mon( this );
        return frames_lost;
    }
    time_t
    get_min( )
    {
        locker mon( this );
        return min_time;
    }
    time_t
    get_max( )
    {
        locker mon( this );
        return max_time;
    }
    int
    is_empty( )
    {
        locker mon( this );
        return !( min_time | max_time );
    }
    int
    get_cur_dir( )
    {
        locker mon( this );
        return cur_update_dir;
    }

    // _update_dir() call (or update_dir() call) puts new filename in this
    // buffer.
    circ_buffer* cb;
};

/// See if the `fname' has valid for us timestamp in it
inline int
filesys_c::valid_fname( char* fname )
{
    // FIXME: do filename regexp matching on the new format: C1-123456789.{F,T}
#if defined( GPS_YMDHS_IN_FILENAME )
    if ( strlen( fname ) < 17 )
        return 0;
#else
    if ( strlen( fname ) < 3 )
        return 0;
#endif

    // Check whether file name starts with the correct prefix
    if ( strncmp( fname, prefix, strlen( prefix ) ) )
        return 0;

    // #include <regexp.h>
    // do regular expression matching here, to determine if there is
    // really a formatted `yy_mm_dd_hh_mm_ss' timestamp in the `*fname'

    return 1;
}

///  Convert `filename' into the UTC time
inline time_t
filesys_c::ftosecs( char* filename, time_t* dt )
{
    int times, frames, framedt;
    int scanned = sscanf( filename, "%d-%d", &times, &framedt );
    switch ( scanned )
    {
    case 1:
        *dt = period;
        return times;
    case 2:
        *dt = framedt;
        return times;
    default:
        return -1;
    }
}

///  Convert UTC time `secs' into the filename-style string timestamp,
///  which is written out into `outs'.
inline void
filesys_c::secstof( time_t secs, char* outs )
{
#if !defined( GPS_YMDHS_IN_FILENAME )
    sprintf( outs, "%d", (int)secs );
#else
    struct tm tms;

    gmtime_r( &secs, &tms );
    tms.tm_mon++;

    sprintf( outs,
             "%02d_%02d_%02d_%02d_%02d_%02d",
             tms.tm_year >= 100 ? tms.tm_year - 100 : tms.tm_year,
             tms.tm_mon,
             tms.tm_mday,
             tms.tm_hour,
             tms.tm_min,
             tms.tm_sec );
#endif
}

#endif
