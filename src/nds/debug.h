#ifndef DEBUG_H
#define DEBUG_H

#include <config.h>
#include <sys/types.h>
#include <time.h>

#ifdef NDEBUG
#define DEBUG1( EX ) ( (void)0 )
#define DEBUG( L, EX ) ( (void)0 )
#else
extern int _debug;
#define DEBUG1( EX ) DEBUG( 1, EX )
#define DEBUG( L, EX )                                                         \
    if ( _debug >= L )                                                         \
    {                                                                          \
        EX;                                                                    \
    }
#endif /* ! NDEBUG */

#include <syslog.h>

extern int nds_log_level;

#if defined( DAEMONIC )

#define system_log( L, format, args... )                                       \
    {                                                                          \
        if ( nds_log_level >= L )                                              \
        {                                                                      \
            syslog( LOG_INFO, format, ##args );                                \
        }                                                                      \
    }
#else /* ! defined(DAEMONIC) */

#include <stdio.h>

#if !defined( _POSIX_C_SOURCE )

#define system_log( L, format, args... )                                       \
    {                                                                          \
        if ( nds_log_level >= L )                                              \
        {                                                                      \
            long t = time( 0 );                                                \
            char ___B__U__F___[ 27 ];                                          \
            ctime_r( &t, ___B__U__F___, 26 );                                  \
            ___B__U__F___[ 24 ] = 0;                                           \
            fprintf( stderr, "[%s] ", ___B__U__F___ );                         \
            fprintf( stderr, format, ##args );                                 \
            fputc( '\n', stderr );                                             \
        }                                                                      \
    }

#else /* defined(_POSIX_C_SOURCE) */

#define system_log( L, format, args... )                                       \
    {                                                                          \
        if ( nds_log_level >= L )                                              \
        {                                                                      \
            long t = time( 0 );                                                \
            char ___B__U__F___[ 27 ];                                          \
            ctime_r( &t, ___B__U__F___ );                                      \
            ___B__U__F___[ 24 ] = 0;                                           \
            fprintf( stderr, "[%s] ", ___B__U__F___ );                         \
            fprintf( stderr, format, ##args );                                 \
            fputc( '\n', stderr );                                             \
        }                                                                      \
    }

#endif /* defined(_POSIX_C_SOURCE) */

#endif /* ! defined(DAEMONIC) */

/*
#define numeric(c)              ((c) >= '0' && (c) <= '9')
#define max(a, b)               ((a) < (b) ? (b) : (a))
#define min(a, b)               ((a) > (b) ? (b) : (a))
#define abs(x)                  ((x) >= 0 ? (x) : -(x))
*/

#endif
