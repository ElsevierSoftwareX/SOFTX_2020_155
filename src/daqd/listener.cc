#include <config.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>

#include <string>
#include <iostream>
#include <fstream>
#if __GNUC__ >= 3
#include <ext/stdio_filebuf.h>
#endif

using namespace std;

#include "circ.hh"

#include "y.tab.h"
#include "FlexLexer.h"
#include "channel.hh"
#include "daqc.h"
#include "daqd.hh"
#include "sing_list.hh"
#include "MyLexer.hh"

void *interpreter_no_prompt (void *);
void *interpreter (void *);
void *strict_interpreter (void *);
void *listener (void *);

extern daqd_c daqd;

int
net_listener::start_listener (ostream *yyout, int port, int pstrict)
{
  listener_port = port;
  this -> strict = pstrict;

  srvr_addr.sin_family = AF_INET;
  srvr_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  srvr_addr.sin_port = htons (port);
  pthread_attr_t attr;
  pthread_attr_init (&attr);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
  pthread_attr_setstacksize (&attr, daqd.thread_stack_size);
  //  pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
  int err_no;
  if (err_no = pthread_create (&tid, &attr, (void *(*)(void *))listener_static, (void *) this)) {
    pthread_attr_destroy (&attr);
    system_log(1, "couldn't create listener thread; pthread_create() err=%d", err_no);    
    return 1;
  }
  pthread_attr_destroy (&attr);
  system_log(3, "listener created; port=%d strict=%d tid=%lx", port, pstrict, (long)tid);
  return 0;
}

void *
net_listener::listener ()
//listener (void * a)
{
  const int on = 1;
  int onlen = sizeof (on);
  int srvr_addr_len;
  // Set thread parameters
  char my_thr_label[16];
  int lport = ntohs (srvr_addr.sin_port);
  sprintf(my_thr_label,"dql%.4d",lport);
  char my_thr_name[80];
  sprintf (my_thr_name,"listener port %.4d",lport);
  daqd_c::set_thread_priority(my_thr_name,my_thr_label,0,0); 

  if ((listenfd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
      system_log(1, "listener: socket(); errno=%d", errno);
      return NULL;
    }
  setsockopt (listenfd, SOL_SOCKET, SO_REUSEADDR, (const char *) &on, sizeof (on));

  srvr_addr_len = sizeof (srvr_addr);
  if (bind (listenfd, (struct sockaddr *) &(srvr_addr), srvr_addr_len) < 0)
    {
      system_log(1, "listener: bind(%d); errno=%d", srvr_addr.sin_port, errno);
      return NULL;
    }
  if (listen (listenfd, 2) < 0)
    {
      system_log(1, "listen(2); errno=%d", errno);
      return NULL;
    }
  for (;;) {
    int	connfd;
    struct sockaddr_in raddr;
    socklen_t len = sizeof (raddr);

    if (daqd.shutting_down)
      return NULL;

    if ( (connfd = accept (listenfd, (struct sockaddr *) &raddr, &len)) < 0) {
#ifdef	EPROTO
      if (errno == EPROTO || errno == ECONNABORTED)
#else
	if (errno == ECONNABORTED)
#endif
	  {
	    DEBUG1(cerr << "accept() aborted\n" << endl);
	    continue;
	  }
	else
	  {
	    system_log(1, "accept(); errno=%d", errno);
	    return NULL;
	  }
    }
    {
      pthread_attr_t attr;
      pthread_t iprt;

      //      cerr << "connfd=" << connfd << endl;

      pthread_attr_init (&attr); /* initialize attr with default attributes */
      pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
      pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED); /* run the interpreter as the detached thread */
      pthread_attr_setstacksize (&attr, daqd.thread_stack_size);
      int err_no;
      int pthr_arg = connfd << 16 | dup(connfd);
      if (err_no = pthread_create (&iprt, &attr,
				       (void *(*)(void *))(strict? strict_interpreter: interpreter),
				       &pthr_arg)) {
	system_log(1, "couldn't create interpreter thread; pthread_create() err=%d", err_no);
	char buf [256];
	system_log(1, "connection dropped on port %d from %s; fd=%d", srvr_addr.sin_port, net_writer_c::ip_fd_ntoa (connfd, buf), connfd);
	pthread_attr_destroy (&attr);
	close (connfd);
      } else {
	pthread_attr_destroy (&attr);
	DEBUG(2, cerr << "interpreter started; tid=" << iprt << endl);
	char buf [256];
	system_log(1, "connection on port %d from %s; fd=%d", srvr_addr.sin_port, net_writer_c::ip_fd_ntoa (connfd, buf), connfd);
      }
    }
  }
}

