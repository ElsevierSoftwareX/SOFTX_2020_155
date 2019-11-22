#ifndef __STATS_PLUS_PLUS_H__
#define __STATS_PLUS_PLUS_H__

#include <math.h>
#include <limits>
#include <sys/time.h>
#include <ostream>

/*
 * class stats computes statistics for a running distribution including the
 * second moment
 */

/*
 *
 * Online variance computation according to Knuth
 *
def online_variance(data):
    n = 0
    mean = 0
    M2 = 0

    for x in data:
        n = n + 1
        delta = x - mean
        mean = mean + delta/n
        M2 = M2 + delta*(x - mean)

    variance = M2/(n - 1)
    return variance
*/

// numeric_limits<double>/*or float*/::infinity()
//
class stats
{
public:
    stats( )
    {
        do_clear = true;
        clear( );
    }
    double
    getMean( )
    {
        return mean;
    };
    double
    getMin( )
    {
        return min;
    };
    double
    getMax( )
    {
        return max;
    };
    double
    getN( )
    {
        return n;
    };
    double
    getStddev( )
    {
        if ( n < 1. )
            return .0;
        // Calculate the running variance
        return sqrt( M2 / ( n - 1.0 ) );
    };
    /*
     * Accumulate a sample
     */
    inline void
    accumulateNext( double x )
    {
        n++;
        double delta = x - mean;
        mean += delta / n;
        M2 += delta * ( x - mean );
        min = x < min ? x : min;
        max = x > max ? x : max;
    };
    /*
     * Begin the measurement of time segment to be completed
     * and recorded by a following call to tick()
     */
    inline void
    sample( )
    {
        clear( );
        t = cur_time( );
    }
    /*
     * Accumulate time difference since the last invocation
     * of tick() or sample()
     */
    inline void
    tick( )
    {
        clear( );
        double nt = cur_time( );
        if ( t != 0.0 )
            accumulateNext( nt - t );
        t = nt;
    }
    /*
     * Override to add your label, then call this one
     */
    virtual void
    print( std::ostream& os )
    {
        os << "mean=" << getMean( ) << " max=" << getMax( )
           << " min=" << getMin( ) << " stddev=" << getStddev( )
           << " n=" << getN( );
    }
    virtual void
    println( std::ostream& os )
    {
        print( os );
        os << std::endl;
    }
    /*
     * Print current date/time
     */
    void
    printDate( std::ostream& os )
    {
        time_t now = time( 0 );
        os << ctime( &now );
    }
    /*
     * Clear the statistics
     */
    virtual void
    clearStats( )
    {
        do_clear = true;
    }
    /*
     * Return the current system time with fractional microseconds
     */
    static inline double
    cur_time( )
    {
        struct timeval  tv;
        struct timezone tz;
        gettimeofday( &tv, &tz );
        // printf("%d %d \n", tv.tv_sec, tv.tv_usec);
        return ( (double)tv.tv_sec ) + ( (double)tv.tv_usec ) / 1000000.;
    }

private:
    double mean;
    double min;
    double max;
    double n;
    double M2;
    double t; // last tick's time
    bool   do_clear; // trigger to clear the accumulated statistics

    /*
     * Clear the accumulated stats if requested
     */
    inline void
    clear( )
    {
        if ( do_clear )
        {
            do_clear = false;
            mean = n = M2 = .0;
            min = std::numeric_limits< double >::max( );
            max = std::numeric_limits< double >::min( );
            t = .0;
        }
    }
};

#endif
