#ifndef WORK_QUEUE_HH
#define WORK_QUEUE_HH

#include <queue>
#include <vector>
#include "raii.hh"

namespace work_queue
{

    /**
     * A simple work queue
     */
    template < typename T >
    class work_queue
    {
        pthread_mutex_t _lock;
        pthread_cond_t  _wait;

        typedef T*                        queue_entry;
        typedef std::queue< queue_entry > queue_type;

        std::vector< queue_type > _queues;

        work_queue( const work_queue& other );
        work_queue operator=( const work_queue& other );

    public:
        work_queue( size_t num_queues ) : _queues( num_queues )
        {
            pthread_mutex_init( &_lock, NULL );
            pthread_cond_init( &_wait, NULL );
        }

        ~work_queue( )
        {
            pthread_mutex_destroy( &_lock );
        }

        void
        add_to_queue( size_t index, queue_entry entry )
        {
            if ( !entry )
                return;
            raii::lock_guard< pthread_mutex_t > lock( _lock );
            _queues.at( index ).push( entry );
            pthread_cond_broadcast( &_wait );
        }

        queue_entry
        get_from_queue( size_t index )
        {
            raii::lock_guard< pthread_mutex_t > lock( _lock );
            while ( _queues.at( index ).empty( ) )
                pthread_cond_wait( &_wait, &_lock );
            queue_type& cur_queue = _queues.at( index );
            queue_entry val = cur_queue.front( );
            cur_queue.pop( );
            return val;
        }

        void
        dump_status( ostream& out )
        {
            raii::lock_guard< pthread_mutex_t > lock( _lock );
            int                                 counter = 0;
            out << "work queue status" << endl;
            for ( typename std::vector< queue_type >::iterator cur =
                      _queues.begin( );
                  cur != _queues.end( );
                  ++cur )
            {
                out << "\tqueue " << counter++ << cur->size( ) << endl;
            }
        }
    };

} // namespace work_queue

#endif // WORK_QUEUE_HH
