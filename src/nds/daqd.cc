///	\file nds/daqd.cc
///	\brief NDS test driver program.

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

///	\brief NDS test driver program.
int
main( int argc, char* argv[] )
{
    if ( argc != 3 )
    {
        fprintf( stderr,
                 "usage: %s <pipe file name> <result file name>\n",
                 argv[ 0 ] );
        return 1;
    }

    int fd = open( argv[ 2 ], O_WRONLY | O_CREAT, 0755 );
    if ( fd < 0 )
    {
        fprintf( stderr, "failed to open `%s'; errno %d\n", argv[ 2 ], errno );
        return 1;
    }
    printf( "`%s' was opened; fd=%d\n", argv[ 2 ], fd );

    int                socketfd;
    struct sockaddr_un servaddr, cliaddr;

    if ( ( socketfd = socket( AF_UNIX, SOCK_STREAM, 0 ) ) < 0 )
    {
        fprintf( stderr, "socket(); errno=%d\n", errno );
        return 1;
    }

    /* This helps to avoid waitings for dead socket to drain */
    {
        int on = 1;
        setsockopt( socketfd,
                    SOL_SOCKET,
                    SO_REUSEADDR,
                    (const char*)&on,
                    sizeof( on ) );
    }

    memset( &servaddr, 0, sizeof( servaddr ) );
    servaddr.sun_family = AF_UNIX;
    strcpy( servaddr.sun_path, argv[ 1 ] );

    if ( connect( socketfd, (struct sockaddr*)&servaddr, sizeof( servaddr ) ) <
         0 )
    {
        fprintf( stderr, "UNIX connect(); errno=%d\n", errno );
        return false;
    }

    // send the file descriptor
    struct msghdr msg;
    struct iovec  iov[ 1 ];

#ifdef HAVE_MSGHDR_MSG_CONTROL
    union
    {
        struct cmsghdr msg;
        char           control[ CMSG_SPACE( sizeof( int ) ) ];
    } control_un;
    struct cmsghdr* cmptr;

    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof( control_un.control );

    cmptr = CMGS_FIRSTHDR( &msg );
    cmptr->cmsg_len = CMGS_LEN( sizeof( int ) );
    cmptr->cmsg_level = SOL_SOCKET;
    cmptr->cmsg_type = SCM_RIGHTS;
    *( (int*)CMSG_DATA( cmptr ) ) = sendfd;
#else
    msg.msg_accrights = (caddr_t)&fd;
    msg.msg_accrightslen = sizeof( int );
#endif

    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    iov[ 0 ].iov_base = "";
    iov[ 0 ].iov_len = 1;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    int res = sendmsg( socketfd, &msg, 0 );
    printf( "file descriptor sent; result was %d\n", res );

    // send the job number
    char a = 2;
    char b[ 2 ] = "0";
    write( socketfd, &a, 1 );
    write( socketfd, b, 2 );
    return 0;
}
