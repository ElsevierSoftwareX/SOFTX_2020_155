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

// Find and send requested minute trend data to the user
bool
Nds::rawMinuteTrend(string path)
{
  // Send reconfiguration block ( this is current calibration values passed by the daqd )
  {
    daqd_net daqd_net(mDataFd, mSpec);
    daqd_net.send_reconfig_data(mSpec);
  }


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
  const vector<string> &signals = mSpec.getSignalNames();
  int num_channels = signals.size();

#ifdef not_def
  // See if result file can be opened
  ofstream out(mResultFileName.c_str());
  if (! out ) {
    system_log(1, "%s: result file open failed", mResultFileName.c_str());
    return false;
  }
#endif

  // Construct a multimap of file names into the signal number(s) `fname_channel_mmap'
  // Here we strip the suffix from the channel name in order to get the raw minute
  // trend file name. If there is no suffix (or rather no dot separator) we do not
  // continue and return an error.
  // Also construct a set of file name we will be working with.
  for (int i = 0 ; i < num_channels; i++) {
    int idx = signals[i].find_first_of('.');
    if (idx == string::npos) {
      system_log(1, "bad signal name: `%s'", signals[i].c_str());
      return false;
    }
    string name (signals [i].substr(0,idx));
    fname_channel_mmap.insert (pair<string, int> (name, i));
    file_set.insert (name);
    DEBUG(1, cerr << "multimap construction: " << name << endl);
  }

  unsigned int gps = mSpec.getStartGpsTime();
  unsigned int end_gps = mSpec.getEndGpsTime();

  // For every file construct a list of data spans (index, gps, length), `data_span_map'
  // Each file contains data with gaps. This we end up with the list of data spans.
  for (SSI p = file_set.begin (); p != file_set.end (); p++) {
    string fname_str = path + "/" + crc8_str(p->c_str()) + "/" +*p;
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
        DEBUG1(cerr << fname << ": length " << st.st_size << "; structs " <<  structs << endl);
        if (st.st_size%RAW_TREND_REC_SIZE)
	  system_log (1, "WARNING: filesize of %s isn't multiple of %d (record size)\n", fname, RAW_TREND_REC_SIZE);

	// index search
	unsigned long offs = find_offs (gps, fd, 0, structs);
	DEBUG1(cerr << "find_offs() returned " << offs << endl);

	// Sort out the cases when no data is available
	if (offs == structs) { // End of file hit -- no data available for the current channel
	  ;
	} else {
	  // memory map and do linear scan until the end is reached, 
	  // constructing a list<data_span>
	  
	  // It should be possible to do a binary search for all the gaps too.

          lseek (fd, 0, SEEK_SET);
	  char *mmap_ptr = (char *) mmap (0, st.st_size, PROT_READ , MAP_SHARED, fd, 0);
	  
	  if (mmap_ptr == MAP_FAILED) {
	    /* QFS on Solaris 9 requires EXEC permission set */
	    mmap_ptr = (char *) mmap (0, st.st_size, PROT_READ | PROT_EXEC, MAP_SHARED, fd, 0);
	  }
	  if (mmap_ptr == MAP_FAILED) {
	    system_log(1, "mmap(0, %d, %d, %d, %d, 0) call failed, errno %d\n", (int)st.st_size, PROT_READ, MAP_SHARED, fd, errno);
	  } else {
	    raw_trend_record_struct *end_ptr = (raw_trend_record_struct *) (mmap_ptr + st.st_size);
	    list<data_span> dvec;
	    data_span ds;
	    ds.gps=0;
	    long nreads = 0;
	    for (raw_trend_record_struct *ss_ptr
		   = (raw_trend_record_struct *) (mmap_ptr + offs * RAW_TREND_REC_SIZE);
		 ;
		 ss_ptr = (raw_trend_record_struct *) (((long)ss_ptr) +  RAW_TREND_REC_SIZE))
	      {
		int eof = 0;
		if (ss_ptr >= end_ptr) // end by file size
		  eof = 1;
		else {
#if 0
	// This check caused some grief at Hannover on the PSL system
	//
		  // Check for garbage data point
		  // If this point is more than one month in the future,
		  // skip it
		  if (dvec.size ()
		      && ((2592000 + dvec.back().end_gps ()) < ss_ptr -> gps))
		  {
#ifndef NDEBUG
		    cerr << "Skipped bad data point gps=" << ss_ptr -> gps
		         << " at offs=" << ((char *) ss_ptr - (char *) mmap_ptr)
			 << endl;
#endif
		    continue;
		  }
#endif

		  if (ss_ptr -> gps >= end_gps) // end by time
		    eof = 1;
	        }

		nreads++;

		if (!ds.gps && !eof) {
		  // first span
		  ds.gps = ss_ptr -> gps;
		  ds.offs = ((char *) ss_ptr - mmap_ptr) / RAW_TREND_REC_SIZE;
		  ds.length = 1;
		} else {
		  // check if there is a gap
		  // limit `length' to 60 points, this is important for the data viewer
		  // if (ss_ptr -> gps != ds.end_gps () || ds.length == 60) 
		  bool neqf = 0;
	          if (!eof) neqf = ss_ptr -> gps != ds.end_gps ();
		  if (eof || neqf) {
		    // See if the span doesn't overlap with the previous
		    // data span.
		    if (dvec.size () && dvec.back().end_gps () > ds.gps) {
#ifndef NDEBUG
	cerr << "Conflict:" << endl;
	cerr << dvec.back().gps << "\t" << dvec.back().length << "\t" << dvec.back().offs << endl;
	cerr << ds.gps << "\t" << ds.length << "\t" << ds.offs << endl;
#endif
			int diff = (dvec.back().end_gps () - ds.gps) / 60;
			diff ++;
			if (ds.length <= diff) {
			// remove this span
			ds.gps = 0;
#ifndef NDEBUG
	cerr << "span deleted" << endl;
#endif
			} else {
			// amend span
			ds.length -= diff;
			ds.gps += diff * 60;
			ds.offs += diff;
#ifndef NDEBUG
      cerr << "Amended span" << endl;
      cerr << ds.gps << "\t" << ds.length << "\t" << ds.offs << endl;
#endif
			}
		    }
		    if (ds.gps)
		       dvec.insert (dvec.end(), ds); // check in completed span
		    if (eof)
			break;

		    // start new span
		    ds.gps = ss_ptr -> gps;
		    ds.offs = ((char *) ss_ptr - mmap_ptr) / RAW_TREND_REC_SIZE;
		    ds.length = 1;
		  } else {
		    ds.length++; // continue current span
		  }
		}
	      }
          DEBUG1(cerr << "number of reads = " << nreads << endl);
	    munmap (mmap_ptr, st.st_size);
	    if (dvec.size ())
	      data_span_map.insert (pair<string, list<data_span> > (*p, dvec)); // File name -> the list of data spans in the file
	  }
	}
      }
      close (fd);
    } 
  }

  // Bail out if no data is found, ie. if `data_span_map' is empty
  // TODO: need to send and indication to the client that no data was found
  if (data_span_map.empty ()) {
    DEBUG1(cerr << "data span map is empty -- bailing out" << endl);
    return true;
  }

  // File set will not be used any more
  file_set.clear ();

