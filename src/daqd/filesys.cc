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

#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>
#include <string.h>
#include <dirent.h>

#include <unistd.h>
#include <iostream>

using namespace std;

#include "filesys.hh"
#include "daqd.hh"

extern daqd_c daqd;

#define WIPER_POSIX_SPAWN


/*
  Start a thread to delete all file in the directory `d'
*/
void
filesys_c::start_wiper (int d)
{
  // error message buffer
  char errmsgbuf[80]; 
  if (wiper_enabled) {
    pthread_t twiper;
    char *dir_path = (char *) malloc (filename_max);
    sprintf (dir_path, "%s%d/", path, d);

#ifdef WIPER_POSIX_SPAWN
    pthread_attr_t attr;
    pthread_attr_init (&attr);
    pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);

    // Lower wiper thread priority
    struct sched_param sparam;
    int policy;
    pthread_getschedparam (pthread_self(), &policy, &sparam);
    sparam.sched_priority--;
    pthread_attr_setschedparam (&attr, &sparam);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setstacksize (&attr, daqd.thread_stack_size);
    if (int err_no = pthread_create (&twiper, &attr, (void *(*)(void *))wiper, (void *) dir_path)) {
      strerror_r(err_no, errmsgbuf, sizeof(errmsgbuf));
      system_log(1, "couldn't create wiper thread; pthread_create() err=%s", errmsgbuf);
    } else {
      system_log(3, "wiper for %s created", dir_path);
    }
    pthread_attr_destroy (&attr);
#else
    thread_t tid;
    int err_no;
    if (err_no = thr_create (0,0, (void *(*)(void *))wiper, (void *) dir_path, THR_DETACHED | THR_NEW_LWP, &tid)) {
      strerror_r(err_no, errmsgbuf, sizeof(errmsgbuf));
      system_log(1, "couldn't create wiper thread; thr_create() err=%s", errmsgbuf);
    } else {
      system_log(3, "wiper for %s created", dir_path);
    }
#endif

  }
}

/*

  Update the map, saying that there is a file with the timestamp `gps' in the
  filename, which contains data for the `period' and the file is in the
  directory number `dnum' and this is the file with the latest data available.

  This function will update the data in `this->dir' array and could possibly
  create new data range in `this->dir->blist' linked list.  New data range
  becomes current range. Current range is the last range in the first element
  of the list.

  New range will be created only if putting (`gps', `period') pair in the map
  would discontinue current range or if there is no ranges yet.

  This function also cleans the next directory, when the `dnum' changes. Next directory
  is the one with number (dnum + 1) % num_dirs

*/

