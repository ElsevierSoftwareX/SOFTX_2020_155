#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#ifndef VXWORKS
#include <pthread.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <signal.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <limits.h>

#include "channel.h"
#include "daqc.h"

/* map error code onto error messages */

int errcmap_max = DAQD_NOT_SUPPORTED;
char *errcmap [DAQD_NOT_SUPPORTED + 1] = {
             "unknown error",
	     "client request parse error",
	     "daqd is not configured",
	     "invalid IP address",
	     "invalid channel name",
/* 5 */	     "socket() failed",
	     "setsockopt() failed",
	     "connect() failed",
	     "daqd is overloaded, too busy",
	     "malloc() failed; virtual memory exhausted",
/* 10 */     "write() failed",
	     "daqd communication protocol version mismatch",
	     "no such net-writer",
	     "no data found",
	     "couldn't get caller's IP address",
/* 15 */     "dup() failed",
	     "invalid channel data rate",
	     "shutdown() failed",
	     "trend data is not available",
	     "full channel data is unavailable",
/* 20 */     "off-line data is not available",
	     "unable to create thread",
	     "too many channels in request",
	     "not supported"
};


static struct listenerArgs {
  int listenfd;
  pthread_mutex_t *lock;
  int error;
} listenerArgs;

/* Cleanup in the listener thread */
void
listener_cleanup (void *args) {
  close (((struct listenerArgs *)args) -> listenfd);
  ((struct listenerArgs *)args) -> error = 1;
  pthread_mutex_unlock ((((struct listenerArgs *)args) -> lock));
}


/* Convert hexadecimal string to integer */
/* Returns -1 if conversion failed */
static long
dca_strtol (char *str)
{
  long res;
  char *ptr;

  res = strtol (str, &ptr, 16);
  if (*ptr)
    return -1;
  return res;
}

#define MAX_RCV_CNT 100000
#define DAQC_ACCESS_VERBOSE_ERRORS 0

/* Read server response -- 4 bytes */
/* Convert the response to integer */
/* Return -1 for invalid response */

long
read_server_response_wait(int fd, int wt)
{
#define response_length 4
  int dca_errno;
  int bread;
  int oread, cnt;
  char buf [response_length + 1];
  int max_wt = wt? 100: MAX_RCV_CNT;

  /* Read server response */
  for (cnt = bread = oread = 0; oread < response_length; oread += bread, cnt++)
    {
      if (cnt > max_wt) {
#ifdef DAQC_ACCESS_VERBOSE_ERRORS
	fprintf(stderr, "read(): timeout\n");
#endif
	return -1;
      }
      if ((bread = read (fd, buf + oread, response_length - oread)) <= 0)
	{
	  if (errno != EAGAIN) {
#ifdef DAQC_ACCESS_VERBOSE_ERRORS
            char errmsgbuf[80];
            strerror_r(errno, errmsgbuf, sizeof(errmsgbuf));
	    fprintf (stderr, "read(); err=%s\n", errmsgbuf);
#endif
	    return -1;
	  }
	}
       if (bread < 0) bread = 0;
       if (wt) usleep(100000);
    }
  buf [response_length] = '\000';
  return dca_strtol (buf);
#undef response_length
}

long read_server_response (int fd) { return read_server_response_wait(fd, 0); }

/* Read 4 byte `long' */
unsigned long
read_long (int fd)
{
  int bread;
  int oread;
  char buf [4];

  for (bread = oread = 0; oread < 4; oread += bread)
    {
      if ((bread = read (fd, buf + oread, 4 - oread)) <= 0)
        {
	  if (errno != EAGAIN) {
#ifdef DAQC_ACCESS_VERBOSE_ERRORS
            char errmsgbuf[80];
            strerror_r(errno, errmsgbuf, sizeof(errmsgbuf));
            fprintf (stderr, "read(); err=%s\n", errmsgbuf);
#endif
	    return 0;
	  }
        }
      if (! bread) /* return zero on EOF -- might be not appropriate for all usage cases */
	return 0;
      if (bread < 0) bread = 0;
    }
  return ntohl (*(unsigned long *) buf);
}