#ifndef	NDEBUG
  cerr << "Data span map:" << endl;
  // Print out span map for debugging
  for (DSMI p = data_span_map.begin (); p != data_span_map.end (); p++) {
    cerr << p -> first << endl;
    for (DSI q = p -> second.begin (); q != p -> second.end (); q++) {
      cerr << q -> gps << "\t" << q -> length << "\t" << q -> offs << endl;
    }
    cerr << endl;
  }
#endif

#ifdef not_def

  // Some spans may overlap, cause the data archived might be bad, corrupted
  // Namely the timestamps could be incorrect
  // Amend data span map to eliminate span overlaps
  for (NCDSMI p = data_span_map.begin (); p != data_span_map.end (); p++) {
    cerr << p -> first << endl;
    data_span ds;
    for (NCDSI q = p -> second.begin (); q != p -> second.end (); q++) {
      if (ds.end_gps () > q -> gps) {
#ifndef NDEBUG
	cerr << "Conflict:" << endl;
	cerr << ds.gps << "\t" << ds.length << "\t" << ds.offs << endl;
	cerr << q -> gps << "\t" << q -> length << "\t" << q -> offs << endl;
#endif
	int diff = (ds.end_gps () - q -> gps) / 60;
	diff ++;
	if (q -> length <= diff) {
		// remove this span
		p -> second.erase (q);
#ifndef NDEBUG
	cerr << "span deleted" << endl;
#endif
	} else {
		// amend span
		q -> length -= diff;
		q -> gps += diff * 60;
		q -> offs += diff;
#ifndef NDEBUG
      cerr << "Amended span" << endl;
      cerr << q -> gps << "\t" << q -> length << "\t" << q -> offs << endl;
#endif
		ds = *q;
	}	
      } else {
	ds = *q;
      }
    }
    cerr << endl;
  }