int
filesys_c::_update_dir (time_t gps, time_t gps_n, time_t period, int dnum)
{
  // If there is a circular buffer, put new filename in it
  if (cb) {
    circ_buffer_block_prop_t prop;
    prop.gps = gps;
    prop.gps_n = gps_n;

    char fname [filename_max];
    _filename_dir (gps, fname, dnum);
    cb -> put (fname, strlen (fname) + 1, &prop);
  }

  range_block *cur_range_block = (range_block *) (dir [dnum].blist.first ());

  if (cur_update_dir < 0)
    {
      cur_update_dir = dnum;

      // This is the first update for this object -- see if minimum times are not set
      if (! dir [dnum].min_time)
	dir [dnum].min_time = gps;
      if (! min_time)
	min_time = gps;
    }

  // Clean next directory if we just switched to the new directory and the number
  // of directories configured is more than one
  if (num_dirs > 1
      && cur_update_dir != dnum)
    {
      DEBUG(3, cerr << "filesys_c::_update_dir(): new directory; old=" << cur_update_dir << "; new=" << dnum << endl);

      cur_update_dir = dnum;
      dir [dnum].min_time = gps;

      // Cleanup next directory
      int next = (dnum + 1) % num_dirs;
      dir [next].nfiles = dir [next].min_time = dir [next].max_time = 0;

      // Delete all ranges
      for (s_link *clink = dir [next].blist.first ();
	   clink;
	   clink = dir [next].blist.first ())
	  {
	    dir [next].blist.remove (); // Remove range block from the list
	    // Freed already in remove() call above
	    //	    ((range_block *) clink) -> destroy ();
	  }

      // Delete all files in the next directory
      start_wiper (next);

      // See if yet next directory is not empty and update global minimum
      int next_next = (dnum + 2) % num_dirs;
      if (dir [next_next].min_time)
	min_time = dir [next_next].min_time;
    }
    
  if (! cur_range_block) // First range should be allocated
    {
      // Create a data range block and assign first range
      void *mptr = malloc (sizeof(range_block));
      if (!mptr) {
	system_log(1, "_update_dir(): memory exhausted");
	return -1;
      }
      cur_range_block = new (mptr) range_block (period, gps);

      // Put the first data range block in the linked list
      dir [dnum].blist.insert_first (cur_range_block);
    }
  else if (cur_range_block -> current_max_time () == gps)
    {
      // Increase the max time for the current range and we're done
      
      cur_range_block -> current_max_time_add (cur_range_block -> current_file_secs ());
    }
  else // New range ought to be started, because there is a gap in the available data
    {

      // See if a new range block should be allocated (current block is full)
      if (cur_range_block -> is_full ())
	{
	  // Create a data range block and assign first range
	  void *mptr = malloc (sizeof(range_block));
	  if (!mptr) {
	    system_log(1, "_update_dir(): memory exhausted");
	    return -1;
	  }
	  cur_range_block = new (mptr) range_block (period, gps);
	  
	  // Put the first data range block in the linked list
	  dir [dnum].blist.insert_first (cur_range_block);
	}
      else
	cur_range_block -> put (period, gps);
    }
  dir [dnum].nfiles++;
  max_time = dir [dnum].max_time = gps + period;
  frames_saved++;
  return 0;
}

/*
  Get frame file length in seconds
*/
inline int
filesys_c::frame_length_secs (time_t times, int dnum)
{
  return period;
}


// Time stamps for scanned files are stored here
typedef struct timestamp_time {
  time_t gps; // gps timestamp
  time_t dt; // frame file delta time
} timestamp_time;

static int
timestamp_time_compare (timestamp_time *i, timestamp_time *j)
{
  if (i->gps > j->gps)
    return (1);
  if (i->gps < j->gps)
    return (-1);
  return (0);
}

