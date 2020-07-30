#ifndef PROFILER_HH
#define PROFILER_HH

#include "epics_pvs.hh"

/// Statistic gathering for main or trend circ buffer
class profile_c
{
    circ_buffer* cb;
    int          shutdown;
    int          started;
    int main_avg_free; /// Number of free block in the main circular buffer,
                       /// averaged over the profiler runtime
    int main_min_free; /// Minimal recorded number of free blocks in main
                       /// circular buffer over the profiler runtime
    int         period;
    int         num_counters;
    int*        counters;
    int         profiling_period;
    int         coredump;
    PV::PV_NAME reporting_dest;
    std::string name;

    /// Thread to collect program statistics
    void* profiler( );
    static void*
    profiler_static( void* a )
    {
        return ( (profile_c*)a )->profiler( );
    };

    void additional_checks( );

public:
    explicit profile_c( string pname, PV::PV_NAME reporting_pv = PV::PV_NAME::PV_PROFILER_FREE_SEGMENTS_MAIN_BUF )
        : main_avg_free( 0 ), main_min_free( -1 ), period( 0 ), started( 0 ),
          shutdown( 0 ), counters( nullptr ), num_counters(0), cb( 0 ), profiling_period( 1 ),
          coredump( 0 ), reporting_dest(reporting_pv), name( std::move(pname) )
    {
    }

    ~profile_c( )
    {
        if ( counters )
            free( (void*)counters );
    }

    void print_status( ostream* );
    void start_profiler( circ_buffer* );
    void stop_profiler( );
    inline void
    set_profiling_period( int secs )
    {
        profiling_period = secs;
    }
    inline void
    coredump_enable( )
    {
        coredump = 1;
    };
};

#endif
