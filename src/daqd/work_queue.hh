#ifndef WORK_QUEUE_HH
#define WORK_QUEUE_HH

#include <array>
#include <condition_variable>
#include <mutex>
#include <queue>

namespace work_queue
{

    /**
     * A simple work queue
     */
    template < typename T, size_t N >
    class work_queue
    {
        std::mutex lock_;
        std::condition_variable  wait_;

        using queue_entry = T*;
        using queue_type = std::queue< queue_entry >;
        using lock_type = std::unique_lock< std::mutex >;

        std::array< queue_type, N > queues_;

    public:
        work_queue( ) : lock_{}, wait_{}, queues_{}
        {
        }
        work_queue( const work_queue& other ) = delete;
        work_queue( work_queue&& other ) = delete;
        work_queue& operator=( const work_queue& other ) = delete;
        work_queue& operator=( work_queue&& other ) = delete;

        void
        add_to_queue( size_t index, queue_entry entry )
        {
            if ( !entry )
                return;
            lock_type l_{ lock_ };
            queues_.at( index ).push( entry );
            wait_.notify_all();
        }

        queue_entry
        get_from_queue( size_t index )
        {
            lock_type l_{ lock_ };
            while ( queues_.at( index ).empty( ) )
            {
                wait_.wait(l_);
            }
            queue_type& cur_queue = queues_.at( index );
            queue_entry val = cur_queue.front( );
            cur_queue.pop( );
            return val;
        }

        void
        dump_status( ostream& out )
        {
            lock_type l_{ lock_ };

            int                                 counter = 0;
            out << "work queue status" << endl;
            for ( const auto& queue:queues_ )
            {
                out << "\tqueue " << counter++ << " " << queue.size( ) << std::endl;
            }
        }
    };

} // namespace work_queue

#endif // WORK_QUEUE_HH
