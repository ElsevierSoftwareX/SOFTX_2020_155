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
#if defined(SERVO64K) || defined(SERVO32K) || defined(SERVO16K) || defined(COMMDATA_INLINE)
		char b[128];
#if defined(SERVO64K) || defined(SERVO32K)
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

#ifdef ADC_MASTER
		/* Output DAC buffer size information */
		for (i = 0; i < cdsPciModules.dacCount; i++) {
			if (cdsPciModules.dacType[i] == GSC_18AO8) {
				sprintf(b, "DAC #%d 18-bit buf_size=%d\n", i, dacOutBufSize[i]);
			} else {
				sprintf(b, "DAC #%d 16-bit fifo_status=%d (%s)\n", i, dacOutBufSize[i],
					dacOutBufSize[i] & 8? "full":
						(dacOutBufSize[i] & 1? "empty":
							(dacOutBufSize[i] & 4? "high quarter": "OK")));
			}
			strcat(buffer, b);
		}
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
#ifdef ADC_SLAVE
	int adcCnt;		/// @param adcCnt Number of ADC cards found by slave model.
	int dacCnt;		/// @param dacCnt Number of 16bit DAC cards found by slave model.
        int dac18Cnt;		/// @param dac18Cnt Number of 18bit DAC cards found by slave model.
	int doCnt;		/// @param doCnt Total number of digital I/O cards found by slave model.
	int do32Cnt;		/// @param do32Cnt Total number of Contec 32 bit DIO cards found by slave model.
	int doIIRO16Cnt;	/// @param doIIRO16Cnt Total number of Acces I/O 16 bit relay cards found by slave model.
	int doIIRO8Cnt;		/// @param doIIRO8Cnt Total number of Acces I/O 8 bit relay cards found by slave model.
	int cdo64Cnt;		/// @param cdo64Cnt Total number of Contec 6464 DIO card 32bit output sections mapped by slave model.
	int cdi64Cnt;		/// @param cdo64Cnt Total number of Contec 6464 DIO card 32bit input sections mapped by slave model.
#endif
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
	status = init_dolphin();
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


// If DAQ is via shared memory (Framebuilder code running on same machine or MX networking is used)
// attach DAQ shared memory location.
        sprintf(fname, "%s_daq", SYSTEM_NAME_STRING_LOWER);
        ret =  mbuf_allocate_area(fname, 64*1024*1024, 0);
        if (ret < 0) {
                printf("mbuf_allocate_area() failed; ret = %d\n", ret);
                return -1;
        }
        _daq_shm = (unsigned char *)(kmalloc_area[ret]);
        // printf("Allocated daq shmem; set at 0x%x\n", _daq_shm);
	printf("DAQSM at 0x%x\n",_daq_shm);
 	daqPtr = (struct rmIpcStr *) _daq_shm;

	// Find and initialize all PCI I/O modules *******************************************************
	  // Following I/O card info is from feCode
	  cards = sizeof(cards_used)/sizeof(cards_used[0]);
	  printf("configured to use %d cards\n", cards);
	  cdsPciModules.cards = cards;
	  cdsPciModules.cards_used = cards_used;
          //return -1;
	printf("Initializing PCI Modules\n");
	cdsPciModules.adcCount = 0;
	cdsPciModules.dacCount = 0;
	cdsPciModules.dioCount = 0;
	cdsPciModules.doCount = 0;

#ifndef ADC_SLAVE
	// Call PCI initialization routine in map.c file.
	status = mapPciModules(&cdsPciModules);
	 //return 0;