void *
interpreter_no_prompt (void *a)
{
  const int ai1 = ((long) a) & 0xffff;
  const int ai2 = ((long) a) >> 16;
  my_lexer* lexer;

  {

#if __GNUC__ >= 3
    ofstream my_yyout;

//__gnu_cxx::stdio_filebuf

    FILE *cfile = fdopen(ai2,"w");
    std::ofstream::__filebuf_type* outbuf
        = new __gnu_cxx::stdio_filebuf<char> (cfile, std::ios_base::out);
    std::ofstream::__streambuf_type* oldoutbuf
        = ((std::ofstream::__ios_type* )&my_yyout)->rdbuf (outbuf);
    //delete oldoutbuf;

#else
    ofstream my_yyout (ai2);
#endif

    extern int yyparse (void *);

  //  pthread_detach (pthread_self ());

    void *mptr = malloc (sizeof(my_lexer));
    if (!mptr) {
      system_log(1,"couldn't construct lexer, memory exhausted");
      close (ai1);
      return NULL;
    }

    lexer = new (mptr) my_lexer (0, &my_yyout, ai1, ai2, 0, 0);
    if (daqd.b1)
      lexer -> cb = daqd.b1 -> buffer_ptr ();
    else
      lexer -> cb = NULL;

    lexer -> auth_ok = 1; // Set privilege to use all commands
    DEBUG(2, cerr << "calling yyparse(" << ai1 << ", " << ai2 << ")" << endl);
    my_yyout.flush ();
    yyparse ((void *) lexer);
    DEBUG(2, my_yyout << "lexer stopped at line " << lexer -> lineno () << endl);

    // cerr << "out of lexer" << endl;
#if __GNUC__ >= 3
    ((std::ofstream::__ios_type* )&my_yyout)->rdbuf (oldoutbuf);
    delete outbuf;
#endif
    my_yyout.flush ();
  }
  close (ai1);
  if (ai2 != ai1)
    close (ai2);
  lexer -> ~my_lexer ();
  free ((void *) lexer);
  return NULL;
}


void *
interpreter (void *a)
{
  const int ai1 = ((long) a) & 0xffff;
  const int ai2 = ((long) a) >> 16;
  my_lexer* lexer;

  {

#if  __GNUC__ >= 3
    ofstream my_yyout;
    FILE *cfile = fdopen(ai2,"w");
    std::ofstream::__filebuf_type* outbuf
        = new __gnu_cxx::stdio_filebuf<char> (cfile, std::ios_base::out);
    std::ofstream::__streambuf_type* oldoutbuf
        = ((std::ofstream::__ios_type* )&my_yyout)->rdbuf (outbuf);
    //delete oldoutbuf;
    
#else
    ofstream my_yyout (ai2);
#endif

    extern int yyparse (void *);

  //  pthread_detach (pthread_self ());

    void *mptr = malloc (sizeof(my_lexer));
    if (!mptr) {
      system_log(1,"couldn't construct lexer, memory exhausted");
      close (ai1);
      if (ai2 != ai1)
	close (ai2);
      return NULL;
    }

    lexer = new (mptr) my_lexer (0, &my_yyout, ai1, ai2, 0, 1);
    if (daqd.b1)
      lexer -> cb = daqd.b1 -> buffer_ptr ();
    else
      lexer -> cb = NULL;

    DEBUG(2, cerr << "calling yyparse(" << ai1 << ", " << ai2 << ")" << endl);
    my_yyout.flush ();
    yyparse ((void *) lexer);
    DEBUG(2, my_yyout << "lexer stopped at line " << lexer -> lineno () << endl);
    system_log(1, "connection on fd %d closed", ai2);

  // cerr << "out of lexer" << endl;
#if __GNUC__ >= 3
    ((std::ofstream::__ios_type* )&my_yyout)->rdbuf (oldoutbuf);
    delete outbuf;
#endif
    my_yyout.flush ();
  }

  close (ai1);
  if (ai2 != ai1)
    close (ai2);
  lexer -> ~my_lexer ();
  free ((void *) lexer);
  return NULL;
}

