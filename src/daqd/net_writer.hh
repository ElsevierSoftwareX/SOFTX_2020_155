#ifndef NET_WRITER_HH
#define NET_WRITER_HH

#include "daqd.hh"

/// Request processing. New net writer is created for each
/// new data request.
class net_writer_c : public s_link {
 private:
  pthread_mutex_t bm;
  void lock (void) {pthread_mutex_lock (&bm);}
  void unlock (void) {pthread_mutex_unlock (&bm);}
  class locker;
  friend class net_writer_c::locker;
  class locker {
    net_writer_c *dp;
  public:
    locker (net_writer_c *objp) {(dp = objp) -> lock ();}
    ~locker () {dp -> unlock ();}
  };

  // This lock is used in the transient producer and consumer
  // to sync the access to the circular buffer with its destruction
  pthread_mutex_t tl;
  class transiency_locker;
  void transiency_lock (void) {pthread_mutex_lock (&tl);}
  void transiency_unlock (void) {pthread_mutex_unlock (&tl);}
  friend class net_writer_c::transiency_locker;
  class transiency_locker {
    net_writer_c *dp;
  public:
    transiency_locker (net_writer_c *objp) {(dp = objp) -> transiency_lock ();}
    ~transiency_locker () {dp -> transiency_unlock ();}
  };

public:

#if defined(NO_BROADCAST)
  static const int broadcast = 0;
#else
  typedef  struct {
    bool   inUse;
    char*  data;
  } radio_buffer;

  int broadcast;
  char *mcast_interface;
  diag::frameSend radio;
#if defined(DATA_CONCENTRATOR)
  // Broadcasting at 16Hz needs smaller buffers
  static const int radio_buf_len = 6 * 1024 * 1024;
  static const int radio_buf_num = 300;
  // Testpoint broadcaster, using different port
  diag::frameSend radio_tp;
#else
  static const int radio_buf_len = 64 * 1024 * 1024;
  static const int radio_buf_num = 20;
#endif
  radio_buffer radio_bufs [radio_buf_num];

  int* first_adc_ptr;
#endif

#if defined(USE_BROADCAST)  || defined(DATA_CONCENTRATOR)
  // concentrator is using next two ports for its broadcast
  static const int concentrator_broadcast_port = diag::frameXmitPort + 1;
  static const int concentrator_broadcast_port_tp = diag::frameXmitPort + 2;
#endif

#ifdef GDS_TESTPOINTS
  int clear_testpoints;
#endif

  // Scattered data vector type for data decimation
  struct dec_put_vec {
    unsigned long vec_idx;
    unsigned long vec_len;
    unsigned int vec_rate;
    unsigned int vec_bps;
  };

  enum {
    slow_writer = 0,
    frame_writer = 1,
    fast_writer = 2,
    name_writer = 3
  } writer_type;

  net_writer_c (int nid) : num_channels (0), shutdown_now (0), buffptr (0),
    block_size (0), transmission_block_size (0), transient (0),
    pvec_len (0), dec_vec_len (0), writer_type (slow_writer), seq_num (0),
    channels (0), pvec (0), dec_vec (0), s_link(nid)

#ifndef NO_BROADCAST
    ,broadcast (0), mcast_interface (0), radio (radio_buf_num-1)
#if defined(DATA_CONCENTRATOR)
    , radio_tp (radio_buf_num-1)
#endif
#endif
#ifdef GDS_TESTPOINTS
    ,clear_testpoints (0)
#endif
    , decimation_requested(false)
    , need_send_reconfig_block(false)
    , no_averaging(false)
    {
      offline.seconds_per_sample = 1;
      pthread_mutex_init (&bm, NULL);
      pthread_mutex_init (&tl, NULL);

#ifndef NO_BROADCAST
      for (int i = 0; i < radio_buf_num; i++) {
	radio_bufs [i].data = 0;
	radio_bufs [i].inUse = false;
      }
#endif
      for (int i = 0; i < 16; i++) pvec16th[i] = 0;
    };
  ~net_writer_c () {
    DEBUG1(cerr << "net writer deleted\n" << endl);
    pthread_mutex_destroy (&bm);
    pthread_mutex_destroy (&tl);
    free_vars();
  }