#endif
#ifdef ADC_SLAVE
// If running as a slave process, I/O card information is via ipc shared memory
	printf("%d PCI cards found\n",ioMemData->totalCards);
	status = 0;
	adcCnt = 0;
	dacCnt = 0;
	dac18Cnt = 0;
	doCnt = 0;
	do32Cnt = 0;
	cdo64Cnt = 0;
	cdi64Cnt = 0;
	doIIRO16Cnt = 0;
	doIIRO8Cnt = 0;

	// Have to search thru all cards and find desired instance for application
	// Master will map ADC cards first, then DAC and finally DIO
	for(ii=0;ii<ioMemData->totalCards;ii++)
	{
		/*
		printf("Model %d = %d\n",ii,ioMemData->model[ii]);
		*/
		for(jj=0;jj<cards;jj++)
		{
			/*
			printf("Model %d = %d, type = %d, instance = %d, dacCnt = %d \n",
				ii,ioMemData->model[ii],
				cdsPciModules.cards_used[jj].type,
				cdsPciModules.cards_used[jj].instance,
 				dacCnt);
				*/
		   switch(ioMemData->model[ii])
		   {
			case GSC_16AI64SSA:
				if((cdsPciModules.cards_used[jj].type == GSC_16AI64SSA) && 
					(cdsPciModules.cards_used[jj].instance == adcCnt))
				{
					printf("Found ADC at %d %d\n",jj,ioMemData->ipc[ii]);
					kk = cdsPciModules.adcCount;
					cdsPciModules.adcType[kk] = GSC_16AI64SSA;
					cdsPciModules.adcConfig[kk] = ioMemData->ipc[ii];
					cdsPciModules.adcCount ++;
					status ++;
				}
				break;
			case GSC_16AO16:
				if((cdsPciModules.cards_used[jj].type == GSC_16AO16) && 
					(cdsPciModules.cards_used[jj].instance == dacCnt))
				{
					printf("Found DAC at %d %d\n",jj,ioMemData->ipc[ii]);
					kk = cdsPciModules.dacCount;
					cdsPciModules.dacType[kk] = GSC_16AO16;
					cdsPciModules.dacConfig[kk] = ioMemData->ipc[ii];
	   				cdsPciModules.pci_dac[kk] = (long)(ioMemData->iodata[ii]);
					cdsPciModules.dacCount ++;
					status ++;
				}
				break;
			case GSC_18AO8:
				if((cdsPciModules.cards_used[jj].type == GSC_18AO8) && 
					(cdsPciModules.cards_used[jj].instance == dac18Cnt))
				{
					printf("Found DAC at %d %d\n",jj,ioMemData->ipc[ii]);
					kk = cdsPciModules.dacCount;
					cdsPciModules.dacType[kk] = GSC_18AO8;
					cdsPciModules.dacConfig[kk] = ioMemData->ipc[ii];
	   				cdsPciModules.pci_dac[kk] = (long)(ioMemData->iodata[ii]);
					cdsPciModules.dacCount ++;
					status ++;
				}
				break;
			case CON_6464DIO:
				if((cdsPciModules.cards_used[jj].type == CON_6464DIO) && 
					(cdsPciModules.cards_used[jj].instance == doCnt))
				{
					kk = cdsPciModules.doCount;
					printf("Found 6464 DIO CONTEC at %d 0x%x\n",jj,ioMemData->ipc[ii]);
					cdsPciModules.doType[kk] = ioMemData->model[ii];
					cdsPciModules.pci_do[kk] = ioMemData->ipc[ii];
					cdsPciModules.doCount ++;
					cdsPciModules.cDio6464lCount ++;
					cdsPciModules.pci_do[kk] = ioMemData->ipc[ii];
					cdsPciModules.doInstance[kk] = doCnt;
					status += 2;
				}
				if((cdsPciModules.cards_used[jj].type == CDO64) && 
					(cdsPciModules.cards_used[jj].instance == doCnt))
				{
					kk = cdsPciModules.doCount;
					cdsPciModules.doType[kk] = CDO64;
					cdsPciModules.pci_do[kk] = ioMemData->ipc[ii];
					cdsPciModules.doCount ++;
					cdsPciModules.cDio6464lCount ++;
					cdsPciModules.doInstance[kk] = doCnt;
					printf("Found 6464 DOUT CONTEC at %d 0x%x index %d \n",jj,ioMemData->ipc[ii],doCnt);
					cdo64Cnt ++;
					status ++;
				}
				if((cdsPciModules.cards_used[jj].type == CDI64) && 
					(cdsPciModules.cards_used[jj].instance == doCnt))
				{
					kk = cdsPciModules.doCount;
					// printf("Found 6464 DIN CONTEC at %d 0x%x\n",jj,ioMemData->ipc[ii]);
					cdsPciModules.doType[kk] = CDI64;
					cdsPciModules.pci_do[kk] = ioMemData->ipc[ii];
					cdsPciModules.doInstance[kk] = doCnt;
					cdsPciModules.doCount ++;
					printf("Found 6464 DIN CONTEC at %d 0x%x index %d \n",jj,ioMemData->ipc[ii],doCnt);
					cdsPciModules.cDio6464lCount ++;
					cdi64Cnt ++;
					status ++;
				}
				break;
                        case CON_32DO:
		                if((cdsPciModules.cards_used[jj].type == CON_32DO) &&
		                        (cdsPciModules.cards_used[jj].instance == do32Cnt))
		                {
					kk = cdsPciModules.doCount;
               			      	printf("Found 32 DO CONTEC at %d 0x%x\n",jj,ioMemData->ipc[ii]);
		                      	cdsPciModules.doType[kk] = ioMemData->model[ii];
                                      	cdsPciModules.pci_do[kk] = ioMemData->ipc[ii];
                                      	cdsPciModules.doCount ++; 
                                      	cdsPciModules.cDo32lCount ++; 
					cdsPciModules.doInstance[kk] = do32Cnt;
					status ++;
				 }
				 break;
			case ACS_16DIO:
				if((cdsPciModules.cards_used[jj].type == ACS_16DIO) && 
					(cdsPciModules.cards_used[jj].instance == doIIRO16Cnt))
				{
					kk = cdsPciModules.doCount;
					printf("Found Access IIRO-16 at %d 0x%x\n",jj,ioMemData->ipc[ii]);
					cdsPciModules.doType[kk] = ioMemData->model[ii];
					cdsPciModules.pci_do[kk] = ioMemData->ipc[ii];
					cdsPciModules.doCount ++;
					cdsPciModules.iiroDio1Count ++;
					cdsPciModules.doInstance[kk] = doIIRO16Cnt;
					status ++;
				}
				break;
			case ACS_8DIO:
			       if((cdsPciModules.cards_used[jj].type == ACS_8DIO) &&
			               (cdsPciModules.cards_used[jj].instance == doIIRO8Cnt))
			       {
			               kk = cdsPciModules.doCount;
			               printf("Found Access IIRO-8 at %d 0x%x\n",jj,ioMemData->ipc[ii]);
			               cdsPciModules.doType[kk] = ioMemData->model[ii];
			               cdsPciModules.pci_do[kk] = ioMemData->ipc[ii];
			               cdsPciModules.doCount ++;
			               cdsPciModules.iiroDioCount ++;
			               cdsPciModules.doInstance[kk] = doIIRO8Cnt;
			               status ++;
			       }
		 	       break;
			default:
				break;
		   }
		}
		if(ioMemData->model[ii] == GSC_16AI64SSA) adcCnt ++;
		if(ioMemData->model[ii] == GSC_16AO16) dacCnt ++;
		if(ioMemData->model[ii] == GSC_18AO8) dac18Cnt ++;
		if(ioMemData->model[ii] == CON_6464DIO) doCnt ++;
		if(ioMemData->model[ii] == CON_32DO) do32Cnt ++;
		if(ioMemData->model[ii] == ACS_16DIO) doIIRO16Cnt ++;
		if(ioMemData->model[ii] == ACS_8DIO) doIIRO8Cnt ++;
	}
	// If no ADC cards were found, then SLAVE cannot run
	if(!cdsPciModules.adcCount)
	{
		printf("No ADC cards found - exiting\n");
		return -1;
	}
	// This did not quite work for some reason
	// Need to find a way to handle skipped DAC cards in slaves
	//cdsPciModules.dacCount = ioMemData->dacCount;
