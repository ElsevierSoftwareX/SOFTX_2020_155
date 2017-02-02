///	@file moduleLoad.c
///	@brief File contains startup routines for real-time code.

#include <asm/uaccess.h>
#include <linux/ctype.h>
#include <linux/spinlock_types.h>
#include <proc.h>


// These externs and "16" need to go to a header file (mbuf.h)
extern void *kmalloc_area[16];
extern int mbuf_allocate_area(char *name, int size, struct file *file);
extern void *fe_start(void *arg);
extern int run_on_timer;
extern char daqArea[2*DAQ_DCU_SIZE];           // Space allocation for daqLib buffers

#include FE_PROC_FILE

/// /proc filesystem directory entry
struct proc_dir_entry *proc_dir_entry;
/// /proc/{sysname}/status entry
struct proc_dir_entry *proc_entry;
/// /proc/{sysname}/gps entry
struct proc_dir_entry *proc_gps_entry;
/// /proc/{sysname}/epics directory entry
struct proc_dir_entry *proc_epics_dir_entry;
/// /proc/{sysname}/futures entry
struct proc_dir_entry *proc_futures_entry;

void remove_epics_proc_files(void);
int create_epics_proc_files(void);

/// Routine to read the /proc/{model}/status file. \n \n
///	 We give all of our information in one go, so if the
///	 user asks us if we have more information the \n
///	 answer should always be no.
///	 
///	  This is important because the standard read
///	  function from the library would continue to issue \n
///	  the read system call until the kernel replies
///	  that it has no more information, or until its \n
///	  buffer is filled.
int
procfile_status_read(char *buffer,
	      char **buffer_location,
	      off_t offset, int buffer_length, int *eof, void *data)
{
	int ret, i;
	i = 0;
	*buffer = 0;

	/* 
	 * We give all of our information in one go, so if the
	 * user asks us if we have more information the
	 * answer should always be no.
	 *
	 * This is important because the standard read
	 * function from the library would continue to issue
	 * the read system call until the kernel replies
	 * that it has no more information, or until its
	 * buffer is filled.
	 */
	if (offset > 0) {
		/* we have finished to read, return 0 */
		ret  = 0;
	} else {
#if defined(SERVO64K) || defined(SERVO32K) || defined(SERVO16K) ||   defined(SERVO256K) ||  defined(SERVO128K) || defined(COMMDATA_INLINE)
		char b[128];
#if defined(SERVO64K) || defined(SERVO32K) ||   defined(SERVO256K) ||  defined(SERVO128K)
		static const int nb = 32;
#elif defined(SERVO16K)
		static const int nb = 64;
#endif
#endif
		/* fill the buffer, return the buffer size */
		ret = sprintf(buffer,

			"startGpsTime=%d\n"
			"uptime=%d\n"
			"cpuTimeEverMax=%d\n"
			"cpuTimeEverMaxWhen=%d\n"
			"adcHoldTime=%d\n"
			"adcHoldTimeEverMax=%d\n"
			"adcHoldTimeEverMaxWhen=%d\n"
			"adcHoldTimeMax=%d\n"
			"adcHoldTimeMin=%d\n"
			"adcHoldTimeAvg=%d\n"
			"usrTime=%d\n"
			"usrHoldTime=%d\n"
			"cycle=%d\n"
			"gps=%d\n"
			"buildDate=%s\n"
			"cpuTimeMax(cur,past sec)=%d,%d\n"
			"cpuTimeMaxCycle(cur,past sec)=%d,%d\n",

			startGpsTime,
			cycle_gps_time - startGpsTime,
			cpuTimeEverMax,
			cpuTimeEverMaxWhen,
			adcHoldTime,
			adcHoldTimeEverMax,
			adcHoldTimeEverMaxWhen,
			adcHoldTimeMax,
			adcHoldTimeMin,
			adcHoldTimeAvgPerSec,
			usrTime,
			usrHoldTime,
			cycleNum,
			cycle_gps_time,
			build_date,
			cycleTime, timeHoldHold,
			timeHoldWhen, timeHoldWhenHold);
#if defined(SERVO64K) || defined(SERVO32K) || defined(SERVO16K)
		strcat(buffer, "cycleHist: ");
		for (i = 0; i < nb; i++) {
			if (!cycleHistMax[i]) continue;
			sprintf(b, "%d=%d@%d ", i, cycleHistMax[i], cycleHistWhenHold[i]);
			strcat(buffer, b);
		}
		strcat(buffer, "\n");
#endif

#ifdef COMMDATA_INLINE
		// See if we have any IPC with errors and print the numbers out
		//
		sprintf(b, "ipcErrBits=0x%x\n", ipcErrBits);
		strcat(buffer, b);

		// The following loop has a chance to overflow the buffer,
		// which is set to PROC_BLOCK_SIZE. (PAGE_SIZE-1024 = 3072 bytes).
		// We will simply stop printing at that point.
#define PROC_BLOCK_SIZE (3*1024)
		unsigned int byte_cnt = strlen(buffer) + 1;
		for (i = 0; i < myIpcCount; i++) {
	  	  if (ipcInfo[i].errTotal) {
	  		unsigned int cnt =
				sprintf(b, "IPC net=%d num=%d name=%s sender=%s errcnt=%d\n", 
					ipcInfo[i].netType, ipcInfo[i].ipcNum,
					ipcInfo[i].name, ipcInfo[i].senderModelName,
					ipcInfo[i].errTotal);
			if (byte_cnt + cnt > PROC_BLOCK_SIZE) break;
			byte_cnt += cnt;
	  		strcat(buffer, b);
	  	  }
 		}
#endif
		ret = strlen(buffer);
	}

	return ret;
}