  //  pthread_mutex_t lock;

  int cnum; /* Consumer number */

  void free_vars() {
    for (int i = 0; i < 16; i++) {
	if (pvec16th[i]) free(pvec16th[i]);
	pvec16th[i] = 0;
    }
    if (channels) free(channels);
    channels = 0;
    if (pvec) free(pvec);
    pvec = 0;
    if (dec_vec) free(dec_vec);
    dec_vec = 0;
  }

  bool vars_fine() {
    bool r = true;
    for (int i = 0; i < 16; i++) {
	r &= (bool)(pvec16th[i]);
    }
    r &= (bool)channels;
    r &= (bool)pvec;
    return r & (bool)dec_vec;
  }

  bool alloc_vars(unsigned int maxchan) {
    channels = (long_channel_t *) malloc(sizeof(long_channel_t) * maxchan);
    pvec = (struct put_vec *) malloc(sizeof(struct put_vec) * maxchan * 2);
    dec_vec = (struct dec_put_vec *) malloc(sizeof(struct dec_put_vec) * maxchan);
    for (int i = 0; i < 16; i++) pvec16th[i] = (struct put_vec *) malloc(sizeof(struct put_vec) * maxchan * 2);
    if (!vars_fine()) {
	free_vars();
	return false;
    } else return true;
  }

  int num_channels; ///< Size of "channels" array below 
  long_channel_t *channels; ///< Data channels to send 

  int pvec_len; ///< Number of elements in `pvec'/`pvec16th'
// :IMPORTANT: max chan num times two
  struct put_vec *pvec; ///< Put vector prepared for `put_nowait_scattered()' operation in producer
// :IMPORTANT: max chan num times two
  struct put_vec *pvec16th [16]; ///< Put vector prepared for `put_nowait_scattered()' operation in the fast producer

  int dec_vec_len; ///< Number of elements in `dec_vec'
  struct dec_put_vec *dec_vec; ///< Scattered vector used for data decimation

  int block_size; ///< net-writer data block size (sum of the sizes of the configured channels)
  // size of the network transmission block (could be less than `block_size' if there are decimated channels
  int transmission_block_size;

  bool decimation_requested;

  short averaging (short *v, int num) {
    assert (num > 0 && num < SHRT_MAX);
    long res = 0;
    for (int i = 0; i < num; i++)
      res += v [i];

    return (short) (res / num);
  }

  int averaging (int *v, int num) {
    assert (num > 0 && num < SHRT_MAX);
    long long res = 0;
    for (int i = 0; i < num; i++)
      res += v [i];

    return (int) (res / num);
  }  

  float averaging (float *v, int num) {
    assert (num > 0 && num < SHRT_MAX);
    double res = 0;
    for (int i = 0; i < num; i++)
      res += v [i];

    return (float) (res / num);
  }

  double averaging (double *v, int num) {
    assert (num > 0 && num < SHRT_MAX);
    long double res = 0;
    for (int i = 0; i < num; i++)
      res += v [i];

    return (double) (res / num);
  }  


  circ_buffer *buffptr;
  circ_buffer *source_buffptr; ///< Data feed
  filesys_c *fsd; ///< Time to filename map to get data from files

  pthread_t producer_tid;
  pthread_t consumer_tid;

/*
 * Socket address, internet style.
struct sockaddr_in {
	short	sin_family;
	u_short	sin_port;
	struct	in_addr sin_addr;
	char	sin_zero[8];
};
*/

