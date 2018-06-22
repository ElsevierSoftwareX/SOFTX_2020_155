#ifndef TREND_HH
#define TREND_HH

#include "daqd.hh"
#include "profiler.hh"
#include "stats/stats.hh"

/// Trend circular buffer block structure 
/// Do not put any shorts in this structure, because compiler puts holes in to align the double

//// This structure is 40 byte long on 32-bit architecture
//// and 48 byte long on 64-bit due to compiler alignment dillidallying
typedef struct {
  union {int I; double D; float F; unsigned int U;} min;
  union {int I; double D; float F; unsigned int U;} max;
  int n; /// the number of valid points used in calculating min, max, rms and mean
  double rms;
  double mean;
} trend_block_t;

/// This one should be 40 bytes long on any architecture
/// do not assign double into it, memcpy them
typedef struct  trend_block_on_disk_t {
/// These two ints represent a double
  unsigned int min;
  unsigned int min2;
/// These two ints represent a double
  unsigned int max;
  unsigned int max2;
  unsigned int n;
/// These two ints represent a double
  unsigned int rms;
  unsigned int rms2;
/// These two ints represent a double
  unsigned int mean;
  unsigned int mean2;

/// Assign with the in-memory trend structure
  void operator=(const trend_block_t& t) {
	memcpy(&min, &t.min, 2*sizeof(unsigned int));
	memcpy(&max, &t.max, 2*sizeof(unsigned int));
	n = t.n;
	memcpy(&rms, &t.rms, 2*sizeof(unsigned int));
	memcpy(&mean, &t.mean, 2*sizeof(unsigned int));
  }
} trend_block_on_disk_t;


typedef struct raw_trend_record_struct {
  unsigned int gps;
  trend_block_on_disk_t tb;
} raw_trend_record_struct;


/// Trend frame calculation thread 
class trender_c {
public:
  enum {
    max_trend_channels = MAX_TREND_CHANNELS,
    num_trend_suffixes = 5, /// That many trend channels are created for each input channel
    max_trend_sufx_len = 5,
  };

  ~trender_c () {
    pthread_cond_destroy (&worker_done);
    pthread_cond_destroy (&worker_notempty);
    pthread_mutex_destroy (&worker_lock);
    pthread_mutex_destroy (&lock);
    pthread_mutex_destroy (&frame_write_lock);
    sem_destroy (&trender_sem);
    sem_destroy (&frame_saver_sem);
    sem_destroy (&minute_frame_saver_sem);
  }

  trender_c () : shutdown_now (0),
    shutdown_minute_now (0),
    minute_trend_buffer_blocks (60),
    mtb (0),
    mt_stats(), mt_file_stats(),
    tb (0),
    num_channels (0), num_trend_channels (0), block_size (0), ascii_output (0),
    frames_per_file (1), trend_buffer_blocks (60), profile ((char *)"trend"),
    		 profile_mt((char *)"mt"),
		 fsd (60), minute_fsd (3600), raw_minute_trend_saving_period (2),
		 worker_first_channel (0),
		 trend_worker_nb (0), worker_busy (0), _configuration_number(0)
    {
      sem_init (&minute_frame_saver_sem, 0, 1);
      sem_init (&frame_saver_sem, 0, 1);
      sem_init (&trender_sem, 0, 1);
      pthread_mutex_init (&lock, NULL);
      pthread_mutex_init (&worker_lock, NULL);
      pthread_cond_init (&worker_notempty, NULL);
      pthread_cond_init (&worker_done, NULL);
      pthread_mutex_init (&frame_write_lock, NULL);
      trend_block_t tbv;
#define TB_OFFS(a) (((char *) &tbv.a) - ((char *) &tbv))
      struct sfxs {
	char *name;
	int short_bps;
	int float_bps;
	int double_bps;
	int tsize; /// size of the trend var in the trend block
	int toffs;
      } sfxs [num_trend_suffixes]
	= {
	  {(char *)".min", sizeof (int), sizeof (float), sizeof (double), sizeof (double), TB_OFFS(min)},
	  {(char *)".max", sizeof (int), sizeof (float), sizeof (double), sizeof (double), TB_OFFS(max)},
	  {(char *)".n", sizeof (int), sizeof (int), sizeof (int), sizeof (double), TB_OFFS(n)},
	  {(char *)".rms", sizeof (double), sizeof (double), sizeof (double), sizeof (double), TB_OFFS(rms)},
	  {(char *)".mean", sizeof (double), sizeof (double), sizeof (double), sizeof (double), TB_OFFS(mean)},
	};
#undef TB_OFFS
      for (int i = 0; i < num_trend_suffixes; i++) {
	strcpy (sufxs [i], sfxs [i].name);
	bps [_16bit_integer][i] = sfxs [i].short_bps;
	bps [_32bit_float][i] = sfxs [i].float_bps;
	bps [_64bit_double][i] = sfxs [i].double_bps;
	tsize [i] = sfxs [i].tsize;
	toffs [i] = sfxs [i].toffs;
      }
    }