#endif
	printf("%d PCI cards found \n",status);
	if(status < cards)
	{
		printf(" ERROR **** Did not find correct number of cards! Expected %d and Found %d\n",cards,status);
		cardCountErr = 1;
	}

	// Print out all the I/O information
        printf("***************************************************************************\n");
#ifdef ADC_MASTER
		// Master send module counds to SLAVE via ipc shm
		ioMemData->totalCards = status;
		ioMemData->adcCount = cdsPciModules.adcCount;
		ioMemData->dacCount = cdsPciModules.dacCount;
		ioMemData->bioCount = cdsPciModules.doCount;
		// kk will act as ioMem location counter for mapping modules
		kk = cdsPciModules.adcCount;
#endif
	printf("%d ADC cards found\n",cdsPciModules.adcCount);
	for(ii=0;ii<cdsPciModules.adcCount;ii++)
        {
#ifdef ADC_MASTER
		// MASTER maps ADC modules first in ipc shm for SLAVES
		ioMemData->model[ii] = cdsPciModules.adcType[ii];
		ioMemData->ipc[ii] = ii;	// ioData memory buffer location for SLAVE to use
#endif
                if(cdsPciModules.adcType[ii] == GSC_18AISS6C)
                {
                        printf("\tADC %d is a GSC_18AISS6C module\n",ii);
                        printf("\t\tChannels = 6 \n");
                        printf("\t\tFirmware Rev = %d \n\n",(cdsPciModules.adcConfig[ii] & 0xfff));
                }
                if(cdsPciModules.adcType[ii] == GSC_16AI64SSA)
                {
                        printf("\tADC %d is a GSC_16AI64SSA module\n",ii);
                        if((cdsPciModules.adcConfig[ii] & 0x10000) > 0) jj = 32;
                        else jj = 64;
                        printf("\t\tChannels = %d \n",jj);
                        printf("\t\tFirmware Rev = %d \n\n",(cdsPciModules.adcConfig[ii] & 0xfff));
                }
        }
        printf("***************************************************************************\n");
	printf("%d DAC cards found\n",cdsPciModules.dacCount);
	for(ii=0;ii<cdsPciModules.dacCount;ii++)
        {
                if(cdsPciModules.dacType[ii] == GSC_18AO8)
		{
                        printf("\tDAC %d is a GSC_18AO8 module\n",ii);
		}
                if(cdsPciModules.dacType[ii] == GSC_16AO16)
                {
                        printf("\tDAC %d is a GSC_16AO16 module\n",ii);
                        if((cdsPciModules.dacConfig[ii] & 0x10000) == 0x10000) jj = 8;
                        if((cdsPciModules.dacConfig[ii] & 0x20000) == 0x20000) jj = 12;
                        if((cdsPciModules.dacConfig[ii] & 0x30000) == 0x30000) jj = 16;
                        printf("\t\tChannels = %d \n",jj);
                        if((cdsPciModules.dacConfig[ii] & 0xC0000) == 0x0000)
			{
                        	printf("\t\tFilters = None\n");
			}
                        if((cdsPciModules.dacConfig[ii] & 0xC0000) == 0x40000)
			{
                        	printf("\t\tFilters = 10kHz\n");
			}
                        if((cdsPciModules.dacConfig[ii] & 0xC0000) == 0x80000)
			{
                        	printf("\t\tFilters = 100kHz\n");
			}
                        if((cdsPciModules.dacConfig[ii] & 0x100000) == 0x100000)
			{
                        	printf("\t\tOutput Type = Differential\n");
			}
                        printf("\t\tFirmware Rev = %d \n\n",(cdsPciModules.dacConfig[ii] & 0xfff));
                }
#ifdef ADC_MASTER
		// Pass DAC info to SLAVE processes
		ioMemData->model[kk] = cdsPciModules.dacType[ii];
		ioMemData->ipc[kk] = kk;
		// Following used by MASTER to point to ipc memory for inputting DAC data from SLAVES
                cdsPciModules.dacConfig[ii]  = kk;
printf("MASTER DAC SLOT %d %d\n",ii,cdsPciModules.dacConfig[ii]);
		kk ++;
#endif
	}
        printf("***************************************************************************\n");
	printf("%d DIO cards found\n",cdsPciModules.dioCount);
        printf("***************************************************************************\n");
	printf("%d IIRO-8 Isolated DIO cards found\n",cdsPciModules.iiroDioCount);
        printf("***************************************************************************\n");
	printf("%d IIRO-16 Isolated DIO cards found\n",cdsPciModules.iiroDio1Count);
        printf("***************************************************************************\n");
	printf("%d Contec 32ch PCIe DO cards found\n",cdsPciModules.cDo32lCount);
	printf("%d Contec PCIe DIO1616 cards found\n",cdsPciModules.cDio1616lCount);
	printf("%d Contec PCIe DIO6464 cards found\n",cdsPciModules.cDio6464lCount);
	printf("%d DO cards found\n",cdsPciModules.doCount);
