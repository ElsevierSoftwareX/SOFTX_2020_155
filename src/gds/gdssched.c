static char *versionId = "Version $Id$" ;
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdssched						*/
/*                                                         		*/
/* Module Description: implements functions for scheduling tasks	*/
/* based on the heartbeat interrupt					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

#ifndef __EXTENSIONS__
#define __EXTENSIONS__
#endif
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

/* Header File List: */
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#ifdef OS_VXWORKS
#include <vxWorks.h>
#include <semLib.h>
#include <taskLib.h>
#include <sysLib.h>
#include <timers.h>

#else
#include <pthread.h>
#endif

#include "dtt/gdsutil.h"
#include "dtt/gdsheartbeat.h"
#include "dtt/gdstask.h"
#include "dtt/gdssched.h"
#ifndef NO_XDR
#include "dtt/gdsxdr_util.h"
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Constants: _DEFAULT_XDR_SIZE   max. length of an XDR'd task arg	*/
/*            _INIT_TASK_LIST 	  length of task list at start		*/
/*            _SCHEDULER_PRIORITY priority of the scheduler task	*/
/*            _SCHEDULER_NAME	  name of the scheduler task		*/
/*            _MAX_TASKS	  maximum number of running tasks	*/ 
/*				  per scheduler entry			*/
/*            _SCHED_PR_LOW 	  low priority task			*/
/*            _SCHED_PR_MED 	  medium priority task			*/
/*            _SCHED_PR_HIGH 	  high priority task			*/
/*            _MIN_TIME	 	  defines a minimum time for timenow 	*/
/*                                (scheduler ignores time values below)	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#ifdef OS_VXWORKS
#define _DEFAULT_XDR_SIZE     100000	/* 100kByte */
#endif
#define _INIT_TASK_LIST		1000
#define _MAX_TASKS		   5
#define _SCHEDULER_NAME		"tSched"
#ifdef OS_VXWORKS
#define _SCHEDULER_PRIORITY	  10
#define _SCHED_PR_LOW		  80
#define _SCHED_PR_MED		  50
#define _SCHED_PR_HIGH		  20
#else
#define _SCHEDULER_PRIORITY	   3
#define _SCHED_PR_LOW		  20
#define _SCHED_PR_MED		  12
#define _SCHED_PR_HIGH		   5
#endif
#define _MIN_TIME		 100

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Types: _schedproc_t	return argument of a thread			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#ifdef OS_VXWORKS
   typedef void 	_schedproc_t;
#else
   typedef void		_schedproc_t;
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Types: _epochtime_t	used to store time in number of epochs		*/
/*        _schedtime_t	structure to store current and old time when	*/
/*			the scheduler was called	 		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   typedef long long _epochtime_t;

   struct _schedtime_struct {
      tainsec_t		now;		/* current time */
      tai_t		taibd;		/* broken down current time */
      taisec_t		tai;		/* current TAI in sec */
      int		epoch;		/* current epoch */
      _epochtime_t 	tnow;		/* current number of epochs */
      taisec_t		oldtai;		/* old TAI in sec */
      int		oldepoch;	/* old epoch */
      _epochtime_t	told;		/* old number of epochs */
   };

   typedef struct _schedtime_struct _schedtime_t;


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Types: _schedarg_t	argument list which is passed to the thread	*/
/*			which calles the user function	 		*/
/*        _schedtask_t	scheduled task information 			*/
/* 	  _schedinfo_t  scheduler enrty information			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   typedef struct _schedinfo_struct _schedinfo_t;

   struct _schedarg_struct {
      _schedinfo_t*	info;		/* scheduler entry info */
      int		tindex;		/* index into task list */
      taisec_t		tai;		/* time when called */
      int		epoch;		/* epoch when called */
      int		result;		/* sync. tasks only */
   };
   typedef struct _schedarg_struct _schedarg_t;

   struct _schedtask_struct {
      int		valid;		/* entry in use */
      schedTID_t	tid;		/* task ID */
      int		terminated;	/* true if terminated */
      _schedarg_t	arg;		/* argument passed to the task */
      int		result;		/* returned value */
   };
   typedef struct _schedtask_struct _schedtask_t;

   struct _schedinfo_struct {
      schedulertask_t	info;		/* user provided info */
      int		id;		/* id number */
      scheduler_t*	sd;		/* pointer to scheduler */
      schedmutex_t	sem;		/* mutex to access data below */
      int		markForTermination; /* delete entry */
      int		firstScheduled; /* scheduled at least once */
      int		firstFinished;	/* first tasks has finished */
      int		lastFinished;	/* last task has finished */
      int		runningtasks;	/* number of running tasks */
      int 		repeatN;	/* down counter for repeat */
      tainsec_t		tinit;		/* time when initialized */
      _epochtime_t	tfirst;		/* time when task sched. first */
      _schedtask_t	tasks[_MAX_TASKS];/* individual task info */
   };


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Types: _timetaginfo_t	scheduled task information 		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   struct _timetaginfo_struct {
      char		tag [TIMETAG_LENGTH];
      taisec_t		tai;		/* expiration time */
      int		epoch;		/* expiration epoch */
   };

   typedef struct _timetaginfo_struct _timetaginfo_t;


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Forward declarations: 						*/
/*	checkValidity		check validity of an entry		*/
/*	schedulerProcess	main scheduler process			*/
/*      schedTask		scheduled task (which calles user func.)*/
/*      setTag			used to set a timetag			*/
/*      isTagExpired		returns the time since expiration	*/
/*      deleteTag		used to delete a tag			*/
/*      updateEndOfTask		used to set the 'finish' variables	*/
/*      cleanupFinishedTasks	used by sched. proc. to cleanup	tasks	*/
/*      evaluateTags		used by sched. proc. to evaluate tags	*/
/*      calcStartTime		returns adj. start time for rep. tasks	*/
/*      isTaskReady		evaluates which tasks are ready		*/
/*      scheduleReadyTasks	used by sched. proc. to start tasks	*/
/*      deleteRetiredEntries	used by sched. proc. to delete entries	*/
/*      deleteRetiredTags	used by sched. proc. to delete tags	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int _closeScheduler (scheduler_t* sd, tainsec_t timeout);
   static int _scheduleTask (scheduler_t* sd, 
                     const schedulertask_t* newtask);
   static int _getScheduledTask (scheduler_t* sd, int id, 
                     schedulertask_t* task);
   static int _removeScheduledTask (scheduler_t* sd, int id, 
                     int terminate);
   static int _waitForSchedulerToFinish (scheduler_t* sd, 
                     tainsec_t timeout);

   static int checkValidity (scheduler_t* sd, 
                     const schedulertask_t* entry);
   static _schedproc_t schedulerProcess (scheduler_t* sd);
   static _schedproc_t schedTask (_schedarg_t* arg);
   static void setTag (scheduler_t* sd, const char* tag, 
                     taisec_t tai, int epoch, int nonotify);
   static int isTagExpired (const scheduler_t* sd, const char* tag, 
                     taisec_t tai, int epoch);
   static void deleteTag (scheduler_t* sd, const char* tag);
   static void updateEndOfTask (_schedinfo_t* info, int result);
   static void cleanupFinishedTasks (scheduler_t* sd);
   static void evaluateTags (scheduler_t* sd, taisec_t tai, int epoch,
                     int tagtype);
   static _epochtime_t calcStartTime (const _schedinfo_t* info, 
                     const _schedtime_t* time);
   static int isTaskReady (const _schedinfo_t*info,
                     const _schedtime_t* time,
                     const int synclist[NUMBER_OF_EPOCHS],
                     _epochtime_t* tfirst);
   static void scheduleReadyTasks (scheduler_t* sd,
                     const _schedtime_t* time,
                     const int synclist[NUMBER_OF_EPOCHS]);
   static void freeResources (const _schedinfo_t* info, 
                     const _schedtime_t* time);
   static void deleteRetiredEntries (scheduler_t* sd, 
                     const _schedtime_t* time);
   static void deleteRetiredTags (scheduler_t* sd, int* testtag);


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: createScheduler				*/
/*                                                         		*/
/* Procedure Description: creates a scheduler and initializes it	*/
/*                                                         		*/
/* Procedure Arguments: flags, setup function				*/
/*                                                         		*/
/* Procedure Returns: scheduler_t if successful, NULL when failed	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   scheduler_t* createScheduler (int flags, int (*setup) (scheduler_t* sd),
                     void* data)
   {
      scheduler_t*	sched;		/* scheduler */
      int		attr;		/* task creation attributes */
   
      /* allocate memory for scheduler struct */
      sched = (scheduler_t*) malloc (sizeof (scheduler_t));
      if (sched == NULL) {
         free (data);
         return NULL;
      }
   
      /* initialize flags, heartbeat rate and functions */
      sched->scheduler_flags = flags;
      sched->heartbeatsPerSec = NUMBER_OF_EPOCHS;
      sched->markedForTermination = 0;
      sched->data = data;
      sched->tasklist = NULL;
      sched->maxtasklist = 0;
      sched->timetaglist = NULL;
      sched->maxtimetaglist = 0;
      sched->sync = syncWithHeartbeat;
      sched->timenow = TAInow;
      sched->closeScheduler = _closeScheduler;
      sched->scheduleTask = _scheduleTask;
      sched->getScheduledTask = _getScheduledTask;
      sched->removeScheduledTask = _removeScheduledTask;
      sched->waitForSchedulerToFinish = _waitForSchedulerToFinish;
      sched->setTagNotify = NULL;
      sched->scheduler_tid = 0;
   
      /* initialize the scheduler mutex */
      if (MUTEX_CREATE (sched->sem) != 0) {
         free (data);
         free (sched);
         return NULL;
      }
   
      /* allocate memory for task list  and initializes to empty */
      if ((flags & SCHED_NOPROC) == 0) {
         sched->maxtasklist = _INIT_TASK_LIST;
         sched->tasklist = (schedulertask_t**) calloc (_INIT_TASK_LIST, 
                                             sizeof (schedulertask_t*));
         if (sched->tasklist == NULL) {
            free (data);
            free (sched);
            return NULL;
         }
         sched->tasklist[0] = NULL;
      
         /* allocate memory for timetag list and initialize to empty */
         sched->maxtimetaglist = _INIT_TASK_LIST;
         sched->timetaglist = (char**) calloc (_INIT_TASK_LIST,
                                              sizeof (char*));
         if (sched->timetaglist == NULL) {
            closeScheduler (sched, 0);
         }
      }
   
      /* call setup routine */
      if ((setup != NULL) && (setup (sched) != 0)) {
         closeScheduler (sched, 0);
         return NULL;
      }
   
      /* don't spawn scheduler task if flag has SCHED_NOPROC */
      if ((flags & SCHED_NOPROC) != 0) {
         sched->scheduler_tid = 0;
         return sched;
      };
   
      /* start scheduler process */
   #ifdef OS_VXWORKS
      attr = VX_FP_TASK;
   #else
      attr = PTHREAD_SCOPE_SYSTEM | PTHREAD_CREATE_DETACHED;
   #endif
      if (taskCreate (attr, _SCHEDULER_PRIORITY, &sched->scheduler_tid,
         _SCHEDULER_NAME, (taskfunc_t) schedulerProcess, 
         (taskarg_t) sched) < 0) {
         /* close scheduler if task creation was unsuccessful */
         closeScheduler (sched, 0);
         return NULL;
      }
   
      /* return newly allocated scheduler struct */
      return sched;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: closeScheduler				*/
