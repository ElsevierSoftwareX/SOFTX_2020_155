/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdsmutex						*/
/*                                                         		*/
/* Module Description: implements a mutex and lock objects		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

/* Header File List: */
#include "gdsmutex.hh"

namespace diag
{

    abstractsemaphore::~abstractsemaphore( )
    {
    }

    mutex::~mutex( )
    {
        MUTEX_DESTROY( mux );
    }

    void
    recursivemutex::lock( )
    {
#ifdef OS_VXWORKS
        MUTEX_GET( mux );
#else
        // cout << "ref count = " << refcount << endl;
        // cout << "thread ID = " << threadID << endl;
        // cout << "self ID = " << pthread_self() << endl;
        if ( ( refcount > 0 ) && ( threadID == pthread_self( ) ) )
        {
            refcount++;
            return;
        }
        MUTEX_GET( mux );
        threadID = pthread_self( );
        refcount = 1;
#endif
    }

    void
    recursivemutex::unlock( )
    {
#ifdef OS_VXWORKS
        MUTEX_RELEASE( mux );
#else
        if ( --refcount == 0 )
        {
            threadID = 0;
            MUTEX_RELEASE( mux );
        }
#endif
    }

    bool recursivemutex::trylock( locktype )
    {
#ifdef OS_VXWORKS
        return ( MUTEX_TRY( mux ) == 0 );
#else
        if ( ( refcount > 0 ) && ( threadID == pthread_self( ) ) )
        {
            refcount++;
            return true;
        }
        if ( MUTEX_TRY( mux ) != 0 )
        {
            return false;
        }
        else
        {
            threadID = pthread_self( );
            refcount = 1;
            return true;
        }
#endif
    }

    readwritelock::~readwritelock( )
    {
        pthread_cond_destroy( &cond );
        MUTEX_DESTROY( mux );
    }

    void
    readwritelock::readlock( )
    {
        /* can share lock: check if available */
        MUTEX_GET( mux );
        while ( ( inuse < 0 ) || ( wrwait != 0 ) ||
                ( ( maxuse > 0 ) && ( inuse >= maxuse ) ) )
        {
            pthread_cond_wait( &cond, &mux );
        }
        inuse++;
        MUTEX_RELEASE( mux );
    }

    void
    readwritelock::writelock( )
    {
        /* need exclusive lock: check if free */
        MUTEX_GET( mux );
        wrwait++;
        while ( inuse != 0 )
        {
            pthread_cond_wait( &cond, &mux );
        }
        inuse--;
        MUTEX_RELEASE( mux );
    }

    void
    readwritelock::unlock( )
    {
        MUTEX_GET( mux );
        if ( inuse == -1 )
        {
            wrwait--;
            inuse = 0;
        }
        else if ( inuse > 0 )
        {
            inuse--;
        }
        pthread_cond_broadcast( &cond );
        MUTEX_RELEASE( mux );
    }

    bool
    readwritelock::trylock( locktype lck )
    {
        bool success = false;
        MUTEX_GET( mux );
        if ( lck == wrlock )
        {
            /* need exclusive lock: check if free */
            if ( inuse == 0 )
            {
                wrwait++;
                inuse--;
                success = true;
            }
        }
        else
        {
            /* can share lock: check if available */
            if ( ( inuse >= 0 ) && ( wrwait == 0 ) &&
                 ( ( maxuse <= 0 ) || ( inuse < maxuse ) ) )
            {
                inuse++;
                success = true;
            }
        }
        MUTEX_RELEASE( mux );
        return success;
    }

    semlock::~semlock( )
    {
        // cout << "unlock mutex" << endl;
        _sem->unlock( );
    }

} // namespace diag