  struct sockaddr_in srvr_addr; ///< Data connection address
  int fileno; ///< Data connection socket file descriptor
  int connect_srvr_addr (int); ///< Connect to `srvr_addr' and set `fileno' socket
  int set_send_buf_size (int, int);
  int disconnect_srvr_addr () { 
    int res = 0;
    if (fileno != -1) {
      system_log(1, "connection closed on fd=%d", fileno);
      res = close (fileno);
      fileno = -1;
    }
    return res;
  }
  int send_reconfig_block(int *status_ptr, bool units_change, bool status_change);

  int start_net_writer (ostream *, int, int, circ_buffer *, filesys_c *, time_t, time_t, int);
  int kill_net_writer ();

  void *producer ();
  static void *producer_static (void *a) { return ((net_writer_c *)a) -> producer (); };
  void *consumer ();
  static void *consumer_static (void *a) {
    {locker mon ((net_writer_c *) a);}
    if (((net_writer_c *)a) -> send_client_header (DAQD_LENGTH_UNKNOWN)) // Send realtime data stream indicator
      return NULL;
    return ((net_writer_c *)a) -> consumer ();
  };

  void *transient_producer ();
  static void *transient_producer_static (void *a) { return ((net_writer_c *)a) -> transient_producer (); };
  void *transient_consumer ();
  static void *transient_consumer_static (void *a) { 
    {locker mon ((net_writer_c *) a);}
    return ((net_writer_c *)a) -> transient_consumer (); 
  };

  static void *frame_consumer_static (void *a) { 
    {locker mon ((net_writer_c *) a);}
    return ((net_writer_c *)a) -> send_frames (); 
  };

  void destroy (void) { 
    this -> ~net_writer_c ();
    free ((void *) this);
  };

  int transient; ///< Set if this producer doesn't send data online

  /*
    Data used by transient net-writer. This sort of net-writer sends off-line
    data to the client. This data is for certain period. Period is set by
    `start net-writer <gps> <delta> ... ' command or `start net-writer
    <delta>' command.

    In the following structure `gps' and `delta' store parameters of `start
    net-writer' command.

    `start_net_writer' starts network writer producer thread which takes some
    data out of the main circular buffer and puts it in the network writer
    circular buffer. This data is for some perioad of time. This period is
    indicated by `bstart' and `blast'. Namely, `bstart' is the timestamp of
    the first available data block and `blast' is the timestamp of the last.

    The consumer thread will use this data to find out where to get the data
    from: some of the data will be coming from the frame files and some from
    the network writer circular buffer.
  */
  struct {
    time_t gps;
    time_t delta;
    time_t bstart;
    time_t blast;
    time_t seconds_per_sample; // set to the period of one minute trend sample for the minute trend; set to one otherwise
    int no_time_check; // set when time check should be bypassed, i.e. unknown channel names present in the request
  } offline;

  int seq_num;

  bool need_send_reconfig_block;

  /// Is set by 'downsample' parameter to 'start net-writer' command
  bool no_averaging;

  /// Send some data to the client
  int send_to_client (char *data, unsigned long len, unsigned long gps = 0, int period = 0, int tp = 0);


  /// Send ACK to the client + net-writer ID
  int send_positive_response () {
    int res;
    char sbuf [9];

// :TODO: must fix the net-writer ID
    sprintf (sbuf, "%08x", (unsigned int) 0);
    if (! (res = send_to_client (S_DAQD_OK, 4)))
      return send_to_client (sbuf, 8);
    return res;
  }

  /// Send to the client transmission header (indicator of how many data blocks wil follow)
  int send_client_header (unsigned int blocks) {
    //    blocks = htonl (blocks);
    blocks = htonl (!!blocks); // 1 for off-line; 0 for online
    return send_to_client ((char *) &blocks, sizeof (unsigned int));
  }

  /// The time period is given by `offline.gps' and `offline.delta'
  int send_files (void);
  int send_raw_files (void);
  void *send_frames (void);