/* 
   Read `numb' bytes.
   Returns number of bytes read or 0 on the error or EOF
*/
static int
read_bytes (int fd, char *cptr, int numb)
{
  int bread;
  int oread;

  for (bread = oread = 0; oread < numb; oread += bread)
    {
      if ((bread = read (fd, cptr + oread, numb - oread)) <= 0)
        {
	  if (errno != EAGAIN) {
#ifdef DAQC_ACCESS_VERBOSE_ERRORS
            char errmsgbuf[80];
            strerror_r(errno, errmsgbuf, sizeof(errmsgbuf));
            fprintf (stderr, "read(); err=%s\n", errmsgbuf);
#endif
	    return 0;
	  }
        }
      if (! bread)
	break;
      if (bread < 0) bread = 0;
    }
  return oread;
}

#ifndef VXWORKS

/* Thread that listens for requests */
static void *
listener (void * a)
{
  int listenfd;
  int srvr_addr_len;
// error message buffer
  char errmsgbuf[80];
  daq_t *daq = (daq_t *) a;

  if ((listenfd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
#ifdef DAQC_ACCESS_VERBOSE_ERRORS
      char errmsgbuf[80];
      strerror_r(errno, errmsgbuf, sizeof(errmsgbuf));
      fprintf (stderr, "socket(); err=%s\n", errmsgbuf);
#endif
      return NULL;
    }

  listenerArgs.listenfd = listenfd;
  listenerArgs.lock = &daq -> lock;
  listenerArgs.error = 0;
/*
  pthread_cleanup_push (listener_cleanup, (void *) &listenerArgs);
*/

#if 0
  {
    // const int max_allowed = 64 * 1024; /* 64K seems to be system imposed limit on Solaris */
    //      int sendbuf_size = (ssize && ssize < max_allowed)? ssize: max_allowed;
    int rcvbuf_size = 1024 * 10;
    
    if (setsockopt (listenfd, SOL_SOCKET, SO_RCVBUF, (const char *) &rcvbuf_size, sizeof (rcvbuf_size)))
#ifdef DAQC_ACCESS_VERBOSE_ERRORS
      fprintf (stderr, "setsockopt(%d, %d); errno=%d\n", listenfd, rcvbuf_size, errno);
#endif

    {
      int rcvbuf_size_len = 4;
      rcvbuf_size = -1;
      if (getsockopt (listenfd, SOL_SOCKET, SO_RCVBUF, (const char *) &rcvbuf_size, &rcvbuf_size_len))
#ifdef DAQC_ACCESS_VERBOSE_ERRORS
	fprintf (stderr, "getsockopt(%d, %d); errno=%d\n", listenfd, rcvbuf_size, errno);
#endif
      else
#ifdef DAQC_ACCESS_VERBOSE_ERRORS
	printf ("RCVBUF size is %d\n", rcvbuf_size);
#endif
    }

  }
#endif

  /* This helps to avoid waitings for dead socket to drain */
  {
    int on = 1;
    setsockopt (listenfd, SOL_SOCKET, SO_REUSEADDR, (const char *) &on, sizeof (on));
  }

  errno = 0;
  if (bind (listenfd, (struct sockaddr *) &daq -> listener_addr, sizeof (daq -> listener_addr)) < 0)
    {
      if (errno ==  EADDRINUSE || errno == 0) {
	short port = ntohs(daq -> listener_addr.sin_port) + 1;
	int stop_port = port + 1024;
	/* Listener's TCP port is already bound (in use)
	   Let's try to bind to the next one */
	for (; port < stop_port; port++) {
	  daq -> listener_addr.sin_port = htons (port);
	  if (bind (listenfd, (struct sockaddr *) &daq -> listener_addr, sizeof (daq -> listener_addr)) < 0) {
	    if (errno != EADDRINUSE && errno != 0) {
              char errmsgbuf[80];
              strerror_r(errno, errmsgbuf, sizeof(errmsgbuf));
	      fprintf (stderr, "bind(); err=%s\n", errmsgbuf);
	      return NULL;
	    }
	  } else
	    break;
	}
      } else {
        char errmsgbuf[80];
        strerror_r(errno, errmsgbuf, sizeof(errmsgbuf));
	fprintf (stderr, "bind(); err=%s\n", errmsgbuf);
	return NULL;
      }
    }

#if 0
  {
    //      const int max_allowed = 64 * 1024; /* 64K seems to be system imposed limit on Solaris */
    //      int sendbuf_size = (ssize && ssize < max_allowed)? ssize: max_allowed;
    int rcvbuf_size = 1024 * 10;
    
    if (setsockopt (listenfd, SOL_SOCKET, SO_RCVBUF, (const char *) &rcvbuf_size, sizeof (rcvbuf_size)))
#ifdef DAQC_ACCESS_VERBOSE_ERRORS
      fprintf (stderr, "setsockopt(%d, %d); errno=%d\n", listenfd, rcvbuf_size, errno);
#endif

    {
      int rcvbuf_size_len = 4;
      rcvbuf_size = -1;
      if (getsockopt (listenfd, 6, SO_RCVBUF, (const char *) &rcvbuf_size, &rcvbuf_size_len))
#ifdef DAQC_ACCESS_VERBOSE_ERRORS
	fprintf (stderr, "getsockopt(%d, %d); errno=%d\n", listenfd, rcvbuf_size, errno);
#endif
      else
#ifdef DAQC_ACCESS_VERBOSE_ERRORS
	printf ("RCVBUF size is %d\n", rcvbuf_size);
#endif
    }

  }
#endif

  if (listen (listenfd, 2) < 0)
    {
      char errmsgbuf[80];
      strerror_r(errno, errmsgbuf, sizeof(errmsgbuf));
      fprintf (stderr, "listen(); err=%s\n", errmsgbuf);
      return NULL;
    }

  pthread_mutex_unlock (&daq -> lock);

  for (;;) {
    int	connfd;
    struct sockaddr raddr;
    int len = sizeof (raddr);
    
    if (daq -> shutting_down)
      break;

    if ( (connfd = accept (listenfd, &raddr, &len)) < 0)
      {
#ifdef	EPROTO
	if (errno == EPROTO || errno == ECONNABORTED)
#else
	  if (errno == ECONNABORTED)
#endif
	    continue;
	  else
	    {
              char errmsgbuf[80];
              strerror_r(errno, errmsgbuf, sizeof(errmsgbuf));
	      fprintf (stderr, "accept(); err=%s\n", errmsgbuf);
	      return NULL;
	    }
      }

#if 0
    {
      int rcvbuf_size_len = 4;

      int rcvbuf_size = 1024 * 10;

#if 0
      if (setsockopt (connfd, SOL_SOCKET, SO_RCVBUF, (const char *) &rcvbuf_size, sizeof (rcvbuf_size)))
	fprintf (stderr, "setsockopt(%d, %d); errno=%d\n", connfd, rcvbuf_size, errno);
#endif

      rcvbuf_size = -1;
      if (getsockopt (connfd, SOL_SOCKET, SO_RCVBUF, (const char *) &rcvbuf_size, &rcvbuf_size_len))
	fprintf (stderr, "getsockopt(%d, %d); errno=%d\n", connfd, rcvbuf_size, errno);
      else
	printf ("connfd RCVBUF size is %d\n", rcvbuf_size);
    }
#endif

    /* Spawn working thread */
    {
      int err_no;
#if 0
      fprintf (stderr, "#connfd=%d\n", connfd);
#endif
      daq -> datafd = connfd;

      if (err_no = pthread_create (&daq -> interpreter_tid, NULL, (void *(*)(void *))daq -> interpreter, (void *) daq))
	{
          strerror_r(err_no, errmsgbuf, sizeof(errmsgbuf));
	  fprintf (stderr, "pthread_create() failed to spawn interpreter thread; err=%s", errmsgbug);
	  close (connfd);
	}
      else
	{
#if 0
	  fprintf (stderr, "#interpreter started; tid=%d\n", daq -> interpreter_tid);
#endif
	  pthread_join (daq -> interpreter_tid, NULL);
	}
    }
  }
/*
  pthread_cleanup_pop(1);
*/
}

/*
  Create network listener on `*tcp_port' that listens for DAQD server
  connections and spawns `start_func' as POSIX thread passing `daq' as the
  argument when connection is established.

  Returns: `daq' or `0' if failed; *tcp_port is set to the actual port used
  Port number can be increased to allow more than one program copy on the same
  system.

*/
daq_t *
daq_initialize (daq_t *daq, int *tcp_port, void * (*start_func)(void *))
{
  int err_no;

  memset (daq, 0, sizeof (daq));

  /* Assign TCP/IP address for the listener to listen at */
  daq -> listener_addr.sin_family = AF_INET;
  daq -> listener_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  daq -> listener_addr.sin_port = htons (*tcp_port);

  daq -> interpreter = start_func;

  /* Initialize mutex to synchronize with the listener initialization */
  pthread_mutex_init (&daq -> lock, NULL);
  pthread_mutex_lock (&daq -> lock);

  /* Start listener thread */
  if (err_no = pthread_create (&daq -> listener_tid, NULL, (void *(*)(void *))listener, (void *) daq))
    {
      char errmsgbuf[80];
      strerror_r(err_no, errmsgbuf, sizeof(errmsgbuf));
      fprintf (stderr, "pthread_create() failed to spawn a listener thread; err=%s", errmsgbuf);
      pthread_mutex_unlock (&daq -> lock);
      pthread_mutex_destroy (&daq -> lock);
      return 0;
    }

  /* Wait until listener unlocks it, signalling us that the
     initialization complete */
  pthread_mutex_lock (&daq -> lock);

  if (listenerArgs.error) return 0;

  /* Update listener port, which may be changed by the listener thread */
  *tcp_port = ntohs (daq -> listener_addr.sin_port);

  daq -> tb = 0;
  daq -> tb_size = 0;
  daq -> s = 0;
  daq -> s_size = 0;
  return daq;
}

#endif /* #ifndef VXWORKS */

/*
  Disconnect from the server and close the socket file descriptor
*/
int
daq_disconnect (daq_t *daq)
{
  char *command = "quit;\n";

  if (write (daq -> sockfd, command, strlen (command)) != strlen (command))
    {
#ifdef DAQC_ACCESS_VERBOSE_ERRORS
      char errmsgbuf[80];
      strerror_r(errno, errmsgbuf, sizeof(errmsgbuf));
      fprintf (stderr, "write(); err=%s\n", errmsgbuf);
#endif
      return DAQD_WRITE;
    }

  close (daq -> sockfd);
  return 0;
}


/*
  Connect to the DAQD server on the host identified by `ip' address.
  Returns zero if OK or the error code if failed.
*/
int
daq_connect (daq_t *daq, char *host, int port)
{
  int cntr = 0;
  int resp;
  int v, rv;
  int gherr;
#if defined __linux__ || defined __APPLE__
  extern int h_errno;
  struct hostent *hentp;
#else
  struct hostent hent;
  char buf [2048];
#endif

  if ((daq -> sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
      char errmsgbuf[80];
      strerror_r(errno, errmsgbuf, sizeof(errmsgbuf));
      fprintf (stderr, "socket(); err=%s\n", errmsgbuf);
      return DAQD_SOCKET;
    }

  {
    int on = 1;
    setsockopt (daq -> sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *) &on, sizeof (on));
  }
#if defined __linux__ || defined __APPLE__
  hentp = gethostbyname (host);
  if (!hentp) {
    fprintf (stderr, "Can't find hostname `%s'\n", host);
#ifdef DAQC_ACCESS_VERBOSE_ERRORS
    char errmsgbuf[80];
    strerror_r(h_errno, errmsgbuf, sizeof(errmsgbuf));
    fprintf (stderr, "Can't find hostname `%s'; gethostbyname(); err=%s\n", host, h_errno);
#endif
    close (daq -> sockfd);
    return DAQD_ERROR;
  }
  (void) memcpy(&daq -> srvr_addr.sin_addr.s_addr,
		*hentp -> h_addr_list, sizeof (daq -> srvr_addr.sin_addr.s_addr));

#else
  if (! gethostbyname_r (host, &hent, buf, 2048, &gherr)) {
    fprintf (stderr, "Can't find hostname `%s'\n", host);
#ifdef DAQC_ACCESS_VERBOSE_ERRORS
    char errmsgbuf[80];
    strerror_r(gherr, errmsgbuf, sizeof(errmsgbuf));
    fprintf (stderr, "Can't find hostname `%s'; gethostbyname_r(); err=%s\n", host, errmsgbuf);
#endif
    close (daq -> sockfd);
    return DAQD_ERROR;
  }

  (void) memcpy(&daq -> srvr_addr.sin_addr.s_addr,
		*hent.h_addr_list, sizeof (daq -> srvr_addr.sin_addr.s_addr));
#endif

  daq -> srvr_addr.sin_family = AF_INET;
  /*  daq -> srvr_addr.sin_addr.s_addr = inet_addr (ip); */
  daq -> srvr_addr.sin_port = htons (port? port: DAQD_PORT);

  fcntl(daq -> sockfd, F_SETFL, O_NONBLOCK);
  printf("Connecting..");
  fflush(stdout);
connect_again:
  if (connect (daq -> sockfd, (struct sockaddr *) &daq -> srvr_addr, sizeof (daq -> srvr_addr)) < 0)
    {
      if (cntr < 4 && (errno == EINPROGRESS || errno == EALREADY)) {
	sleep(1);
	cntr++;
  	printf("..");
  	fflush(stdout);
	goto connect_again;
      }
      if (errno != EISCONN) {
        //printf(" failed errno=%d; EALREADY=%d\n", errno, EALREADY);
        char errmsgbuf[80];
        strerror_r(errno, errmsgbuf, sizeof(errmsgbuf));
        fprintf (stdout, "connect(); err=%s\n", errmsgbuf);
        close (daq -> sockfd);
        return DAQD_CONNECT;
      }
    }
  printf(" done\n");
  fcntl(daq -> sockfd, F_SETFL, 0);

  // Doing this does not work on Solaris
#if defined __linux__ || defined __APPLE__
  {
  int fl = fcntl(daq -> sockfd, F_GETFL, 0);
  fcntl(daq -> sockfd, F_SETFL, O_NONBLOCK);
#endif
  /* Do protocol version check */
  if (resp = daq_send (daq, "version;"))
    return resp;

#if defined __linux__ || defined __APPLE__
  fcntl(daq -> sockfd, F_SETFL, fl);
  }
#endif

  /* Read server response */
  if ((v = read_server_response (daq -> sockfd)) != DAQD_PROTOCOL_VERSION)
    {
      fprintf (stderr, "communication protocol version mismatch: expected %d, received %d\n", DAQD_PROTOCOL_VERSION, v);
      close (daq -> sockfd);
      return DAQD_VERSION_MISMATCH;
    }

  /* Do protocol revision check */
  if (resp = daq_send (daq, "revision;"))
    return resp;

  /* Read server response */
  if ((rv = read_server_response (daq -> sockfd)) != DAQD_PROTOCOL_REVISION)
    fprintf (stderr, "Warning: communication protocol revision mismatch: expected %d.%d, received %d.%d\n",
	     DAQD_PROTOCOL_VERSION, DAQD_PROTOCOL_REVISION, v, rv);

  daq -> rev = rv;
  daq -> datafd = daq -> sockfd;

#ifdef VXWORKS
  {
    int blen = 4*4096;
    if (setsockopt (daq -> sockfd, SOL_SOCKET, SO_SNDBUF, &blen, sizeof (blen)) < 0)
      {
        fprintf (stderr, "setsockopt SO_SNDBUF failed\n");
	close (daq -> sockfd);
        return DAQD_SETSOCKOPT;
      }
  }
#endif

  return 0;
}

/*
  Send `command' to DAQD, read and return response code
*/
int
daq_send (daq_t *daq, char *command)
{
  int resp;

  if (write (daq -> sockfd, command, strlen (command)) != strlen (command))
    {
#ifdef DAQC_ACCESS_VERBOSE_ERRORS
      char errmsgbuf[80];
      strerror_r(errno, errmsgbuf, sizeof(errmsgbuf));
      fprintf (stderr, "write(); err=%s\n", errmsgbuf);
#endif
      return DAQD_WRITE;
    }

  if (write (daq -> sockfd, "\n", 1) != 1)
    {
#ifdef DAQC_ACCESS_VERBOSE_ERRORS
      char errmsgbuf[80];
      strerror_r(errno, errmsgbuf, sizeof(errmsgbuf));
      fprintf (stderr, "write(); err=%s\n", errmsgbuf);
#endif
      return DAQD_WRITE;
    }

  /* Read server response */
  if (resp = read_server_response_wait (daq -> sockfd, 1))
    {
      if (resp > 0)
#ifndef DAQC_ACCESS_VERBOSE_ERRORS
	if (resp != DAQD_SHUTDOWN)
#endif
	  fprintf (stderr, "Server error %d: %s\n", resp, errcmap[resp%(errcmap_max+1)]);
      return resp;
    }
  return 0;
}

#ifndef VXWORKS
/*
  Kill listener thread. This relies on the fact that the interpreter thread
  (with which listener is synchronized) is finished and listener is blocked on
  the accept() right now.  If this is too much to bother about you could just
  exit().
*/
int
daq_shutdown (daq_t *daq)
{
  int sockfd;

  daq -> shutting_down = 1;

  /*
    For each listener thread: cancel it then connect to
    kick off the accept()
  */
  //pthread_cancel (daq -> listener_tid);

  if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
#ifdef DAQC_ACCESS_VERBOSE_ERRORS
      char errmsgbuf[80];
      strerror_r(errno, errmsgbuf, sizeof(errmsgbuf));
      fprintf (stderr, "socket(); err=%s\n", errmsgbuf);
#endif
      return DAQD_SOCKET;
    }
  connect (sockfd, (struct sockaddr *) &daq -> listener_addr, sizeof (daq -> listener_addr));
  return 0;
}
#endif /* #ifndef VXWORKS */