/// Routine to read the /proc/{model}/gps file. \n \n
///
int
procfile_gps_read(char *buffer,
	      char **buffer_location,
	      off_t offset, int buffer_length, int *eof, void *data)
{
	*buffer = 0;

	if (offset > 0) {
		/* we have finished to read, return 0 */
		return 0;
	} else {
		/* fill the buffer, return the buffer size */
		/* print current GPS time and zero padded fraction */
		return sprintf(buffer, "%d.%02d\n", cycle_gps_time, (cycleNum*100/CYCLE_PER_SECOND)%100);
	}
	// Never reached
}

/// Routine to read the /proc/{model}/epics/{chname} files. \n \n
///
int
procfile_epics_read(char *buffer,
	      char **buffer_location,
	      off_t offset, int buffer_length, int *eof, void *data)
{
	*buffer = 0;

	if (offset > 0) {
		/* we have finished to read, return 0 */
		return 0;
	} else {
		/* fill the buffer, return the buffer size */
		struct proc_epics *pe = (struct proc_epics *)data;
		unsigned int ncel = pe->nrow * pe->ncol; // Matrix size
		switch (pe -> type) {
			case 0: /* int */
				return sprintf(buffer, "%s%d\n",
					(*((char *)(((void *)pLocalEpics) + pe->mask_idx)))? "\t": "",
					*((int *)(((void *)pLocalEpics) + pe->idx)));
	//			printf("Accessing data at %x\n", (((void *)pLocalEpics) + pe->idx));
				break;
			case 1: /* double */ {
				char lb[64];
				if (ncel) { /* Print Matrix */
					unsigned int i, j;
					unsigned int nrow = pe->nrow;
					unsigned int ncol = pe->ncol;
					unsigned int s = 0;
					unsigned int masked = *((char *)(((void *)pLocalEpics) + pe->mask_idx));
					for (i = 0; i < nrow; i++)  {
						if (masked) s += sprintf(buffer + s, "\t");
						for (j = 0; j < ncol; j++)  {
							s += sprintf(buffer + s, "%s ", dtoa_r(lb, ((double *)(((void *)pLocalEpics) + pe->idx))[ncol*i + j]));
						}
						s += sprintf(buffer + s, "\n");
					}
					return s;
				}  else { /* Print single value */
					return sprintf(buffer, "%s%s\n",
						(*((char *)(((void *)pLocalEpics) + pe->mask_idx)))? "\t": "",
						dtoa_r(lb, *((double *)(((void *)pLocalEpics) + pe->idx))));
				}
				break;
			}
		}
	}
	// Never reached
}

