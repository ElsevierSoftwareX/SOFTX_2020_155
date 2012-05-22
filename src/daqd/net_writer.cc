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
#include <sys/mman.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <limits.h>
#ifdef sun
#include <sys/priocntl.h>
#include <sys/rtpriocntl.h>
#include <sys/lwp.h>
#endif

#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>

using namespace std;

#include <functional>
#include <algorithm>
#include <list>
#include <set>
#include <map>
#include <string>

#include "circ.hh"
#include "channel.hh"
#include "daqc.h"
#include "daqd.hh"
#include "sing_list.hh"
#include "net_writer.hh"

extern daqd_c daqd;
extern pthread_mutex_t framelib_lock;

#ifdef USE_FRAMECPP
#if FRAMECPP_DATAFORMAT_VERSION <= 4
class myImageFrameWriter : public FrameCPP::ImageFrameWriter {
public:
	myImageFrameWriter(std::strstream &s): FrameCPP::ImageFrameWriter(s) {}
	void writeTOC() {
		Output::mClosed = false;
		(const_cast<std::vector<INT_8U>&>(getPositionEOF())).clear();
		// Write table of contents
		TOC::write ( *this );
	}
};
#else
#include "myimageframewriter.hh"
#endif
#endif


/*
  Off-line frame net-writer thread.
  Sends whole frame files right now.
  Assumes one second long files.
*/
void *
net_writer_c::send_frames (void)
{
#ifdef USE_FRAMECPP
  int err_flag = 0;
  time_t gps = offline.gps;
  time_t barrier = offline.gps + offline.delta;
  
  if (! send_client_header (1))
    {
      transmission_block_size = 0; // Set this to the frame file length

      unsigned int header [5];
      // size of the transmission block minus size of this length word
      header [0] = htonl (4 * sizeof (unsigned int) + transmission_block_size);
      // number of seconds of data
      header [1] = htonl (1);

      for (time_t i = gps; i < barrier; i++) { // For every second required
	time_t file_first_second;
	time_t file_length_seconds;
	time_t file_second_after_the_last;
	char tmpf [filesys_c::filename_max + 10];

	// Determine filename and for what time there is data in the file
	if (fsd -> filename (i, tmpf, &file_first_second, &file_length_seconds) < 0)
	  {
#ifdef not_def
	    system_log(1, "net_writer_c::send_frames(): Couldn't map GPS time %d onto the file name", i);
	    if (send_zero_block (i, 0))
	      break;
#endif
	    continue;
	  }
	file_second_after_the_last = file_first_second + file_length_seconds;

	int fildes = open (tmpf, O_RDONLY);
	if (fildes < 0) {
	  system_log(2, "net_writer_c::send_frames(): can't open `%s'", tmpf);

#ifdef not_def
	  if (send_zero_block (i, 0))
	    break;
#endif

	  continue;
	}

	// Need to stat file to get its length
	struct stat st;
	if (fstat (fildes, &st) < 0) {
	  close (fildes);
	  system_log(2, "net_writer_c::send_frames(): can't fstat `%s'", tmpf);

#ifdef not_def
	  if (send_zero_block (i, 0))
	    break;
#endif

	  continue;
	}
	DEBUG1(cerr << "net_writer::send_frames(): opened `" << tmpf << "'" << endl);

	transmission_block_size = st.st_size;
	header [0] = htonl (4 * sizeof (unsigned int) + transmission_block_size);

	// send block length and the header
	header [2] = htonl (0);
	header [3] = htonl (0);
	header [4] = htonl (seq_num++);

	// Send header
	if (! send_to_client ((char *) &header, 5 * sizeof (unsigned int))) {

	  // Memory map the file
	  char *bptr = (char *) mmap ((caddr_t) 0, transmission_block_size, (PROT_READ | PROT_WRITE),
				      MAP_PRIVATE, fildes, 0);

	  if (bptr == MAP_FAILED) {
	    system_log(2, "net_writer_c::send_frames(): couldn't mmap `%s'", tmpf);

#ifdef not_def
	    send_zero_block (i, 0);
#endif

	    err_flag = 1;
	  } else {
	    // Send frame data from the memory mapped frame file
	    if (send_to_client (bptr, transmission_block_size))
	      err_flag = 1;
	    // Unmap mapped frame file
	    munmap (bptr, transmission_block_size);
	  }
	} else {
	  err_flag = 1;
	}

	close(fildes);

#ifndef __linux__
	yield ();
#endif

	if (err_flag)
	  break;
      } // end for every second required

      (void) send_trailer ();
    }
  disconnect_srvr_addr ();

  s_link *nwres = daqd.net_writers.remove (this);
  assert (nwres);

#endif
  return 0;
}