/*

  Receive one data block (data channel samples). 

  Data block is malloced and realloced as needed and pointer is assigned to
  `daq -> tb'. Block size is assigned to `daq -> tb_size'.
  
  Returns 0 if zero length data block is received. Zero length block consists
  of the block header and no data. It is sent by the server when it failed to
  find the data for the second, specified in the block header. This could
  only happen for the off-line data request.

  Returns -1 on error.

  Returns -2 on channel reconfiguration. Special reconfiguration data block was received
  from the server. For the client it meens he need to reread channel data conversion
  variables and status from the *daq structure.

  Returns number bytes of sample data read otherwise.

*/
int
daq_recv_block (daq_t *daq)
{
  long block_len;
  long bread, oread;
  long seconds;
  long alloc_size;

  /* read block length */
  if (! (block_len = read_long (daq -> datafd)))
    return -1;

#if 0
  fprintf (stderr, "#block length is %d\n", block_len);
#endif

  seconds = read_long (daq -> datafd);

  /* channel reconfiguration block (special block that's not data) */
  if (seconds == 0xffffffff) {
    long nchannels, i;
    char* cptr;

    /* finish reading the header */
    read_long (daq -> datafd);read_long (daq -> datafd);read_long (daq -> datafd);
    block_len -= HEADER_LEN;

    nchannels = block_len / sizeof(signal_conv_t1);
    if (nchannels * sizeof(signal_conv_t1) != block_len) {
      fprintf (stderr, "warning:channel reconfigure block length has bad length");
    }
    alloc_size = sizeof(signal_conv_t1)*nchannels;

    if (! daq -> s) {
      daq -> s = (signal_conv_t1 *) malloc (alloc_size);
      
      if (daq -> s)
	daq -> s_size = nchannels;
      else {
        char errmsgbuf[80];
        strerror_r(errno, errmsgbuf, sizeof(errmsgbuf));
	fprintf (stderr, "malloc(%ld) failed; err=%s\n", alloc_size, errmsgbuf);
	daq -> s_size = 0;
	return -1;
      }
    } else if (daq -> s_size != nchannels) {
      daq -> s = (signal_conv_t1 *) realloc ((void *) daq -> s, alloc_size);

      if (daq -> s)
	daq -> s_size = nchannels;
      else {
        char errmsgbuf[80];
        strerror_r(errno, errmsgbuf, sizeof(errmsgbuf));
	fprintf (stderr, "realloc(%ld) failed; err=%s\n", alloc_size, errmsgbuf);
	daq -> s_size = 0;
	return -1;
      }
    }

    for (i = 0; i < nchannels; i++) {
      if (read_bytes (daq -> datafd,
		      (char *) &daq -> s[i].signal_offset, sizeof(float)) != sizeof(float))
	return DAQD_ERROR;
      *((unsigned long *)&daq -> s[i].signal_offset) = ntohl (*((unsigned long *)&daq -> s[i].signal_offset));
      if (read_bytes (daq -> datafd,
		      (char *) &daq -> s[i].signal_slope, sizeof(float)) != sizeof(float))
	return DAQD_ERROR;
      *((unsigned long *)&daq -> s[i].signal_slope) = ntohl (*((unsigned long *)&daq -> s[i].signal_slope));
      daq -> s[i].signal_status = read_long (daq -> datafd);
    }

    return -2; 
  }


  {
    int bsize = sizeof(daq_block_t) + block_len;

    if (! daq -> tb) {
      daq -> tb = (daq_block_t *) malloc (bsize);

      if (daq -> tb)
	daq -> tb_size = bsize;
      else {
        char errmsgbuf[80];
        strerror_r(errno, errmsgbuf, sizeof(errmsgbuf));
	fprintf (stderr, "malloc(%d) failed; err=%s\n", bsize, errmsgbuf);
	daq -> tb_size = 0;
	return -1;
      }
    } else if (daq -> tb_size < bsize) {
      daq -> tb = (daq_block_t *) realloc ((void *) daq -> tb, bsize);

      if (daq -> tb)
	daq -> tb_size = bsize;
      else {
        char errmsgbuf[80];
        strerror_r(errno, errmsgbuf, sizeof(errmsgbuf));
	fprintf (stderr, "realloc(%d) failed; err=%s\n", bsize, errmsgbuf);
	daq -> tb_size = 0;
	return -1;
      }
    }
  }

  daq -> tb -> secs = seconds;

#if 0
  fprintf (stderr, "#block length is %d seconds\n", daq -> tb -> secs);
#endif

  daq -> tb -> gps = read_long (daq -> datafd);
  daq -> tb -> gpsn = read_long (daq -> datafd);
  daq -> tb -> seq_num = read_long (daq -> datafd);

#if 0
  fprintf (stderr, "seq_num=%d\n", daq -> tb -> seq_num);
#endif

  /* block length does not include the length of itself while the header length does */
  block_len -= HEADER_LEN;

  /* zero length block received */
  if (! block_len)
    return 0;

  /* read data samples */
  for (oread = 0, bread = 0; oread < block_len; oread += bread)
    {
      if ((bread = read (daq -> datafd, daq -> tb -> data + oread, block_len - oread)) <= 0)
	{
	  if (errno != EAGAIN) {
#ifdef DAQC_ACCESS_VERBOSE_ERRORS
            char errmsgbuf[80];
            strerror_r(errno, errmsgbuf, sizeof(errmsgbuf));
	    fprintf (stderr, "read(2) error=%s; oread=%ld;\n", errmsgbuf, oread);
#endif
	    return -1;
	  }
	}
      if (!bread)
	return -1;
      if (bread < 0) bread = 0;
    }
  return oread;
}