  /// Send data transmission trailer
  int send_trailer () {
    unsigned int header [5];
    
    // size of the transmission block (zero) minus size of this length word
    header [0] = htonl (4 * sizeof (unsigned int));
    header [1] = htonl (0);
    header [2] = htonl (0);
    header [3] = htonl (0);
    header [4] = htonl (0);

    if (send_to_client ((char *) &header, 5 * sizeof (unsigned int)))
      return -1;

    DEBUG1(cerr << "net writer trailer sent" << endl);

    return 0;
  }

  /// Determine IP address given socket file descriptor
  /// Returns IP on success (similar to inet_addr(3N)), `-1' on failure
  static int ip_fd (int fd)
    {
      struct sockaddr_in paddr;
      socklen_t paddr_len = sizeof (paddr);

      if (getpeername (fd, (struct sockaddr *) &paddr, &paddr_len) < 0)
	return -1;

      return (int) paddr.sin_addr.s_addr;
    }

  static struct in_addr ip_fd_in_addr (int fd)
    {
      struct in_addr ia;
      ia.s_addr = ip_fd (fd);
      return ia;
    }

  static char *ip_fd_ntoa (int fd, char *buf) {
    char *istr = net_writer_c::inet_ntoa (ip_fd_in_addr (fd), buf);
    //    istr = ((int) istr) != 0? istr: "unknown";
    return istr;
  }

  /// This is here to substitute unrealiable system's inet_ntoa() function
  /// There were memory leaks detected inside of it by Purify
  static char *inet_ntoa (struct in_addr in, char *buf) {
    unsigned char bt [4];
    memcpy (bt, (const void *) &in.s_addr, 4);
    sprintf (buf, "%u.%u.%u.%u", bt[0], bt[1], bt[2], bt[3]);
    return buf;
  }

  /// Match `str' against the regular expression to see if
  /// this is a valid IP address. If not try to resolv `str' into the IP address.
  static int get_inet_addr (char *str)
    {
      // See if the string is IP address
      if (! daqd_c::is_valid_ip_address (str)) {
	int error;
	struct hostent *hp;
	int ret;
	struct hostent hent;
	char buf [2048];
	
	// Try to resolve name into IP address
#if 0
       int gethostbyname_r(const char *name,
         struct hostent *ret, char *buf, size_t buflen,
         struct hostent **result, int *h_errnop);
#endif

	if (gethostbyname_r (str, &hent, buf, 2048, &hp, &error)) 
	  return -1;

	(void) memcpy(&ret, *hent.h_addr_list, sizeof (ret));
	return ret;
      } else
	return inet_addr (str);
    }


  /// Get IP address from the string in format `127.0.0.1:9090'
  /// Returns IP (value of inet_addr(3N)) on success, `-1' if failed.
  /// Fails if there is no IP address in the `str' or if `inet_addr()' failed.
  /// Data in the string `str' must not be in the static storage.
  static int ip_str (char *str)
    {
      char *nc = str - 1;

      // `:' separates IP address from the port number
      int res;
      if (nc = strchr (str, ':'))
	{
	  *nc = 0; // Substitute `:' on `\000' for an instant, shouldn't be a problem
	  res = get_inet_addr (str);
	  *nc = ':';
	}
      else // Consider whole string an IP address if there is no `:' in it
	res = get_inet_addr (str);

      return res;
    }

  /// Get port number from the string in the format `127.0.0.1:9090'
  static int port_str (char *str)
    {
      char *nc;
      if (nc = strchr (str, ':'))
	return htons (atoi (nc + 1));
      return htons (atoi (str));
    }

 private:
  int shutdown_now;
  void shutdown_net_writer ();
  void shutdown_buffer () {
    circ_buffer *oldb;

    oldb = buffptr;
    buffptr = 0;
    oldb -> ~circ_buffer ();
    free ((void *) oldb);
    shutdown_now = 0;
  }
}; // class net_writer

#endif
