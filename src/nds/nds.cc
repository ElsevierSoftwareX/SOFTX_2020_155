///	\file nds.cc
///	\brief UNIX daemon forking, job spec file reading code.

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <iostream>
#include <fstream>
#include <list>
#include <set>
#include <map>
#include <string>
#include <algorithm>
#include "nds.hh"
#include "io.h"

using namespace CDS_NDS;

Nds::Nds( std::string p )
    : mPipeFileName( p ), mSpecFileName( ), mResultFileName( ), mDataFd( ),
      mSpec( ), seq_num( 0 )
{
    ;
}

bool
Nds::run( )
{
    int                listenfd;
    struct sockaddr_un servaddr, cliaddr;

    if ( sizeof( servaddr.sun_path ) - 1 < mPipeFileName.size( ) )
    {
        system_log( 1,
                    "pipe filename `%s' is too long; maximum size is %ld",
                    mPipeFileName.c_str( ),
                    sizeof( servaddr.sun_path ) - 1 );
    }

    if ( ( listenfd = socket( AF_UNIX, SOCK_STREAM, 0 ) ) < 0 )
    {
        system_log( 1, "socket(); errno=%d\n", errno );
        return 1;
    }

    /* This helps to avoid waitings for dead socket to drain */
    {
        int on = 1;
        setsockopt( listenfd,
                    SOL_SOCKET,
                    SO_REUSEADDR,
                    (const char*)&on,
                    sizeof( on ) );
    }

    unlink( mPipeFileName.c_str( ) );
    memset( &servaddr, 0, sizeof( servaddr ) );
    servaddr.sun_family = AF_UNIX;
    strcpy( servaddr.sun_path, mPipeFileName.c_str( ) );

    if ( bind( listenfd, (struct sockaddr*)&servaddr, sizeof( servaddr ) ) < 0 )
    {
        system_log( 1, "bind(); errno=%d\n", errno );
        close( listenfd );
        return 1;
    }

    if ( listen( listenfd, 2 ) < 0 )
    {
        system_log( 1, "listen(); errno=%d\n", errno );
        close( listenfd );
        return 1;
    }

    int connfd;
    for ( ;; )
    {
        struct sockaddr raddr;
        socklen_t       len = sizeof( raddr );

        if ( ( connfd = accept( listenfd, &raddr, &len ) ) < 0 )
        {
#ifdef EPROTO
            if ( errno == EPROTO || errno == ECONNABORTED )
#else
            if ( errno == ECONNABORTED )
#endif
                continue;
            else
            {
                system_log( 1, "accept(); errno=%d\n", errno );
                return 1;
            }
        }

#if 1
        int fres = fork( );
        if ( !fres )
        {
            int fres = fork( );
            if ( !fres )
                break;
            if ( fres < 0 )
            {
                system_log( 1, "second fork() failed; errno=%d", errno );
            }
            exit( 0 ); // end the imtermediate process
        }
        else if ( fres < 0 )
        {
            system_log( 1, "fork() failed; errno=%d", errno );
        }
        else
            wait( 0 );
#else
        break;
#endif
        wait( 0 );
        close( connfd );
    }

    //
    // The following continues to run in the double forked child process
    //
    // Receive client data socket file descriptor
    //
    struct msghdr msg;
    struct iovec  iov[ 1 ];
    int           newfd;

#ifdef __linux__
#ifndef CMSG_SPACE
#define CMSG_SPACE( size ) ( sizeof( struct msghdr ) + ( size ) )
#endif
    union
    {
        struct cmsghdr msg;
        char           control[ CMSG_SPACE( sizeof( int ) ) ];
    } control_un;
    struct cmsghdr* cmptr;

    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof( control_un.control );
#else
    msg.msg_accrights = (caddr_t)&newfd;
    msg.msg_accrightslen = sizeof( int );
#endif

    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    char junk[ 1 ];
    iov[ 0 ].iov_base = junk;
    iov[ 0 ].iov_len = 1;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    int res = recvmsg( connfd, &msg, 0 );
    if ( res <= 0 )
    {
        system_log( 1, "recvmsg() failed; errno %d", errno );
        close( listenfd );
        return false;
    }

#ifdef __linux__

#ifndef CMSG_LEN
#define CMSG_LEN( size ) ( sizeof( struct msghdr ) + ( size ) )
#endif

    if ( ( cmptr = CMSG_FIRSTHDR( &msg ) ) != NULL &&
         cmptr->cmsg_len == CMSG_LEN( sizeof( int ) ) )
    {
        if ( cmptr->cmsg_level != SOL_SOCKET )
        {
            system_log( 1, "controls level isn't SOL_SOCKET" );
            return false;
        }
        if ( cmptr->cmsg_type != SCM_RIGHTS )
        {
            system_log( 1, "controls type isn't SCM_RIGHTS" );
            return false;
        }
        mDataFd = *( (int*)CMSG_DATA( cmptr ) );
    }
#else
    if ( msg.msg_accrightslen == sizeof( int ) )
    {
        mDataFd = newfd;
    }
#endif
    else
    {
        system_log( 1, "file descriptor was not passed" );
        return false;
    }

    // Receive job spec file name
    //
    char lenc;
    if ( read( connfd, &lenc, 1 ) != 1 )
    {
        system_log( 1, "read(1) failed; errno=%d", errno );
        return false;
    }
    char jobn[ (int)lenc ];

    if ( read( connfd, jobn, (int)lenc ) != (int)lenc )
    {
        system_log( 1, "read(%d) failed; errno=%d", (int)lenc, errno );
        return false;
    }

    mSpecFileName = jobn;
    mResultFileName = dirname( mSpecFileName ) + "/res";

    int res1 = chdir( dirname( mSpecFileName ).c_str( ) );

    //
    //  nice(2 * NZERO  -1);

    bool job_res = false;
    ;

    // Open and parse job specification file
    //
    if ( mSpec.parse( mSpecFileName ) )
    {

        DEBUG( 1, std::cerr << mSpecFileName << " parsed" << std::endl );

        // Process job according to the specification
        //

        if ( mSpec.getDataType( ) == Spec::RawMinuteTrendData )
        {
            // See if the data comes from more than one archive.
            if ( mSpec.getAddedArchives( ).size( ) != 0 )
            { // if there are some added archives
                // If all signals are in a single added archive no need to do
                // combination processing
                if ( mSpec.getAddedArchives( ).size( ) == 1 &&
                     ( mSpec.getAddedFlags( ).end( ) ==
                       find( mSpec.getAddedFlags( ).begin( ),
                             mSpec.getAddedFlags( ).end( ),
                             std::string( "0" ) ) ) )
                {
                    // See if this single archive is the obsolete channel
                    // archive
                    if ( mSpec.getAddedFlags( )[ 0 ] == "obsolete" )
                    {
                        mSpec.setSignalSlopes( 1.0 ); // Correct signal slopes
                        std::vector< std::string > v =
                            Spec::split( mSpec.getArchiveDir( ) );
                        job_res = true;
                        for ( int i = 0; i < v.size( ); i++ )
                            job_res &= rawMinuteTrend( v[ i ] );
                    }
                    else
                    {
                        mSpec.setMainArchive(
                            mSpec.getAddedArchives( )
                                [ 0 ] ); // make added archive into main archive
                        mSpec.setDataType( Spec::MinuteTrendData );
                        job_res = readTocFrameFileArchive( );
                    }
                }
                else
                    job_res = combineMinuteTrend( );
            }
            else
            {
                std::vector< std::string > v =
                    Spec::split( mSpec.getArchiveDir( ) );
                job_res = true;
                for ( int i = 0; i < v.size( ); i++ )
                    job_res &= rawMinuteTrend( v[ i ] );
            }
        }
        else
            job_res = readTocFrameFileArchive( );
    }

    //
    // Save gps time for the data that's included into the job trasmission image
    // ie. save "spec" for the data in file "job"
    //

    // send sequence number back to the daqd (even if processing failed)
    if ( basic_io::writen( connfd, &seq_num, sizeof( int ) ) != sizeof( int ) )
    {
        system_log( 1, "write(seq_num) to NDS failed; errno=%d", errno );
        res = false;
    }

#ifndef NDEBUG
    if ( job_res )
        std::cerr << "successfully processed" << std::endl;
    else
        std::cerr << "processing failed" << std::endl;
#endif

    return true;
}
