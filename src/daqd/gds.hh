#ifndef GDS_C_H
#define GDS_C_H

#include "channel.hh"

class gds_c
{
private:
    int             signal_p;
    pthread_mutex_t signal_mtx;
    pthread_cond_t  signal_cv;

    char* construct_req_string( char* alias[], int nptr );

    // Scope locker
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
    friend class gds_c::locker;
    class locker
    {
        gds_c* dp;

    public:
        locker( gds_c* objp )
        {
            ( dp = objp )->lock( );
        }
        ~locker( )
        {
            dp->unlock( );
        }
    };

public:
    static const int max_gds_servers = 256;

    // GDS server host names
    char gds_servers[ max_gds_servers ][ 32 ];

    // The names of the control systems associated with the gds_servers and
    // gds_nodes
    char         gds_server_systems[ max_gds_servers ][ 32 ];
    unsigned int gds_nodes[ max_gds_servers ];
    int          n_gds_servers; // How many servers we have got

    // DCU id for each GDS server
    int dcuid[ max_gds_servers ];

public:
    //#ifdef USE_GM
    // int gdsTpCounter[DCU_COUNT];
    // int gdsTpTable[DCU_COUNT][GM_DAQ_MAX_TPS];
    //#endif

#ifdef DATA_CONCENTRATOR
    // Construct test point data block for broadcasting in concentrator
    char* build_tp_data( int* l, char* data, int bn );
#endif

    // Update test point tables with received broadcast
    void update_tp_data( unsigned int* d, char* dest );

public:
    gds_c( ) : signal_p( 0 ), n_gds_servers( 0 )
    {
        pthread_mutex_init( &bm, NULL );
        pthread_mutex_init( &signal_mtx, NULL );
        pthread_cond_init( &signal_cv, NULL );
        for ( int i = 0; i < max_gds_servers; i++ )
        {
            dcuid[ i ] = 0;
            gds_servers[ i ][ 0 ] = 0;
            gds_server_systems[ i ][ 0 ] = 0;
            gds_nodes[ i ] = 0;
        }
    }
    ~gds_c( )
    {
        pthread_mutex_destroy( &bm );
        pthread_mutex_destroy( &signal_mtx );
        pthread_cond_destroy( &signal_cv );
    }

    //
    // Gets the array of aliases `alias' for which
    // the request to GDS server is made.
    // Then it waits for the requested test point channels
    // to appear in the reflective memory and writes
    // to `gds[]' pointers to GDS channels that have
    // the data corresponding to each `alias[]' entry.
    // The caller must provide space sufficient to write
    // `nptr' elements into `gds[]'.
    // Returns -1 on error, 0 on success
    // Logs error into the error log
    //
#if 0
  int req_names (char *alias[], unsigned int *tpnum, channel_t *gds[], int nptr);
#endif
    int req_tps( long_channel_t* ac[], channel_t* gds[], int nptr );

    //
    // Sends channel clear request to the GDS server.
    // This disconnects named test points.
    // Return -1 on error, 0 on success
    // Logs error into error log.
    //
    int clear_names( char* alias[], int nptr );
    int clear_tps( long_channel_t* ac[], int nptr );

    // Set GDS server RPC connection attributes
    void set_gds_server( char* cfg_file_name );

    // Initialize GDS testpoint library
    int gds_initialize( );

    // Notify about alias table change
    void
    signal( )
    {
        pthread_mutex_lock( &signal_mtx );
        signal_p++;
        pthread_cond_signal( &signal_cv );
        pthread_mutex_unlock( &signal_mtx );
    }
};

#endif
