/* Version: $Id$ */
#ifndef _LIGO_SOCKUTIL_H
#define _LIGO_SOCKUTIL_H

#ifdef __cplusplus
extern "C" {
#endif


/* Header File List: */
#ifndef __EXTENSIONS__
#define __EXTENSIONS__
#endif
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>

/** @name Socket Utilities
    This API provides additional utility functions to deal with
    sockets.

    @memo Socket Utilities
    @author Written August 1999 by Daniel Sigg
    @version 0.1
************************************************************************/

/*@{*/

/** Looks up a hostname. This function uses DNS if necessary. If the
    hostname is NULL, the (first) address of the local machine is 
    returned.

    @param hostname name of the host (NULL for self)
    @param addr IP address (return)
    @return 0 if successful, <0 otherwise
    @author DS, July 98
************************************************************************/
   int nslookup (const char* hostname, struct in_addr* addr);

/** Looks up an address (inverse lookup). This function uses DNS if 
    necessary. If the address is NULL, the name of the local 
    machine is returned. Hostname should contain at least 256 chars.

    @param addr IP address
    @param hostname name of the host (return)
    @return 0 if successful, <0 otherwise
    @author DS, July 98
************************************************************************/
   int nsilookup (const struct in_addr* addr, char* hostname);

/** Connects a socket with a timeout. This function is similar to 
    connect, but implements an additional user specified timeout.

    @param sock socket
    @param name address to conenct
    @param size of socket address
    @param timeout connection timeout
    @return 0 if successful, <0 otherwise
    @author DS, July 98
************************************************************************/
   int connectWithTimeout (int sock, struct sockaddr* name, int size, 
                     struct timeval* timeout);

/** Pings the given host. This ping will try to establish a connection
    to the echo port of the specified host machine. If a connection
    is established or refused, ping returns with true. If the connection
    attempt aborts with a timeout, the function returns false. A non
    positive timeout value is interpreted as the default (10 sec).

    @param hostname name of the machine to ping
    @param timeout maximum time to wait for response (sec)
    @return true if alive, false otherwise
    @author DS, July 98
************************************************************************/
   int ping (const char* hostname, double timeout);


/*@}*/

#ifdef __cplusplus
}
#endif

#endif /*_LIGO_SOCKUTIL_H */
