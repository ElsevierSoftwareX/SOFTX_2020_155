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
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <list>
#include <set>
#include <map>
#include <string>

#include "nds.hh"
#include "io.h"
#include "daqd_net.hh"
#include "../daqd/crc8.cc"

using namespace CDS_NDS;
using namespace std;

int nds_log_level=4; // Controls volume of log messages

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

typedef struct raw_trend_disk_record_struct {
  unsigned int gps;
  trend_block_on_disk_t tb;
} raw_trend_disk_record_struct;

// Fixed record size to be independent of 32/64 bit difference
// struct raw_trend_record_struct does not represent the layout of data on disk
// only in memory.
#define RAW_TREND_REC_SIZE 40


static unsigned long
find_offs (unsigned int gps, int fd, unsigned long start, unsigned long structs)
{
        lseek (fd, RAW_TREND_REC_SIZE * (start + structs/2), SEEK_SET);
        raw_trend_record_struct ss;
        int nread = read (fd, &ss, RAW_TREND_REC_SIZE);
	//if (sizeof(raw_trend_record_struct) == 48) {
	// we must be on the 64-bit computer
	//}
	// Do not need to shuffle the data since ss.gps is the first 4 bytes always
        if (1 == structs) {
                if (gps < ss.gps + 30)
                        return start;
                else
                        return start + 1;
        }

        int begin;
        if (gps > ss.gps)
                begin = start + structs/2;
        else
                begin = start;
        DEBUG1(cerr << "recursion: " << gps << " " << ss.gps << " " << begin << " " <<  (structs+1)/2 << endl);
        return find_offs (gps, fd, begin, (structs+1)/2);
}

inline void
byteswap(char *image_ptr, int chb)
{
	char a[8];
	switch(chb) {
		case 2:
			a[0] = *image_ptr;
			*image_ptr = image_ptr[1];
			image_ptr[1] = a[0];
			break;
		case 4:
			a[0] = image_ptr[0];
			a[1] = image_ptr[1];
			a[2] = image_ptr[2];
			a[3] = image_ptr[3];
			image_ptr[0] = a[3];
			image_ptr[1] = a[2];
			image_ptr[2] = a[1];
			image_ptr[3] = a[0];
			break;
		case 8:
			a[0] = image_ptr[0];
			a[1] = image_ptr[1];
			a[2] = image_ptr[2];
			a[3] = image_ptr[3];
			a[4] = image_ptr[4];
			a[5] = image_ptr[5];
			a[6] = image_ptr[6];
			a[7] = image_ptr[7];
			image_ptr[0] = a[7];
			image_ptr[1] = a[6];
			image_ptr[2] = a[5];
			image_ptr[3] = a[4];
			image_ptr[4] = a[3];
			image_ptr[5] = a[2];
			image_ptr[6] = a[1];
			image_ptr[7] = a[0];
			break;
	}
}

void print_usage(const char *progname) {
    fprintf(stderr, "Usage:  %s [-g] [-v] [-e] [-s] [-f] [-m maxgap] <filename>\n", progname);
    fprintf(stderr, "        -g  Print each gap\n");
    fprintf(stderr, "        -v  Print each data record\n");
    fprintf(stderr, "        -e  Exit the first time an error is found\n");
    fprintf(stderr, "        -s  Do not print the summary of gaps and time spans\n");
    fprintf(stderr, "        -f  Purge invalid data and write the result to stdout\n");
    fprintf(stderr, "        -m  maxgap Maximum allowed size of a gap.\n");
}

