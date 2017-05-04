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

int printUntil(int fd, int gpsEnd, int lastReadGPS, raw_trend_disk_record_struct *sd) {
  int nread = 0;

  do {
    if (sd->gps <= lastReadGPS) {
	fprintf(stderr, "Skipping record: time goes backwards: %d -> %d\n", sd->gps, lastReadGPS);
    } else if ((lastReadGPS - sd->gps) %60 != 0) {
	fprintf(stderr, "Skipping record: time increased by an interval != 60seconds: %d\n", lastReadGPS - sd->gps);
    } else {
	//fprintf(stderr, "Wrote data at %d\n", sd->gps);
	fwrite(sd, RAW_TREND_REC_SIZE, 1, stdout);
	lastReadGPS = sd->gps;
    }

    nread = read (fd, sd, RAW_TREND_REC_SIZE);
  } while (sd->gps < gpsEnd && nread != 0);

  return nread;
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


  if (argc != 6) {
    fprintf(stderr, "Usage:  %s <channel> <path1> <path2> <gpsStart> <gpsEnd>\n", argv[0]);
    exit(1);
  }

  string channel = argv[1];
  string path = argv[2];
  string pathfiller = argv[3];
  int gpsstart = atoi(argv[4]);
  int gpsend = atoi(argv[5]);

  file_set.insert(channel);

  // For every file construct a list of data spans (index, gps, length), `data_span_map'
  // Each file contains data with gaps. This we end up with the list of data spans.
  for (SSI p = file_set.begin (); p != file_set.end (); p++) {
    //string fname_str = path + "/" + crc8_str(p->c_str()) + "/" +*p;
    string fname_str = path + "/" + *p;
    const char *fname = fname_str.c_str ();
    int fd = open (fname, O_RDONLY);

    string fillername_str = pathfiller + "/" + crc8_str(p->c_str()) + "/" +*p;
    const char *fillername = fillername_str.c_str ();
    int fdfiller = open (fillername, O_RDONLY);

    if (fd < 0) {
      system_log(1, "Couldn't open raw minute trend file `%s' for reading; errno %d", fname, errno);
    } else if (fdfiller < 0) {
      system_log(1, "Couldn't open raw minute trend file `%s' for reading; errno %d", fillername, errno);
    } else {
      struct stat st;
      struct stat stfiller;
      int res = fstat (fd, &st);
      int resfiller = fstat (fdfiller, &stfiller);
      if (res) {
	system_log(1, "Couldn't stat raw minute trend file `%s'; errno %d", fname, errno);
      } else if (resfiller) {
	system_log(1, "Couldn't stat raw minute trend file `%s'; errno %d", fillername, errno);
      } else {
	int structs = st.st_size / RAW_TREND_REC_SIZE;
	int structsfiller = stfiller.st_size / RAW_TREND_REC_SIZE;
        raw_trend_disk_record_struct sd;
        raw_trend_disk_record_struct sdfiller;
        DEBUG1(cerr << fname << ": length " << st.st_size << "; structs " <<  structs << endl);
        if (st.st_size%RAW_TREND_REC_SIZE) {
	  system_log (1, "WARNING: filesize of %s isn't multiple of %d (record size)\n", fname, RAW_TREND_REC_SIZE);
	  exit(1);
	} else if (stfiller.st_size%RAW_TREND_REC_SIZE) {
	  system_log (1, "WARNING: filesize of %s isn't multiple of %d (record size)\n", fillername, RAW_TREND_REC_SIZE);
	  exit(1);
	} else {
	  int nread = 0;
	  int nreadfiller = 0;
	  int endFound = 0;
	  int lastReadGPS = 0;
	  nread = read (fd, &sd, RAW_TREND_REC_SIZE);
	  nreadfiller = read (fdfiller, &sdfiller, RAW_TREND_REC_SIZE);

	  fprintf(stderr, "Start of src: %d\n", sd.gps);
	  fprintf(stderr, "Start of filler: %d\n", sdfiller.gps);

	  while (!endFound) {
	    if (nread == 0) {
	      /*
	       * EOF on source file.  Continue filling in data from 
	       * the filler file.
	       */
	      fprintf(stderr, "EOF on input file at %d.  Completing with filler file.\n", sd.gps);
	      nreadfiller = printUntil(fdfiller, gpsend, lastReadGPS, &sdfiller);
	      lastReadGPS = sdfiller.gps;
	    } else if (nreadfiller == 0) {
	      /*
	       * EOF on filler file.  Continue filling in data from 
	       * the source file.
	       */
	      fprintf(stderr, "EOF on filler file at %d.  Completing with input file.\n", sdfiller.gps);
	      nread = printUntil(fd, gpsend, lastReadGPS, &sd);
	      lastReadGPS = sd.gps;
	    } else if (sd.gps == sdfiller.gps) {
	      if (sd.gps - lastReadGPS > 60) {
		fprintf(stderr, "Unfillable gap from %d to %d (%d s)\n", lastReadGPS, sd.gps, sd.gps - lastReadGPS);
	      }
	      if (sd.tb.n > sdfiller.tb.n) {
		fprintf(stderr, "Filled partial trend from src (%d vs. %d) at %d\n", sd.tb.n, sdfiller.tb.n, sdfiller.gps);
		fwrite(&sd, RAW_TREND_REC_SIZE, 1, stdout);
		lastReadGPS = sd.gps;
	      } else if (sd.tb.n < sdfiller.tb.n) {
		fprintf(stderr, "Filled partial trend from filler (%d vs. %d) at %d\n", sd.tb.n, sdfiller.tb.n, sdfiller.gps);
		fwrite(&sdfiller, RAW_TREND_REC_SIZE, 1, stdout);
		lastReadGPS = sdfiller.gps;
	      } else {
		//fprintf(stderr, "Wrote data at %d\n", sd.gps);
		fwrite(&sd, RAW_TREND_REC_SIZE, 1, stdout);
		lastReadGPS = sd.gps;
	      }
	      nread = read (fd, &sd, RAW_TREND_REC_SIZE);
	      nreadfiller = read (fdfiller, &sdfiller, RAW_TREND_REC_SIZE);
	    } else if (sd.gps < sdfiller.gps) {
	      fprintf(stderr, "Filling reverse gap from %d to %d (%d s)\n", sd.gps, sdfiller.gps, sdfiller.gps - sd.gps);
	      nread = printUntil(fd, sdfiller.gps, lastReadGPS, &sd);
	      lastReadGPS = sd.gps;
	    } else {
	      fprintf(stderr, "Filling gap from %d to %d (%d s)\n", sdfiller.gps, sd.gps, sd.gps - sdfiller.gps);
	      nreadfiller = printUntil(fdfiller, sd.gps, lastReadGPS, &sdfiller);
	      lastReadGPS = sdfiller.gps;
	    }

	    if ((nread == 0 || sd.gps > gpsend) && (nreadfiller == 0 || sdfiller.gps > gpsend)) {
	      if (nread == 0) {
		fprintf(stderr, "EOF on input file at %d.\n", sd.gps);
	      }
	      if (nreadfiller == 0) {
		fprintf(stderr, "EOF on filler file at %d.\n", sdfiller.gps);
	      }
	      if (sd.gps > gpsend) {
		fprintf(stderr, "GPS End reached on src file at %d.\n", sd.gps);
	      }
	      if (sdfiller.gps > gpsend) {
		fprintf(stderr, "GPS End reached on filler file at %d.\n", sdfiller.gps);
	      }
	      endFound = 1;
	    }
	  }
	}
      }
      close (fd);
      close (fdfiller);
    } 
  }

  exit(0);
}