#define PROC_BLOCK_SIZE (3*1024)

// Scan a double
double
simple_strtod(char *start, char **end) {
	int integer;
	if (*start != '.') {
		integer = simple_strtol(start, end, 10);
        	if (*end == start) return 0.0;
		start = *end;
	} else integer = 0;
	if (*start != '.') return integer;
	else {
		start++;
		double frac = simple_strtol(start, end, 10);
        	if (*end == start) return integer;
		int i;
		for (i = 0; i < (*end - start); i++) frac /= 10.0;
		return ((double)integer) + frac;
	}
	// Never reached
}


int
procfile_epics_write(struct file *file, const char __user *buf,
                     unsigned long count, void *data)
{
        ssize_t ret = -ENOMEM;
        char *page;
        char *start;
 	unsigned int future = 0;

        if (count > PROC_BLOCK_SIZE)
                return -EOVERFLOW;

        start = page = (char *)__get_free_page(GFP_KERNEL);
        if (page) {
                ret = -EFAULT;
                if (copy_from_user(page, buf, count))
                        goto out;
		page[count] = 0;
		ret = count;
		struct proc_epics *pe = (struct proc_epics *)data;
	        char *end;
		for(;isspace(*start);start++);
		// See if this a single-character command
		switch (*start) {
			case 'm':
				*((char *)(((void *)pLocalEpics) + pe->mask_idx)) = 1;
				start++;
				break;
			case 'u':
				*((char *)(((void *)pLocalEpics) + pe->mask_idx)) = 0;
				start++;
				break;
			case 'f':
				future=1;
				start++;
				break;
		}
		for(;isspace(*start);start++);
		double new_double = 0.0;
		int new_int = 0;
		switch (pe -> type) {
			case 0: /* int */ 
			        new_int = simple_strtol(start, &end, 10);
			        if (new_int > INT_MAX || new_int < INT_MIN) {
                			ret = -EFAULT;
					goto out;
				}
				break;
			case 1: /* double */ 
			        new_double = simple_strtod(start, &end);
				//printf("User wrote %s\n", dtoa1(new_double));
				break;
		}
	        if (end == start) goto out;
		// See if this is a matrix
		int idx = 0;
		int ncel = pe->nrow * pe->ncol;
		if (ncel) {
			// Expect the cell index number next
			start = end;
			for(;isspace(*start);start++);
			idx = simple_strtol(start, &end, 10);
			if (idx >= ncel || idx < 0) {
               			ret = -EFAULT;
				goto out;
			}
			
		}
		if (!future) {
			switch (pe -> type) {
				case 0: /* int */ 
					*((int *)(((void *)pLocalEpics) + pe->idx)) = new_int;
					break;
				case 1: /* double */
					((double *)(((void *)pLocalEpics) + pe->idx))[idx] = new_double;
					break;
			}
		} else if ((*((char *)(((void *)pLocalEpics) + pe->mask_idx)))) { // Allow to setup a future only if masked
			unsigned long cycle;
			start = end;
			for(;isspace(*start);start++);
			// check for gps seconds
			double gps = simple_strtod(start, &end);
			if (end == start) {
				gps = cycle_gps_time + 5;
				cycle = cycleNum;
			} else {
				// Convert gps time
				unsigned long gps_int = gps;
				gps -= gps_int;
				cycle = gps * CYCLE_PER_SECOND;
				gps = gps_int;
			}
			// Add new future setpoint
			// Find first available slot
			int i;
			static DEFINE_SPINLOCK(fut_lock);
			spin_lock(&fut_lock);
			for (i = 0; (i < MAX_PROC_FUTURES) && proc_futures[i].proc_epics; i++);
			if (i == MAX_PROC_FUTURES) {
				spin_unlock(&fut_lock);
				ret = -ENOMEM;
			} else {
				// Found a slot
				proc_futures[i].cycle = cycle;
				proc_futures[i].gps = gps;
				proc_futures[i].val = pe->type? new_double: new_int;
				proc_futures[i].idx = idx;

				// This pointer has to be set last; Indicates a vailid entry for the
 				// real-time code in controller.c
				proc_futures[i].proc_epics = pe; // FE code reads/writes this variable
				spin_unlock(&fut_lock);
			}
		} else ret = -EFAULT;
        }
out:
        free_page((unsigned long)page);
        return ret;
}