#ifndef	NDEBUG
  cerr << "Amended data span map:" << endl;
  for (DSMI p = data_span_map.begin (); p != data_span_map.end (); p++) {
    cerr << p -> first << endl;
    for (DSI q = p -> second.begin (); q != p -> second.end (); q++) {
       cerr << q -> gps << "\t" << q -> length << "\t" << q -> offs << endl;
    }
    cerr << endl;
  }
#endif

#endif // not_def

  // Construct a summary `summary_data_spans' list, representing a union
  // of all data spans for all files.
  // Each element will start a new data transmission block of period of `length*60' seconds.
  // `merge' operation on list is producing sorted list, since all source lists
  // are merged
  list<data_span> summary_data_spans;
  for (DSMI p = data_span_map.begin (); p != data_span_map.end (); p++) {
    list<data_span> dsl(p->second);
    summary_data_spans.merge (dsl);
  }

#ifndef	NDEBUG
  cerr << "Summary data spans:" << endl;
  // Print out span map for debugging
  for (DSI p = summary_data_spans.begin (); p != summary_data_spans.end (); p++) {
    cerr << p -> gps << "\t" << p -> length << "\t" << p -> offs << endl;
  }
#endif

  // Iterate over this merged list and merge the spans into the united spans.
  // This operation finds "real" holes in the data, i.e. holes common to all channels.
  // Data transmission protocol cannot send different holes for different channels.
  list<mapping_data_span> united_data_spans;
  mapping_data_span cur_span;
  cur_span.gps=0;
  for (DSI p = summary_data_spans.begin ();; p++) {

    if (p == summary_data_spans.end ()) {
      if (cur_span.gps)
	united_data_spans.insert (united_data_spans.end(), cur_span); // check the last one
      break;
    }

    if (!cur_span.gps)
      cur_span = *p;
    else if (p -> gps <= cur_span.end_gps ())
      cur_span.extend (*p);
    else {
      united_data_spans.insert (united_data_spans.end(), cur_span); // check in completed span
      cur_span = *p;
    }
  }  

  // No longer needed
  summary_data_spans.clear ();

#ifndef	NDEBUG
  cerr << "United data spans:" << endl;
  // Print out span map for debugging
  for (MDSI p = united_data_spans.begin (); p != united_data_spans.end (); p++) {
    cerr << p -> gps << "\t" << p -> length << "\t" << p -> offs << endl;
  }