/*                                                         		*/
/* Procedure Description: destroys a schedulerand frees its memory	*/
/*                                                         		*/
/* Procedure Arguments: sd scheduler, timeout				*/
/*                                                         		*/
/* Procedure Returns:0 if successful, negative error code when failed	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int closeScheduler (scheduler_t* sd, tainsec_t timeout)       
   {
      if (sd == NULL) {
         return 0;
      } 
      else {
         return sd->closeScheduler (sd, timeout);
      }
   }

   static int _closeScheduler (scheduler_t* sd, tainsec_t timeout)
   {
      int	i;
   
      /* tests whether valid pointer to scheduler */
      if (sd == NULL) {
         return 0;
      }
   
      /* mark for termination */
      sd->markedForTermination = 1;
   
      /* wait for termination and then remove all tasks */
      if (timeout == 0) {
         removeScheduledTask (sd, -1, SCHED_NOWAIT);
      }
      else {
         if (waitForSchedulerToFinish (sd, timeout) != 0) {
            removeScheduledTask (sd, -1, SCHED_NOWAIT);
         
            /* wait for cleanup, but only a couple of epochs */
            if (waitForSchedulerToFinish (sd, 3 * _EPOCH) != 0) {
               return -2;
            }
         }
      }
   
      /* get the mutex first */
      if (MUTEX_TRY (sd->sem) != 0) {
         gdsDebug ("Couldn't get scheduler mutex"); /* proceed anyway */
      }
      /* cancel the scheduler process before freeing memory */
      taskCancel (&sd->scheduler_tid);
   
      /* destroy mutex */
      if (MUTEX_DESTROY (sd->sem) != 0) {
         gdsDebug ("Couldn't destroy scheduler mutex");
      }
   
      /* free the memory and return */
      if (sd->tasklist != NULL) {
         free (sd->tasklist);
      }
      if (sd->timetaglist != NULL) {
         for (i = 0; (i < sd->maxtimetaglist) && 
             (sd->timetaglist[i] != NULL); i++) {
            free (sd->timetaglist[i]);
         }
         free (sd->timetaglist);
      }
      free (sd->data);
      free (sd);
   
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: checkValidity				*/
/*                                                         		*/
/* Procedure Description: check the validity of an entry		*/
/* 									*/
/* Procedure Arguments: sd -  scheduler					*/
/*			entry - pointer to scheduler entry info struct	*/
/*									*/
/* Procedure Returns: 0 if valid; neg. error number otherwise		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int checkValidity (scheduler_t* sd, 
                     const schedulertask_t* entry)
   {
      int	i;	/* temp var */
   
      /* check timeout */
      if ((entry->flag & SCHED_TIMEOUT) != 0) {
         if (entry->timeout < 0) {
            return -10;		/* negative timeout */
         }
         if (((entry->flag & SCHED_MAXTIME_LONG) == 0) && 
            (entry->timeout > _ONEHOUR)) {
            return -11;		/* timeout > 1h */
         }
      }
   
      /* chech priority */
      if (((entry->flag & SCHED_PRIORITY_USER) != 0) && 
         ((entry->priority < 0) || 
         (entry->priority >= _SCHEDULER_PRIORITY))) {
         return -12;
      }
   
   /* check wait stuff */
      if ((entry->flag & SCHED_WAIT) != 0) {
         /* check wait type */
         if ((entry->waittype < 0) || 
            (entry->waittype > SCHED_WAIT_TAGEPOCHDELAYED)) {
            return -13;
         }
      
         /* check wait value */
         if ((entry->waittype == SCHED_WAIT_TAGDELAYED) ||
            (entry->waittype == SCHED_WAIT_TAGEPOCHDELAYED) ||
            (entry->waittype == SCHED_WAIT_TIMEDELAY) ||
            (entry->waittype == SCHED_WAIT_EPOCHDELAY) ||
            (entry->waittype == SCHED_WAIT_STARTTIME)) {
            if (entry->waitval < 0) {
               return -14;
            }
            /* wait longer than an hour */
            if (((entry->flag & SCHED_MAXTIME_LONG) == 0) &&
               ((entry->waittype == SCHED_WAIT_TAGDELAYED) ||
               (entry->waittype == SCHED_WAIT_TIMEDELAY)) &&
               (entry->waitval > _ONEHOUR)) {
               return -15;
            }
            /* wait longer than an hour */
            if (((entry->flag & SCHED_MAXTIME_LONG) == 0) &&
               ((entry->waittype == SCHED_WAIT_TAGEPOCHDELAYED) ||
               (entry->waittype == SCHED_WAIT_EPOCHDELAY)) &&
               (entry->waitval * _EPOCH > _ONEHOUR)) {
               return -15;
            }
            /* start time more than an hour in the future */
            if (((entry->flag & SCHED_MAXTIME_LONG) == 0) &&
               (entry->waittype == SCHED_WAIT_STARTTIME) &&
               (entry->waittype + _ONEHOUR > sd->timenow())) {
               return -15;
            }
         }
      
      	 /* check wait tag */
         if ((entry->waittype == SCHED_WAIT_TAG) ||
            (entry->waittype == SCHED_WAIT_TAGDELAYED) ||
            (entry->waittype == SCHED_WAIT_TAGEPOCHDELAYED)) {
            for (i = 0; i < TIMETAG_LENGTH; i++) {
               if (entry->waittag[i] == '\0') {
                  break;
               }
            }
            if (i == TIMETAG_LENGTH) {
               return -16;
            }
         }
      }
   
      /* check sync stuff */
      if ((entry->flag & SCHED_WAIT) != 0) {
         /* check sync type */
         if ((entry->synctype < 0) || 
            (entry->synctype > SCHED_SYNC_IEPOCH)) {
            return -17;
         }
      
         /* check wait sync val */
         switch (entry->synctype)
         {
            case SCHED_SYNC_IEPOCH:
               {
                  if (entry->syncval < 1) {
                     return -18;
                  }
                  break;
               }
            case SCHED_SYNC_EPOCH:
               {
                  if ((entry->syncval < 0) || 
                     (entry->syncval >= NUMBER_OF_EPOCHS)) {
                     return -18;
                  }
                  break;
               }
            default: 
               {
                  break;
               }
         }
      }
   
      /* check time tag stuff */
      if ((entry->flag & SCHED_TIMETAG) != 0) {
         /* check time tag name */
         for (i = 0; i < TIMETAG_LENGTH; i++) {
            if (entry->timetag[i] == '\0') {
               break;
            }
         }
         if (i == TIMETAG_LENGTH) {
            return -19;
         }
      
      	/* check tagtype */
         if ((entry->tagtype < 0) || (entry->tagtype > SCHED_TAG_LAST)) {
            return -20;
         }
      }
   
      /* check repeat stuff */
      if ((entry->flag & SCHED_REPEAT) != 0) {
         /* check repeat type */
         if ((entry->repeattype < 0) || 
            (entry->repeattype > SCHED_REPEAT_COND)) {
            return -21;
         }
      
      	/* check repeat value */
         if ((entry->repeattype == SCHED_REPEAT_N) &&
            (entry->repeatval < 1)) {
            return -22;
         }
      
      	/* check repeat rate type */
         if ((entry->repeatratetype < 0) || 
            (entry->repeatratetype > SCHED_REPEAT_IEPOCH)) {
            return -23;
         }
      
      	/* check repeat sync type */
         if ((entry->repeatsynctype < 0) || 
            (entry->repeatsynctype > SCHED_SYNC_IEPOCH)) {
            return -24;
         }
         if ((entry->repeatratetype == SCHED_REPEAT_EPOCH) &&
            (entry->repeatsynctype != SCHED_SYNC_NEXT)) {
            return -24;
         }
      
      	/* check repeat sync value */
         switch (entry->repeatsynctype)
         {
            case SCHED_SYNC_EPOCH :
               {
                  if ((entry->repeatsyncval < 0) || 
                     (entry->repeatsyncval >= NUMBER_OF_EPOCHS)) {
                     return -25;
                  }
                  break;
               }
            case SCHED_SYNC_IEPOCH :
               {
                  if (entry->repeatsyncval < 1) {
                     return -25;
                  }
                  break;
               }
            default :
               {
                  break;
               }
         }
      }
   
      /* check function pointer */
      if (entry->func == NULL) {
         return -26;
      }
   
      /* all checks passed */
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: scheduleTask				*/
/*                                                         		*/
/* Procedure Description: add an entry to the scheduler			*/
/* 									*/
/* Procedure Arguments: sd: scheduler;		 			*/
/* 			newtask: pointer to scheduler entry info struct	*/
/*									*/
/* Procedure Returns: neg. error number if failed; entry id otherwise	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int scheduleTask (scheduler_t* sd, const schedulertask_t* newtask)
   {
      if (sd == NULL) {
         return -1;
      } 
      else {
         return sd->scheduleTask (sd, newtask);
      }
   }

   static int _scheduleTask (scheduler_t* sd, 
                     const schedulertask_t* newtask)
   {
      static int	newid = 0; /* new entry id */
      int		indx = 0;  /* used as index */
      _schedinfo_t*	info;	/* pointer to scheduler entry info */
      _schedinfo_t*	new;	/* pointer to new entry */
      int		err;	/* error number */
      int		found;	/* found index */
      int		i;	/* used as index into list of tasks */
      schedulertask_t**	newlist;/* new list if old one is too short */
   
      if ((sd == NULL) || (newtask == NULL) || (sd->tasklist == NULL)) {
         return -1;
      }
   
      /* check newtask for validity */
      err = checkValidity (sd, newtask);
      if (err != 0) {
         return err;
      }
   
      /* get scheduler mutex */
      if (MUTEX_GET (sd->sem) != 0) {
         gdsWarningMessage ("Failure to obtain scheduler semaphore");
         return -2;
      }
   
      /* allocate memory for new entry info structure */ 
      new = malloc (sizeof (_schedinfo_t));
      if (new == NULL) {
         gdsError (GDS_ERR_MEM, "Can't add new scheduler entry");
         err = -3;
      } 
      else {
        /* fill in defaults */
         new->info = *newtask;
         new->sd = sd;
         new->markForTermination = 0;
         new->firstScheduled = 0;
         new->firstFinished = 0;
         new->lastFinished = 0;
         new->runningtasks = 0;
         new->repeatN = 0;
         new->tfirst = 0;
         new->tinit = sd->timenow();
         for (i = 0; i < _MAX_TASKS; i++) {
            new->tasks[i].valid =0;
            new->tasks[i].terminated =0;
            new->tasks[i].result =0;
            new->tasks[i].tid =0;
         }
      	 /* fill in default for no wait flag */
         if ((new->info.flag & SCHED_WAIT) == 0) {
            new->info.synctype = SCHED_SYNC_NEXT;
            new->info.waittype = SCHED_WAIT_IMMEDIATE;
         }	
      	 /* set default priority if necessary */
         if ((new->info.flag & SCHED_PRIORITY_USER) == 0) {
            new->info.priority = _SCHED_PR_MED;
         }
      	 /* fill in repeat N if necessary */
         if (((new->info.flag & SCHED_REPEAT) != 0) &&
            (new->info.repeattype == SCHED_REPEAT_N)) {
            new->repeatN = new->info.repeatval;
         }
      
         if (MUTEX_CREATE (new->sem) != 0) {
            err = -4;
            free (new);
         }
         else {
            do  {
               newid = (newid > 1000000) ? 1 : newid + 1;
               found = 0;
               for (indx = 0; (indx < sd->maxtasklist) &&
                   (sd->tasklist[indx] != NULL); indx++) {
                  info = (_schedinfo_t*) sd->tasklist[indx];
                  if (info->id == newid) {
                     found = 1;
                     break;
                  }
               }
            } while (found == 1);
            new->id = newid;
         }
      }
   
      /* xdr stuff */
   #ifndef NO_XDR
      if ((err == 0) && (newtask->xdr_arg != NULL) && 
         (newtask->arg != NULL)) 
      {
         unsigned int		size;	/* size of xdr stream */
         char*			temp;	/* temporary memory */
      
         err = xdr_encodeArgument (newtask->arg, &temp, &size, 
                                 newtask->xdr_arg);
         if (err == 0) {
            err = xdr_decodeArgument ((char**) &new->info.arg, 
                                    new->info.arg_sizeof, temp, size, 
                                    new->info.xdr_arg);
            free (temp);
         }
      }
   #endif
   
      /* make sure list is long enough */
      if (err ==  0) {
         indx = 0;
      	 /* find number of entries */
         while ((indx < sd->maxtasklist) &&
               (sd->tasklist[indx] != NULL)) {
            indx++;
         }
      	/* test if long enough */
         if (indx + 2 >= sd->maxtasklist) {
            /* make it longer */
            gdsDebug ("make task list longer");
            newlist = realloc (sd->tasklist, sizeof (schedulertask_t*) *
                              (sd->maxtasklist + _INIT_TASK_LIST));
            if (newlist == NULL) {
               err = -3;
            } 
            else {
               sd->tasklist = newlist;
               sd->maxtasklist += _INIT_TASK_LIST;
            }
         }
      }
   
      /* find the right spot (priority-wise) */
      if (err == 0) {
      
      	 /* search through list */
         found = 0;
         while ((indx < sd->maxtasklist) &&
               (sd->tasklist[indx] != NULL) &&
               (sd->tasklist[indx]->priority >= new->info.priority)) {
            found++;
         }
      
         /* now add it to the list */
         indx = found;
         while ((indx < sd->maxtasklist) &&
               (sd->tasklist[indx] != NULL)) {
            indx++;
         }
         while (indx >= found) {
            sd->tasklist[indx + 1] = sd->tasklist[indx];
            indx--;
         }
         sd->tasklist[found] = (schedulertask_t*) new;
      }
   
      /* release scheduler mutex if it was taken */
      if (MUTEX_RELEASE (sd->sem) != 0) {
         gdsError (GDS_ERR_PROG,
                  "Failure to release scheduler semaphore");
      }
   
      /* return error code if failed and new id if successful */
      if (err < 0) {
         return err;
      }
      else {
         return newid;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: getScheduledTask				*/
/*                                                         		*/
/* Procedure Description: get the task info with the given id		*/
/* 									*/
/* Procedure Arguments: sd: scheduler; id: entry id; 			*/
/* 			task: pointer to scheduler entry info structure	*/ 
/*			      for storing result			*/
/*									*/
/* Procedure Returns: <0 error; -5 if not found; 			*/
/*		      next higher id, or lowest if last			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int getScheduledTask (scheduler_t* sd, int id, schedulertask_t* task)
   {
      if (sd == NULL) {
         return -1;
      } 
      else {
         return sd->getScheduledTask (sd, id, task);
      }
   }

   static int _getScheduledTask (scheduler_t* sd, int id, 
                     schedulertask_t* task)
   {
      int		semstat;/* status of mutex */
      int		goodid;	/* task id used for search */
      int		found;	/* used as index for found entry */
      int		nextid;	/* next higher valid id */
      int		indx;	/* used as index */
      _schedinfo_t*	info;	/* pointer to scheduler entry info */
   
      /* return if invalid arguments */
      if ((sd == NULL) || (sd->tasklist == NULL)) {
         return -1;
      }
   
      /* get scheduler mutex
         POSIX: returns EDEADLK if task alreday owns mutex.
         VxWorks: can handle recursive mutex */
      semstat = MUTEX_GET (sd->sem);
      if ((semstat != 0) && (semstat != EDEADLK)) {
         gdsWarningMessage ("Failure to obtain scheduler semaphore");
         return -2;
      }
   
      /* handle negative id numbers */
      goodid = id;
      if ((id < 0) && (sd->tasklist[0] != NULL)) {
      
         /* find the lowest valid id number */
         goodid = ((_schedinfo_t*) sd->tasklist[0])->id;
         for (indx = 1; (indx < sd->maxtasklist) &&
             (sd->tasklist[indx] != NULL); indx++) {
            info = (_schedinfo_t*)  sd->tasklist[indx];
            if (info->id < goodid) {
               goodid = info->id;
            }
         }
      }
   
      /* go through list and find entry */
      for (indx = 0, found = -1;
          (indx < sd->maxtasklist) &&
          (sd->tasklist[indx] != NULL); indx++) {
         info = (_schedinfo_t*)  sd->tasklist[indx];
         if (info->id == goodid) {
            /* found it */
            if (task != NULL) {
               *task = info->info;
            }
            found = indx;
            break;
         }
      }
   
      /* if found, find next higher id */
      if (found >= 0) {
         nextid = goodid;
         for (indx = 0;
             (indx < sd->maxtasklist) &&
             (sd->tasklist[indx] != NULL); indx++) {
            info = (_schedinfo_t*)  sd->tasklist[indx];
            if ((info->id > goodid) && 
               ((nextid == goodid) || (nextid > info->id))) {
               nextid = info->id;
            }
         }
      
         /* handle case where the next id doesn't exist */
         if (nextid == goodid) {
            /* find the lowest valid id number */
            nextid = ((_schedinfo_t*) sd->tasklist[0])->id;
            for (indx = 1; (indx < sd->maxtasklist) &&
                (sd->tasklist[indx] != NULL); indx++) {
               info = (_schedinfo_t*)  sd->tasklist[indx];
               if (info->id < nextid) {
                  nextid = info->id;
               }
            }
         }
      }
      else {
         nextid = -5;	/* task info was not found */
      }
   
      /* release scheduler mutex if it was taken */
      if ((semstat != EDEADLK) && (MUTEX_RELEASE (sd->sem) != 0)) {
         gdsError (GDS_ERR_PROG,
                  "Failure to release scheduler semaphore");
      }
   
      /* return next id or error */
      return nextid;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: removeScheduledTask				*/
/*                                                         		*/
/* Procedure Description: removes the task with the given id		*/
/* 									*/
/*                                                         		*/
/* Procedure Arguments: sd: scheduler; id: entry id; 			*/
/* 			task: pointer to scheduler entry info structure	*/ 
/*			      for storing result			*/
/*									*/
/* Procedure Returns: 0 if found, <0 otherwise				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int removeScheduledTask (scheduler_t* sd, int id, int terminate)
   {
      if (sd == NULL) {
         return -1;
      } 
      else {
         return sd->removeScheduledTask (sd, id, terminate);
      }
   }

   static int _removeScheduledTask (scheduler_t* sd, int id, 
                     int terminate)
   {
      int		semstat;/* status of mutex */
      int		indx;	/* used as index */
      _schedinfo_t*	info;	/* pointer to scheduler entry info */
      int		err;	/* used for error number */
      int		i;	/* used as index into tid list */
   
      /* return if invalid arguments */
      if ((sd == NULL) || (sd->tasklist == NULL)) {
         return -1;
      }
      err = 0;
   
      /* get scheduler mutex
         POSIX: returns EDEADLK if task alreday owns mutex.
         VxWorks: can handle recursive mutex */
      semstat = MUTEX_GET (sd->sem);
      if ((semstat != 0) && (semstat != EDEADLK)) {
         gdsWarningMessage ("Failure to obtain scheduler semaphore");
         return -2;
      }
   
      /* first mark task(s) for termination */
      for (indx = 0;
          (indx < sd->maxtasklist) &&
          (sd->tasklist[indx] != NULL); indx++) {
         info = (_schedinfo_t*)  sd->tasklist[indx];
         if ((id < 0) || (info->id == id)) {
         
            /* found it: first mark task(s) for termination */
            info->markForTermination = 1;
         
            /* now handle brute-force termination */
            if (((info->info.flag & SCHED_ASYNC) != 0) &&
               (terminate == SCHED_NOWAIT)) {
            
               /* get task mutex */
               if (MUTEX_GET (info->sem) != 0) {
                  gdsWarningMessage ("Failure to obtain "
                                    "scheduler entry semaphore");
                  err = -6;
                  continue;
               }
            
               /* look for running tasks and cancel them */
               for (i = 0; i < _MAX_TASKS; i++) {
               
                  /* check for validity */
                  if (info->tasks[i].valid == 0) {
                     continue;
                  }
               
                  /* send cancel signal */
               #ifdef OS_VXWORKS
                  if (taskDelete (info->tasks[i].tid) == OK) {
                     info->tasks[i].terminated = 1;
                  }
               #else
                  if (pthread_cancel (info->tasks[i].tid) == 0) {
                     info->tasks[i].terminated = 1;
                  }
               #endif
               }
            
               /* release entry mutex */
               if (MUTEX_RELEASE (info->sem) != 0) {
                  gdsError (GDS_ERR_PROG, 
                           "Failure to release scheduler entry semaphore");
               }
            }
         }
      }
   
      /* release scheduler mutex if it was taken */
      if ((semstat != EDEADLK) && (MUTEX_RELEASE (sd->sem) != 0)) {
         gdsError (GDS_ERR_PROG,
                  "Failure to release scheduler semaphore");
      }
   
      /* return err */
      return err;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: waitForSchedulerToFinish			*/
/*                                                         		*/
/* Procedure Description: waits for the scheduler to finish		*/
/*                                                         		*/
/* Procedure Arguments: sd: scheduler; timeout: 0 no, <0 forever	*/
/*                                                         		*/
/* Procedure Returns: 0 if finished, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int waitForSchedulerToFinish (scheduler_t* sd, tainsec_t timeout)
   {
      if (sd == NULL) {
         return -1;
      } 
      else {
         return sd->waitForSchedulerToFinish (sd, timeout);
      }
   }

   static int _waitForSchedulerToFinish (scheduler_t* sd, 
                     tainsec_t timeout)
   {
      tainsec_t		start = 0;
      tainsec_t		now = 0;
      struct timespec	delay = {0, _ONESEC/10}; /* 100ms */
   
      if ((sd == NULL) || (sd->tasklist == NULL)) {
         return 0;	/* this scheduler is finished! */
      }
   
      /* handle timeout */
      if (timeout != 0) {
      
         /* get current time */
         if (timeout > 0) {
            now = sd->timenow();
            start = now;
         }
      
         /* wait if necessary */
         while ((timeout < 0) || (start + timeout > now)) {
            /* return when scheduler empty */
            if (sd->tasklist[0] == NULL) {
               return 0;
            }
         
            /* go to sleep for a little while */
            nanosleep (&delay, NULL);
         
            /* update the current time */
            if (timeout > 0) {
               now = sd->timenow();
            }
         }
      }
   
      if (sd->tasklist[0] == NULL) {
         return 0;
      }
      else {
         return -7;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: setSchedulerTag				*/
/*                                                         		*/
/* Procedure Description: sets the time tag				*/
/*                                                         		*/
/* Procedure Arguments: sd: scheduler; timeout: 0 no, <0 forever	*/
/*                                                         		*/
/* Procedure Returns: 0 if finished, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int setSchedulerTag (scheduler_t* sd, const char* tag,
                     tainsec_t time, int nonotify)
   {
      int		semstat; 	/* status of mutex */
      tainsec_t		tagtime;	/* tag time */
      taisec_t 		tai;		/* tai of time tag */
      int		epoch;		/* epoch of time tag */
   
      if (sd == NULL) {
         return -1;
      }
      if (tag == NULL) {
         return -8;
      }
   
      /* get tag time */
      if (time == 0) {
         tagtime = sd->timenow();
      }
      else {
         tagtime = time;
      }
   
      /* calculate time/epoch */
      tai = tagtime / 1000000000LL;
      epoch = ((tagtime % 1000000000LL) + (_EPOCH / 2)) / _EPOCH;
      if (epoch >= NUMBER_OF_EPOCHS) {
         epoch -= NUMBER_OF_EPOCHS;
         tai++;
      }
   
      /* get scheduler mutex
         POSIX: returns EDEADLK if task alreday owns mutex.
         VxWorks: can handle recursive mutex */
      semstat = MUTEX_GET (sd->sem);
      if ((semstat != 0) && (semstat != EDEADLK)) {
         gdsWarningMessage ("Failure to obtain scheduler semaphore");
         return -2;
      }
   
      /* now set the tag */
      setTag (sd, tag, tai, epoch, nonotify);
   
      /* release scheduler mutex if it was taken */
      if ((semstat != EDEADLK) && (MUTEX_RELEASE (sd->sem) != 0)) {
         gdsError (GDS_ERR_PROG,
                  "Failure to release scheduler semaphore");
      }
   
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: schedulerProcess				*/
/*                                                         		*/
/* Procedure Description: syncs with heartbeat and makes the scheduling	*/
/* 									*/
/*                                                         		*/
/* Procedure Arguments: sd scheduler					*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static _schedproc_t schedulerProcess (scheduler_t* sd)
   {
      _schedtime_t	time;		/* current and old time */
      int		i;		/* index in synclist */
      int		testtag;	/* tag to be tested */
      int		synclist[NUMBER_OF_EPOCHS];	
         /* list of epochs which are considered to be sync-ed */
   
      /* initialize time record */
      time.oldepoch = -1;
      time.oldtai = 0;
      time.told = 0;
      /* initialize testtag */
      testtag = 0;
   
      /* never to return */
      while (1) {
         /* wait for next heartbeat */
         sd->sync();
      
       	 /* quick check if scheduler is empty */
         if (sd->tasklist[0] == NULL) {
            continue;
         }
      
      	 /* obtain current time and calculate epoch */
         time.now = sd->timenow();
         TAIsec (time.now, &time.taibd);
         time.tai = time.taibd.tai;
         time.epoch = (time.taibd.nsec + (_EPOCH / 10)) / _EPOCH;
         if (time.epoch >= NUMBER_OF_EPOCHS) {
            time.epoch -= NUMBER_OF_EPOCHS;
            time.tai++;
         }
         time.tnow = ((_epochtime_t) time.tai) * NUMBER_OF_EPOCHS + 
                     time.epoch;
      
         /* check if valid time */
         if (time.tnow < (_epochtime_t) _MIN_TIME * NUMBER_OF_EPOCHS) {
            continue;
         }
      
      #if 0
         {
            struct sched_param	prm;
            int			policy;
            if (pthread_getschedparam (sd->scheduler_tid, &policy, &prm)) {
               printf ("Priotity error, TID = %i\n", sd->scheduler_tid);
            }
            printf ("Scheduler priority = %i\n", prm.sched_priority);
            continue;
         }
      #endif
      #if 0
      
         {
            static int 	count = 0;
            if ((count % 16) == 1) {
               printf ("beat\n");
            }
            count++;
         }
      /* printf stuff */
         {
            utc_t	utc;
            char	buf[100];
            strftime (buf, 100, "%c", TAItoUTC (time.tai, &utc));
            printf ("beat at epoch = %i (%s)\n", time.epoch, buf);
         }
      #endif
      
         /* clean up finished tasks first */
         cleanupFinishedTasks (sd);
      
      	 /* evaluate finished tags and deletes finished tasks */
         evaluateTags (sd, time.tai, time.epoch, 1);
      
         /* check whether an epoch was lost */
         if ((time.oldepoch != -1) && (time.told + 1 < time.tnow)) {
            gdsDebug ("lost an epoch");
         }
      
         /* calculate synchronization table */
         for (i = 0; i < NUMBER_OF_EPOCHS; i++) {
            if (((time.oldepoch == -1) && (i == time.epoch)) ||
               ((time.oldepoch != -1) && 
               ((_epochtime_t) (time.tai - time.oldtai ) * NUMBER_OF_EPOCHS + 
               i - ((i > time.epoch) ? NUMBER_OF_EPOCHS : 0) > 
               time.oldepoch))) {
               synclist[i] = 1;
            }
            else {
               synclist[i] = 0;
            }
         }
      
         /* schedule ready tasks */
         scheduleReadyTasks (sd, &time, synclist);
      
      	 /* check for first scheduled tags */
         evaluateTags (sd, time.tai, time.epoch, 0);
      
         /* check for retired entries and tags */ 
         deleteRetiredEntries (sd, &time);
         deleteRetiredTags (sd, &testtag);
      
         /* set old tai and epoch */
         time.oldtai = time.tai;
         time.oldepoch = time.epoch;
         time.told = time.tnow;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: schedTask					*/
/*                                                         		*/
/* Procedure Description: this is the standard task which has		*/
/*           to call the user function of a scheduler entry		*/
/*                                                         		*/
/* Procedure Arguments: arg - pointer to a list of arguments		*/
/*                                                         		*/
/* Procedure Returns: exit status					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static _schedproc_t schedTask (_schedarg_t* arg)
   {
      schedulertask_t*	taskinfo;	/* pointer to the task info */
      int		status;		/* return argument of func. */
   
      /* setup variables */
      taskinfo = &(arg->info->info);
   
      /* call user function */
      status = taskinfo->func (taskinfo, arg->tai, arg->epoch, 
                              taskinfo->arg);
   
      /* remember to set the return value */
      if ((taskinfo->flag & SCHED_ASYNC) == 0) {
         /* sync. task */
         arg->result = status;
      }
      else {
         /* don't need mutex */
         arg->info->tasks[arg->tindex].result = status;
      }
   
      /* remember to set the terminate flag when asynchronous */
      if ((taskinfo->flag & SCHED_ASYNC) != 0) {
      
         /* set the terminated flag: this is atomic, don't need mutex */
         arg->info->tasks[arg->tindex].terminated = 1;
      
         /* exit properly from async. tasks */
      #ifdef OS_VXWORKS
         exit (OK);
      #else
         pthread_exit (0);
      #endif
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: setTag					*/
/*                                                         		*/
/* Procedure Description: sets a timetag to be expired			*/
/*           The calling task must own the scheduler mutex! 		*/
/*                                                         		*/
/* Procedure Arguments: sd scheduler, tag name of timetag		*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void setTag (scheduler_t* sd, const char* tag,
                     taisec_t tai, int epoch, int nonotify)
   {
      int		i;
      int		retval;
      char**		newlist;
      _timetaginfo_t* 	ltag;
   
      /* return when invalid arguments */
      if ((sd == NULL) || (tag == NULL)) {
         return;
      }
   
      /* call tag notify routine */
      if ((nonotify == 0) && (sd->setTagNotify != NULL)) {
         /* first release scheduler mutex to prevent dead locks */
         if (MUTEX_RELEASE (sd->sem) != 0) {
            gdsError (GDS_ERR_PROG,
                     "Failure to release scheduler semaphore");
            return;
         }
         /* now call notify routine */
         retval = sd->setTagNotify (sd, tag, tai, epoch);
         /* get scheduler mutex again */
         if (MUTEX_GET (sd->sem) != 0) {
            gdsWarningMessage ("Failure to obtain scheduler semaphore");
            return;
         }
      	  /* quit if return value is non-zero */
         if (retval != 0) {
            return;
         }
      }
   
      /* check whether tag list is NULL */
      if (sd->timetaglist == NULL) {
         return;
      }
   
      /* first check whether the tag is already in the list */
      for (i = 0; (i < sd->maxtimetaglist) && 
          (sd->timetaglist[i] != NULL); i++) {
         ltag = (_timetaginfo_t*) sd->timetaglist[i];	/* this is OK */
         if (gds_strncasecmp (ltag->tag, tag, TIMETAG_LENGTH) == 0) {
            return;	/* found it */
         }
      }
   
      /* Check whether timetag list is long enough */
      if (i + 2 >= sd->maxtimetaglist) {
         gdsDebug ("make time tag list longer");
         newlist = realloc (sd->timetaglist, sizeof (char*) *
                           (sd->maxtimetaglist + _INIT_TASK_LIST));
         if (newlist == NULL) {
            return;
         }
         sd->timetaglist = newlist;
         sd->maxtimetaglist += _INIT_TASK_LIST;
      }
   
      /* allocate memory and set the tag */
      sd->timetaglist[i] = malloc (sizeof (_timetaginfo_t));
      if (sd->timetaglist[i] != NULL) {
         ltag = (_timetaginfo_t*) sd->timetaglist[i];	/* this is OK */
         strncpy (ltag->tag, tag, TIMETAG_LENGTH);
         ltag->tag[TIMETAG_LENGTH-1] = 0;
         ltag->tai = tai;
         ltag->epoch = epoch;
         sd->timetaglist[i+1] = NULL;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: isTagExpired				*/
/*                                                         		*/
/* Procedure Description: checks if a time tag is expired and		*/
/*    	     returns the time delay since expiration in epochs 		*/
/*           The calling task must own the scheduler mutex! 		*/
/*                                                         		*/
/* Procedure Arguments: sd: scheduler, tag: name of timetag		*/
/*                      tai: TAI in sec, epoch: epoch			*/
/*                                                         		*/
/* Procedure Returns: Number of epochs since expiration,		*/
/*                    -1 means the tag hasn't been set yet		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/     
   static int isTagExpired (const scheduler_t* sd, const char* tag, 
                     taisec_t tai, int epoch)
   {
      int		i;
      _timetaginfo_t* 	ltag;
   
      /* return when invalid arguments */
      if ((sd == NULL) || (tag == NULL) || (sd->timetaglist == NULL)) {
         return -1;
      }
   
      /* check whether the tag is in the list */
      for (i = 0; (i < sd->maxtimetaglist) && 
          (sd->timetaglist[i] != NULL); i++) {
         ltag = (_timetaginfo_t*) sd->timetaglist[i];	/* this is OK */
         if (gds_strncasecmp (ltag->tag, tag, TIMETAG_LENGTH) == 0) {
         
            /* found it */
            return (tai - ltag->tai) * NUMBER_OF_EPOCHS +
               (epoch - ltag->epoch);
         }
      }
   
      /* tag not in list */
      return -1;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: deleteTag					*/
/*                                                         		*/
/* Procedure Description: deletes a timetag 				*/
/*           The calling task must own the scheduler mutex! 		*/
/*                                                         		*/
/* Procedure Arguments: sd: scheduler, tag: name of timetag		*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/     
   static void deleteTag (scheduler_t* sd, const char* tag)
   
   {
      int	i;
      int	li;
      char**	newlist;
   
      /* return when invalid arguments */
      if ((sd == NULL) || (tag == NULL) || (sd->timetaglist == NULL)) {
         return;
      }
   
      /* go through list and remove tag */
      for (i = 0, li = 0; (i < sd->maxtimetaglist) && 
          (sd->timetaglist[i] != NULL); i++) {
         if (gds_strncasecmp (sd->timetaglist[i], tag, 
            TIMETAG_LENGTH) == 0) {
            free (sd->timetaglist[i]);	/* found it */
            sd->timetaglist[i] = NULL;
         }
         else {
            if (i > li) {
               sd->timetaglist[li] = sd->timetaglist[i];
               sd->timetaglist[i] = NULL;
            }
            li++;
         }
      }
   
      /* Shorten timetag list if too long */
      if (li < sd->maxtimetaglist - 2 * _INIT_TASK_LIST) {
         gdsDebug ("make time tag list shorter");
         newlist = realloc (sd->timetaglist, sizeof (char*) *
                           (sd->maxtimetaglist - _INIT_TASK_LIST));
         if (newlist != NULL) {
            sd->timetaglist = newlist;
            sd->maxtimetaglist -= _INIT_TASK_LIST;
         }
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: updateEndOfTask				*/
/*                                                         		*/
/* Procedure Description: updates the 'finished' variables when task	*/
/*                        has finished.			 		*/
/*                        ASSUMES calling task has the entry mutex	*/
/*                                                         		*/
/* Procedure Arguments: info - scheduler entry; result - result of task */
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void updateEndOfTask (_schedinfo_t* info, int result)
   {
      /* the task has finished at least once when routine is called */
      if (info->firstFinished == 0) {
         info->firstFinished = 1;
      }
   
      /* check if return value is used (repeat cond. only) */
      if (((info->info.flag & SCHED_REPEAT) != 0) && 
         (info->info.repeattype == SCHED_REPEAT_COND) &&
         (result != 0)) {
         info->lastFinished = 1;
         info->markForTermination = 1;
      }
   
      /* check down counter (repeatN only) */
      if (((info->info.flag & SCHED_REPEAT) != 0) && 
         (info->info.repeattype == SCHED_REPEAT_N) &&
         (info->repeatN <= 0)) {
         info->lastFinished = 1;
         info->markForTermination = 1;
      }
   
      /* check single shot */
      if ((info->info.flag & SCHED_REPEAT) == 0) {
         info->lastFinished = 1;
         info->markForTermination = 1;
      }
   }

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: cleanupFinishedTasks			*/
/*                                                         		*/
/* Procedure Description: goes through list of scheduled tasks and	*/
/*    updates returned values and cleans up data structures		*/
/*                                                         		*/
/* Procedure Arguments: sd scheduler					*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void cleanupFinishedTasks (scheduler_t* sd)
   {
      _schedinfo_t*	info;	/* pointer to scheduler enrty info */
      int		indx;	/* used as an entry index */
      int		i;	/* used as an task index */
      int		ftasks;	/* count finished tasks */
   
      /* grap scheduler mutex */
      if (MUTEX_GET (sd->sem) != 0) {
         gdsWarningMessage ("Failure to obtain scheduler semaphore");
         return;
      }
   
      for (indx = 0; (indx < sd->maxtasklist) && 
          (sd->tasklist[indx] != NULL); indx++) {
         info = (_schedinfo_t*)  sd->tasklist[indx];
      
      	 /* check whether a task has finished (don't get mutex yet!) */
         for (i = 0, ftasks = 0; i < _MAX_TASKS; i++) {
            if ((info->tasks[i].valid != 0) &&
               (info->tasks[i].terminated != 0)) {
               ftasks++;
            }
         }
         if (ftasks == 0) {
            continue;
         }
      
       	 /* at least one task has finished; grap the entry mutex */
         if (MUTEX_GET (info->sem) != 0) {
            gdsWarningMessage ("Failure to obtain scheduler entry semaphore");
            continue;
         }
      
         /* go through list */
         for (i = 0; i < _MAX_TASKS; i++) {
         
            /* check for validity and termination */
            if ((info->tasks[i].valid == 0) || 
               (info->tasks[i].terminated == 0)) {
               continue;
            }
         
            updateEndOfTask (info, info->tasks[i].result);
         
         /* clean up thread */
         #ifndef OS_VXWORKS
            pthread_join (info->tasks[i].tid, NULL);
         #endif
         
            /* cleanup scheduler entry, free argument memory */
            info->tasks[i].valid = 0;
            info->runningtasks--;
         }
      
         /* release entry mutex */
         if (MUTEX_RELEASE (info->sem) != 0) {
            gdsError (GDS_ERR_PROG, 
                     "Failure to release scheduler entry semaphore");
         }
      }
   
      /* release scheduler mutex */
      if (MUTEX_RELEASE (sd->sem) != 0) {
         gdsError (GDS_ERR_PROG,
                  "Failure to release scheduler semaphore");
         return;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: evaluateTags				*/
/*                                                         		*/
/* Procedure Description: goes through list of scheduled tasks and	*/
/*    			  updates the tag list				*/
/*                                                         		*/
/* Procedure Arguments: sd scheduler; tai time in sec; epoch;		*/
/*                      tagtype : 0 scheduled, 1 finished		*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void evaluateTags (scheduler_t* sd, taisec_t tai, int epoch,
                     int tagtype)
   {
      _schedinfo_t*	info;	/* pointer to scheduler enrty info */
      int		indx;	/* used as an entry index */
   
     /* grap scheduler mutex */
      if (MUTEX_GET (sd->sem) != 0) {
         gdsWarningMessage ("Failure to obtain scheduler semaphore");
         return;
      }
   
      for (indx = 0; (indx < sd->maxtasklist) && 
          (sd->tasklist[indx] != NULL); indx++) {
         info = (_schedinfo_t*)  sd->tasklist[indx];
      
         /* test whether tags are used at all by this entry */
         if ((info->info.flag & SCHED_TIMETAG) == 0) {
            continue;
         }
      
      	 /* test whether any of the tags are set */
         if (!(((tagtype == 0) && (info->firstScheduled == 1)) ||
            ((tagtype == 1) && (info->firstFinished == 1)) ||
            ((tagtype == 1) && (info->lastFinished == 1)))) {
            continue;
         }
      
         /* get the entry mutex */
         if (MUTEX_GET (info->sem) != 0) {
            gdsWarningMessage ("Failure to obtain scheduler "
                              "entry semaphore");
            continue;
         }
      
        /* test for tags and update tag list */
         switch (info->info.tagtype) {
            case SCHED_TAG_START :
               if ((tagtype == 0) && (info->firstScheduled == 1)) {
                  setTag (sd, info->info.timetag, tai, epoch, 0);
                  info->firstScheduled = 2;
               }
               break;
            case SCHED_TAG_END :
               if ((tagtype == 1) && (info->firstFinished == 1)) {
                  setTag (sd, info->info.timetag, tai, epoch, 0);
                  info->firstFinished = 2;
               }
               break;
            case SCHED_TAG_LAST :
               if ((tagtype == 1) && (info->lastFinished == 1)) {
                  setTag (sd, info->info.timetag, tai, epoch, 0);
                  info->lastFinished = 2;
               }
               break;
            default:
               break;
         }
      
         /* set unused tags to 2 as well */
         if ((tagtype == 0) && (info->firstScheduled == 1)) {
            info->firstScheduled = 2;
         }
         if ((tagtype == 1) && (info->firstFinished == 1)) {
            info->firstFinished = 2;
         }
         if ((tagtype == 1) && (info->lastFinished == 1)) {
            info->lastFinished = 2; 
         } 
      
         /* release entry mutex */
         if (MUTEX_RELEASE (info->sem) != 0) {
            gdsError (GDS_ERR_PROG, 
                     "Failure to release scheduler entry semaphore");
         }
      
      }
   
      /* release scheduler mutex */
      if (MUTEX_RELEASE (sd->sem) != 0) {
         gdsError (GDS_ERR_PROG, 
                  "Failure to release scheduler semaphore");
         return;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: calcStartTime				*/
/*                                                         		*/
/* Procedure Description: calculates the adjusted start time for	*/
/*			  repeated tasks				*/
/*                                                         		*/
/* Procedure Arguments: info scheduler entry; time - scheduling time 	*/
/*                                                         		*/
/* Procedure Returns: adjusted firsttime				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static _epochtime_t calcStartTime (const _schedinfo_t* info, 
                     const _schedtime_t* time)
   {
      _epochtime_t	t;	/* adjusted time */
   
      /* test if of repeat type */
      if ((info->info.flag & SCHED_REPEAT) == 0) {
         return -1;
      }
   
      /* first handle the case where repeat is not synchronized itself */
      if ((info->info.repeatratetype == SCHED_REPEAT_EPOCH) ||
         (info->info.repeatsynctype == SCHED_SYNC_NEXT)) {
         /* now check if start was synchronized */
         if (((info->info.flag & SCHED_WAIT) != 0) &&
            (info->info.synctype != SCHED_SYNC_NEXT)) {
            /* start was synchronized */
            t = time->tnow;
            if (info->info.synctype == SCHED_SYNC_EPOCH) {
               /* adjust sync for epoch */
               while (t % NUMBER_OF_EPOCHS != info->info.syncval) {
                  t--;
               }
            }
            else {
               /* adjust sync for inverse epoch */
               t = NUMBER_OF_EPOCHS * (t / NUMBER_OF_EPOCHS);
               while ((t / (NUMBER_OF_EPOCHS) % info->info.syncval) != 0) {
                  t -= NUMBER_OF_EPOCHS;
               }
            } 
         }
         else {
            /* neither repeat nor start were synchronized */
            if (((info->info.flag & SCHED_WAIT) != 0) &&
               (info->info.waittype == SCHED_WAIT_STARTTIME)) {
               /* sync with abosolute start time */
               t = info->info.waitval / _EPOCH;
            }
            else {
               /* no adjustment necessary */
               t = time->tnow;
            }
         }
      }
      else {
         /* repeat is synchronized */
         t = time->tnow;
         if (info->info.repeatsynctype == SCHED_SYNC_EPOCH) {
            /* adjust sync for epoch */
            while (t % NUMBER_OF_EPOCHS != info->info.repeatsyncval) {
               t--;
            }
         }
         else {
            /* adjust sync for inverse epoch */
            t = NUMBER_OF_EPOCHS * (t / NUMBER_OF_EPOCHS);
            while ((t / NUMBER_OF_EPOCHS) % info->info.repeatrate != 
                  info->info.repeatsyncval) {
               t -= NUMBER_OF_EPOCHS;
            }
         }
      }
      return t;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: isTaskReady					*/
/*                                                         		*/
/* Procedure Description: returns 1 if the task is ready to be scheduled*/
/*                        ASSUMES calling task has the entry mutex	*/
/*                                                         		*/
/* Procedure Arguments: info - scheduler entry; time - scheduling time;	*/
/*                      synclist - syncronization table; 		*/
/*			tfirst - adjusted first time for repeated tasks	*/
/*                                                         		*/
/* Procedure Returns: 1 if ready; -1 when timeout, 0 otherwise		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int isTaskReady (const _schedinfo_t* info, 
                     const _schedtime_t* time, 
                     const int synclist[NUMBER_OF_EPOCHS],
                     _epochtime_t* tfirst) 
   {
      _epochtime_t	num;  		/* number of repeats */
      _epochtime_t	interval;	/* interval between repeats */
      _epochtime_t	delay;		/* delay in # of epoch */
   
      /* initialize tfirst */
      *tfirst = -1;
   
      /* check whether marked for deletion */
      if (info->markForTermination != 0) {
         return 0;
      }
   
      /* check timeout first */
      if (((info->info.flag & SCHED_TIMEOUT) != 0) &&
         (info->tinit + info->info.timeout <= time->now)) {
         return -1;
      }
   
      /* check whether this entry was ever scheduled */
      if (info->firstScheduled == 0) {
      
         /* first time around: check whether to wait or not */
         if ((info->info.flag & SCHED_WAIT) == 0) {
            /* don't wait: ready to go */
            /* set tfirst for repeated tasks */
            *tfirst = calcStartTime (info, time);
            return 1;
         }
         else {
            /* test tag wait condition */
            switch (info->info.waittype)
            {
               case SCHED_WAIT_TAG :	/* wait for a tag */
               case SCHED_WAIT_TAGDELAYED :
               case SCHED_WAIT_TAGEPOCHDELAYED :
                  {
                     delay = isTagExpired (info->sd, info->info.waittag, 
                                          time->tai, time->epoch);
                     if (delay == -1) {
                        return 0;	/* tag not set */
                     }
                     break;
                  }
               case SCHED_WAIT_TIMEDELAY :	/* wait for a delay */
               case SCHED_WAIT_EPOCHDELAY :
                  {
                     delay = time->tnow - 
                             ((info->tinit + _EPOCH - 1) / _EPOCH);
                     break;	/* round tinit to the next epoch */
                  }
               default: 		/* immediate or absolute */
                  {
                     delay = 0;
                     break;
                  }
            }
         
            /* if required, tag is set: check delay */
            switch (info->info.waittype)
            {
               case SCHED_WAIT_TAGDELAYED :
               case SCHED_WAIT_TIMEDELAY :
                  {
                     if (delay * _EPOCH < info->info.waitval) {
                        return 0;		/* need to wait longer */
                     }
                     break;
                  }
               case SCHED_WAIT_TAGEPOCHDELAYED :
               case SCHED_WAIT_EPOCHDELAY :
                  {
                     if (delay < info->info.waitval) {
                        return 0;		/* need to wait longer */
                     }
                     break;
                  }
               case SCHED_WAIT_STARTTIME :	/* check absolute time */
                  {
                     if (info->info.waitval > time->tnow * _EPOCH) {
                        return 0;		/* need to wait longer */
                     }
                     break;
                  }
               default :	/* no delay sepcified */
                  {
                     break;
                  }
            }
         
            /* wait is over: check synchronization */
            switch (info->info.synctype)
            {
               case SCHED_SYNC_EPOCH :
                  {
                     if ((info->info.syncval < 0) ||
                        (info->info.syncval >= NUMBER_OF_EPOCHS) ||
                        (synclist[info->info.syncval] == 0)) {
                        return 0;		/* no sync */
                     }
                     break;
                  }
               case SCHED_SYNC_IEPOCH :
                  {
                     if ((info->info.syncval <= 0) ||
                        (time->epoch != 0) ||
                        (time->tai % info->info.syncval != 0)) {
                        return 0;		/* no sync */
                     }
                     break;
                  }
               default :
                  {
                     break;
                  }
            }
         
            /* task is ready! now set tfirst for repeated tasks */
            *tfirst = calcStartTime (info, time);
         
            return 1;
         }
      }
      else if ((info->info.flag & SCHED_REPEAT) != 0) {
      
         /* test for repeated scheduling */
         if ((info->info.repeattype == SCHED_REPEAT_N) &&
            (info->repeatN == 0)) {
            return 0;	/* already done N times*/
         }
         if ((info->info.repeattype == SCHED_REPEAT_COND) &&
            (info->lastFinished != 0)) {
            return 0;	/* terminate condition was fullfilled */
         }
      
      	/* check repeat rate */
         if (info->info.repeatratetype == SCHED_REPEAT_EPOCH) {
            /* repeat rate is given in number of epochs */
            interval = info->info.repeatrate;
         }
         else {
            /* repeat rate is given in number of inverse epochs */
            interval = info->info.repeatrate * NUMBER_OF_EPOCHS;
         }
         num = (time->tnow - info->tfirst) / interval;
         if (num * interval + info->tfirst > time->told) {
            return 1;
         }
         else {
            return 0;
         }
      }
      else {
      
         /* single shot, but already scheduled */
         return 0;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: scheduleReadyTasks				*/
/*                                                         		*/
/* Procedure Description: goes through list of scheduler entries and	*/
/*    executes them when they are ready					*/
/*                                                         		*/
/* Procedure Arguments: scheduler; tai time in sec; epoch			*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void scheduleReadyTasks (scheduler_t* sd,
                     const _schedtime_t* time, 
                     const int synclist[NUMBER_OF_EPOCHS])
   {
      _schedinfo_t*	info;	/* pointer to scheduler enrty info */
      int		indx;	/* used as an entry index */
      _schedarg_t	arg;	/* arguments for scheduleTask */
      int		i;	/* used as an task index */
      int		err;	/* error flag */
      _epochtime_t	tfirst;	/* time of first scheduling */
   
      /* get scheduler mutex */
      if (MUTEX_GET (sd->sem) != 0) {
         gdsWarningMessage ("Failure to obtain scheduler semaphore");
         return;
      }
   
      for (indx = 0; (indx < sd->maxtasklist) && 
          (sd->tasklist[indx] != NULL); indx++) {
         info = (_schedinfo_t*)  sd->tasklist[indx];
      
         /* test whether task is ready */
         switch (isTaskReady (info, time, synclist, &tfirst))
         {
            case -1: 	/* task timed out */
               {
                  info->markForTermination = 1;
                  continue;
               }
            case 0: 	/* task not ready */
               {
                  continue;
               }
            default: 	/* task is ready */
               {
                  break;
               }
         }
      
      
         /* build argument lists */
         arg.info = info;
         arg.tai = time->tai;
         arg.epoch = time->epoch;
         arg.result = 0;
      
         /* distinguish synchronous and asynchronous */
         if ((info->info.flag & SCHED_ASYNC) == 0) {
         
            /* get the entry mutex */
            if (MUTEX_GET (info->sem) != 0) {
               gdsWarningMessage ("Failure to obtain scheduler "
                                 "entry semaphore");
               continue;
            }
         
            /* decrease repeat down counter (repeat N only) */
            if (((info->info.flag & SCHED_REPEAT) != 0) &&
               (info->info.repeattype == SCHED_REPEAT_N)) {
               info->repeatN--;
            }
            /* increase run number */
            info->runningtasks++;
            /* update run time */
            if (tfirst != -1) {
               info->tfirst = tfirst;
            }
            /* set first time scheduled */
            if (info->firstScheduled == 0) {
               info->firstScheduled = 1;
            }
         
            /* release entry mutex */
            if (MUTEX_RELEASE (info->sem) != 0) {
               gdsError (GDS_ERR_PROG, 
                        "Failure to release scheduler entry semaphore");
            }
            /* release scheduler mutex */
            if (MUTEX_RELEASE (sd->sem) != 0) {
               gdsError (GDS_ERR_PROG, 
                        "Failure to release scheduler semaphore");
            }
         
            /* synchronous call: entry and scheduler mutex are free */
            schedTask (&arg);
         
            /* get scheduler mutex back */
            if (MUTEX_GET (sd->sem) != 0) {
               gdsErrorMessage ("Failure to obtain scheduler semaphore");
            }
            /* get the entry mutex back */
            if (MUTEX_GET (info->sem) != 0) {
               gdsWarningMessage ("Failure to obtain scheduler "
                                 "entry semaphore");
            }
         
            /* decrease run number */
            info->runningtasks--;
            /* update finished... variables */
            updateEndOfTask (info, arg.result);
         
            /* release entry mutex */
            if (MUTEX_RELEASE (info->sem) != 0) {
               gdsError (GDS_ERR_PROG, 
                        "Failure to release scheduler entry semaphore");
            }
         }
         else {
            /* asynchronous */
         
            /* get the entry mutex */
            if (MUTEX_GET (info->sem) != 0) {
               gdsWarningMessage ("Failure to obtain scheduler "
                                 "entry semaphore");
               continue;
            }
         
            /* check for available slot in task list */
            i = 0;
            while ((i < _MAX_TASKS) && (info->tasks[i].valid != 0)) {
               i++;
            }
            if (i == _MAX_TASKS) {
               /* all occupied */
               err = -1;
               gdsDebug ("task list is full");
            } 
            else {
               /* found one */
               err = 0;
               arg.tindex = i;
               info->tasks[i].terminated = 0;
               info->tasks[i].result = 0;
               info->tasks[i].tid = 0;
               /*copy arguments to save memory location */
               info->tasks[i].arg = arg;
            }
         
            /* create thread/task */
            if (err == 0) {
            
            #ifdef OS_VXWORKS
            /* VxWorks task */
               {
                  info->tasks[i].tid = 
                     taskSpawn ("tSchedAsyn", info->info.priority, 
                               VX_FP_TASK,
                               10000, (FUNCPTR) schedTask, 
                               (int) &info->tasks[i].arg, 
            			0, 0, 0, 0, 0, 0, 0, 0, 0);
                  if (info->tasks[i].tid == ERROR) {
                     err = -2;
                  }
               }
            
            #else
            /* POSIX task */
               {
                  pthread_attr_t	tattr;
                  struct sched_param	schedprm;
                  int			status;
               
                  /* set thread parameters: joinable & system scope */
                  if (pthread_attr_init (&tattr) != 0) {
                     /* failure */
                     err = -3;
                  }
                  else {
                  
                     pthread_attr_setdetachstate (&tattr, 
                                          PTHREAD_CREATE_JOINABLE);
                     pthread_attr_setscope (&tattr, PTHREAD_SCOPE_PROCESS);
                     /* set priority */
                     pthread_attr_getschedparam (&tattr, &schedprm);
                     schedprm.sched_priority = info->info.priority;
                     pthread_attr_setschedparam (&tattr, &schedprm);
                     /* create thread */
                     status = 
                        pthread_create (&info->tasks[i].tid, &tattr,
                                       (void* (*) (void*)) schedTask, 
                                       (void*) &info->tasks[i].arg);
                     if (status != 0) {
                        /* task creation failed */
                        err = -4;
                     }
                  
                     pthread_attr_destroy (&tattr);
                  }
               }
               #endif
            }
         
            /* make it valid, mark first scheduled, decrease repeat N */
            if (err == 0) {
               if (info->firstScheduled == 0) {
                  info->firstScheduled = 1;
               }
               info->tasks[i].valid = 1;
               info->runningtasks++;
               if (((info->info.flag & SCHED_REPEAT) != 0) &&
                  (info->info.repeattype == SCHED_REPEAT_N)) {
                  info->repeatN--;
               }
               /* update run time */
               if (tfirst != -1) {
                  info->tfirst = tfirst;
               }
            }
            else {
               gdsWarningMessage ("Failure to create scheduled task");
            }
         
            /* release entry mutex */
            if (MUTEX_RELEASE (info->sem) != 0) {
               gdsError (GDS_ERR_PROG, 
                        "Failure to release scheduler entry semaphore");
            }
         }
      }
   
      /* release scheduler mutex */
      if (MUTEX_RELEASE (sd->sem) != 0) {
         gdsError (GDS_ERR_PROG, 
                  "Failure to release scheduler semaphore");
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: freeResources				*/
/*                                                         		*/
/* Procedure Description: calls freeResources routine of the task info	*/
/*                                                         		*/
/* Procedure Arguments: info - task info; time 				*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   struct _argFree_struct {
      void (*freeResources) (taisec_t time, int epoch, void* arg);
      taisec_t 	time;
      int 	epoch;
      void* 	arg;
      #ifndef NO_XDR
      xdrproc_t xdr_arg;
      #endif
   };
   typedef struct _argFree_struct _argFree_t;

   static void _freeResources (_argFree_t* argf) 
   {
      argf->freeResources (argf->time, argf->epoch, argf->arg);
   
      /* xdr stuff */
   #ifndef NO_XDR
      if ((argf->xdr_arg != NULL) && (argf->arg != NULL)) {
         xdr_free (argf->xdr_arg, argf->arg);
         free (argf->arg);
      }
   #endif
      /* free argf */
      free (argf);
   }

   static void freeResources (const _schedinfo_t* info, 
                     const _schedtime_t* time)
   {
      _argFree_t*	argf;
   
      if (info->info.freeResources == NULL) {
         return;
      }
   
      /* initialize arguments */
      argf = malloc (sizeof (_argFree_t));
      if (argf == NULL) {
         return;
      }
      argf->freeResources = info->info.freeResources;
      argf->time = time->tai;
      argf->epoch = time->epoch;
      argf->arg = info->info.arg;
   #ifndef NO_XDR
      argf->xdr_arg = info->info.xdr_arg;
   #endif
   
   #ifdef OS_VXWORKS
      /* VxWorks task */
      {
         taskSpawn ("tSchedFree", info->info.priority, VX_FP_TASK,
                   10000, (FUNCPTR) _freeResources, 
                   (int) argf, 0, 0, 0, 0, 0, 0, 0, 0, 0);
      }
   
   #else
      /* POSIX task */
      {
         pthread_attr_t		tattr;
         struct sched_param	schedprm;
         schedTID_t		tid;
      
         /* set thread parameters: joinable & system scope */
         if (pthread_attr_init (&tattr) != 0) {
                     /* failure */
            return;
         }
         pthread_attr_setdetachstate (&tattr, 
                              PTHREAD_CREATE_DETACHED);
         pthread_attr_setscope (&tattr, PTHREAD_SCOPE_PROCESS);
         /* set priority */
         pthread_attr_getschedparam (&tattr, &schedprm);
         schedprm.sched_priority = info->info.priority;
         pthread_attr_setschedparam (&tattr, &schedprm);
         /* create thread */
         pthread_create (&tid, &tattr,
                        (void* (*) (void*)) _freeResources, 
                        (void*) argf);
      
         pthread_attr_destroy (&tattr);
      }
      #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: deleteRetiredEntries			*/
/*                                                         		*/
/* Procedure Description: goes through list of scheduled tasks and	*/
/*    updates the tag list; also deletes unused entries			*/
/*                                                         		*/
/* Procedure Arguments: sd scheduler; time 				*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void deleteRetiredEntries (scheduler_t* sd, 
                     const _schedtime_t* time)
   {
      _schedinfo_t*	info;	/* pointer to scheduler enrty info */
      int		indx;	/* used as an entry index */
      int		lindx;	/* used as an entry index */
      schedulertask_t** newlist;/* new entry list if too long */ 
   
     /* get scheduler mutex */
      if (MUTEX_GET (sd->sem) != 0) {
         gdsWarningMessage ("Failure to obtain scheduler semaphore");
         return;
      }
   
      /* condense list and delete unused entries */
      for (indx = 0, lindx = 0; (indx < sd->maxtasklist) && 
          (sd->tasklist[indx] != NULL); indx++) {
         info = (_schedinfo_t*)  sd->tasklist[indx];
      
      
         /* get the entry mutex */
         if (MUTEX_GET (info->sem) != 0) {
            gdsWarningMessage ("Failure to obtain scheduler entry "
                              "semaphore");
            continue;
         }
      
         /* printf ("entry %i: mark = %i, run = %i, flag = %i, "
                "fini = %i %i %i\n", 
                (int) info->info.arg,
                info->markForTermination, 
                info->runningtasks,
                info->info.flag & SCHED_TIMETAG,
                info->firstScheduled,
                info->firstFinished,
                info->lastFinished); */
      
         /* check whether entry is marked for deletion, whether all task 
            have finished and whether tags were processed */
         if ((info->markForTermination != 0) &&
            (info->runningtasks == 0) &&
            (((info->info.flag & SCHED_TIMETAG) == 0) || 
            ((info->firstScheduled != 1) && 
            (info->firstFinished != 1) && 
            (info->lastFinished != 1)))) {
            /* free resources */
            if (MUTEX_DESTROY (info->sem) != 0) {
               gdsDebug ("Couldn't destroy scheduler entry semaphore");
            }
            /* call freeResources */
            freeResources (info, time);
            /* free task info */
            free (info);
            sd->tasklist[indx] = NULL;
         }
         else {
            /* copy entry pointer to new list place */
            if (lindx < indx) {
               sd->tasklist[lindx] = sd->tasklist[indx];
               sd->tasklist[indx] = NULL;
            }
            lindx++;
         
            /* release entry mutex */
            if (MUTEX_RELEASE (info->sem) != 0) {
               gdsError (GDS_ERR_PROG, 
                        "Failure to release scheduler entry semaphore");
            }
         }
      }
   
      /* Make list shorter, if too long */ 
      indx = 0;
      /* find number of entries */
      while ((indx < sd->maxtasklist) && (sd->tasklist[indx] != NULL)) {
         indx++;
      }
      /* test if too long */
      if (indx < sd->maxtasklist - 2 * _INIT_TASK_LIST) {
         /* make it shorter */
         gdsDebug ("make entry list shorter");
         newlist = realloc (sd->tasklist, sizeof (schedulertask_t*) *
                           (sd->maxtasklist - _INIT_TASK_LIST));
         if (newlist != NULL) {
            {
               sd->tasklist = newlist;
               sd->maxtasklist -= _INIT_TASK_LIST;
            }
         }
      }
   
      /* release scheduler mutex */
      if (MUTEX_RELEASE (sd->sem) != 0) {
         gdsError (GDS_ERR_PROG, 
                  "Failure to release scheduler semaphore");
         return;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: deleteRetiredTags				*/
/*                                                         		*/
/* Procedure Description: goes through list of scheduled tasks and	*/
/*    updates the tag list; also deletes unused entries			*/
/*                                                         		*/
/* Procedure Arguments: sd scheduler; tai time in sec; epoch		*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void deleteRetiredTags (scheduler_t* sd, int* testtag)
   {
      _schedinfo_t*	info;	/* pointer to scheduler enrty info */
      int		indx;	/* used as an entry index */
      int		found;	/* tag found */
      _timetaginfo_t* 	ltag;	/* test tag */
   
      /* get scheduler mutex */
      if (MUTEX_GET (sd->sem) != 0) {
         gdsWarningMessage ("Failure to obtain scheduler semaphore");
         return;
      }
      /* quick test whether tag list is empty */
      if ((sd->timetaglist[0] == NULL) ||
         (sd->timetaglist[*testtag] == NULL)) {
         *testtag = 0;
         /* release scheduler mutex */
         if (MUTEX_RELEASE (sd->sem) != 0) {
            gdsError (GDS_ERR_PROG, 
                     "Failure to release scheduler semaphore");
         }
         return;
      }
   
      /* test only one tag per call */
      ltag = (_timetaginfo_t*) sd->timetaglist[*testtag]; /* this is OK */
   
      for (indx = 0, found = 0; (indx < sd->maxtasklist) && 
          (sd->tasklist[indx] != NULL); indx++) {
         info = (_schedinfo_t*)  sd->tasklist[indx];
      	 /* test whether tags are used */
         if (((info->info.flag & SCHED_WAIT) == 0) ||
            (info->info.waittype == SCHED_WAIT_IMMEDIATE) ||
            (info->info.waittype == SCHED_WAIT_TIMEDELAY) ||
            (info->info.waittype == SCHED_WAIT_EPOCHDELAY) ||
            (info->info.waittype == SCHED_WAIT_STARTTIME)) {
            continue;
         }
         /* now check tag against the test tag */
         if (gds_strncasecmp (ltag->tag, info->info.waittag, 
            TIMETAG_LENGTH) == 0) {
            found = 1;	/* found it */
            break;
         }
      }
   
      if (found == 0) {
         char	buf[TIMETAG_LENGTH + 20];
      
         sprintf (buf, "retire tag = %s", ltag->tag);
         gdsDebug (buf);
         deleteTag (sd, ltag->tag);
      }
      else {
         (*testtag)++;
      }
      /* set test tag index to zero if last tag */
      if (sd->timetaglist[*testtag] == NULL) {
         *testtag = 0;
      }
   
      /* release scheduler mutex */
      if (MUTEX_RELEASE (sd->sem) != 0) {
         gdsError (GDS_ERR_PROG, 
                  "Failure to release scheduler semaphore");
         return;
      }
   }

