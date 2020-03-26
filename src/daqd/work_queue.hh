#ifndef WORK_QUEUE_HH
#define WORK_QUEUE_HH

#include <array>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <ostream>
#include <queue>

namespace work_queue
{

    class queue_terminating : public std::runtime_error
    {
    public:
        queue_terminating( )
            : runtime_error( "The shared work queue is terminating" )
        {
        }
    };

    /**
     * A simple work queue
     */
    template < typename T, size_t N >
    class work_queue
    {
        std::mutex              lock_;
        std::condition_variable wait_;
        bool                    aborting_;

        using queue_entry = T*;
        using queue_type = std::queue< queue_entry >;
        using lock_type = std::unique_lock< std::mutex >;

        std::array< queue_type, N > queues_;

    public:
        work_queue( ) : lock_{}, wait_{}, aborting_{ false }, queues_{}
        {
        }
        work_queue( const work_queue& other ) = delete;
        work_queue( work_queue&& other ) = delete;
        work_queue& operator=( const work_queue& other ) = delete;
        work_queue& operator=( work_queue&& other ) = delete;

        void
        abort( )
        {
            lock_type l_{ lock_ };
            if ( aborting_ )
            {
                return;
            }
            aborting_ = true;
            wait_.notify_all( );
        }

        void
        add_to_queue( size_t index, queue_entry entry )
        {
            if ( !entry )
                return;
            lock_type l_{ lock_ };
            if ( aborting_ )
            {
                throw queue_terminating( );
            }
            queues_.at( index ).push( entry );
            wait_.notify_all( );
        }

        queue_entry
        get_from_queue( size_t index )
        {
            lock_type l_{ lock_ };
            while ( queues_.at( index ).empty( ) && !aborting_ )
            {
                wait_.wait( l_ );
            }
            if ( aborting_ )
            {
                throw queue_terminating( );
            }
            queue_type& cur_queue = queues_.at( index );
            queue_entry val = cur_queue.front( );
            cur_queue.pop( );
            return val;
        }

        void
        dump_status( std::ostream& out )
        {
            lock_type l_{ lock_ };

            int counter = 0;
            out << "work queue status" << std::endl;
            for ( const auto& queue : queues_ )
            {
                out << "\tqueue " << counter++ << " " << queue.size( )
                    << std::endl;
            }
        }
    };

    template < typename T >
    class aborter
    {
    public:
        explicit aborter( T& target ) : target_{ target }
        {
        }
        aborter( const aborter& other ) = delete;
        aborter( aborter&& other ) = delete;
        aborter& operator=( const aborter& other ) = delete;
        aborter& operator=( aborter&& other ) = delete;
        ~aborter( )
        {
            target_.abort( );
        }

    private:
        T& target_;
    };
} // namespace work_queue

#endif // WORK_QUEUE_HH