#endif

  // Calculate final size.
  // FIXME

  unsigned long transmission_block_size = 0;
  for (int i = 0; i < mSpec.getSignalBps().size(); i++)
    transmission_block_size += mSpec.getSignalBps()[i];

  data_span s;
  s = for_each (united_data_spans.begin (), united_data_spans.end (), s);
  unsigned long image_size = s.length * transmission_block_size // data
    + 5 * sizeof (unsigned int) * united_data_spans.size (); // headers
  DEBUG1(cerr << "data size is " << s.length << " x " << transmission_block_size << endl);
  DEBUG1(cerr << "total image size is " << image_size << endl);

  // Move the 60 point subdivision code in here, apply it after the merge is done.???

  // Construct a transmission image
  if (image_size == 0)
    return true;

  char *block_image = (char *) malloc (image_size);
  if (!block_image) {
    system_log(1, "out of memory");
    return false;
  }
  memset (block_image, 0, image_size);

  // fill the headers in
  unsigned long offs = 0;
  for (NCMDSI p = united_data_spans.begin (); p != united_data_spans.end (); p++) {
    unsigned int header [5];
    header [0] = htonl (4 * sizeof (unsigned int) + p -> length * transmission_block_size);
    header [1] = htonl (p -> length * 60); // period in seconds, each point 60 seconds long
    header [2] = htonl (p -> gps);
    header [3] = htonl (0); // nanoseconds are assumed to be all zeroes for all the data stored
    header [4] = htonl (seq_num++);
    memcpy (block_image + offs, &header, 5*sizeof (unsigned int));
    offs += 5*sizeof (unsigned int);
    p -> image_offs = offs; // set the offset into the transmission image
    offs += p -> length * transmission_block_size;
  }

  // Iterate over the file names in `data_span_map'
  // Get channel numbers from `fname_channel_map'
  // copy the data  into the transmission image.
  for (DSMI p = data_span_map.begin (); p != data_span_map.end (); p++) {
    string fname_str = path + "/" + crc8_str(p->first.c_str()) + "/" + p->first;
    const char *fname = fname_str.c_str ();
    int fd = open (fname, O_RDONLY);
    if (fd < 0) {
      system_log(1, "Couldn't open raw minute trend file `%s' for reading {2}; errno %d", fname, errno);
    } else {
      struct stat st;
      int res = fstat (fd, &st);
      if (res) {
	system_log(1, "Couldn't stat raw minute trend file `%s' {2}; errno %d", fname, errno);
      } else {
	int structs = st.st_size / RAW_TREND_REC_SIZE;
        DEBUG1(cerr << fname << ": length " << st.st_size << "; structs " <<  structs << endl);
        if (st.st_size%RAW_TREND_REC_SIZE)
	  system_log(1, "Warning: filesize of %s isn't multiple of %d (record size) {2}\n", fname, RAW_TREND_REC_SIZE);
	char *mmap_ptr = (char *) mmap (0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (mmap_ptr < 0) {
	  system_log(1, "mmap() call failed {2}, errno %d\n", errno);
	} else {
	  // find out which channels are needed
	  int num_chnum = fname_channel_mmap.count(p -> first);
	  unsigned long ch_image_offset [num_chnum];
	  unsigned long ch_offset [num_chnum];
	  unsigned long ch_bytes [num_chnum];
	  //	  pair<FCMI, FCMI> g = fname_channel_mmap.equal_range(p -> first); -- didn't link on cdssol8 w/egcs-2.91.57
	  FCMI lb = fname_channel_mmap.lower_bound(p -> first);
	  FCMI ub = fname_channel_mmap.upper_bound(p -> first);

	  int i = 0;
	  DEBUG1(cerr << "Channel order:" << endl);
	  for (FCMI q = lb; q != ub; ++q, i++) {
	    ch_bytes [i] = mSpec.getSignalBps()[q -> second];
	    const char *sfx = strrchr (mSpec.getSignalNames()[q -> second].c_str(), '.');
	    if (sfx == NULL)
	      abort ();
	    sfx++;
	    raw_trend_record_struct junk;
	    Spec::DataTypeType data_type = mSpec.getSignalTypes()[q -> second];
	    if (!strcmp (sfx, "min")) {
#if 0
	      if (data_type == Spec::_64bit_double) {
		ch_offset [i] = (((char *) &junk.tb.min.D) - (char*) &junk);
	      } else if (data_type == Spec::_32bit_float) {
		ch_offset [i] = (((char *) &junk.tb.min.F) - (char*) &junk);
	      } else {
		ch_offset [i] = (((char *) &junk.tb.min.I) - (char*) &junk);
	      }
#endif
		ch_offset [i] = 4;
	    } else if (!strcmp (sfx, "max")) {
#if 0
	      if (data_type == Spec::_64bit_double) {
		ch_offset [i] = (((char *) &junk.tb.max.D) - (char*) &junk);
	      } else if (data_type == Spec::_32bit_float) {
		ch_offset [i] = (((char *) &junk.tb.max.F) - (char*) &junk);
	      } else {
		ch_offset [i] = (((char *) &junk.tb.max.I) - (char*) &junk);
	      }
#endif
		ch_offset [i] = 4 + 8;
	    } else if (!strcmp (sfx, "n")) {
	      //ch_offset [i] = (((char *) &junk.tb.n) - (char*) &junk);
		ch_offset [i] = 4 + 8 + 8;
	    } else if (!strcmp (sfx, "rms")) {
	      //ch_offset [i] = (((char *) &junk.tb.rms) - (char*) &junk);
		ch_offset [i] = 4 + 8 + 8 + 4;
	    } else if (!strcmp (sfx, "mean")) {
	      //ch_offset [i] = (((char *) &junk.tb.mean) - (char*) &junk);
		ch_offset [i] = 4 + 8 + 8 + 4 + 8; 
	    }
	    ch_image_offset [i] = 0;
	    for (int j = 0; j < q -> second; j++)
	      ch_image_offset [i] += mSpec.getSignalBps()[j];

	    DEBUG1(cerr << q -> second << "\t" << mSpec.getSignalNames()[q -> second] << " image_offset=" << ch_image_offset [i] << " ch_offset=" << ch_offset [i] << endl);
	  }

	  DSI dsi = p -> second.begin (); // spans of data needed from the current file
	  MDSI mdsi = united_data_spans.begin (); // spans of space in the `block_image'

	  // Copy the data over -- for all spans
	  for (;dsi != p -> second.end (); dsi++) {
	    // One data span `*dsi' fits fully into one and only one mapping data span `*mdsi'
	    // Find mapping data span
	    while (mdsi != united_data_spans.end() && dsi -> gps > mdsi -> end_gps ())
		mdsi++;

	    if (mdsi == united_data_spans.end()) {
		system_log(1, "mapping dat span not found, system_logic error");
		break;
	    }

//	    if (dsi -> gps < mdsi -> end_gps () && dsi -> gps >= mdsi -> gps) { // some data is in the current mapping span

#ifndef NDEBUG
	cerr << "dsi: " << dsi -> gps << '\t' << dsi -> length << '\t' << dsi -> offs << endl;
	cerr << "mdsi: " << mdsi -> gps << '\t' << mdsi -> length << '\t' << mdsi -> image_offs << endl;
	cerr << "num channels: " << num_chnum << endl;
#endif
	      for (int i = 0; i < num_chnum; i++) {
		  unsigned long npoints = dsi -> length;
		  char *image_ptr =
		      block_image + mdsi -> image_offs // data start within the block
		      + ch_image_offset [i] * mdsi -> length  // data start for the channel `i' within current mapping data span
		      + ch_bytes [i] * ((dsi -> gps - mdsi -> gps)/60); // skip to start of span
		  //char *src_ptr =
		      //((char *)(((raw_trend_record_struct *) mmap_ptr) + dsi -> offs)) + ch_offset [i];
		  char *src_ptr =
		      ((char *)(mmap_ptr + RAW_TREND_REC_SIZE * dsi -> offs)) + ch_offset [i];
		  int chb = ch_bytes [i];
#ifndef NDEBUG
        cerr << "image_ptr=" << image_ptr - block_image << " src_ptr=" << src_ptr - mmap_ptr << endl;
#endif
		  for(; npoints; --npoints) {
		      memcpy (image_ptr, src_ptr, chb);
#ifdef __linux__
		      byteswap(image_ptr, chb);
#endif
		      DEBUG(5, cerr <<" memcpy (" << (unsigned long) image_ptr << ", " 
			     << (unsigned long) src_ptr << ", " << chb << ")" << endl);
		      // move to next struct in the file
		      src_ptr += RAW_TREND_REC_SIZE;
		      // move to next point in the image
		      image_ptr += chb;
		  }
	      }

#ifdef not_def
	      if (dsi -> end_gps() == mdsi -> end_gps()) {
		// move to the next mapping data span
		mdsi++;
	      }
#endif
//	    }
	  }
	  munmap (mmap_ptr, st.st_size);
	}
      }
      close (fd);
    }
  }

  bool res = true;

#ifdef not_def
  // Save the image

  try {
    out.write(block_image, image_size);
  } catch (...) {
    system_log(1, "%s: write failed", mResultFileName.c_str());
    res = false;
  }
#endif

  // Write image to passed file descriptor
  int written = write(mDataFd, block_image, image_size);
  if (written != image_size) {
    system_log(1, "failed to write data into passed fd; errno=%d", errno);
    res = false;
  }

  free (block_image);

  // May need to do the subspans of `data_spans' individually, if the image size required is getting too big to handle
  
  return res;
}