#ifdef ADC_MASTER
	// MASTER sends DIO module information to SLAVES
	// Note that for DIO, SLAVE modules will perform the I/O directly and therefore need to
	// know the PCIe address of these modules.
	ioMemData->bioCount = cdsPciModules.doCount;
	for(ii=0;ii<cdsPciModules.doCount;ii++)
        {
		// MASTER needs to find Contec 1616 I/O card to control timing slave.
		if(cdsPciModules.doType[ii] == CON_1616DIO)
		{
			tdsControl[tdsCount] = ii;
			printf("TDS controller %d is at %d\n",tdsCount,ii);
			tdsCount ++;
		}
		ioMemData->model[kk] = cdsPciModules.doType[ii];
		// Unlike ADC and DAC, where a memory buffer number is passed, a PCIe address is passed
		// for DIO cards.
		ioMemData->ipc[kk] = cdsPciModules.pci_do[ii];
		kk ++;
	}
	printf("Total of %d I/O modules found and mapped\n",kk);
#endif
        printf("***************************************************************************\n");
// Following section maps Reflected Memory, both VMIC hardware style and Dolphin PCIe network style.
// Slave units will perform I/O transactions with RFM directly ie MASTER does not do RFM I/O.
// Master unit only maps the RFM I/O space and passes pointers to SLAVES.