/*
  Start client access library support interpreter.
  This interpreter must have predetermined answers, so
  that the client's library won't be confused.
*/
void *
strict_interpreter (void *a)
{
  const int ai1 = ((long) a) & 0xffff;
  const int ai2 = ((long) a) >> 16;
  my_lexer* lexer;

  {

#if __GNUC__ >= 3
    ofstream my_yyout;
    FILE *cfile = fdopen(ai2,"w");
    std::ofstream::__filebuf_type* outbuf
        = new __gnu_cxx::stdio_filebuf<char> (cfile, std::ios_base::out);
    std::ofstream::__streambuf_type* oldoutbuf
        = ((std::ofstream::__ios_type* )&my_yyout)->rdbuf (outbuf);
    //delete oldoutbuf;

#else
    ofstream my_yyout (ai2);
#endif

    extern int yyparse (void *);
    
    //  pthread_detach (pthread_self ());

    void *mptr = malloc (sizeof(my_lexer));
    if (!mptr) {
      system_log(1,"couldn't construct lexer, memory exhausted");
      close (ai1);
      if (ai2 != ai1)
	close (ai2);
      return NULL;
    }

    lexer = new (mptr) my_lexer (0, &my_yyout, ai1, ai2, 1, 0);
    if (daqd.b1)
      lexer -> cb = daqd.b1 -> buffer_ptr ();
    else
      lexer -> cb = NULL;

    //  my_yyout << "calling yyparse(" << ai1 << ", " << ai2 << ")" << endl << flush;
    yyparse ((void *) lexer);
    //  my_yyout << "lexer stopped at line " << lexer -> lineno () << endl << flush;

    DEBUG(2, cerr << "out of lexer" << endl << flush);
#if __GNUC__ >= 3
    ((std::ofstream::__ios_type* )&my_yyout)->rdbuf (oldoutbuf);
    delete outbuf;
#endif
    my_yyout.flush ();
  }

#if defined(NETWORK_PRODUCER)
  // Use this connection to receive data from the controller
  if (lexer -> used_by_controller) {
    // Will not need the parser any more on this connection
    lexer -> ~my_lexer ();
    free ((void *) lexer);

    if (ai2 != ai1)
      close (ai2);


    if (lexer -> connection_num) {
      system_log(1, "secondary producer connected on fd %d", ai1);
      sem_post (&daqd.producer1.netp_sem1); // Signal the producer that the second connection is ready
      daqd.producer1.fd1 = ai1;
      return NULL; // End this thread
    }

    system_log(1, "primary producer connected on fd %d", ai1);
    daqd.producer1.receive_cb (ai1);
    close (ai1);
    system_log(1, "producer connection on fd %d closed", ai1);
  } else {

#endif

    close (ai1);
    if (ai2 != ai1)
      close (ai2);
    system_log(1, "connection on fd %d closed", ai2);
    lexer -> ~my_lexer ();
    free ((void *) lexer);

#if defined(NETWORK_PRODUCER)
  }
#endif

  return NULL;
}