/*
  This scan will handle the changes in the file length (frame file length is
  in seconds) only if there is a gap in available data. So it will hnbdle the
  situations when we ran for a while, generated some data files (full frames
  or trend frames), then we were shutdown and restarted.
*/
int
filesys_c::scan ()
{
  //  locker mon (this);
  int global_max_dir = -1;
  time_t cur_tst = 0;
  time_t global_max = 0;
  time_t global_min = LONG_MAX;
  const int max_tstamps = 10000;
  timestamp_time tstamps [max_tstamps];

  // Clean the file map first
  // Delete all ranges in all dirs
  for (int i = 0; i < MAX_FRAME_DIRS; i++) {
    for (s_link *clink = dir [i].blist.first ();
	 clink;
	 clink = dir [i].blist.first ())
      {
	dir [i].blist.remove (); // Remove range block from the list
      }
  }


  system_log(3, "Scanning `%s'", path);

  for (int i = 0; i < num_dirs; i++) {
    DIR *dirp;
    struct dirent *direntp;
    char dirname [filename_max + 1];

    sprintf (dirname, "%s%d", path, i);

    int dir_nfiles = 0;
    time_t dir_max = 0;
    time_t dir_min = LONG_MAX;

    if (! (dirp = opendir (dirname)))
      {
	system_log(1, "Couldn't open directory `%s'", dirname);
	continue;
      }

    DEBUG(12, cerr << "pathconf of PC_NAME_MAX=" << pathconf (dirname, _PC_NAME_MAX) << endl);


    // Scan all filenames from the directory, convert each name into the timestamp
    // and store the timestamp in `tstamps' array

    int num_tstamps = 0;
    int dir_not_full_flag = 1;
    char *buf = (char *) malloc (sizeof (struct dirent)
				 + ((pathconf (dirname, _PC_NAME_MAX) > 0)? pathconf (dirname, _PC_NAME_MAX): 1024)
				 + 1);
#if defined(_POSIX_C_SOURCE)
    while (! readdir_r (dirp, (struct dirent *) buf, &direntp)) {
#else
    while (direntp = readdir_r (dirp, (struct dirent *) buf)) {
#endif
      if (!direntp)
	break;
      char *cfile = direntp -> d_name;
      if (valid_fname (cfile)) {
	if (dir_not_full_flag) {
	  timestamp_time tst;
	  tst.gps = ftosecs (cfile + strlen (prefix), &tst.dt);

	  if (tst.gps > 0)
	    {
	      dir_nfiles++;
	      if (num_tstamps < max_tstamps)
		tstamps [num_tstamps++] = tst;
	      else {
		system_log(1, "too many files in directory `%s'; supported max %d files", dirname, max_tstamps);
		dir_not_full_flag = 0;
	      }
	      //	    cerr << tst << endl;
	    } else {
	      system_log(1, "scan(): `%s/%s' is invalid filename -- skipped", dirname, cfile);
	    }
	} else {
	  dir_nfiles++;
	}
      }
    }
    
    (void) closedir (dirp);
    free ((void *) buf);

    //    perror_log ("after while()");

    if (! num_tstamps)
      {
	system_log(3, "directory %d is empty", i);
	continue;
      }

    DEBUG(12, cerr << "sorting array of " << num_tstamps << " integers" << endl);

    // Sort the array of timestamps
    qsort (tstamps, num_tstamps, sizeof (tstamps [0]),
	   (int (*) (const void*, const void*)) timestamp_time_compare);

    DEBUG(12, cerr << "Done" << endl);

    time_t frame_len = 0; // Tracks the length of the frame file

    range_block *cur_range_block = 0; 

    // Set the data ranges    
    for (int j = 0; j < num_tstamps; j++)
      {
	if (! frame_len) // The first time around
	  {
	    DEBUG(2, cerr << "scan(): before first frame_length_secs(); tstamps [j]=" << tstamps [j].gps << " " << tstamps[j].dt << endl);
	    //	    frame_len = frame_length_secs (tstamps [j], i);

	    frame_len = tstamps [j].dt;
	    DEBUG(2, cerr << "scan(): after first frame_length_secs()" << endl);
	    if (frame_len <= 0)
	      {
		frame_len = 0;
		system_log(1, "scan(): Can't determine the length of the FIRST frame %d", (int)tstamps [j].gps);
		continue;
	      }

	    // Create a data range block and assign first range
	    void *mptr = malloc (sizeof(range_block));
	    if (!mptr) {
	      system_log(1, "scan(): memory exhausted");
	      return -1;
	    }
	    cur_range_block = new (mptr) range_block (frame_len, tstamps [j].gps);

	    // Since the `tstams[]' is sorted, now this is the minimum available time
	    dir_min = tstamps [j].gps;

	    // Put the first data range block in the linked list
	    dir [i].blist.insert_first (cur_range_block);

	    system_log(3, "first range start %d length is %d", (int)tstamps [j].gps, (int)frame_len);
	  }
	else
	  {
	    // Determine if the current file, represented by `tstamps [j]', is time contiguous 
	    if (cur_range_block -> current_max_time ()  == tstamps [j].gps)
	      {
		// Increase the max time for the current range and we're done

		cur_range_block -> current_max_time_add (cur_range_block -> current_file_secs ());
	      }
	    else // New range ought to be started, because there is a gap in the available data
	      {
		// Determine the length of the last frame in the current range
		// The last file could be incomplete, so we may need to adjust the 
		// this range `max_time'
		if (cur_range_block -> current_max_time ()
		    > (cur_range_block -> current_min_time ()
		       + cur_range_block -> current_file_secs ())) // Only if the file wasn't checked already		  
		  {
		    time_t last_frame_tst = cur_range_block -> current_max_time ()
		      - cur_range_block -> current_file_secs ();
		    time_t last_frame_len = frame_length_secs (last_frame_tst, i);
		    if (last_frame_len <= 0) {
		      system_log(1, "scan(): Can't determine the length of LAST frame in the range %d", (int)last_frame_tst);
		    } else
		      cur_range_block -> current_max_time_sub (cur_range_block -> current_file_secs () - last_frame_len);
		  }

		//		time_t tmp_frame_len = frame_length_secs (tstamps [j], i);
		time_t tmp_frame_len = tstamps [j].dt;
		if (tmp_frame_len <= 0)
		  {
		    system_log(1, "scan(): Can't determine the length of the FIRST frame in the range %d", (int)tstamps [j].gps);
		    continue;
		  }
		frame_len = tmp_frame_len;

		system_log(3, "new range start %d length is %d", (int)tstamps [j].gps, (int)frame_len);

		// See if a new range block should be allocated (current block is full)
		if (cur_range_block -> is_full ())
		  {
		    // Create a data range block and assign first range
		    void *mptr = malloc (sizeof(range_block));
		    if (!mptr) {
		      system_log(1, "scan(): memory exhausted");
		      return -1;
		    }
		    cur_range_block = new (mptr) range_block (frame_len, tstamps [j].gps);

		    // Put the first data range block in the linked list
		    dir [i].blist.insert_first (cur_range_block);
		  }
		else
		  cur_range_block -> put (frame_len, tstamps [j].gps);
	      }
	  }

      }

    if (cur_range_block) {
      // Check on the size of the last frame in the directory only if there
      // are more than one file in the range.
      // Determine the length of the last frame in the current range
      // The last file could be incomplete, so we may need to adjust
      // this range's `max_time'
      if (cur_range_block -> current_max_time ()
	  > (cur_range_block -> current_min_time ()
	     + cur_range_block -> current_file_secs ()))
	{
	  time_t last_frame_tst = cur_range_block -> current_max_time ()
	    - cur_range_block -> current_file_secs ();
	  time_t last_frame_len = frame_length_secs (last_frame_tst, i);
	  if (last_frame_len <= 0) {
	    system_log(1, "scan(): Can't determine the length of LAST frame %d", (int)last_frame_tst);
	  } else
	    cur_range_block -> current_max_time_sub (cur_range_block -> current_file_secs ()
						     - last_frame_len);
	}
    }

    range_block *rb = (range_block *) dir [i].blist.first ();

    if (! rb)
      {
	system_log(3,"Directory %d is empty", i);
	dir [i].nfiles = dir [i].min_time = dir [i].max_time = 0;
      }
    else
      {
	// The last range will be the oldest
	dir_max = rb -> current_max_time ();

	for (s_link *clink = dir [i].blist.first (); clink;
	     clink = clink -> next())
	  {
	    range_block *rb = (range_block *) clink;

	    system_log(3, "num_ranges=%d", rb -> num_ranges);
	    for (int k = 0; k < rb -> num_ranges; k ++) {
	      system_log(3, "range %d file_secs=%d num_time=%d max_time=%d", k, (int) (rb -> d [k].file_secs), (int) (rb -> d [k].min_time), (int) (rb -> d [k].max_time));
	    }
	  }

	system_log(3, "directory  %d nfiles=%d", i, dir_nfiles);
	system_log(3, "directory  %d minimum=%d", i, (int)dir_min);
	system_log(3, "directory  %d maximum=%d", i, (int)dir_max);

	dir [i].nfiles =  dir_nfiles;
	dir [i].min_time =  dir_min;
	dir [i].max_time =  dir_max;
	  
	if (dir_max > global_max) {
	  global_max = dir_max;
	  global_max_dir = i;
	}
      
	if (dir_min < global_min)
	  global_min = dir_min;
      }
  }
    
  if (global_min == LONG_MAX)
    global_min = 0;

  //system_log(1, "global minimum=%d", global_min);
  //system_log(1, "global maximum=%d", global_max);

  min_time = global_min;
  max_time = global_max;
  cur_update_dir = global_max_dir;

  return 0;
}

char *
filesys_c::_filename_dir (time_t pt, char *fname, int dnum, int framedt)
{
    DEBUG(22, cerr << "_filename_dir(): pt=" << pt << ";dnum=" << dnum << endl);

    if ( framedt != 1) {
      sprintf (fname, "%s%d/%s%d-%d%s", path, dnum, prefix, (int) pt, framedt, suffix);
    } else {
      sprintf (fname, "%s%d/%s%d%s", path, dnum, prefix, (int) pt, suffix);
    }
    return fname;
  }

  /*
    Delete all files in the directory.
    `dir_path' must be allocate with malloc()
  */
void *
filesys_c::wiper (void *dir_path)
{
  char *dirname = (char *) dir_path;
  DIR *dirp;

#if defined(sun)
  // Put wiper into the time sharing class with 1/1000th of max priority
  daqd_c::time_sharing ((char *) 0, 1000);
#endif 
  system_log(3, "wiper thread started on `%s' directory", (char *) dir_path);
  if (! (dirp = opendir (dirname))) {
    system_log(1, "wiper(): couldn't open directory `%s'", dirname);
  } else {
    struct dirent *direntp;
    char *buf = (char *) malloc (sizeof (struct dirent) + ((pathconf (dirname, _PC_NAME_MAX))? pathconf (dirname, _PC_NAME_MAX): 1024) + 1);
    unsigned cntr = 1;
#if defined(_POSIX_C_SOURCE)
    while (! readdir_r (dirp, (struct dirent *) buf, &direntp))
#else
    while (direntp = readdir_r (dirp, (struct dirent *) buf))
#endif
      {
        if (!direntp)
	  break;
	char *cfile = direntp -> d_name;
	{
	  char fname [filename_max + 10];
	  sprintf (fname, "%s/%s", dirname, cfile);
	  DEBUG1(cerr << "unlink `" << fname << "'" << endl);
	  if (unlink (fname)) {
	    if (strcmp(cfile, ".") && strcmp(cfile, "..")) 
	      system_log(1, "wiper(): unlink(`%s') failed; errno=%d", fname, errno);
	  }
#if 0
	  if (!(cntr % 10)) {
#endif

            struct timespec tspec = {0,500000000};
            nanosleep (&tspec, NULL);
#if 0
          }
          cntr++;
#endif
	  //yield (); // ???
	}
      }
    (void) closedir (dirp);
    free ((void *) buf);
  }

  system_log(3, "wiped files in `%s' directory", (char *) dir_path);
  free ((void *) dir_path);
  return NULL;
}


int
filesys_c::construct_cb (int num_blocks)
{
  void *mptr = malloc (sizeof(circ_buffer));
  if (!mptr) {
    system_log(1,"couldn't construct filesys_c circular buffer, memory exhausted");
    return DAQD_MALLOC;
  }

  cb = new (mptr) circ_buffer (0, num_blocks, filename_max);
  if (! (cb -> buffer_ptr ())) {
    destroy_cb ();
    system_log(1, "couldn't allocate filesys_c buffer data blocks, memory exhausted");
    return DAQD_MALLOC;
  }

  return 0;
}


int
filesys_c::destroy_cb ()
{
  if (!cb)
    return 0;

  cb -> ~circ_buffer();
  free ((void *) cb);
  cb = 0;

  return 0;
}