#ifdef ADC_SLAVE
	// Slave gets RFM module count from MASTER.
	cdsPciModules.rfmCount = ioMemData->rfmCount;
	cdsPciModules.dolphinCount = ioMemData->dolphinCount;
	cdsPciModules.dolphin[0] = ioMemData->dolphin[0];
	cdsPciModules.dolphin[1] = ioMemData->dolphin[1];
#endif
	printf("%d RFM cards found\n",cdsPciModules.rfmCount);
#ifdef ADC_MASTER
	ioMemData->rfmCount = cdsPciModules.rfmCount;
#endif
	for(ii=0;ii<cdsPciModules.rfmCount;ii++)
        {
                 printf("\tRFM %d is a VMIC_%x module with Node ID %d\n", ii, cdsPciModules.rfmType[ii], cdsPciModules.rfmConfig[ii]);
#ifdef ADC_SLAVE
		cdsPciModules.pci_rfm[ii] = ioMemData->pci_rfm[ii];
		cdsPciModules.pci_rfm_dma[ii] = ioMemData->pci_rfm_dma[ii];
#endif
		printf("address is 0x%lx\n",cdsPciModules.pci_rfm[ii]);
#ifdef ADC_MASTER
		// Master sends RFM memory pointers to SLAVES
		ioMemData->pci_rfm[ii] = cdsPciModules.pci_rfm[ii];
		ioMemData->pci_rfm_dma[ii] = cdsPciModules.pci_rfm_dma[ii];
#endif
	}
	// ioMemData->dolphinCount = 0;
#ifdef DOLPHIN_TEST
	ioMemData->dolphinCount = cdsPciModules.dolphinCount;
	ioMemData->dolphin[0] = cdsPciModules.dolphin[0];
	ioMemData->dolphin[1] = cdsPciModules.dolphin[1];

#else
#ifdef ADC_MASTER
// Clear Dolphin pointers so the slave sees NULLs
	ioMemData->dolphinCount = 0;
        ioMemData->dolphin[0] = 0;
        ioMemData->dolphin[1] = 0;
#endif
#endif
        printf("***************************************************************************\n");
  	if (cdsPciModules.gps) {
	printf("IRIG-B card found %d\n",cdsPciModules.gpsType);
        printf("***************************************************************************\n");
  	}


	// Code will run on internal timer if no ADC modules are found
	if (cdsPciModules.adcCount == 0) {
		printf("No ADC modules found, running on timer\n");
		run_on_timer = 1;
        	//munmap(_epics_shm, MMAP_SIZE);
        	//close(wfd);
        	//return 0;
	}

	// Initialize buffer for daqLib.c code
	printf("Initializing space for daqLib buffers\n");
	daqBuffer = (long)&daqArea[0];
 
#ifndef NO_DAQ
#ifndef SHMEM_DAQ
	printf("Initializing Network\n");
	numFb = 1;
	if (numFb <= 0) {
		printf("Couldn't initialize Myrinet network connection\n");
		return -1;
	}
	printf("Found %d frameBuilders on network\n",numFb);
#endif
#endif


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