/// Routine to read the /proc/{model}/epics/futures files. \n \n
///
// :TODO: the function does not check for buffer overflow, Linux kernel catches it though,
//  when proc_futures contains too many entries.
int
procfile_futures_read(char *buffer,
	      char **buffer_location,
	      off_t offset, int buffer_length, int *eof, void *data)
{
	*buffer = 0;

	if (offset > 0) {
		/* we have finished to read, return 0 */
		return 0;
	} else {
		/* fill the buffer, return the buffer size */
		char b[128];
		int i;
		for (i = 0; i < MAX_PROC_FUTURES; i++) {
			// Copy the entry to avoid race with the FE code
			struct proc_futures pf = proc_futures[i];
			if (pf.proc_epics) {
				char s[64];
				char s1[64];
				if (pf.idx) { /* matrix */
					sprintf(b, "%s %s@%s %d %d\n", pf.proc_epics->name, dtoa_r(s, pf.val), dtoa_r(s1, pf.idx), pf.gps, pf.cycle);
				} else {
					sprintf(b, "%s %s %d %d\n", pf.proc_epics->name, dtoa_r(s, pf.val), pf.gps, pf.cycle);
				}
				strcat(buffer, b);
			}
		}
		return strlen(buffer);
	}
	// Never reached
}


static const ner = sizeof(proc_epics)/sizeof(struct proc_epics);

/// Function to create /proc/{model}/epics/* files
///
int
create_epics_proc_files() {
	int i;

	for (i = 0; i < ner; i++) proc_epics_entry[i]  = 0;

	for (i = 0; i < ner; i++) {
		//printf("%s\n", proc_epics[i].name);
        	proc_epics_entry[i] = create_proc_entry(proc_epics[i].name, PROC_MODE, proc_epics_dir_entry);
        	if (proc_epics_entry[i]  == NULL) {
		        remove_epics_proc_files();
                	printk(KERN_ALERT "Error: Could not initialize /proc/%s/epics/%s\n", SYSTEM_NAME_STRING_LOWER, proc_epics[i].name);
                	return 0;
        	}
		proc_epics_entry[i]->mode = S_IFREG | S_IRUGO;
		switch (proc_epics[i].in) {
			case 0:
				proc_epics_entry[i]->write_proc = procfile_epics_write;
				proc_epics_entry[i]->mode |= S_IWUGO;
				// Fall through
			case 1:
				proc_epics_entry[i]->read_proc = procfile_epics_read;
				break;
		}
		proc_epics_entry[i]->uid 	 = PROC_UID;
		proc_epics_entry[i]->gid 	 = 0;
		unsigned int ncel = proc_epics[i].nrow * proc_epics[i].ncol; // Enough space to deal with the matrix
		proc_epics_entry[i]->size 	 = ncel? 64 * ncel: 128;
		proc_epics_entry[i]->data 	 = proc_epics + i;
	}
	proc_futures_entry = create_proc_entry("futures", PROC_MODE, proc_epics_dir_entry);
        if (proc_futures_entry  == NULL) {
                remove_epics_proc_files();
                printk(KERN_ALERT "Error: Could not initialize /proc/%s/epics/futures\n", SYSTEM_NAME_STRING_LOWER);
                return 0;
	}
	proc_futures_entry->mode = S_IFREG | S_IRUGO;
	proc_futures_entry->uid 	= PROC_UID;
	proc_futures_entry->gid 	= 0;
	proc_futures_entry->size	= 10240;
	proc_futures_entry->read_proc = procfile_futures_read;
	return 1;
}

