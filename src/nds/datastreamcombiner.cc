#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <dirent.h>
#include <poll.h>

#include <iostream>
#include <fstream>
#include <list>
#include <set>
#include <map>
#include <string>

#include "nds.hh"
#include "daqd_net.hh"
#include "mmstream.hh"

using namespace CDS_NDS;
using namespace std;

// Called to deal with complex request when signals are spread across multiple
// archives. Issue subjobs, one per archive. Let them do processing in parallel.
// Collect and combine results.
// Send combined data stream to the user.
//

bool
Nds::combineMinuteTrend( )
{
    unsigned long t_start = time( 0 ); // mark the beginning of the processing

    //  typedef std::map<int , std::vector<unsigned int> > CPT
    daqd_net::CPT cpm;
    unsigned int  nSignalIndices = 0;
    std::vector< unsigned int >
        signalIndices[ mSpec.getAddedArchives( ).size( ) + 1 ];

    // Write one file for striped minute trend data subjob
    //
    bool mainArchiveSignalsExist = false;
    for ( int i = 0; i < mSpec.getAddedFlags( ).size( ); i++ )
        if ( mSpec.getAddedFlags( )[ i ] == "0" )
        {
            mainArchiveSignalsExist = true;
            break;
        }

    if ( mainArchiveSignalsExist )
    {
        std::string   ofn = mSpecFileName + ".0";
        std::ofstream out( ofn.c_str( ) );
        if ( !out )
        {
            system_log( 1, "%s: open failed", ofn.c_str( ) );
            return false;
        }
        out << "# NDS raw striped minute trend subjob specification file"
            << endl;
        out << "datatype=rawminutetrend" << endl;
        out << "archive_dir=" << mSpec.getArchiveDir( ) << endl;
        out << "startgps=" << mSpec.getStartGpsTime( ) << endl;
        out << "endgps=" << mSpec.getEndGpsTime( ) << endl;
        out << "signals=";
        for ( int i = 0; i < mSpec.getAddedFlags( ).size( ); i++ )
        {
            if ( mSpec.getAddedFlags( )[ i ] == "0" )
            {
                out << mSpec.getSignalNames( )[ i ] << " ";
                signalIndices[ nSignalIndices ].push_back( i );
            }
        }
        out << endl << "types=";
        for ( int i = 0; i < mSpec.getAddedFlags( ).size( ); i++ )
        {
            if ( mSpec.getAddedFlags( )[ i ] == "0" )
                out << Spec::dataTypeString( mSpec.getSignalTypes( )[ i ] )
                    << " ";
        }
        out << endl << "signal_offsets=";
        for ( int i = 0; i < mSpec.getAddedFlags( ).size( ); i++ )
        {
            if ( mSpec.getAddedFlags( )[ i ] == "0" )
                out << mSpec.getSignalOffsets( )[ i ] << " ";
        }
        out << endl << "signal_slopes=";
        for ( int i = 0; i < mSpec.getAddedFlags( ).size( ); i++ )
        {
            if ( mSpec.getAddedFlags( )[ i ] == "0" )
                out << mSpec.getSignalSlopes( )[ i ] << " ";
        }
        out << endl;

        out.close( );
        nSignalIndices++;
    }

    // For each added archive write one config file
    //
    for ( int i = 0; i < mSpec.getAddedArchives( ).size( ); i++ )
    {
        char buf[ 16 ];
        sprintf( buf, ".%d", i + ( mainArchiveSignalsExist == true ) );
        std::string   ofn = mSpecFileName + buf;
        std::ofstream out( ofn.c_str( ) );
        if ( !out )
        {
            system_log( 1, "%s: open failed", ofn.c_str( ) );
            return false;
        }
        out << "# NDS minute trend frame archive subjob specification file"
            << endl;
        out << "datatype=minutetrend" << endl;
        const Spec::AddedArchive* a = &mSpec.getAddedArchives( )[ i ];
        out << "archive_dir=" << a->dir << endl;
        out << "archive_prefix=" << a->prefix << endl;
        out << "archive_suffix=" << a->suffix << endl;
        out << "startgps=" << mSpec.getStartGpsTime( ) << endl;
        out << "endgps=" << mSpec.getEndGpsTime( ) << endl;
        out << "times=";
        for ( int i = 0; i < a->gps.size( ); i++ )
        {
            out << a->gps[ i ].first << " " << a->gps[ i ].second << " ";
        }
        out << endl << "signals=";
        for ( int i = 0; i < mSpec.getAddedFlags( ).size( ); i++ )
        {
            if ( mSpec.getAddedFlags( )[ i ] == a->dir )
            {
                out << mSpec.getSignalNames( )[ i ] << " ";
                signalIndices[ nSignalIndices ].push_back( i );
            }
        }
        out << endl << "types=";
        for ( int i = 0; i < mSpec.getAddedFlags( ).size( ); i++ )
        {
            if ( mSpec.getAddedFlags( )[ i ] == a->dir )
                out << Spec::dataTypeString( mSpec.getSignalTypes( )[ i ] )
                    << " ";
        }
        out << endl;
        out.close( );
        nSignalIndices++;
    }

    // Here is one problem: striped minute trend calibration values are coming
    // from the current configuration (which is nuts BTW), which is not known to
    // the NDS server, only to the daqd. Resolution: 1) let daqd combine data;
    // OR 2) write calibraion values into the job spec file for the NDS.

    // For each subjob config file:
    //    Create communication fifo
    //    Connect UNIX socket on pipe (it spawns new 'nds')
    //    Send fifo input end file descriptor to the unix socket (to the subNDS)
    //    Send job subjob config file name to the unix socket (to the subNDS)
    // End for each
    int nSubJobs = mSpec.getAddedArchives( ).size( ) + mainArchiveSignalsExist;
    int fildes[ 2 * nSubJobs ]; // communication FIFOs (pipes)
    for ( int i = 0; i < 2 * nSubJobs; i += 2 )
    {
        if ( pipe( fildes + i ) < 0 )
        {
            system_log( 1, "pipe() failed; errno=%d", errno );
            // No matter close opened pipes or not, this process ends soon
            return false;
        }
    }

    // connect UNIX sockets on pipe (spawn sub-NDS processes)
    int socketfd[ nSubJobs ]; // sockets used to communicate to the sub-NDS
                              // processes
    for ( int i = 0; i < nSubJobs; i++ )
    {
        struct sockaddr_un servaddr;
        if ( sizeof( servaddr.sun_path ) - 1 < mPipeFileName.size( ) )
        {
            system_log( 1,
                        "pipe filename `%s' is too long; maximum size is %ld",
                        mPipeFileName.c_str( ),
                        sizeof( servaddr.sun_path ) - 1 );
            return false;
        }

        if ( ( socketfd[ i ] = socket( AF_UNIX, SOCK_STREAM, 0 ) ) < 0 )
        {
            system_log( 1, "UNIX socket(); errno=%d\n", errno );
            return false;
        }

        memset( &servaddr, 0, sizeof( servaddr ) );
        servaddr.sun_family = AF_UNIX;
        strcpy( servaddr.sun_path, mPipeFileName.c_str( ) );

        sleep( 1 );
        if ( connect( socketfd[ i ],
                      (struct sockaddr*)&servaddr,
                      sizeof( servaddr ) ) < 0 )
        {
            system_log( 1, "UNIX connect(); errno=%d\n", errno );
            return false;
        }
    }

    DEBUG( 5,
           for ( int i = 0; i < nSubJobs; i++ ) cerr
               << "sockedfd[" << i << "]=" << socketfd[ i ] << endl );

    for ( int i = 0; i < nSubJobs; i++ )
    {
        // send write end of the FIFOs to the sub-NDS servers
        struct msghdr msg;
        struct iovec  iov[ 1 ];

#ifdef __linux__
#define HAVE_MSGHDR_MSG_CONTROL

#ifndef CMSG_SPACE
#define CMSG_SPACE( size ) ( sizeof( struct msghdr ) + ( size ) )
#endif
#ifndef CMSG_LEN
#define CMSG_LEN( size ) ( sizeof( struct msghdr ) + ( size ) )
#endif
#endif

#ifdef HAVE_MSGHDR_MSG_CONTROL
        union
        {
            struct cmsghdr msg;
            char           control[ CMSG_SPACE( sizeof( int ) ) ];
        } control_un;
        struct cmsghdr* cmptr;

        msg.msg_control = control_un.control;
        msg.msg_controllen = sizeof( control_un.control );

        cmptr = CMSG_FIRSTHDR( &msg );
        cmptr->cmsg_len = CMSG_LEN( sizeof( int ) );
        cmptr->cmsg_level = SOL_SOCKET;
        cmptr->cmsg_type = SCM_RIGHTS;
        *( (int*)CMSG_DATA( cmptr ) ) = fildes[ 2 * i + 1 ];
#else
        msg.msg_accrights = ( caddr_t )( fildes + 2 * i + 1 );
        msg.msg_accrightslen = sizeof( int );
#endif

        msg.msg_name = NULL;
        msg.msg_namelen = 0;

#if __GNUC__ >= 3
        iov[ 0 ].iov_base = (void*)"";
#else
#ifdef __linux__
        iov[ 0 ].iov_base = (void*)"";
#else
        iov[ 0 ].iov_base = "";
#endif
#endif
        iov[ 0 ].iov_len = 1;
        msg.msg_iov = iov;
        msg.msg_iovlen = 1;

        errno = 0;
        int res = sendmsg( socketfd[ i ], &msg, 0 );
        if ( res != 1 )
        {
            system_log(
                1, "sendmsg() to NDS failed; res=%d; errno=%d", res, errno );
            return false;
        }
        else
        {
            DEBUG( 5,
                   cerr << "file descriptor " << i << " sent; result was "
                        << res << endl );
        }

        // send job spec file name
        char buf[ 16 ];
        sprintf( buf, ".%d", i );
        std::string spec_filename = mSpecFileName + buf;
        char        a = strlen( spec_filename.c_str( ) ) + 1;
        if ( 1 != write( socketfd[ i ], &a, 1 ) )
        {
            system_log( 1, "write(1) to NDS failed; errno=%d", errno );
            return 1;
        }

        if ( write( socketfd[ i ], spec_filename.c_str( ), (int)a ) != (int)a )
        {
            system_log( 1, "write(jobn) to NDS failed; errno=%d", errno );
            return 1;
        }
    }

    // Close write ends of the FIFOs and assign read ends into the combination
    // processing map
    for ( int i = 0; i < nSubJobs; i++ )
    {
        close( fildes[ 2 * i + 1 ] );
        cpm.insert( std::pair< int, std::vector< unsigned int > >(
            fildes[ 2 * i ], signalIndices[ i ] ) );
    }

    daqd_net daqd_net( mDataFd, mSpec, cpm );

    // Poll on the fifo read descriptors and unix socket pipe descriptors:
    //   if unix socket pipe -- read integer (seq_num); close communication with
    //   corresponding subNDS if fifo read descriptor -- receive data block and
    //   do combination processing.

#if 0
/*
 * Structure of file descriptor/event pairs supplied in
 * the poll arrays.
 */
typedef struct pollfd {
	int fd;				/* file desc to poll */
	short events;			/* events of interest on fd */
	short revents;			/* events that occurred on fd */
} pollfd_t;

typedef unsigned long	nfds_t;

/*
 * Testable select events
 */
#define POLLIN 0x0001 /* fd is readable */
#define POLLPRI 0x0002 /* high priority info at fd */
#define POLLOUT 0x0004 /* fd is writeable (won't block) */
#define POLLRDNORM 0x0040 /* normal data is readable */
#define POLLWRNORM POLLOUT
#define POLLRDBAND 0x0080 /* out-of-band data is readable */
#define POLLWRBAND 0x0100 /* out-of-band data is writeable */

#define POLLNORM POLLRDNORM

/*
 * Non-testable poll events (may not be specified in events field,
 * but may be returned in revents field).
 */
#define POLLERR 0x0008 /* fd has error condition */
#define POLLHUP 0x0010 /* fd has been hung up on */
#define POLLNVAL 0x0020 /* invalid pollfd entry */
#endif // 0

    nfds_t nPollFds = nSubJobs;
#ifdef __linux__
    typedef struct pollfd pollfd_t;
#endif
    pollfd_t pollfd[ nPollFds ];
    for ( int i = 0; i < nPollFds; i++ )
    {
        // FIFO read end
        pollfd[ i ].fd = fildes[ 2 * i ];
        pollfd[ i ].events = POLLIN;
        pollfd[ i ].revents = 0;
    }

    for ( ; nPollFds; )
    {
        DEBUG( 5, cerr << "nPollFds=" << nPollFds << endl );
        DEBUG( 5,
               for ( int i = 0; i < nPollFds; i++ ) cerr
                   << i << " fd=" << pollfd[ i ].fd
                   << " events=" << pollfd[ i ].events
                   << " revents=" << pollfd[ i ].revents << endl );

        int res = poll( pollfd, nPollFds, 60 * 60 * 1000 );
        if ( res == 0 )
        {
            system_log(
                1,
                "poll(60min) timed out; %d subjobs initially; %d subjobs now",
                nSubJobs,
                (int)nPollFds );
            return false;
        }
        else if ( res < 0 )
        {
            system_log(
                1,
                "poll() error; errno=%d; %d subjobs initially; %d subjobs now",
                errno,
                nSubJobs,
                (int)nPollFds );
            return false;
        }
        else
        {
            pollfd_t new_pollfd[ nPollFds ];
            nfds_t   new_nPollFds = 0;

            for ( int i = 0; i < nPollFds; i++ )
            {
                // Check FIFOs read end
                if ( pollfd[ i ].fd > 0 )
                {
                    if ( pollfd[ i ].revents & POLLIN )
                    { // Data is waiting

                        // Read one data block and do the combination processing
                        // This may send data to the client (if one combined
                        // data block is ready)
                        if ( !daqd_net.comb_read( pollfd[ i ].fd ) )
                        {
                            return false;
                        }

                        pollfd[ i ].revents = 0;
                    }
                    else if ( pollfd[ i ].revents & POLLERR ||
                              pollfd[ i ].revents & POLLHUP ||
                              pollfd[ i ].revents & POLLNVAL )
                    {
#if 0
	  system_log(1,"poll() FIFO error; errno=%d", errno);
	  return false;
#endif
                        // Let daqd_net know that the subjob finished
                        if ( !daqd_net.comb_subjob_done( pollfd[ i ].fd,
                                                         seq_num ) )
                        {
                            return false;
                        }

                        close( pollfd[ i ].fd );
                        pollfd[ i ].revents = 0;
                        pollfd[ i ].fd = -1;
                    }
                }
                if ( pollfd[ i ].fd > 0 )
                    new_pollfd[ new_nPollFds++ ] = pollfd[ i ];
            }

            nPollFds = new_nPollFds;
            memcpy( pollfd, new_pollfd, sizeof( pollfd_t ) * nPollFds );
        }
    }

#if 0
  // Transmit merged data to the client
  daqd_net.combine_send_data();
#endif

#if 0
  // get seq_num back from the sub-NDS processes
  for (int i = 0; i < nSubJobs; i++) {
    int seq_num = 0;

    // This call blocks waiting for NDS to complete processing and
    // data transmission to the client
    if (read(socketfd[i], &seq_num, sizeof(int)) != sizeof(int)) {
      system_log(1,"read(seq_num) from sub-NDS %d failed; errno=%d", i, errno);
      return false;
    }

    DEBUG(5,cerr << "seq_num from sub-NDS " << i << " was " << seq_num << endl);
  }
#endif

    // Combination processing will be in a separate module. It will receive the
    // data from the subNDSs, combine data and send the combined result to the
    // user as soon as possible. It will zero-fill missing data.
    //

    system_log( 1, "merge processing overall time=%ld", time( 0 ) - t_start );
    return true;
}
