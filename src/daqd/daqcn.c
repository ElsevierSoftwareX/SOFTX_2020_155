
/*
  DAQD channel access using control connection for data transport 
*/

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <sys/timeb.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>

#include "channel.h"
#include "daqc.h"

/* Listener listens on this TCP port */
#define LISTENER_PORT 9080

/* Connect to DAQD on the localhost */
#define DAQD_HOST "localhost"

daq_t daq;
void *interpreter (daq_t *);
char *programname;
int listening = 0;
int listener_port = LISTENER_PORT;
char *host = DAQD_HOST;

void
usage() 
{
  fputs ("give `-w' to dump first received block to stderr and quit\n", stderr);
  fputs ("give `-d' to see some data printed out\n", stderr);
  fputs ("give `-a' to print the data out as a string\n", stderr);
  fputs ("give `-c' to make it send arguments as a command and exit\n", stderr);
  fputs ("give `-p8089' to connect to DAQD on port 8089\n", stderr);
  fputs ("give `-hhostname' to connect to DAQD on hostname\n", stderr);
  exit(1);
}

int send_and_quit = 0;
int dump_to_stderr_and_quit = 0;
int data_printout = 0;
int ascii_data = 0;
int daqd_port = 8088;

daq_channel_t channels [MAX_CHANNELS];

main (int argc, char *argv[])
{
  int i;
  int dca_errno;
  char *configure_channels = "%s;";
  int bread;
  char buf [5];
  char cbuf [2048];
  int num_channels;
  unsigned long nid;
  int stop_this_nonsense;

  for (programname = argv [0] + strlen (argv [0]) - 1;
       programname != argv [0] && *programname != '/';
       programname--)
    ;
  if (*programname == '/')
    programname++;

  if (argc > 1 && '-' == *argv[1]) {
    for (i = 1; i < strlen (argv[1]); i++) {
      switch (argv[1][i]) {
      case 'd':
	data_printout = 1;
	break;
      case 'w':
	dump_to_stderr_and_quit = 1;
	break;
      case 'c':
        send_and_quit = 1;
        break;
      case 'a':
	ascii_data = 1;
	break;
      case 'p':
	daqd_port = atoi (&argv [1][i+1]);
	i = strlen (argv[1]);
	break;
      case 'h':
	host = &argv [1][i+1];
	i = strlen (argv[1]) - 1;
	break;
      default:
	usage();
	break;
      }
    }
    if (i==1)
      usage();

    argc--;
    argv++;
  }

  if (! strcmp (programname, "daqc"))
    listening = 1;

  if (listening)
    daq_initialize (&daq, &listener_port, interpreter);

  for (stop_this_nonsense = 1;stop_this_nonsense;stop_this_nonsense=0) {
    if (daq_connect (&daq, host, daqd_port))
      exit (1);


    if(0)
    {
      //      const int max_allowed = 64 * 1024; /* 64K seems to be system imposed limit on Solaris */
      //      int sendbuf_size = (ssize && ssize < max_allowed)? ssize: max_allowed;
      int rcvbuf_size = 20;

      if (setsockopt (daq.sockfd, SOL_SOCKET, SO_RCVBUF, (const char *) &rcvbuf_size, sizeof (rcvbuf_size)))
	fprintf (stderr, "setsockopt(%d, %d); errno=%d", daq.sockfd, rcvbuf_size, errno);
    }

    if (!send_and_quit) {
    if (i = daq_recv_channels (&daq, channels, MAX_CHANNELS, &num_channels))
      {
	fprintf (stderr, "Couldn't receive channel data; errno=%d\n", i);
	exit (1);
      }

    printf ("%d channels configured\n", num_channels);
    if(argc == 1) {
      printf ("name\tgroup\trate\ttpnum\tBPS\ttype\tgain\toffset\tslope\tunits\n");
      for (i = 0; i < num_channels; i++)
        printf ("%s\t%d\t%d\t%d\t%d\t%d\t\%f\t%f\t%f\t%s\n",
	      channels [i].name, channels [i].group_num,
	      channels [i].rate, channels [i].tpnum,
	      channels [i].bps, channels [i].data_type,
	      channels [i].s.signal_gain,
	      channels [i].s.signal_offset, channels [i].s.signal_slope, channels [i].s.signal_units);
    }
 } // ! send_and_quit
    if (listening) {
      if (argc > 1)
	sprintf (cbuf, configure_channels, argv [1]);
      else
	sprintf (cbuf, "start net-writer \"%s:%d\" all;", host, listener_port);
    } else
      sprintf (cbuf, configure_channels, argc > 1? argv [1]: "start net-writer all");

    puts (cbuf);
    fflush (stdout);

    /* start network writer  */
    if (daq_send (&daq, cbuf))
      exit (1);

    sleep(1);
    if (send_and_quit)
	exit(0);

    nid = daq_recv_id (&daq);
    printf ("net-writer started; ID=%lx\n", nid);
    
    if (listening)
      pthread_exit (0);
    else
      interpreter (&daq);
  }
}

