/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdstask							*/
/*                                                         		*/
/* Module Description: functions for handling tasks & semaphores	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

/* Header File List: */
#include <stdlib.h>

#ifdef OS_VXWORKS
#include <vxWorks.h>
#endif

/* #include "gdsutil.h" */
#include "gdstask.h"

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: taskCreate					*/
/*                                                         		*/
/* Procedure Description: spawns a new task				*/
/*                                                         		*/
/* Procedure Arguments: attr - 	thread attr. detached/process (UNIX);	*/
/*				all task attr. (VxWorks)		*/
/* 			priority - thread/task priority			*/
/* 			taskIF - pointer to TID (return value)		*/
/* 			task - thread/task function			*/
/* 			arg - argument passed to the task		*/
/*                                                         		*/
/* Procedure Returns: 0 if succesful, -1 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
int
taskCreate( int         attr,
            int         priority,
            taskID_t*   taskID,
            const char* taskname,
            taskfunc_t  task,
            taskarg_t   arg )
{
#ifdef OS_VXWORKS
    /* VxWorks task */
    *taskID = taskSpawn( (char*)taskname,
                         priority,
                         attr,
                         50000,
                         (FUNCPTR)task,
                         (int)arg,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0 );
    if ( *taskID == ERROR )
    {
        return -1;
    }
#else

    /* POSIX task */
    {
        pthread_attr_t     tattr;
        struct sched_param schedprm;
        int                status;

        /* set thread parameters: joinable & system scope */
        if ( pthread_attr_init( &tattr ) != 0 )
        {
            return -1;
        }
        pthread_attr_setdetachstate( &tattr, attr & PTHREAD_CREATE_DETACHED );
        pthread_attr_setscope( &tattr, attr & PTHREAD_SCOPE_SYSTEM );
        /* set priority */
        pthread_attr_getschedparam( &tattr, &schedprm );
        schedprm.sched_priority = priority;
        pthread_attr_setschedparam( &tattr, &schedprm );

        /* create thread */
        status = pthread_create( taskID, &tattr, task, (void*)arg );
        pthread_attr_destroy( &tattr );
        if ( status != 0 )
        {
            return -1;
        }
    }
#endif
    return 0;
}

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: taskCancel					*/
/*                                                         		*/
/* Procedure Description: cancels a task				*/
/*                                                         		*/
/* Procedure Arguments: taskID - pointer to TID (return value)		*/
/*                                                         		*/
/* Procedure Returns: 0 if succesful, -1 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
int
taskCancel( taskID_t* taskID )
{
    if ( ( taskID == NULL ) || ( *taskID == 0 ) )
    {
        return 0;
    }
    /* cancel task */
#ifdef OS_VXWORKS
    if ( taskDelete( *taskID ) == ERROR )
    {
        return -1;
    }
#else
    if ( pthread_cancel( *taskID ) != 0 )
    {
        return -1;
    }
#endif
    *taskID = 0;
    return 0;
}
