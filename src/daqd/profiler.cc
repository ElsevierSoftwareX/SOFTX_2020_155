#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include "daqd.hh"

extern daqd_c daqd;
/*
  Profiler thread code
*/
void *
profile_c::profiler ()
{
/* Get Thread ID */
  pid_t prof_tid;
  prof_tid = (pid_t) syscall(SYS_gettid);
  system_log(1, "%s profiler thread pid=%d", name.c_str(),(int) prof_tid);  

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

    if (counters)
      counters[bfree]++;

    main_avg_free += bfree;
    if (main_min_free < 0 || bfree < main_min_free)
      main_min_free = bfree;

  }
  started = 0;
  shutdown = 0;
  return NULL;
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
    free (counters); counters = 0;
    system_log(1, "profiler thread not started: pthread_create() err=%d", err_no);
  }
  pthread_attr_destroy (&attr);
}
