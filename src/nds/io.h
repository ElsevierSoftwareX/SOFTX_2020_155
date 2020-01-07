#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#ifndef BASIC_IO_H
#define BASIC_IO_H

class basic_io
{
public:
    /// Read "n" bytes from a descriptor.
    static ssize_t
    readn( int fd, void* vptr, size_t n )
    {
        size_t  nleft;
        ssize_t nread;
        char*   ptr;

        ptr = (char*)vptr;
        nleft = n;
        while ( nleft > 0 )
        {
            if ( ( nread = read( fd, ptr, nleft ) ) < 0 )
            {
                if ( errno == EINTR )
                    nread = 0; /* and call read() again */
                else
                    return ( -1 );
            }
            else if ( nread == 0 )
                break; /* EOF */

            nleft -= nread;
            ptr += nread;
        }
        return ( n - nleft ); /* return >= 0 */
    }
    /* end readn */

    /// Write "n" bytes to a descriptor.
    static ssize_t
    writen( int fd, const void* vptr, size_t n )
    {
        size_t      nleft;
        ssize_t     nwritten;
        const char* ptr;

        ptr = (char*)vptr;
        nleft = n;
        while ( nleft > 0 )
        {
            if ( ( nwritten = write( fd, ptr, nleft ) ) <= 0 )
            {
                if ( errno == EINTR )
                    nwritten = 0; /* and call write() again */
                else
                    return ( -1 ); /* error */
            }

            nleft -= nwritten;
            ptr += nwritten;
        }
        return ( n );
    }
    /* end writen */
};
#endif