/// Function to delete /proc/{model}/epics/* files
///
void remove_epics_proc_files() {
	int i;
	for (i = 0; i < ner; i++)
		if (proc_epics_entry[i] != NULL) {
			remove_proc_entry(proc_epics[i].name, proc_epics_dir_entry);
			proc_epics_entry[i] = 0;
		}
	remove_proc_entry("futures", proc_epics_dir_entry);
}

// MAIN routine: Code starting point ****************************************************************
#ifdef ADC_MASTER
int need_to_load_IOP_first;
EXPORT_SYMBOL(need_to_load_IOP_first);
#endif
#ifdef ADC_SLAVE
extern int need_to_load_IOP_first;
#endif

extern void set_fe_code_idle(void *(*ptr)(void *), unsigned int cpu);
extern void msleep(unsigned int);

/// Startup function for initialization of kernel module.
int init_module (void)
{
 	int status;
	int ii,jj,kk;		/// @param ii,jj,kk default loop counters
	char fname[128];	/// @param fname[128] Name of shared mem area to allocate for DAQ data
	int cards;		/// @param cards Number of PCIe cards found on bus
	int ret;		/// @param ret Return value from various Malloc calls to allocate memory.
	int cnt;
	extern int cpu_down(unsigned int);	/// @param cpu_down CPU shutdown call.
	extern int is_cpu_taken_by_rcg_model(unsigned int cpu);	/// @param is_cpu_taken_by_rcg_model Check to verify CPU availability for shutdown.

	kk = 0;
#ifdef SPECIFIC_CPU
#define CPUID SPECIFIC_CPU
#else 
#define CPUID 1 
#endif

#ifndef NO_CPU_SHUTDOWN
	// See if our CPU core is free
        if (is_cpu_taken_by_rcg_model(CPUID)) {
		printk(KERN_ALERT "Error: CPU %d already taken\n", CPUID);
		return -1;
	}
#endif

#ifdef ADC_SLAVE
        need_to_load_IOP_first = 0;
#endif


#ifdef DOLPHIN_TEST
	status = init_dolphin(2);
	if (status != 0) {
		return -1;
	}
#endif

	// Create /proc filesystem tree
	proc_dir_entry = proc_mkdir(SYSTEM_NAME_STRING_LOWER, NULL);
	if (proc_dir_entry == NULL) {
		printk(KERN_ALERT "Error: Could not initialize /proc/%s\n", SYSTEM_NAME_STRING_LOWER);
		return -ENOMEM;
	}
	
	proc_entry = create_proc_entry("status", PROC_MODE, proc_dir_entry);
	if (proc_entry == NULL) {
		remove_proc_entry(SYSTEM_NAME_STRING_LOWER, NULL);
		printk(KERN_ALERT "Error: Could not initialize /proc/%s/status\n", SYSTEM_NAME_STRING_LOWER);
		return -ENOMEM;
	}
	
	proc_gps_entry = create_proc_entry("gps", PROC_MODE, proc_dir_entry);
	if (proc_gps_entry == NULL) {
		remove_proc_entry("status", proc_dir_entry);
		remove_proc_entry(SYSTEM_NAME_STRING_LOWER, NULL);
		printk(KERN_ALERT "Error: Could not initialize /proc/%s/gps\n", SYSTEM_NAME_STRING_LOWER);
		return -ENOMEM;
	}

	proc_epics_dir_entry = proc_mkdir("epics", proc_dir_entry);
	if (proc_epics_dir_entry == NULL) {
		remove_proc_entry("gps", proc_dir_entry);
		remove_proc_entry("status", proc_dir_entry);
		remove_proc_entry(SYSTEM_NAME_STRING_LOWER, NULL);
		printk(KERN_ALERT "Error: Could not initialize /proc/%s/epics\n", SYSTEM_NAME_STRING_LOWER);
		return -ENOMEM;
	}

	proc_entry->read_proc = procfile_status_read;
	proc_entry->mode 	 = S_IFREG | S_IRUGO;
	proc_entry->uid 	 = PROC_UID;
	proc_entry->gid 	 = 0;
	proc_entry->size 	 = 10240;

	proc_gps_entry->read_proc = procfile_gps_read;
	proc_gps_entry->mode 	 = S_IFREG | S_IRUGO;
	proc_gps_entry->uid 	 = PROC_UID;
	proc_gps_entry->gid 	 = 0;
	proc_gps_entry->size 	 = 128;
	proc_gps_entry->data 	 = &cycle_gps_time;

	// Create all /proc/{system}/epics/* files, one file per  existing Epics channel
  	if (create_epics_proc_files() == 0) {
		remove_proc_entry("epics", proc_dir_entry);
		remove_proc_entry("gps", proc_dir_entry);
		remove_proc_entry("status", proc_dir_entry);
		remove_proc_entry(SYSTEM_NAME_STRING_LOWER, NULL);
		return -ENOMEM;
	}

	printf("startup time is %ld\n", current_time());
	jj = 0;
	printf("cpu clock %u\n",cpu_khz);


        ret =  mbuf_allocate_area(SYSTEM_NAME_STRING_LOWER, 64*1024*1024, 0);
        if (ret < 0) {
                printf("mbuf_allocate_area() failed; ret = %d\n", ret);
                return -1;
        }
        _epics_shm = (unsigned char *)(kmalloc_area[ret]);
        printf("EPICSM at 0x%x\n", _epics_shm);
        ret =  mbuf_allocate_area("ipc", 4*1024*1024, 0);
        if (ret < 0) {
                printf("mbuf_allocate_area(ipc) failed; ret = %d\n", ret);
                return -1;
        }
        _ipc_shm = (unsigned char *)(kmalloc_area[ret]);

	printf("IPC    at 0x%x\n",_ipc_shm);
  	ioMemData = (IO_MEM_DATA *)(_ipc_shm+ 0x4000);
	printf("IOMEM  at 0x%x size 0x%x\n",(_ipc_shm + 0x4000),sizeof(IO_MEM_DATA));


	// Find and initialize all PCI I/O modules *******************************************************
	  // Following I/O card info is from feCode
	  cards = sizeof(cards_used)/sizeof(cards_used[0]);
	  printf("configured to use %d cards\n", cards);
	  cdsPciModules.cards = cards;
	  cdsPciModules.cards_used = cards_used;
          //return -1;
	printf("Initializing PCI Modules\n");
	cdsPciModules.adcCount = 1;
	for(ii=0;ii<cdsPciModules.adcCount;ii++)
		cdsPciModules.adcType[ii] = GSC_16AI64SSA;
	cdsPciModules.dacCount = 0;
	cdsPciModules.dioCount = 0;
	cdsPciModules.doCount = 0;
#ifdef DOLPHIN_TEST
	ioMemData->dolphinCount = 0;
        ioMemData->dolphinRead[0] = 0;
        ioMemData->dolphinWrite[0] = 0;
        ioMemData->dolphinRead[1] = 0;
        ioMemData->dolphinWrite[1] = 0;
	ioMemData->dolphinCount = cdsPciModules.dolphinCount;
        ioMemData->dolphinRead[0] = cdsPciModules.dolphinRead[0];
        ioMemData->dolphinWrite[0] = cdsPciModules.dolphinWrite[0];
        ioMemData->dolphinRead[1] = cdsPciModules.dolphinRead[1];
        ioMemData->dolphinWrite[1] = cdsPciModules.dolphinWrite[1];
#else
	cdsPciModules.dolphinCount = ioMemData->dolphinCount;
	cdsPciModules.dolphinRead[0] = ioMemData->dolphinRead[0];
	cdsPciModules.dolphinWrite[0] = ioMemData->dolphinWrite[0];
	cdsPciModules.dolphinRead[1] = ioMemData->dolphinRead[1];
	cdsPciModules.dolphinWrite[1] = ioMemData->dolphinWrite[1];
	printf("dr0 = 0x%lx \n",(unsigned long) cdsPciModules.dolphinRead[0]);
	printf("dw0 = 0x%lx \n",(unsigned long) cdsPciModules.dolphinWrite[0]);
	printf("dr1 = 0x%lx \n",(unsigned long) cdsPciModules.dolphinRead[1]);
	printf("dw1 = 0x%lx \n",(unsigned long) cdsPciModules.dolphinWrite[1]);
	if (cdsPciModules.dolphinCount != 2) {
		return -1;
	}
#endif


	// Code will run on internal timer if no ADC modules are found
	printf("Virtual IOP ******* running on timer\n");
	run_on_timer = 1;

	// Initialize buffer for daqLib.c code
	printf("Initializing space for daqLib buffers\n");
	daqBuffer = (long)&daqArea[0];
 

        pLocalEpics = (CDS_EPICS *)&((RFM_FE_COMMS *)_epics_shm)->epicsSpace;
	for (cnt = 0;  cnt < 10 && pLocalEpics->epicsInput.burtRestore == 0; cnt++) {
        	printf("Epics burt restore is %d\n", pLocalEpics->epicsInput.burtRestore);
        	msleep(1000);
	}
	if (cnt == 10) {
		// Cleanup
		remove_epics_proc_files();
		remove_proc_entry("epics", proc_dir_entry);
		remove_proc_entry("gps", proc_dir_entry);
		remove_proc_entry("status", proc_dir_entry);
		remove_proc_entry(SYSTEM_NAME_STRING_LOWER, NULL);
#ifdef DOLPHIN_TEST
		finish_dolphin();
#endif
		return -1;
	}

        pLocalEpics->epicsInput.vmeReset = 0;

#ifdef NO_CPU_SHUTDOWN
        struct task_struct *p;
        p = kthread_create(fe_start, 0, "fe_start/%d", CPUID);
        if (IS_ERR(p)){
                printf("Failed to kthread_create()\n");
                return -1;
        }
        kthread_bind(p, CPUID);
        wake_up_process(p);
#endif


#ifndef NO_CPU_SHUTDOWN
        set_fe_code_idle(fe_start, CPUID);
        msleep(100);

	cpu_down(CPUID);

	// The code runs on the disabled CPU
#endif
        return 0;
}