int main (int argc, char **argv) {
  multimap<string, int> fname_channel_mmap; // file_name -> index of the signal in the user request
  typedef multimap<string, int>::const_iterator FCMI;
  set<string> file_set; // to keep file names
  map<string, list<data_span> > data_span_map; // file_name -> list of data spans
  typedef map<string, list<data_span> >::const_iterator DSMI;
  typedef map<string, list<data_span> >::iterator NCDSMI;
  typedef list<data_span>::const_iterator DSI;
  typedef list<data_span>::iterator NCDSI;
  typedef list<mapping_data_span>::const_iterator MDSI;
  typedef list<mapping_data_span>::iterator NCMDSI;
  typedef set<string>::const_iterator SSI;

  int opt;
  int verbose = 0;
  int exit_on_error = 0;
  int print_gaps = 0;
  int print_summary = 1;
  int fix_file = 0;
  int max_gap = 0;

  while ((opt = getopt(argc, argv, "sgvefm:")) != -1) {
    switch (opt) {
      case 'v':
	verbose++;
	break;
      case 'g':
	print_gaps++;
	break;
      case 's':
	print_summary = 0;
	break;
      case 'e':
	exit_on_error++;
	break;
      case 'f':
	fix_file++;
	break;
      case 'm':
	max_gap=atoi(optarg);
	break;
      default:
	print_usage(argv[0]);
	exit(1);
    }
  }

  if (optind >= argc) {
    print_usage(argv[0]);
    exit(1);
  }

  int errorFound = 0;
  string channel = argv[optind];
  file_set.insert(channel);

  // For every file construct a list of data spans (index, gps, length), `data_span_map'
  // Each file contains data with gaps. This we end up with the list of data spans.
  for (SSI p = file_set.begin (); p != file_set.end (); p++) {
    string fname_str = *p;
    const char *fname = fname_str.c_str ();
    int fd = open (fname, O_RDONLY);
    if (fd < 0) {
      system_log(1, "Couldn't open raw minute trend file `%s' for reading; errno %d", fname, errno);
    } else {
      struct stat st;
      int res = fstat (fd, &st);
      if (res) {
	system_log(1, "Couldn't stat raw minute trend file `%s'; errno %d", fname, errno);
      } else {
	int structs = st.st_size / RAW_TREND_REC_SIZE;
        raw_trend_disk_record_struct sd;
        DEBUG1(cerr << fname << ": length " << st.st_size << "; structs " <<  structs << endl);
        if (st.st_size%RAW_TREND_REC_SIZE) {
	  system_log (1, "WARNING: filesize of %s isn't multiple of %d (record size)\n", fname, RAW_TREND_REC_SIZE);
	}

	int lastRecordGPS = 0;
	lseek (fd, -RAW_TREND_REC_SIZE, SEEK_END);
	int nread = read (fd, &sd, RAW_TREND_REC_SIZE);
	if (nread != 0) {
	    lastRecordGPS = sd.gps;
	}
	lseek (fd, 0, SEEK_SET);

	int startGPS = 0;
	int numRecords = 0;
	int numGaps = 0;
	int gapSpan = 0;
	int lastGPS = 0;
	int longestGap = 0;
	nread = read (fd, &sd, RAW_TREND_REC_SIZE);
	while (nread != 0) {
	  numRecords++;
	  if (lastGPS == 0) {
	    startGPS = sd.gps;
	    if (startGPS > lastRecordGPS) {
		fprintf(stderr, "Timestamp of final record is less than timestamp of first record: %d vs. %d\n", startGPS, lastRecordGPS);
		if (exit_on_error) {
		    exit (1);
		}
	    }
	  }

	  if (sd.gps % 60 != 0) {
	    fprintf(stderr, "Record %d does not start at a 60-second boundary: %d (off by %d)\n", numRecords, sd.gps, sd.gps%60);
	    errorFound++;
	    if (exit_on_error) {
	      exit (1);
	    }
	  } else if (sd.gps < startGPS || sd.gps > lastRecordGPS) {
	    fprintf(stderr, "Record %d has a timestamp outside the bounds of the file: %d not within %d -> %d\n", numRecords, sd.gps, startGPS, lastRecordGPS);
	    errorFound++;
	    if (exit_on_error) {
	      exit (1);
	    }
	  } else if (lastGPS > 0 && sd.gps < lastGPS) {
	    fprintf(stderr, "Time goes backwards at record %d: %d -> %d\n", numRecords, lastGPS, sd.gps);
	    errorFound++;
	    if (exit_on_error) {
	      exit (1);
	    }
	  } else if (lastGPS > 0 && max_gap > 0 && sd.gps - lastGPS > max_gap) {
	    fprintf(stderr, "Gap found between %d and %d lasting %d s, exceeded max allowed gap size of %d.\n", lastGPS, sd.gps, sd.gps - lastGPS, max_gap);
	    if (sd.gps - lastGPS > longestGap) {
		longestGap = sd.gps = lastGPS;
	    }
	    errorFound++;
	    if (exit_on_error) {
		exit (1);
	    }
	  } else if (lastGPS > 0 && sd.gps - lastGPS > 60) {
	    if (print_gaps >= 1) {
		fprintf(stderr, "Gap found between %d and %d lasting %d s\n", lastGPS, sd.gps, sd.gps - lastGPS);
	    }
	    if (sd.gps - lastGPS > longestGap) {
		longestGap = sd.gps = lastGPS;
	    }
	    numGaps++;
	    gapSpan += sd.gps - lastGPS;
	    lastGPS = sd.gps;
	    if (fix_file && sd.gps - lastGPS <= max_gap) {
		fwrite(&sd, RAW_TREND_REC_SIZE, 1, stdout);
	    }
	  } else if (lastGPS > 0 && sd.gps - lastGPS != 60) {
	    fprintf(stderr, "Bad gap size found at record %d between %d and %d lasting %d s\n", numRecords, lastGPS, sd.gps, sd.gps - lastGPS);
	    errorFound++;
	    if (exit_on_error) {
	      exit (1);
	    }
	    lastGPS = sd.gps;
	  } else {
	    if (fix_file) {
		fwrite(&sd, RAW_TREND_REC_SIZE, 1, stdout);
	    }
	    lastGPS = sd.gps;
	  }

	  if (verbose) {
	    fprintf(stderr, "%d: %d min:%d+%d max:%d+%d rms:%d+%d mean:%d+%d n:%d\n", numRecords, sd.gps, sd.tb.min, sd.tb.min2, sd.tb.max, sd.tb.max2, sd.tb.rms, sd.tb.rms2, sd.tb.mean, sd.tb.mean2, sd.tb.n);
	  }
	  nread = read (fd, &sd, RAW_TREND_REC_SIZE);
	}
	if (print_summary) {
	  fprintf(stderr, "%d records with %d gaps spanning %d seconds found\n", numRecords, numGaps, gapSpan);
	  fprintf(stderr, "Max gap length is %d\n", longestGap);
	  fprintf(stderr, "GPS time range: %d to %d\n", startGPS, lastGPS);
	}
      }
      close (fd);
    } 
  }

  if (errorFound) {
    exit(1);
  }
  exit(0);
}