  pthread_mutex_t lock;
  circ_buffer *tb;  ///< trend circular buffer object
  circ_buffer *mtb;  ///< minute trend circular buffer object
  class stats mt_stats; ///< minute trend period stats
  class stats mt_file_stats; ///< minute trend file saving stats
  pthread_t tsaver; ///< This thread saves trend data into the `fname'
  pthread_t mtsaver; ///< This thread saves minute trend data into the `fname'
  pthread_t mtraw; ///< This thread saves minute trend data to raw minute trend files
  pthread_t consumer; ///< Thread reads data from the main circular buffer, calculates trend and puts it into `tb' 
  pthread_t mconsumer; ///< Thread reads data from the trend circular buffer, `tb', calculates minute trend and puts it into `mtb' 
  int ascii_output; ///< If set, no frame files, just plain ascii trend file is created
  ofstream *fout;
  int cnum; ///< trend consumer `consumer' thread consumer number in main cb 
  int mcnum; ///< minute trend consumer `mconsumer' consumer number in `tb' 
  int saver_cnum; ///< Saver consumer number (in the trend circular buffer `tb')
  int msaver_cnum; ///< Minute trend saver consumer number (in the minute trend circular buffer, `mtb')
  int raw_msaver_cnum; ///< Raw minute trend saver consumer number (in the minute trend cir cular buffer, `mtb')

  /* Trend calculation params */
  int frames_per_file; ///< Determines when framer ought to create new file 
  int minute_frames_per_file; ///< Determines when minute framer ought to create new file 
  int trend_buffer_blocks; ///< Size of trend circ buffer. Also the size of the trend frame in seconds 
  int minute_trend_buffer_blocks; ///< Size of minute trend circ buffer. Also the size of the minute trend frame in minutes 

  char sufxs [num_trend_suffixes][max_trend_sufx_len + 1]; ///< Added to input channel name, giving `input_channel.min'
  int bps [MAX_DATA_TYPE + 1][num_trend_suffixes]; ///< Bytes per second for each data type for each trend type
  int tsize [num_trend_suffixes]; ///< sizeof each trend type variable in the trend block
  int toffs [num_trend_suffixes]; ///< offset to each trend type variable in the trend bloc
  daq_data_t dtypes [num_trend_suffixes]; ///< data type of each trend variable

  /// Input channels
  int num_channels;
  channel_t channels [max_trend_channels];  ///< Channels for which trend is calculated

  /// Output or trend channels
  int num_trend_channels;
  channel_t trend_channels [max_trend_channels * num_trend_suffixes];  ///< Channels for which trend is calculated

  filesys_c fsd; ///< Trend frames filesystem map
  filesys_c minute_fsd; ///< Minute trend frames filesystem map
  raw_filesys_c raw_minute_fsd; ///< Raw minute trend filesystem map

  int block_size; ///< circ buffer data block size (sum of the sizes of the configured channels)

  int start_trend (ostream *, int, int, int, int);
  sem_t trender_sem;
  int start_trend_saver (ostream *);
  int start_minute_trend_saver (ostream *);
  int start_raw_minute_trend_saver (ostream *);
  sem_t frame_saver_sem;
  sem_t minute_frame_saver_sem;

  int kill_trend (ostream *);

  inline void trend_loop_func (int, int*, char*);
  void *trend ();
  static void *trend_static (void *a) { return ((trender_c *)a) -> trend (); };
  void *trend_worker ();
  static void *trend_worker_static (void *a) { return ((trender_c *)a) -> trend_worker (); };
  void *saver ();
  static void *saver_static (void *a) { return ((trender_c *)a) -> saver (); };
  void *framer ();
  static void *trend_framer_static (void *a) { return ((trender_c *)a) -> framer (); };

  void *minute_trend ();
  static void *minute_trend_static (void *a) { return ((trender_c *)a) -> minute_trend (); };
  void *minute_framer ();
  void *raw_minute_saver ();
  static void *minute_trend_framer_static (void *a) { return ((trender_c *)a) -> minute_framer (); };
  static void *raw_minute_trend_saver_static (void *a) { return ((trender_c *)a) -> raw_minute_saver (); };

  profile_c profile; ///< profile on trend circular buffer.
  profile_c profile_mt; ///< profile on minute trend circular buffer.
  unsigned int raw_minute_trend_saving_period;

  /// worked thread does processing from this channel until the last one
  unsigned int worker_first_channel;

  /// next block number for trend worker
  /// trender sets this before signaling to worker
  int trend_worker_nb;

  /// worker thread mutex and condition var
  /// trender signals on cond var when new data is ready to be processed
  pthread_mutex_t worker_lock;
  pthread_cond_t worker_notempty;

  /// worker signals to trender on cond var when it is done processing
  pthread_cond_t worker_done;
  unsigned int worker_busy; ///< worker clears it when it is finished

  pthread_t worker_tid;

  /// worker will start processing channels in the direction from the end to the start
  /// trender will do from the start to the end.
  /// They will meet at some point, when next_trender_block >= next_worker_block.
  unsigned int next_trender_block;
  unsigned int next_worker_block;

  /// Main trender local buffer area
  trend_block_t ttb [max_trend_channels];

  /// prevent two trend frames from being written at the same time
  pthread_mutex_t frame_write_lock;

  void set_configuration_number(int value) {
    _configuration_number = value;
  }
  int configuration_number() const { return _configuration_number; }
private:
  int _configuration_number;
  int shutdown_now;
  int shutdown_minute_now;
  void shutdown_trender () { shutdown_now = 1; }
  void shutdown_minute_trender () { shutdown_minute_now = 1; }
  /// kill the buffer and close the socket
  void shutdown_buffer () {
    circ_buffer *oldb;

    oldb = tb;
    tb = 0;
    oldb -> ~circ_buffer();
    free ((void *) oldb);

    oldb = mtb;
    mtb = 0;
    oldb -> ~circ_buffer();
    free ((void *) oldb);

    shutdown_now = 0;
  }
}; // class trender_c


#endif