void *
interpreter (daq_t *daq)
{
  unsigned long blocks, seq_num;
  long bread, oread;
  int j, i;
      float former_a = 0.;
      float last_a = 0.;
      float last_b = 0.;


#if 0
  /* Allocate some space, should be sufficient to put samples data for all configured channels */
#define bufsize 1024 * 1024 * 2
  daq_block_t *buf = (daq_block_t *) malloc (sizeof (daq_block_t) + bufsize);

  if (!buf)
    {
      fprintf (stderr, "dynamic memory exhausted\n");
      exit (1);
    }
#endif

  if (! (blocks = daq_recv_block_num (daq)))
    printf ("receiving data blocks online\n");
  else
    printf ("receiving %d data blocks\n", blocks);


  oread = 0;
  seq_num = ~0;
  for (j = 0; 1 || j< 100; j++)
    {
      int bread;

      if ((bread = daq_recv_block (daq)) < 0) {
	if (bread == -2) {
	  // reconfiguration;
	  int i;
	  printf("channel reconfiguration packet received\n");
	    printf("slope\toffset\tstatus\n");
	  for (i=0;i<daq->s_size;i++) {
	    printf("%f\t%f\t0x%x\n", daq->s[i].signal_slope, daq->s[i].signal_offset, daq->s[i].signal_status);
	  }
	  continue;
	} else {
	  printf ("daq_recV_block() error; bread=%d\n", bread);
	  break;
	}
      }

      if (dump_to_stderr_and_quit) {
	write (2, daq -> tb -> data, bread);
	exit(1);
      }


#if 0
      // Compare data halves
      // This is useful to check on consistency of control signals:
      // daqcn 'start net-writer {"H1:LSC-ETMY_OUT" "H1:SUS-ETMY_LSC_IN1" }'
      //
      for (i = 0; i < bread/2; i += 4) {
	float a, b;
	a = ((float *)(daq -> tb -> data + i))[-1];
	switch (i/4) {
		case 0:
			a = former_a;	
			break;
		case 16383:
			former_a = 
			  ((float *)(daq -> tb -> data + i))[0];
	}
	b = ((float *)(daq -> tb -> data + bread/2 + i))[0];
	if (a != b) {
		printf("%d: a=[%f]%f b=[%f]%f\n", i/4,
			a, last_a,
			b, last_b);
	}
	last_a = a;
	last_b = b;
      }
#endif

      if (daq -> tb -> seq_num != seq_num + 1) {
	printf ("%d block(s) dropped by the server\n", daq -> tb -> seq_num - seq_num - 1);
	printf ("seq_num=%d; status=%d\n", daq -> tb -> seq_num, seq_num);
      }

      seq_num = daq -> tb -> seq_num;

      //time_t t = time(0);
      struct timeb t;
      ftime(&t);

      /* Process received data here */
      printf ("%d bytes read; %d(s); time=%d--%d; seq_num=%d time=%dms\n", bread, daq->tb->secs, daq -> tb -> gps, daq -> tb -> gpsn, daq -> tb -> seq_num,
	t.millitm);

      if (!bread)
	  break;

      if (data_printout) {
	if (ascii_data) {
	  puts (daq -> tb -> data);
	} else {
	      int i;
#if 0
	      for (i = 0; i < bread /2; i++) {
		printf ("%hd ", ntohs(((short *)daq -> tb -> data) [i]));
	      }
#endif

#if 0
	      for (i = 0; i < bread /4; i++) {
		printf ("%d ", ntohl(((int *)daq -> tb -> data) [i]));
	      }
#endif
	      
#if 0
inline
double htond(double in) {
    double retVal;
    char* p = (char*)&retVal;
    char* i = (char*)&in;
    p[0] = i[7];
    p[1] = i[6];
    p[2] = i[5];
    p[3] = i[4];

    p[4] = i[3];
    p[5] = i[2];
    p[6] = i[1];
    p[7] = i[0];

    return retVal;
}

	      for (i = 0; i < bread /8; i++) {
		printf ("%f ", htond(((double *)daq -> tb -> data) [i]));
	      }
#endif

inline
float hton_float(float in) {
    float retVal;
    char* p = (char*)&retVal;
    char* i = (char*)&in;
    p[0] = i[3];
    p[1] = i[2];
    p[2] = i[1];
    p[3] = i[0];

    return retVal;
}
	      for (i = 0; i < bread /4; i++) {
		printf ("%f ", hton_float(((float *)daq -> tb -> data) [i]));
	      }

	      puts ("\n");
	}
      }

#if 0
      /* Delay */
      {
	int k;
	for (k=0;k<0xffffff;k++)
	  ;
      }
#endif

      oread += bread;

      //if (dump_to_stderr_and_quit)
	//exit(1);
    }

#if 0
  free (buf);
#endif

  printf ("%d bytes received\n", oread);
  fflush (stdout);

  daq_recv_shutdown (daq);
  return NULL;
}
