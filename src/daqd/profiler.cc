#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "daqd.hh"
#include "epics_pvs.hh"

extern daqd_c daqd;
/*
  Profiler thread code
*/
void *
profile_c::profiler ()
{
  // Set thread parameters
  char my_thr_label[16] = "dqp";
  strncat(my_thr_label,name.c_str(),5);
  char my_thr_name[40];
  snprintf (my_thr_name,40,"%.10s profiler thread",name.c_str());
  daqd_c::set_thread_priority(my_thr_name,my_thr_label,0,0); 

  started = 1;
  period = 0;
  for (unsigned long i = 0;;++i) {
    sleep (profiling_period);

#ifdef USE_BROADCAST
    if (profiling_period > 5) {
                daqd.fsd.scan ();
                daqd.trender.fsd.scan ();
    }
#endif

    if (this -> cb == daqd.b1 && daqd.edcu_ini_fckrs.size()) {
	// Go through the vector of files one per iteration
	file_checker &f = daqd.edcu_ini_fckrs[i%daqd.edcu_ini_fckrs.size()];
	int res = f.match();
	if (res == 0) {
      		DEBUG1(printf("%s CRC mismatch; dcu=%ld", f.file_name.c_str(), f.tag));
        }
	daqd.edcuFileStatus[f.tag] = !res;
    }

    if (shutdown)
      break;
    period++;
    int bfree = this -> cb -> bfree ();
    //int bfree = daqd.b1 -> blocks ();

    if (bfree < 2) {
      system_log(1, "%s profiler warning: %d empty blocks in the buffer", name.c_str(), bfree);
      if (!bfree && coredump) {
	system_log(1, "%s profiler: buffer is full -- aborting the program", name.c_str());
	abort ();
      }
    }
    PV::set_pv(PV::PV_PROFILER_FREE_SEGMENTS_MAIN_BUF, bfree);

    if (counters)
      counters[bfree]++;

    main_avg_free += bfree;
    if (main_min_free < 0 || bfree < main_min_free) {
      main_min_free = bfree;
    }

    additional_checks();
  }
  started = 0;
  shutdown = 0;
  return NULL;
}

void
profile_c::additional_checks()
{
    if (this -> cb == daqd.b1) {
        std::string filename;
        try {
            std::string hash;
            while (daqd.dequeue_frame_checksum(filename, hash)) {
                std::string::size_type index = filename.rfind('/');
                if (index != std::string::npos) {
                    std::string directory(filename.substr(0, index));
                    mkdir(directory.c_str(), 0777);
                }
                DEBUG1(cout << "Writing md5sum out to '" << filename << "' of '" << hash << "'" << std::endl);
                std::ofstream chksumFile(filename.c_str(), std::ios::binary | std::ios::out);
                chksumFile << hash << std::endl;
                chksumFile.close();
            }
        } catch(...) {
            std::cerr << "Error raised while writing a checksum file " << filename << std::endl;
            system_log(1, "Error raised while writing a checksum file");
        }
    }
}

void
profile_c::print_status (ostream *outs)
{
  if (started) {
    *outs << name << " profiler is running" << endl;
  } else {
    *outs << name << " profiler is not running" << endl;
  }

  if (period)
    *outs << "main_avg_free=" << (double)main_avg_free/(double)period << endl;
  *outs << "main_min_free=" << main_min_free << endl;

  *outs << "Num free block counters" << endl;;
  for (int i = 0; i < num_counters; i++)
    *outs << i << "\t- " << counters[i] << endl;
}

void
profile_c::stop_profiler () {
  shutdown = 1;
}

void
profile_c::start_profiler (circ_buffer *pcb) {
  pthread_t tprof;
  // error message buffer
  char errmsgbuf[80]; 

  if (counters)
    free ((void *) counters);

  this -> cb = pcb;

  counters = (int *) malloc (sizeof(int) * (num_counters = cb -> blocks()+1));
  if (! counters)
    num_counters = 0;
  else
    memset (counters, 0, sizeof(counters[0])*num_counters);

  pthread_attr_t attr;
  pthread_attr_init (&attr);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
  pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
  pthread_attr_setstacksize (&attr, daqd.thread_stack_size);

  // Lower profiler thread priority
  struct sched_param sparam;
  int policy;
  pthread_getschedparam (pthread_self(), &policy, &sparam);
  sparam.sched_priority--;
  pthread_attr_setschedparam (&attr, &sparam);
  
  //    pthread_attr_setstacksize (&attr, daqd.thread_stack_size);
  int err_no;
  if (err_no = pthread_create (&tprof, &attr, (void *(*)(void *))profiler_static, this)) {
    strerror_r(err_no, errmsgbuf, sizeof(errmsgbuf));
    free (counters); counters = 0;
    system_log(1, "profiler thread not started: pthread_create() err=%s", errmsgbuf);
  }
  pthread_attr_destroy (&attr);
}