/*
  This returns 0 for online data feed and positive number for off-line.
*/
int
daq_recv_block_num (daq_t *daq)
{
  return read_long (daq -> datafd);
}

/*
  Close data connection socket 
*/
int
daq_recv_shutdown (daq_t *daq)
{
  if (daq -> tb)
    free (daq -> tb);

  daq -> tb = 0;
  daq -> tb_size = 0;

  if (daq -> s)
    free (daq -> s);

  daq -> s = 0;
  daq -> s_size = 0;

  return close (daq -> datafd);
}

/*
  Get channels description
  Returns zero on success.
  Sets `*num_channels_received' to the actual number of configured channels.
*/
int
daq_recv_channels (daq_t *daq, daq_channel_t *channel,
		   int num_channels, int *num_channels_received)
{
  int i;
  int resp;
  int channels;
  char buf [MAX_LONG_CHANNEL_NAME_LENGTH+2];

  {
    FILE *f;
#define READ_STRING \
    if (0 == fgets(buf, MAX_LONG_CHANNEL_NAME_LENGTH+1, f)) { \
      fclose(f); \
      return DAQD_ERROR; \
    }

    if (resp = daq_send (daq, "status channels 3;"))
      return resp;

    f = fdopen(dup(daq -> sockfd), "r");
    READ_STRING;
    channels = atoi(buf);

    if (channels <= 0) {
      fprintf (stderr, "couldn't determine the number of data channels\n");
      fclose(f); \
      return DAQD_ERROR;
    }

    *num_channels_received = channels;
    if (num_channels < channels)
      channels = num_channels;

    for (i = 0; i < channels; i++) {
	char *s;
	// Input channel name
        READ_STRING;
	s = strchr(buf, '\n'); if (s) *s = 0;
	strcpy(channel[i].name, buf);
	// Read rate
        READ_STRING;
	channel [i].rate = atoi(buf);
	// Read data type
        READ_STRING;
	channel [i].data_type = atoi(buf);
	channel [i].bps = data_type_size(channel [i].data_type);
	// Read test point number
        READ_STRING;
	channel [i].tpnum = atoi(buf);
	// Read group number
        READ_STRING;
	channel [i].group_num = atoi(buf);
	// Read units
        READ_STRING;
	s = strchr(buf, '\n'); if (s) *s = 0;
	strncpy(channel [i].s.signal_units, buf, MAX_ENGR_UNIT_LENGTH-1);
	// Read gain,slope and offset
        READ_STRING;
	channel [i].s.signal_gain = atof(buf);
        READ_STRING;
	channel [i].s.signal_slope = atof(buf);
        READ_STRING;
	channel [i].s.signal_offset = atof(buf);
    }
    fclose(f);
    return 0;
#undef READ_STRING
  }

  /* Read the number of channels */
  if (channels <= 0) {
    fprintf (stderr, "couldn't determine the number of data channels\n");
    return DAQD_ERROR;
  }

  *num_channels_received = channels;
  if (num_channels < channels)
    channels = num_channels;

  for (i = 0; i < channels; i++) {
      char *cptr;

      if (read_bytes (daq -> sockfd, channel [i].name, MAX_CHANNEL_NAME_LENGTH) != MAX_CHANNEL_NAME_LENGTH)
	return DAQD_ERROR;

      /* null terminate the name; strip all trailing white space */
      for (cptr = channel [i].name + MAX_CHANNEL_NAME_LENGTH - 1; isspace (*cptr) && cptr > channel [i].name; cptr--)
	;
      *++cptr = '\000';


      if ((channel [i].rate = daq_recv_id (daq)) <= 0) return DAQD_ERROR;

#if 0
      if (channel[i].rate > 16384)
	printf("%s rate=%d\n", channel[i].name, channel[i].rate);
#endif

      if ((channel [i].tpnum = daq_recv_id (daq)) < 0) return DAQD_ERROR;

      if ((channel [i].group_num = read_server_response (daq -> sockfd)) < 0)
	return DAQD_ERROR;

      if ((channel [i].data_type = read_server_response (daq -> sockfd)) < 0)
	return DAQD_ERROR;

      channel [i].bps = data_type_size(channel [i].data_type);

      /* signal conversion data */
      if ((*((unsigned long*)&channel [i].s.signal_gain) = daq_recv_id (daq)) < 0)
	return DAQD_ERROR;

      if ((*((unsigned long*)&channel [i].s.signal_slope) = daq_recv_id (daq)) < 0)
	return DAQD_ERROR;

      if ((*((unsigned long*)&channel [i].s.signal_offset) = daq_recv_id (daq)) < 0)
	return DAQD_ERROR;

      if (read_bytes (daq -> sockfd, channel [i].s.signal_units, MAX_ENGR_UNIT_LENGTH) != MAX_ENGR_UNIT_LENGTH)
	return DAQD_ERROR;

      /* null terminate units string; strip all trailing white space */
      for (cptr = channel [i].s.signal_units + MAX_ENGR_UNIT_LENGTH-1;
	   isspace (*cptr) && cptr > channel [i].s.signal_units; cptr--)
	;
      *++cptr = '\000';      
  }
  return 0;
}