// Process request using the NDS server.
// NDS listens on the UNIX domain socket (pipe).
// returns 0 on success
int
net_writer_c::send_files (void)
{
  static pthread_mutex_t bm = PTHREAD_MUTEX_INITIALIZER;
  static unsigned long last_job_number = 0;
  unsigned long job_num;

  // unique job number is generated here (conflicts are possible on restart)
  pthread_mutex_lock (&bm);
  {
    time_t t = time(0);
    if (t <= last_job_number) t = last_job_number + 1;
    job_num = last_job_number = t;
  }
  pthread_mutex_unlock (&bm);
  char jobn_buf[256];
  sprintf(jobn_buf,"%d",job_num);
  string spec_filename = daqd.nds_jobs_dir + "/jobs/" + jobn_buf;
  string pipe =  daqd.nds_jobs_dir + "/pipe";   // unix domain socket communication endpoint

  // connect UNIX socket on pipe
  int socketfd;
  struct sockaddr_un servaddr;
  
  if (sizeof(servaddr.sun_path)-1 < pipe.size())
    {
      system_log(1, "pipe filename `%s' is too long; maximum size is %d", pipe.c_str(), sizeof(servaddr.sun_path)-1);
      return 1;
    }

  if ((socketfd = socket (AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
      system_log(1, "UNIX socket(); errno=%d\n", errno);
      return 1;
    }
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sun_family = AF_UNIX;
  strcpy(servaddr.sun_path, pipe.c_str());

  if (connect (socketfd, (struct sockaddr *) &servaddr, sizeof (servaddr)) < 0)
    {
      system_log(1, "UNIX connect(); errno=%d\n", errno);
      close(socketfd);
      return 1;
    }

  // Create job specification file
  {

    ofstream out(spec_filename.c_str());
    if (! out ) {
      close(socketfd);
      system_log(1, "%s: NDS job spec file open failed", spec_filename.c_str());
      return 1;
    }

    try {
      out << "# NDS job specification file" << endl;
      if (source_buffptr == daqd.trender.mtb) {
	out << "datatype=rawminutetrend" << endl;
	out << "archive_dir=" << daqd.trender.raw_minute_fsd.get_path() << endl;
      } else  if (source_buffptr == daqd.trender.tb) {
	out << "datatype=secondtrend" << endl;
	if (daqd.trender.fsd.gps_time_dirs)
	  out << "archive_dir=" << daqd.trender.fsd.get_path() << "/" << endl;
	else
	  out << "archive_dir=" << daqd.trender.fsd.get_path() << endl;
	out << "archive_prefix=" << daqd.trender.fsd.get_prefix() << endl;
	out << "archive_suffix=" << daqd.trender.fsd.get_suffix() << endl;
	if (daqd.trender.fsd.gps_time_dirs == 0) {
	  out << "times=";
	  daqd.trender.fsd.print_times(out);
	  out << endl;
        }// else out << "gps_time_dirs=1" << endl;
      } else {
	out << "datatype=full" << endl;
	if (no_averaging) 
	  out << "decimate=nofilter" << endl;
	else
	  out << "decimate=average" << endl;
	
	if (daqd.fsd.gps_time_dirs)
	  out << "archive_dir=" << daqd.fsd.get_path() << "/" << endl;
	else
	  out << "archive_dir=" << daqd.fsd.get_path() << endl;
	out << "archive_prefix=" << daqd.fsd.get_prefix() << endl;
	out << "archive_suffix=" << daqd.fsd.get_suffix() << endl;
	if (daqd.fsd.gps_time_dirs == 0) {
	  out << "times=";
	  daqd.fsd.print_times(out);
	  out << endl;
        } // else out << "gps_time_dirs=1" << endl;

	out << "rates=";
	for (int i = 0; i < num_channels; i++)
	  out << (channels [i].req_rate? channels [i].req_rate: channels [i].sample_rate) << " ";
	out << endl;
      }
      out << "startgps=" << offline.gps << endl;
      out << "endgps=" << offline.gps + offline.delta -1 << endl;
      out << "signals=";
      for (int i = 0; i < num_channels; i++)
	out << channels[i].name << " ";
      out << endl;
      if (source_buffptr == daqd.trender.mtb) {
	// Write out current conversion values into the job config file
	// NDS will send them (if it decides to do so) in the reconfig block
	out << "signal_offsets=";
	for (int i = 0; i < num_channels; i++)
	  out << channels[i].signal_offset << " ";
	out << endl << "signal_slopes=";
	for (int i = 0; i < num_channels; i++)
	  out << channels[i].signal_slope << " ";
	out << endl;
      }
      out << "types=";
      for (int i = 0; i < num_channels; i++) {
	switch (channels[i].data_type) {
	case _16bit_integer:
	  out << "_16bit_integer";
	  break;
	case _32bit_integer:
	  out << "_32bit_integer";
	  break;
	case _64bit_integer:
	  out << "_64bit_integer";
	  break;
	case _32bit_float:
	  out << "_32bit_float";
	  break;
	case _64bit_double:
	  out << "_64bit_double";
	  break;
	case _32bit_complex:
	  out << "_32bit_complex";
	  break;
	default:
	  out << "unknown";
	  break;
	}
	out << " ";
      }
      out << endl;

      // Add extra info to the config file when there are any archive channel names present
      // in the request
      if (offline.no_time_check) {
	unsigned int naptrs = 0;
	archive_c *aptr[daqd.archive.count()];
	out << "added_flags=";
	for (int i = 0; i < num_channels; i++) {
	  if (channels [i].group_num != channel_t::arc_groupn) out << "0 ";
	  else {
	    archive_c *a = (archive_c *) channels [i].id;
	    a -> lock();
	    out <<  a -> fsd.get_path () << " ";

	    // Do not add duplicates to the list
	    int j = 0;
	    for (; j < naptrs; j++)
	      if (aptr[j] == a)
		break;
	    if (j == naptrs) aptr [naptrs++] = a;
	    a -> unlock();
	  }
	}
	out << endl;

	for(int i = 0; i < naptrs; i++) {
	  aptr[i] -> lock();
	  out << endl << "[" << aptr[i] -> fsd.get_path() << "]" << endl;
	  out <<"added_prefix=" << aptr[i] -> fsd.get_prefix() << endl;
	  out <<"added_suffix=" << aptr[i] -> fsd.get_suffix() << endl;
	  out << "added_times=";
	  aptr[i] -> fsd.print_times(out);
	  out << endl;
	  aptr[i] -> unlock();
	}
      }
      out.close();
   } catch (...) {
      system_log(1, "%s: output error", spec_filename.c_str());
      close(socketfd);
      return 1;
    }
  }

#if 0
  // FIXME: temporary solution
  // transmit current calibration values if this is a request for minute trend data
  //
  if (source_buffptr == daqd.trender.mtb) {
    int written;
    unsigned long rh [5];
    rh [0] = htonl (4 * sizeof (unsigned long)
		    + num_channels*(2*sizeof(float)+sizeof(int)));
    rh [1] = rh [2] = rh [3] = rh [4] = htonl(0xffffffff);

    written = basic_io::writen(fileno, rh, sizeof(rh));
    if (written != sizeof(rh)) {
      close(socketfd);
      system_log(1, "failed to send reconfig header; errno=%d", errno);
      return 1;
    }
    for (int k = 0; k < num_channels; k++) {
      written = basic_io::writen(fileno, (char *) &channels[k].signal_offset, sizeof(float));
      if (written != sizeof(float)) {
	close(socketfd);
	system_log(1, "failed to send reconfig data; errno=%d", errno);
	return 1;
      }
      written = basic_io::writen(fileno, (char *) &channels[k].signal_slope, sizeof(float));
      if (written != sizeof(float)) {
	close(socketfd);
	system_log(1, "failed to send reconfig data; errno=%d", errno);
	return 1;
      }
      unsigned int status = 0;
      written = basic_io::writen(fileno, (char *) &status, sizeof(int));
      if (written != sizeof(int)) {
	close(socketfd);
	system_log(1, "failed to send reconfig data; errno=%d", errno);
	return false;
      }
    }
  }
#endif
  
  // send data connection fd to the NDS
  struct msghdr msg;
  struct iovec iov[1];


#if  defined(__linux__) || defined(_XPG4_2)
#define HAVE_MSGHDR_MSG_CONTROL

#ifndef CMSG_SPACE
#define CMSG_SPACE(size) (sizeof(struct msghdr) + (size))
#endif
#ifndef CMSG_LEN
#define CMSG_LEN(size) (sizeof(struct msghdr) + (size))
#endif
#endif

#ifdef HAVE_MSGHDR_MSG_CONTROL
  union {
    struct cmsghdr msg;
    char control[CMSG_SPACE(sizeof(int))];
  } control_un;
  struct cmsghdr *cmptr;

  msg.msg_control = control_un.control;
  msg.msg_controllen = sizeof(control_un.control);

  cmptr = CMSG_FIRSTHDR(&msg);
  cmptr->cmsg_len = CMSG_LEN(sizeof(int));
  cmptr->cmsg_level = SOL_SOCKET;
  cmptr->cmsg_type = SCM_RIGHTS;
  *((int *) CMSG_DATA(cmptr)) = fileno;
#else
  msg.msg_accrights = (caddr_t) &fileno;
  msg.msg_accrightslen = sizeof(int);
#endif

  msg.msg_name = NULL;
  msg.msg_namelen = 0;

#if __GNUC__ >= 3
  iov[0].iov_base = (void *)"";
#else
#ifdef __linux__
  iov[0].iov_base = (void *)"";
#else
  iov[0].iov_base = "";
#endif
#endif
  iov[0].iov_len = 1;
  msg.msg_iov = iov;
  msg.msg_iovlen = 1;

  errno = 0;
  int res = sendmsg(socketfd, &msg, 0);
  if (res != 1) {
    system_log(1,"senmsg() to NDS failed; res=%s; errno=%d", res, errno);
    close(socketfd);
    return 1;
  } else {
    DEBUG(5,cerr << "file descriptor sent; result was " << res << endl);
  }

  // send job spec file name
  char a = strlen(spec_filename.c_str()) + 1;
  if (1 != write(socketfd,&a,1)) {
    system_log(1,"write(1) to NDS failed; errno=%d", errno);
    close(socketfd);
    return 1;
  }

  if (write(socketfd, spec_filename.c_str(),(int)a) != (int)a) {
    system_log(1,"write(jobn) to NDS failed; errno=%d", errno);
    close(socketfd);
    return 1;
  }

  // At this points processing and data transmission to the client program
  // is running in the NDS server

  // get seq_num back from the NDS
  // This call blocks waiting for NDS to complete processing and
  // data transmission to the client
  if (read(socketfd, &seq_num, sizeof(int)) != sizeof(int)) {
    system_log(1,"read(seq_num) from NDS failed; errno=%d", errno);
    close(socketfd);
    return 1;
  }

  close (socketfd);
  return 0;
}


/*
  Online net-writer producer.

  There are two parts, divided by writer type: first part works for fast
  writers (working at 16 Hz); second part works for slow writers (1Hz
  writers).

  There are two parts in slow writer type part, one that uses `put_nowait()'
  to put all the channels from the source circular buffer and the other that
  uses `put_nowait_scattered()' call to put a selection of channels of the
  source circular buffer. Both put data into the net-writer circular buffer.
*/

void *
net_writer_c::producer ()
{
  int nb;

#ifdef VMICRFM_PRODUCER
  // Only if this thread works on main circular buffer
  if (buffptr == daqd.b1) {
    // Put this thread into the realtime scheduling class with half the priority
    daqd_c::realtime ("net_writer transient producer", 2);
  }
#endif

  if (writer_type == fast_writer) {
    int first_time = 1;
    DEBUG(5, cerr << "in fast producer...");
    for (;;)
      {
	assert (buffptr);

	if (shutdown_now)
	  {
	    //	  pthread_join (consumer_tid, NULL);

	    source_buffptr -> delete_consumer (cnum);
	    shutdown_buffer ();
	    s_link *res = daqd.net_writers.remove (this);
	    assert (res);
	    return NULL;
	  }

	for (;;)
	  {
	    nb = source_buffptr -> get16th (cnum);
	    int nb16th = nb & 0xf;
	    nb >>= 4;

	    if (first_time) { // Skip to a second's boundary
	      if (! nb16th)
		first_time = 0;
	      else if (nb16th == 0xf)
		break;
	      else {
		DEBUG(5, cerr << "fast producer skipped adjusting to a second's boundary");
		continue;
	      }
	    }
	    circ_buffer_block_prop_t &prop = source_buffptr -> block_prop (nb) -> prop16th [nb16th];
#ifdef not_def
	    prop.gps = source_buffptr -> block_prop (nb) -> prop16th [nb16th].gps;
	    prop.gps_n = source_buffptr -> block_prop (nb) -> prop16th [nb16th].gps_n;
#endif

	    DEBUG(5, cerr << "GPS time in fast producer: " << prop.gps << ":" << prop.gps_n << endl);

	    (void) buffptr -> put_nowait_scattered ((char *) source_buffptr -> block_ptr (nb),
						    pvec16th [nb16th], pvec_len, &prop);

	    //	cerr << "net_writer_c::producer(): put() of " << source_buffptr -> block_prop (nb) -> bytes << " bytes" << endl;
	    if (nb16th == 0xf)
	      break;
	  }
	source_buffptr -> unlock16th (cnum);
      }
  } else {
    for (;;)
      {
	assert (buffptr);

	if (shutdown_now)
	  {
	    //	  pthread_join (consumer_tid, NULL);

	    source_buffptr -> delete_consumer (cnum);
	    shutdown_buffer ();
	    s_link *res = daqd.net_writers.remove (this);
	    assert (res);
	    return NULL;
	  }
	
	nb = source_buffptr -> get (cnum);
	{
	  circ_buffer_block_prop_t &prop = source_buffptr -> block_prop (nb) -> prop;
#ifdef not_def
	  prop.gps = source_buffptr -> block_prop (nb) -> prop.gps;
	  prop.gps_n = source_buffptr -> block_prop (nb) -> prop.gps_n;
#endif
	  int nbi = buffptr -> put_nowait_scattered ((char *) source_buffptr -> block_ptr (nb),
						     pvec, pvec_len, &prop);

	  //	cerr << "net_writer_c::producer(): put() of " << source_buffptr -> block_prop (nb) -> bytes << " bytes" << endl;
	}
	source_buffptr -> unlock (cnum);
      }
  } /* ! fast writer */
  return NULL;
}



/*
  Send reconfiguration block to client

  `status_ptr' points to the first status word. 

  Returns 0 if OK; -1 if failed
 */
int
net_writer_c::send_reconfig_block(int *status_ptr, bool units_change, bool status_change) {
  unsigned int header [5];   
  header [0] = htonl (4 * sizeof (unsigned int)
		      + num_channels*(2*sizeof(float)+sizeof(int)));
  header [1] = header [2] = header [4] = htonl(0xffffffff);
  header [3] = 0;
  if (units_change)
    header [3] |= 1;
  if (status_change)
    header [3] |= 2;

  header [3] = htonl(header [3]);

  if (send_to_client ((char *) &header, 5 * sizeof (unsigned int)))
    return -1;
  for (int i = 0; i < num_channels; i++) {
    if (send_to_client ((char *) &channels[i].signal_offset, sizeof (float)))
      return -1;
    if (send_to_client ((char *) &channels[i].signal_slope, sizeof (float)))
      return -1;
    if (send_to_client ((char *) (status_ptr + i), sizeof (int)))
      return -1;
  }
  return 0;
}

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



/*
  Online net-writer consumer.

  Since `producer()' handles channel selection, the job of this consumer
  is simple: just get the data out of the net-writer circular buffer and
  send it to the client.

*/
void *
net_writer_c::consumer ()
{
#ifdef not_def
  unsigned int drops = 0;
#endif
  int nb;
  int num_sent = 0;
  unsigned int header [5];

  // size of the transmission block minus size of this length word
  header [0] = htonl (4 * sizeof (unsigned int) + transmission_block_size);

  // number of seconds of data
  header [1] = htonl (buffptr -> block_period ());

  if (decimation_requested) {
    DEBUG1(cerr << "transmission_block_size=" << transmission_block_size << endl);    
    int channel_status [num_channels];
    do {
      int bytes;

      nb = buffptr -> get (0);
      if (! (bytes = buffptr -> block_prop (nb) -> bytes))
	break;

      if (need_send_reconfig_block) {
        // Always send reconfiguration block in the beginning
        // See if status changed and send reconfig block if it did
        int *status_ptr = (int *)(buffptr -> block_ptr (nb) + buffptr -> block_prop (nb) -> bytes)
	  - num_channels;
        if (! num_sent || memcmp (channel_status, status_ptr, num_channels * sizeof(int))) {
	    if (send_reconfig_block (status_ptr, true, true))
	      return NULL;
	    memcpy (channel_status, status_ptr, num_channels * sizeof(int));
        }
     }

      // send block length and the header
      header [2] = htonl (buffptr -> block_prop (nb) -> prop.gps);
      header [3] = htonl (buffptr -> block_prop (nb) -> prop.gps_n);
      header [4] = htonl (buffptr -> block_prop (nb) -> prop.seq_num + seq_num);

      DEBUG(5, cerr << "GPS time in consumer: " << buffptr -> block_prop (nb) -> prop.gps << ":" << buffptr -> block_prop (nb) -> prop.gps_n << endl);

      if (send_to_client ((char *) &header, 5 * sizeof (unsigned int)))
	return NULL;

      // send samples doing decimation if required
      for (int i = 0; i < dec_vec_len; i++)
	{
	  char *bptr = (char *) (buffptr -> block_ptr (nb) + dec_vec [i].vec_idx);
	  if (dec_vec [i].vec_rate)
	    {
	      int samples_per_point = dec_vec [i].vec_len / dec_vec [i].vec_bps / dec_vec [i].vec_rate;
	      //	      DEBUG1(cerr << "net_writer_c::consumer(decimation): averaging " << samples_per_point << " samples" << endl);
	      DEBUG1(cerr << "net_writer_c::consumer(decimation) decimating" << samples_per_point << " samples" << endl);

	      if (channels[i].data_type == _32bit_complex) {
		    for (int j = 0; j < dec_vec [i].vec_rate; j++) {
		      ((float *) bptr) [2*j] = ((float *) bptr) [2*j * samples_per_point];
		      ((float *) bptr) [2*j + 1] = ((float *) bptr) [2*j * samples_per_point + 1];
		    }
	      } else {

	      if (no_averaging) {
		if (dec_vec [i].vec_bps == 2) {
		  for (int j = 0; j < dec_vec [i].vec_rate; j++)
		    ((short *) bptr) [j] = ((short *) bptr) [j * samples_per_point];
		} else if (dec_vec [i].vec_bps == 4) {
		  for (int j = 0; j < dec_vec [i].vec_rate; j++)
		    ((float *) bptr) [j] = ((float *) bptr) [j * samples_per_point];
		} else if (dec_vec [i].vec_bps == 8) {
		  for (int j = 0; j < dec_vec [i].vec_rate; j++)
		    ((double *) bptr) [j] = ((double *) bptr) [j * samples_per_point];
		}
	      } else {
		if (dec_vec [i].vec_bps == 2) {
		  for (int j = 0; j < dec_vec [i].vec_rate; j++)
		    ((short *)bptr) [j] = averaging ((short *)bptr + j * samples_per_point, samples_per_point);
		  //	((short *) bptr) [j] = ((short *) bptr) [j * samples_per_point];
		} else if (dec_vec [i].vec_bps == 4) {
		  for (int j = 0; j < dec_vec [i].vec_rate; j++)
		    ((float *)bptr) [j] = averaging ((float *)bptr + j * samples_per_point, samples_per_point);
		  // ((int *) bptr) [j] = ((int *) bptr) [j * samples_per_point];
		} else if (dec_vec [i].vec_bps == 8) {
		  for (int j = 0; j < dec_vec [i].vec_rate; j++)
		    ((double *) bptr) [j] = averaging ((double *)bptr + j * samples_per_point, samples_per_point);
		  // ((double *) bptr) [j] = ((double *) bptr) [j * samples_per_point];
		}
	      }
 	      }

	      int bytes_to_send = dec_vec [i].vec_rate * dec_vec [i].vec_bps;

//#ifndef DATA_CONCENTRATOR
#if defined(USE_BROADCAST) || defined(DATA_CONCENTRATOR)
	// Only byteswap full-res data
	//if (source_buffptr == daqd.b1) {

	  // Swap short as ints (required at the 40m)
	  if (dec_vec [i].vec_bps == 8) {
            for (int j = 0; j < dec_vec [i].vec_len/8; j++)
              ((double *)bptr) [j] = htond(((double *)bptr) [j]);
          } else {
	    //printf("Byteswaping data as integers\n");
	    for (int j = 0; j < dec_vec [i].vec_len/4; j++)
	      ((unsigned int *)bptr) [j] = htonl(((unsigned int *)bptr) [j]);
	  }

	//}
#else
	if (dec_vec [i].vec_bps == 2) {
	  for (int j= 0; j < bytes_to_send/2; j++)
	    ((unsigned short *)bptr) [j] = htons(((unsigned short *)bptr) [j]);
	} else if (dec_vec [i].vec_bps == 4) {
	  for (int j= 0; j < bytes_to_send/4; j++)
	    ((unsigned int *)bptr) [j] = htonl(((unsigned int *)bptr) [j]);
	} else if (dec_vec [i].vec_bps == 8) {
	  for (int j= 0; j < bytes_to_send/8; j++)
	    ((double *)bptr) [j] = htond(((double *)bptr) [j]);
	} 
#endif
//#endif
	      DEBUG1(cerr << "net_writer_c::consumer(decimation): sending offs=" << dec_vec [i].vec_idx << " of " << bytes_to_send << " bytes" << endl);
	      if (send_to_client ((char *) bptr, bytes_to_send))
		return NULL;
	    }
	  else
	    {
//#ifndef DATA_CONCENTRATOR
#if defined(USE_BROADCAST) || defined(DATA_CONCENTRATOR)
	// Only byteswap full-res data
	//if (source_buffptr == daqd.b1) {
	  // Swap short as ints (required at the 40m)
	  if (dec_vec [i].vec_bps == 8) {
            for (int j = 0; j < dec_vec [i].vec_len/8; j++)
              ((double *)bptr) [j] = htond(((double *)bptr) [j]);
          } else {
	    //printf("Byteswaping data as integers\n");
	    for (int j = 0; j < dec_vec [i].vec_len/4; j++)
	      ((unsigned int *)bptr) [j] = htonl(((unsigned int *)bptr) [j]);
	  }
	//}
#else
	if (dec_vec [i].vec_bps == 2) {
	  for (int j = 0; j < dec_vec [i].vec_len/2; j++)
	    ((unsigned short *)bptr) [j] = htons(((unsigned short *)bptr) [j]);
	} else if (dec_vec [i].vec_bps == 4) {
	  for (int j = 0; j < dec_vec [i].vec_len/4; j++)
	    ((unsigned int *)bptr) [j] = htonl(((unsigned int *)bptr) [j]);
	} else if (dec_vec [i].vec_bps == 8) {
	  for (int j = 0; j < dec_vec [i].vec_len/8; j++)
	    ((double *)bptr) [j] = htond(((double *)bptr) [j]);
	} 
#endif
//#endif
	      DEBUG1(cerr << "net_writer_c::consumer(decimation): sending offs=" << dec_vec [i].vec_idx << " of " << dec_vec [i].vec_len << " bytes" << endl);
	      if (send_to_client ((char *) bptr, dec_vec [i].vec_len))
		return NULL;
	    }
	}

      num_sent++;

      buffptr -> unlock (0);
    } while (1);
  } else { // No decimation on the data is done
    if (writer_type == frame_writer) {
#ifndef USE_FRAMECPP
	  this -> shutdown_net_writer ();
	  return NULL;
#else

#ifndef FILE_CHANNEL_CONFIG
#if !defined(NO_BROADCAST) && defined(GDS_TESTPOINTS)
      for (int i = 0; i < num_channels; i++) {      
	if (IS_GDS_SIGNAL(channels[i])||IS_GDS_ALIAS(channels[i])) {
	  // Pad channels name to the maximum possible length
	  memset(channels[i].name + strlen(channels[i].name),
		 ' ', channel_t::channel_name_max_len - 1 - strlen(channels[i].name));
	  channels[i].name[channel_t::channel_name_max_len-1]=0;
	}
      }
#endif
#endif

#if FRAMECPP_DATAFORMAT_VERSION > 4
      FrameCPP::Version_6::FrameH* frame = daqd.full_frame_long(channels, num_channels);
#else
      FrameCPP::Frame* frame = daqd.full_frame_long(channels, num_channels);
#endif
      if (! frame)
	{
	  system_log(1, "Network writer couldn't create frame");
	  this -> shutdown_net_writer ();
	  return NULL;
	}
      myImageFrameWriter *fw = 0;
      strstream *ost = 0;

      // create frame image
      //
      try {
	fw = new myImageFrameWriter (*(ost = new strstream ()));
	fw -> writeFrame (*frame);
	fw -> close ();
      } catch (write_failure) {
	system_log(1, "Couldn't create image frame writer in net_writer; write_failure");
	delete frame;
	delete fw;
	delete ost;
	this -> shutdown_net_writer ();
	return NULL;
      }

      int image_size = ost -> pcount ();
#ifndef FILE_CHANNEL_CONFIG
#if !defined(NO_BROADCAST) && defined(GDS_TESTPOINTS)
      char *eofImage = 0;
      unsigned int eofImageLength = 0;
      {
	unsigned int l, m;
	// Read TOC offset from file's end
 	memcpy(&l, ost -> str() + image_size - 4, 4);
	// Read TOC length
#if FRAMECPP_DATAFORMAT_VERSION > 4
	memcpy(&m, ost -> str() + image_size - l + 4, 4);
#else
	memcpy(&m, ost -> str() + image_size - l, 4);
#endif
	// Image includes all dictionary junk and the end of file structure itself
	eofImageLength = l - m;
	eofImage = (char *)malloc(eofImageLength);
	if (!eofImage) {
		system_log(1,"Out of memory");
		delete frame;
		delete fw;
		delete ost;
		this -> shutdown_net_writer ();
		return NULL;
	}
	memcpy(eofImage, ost -> str() + image_size - eofImageLength, eofImageLength); 
      }
#endif
#endif

      // prepare an array of pointers to the ADC data
      void *adc_name_ptr[num_channels];
      void *adc_data_ptr[num_channels];
      void *data_valid_ptr [num_channels];

      {
#if FRAMECPP_DATAFORMAT_VERSION > 4
	typedef myImageFrameWriter::OMI OMI;
#else
	typedef FrameCPP::ImageFrameWriter::OMI OMI;
#endif
	for (int i = 0; i < num_channels; i++) {
	  INT_8U offs = 
	    fw -> adcNamePositionMap [channels [i].name].first[0];
	  INT_4U classInstance = fw -> adcDataVectorPtr (offs);
	  INT_2U instance = classInstance  & 0xffff;
	  OMI v = fw -> vectInstanceOffsetMap.find (instance);
#if 0
#ifndef NDEBUG
	  cout << "net_writer vector " << (classInstance >> 16) << "\t" << (classInstance&0xffff) << endl;
	  if (v == fw -> vectInstanceOffsetMap.end()) {
	    cout << "net_writer vector " << instance << " not found!" << endl;
	    exit(1);
	  } else {
	    cout << "data vector offset is " << v -> second[0] << endl;
	  }
#endif
#endif

#if FRAMECPP_DATAFORMAT_VERSION > 4
	  adc_name_ptr [i] = fw -> adcName (offs+14);
#else
	  adc_name_ptr [i] = fw -> adcName (offs+8);
#endif
	  adc_data_ptr [i] = fw -> vectorData (v -> second[0]);
	  data_valid_ptr [i] = fw -> adcDataValidPtr (offs);
	}
      }
	
#ifndef FILE_CHANNEL_CONFIG
#if !defined(NO_BROADCAST) && defined(GDS_TESTPOINTS)
	
      // Change pad character from ' ' to '\0' in all GDS signal names.
      // This basically places zero terminator in place of first pad character.
      //
      for (int i = 0; i < num_channels; i++) {
	if (IS_GDS_SIGNAL(channels[i])||IS_GDS_ALIAS(channels[i])) {
	  char *ptr = strchr((char *)(adc_name_ptr [i]), ' ');
	  if (ptr)
	    *ptr =0;
	  ptr = strchr(channels [i].name, ' ');
	  if (ptr) {
#if FRAMECPP_DATAFORMAT_VERSION > 4
	    FrameCPP::Version_6::FrTOC::MapADC_t::const_iterator itr
		    = fw -> oframestream.m_toc.m_ADC.find((char *)(channels [i].name));
		  if (itr != fw -> oframestream.m_toc.m_ADC.end()) {
		    FrameCPP::Version_6::FrTOC::ADC_t v = itr -> second;
		    fw -> oframestream.m_toc.m_ADC.erase(channels [i].name);
		    *ptr = 0;
		    fw -> oframestream.m_toc.m_ADC[channels [i].name] = v;
		  } 
#else
	    FrameCPP::ImageFrameWriter::ANPINC itr
	      = fw -> adcNamePositionMap.find(channels[i].name);
	    if (itr != fw -> adcNamePositionMap.end()) {
		std::pair< std::vector<INT_8U>, std::vector<INT_4U> > v = itr -> second;
		fw -> adcNamePositionMap.erase(channels [i].name);
		*ptr = 0;
	    	fw -> adcNamePositionMap[channels [i].name] = v;
	    }
#endif
	  }
	}
      }

#endif
#endif

      printf("Frame broadcaster is finished constructing frame image\n");

      nb = 0;

      int *status_ptr = (int *)(buffptr -> block_ptr (nb) + buffptr -> block_prop (nb) -> bytes)
	- num_channels;
      do {
	int bytes;

	nb = buffptr -> get (0);
	if (! (bytes = buffptr -> block_prop (nb) -> bytes))
	  break;

	char *buf = buffptr -> block_ptr (nb);
	int offset = 0;

	// Put data into the ADC structures
	for (int i = 0; i < num_channels; offset += channels [i].bytes, i++) {
#ifdef USE_BROADCAST
                  // Fixed after John Z found out a "big problem" with the boadcast frames
                  // Short data needed to be sample swapped
                  if (channels [i].bps == 2) {
                    short *dest = (short *)(adc_data_ptr [i]);
                    short *src = (short*)(buf + offset);
                    unsigned int samples = channels [i].bytes / 2;
                    for (int k = 0; k < samples; k++) dest[k] = src[k^1];
		  } else
#endif
	  memcpy (adc_data_ptr [i], buf + offset, channels [i].bytes);
	  
	  // This memcpy() converts integer status into short
	  memcpy (data_valid_ptr [i], ((INT_2U *)(status_ptr + i)) + 1, sizeof(INT_2U));
	}

#ifndef FILE_CHANNEL_CONFIG
#if !defined(NO_BROADCAST) && defined(GDS_TESTPOINTS)
#ifdef not_def
	system_log(1, "Got gds signal %d", buffptr -> block_prop (nb) -> prop.gds_signal_refresh);
	for ( int i = 0; i < 16; i++) {
	system_log(1, "%d", buffptr -> block_prop (nb) -> prop16th[i].gds_signal_refresh);
	}
#endif

	// Replace test point names, include active test point ADCs
	// into the frame.
	// Done in the very beginning and after refresh signal is received
	if (!num_sent || buffptr -> block_prop (nb) -> prop.gds_signal_refresh) {
	  system_log(1,"gds refresh signal received");
	  // Create a list of active alias channels (real signals)
	  //
	  channel_t *aliasch[daqd.num_gds_channel_aliases];
	  int num_alias_ch = 0;
	  for (int i = 0; i < daqd.num_channels; i++) {
	    if (IS_GDS_ALIAS(daqd.channels [i])) {
	      struct dataInfoStr *dinfo = daqd.channels [i].rm_dinfo;
	      assert (dinfo);
	      // check data offset
	      if (bsw(dinfo->dataOffset) != 0) {
		aliasch[num_alias_ch++] = daqd.channels + i;
	      }
	    }
	  }

	  // Replace GDS signal names

	  //:TODO: Fix for FrameCPP::Version6

	  for (int i = 0; i < num_channels; i++) {
	    if (IS_GDS_SIGNAL(channels[i])) {
	      int j;
	      for (j = 0; j < num_alias_ch; j++) {
		if (bsw(aliasch [j] -> rm_dinfo -> dataOffset) + sizeof(int)
		    == channels[i].rm_offset)
		  break;
	      }
	      
	      if (j != num_alias_ch) {
		// ALIAS or real signal name is set
		if (strcmp((char *)(adc_name_ptr[i]), aliasch[j] -> name)) {
#if FRAMECPP_DATAFORMAT_VERSION > 4
		  FrameCPP::Version_6::FrTOC::MapADC_t::const_iterator itr
		    = fw -> oframestream.m_toc.m_ADC.find((char *)(adc_name_ptr[i]));
		  if (itr != fw -> oframestream.m_toc.m_ADC.end()) {
		    FrameCPP::Version_6::FrTOC::ADC_t v = itr -> second;
		    fw -> oframestream.m_toc.m_ADC.erase((char *)(adc_name_ptr[i]));
		    fw -> oframestream.m_toc.m_ADC[aliasch[j] -> name] = v;
		  } 
#else
		  FrameCPP::ImageFrameWriter::ANPINC itr
		    = fw -> adcNamePositionMap.find((char *)(adc_name_ptr[i]));
		  if (itr != fw -> adcNamePositionMap.end()) {
		    std::pair< std::vector<INT_8U>, std::vector<INT_4U> > v = itr -> second;
		    fw -> adcNamePositionMap.erase((char *)(adc_name_ptr[i]));
		    fw -> adcNamePositionMap[aliasch[j] -> name] = v;
		  }
#endif
		  else {
		    system_log(1,"Warning: failed to activate %s (%s) in the TOC", aliasch[j] -> name, adc_name_ptr[i]);
		  }
		  
		  strcpy((char *)(adc_name_ptr[i]), aliasch[j] -> name);
		  system_log(1,"active GDS test point `%s'", adc_name_ptr[i]);
		}
	      } else {
		if (strcmp((char *)(adc_name_ptr[i]), channels [i].name)) {
#if FRAMECPP_DATAFORMAT_VERSION > 4
		  FrameCPP::Version_6::FrTOC::MapADC_t::const_iterator itr
		    = fw -> oframestream.m_toc.m_ADC.find((char *)(adc_name_ptr[i]));
		  if (itr != fw -> oframestream.m_toc.m_ADC.end()) {
		    FrameCPP::Version_6::FrTOC::ADC_t v = itr -> second;
		    fw -> oframestream.m_toc.m_ADC.erase((char *)(adc_name_ptr[i]));
		    fw -> oframestream.m_toc.m_ADC[channels [i].name] = v;
		  }
#else
		  FrameCPP::ImageFrameWriter::ANPINC itr
		    = fw -> adcNamePositionMap.find((char *)(adc_name_ptr[i]));
		  if (itr != fw -> adcNamePositionMap.end()) {
		    std::pair< std::vector<INT_8U>, std::vector<INT_4U> > v = itr -> second;
		    fw -> adcNamePositionMap.erase((char *)(adc_name_ptr[i]));
		    fw -> adcNamePositionMap[channels [i].name] = v;
		  }
#endif
		  else {
		    system_log(1,"Warning: failed to deactivate %s in the TOC", adc_name_ptr[i]);
		  }

		  // Reset back to placeholder name
		  system_log(1,"%s is deactivated to generic `%s'", adc_name_ptr[i], channels [i].name);
		  strcpy((char *)(adc_name_ptr[i]), channels [i].name);
		}
	      }
	    }
	  }	  
		
	  // Position to the beginning of TOC image
#if FRAMECPP_DATAFORMAT_VERSION > 4
	  ost -> seekp(fw -> getTOCOffset());
#else
	  ost -> seekp(fw -> getTOCOffset() - 8);
#endif
	  // Write new TOC data and then EOF
	  INT_4U pre_toc_pos = ost ->tellp();
	  fw -> writeTOC();
	  // Append EOF structure
#if FRAMECPP_DATAFORMAT_VERSION > 4
	  fw -> oframestream.write(eofImage, eofImageLength-24);
	  fw -> oframestream << (INT_8U)(ost -> tellp() + 24) << (INT_4U)0 << (INT_4U)0;
	  // Write offset to the TOC from the end of the file
	  fw -> oframestream << (INT_8U)(ost -> tellp() + 8 - pre_toc_pos);
#else
	  fw -> FrameCPP::Output::write(eofImage, eofImageLength-16);
	  *fw << (INT_4U)(ost -> tellp() + 16) << (INT_4U)0 << (INT_4U)0;
	  // Write offset to the TOC from the end of the file
	  *fw << (INT_4U)(ost -> tellp() + 4 - pre_toc_pos);
#endif
	  image_size = ost -> tellp();
	}
#endif
#endif

	// Assign status 
	
	// Update frame header

	fw -> setFrameFileAttributes (buffptr -> block_prop (nb) -> prop.run,
				      buffptr -> block_prop (nb) -> prop.seq_num + seq_num,
				      0,
				      buffptr -> block_prop (nb) -> prop.gps, 1,
				      buffptr -> block_prop (nb) -> prop.gps_n,
				      buffptr -> block_prop (nb) -> prop.leap_seconds,
				      buffptr -> block_prop (nb) -> prop.altzone);

	// send block length and the header
	header [0] = htonl (4 * sizeof (unsigned int) + image_size);
	header [2] = htonl (buffptr -> block_prop (nb) -> prop.gps);
	header [3] = htonl (buffptr -> block_prop (nb) -> prop.gps_n);
	header [4] = htonl (buffptr -> block_prop (nb) -> prop.seq_num + seq_num);

	DEBUG(5, cerr << "GPS time in consumer: " << buffptr -> block_prop (nb) -> prop.gps << ":" << buffptr -> block_prop (nb) -> prop.gps_n << endl);

	if (send_to_client ((char *) &header, 5 * sizeof (unsigned int)))
	  break;

	// send frame image
	DEBUG1(cerr << "net_writer_c::consumer(): sending block " << nb << " of " << image_size << " bytes" << endl);
	if (send_to_client (ost -> str (), image_size, buffptr -> block_prop (nb) -> prop.gps, buffptr -> block_period ()))
	  break;

	num_sent++;

	buffptr -> unlock (0);
      } while (1);

      delete fw;
      delete frame;
      delete ost;
#ifndef FILE_CHANNEL_CONFIG
#if !defined(NO_BROADCAST) && defined(GDS_TESTPOINTS)
      free(eofImage);
#endif
#endif
#endif
#ifdef DATA_CONCENTRATOR
    } else if (broadcast) {

// The header comes in front of the data in the transmission buffer
// This header specifies DCU numbers and for each DCU it sends dcuid, data size,
// and data CRC, config CRC, status.
// The receiver will use this information to check the data and set its own DCU status
//
	static const unsigned int header_size = 2048; // Header has fixed size
	char *tbuf =  (char *) malloc (transmission_block_size + header_size);
	if (tbuf == 0) abort(); // Memory allocation failed for some reason, we can't continue
	unsigned int *hptr = (unsigned int *) tbuf; // Pointer into header area
	unsigned int ndcu = 0;	// Count how many DCUs will be included in the transmission
	unsigned int tidx = 1; // table index (integers) 

	for (int ifo = 0; ifo < daqd.data_feeds; ifo++) {
	  for (int i = DCU_ID_EDCU; i < DCU_COUNT; i++) {
		if (IS_TP_DCU(i)) continue; 	// Skip TP and EXC DCUs
		if (daqd.dcuSize[ifo][i] == 0) continue; // Skip unconfigured DCUs
		printf("DCU %d IFO %d; size=%d\n", i, ifo, daqd.dcuSize[ifo][i]);
		ndcu++;
// broadcast protocol dependency
#if DCU_COUNT != 32
//#error
#endif
		// DCU number
		hptr[tidx++] = htonl(i + ifo*DCU_COUNT);
		// DCU size
		hptr[tidx++] = htonl(daqd.dcuDAQsize[ifo][i]);
		// DCU config file CRC
		hptr[tidx++] = htonl(daqd.dcuConfigCRC[ifo][i]);
		// DCU data block CRC (will patch it in for each data block)
		hptr[tidx++] = htonl(0);
		// DCU status (will patch in later)
		hptr[tidx++] = htonl(0);
		// DCU cycle (will patch in later)
		hptr[tidx++] = htonl(0);
	  }
	}
	*hptr = htonl(ndcu); // Assign the number of DCUs after they're counted

      do {
	int bytes;
        nb = buffptr -> get16th (cnum);
        int nb16th = nb & 0xf; 
        nb >>= 4;
	circ_buffer_block_prop_t &prop = buffptr -> block_prop (nb) -> prop16th [nb16th];
	if (! (bytes = buffptr -> block_prop (nb) -> bytes)) break;
        //bytes -= 4 * num_channels; // Subtract the size of the status words
        bytes = transmission_block_size + header_size;

	// send samples
	DEBUG1(cerr << "net_writer_c::consumer(): sending block " << nb << " of " << bytes << " bytes" << endl);

	char *ptr = buffptr -> block_ptr (nb);
	struct put_vec *pv = pvec16th [nb16th];
	unsigned int dlen = header_size;

	// Copy all DAQ configured channels over to our transmission buffer
    	for (int i = 0; i < pvec_len; i++) {
	 // printf("%d %d\n", transmission_block_size, dlen + pv [i].vec_len);
          memcpy (tbuf + dlen, ptr + pv [i].vec_idx, pv [i].vec_len);
	  dlen += pv [i].vec_len;
	}
	// Make sure we didn't overwrite the buffer boundary
	if (dlen > bytes) abort();

 	// Assign DCU status data into the header
	tidx = 1;
	for (int ifo = 0; ifo < daqd.data_feeds; ifo++) {
	  for (int i = DCU_ID_EDCU; i < DCU_COUNT; i++) {
		if (IS_TP_DCU(i)) continue; 	// Skip TP and EXC DCUs
		if (daqd.dcuSize[ifo][i] == 0) continue; // Skip unconfigured DCUs
		// DCU number
		// DCU size
		// DCU config file CRC
		tidx += 3;
		// DCU data block CRC 
		hptr[tidx++] = htonl(prop.dcu_data[i + ifo*DCU_COUNT].crc);
		// DCU status
		hptr[tidx++] = htonl(prop.dcu_data[i + ifo*DCU_COUNT].status);
		// DCU cycle
		hptr[tidx++] = htonl(prop.dcu_data[i + ifo*DCU_COUNT].cycle);
	  }
	}
	if (send_to_client (tbuf, bytes, prop.gps, prop.gps_n)) return NULL;
	int tpdata_len = 0;

#ifdef GDS_TESTPOINTS
	// Build test points data block
	char *tpdata = daqd.gds.build_tp_data(&tpdata_len, ptr, nb16th);

	if (send_to_client (tpdata, tpdata_len, prop.gps, prop.gps_n, 1)) return NULL;
#endif

	num_sent++;

	if (nb16th == 0xf) buffptr -> unlock16th (cnum);
      } while (1);
#endif
    } else {
      do {
	int bytes;
	nb = buffptr -> get (0);
	if (! (bytes = buffptr -> block_prop (nb) -> bytes)) break;
        //bytes -= 4 * num_channels; // Subtract the size of the status words
        bytes = transmission_block_size;

        if (need_send_reconfig_block) {
      	  int channel_status [num_channels];
	  // Always send reconfiguration block in the beginning
	  // See if status changed and send reconfig block if it did
	  int *status_ptr = (int *)(buffptr -> block_ptr (nb) + buffptr -> block_prop (nb) -> bytes)
	    - num_channels;
	  if (! num_sent || memcmp (channel_status, status_ptr, num_channels * sizeof(int))) {
	    if (send_reconfig_block (status_ptr, true, true))
	      return NULL;
	    memcpy (channel_status, status_ptr, num_channels * sizeof(int));
	  }
	}

	// send block length and the header
	header [0] = htonl (4 * sizeof (unsigned int) + bytes);
	header [2] = htonl (buffptr -> block_prop (nb) -> prop.gps);
	header [3] = htonl (buffptr -> block_prop (nb) -> prop.gps_n);
	header [4] = htonl (buffptr -> block_prop (nb) -> prop.seq_num + seq_num);

	DEBUG(5, cerr << "GPS time in consumer: " << buffptr -> block_prop (nb) -> prop.gps << ":" << buffptr -> block_prop (nb) -> prop.gps_n << endl);

	if (send_to_client ((char *) &header, 5 * sizeof (unsigned int)))
	  return NULL;

	// send samples
	DEBUG1(cerr << "NO DECIMATION net_writer_c::consumer(): sending block " << nb << " of " << bytes << " bytes" << endl);

	// Byteswap to network order
	char *data = buffptr -> block_ptr (nb);
	for (int i = 0; i < num_channels; i++) {
		//printf("Byteswap %s rate=%d type=%d\n", channels[i].name, channels[i].sample_rate, channels[i].data_type);
	        if (channels[i].bps == 4) {
		   for (int j = 0; j < channels[i].sample_rate; j++) {
			*((int *)data) = htonl(*((int *)data));
			data += 4;
		   }
	        } else if (channels[i].bps == 2) {
		   for (int j = 0; j < channels[i].sample_rate; j++) {
			*((short *)data) = htons(*((short *)data));
			data += 2;
		   }
	        } else if (channels[i].bps == 8) {
		   for (int j = 0; j < channels[i].sample_rate; j++) {
			*((double *)data) = htond(*((double *)data));
			data += 8;
		   }
		}
	}

	if (send_to_client (buffptr -> block_ptr (nb), bytes,
		    buffptr -> block_prop (nb) -> prop.gps,
		    buffptr -> block_prop (nb) -> prop.gps_n))
  		return NULL;
	num_sent++;

	buffptr -> unlock (0);
      } while (1);
    }
  }

  (void) send_trailer ();

  // shutdown the buffer if zero-length block received
  this -> shutdown_net_writer ();

  DEBUG1(cerr << "network writer got zero length block; sent " << num_sent << " transmission blocks" << endl);
  DEBUG1(cerr << "there were " << buffptr -> drops () << " blocks dropped" << endl);
  return NULL;
}


/*
  Offline net-writer producer.

  Transient producer is pretty much the same as the `producer()'.
  The difference is in the use of `get_nowait()' circular buffer call,
  and the way it shuts down. It shuts down whenever `get_nowait()' fails
  (well, it fails when there is no block available in the circular buffer).

  Access to buffptr is synchronized with its destruction in
  transient_consumer() by the transiency_locker.

 */
void *
net_writer_c::transient_producer ()
{
  transiency_locker mon(this); // Synch with pbuffer destruction in transient consumer
  int nb;
  int num_sent = 0;

  if (! buffptr)
    {
      DEBUG1(cerr << "transient producer encountered empty pbuffer; finished" << endl);
      return NULL;
    }

#ifndef VMICRFM_PRODUCER
  // Only if this thread works on main circular buffer
  if (buffptr == daqd.b1) {
    // Put this  thread into the realtime scheduling class with half the priority
    daqd_c::realtime ("net_writer transient producer", 2);
  }
#endif

  for (;;)
    {
      assert (buffptr);

      nb = source_buffptr -> get_nowait (cnum);
      {
	if (nb < 0) // this signals the end of input for this thread
	  {
	    char buf [1];

	    // no unlock if `get_nowait()' "failed"
	    //	    source_buffptr -> unlock (cnum);

	    source_buffptr -> delete_consumer (cnum);
	    buffptr -> put (buf, 0);
	    DEBUG1(cerr << "transient producer finished; cnum=" << cnum << " passed " << num_sent << " blocks" << endl);
	    return NULL;
	  }

	num_sent ++;

	buffptr -> put_nowait_scattered ((char *) source_buffptr -> block_ptr (nb), pvec, pvec_len,
					 &source_buffptr -> block_prop (nb) -> prop);
	DEBUG1(cerr << "net_writer_c::transient_producer(): put() of " << source_buffptr -> block_prop (nb) -> bytes << " bytes" << endl);
      }
      source_buffptr -> unlock (cnum);
    }
  return NULL;
}


/*
  Offline net-writer consumer.

  Takes the data from either net-writer circular buffer or frame files or both
  and sends it out to the client.
*/
void *
net_writer_c::transient_consumer ()
{
  // Most of these checks have to be transferred to the `start_net_writer'
  // function -- need to report an error to the client through the control connection

  // See if all the data we need to send is in memory and we pretty much done
  if (! offline.delta && offline.gps <= (offline.blast - offline.bstart + source_buffptr -> block_period ()))
    {
      assert (buffptr);

      if (! (send_client_header (1))) {
	(void) consumer (); // Send the blocks
      }
    }
  else
    {
      time_t gps_time, delta_time;

      if (! offline.delta) // Request to get data for last seconds
	{
	  delta_time = offline.gps - (offline.blast - offline.bstart + source_buffptr -> block_period ());
	  gps_time = offline.bstart - delta_time;
	}
      else // Specified time period in the request
	{
	  if (offline.bstart)
	    {
	      gps_time = offline.gps < offline.bstart? offline.gps: offline.bstart;
	      delta_time = offline.bstart - gps_time;
	    }
	  else
	    {
	      gps_time = offline.gps;
	      delta_time = offline.delta;
	    }
	}

      // Send data that's read from files

      /* 
	 Frames files are stored for a certain time period in several
	 directories, identified by number. So, having `gps_time' and
	 `delta_time' now we need to find out what we have available in
	 files: what file to read first and how many to read. The
	 information, which makes it possible to do that, is stored in
	 `*fsd' structure.  */

      int raw_minute_data_kludge = source_buffptr == daqd.trender.mtb || offline.no_time_check;

      // Check if the `daqd.fsd' contains valid information
      if (fsd -> is_empty () && !raw_minute_data_kludge)
	{
	  DEBUG1(cerr << "gps -> filename map is empty -- no access to frame files" << endl);

	  if (offline.bstart) // If there is data in memory buffer, just send that data
	    {
	      if (! (send_client_header (1)))
		{
		  assert (buffptr);
		  (void) consumer (); // Send data blocks
		}
	    }
	  else
	    {
	      //	      send_to_client (S_DAQD_NOT_FOUND, 4);
	      (void) send_trailer ();
	      shutdown_net_writer ();
	    }
	}
      else
	{
	  if (!raw_minute_data_kludge) {
	  	time_t min_time = fsd -> get_min();
	  	time_t new_gps_time = gps_time < min_time? min_time: gps_time;
	  	delta_time -= new_gps_time - gps_time;
	  	gps_time = new_gps_time;
	  }

	  if (delta_time <= 0 && !raw_minute_data_kludge)
	    {
	      DEBUG1(cerr << "no data available from files" << endl);

	      if (offline.bstart) // If there is data in memory buffer
		{
		  if (! (send_client_header (1)))
		    {
		      assert (buffptr);
		      (void) consumer (); // Send data blocks
		    }
		}
	      else
		{
		  // FIXME: NACK is sent on the data connection ???
		  // The whole check (delta_time <= 0 && !offline.bstart) must be moved
		  // to the the start_net_writer()

		  // FIXME: start trend net-writer 0 5 {"IFO_DMRO.min"} sends ACK
		  // and then disconnects

		  //		  send_to_client (S_DAQD_NOT_FOUND, 4);

		  (void) send_trailer ();
		  shutdown_net_writer ();
		}
	    }
	  else
	    {
#ifdef not_def
	      int num_blocks = 1;
	      num_blocks = delta_time; // that long will be read from files
	      if (offline.bstart)
		num_blocks += offline.blast - offline.bstart + 1; // Plus several blocks from memory, possibly
#endif

	      if (! (send_client_header (1)))
		{
		    
		  DEBUG1(cerr << "net_writer_c::transient_consumer(): read data from disk starting at " << gps_time << " for " << delta_time << " seconds and send out to the client" << endl);
		    
		  offline.gps = gps_time;
		  offline.delta = delta_time;

		  int res;

		  // Send data from files starting at `offline.gps_time'
		  // for `offline.delta' seconds

#ifdef MINI_NDS

#error Older code was removed...

		  res = older_send_files ();
#else

		  if (source_buffptr == daqd.trender.tb) {
		    time_t file_gps = fsd->file_gps(offline.gps);
		    res = send_files ();
		  } else 
		    res = send_files ();

#endif // MINI_NDS

		  // Send data from the memory buffer
		  if (! res) {
		    if (offline.bstart && buffptr) // If there is data in memory buffer
		      {
			assert (buffptr);
			(void) consumer (); // Send data blocks
		      }
		    else
		      {
			(void) send_trailer ();
			shutdown_net_writer ();
		      }
		  }
		}
	      else
		shutdown_net_writer ();
	    }
	}
    }

  // Will do shutdown twice for most of the cases, but it is better, I guess, than to run out of descriptors
  shutdown_net_writer ();

  if (buffptr) {
    transiency_locker mon(this);
    shutdown_buffer ();
  }

  s_link *nwres = daqd.net_writers.remove (this);
  assert (nwres);

  return NULL;
}


int
net_writer_c::connect_srvr_addr (int ssize)
{
#ifndef NO_BROADCAST

  // Initialize Framexmit connection for broadcast

  if (broadcast) {
    char buf [256];
    char *istr = net_writer_c::inet_ntoa  (srvr_addr.sin_addr, buf);

    // allocate transmit buffers, if first time

    for (int i = 0; i < radio_buf_num; i++) {
      radio_bufs [i].inUse = false;
      if (! radio_bufs [i].data) {
	radio_bufs [i].data = new (nothrow) char [radio_buf_len];
	if (radio_bufs [i].data == 0) {
	  system_log(1, "cannot create framexmit buffers");
	  return DAQD_ERROR;
	}
      }
    }

#ifdef DATA_CONCENTRATOR
    // open connection
    if (!radio.open (istr, mcast_interface, concentrator_broadcast_port)) {
      system_log(1, "framexmit open failed");
      return DAQD_ERROR;
    }
    DEBUG1(cerr << "opened framexmit addr=" << istr << " iface=" << (mcast_interface? mcast_interface: "") << endl);

    if (!radio_tp.open (istr, mcast_interface, concentrator_broadcast_port_tp)) {
      system_log(1, "TP framexmit open failed");
      return DAQD_ERROR;
    }
    DEBUG1(cerr << "opened TP framexmit addr=" << istr << " iface=" << (mcast_interface? mcast_interface: "") << endl);
#else
    // open connection
    printf("WARNING: DMT broadcaster opened on non-standard port %d\n", diag::frameXmitPort + 2);
    if (!radio.open (istr, mcast_interface, diag::frameXmitPort)) {
      system_log(1, "framexmit open failed");
      return DAQD_ERROR;
    }
    DEBUG1(cerr << "opened framexmit addr=" << istr << " iface=" << (mcast_interface? mcast_interface: "") << endl);


#endif

    fileno = -1; // socket is used by framexmit privately
    system_log(1,"framexmit connected to %s", istr);
  } else 
#endif
{
  int srvr_addr_len = sizeof (srvr_addr);
  int sockfd;
  const int max_allowed = 64 * 1024; /* 64K seems to be system imposed limit on Solaris */
  int sendbuf_size = (ssize && ssize < max_allowed)? ssize: max_allowed;
  char buf [256];
  char *istr = net_writer_c::inet_ntoa  (srvr_addr.sin_addr, buf);

  /* Connect to the destination */
  if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
      system_log(1, "socket(%s.%d); errno=%d", net_writer_c::inet_ntoa (srvr_addr.sin_addr, buf), srvr_addr.sin_port, errno);
      return DAQD_SOCKET;
    }

  if (setsockopt (sockfd, SOL_SOCKET, SO_SNDBUF, (const char *) &sendbuf_size, sizeof (sendbuf_size)))
    {
      system_log(1, "setsockopt(%s.%d); errno=%d", net_writer_c::inet_ntoa (srvr_addr.sin_addr, buf), srvr_addr.sin_port, errno);
      close (sockfd);
      return DAQD_SETSOCKOPT;
    }

  int flag = 1;


#ifdef not_def
  // Defeats Nagle buffering algorithm
  int result = setsockopt(sockfd,            /* socket affected */
			  IPPROTO_TCP,     /* set option at TCP level */
			  TCP_NODELAY,     /* name of option */
			  (char *) &flag,  /* the cast is historical
					      cruft */
			  sizeof(int));    /* length of option value */
#endif

#ifdef not_def
  int rcvbuf_size = 1024 * 10;  
  if (setsockopt (sockfd, SOL_SOCKET, SO_RCVBUF, (const char *) &rcvbuf_size, sizeof (rcvbuf_size)))
    fprintf (stderr, "setsockopt(%d, %d); errno=%d\n", sockfd, rcvbuf_size, errno);

  {
    int sendbuf_size_len = 4;
    sendbuf_size = -1;
    if (getsockopt (sockfd, SOL_SOCKET, SO_RCVBUF, (const char *) &sendbuf_size, &sendbuf_size_len))
      fprintf (stderr, "getsockopt(%d, %d); errno=%d\n", sockfd, sendbuf_size, errno);
    else {
      system_log(1,"RCVBUF size is %d\n", sendbuf_size);
    }
  }
#endif
  
  if (connect (sockfd, (struct sockaddr *) &srvr_addr, srvr_addr_len) < 0)
    {
      system_log(1, "connect(%s.%d); errno=%d", net_writer_c::inet_ntoa (srvr_addr.sin_addr, buf), srvr_addr.sin_port, errno);
      close (sockfd);
      return DAQD_CONNECT;
    }

  {
    socklen_t sendbuf_size_len = 4;
    sendbuf_size = -1;
    if (getsockopt (sockfd, SOL_SOCKET, SO_SNDBUF, (char *) &sendbuf_size, &sendbuf_size_len)) {
      system_log(1, "getsockopt(%d, %d); errno=%d\n", sockfd, sendbuf_size, errno);
    } else {
      DEBUG1(cerr << "SNDBUF size is " << sendbuf_size << endl);
    }
  }
  fileno = sockfd;
  system_log(1,"connected to %s.%d; fd=%d", istr, srvr_addr.sin_port, fileno);
}
  return 0;
}


int
net_writer_c::set_send_buf_size (int sockfd, int ssize)
{
  const int max_allowed = 64 * 1024; /* 64K seems to be system imposed limit on Solaris */
  int sendbuf_size = (ssize && ssize < max_allowed)? ssize: max_allowed;

  // Do not do this setsockopt if the size is too small
  //
  if ( sendbuf_size < max_allowed )
	return 0;

  if (setsockopt (sockfd, SOL_SOCKET, SO_SNDBUF, (const char *) &sendbuf_size, sizeof (sendbuf_size)))
    {
      system_log(1, "setsockopt(%d, %d); errno=%d", sockfd, sendbuf_size, errno);
      return -1;
    }
  return 0;
}



/*
  Start network data transmission.

  `*yyout' is the control connection data stream.  `ofd' is control
  connection file descriptor , usable only if `no_data_connection' is set.
  `no_data_connection' is set if we shouldn't establish data connection, but
  use control connection for data transport. 

  `src_buffer' is the source circular buffer.

  `fmap' is the filesystem map to use to get off-line data frame file names.

  `no_online' is set if I should not get the data from the online buffers.
*/
int
net_writer_c::start_net_writer (ostream *yyout, int ofd, int no_data_connection,
				circ_buffer *src_buffer, filesys_c *fmap,
				time_t gps, time_t delta, int no_online)
{
  locker mon (this);   // lock is held as long as the mon exists
  int res;

  DEBUG1(cerr << "start_net_writer() called" << endl);
  /*
    Set the TCP buffer, so I can avoid buffering on the network. This
    buffering causes update delay (data backlog) effect for the very slow
    client.
  */

  if (no_data_connection)
    {
      // Duplicate passed file descriptor, so it can be closed in the net_writer code
      // leaving control connection intact.
      if ((fileno = dup (ofd)) < 0)
	return DAQD_DUP;
      if (set_send_buf_size (fileno, transmission_block_size) < 0) {
	close (fileno);
	return DAQD_SETSOCKOPT;
      }
#ifndef NO_BROADCAST
      // Cancel request if IP address isn't specified, since I cannot broadcast
      // into the client's incoming TCP connection's IP address.

      if (broadcast) {
	system_log(1, "must specify broadcast/multicast IP address in the request");
	return DAQD_ERROR;
      }
#endif
    }
  else
    {
      // Establish data connection
      if (res = connect_srvr_addr (transmission_block_size))
	return res;
    }

  source_buffptr = src_buffer;
  fsd = fmap;
  int online_writer = !gps && !delta;

  if (writer_type == frame_writer && !online_writer) // Send frame files
    {
      offline.gps = gps;
      offline.delta = delta;
      offline.bstart = offline.blast = 0;

      daqd.net_writers.insert_first (this);

      pthread_attr_t attr;
      pthread_attr_init (&attr);
      pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
      pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
      pthread_attr_setstacksize (&attr, daqd.thread_stack_size);
      int err_no;
      if (err_no = pthread_create (&consumer_tid, &attr, (void *(*)(void *)) frame_consumer_static, (void *) this)) {
	system_log(1, "couldn't create frame net-writer consumer thread; pthread_create() err=%d", err_no);
	disconnect_srvr_addr ();
	pthread_attr_destroy (&attr);
	return DAQD_THREAD_CREATE;
      }
      pthread_attr_destroy (&attr);
      DEBUG1(cerr << "frame net writer consumer thread started; tid=" << consumer_tid << endl);
    }

#ifdef not_def
  else if (name_writer == writer_type)
    {
      daqd.net_writers.insert_first (this);

      pthread_attr_t attr;
      pthread_attr_init (&attr);
      pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
      pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
      pthread_attr_setstacksize (&attr, daqd.thread_stack_size);
      pthread_create (&consumer_tid, &attr, (void *(*)(void *)) name_consumer_static, (void *) this);
      pthread_attr_destroy (&attr);
      DEBUG1(cerr << "name writer consumer thread started; tid=" << consumer_tid << endl);
    }
#endif

  else
    {
      bool no_consumer_buffer = false;

#ifdef DATA_CONCENTRATOR
      if (broadcast) {
        // Concentrator reads data directly from the main buffer
        buffptr = source_buffptr;
      } else
#endif
      if (no_online == 0 || online_writer) {
	int buffer_blocks;

	// Use just two blocks long buffer for online writer
	// If I use long buffer, then the sluggish client or the client
	// reading over the slow network will be lagging behind significantly.
	// The lower the size of the circular buffer, the shorter the lag.
	// TCP send/receive buffers are another story...
	
	if (online_writer && writer_type != fast_writer)
	  buffer_blocks = 2;
	else
	  buffer_blocks = src_buffer -> blocks () + 1;
	
	void *mptr = malloc (sizeof(circ_buffer));
	if (!mptr) {
	  system_log(1,"couldn't construct net_writer circular buffer, memory exhausted");
	  disconnect_srvr_addr ();
	  return DAQD_MALLOC;
	}
	
#ifdef not_def
	if (! (buffptr = new circ_buffer (1, buffer_blocks, block_size))) {
	  system_log(1, "couldn't construct net_writer circular buffer, memory exhausted");
	  disconnect_srvr_addr ();
	  return DAQD_MALLOC;
	}
#endif

	buffptr = new (mptr) circ_buffer (1, buffer_blocks, block_size, src_buffer -> block_period ());
	if (! (buffptr -> buffer_ptr ())) {
	  if (!online_writer) {
	    system_log(1, "couldn't allocate net_writer buffer data blocks; size=%d; errno=%d",
		       buffer_blocks * block_size + sizeof (circ_buffer_t) - 1, errno);
	    no_consumer_buffer = 1;
	  }
	  buffptr -> ~circ_buffer();
	  free ((void *) buffptr);
	  buffptr = 0;
	  if (online_writer) {
	    system_log(1, "couldn't allocate net_writer buffer data blocks, memory exhausted");
	    disconnect_srvr_addr ();
	    return DAQD_MALLOC;
	  }
	}
      }

      // Start network writer pipework
      if (online_writer)
	{
	  if ((cnum = source_buffptr -> add_consumer (writer_type == fast_writer)) >= 0)
	      {
#ifdef DATA_CONCENTRATOR
	      if (!broadcast) {
#endif
		pthread_attr_t attr;
		pthread_attr_init (&attr);
		pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
		pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
		pthread_attr_setstacksize (&attr, daqd.thread_stack_size);
		int err_no;
		if (err_no = pthread_create (&producer_tid, &attr, (void *(*)(void *))producer_static, (void *) this)) {
		  system_log(1, "couldn't create net-writer producer thread; pthread_create() err=%d", err_no);
		  source_buffptr -> delete_consumer (cnum);
		  cnum = 0;
		  disconnect_srvr_addr ();
		  buffptr -> ~circ_buffer();
		  free ((void *) buffptr);
		  buffptr = 0;
		  pthread_attr_destroy (&attr);
		  return DAQD_THREAD_CREATE;
		}

		pthread_attr_destroy (&attr);
		DEBUG1(cerr << "network writer created; tid=" << producer_tid << endl);
#ifdef DATA_CONCENTRATOR
	      }
#endif
	      }
	    else
	      {
		system_log(1, "couldn't create net-writer");
		disconnect_srvr_addr ();
		buffptr -> ~circ_buffer();
		free ((void *) buffptr);
		buffptr = 0;
		return DAQD_BUSY;
	      }

	  pthread_attr_t attr1;
	  pthread_attr_init (&attr1);
          pthread_attr_setscope(&attr1, PTHREAD_SCOPE_SYSTEM);
	  pthread_attr_setdetachstate (&attr1, PTHREAD_CREATE_DETACHED);
	  pthread_attr_setstacksize (&attr1, daqd.thread_stack_size);

#ifdef DATA_CONCENTRATOR
	if (!broadcast) {
#endif
	  // Lower this thread's priority
	  struct sched_param sparam;
	  int policy;
	  pthread_getschedparam (pthread_self(), &policy, &sparam);
	  sparam.sched_priority--;
	  pthread_attr_setschedparam (&attr1, &sparam);
#ifdef DATA_CONCENTRATOR
	}
#endif

	  // Start consumer thread, which transmits data blocks
	  int err_no;
try_again:
	  if (err_no = pthread_create (&consumer_tid, &attr1, (void *(*)(void *)) consumer_static, (void *) this)) {
	    system_log(1, "Couldn't create net-writer consumer thread; pthread_create() err=%d", err_no);
	    if (err_no == EAGAIN || err_no == ENOMEM) {
		sleep(5);
		goto try_again;
	    }

	    // FIXME: must cancel producer thread somehow.
	    abort();

	    source_buffptr -> delete_consumer (cnum);
	    disconnect_srvr_addr ();
	    buffptr -> ~circ_buffer();
	    free ((void *) buffptr);
	    buffptr = 0;
	    pthread_attr_destroy (&attr1);
	    return DAQD_THREAD_CREATE;
	  }
	  pthread_attr_destroy (&attr1);
	  daqd.net_writers.insert_first (this);
	  DEBUG1(cerr << "net writer consumer thread started; tid=" << consumer_tid << endl);
	}
      else // send data for the specified time period
	{
	  transient = 1;
	  time_t bstart = 0; time_t blast = 0; // timestamp of the first and last blocks fetched into the buffer
	  
	  // very strange logic is in here..
	  offline.seconds_per_sample = src_buffer -> block_period ();

	  if (no_online || no_consumer_buffer) { // Do not mark online data buffer blocks
	    if (daqd.b1 && !delta) {
	      // Figure out current GPS time and set block indices
	      bstart = blast = daqd.b1->gps();
	    }
	  } else  {
	    // Copy data from the main buffer into the net-writer buffer
	    // All data available in memory will be copied

	    // FIXME: circular buffer must be allocated after we know how many blocks (if any) were marked
	    // right now, every request to get off-line data will do malloc for 17 blocks for the net-writer
	    // circular buffer and then just deletes the buffer.
	    
	    if ((cnum = source_buffptr -> add_transient_consumer (gps, delta, &bstart, &blast)) >= 0)
	      {
		// Some blocks were marked, so need to start producer thread to read
		// those blocks and transfer them into the `*buffptr' circular buffer

		pthread_attr_t attr;
		pthread_attr_init (&attr);
		pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
		pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
		pthread_attr_setstacksize (&attr, daqd.thread_stack_size);
		int err_no;
		if (err_no = pthread_create (&producer_tid, &attr, (void *(*)(void *)) transient_producer_static, (void *) this)) {
		  system_log(1, "couldn't create transient net-writer producer thread; pthread_create() err=%d", err_no);
		  source_buffptr -> delete_consumer (cnum);
		  cnum = 0;
		  disconnect_srvr_addr ();
		  pthread_attr_destroy (&attr);
		  buffptr -> ~circ_buffer();
		  free ((void *) buffptr);
		  buffptr = 0;
		  return DAQD_THREAD_CREATE;
		}
		pthread_attr_destroy (&attr);

		DEBUG1(cerr << "network writer created; tid=" << producer_tid << endl);
	      }
	    else // No blocks were marked
	      {
		buffptr -> ~circ_buffer();
		free ((void *) buffptr);
		buffptr = 0;

		if (cnum > -2) 
		  {
		    system_log(1, "couldn't create transient net-writer; too many consumers");
		    disconnect_srvr_addr ();
		    return DAQD_BUSY;
		  }
		else
		  {
		    if (cnum > -3)
		      {
			// No suitable data found in the main buffer
			if (! delta) 
			  {
			    // This the request "to get data up to now"; and no data
			    // found in the main circular buffer
		      
			    disconnect_srvr_addr ();
			    return DAQD_NOT_FOUND;
			  }
			else
			  {
			    // No data available from files if the map is empty
			    int raw_minute_data_kludge = source_buffptr == daqd.trender.mtb || daqd.fsd.gps_time_dirs;
			    if (fsd -> is_empty () && !raw_minute_data_kludge)
			      {
				disconnect_srvr_addr ();
				return DAQD_NOT_FOUND;
			      }
		      
			    // Set the timestamps to indicate that no data is available in the circ buffer
			    bstart = blast = 0;
			  }
		      }
		    else
		      {
			DEBUG1(cerr << "future date given, come again" << endl);
			disconnect_srvr_addr ();
			return DAQD_NOT_FOUND;
		      }
		  }
	      }
	  }

	  offline.gps = gps;
	  offline.delta = delta;
	  offline.bstart = bstart;
	  offline.blast = blast;
	  offline.no_time_check = no_online;

	  if (daqd.fsd.gps_time_dirs) 
		offline.no_time_check = 1;

	  DEBUG1(cerr << "Right before spawning transient consumer: gps=" << gps << " delta=" << delta << endl);
      
	  // Start consumer thread
	  // This consumer reads frame files and sends data to the client.
	  // Sends data from the net-writer buffer after that (if any).

	  pthread_attr_t attr1;
	  pthread_attr_init (&attr1);
          pthread_attr_setscope(&attr1, PTHREAD_SCOPE_SYSTEM);
	  pthread_attr_setdetachstate (&attr1, PTHREAD_CREATE_DETACHED);

	  // Lower this thread priority
	  struct sched_param sparam;
	  int policy;
	  pthread_getschedparam (pthread_self(), &policy, &sparam);
	  sparam.sched_priority--;
	  pthread_attr_setschedparam (&attr1, &sparam);

	  // FIXME
	  int err_no;
	  if (err_no = pthread_create (&consumer_tid, &attr1, (void *(*)(void *)) transient_consumer_static, (void *) this)) {
	    system_log(1, "FIXME:producer thread must be cancelled. Couldn't create transient net-writer consumer thread; pthread_create() err=%d", err_no);
	    abort();

	    if (cnum) {
	      source_buffptr -> delete_consumer (cnum);
	      cnum = 0;
	    }
	    pthread_attr_destroy (&attr1);	    
	    buffptr -> ~circ_buffer();
	    free ((void *) buffptr);
	    buffptr = 0;
	    return DAQD_THREAD_CREATE;
	  }
	  pthread_attr_destroy (&attr1);

	  daqd.net_writers.insert_first (this);

	  DEBUG1(cerr << "net writer consumer thread started; tid=" << consumer_tid << endl);
	}
    }

  // If there is no data connection, then it is our responsibility to
  // send out control connection ACK and net-writer ID before any data is transmitted

// :TODO: can't keep using a pointer as network writer ID on 64-bit
// :TODO: give each net-writer a sequential ID
  if (no_data_connection)
    {
      *yyout << S_DAQD_OK << flush;
      // send writer ID
      *yyout << setfill ('0') << setw (sizeof (unsigned int) * 2) << hex
	     << (unsigned int) 0 << dec << flush;
    }

  // Return will release the lock allowing consumer to proceed
  return 0;
}


int
net_writer_c::kill_net_writer ()
{
  locker mon (this);
  char buf [1];
  circ_buffer *oldb;

  //  if (! buffptr)
  //    return DAQD_NO_WRITER;

  /* Put zero-length block in the queue
     and wait till the saver is done */
  //  buffptr -> put_nowait (buf, 0);
  //  pthread_join (consumer_tid, NULL);
  //  pthread_join (producer_tid, NULL);

  /* Well, just putting zero-length block into the buffer
     is not reliable, since the block can be dropped ...
     Let's just clse the socket */

  //  close (fileno);
  //  disconnect_srvr_addr ();

  // shutdown() works much better than close() in here: is doesn't block
  int res = shutdown (fileno, 2);
  return res? DAQD_SHUTDOWN: DAQD_OK;
}

void
net_writer_c::shutdown_net_writer () {
  locker mon (this);
#ifdef GDS_TESTPOINTS
  if (clear_testpoints) {
    int na = 0;
    //char *alias[daqd.max_channels];
    long_channel_t *tps[num_channels];

    //system_log(1, "net-writer shutdown clearing test points");
    for (int i = 0; i < num_channels; i++)
      if (IS_GDS_ALIAS(channels [i])) {
        //alias [na++] = channels [i].name;
	tps[na++] = channels + i;
        //system_log(1, "%s", channels[i].name);
      }

    if (na) {
      //int res = daqd.gds.clear_names (alias, na);
      int res = daqd.gds.clear_tps (tps, na);
      if (res) {
        system_log(1, "GDS testpoint clear failed");
      }
    }
    clear_testpoints = 0;
  }
#endif
  disconnect_srvr_addr ();
  shutdown_now = 1;

#ifndef NO_BROADCAST
// Do not close the broadcaster stuff
// This seems to cause segfaults on fb0
// due to broadcast variable corruption by unknown causes.
#if 0
  if (broadcast) {
    radio.close ();
  }
#endif
#endif
}

  // Send some data to the client
int
net_writer_c::send_to_client (char *data, unsigned long len, unsigned long gps, int period, int tp) {
#ifndef NO_BROADCAST
    if (broadcast) {
      // look for a free data buffer
      radio_buffer*        buf = 0;

#ifndef DATA_CONCENTRATOR
      if (len <= (sizeof(int)*5)) {
	return 0;
      } else 
#endif
	for (radio_buffer* b = radio_bufs;
	     b < radio_bufs + radio_buf_num; b++) {
	  if (!radio.isUsed(b->inUse)) {
	    buf = b;
	    break;
	  }
	}

      if (buf == 0) {
	system_log(1, "cannot find unused radio buffer; frame was lost");
	abort();
	return 0;
      }

      if (len > radio_buf_len) {
        system_log(1, "radio buffers too small");
        this -> shutdown_net_writer ();
        return -1;
      }


      memcpy (buf -> data, data, len);

#ifdef DATA_CONCENTRATOR
      if (tp) {
        // send data buffer
        radio_tp.send (buf -> data, len, &buf -> inUse, false, gps, period);
        DEBUG1(cerr << "tp framexmit write gps=" << gps << " period=" << period << endl);
      } else 
#endif
      {
        // send data buffer
        radio.send (buf -> data, len, &buf -> inUse, false, gps, period);
        DEBUG1(cerr << "framexmit write gps=" << gps << " period=" << period << endl);
      }

    } else 
#endif

    if (basic_io::writen (fileno, (char *) data, len) != len)
      {
	// Write failed -- shutdown this thread and destroy this circular buffer
	DEBUG1(cerr << "net_writer_c::consumer(): write(" << fileno << ") failed; errno=" << errno << endl);
	this -> shutdown_net_writer ();
	return -1;
      }
    return 0;
}