void cleanup_module (void) {
	int i;
	extern int __cpuinit cpu_up(unsigned int cpu);

	remove_epics_proc_files();
	remove_proc_entry("epics", proc_dir_entry);
	remove_proc_entry("gps", proc_dir_entry);
	remove_proc_entry("status", proc_dir_entry);
	remove_proc_entry(SYSTEM_NAME_STRING_LOWER, NULL);

#ifndef NO_CPU_SHUTDOWN
	// Unset the code callback
        set_fe_code_idle(0, CPUID);
#endif

	printk("Setting stop_working_threads to 1\n");
	// Stop the code and wait
        stop_working_threads = 1;
        msleep(1000);

#ifdef DOLPHIN_TEST
	finish_dolphin();
#endif

#ifndef NO_CPU_SHUTDOWN

	// Unset the code callback
        set_fe_code_idle(0, CPUID);
	printkl("Will bring back CPU %d\n", CPUID);
        msleep(1000);
	// Unmask all masked epics channels
	for (i = 0; i < ner; i++) {
  	  *((char *)(((void *)pLocalEpics) + proc_epics[i].mask_idx)) = 0;
	}
	// Bring the CPU back up
        cpu_up(CPUID);
        //msleep(1000);
	printkl("Brought the CPU back up\n");
#endif
	printk("Just before returning from cleanup_module for " SYSTEM_NAME_STRING_LOWER "\n");

}

MODULE_DESCRIPTION("Control system");
MODULE_AUTHOR("LIGO");
MODULE_LICENSE("Dual BSD/GPL");