/*
  Get channel groups description
  Returns zero on success.
  Sets `*num_channel_groups_received' to the actual number of configured channel groups.
*/
int
daq_recv_channel_groups (daq_t *daq, daq_channel_group_t *group,
			 int num_groups, int *num_channel_groups_received)
{
  int i;
  int resp;
  int groups;

  if (resp = daq_send (daq, "status channel-groups;"))
    return resp;

  /* Read the number of channels */
  if ((groups = read_server_response (daq -> sockfd)) <= 0)
    {
      fprintf (stderr, "couldn't determine the number of channel groups\n");
      return DAQD_ERROR;
    }

  *num_channel_groups_received = groups;
  if (num_groups < groups)
    groups = num_groups;

  for (i = 0; i < groups; i++)
    {
      char *cptr;

      if (read_bytes (daq -> sockfd, group [i].name, MAX_CHANNEL_NAME_LENGTH) != MAX_CHANNEL_NAME_LENGTH)
	return DAQD_ERROR;

      /* null terminate the name; strip all trailing white space */
      for (cptr = group [i].name + MAX_CHANNEL_NAME_LENGTH - 1;
	   isspace (*cptr) && cptr > group [i].name;
	   cptr--)
	;
      *++cptr = '\000';
      if ((group [i].group_num = read_server_response (daq -> sockfd)) < 0)
	return DAQD_ERROR;
    }
    return 0;
}


/*
  Receive some ID, coded as two server responses, ie two groups, four hex digits in each
*/
unsigned long
daq_recv_id (daq_t *daq)
{
  return read_server_response (daq -> sockfd) << (sizeof (unsigned long) * CHAR_BIT / 2) | read_server_response (daq -> sockfd);
}
