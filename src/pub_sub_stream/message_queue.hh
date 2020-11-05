//
// Created by jonathan.hanks on 10/6/20.
//

#ifndef DAQD_TRUNK_CHANNEL_QUEUE_HH
#define DAQD_TRUNK_CHANNEL_QUEUE_HH

#include <condition_variable>
#include <thread>
#include <type_traits>

#include <boost/optional.hpp>

template < typename T, std::size_t MaxSize >
class Message_queue;

namespace detail
{
    template < typename T, std::size_t MaxSize >
    class mq_introspect
    {
    public:
        using mq_type = Message_queue< T, MaxSize >;
        explicit mq_introspect( const mq_type& mq ) noexcept : mq_{ mq }
        {
        }
        mq_introspect( const mq_introspect& other ) noexcept = default;

        typename mq_type::size_type
        begin_index( ) const
        {
            return mq_.begin_;
        }
        typename mq_type::size_type
        end_index( ) const
        {
            return mq_.end_;
        }

    private:
        const mq_type& mq_;
    };

    template < typename T, std::size_t MaxSize >
    mq_introspect< T, MaxSize >
    introspect( const Message_queue< T, MaxSize >& mq )
    {
        return mq_introspect< T, MaxSize >( mq );
    }
} // namespace detail

template < typename T, std::size_t MaxSize >
class Message_queue
{
    using lock_type = std::unique_lock< std::mutex >;
    using storage_type =
        typename std::aligned_storage< sizeof( T ), alignof( T ) >::type;

    friend class detail::mq_introspect< T, MaxSize >;

public:
    using value_type = T;

    static_assert( MaxSize > 0, "the MaxSize parameter must be > 0" );
    static_assert( std::is_nothrow_move_constructible< T >::value == true,
                   "The type must be no through move constructable" );
    static_assert( std::is_nothrow_move_assignable< T >::value == true,
                   "The type must be no throw move assignable" );

    ~Message_queue( )
    {
        while ( size_ > 0 )
        {
            T* src = reinterpret_cast< T* >( &storage_[ end_ ] );
            end_ = decr( end_ );
            --size_;
            src->~T( );
        }
    }

    using size_type = std::size_t;

    size_type
    size( ) const
    {
        lock_type l{ m_ };
        return size_;
    }

    bool
    empty( ) const
    {
        lock_type l{ m_ };
        return size_ == 0;
    }

    constexpr size_type
              capacity( ) const
    {
        return MaxSize;
    }

    template < typename... Args >
    void
    emplace( Args... args )
    {
        lock_type l{ m_ };
        wait_for_free_space( l );
        emplace_while_locked( std::forward< Args >( args )... );
    }

    template < typename Duration, typename... Args >
    boost::optional< T >
    emplace_with_timeout( const Duration duration, Args... args )
    {
        boost::optional< T > results = boost::none;

        lock_type l{ m_ };
        wait_for_free_space_with_timeout( l, duration );
        if ( size_ < MaxSize )
        {
            emplace_while_locked( std::forward< Args >( args )... );
        }
        else
        {
            results = std::move( T( std::forward< Args >( args )... ) );
        }
        return results;
    };

    T
    pop( ) noexcept
    {
        lock_type l{ m_ };
        wait_for_non_empty( l );

        return pop_while_locked( );
    }

    template < typename Duration >
    boost::optional< T >
    pop( const Duration& duration )
    {
        boost::optional< T > result = boost::none;

        lock_type l{ m_ };
        wait_for_non_empty_with_timeout( l, duration );
        if ( size_ > 0 )
        {
            result = pop_while_locked( );
        }
        return result;
    }

private:
    template < typename... Args >
    void
    emplace_while_locked( Args... args )
    {
        T* dest = reinterpret_cast< T* >( &storage_[ begin_ ] );
        new ( dest ) T( std::forward< Args >( args )... );
        begin_ = decr( begin_ );
        ++size_;
        item_added_flag_.notify_one( );
    }

    T
    pop_while_locked( ) noexcept
    {
        T* src = reinterpret_cast< T* >( &storage_[ end_ ] );
        end_ = decr( end_ );
        --size_;
        T tmp( std::move( *src ) );
        src->~T( );
        item_removed_flag_.notify_one( );
        return std::move( tmp );
    }

    constexpr size_type
              decr( size_type i ) const noexcept
    {
        return ( i == 0 ? MaxSize - 1 : i - 1 );
    }

    void
    wait_for_free_space( lock_type& l )
    {
        while ( size_ == MaxSize )
        {
            item_removed_flag_.wait( l );
        }
    }

    template < typename Duration >
    void
    wait_for_free_space_with_timeout( lock_type& l, const Duration& timeout )
    {
        if ( size_ < MaxSize )
        {
            return;
        }
        item_removed_flag_.wait_for(
            l, timeout, [this]( ) -> bool { return size_ < MaxSize; } );
    }

    void
    wait_for_non_empty( lock_type& l )
    {
        while ( size_ == 0 )
        {
            item_added_flag_.wait( l );
        }
    }

    template < typename Duration >
    void
    wait_for_non_empty_with_timeout( lock_type& l, const Duration& timeout )
    {
        if ( size_ > 0 )
        {
            return;
        }
        item_added_flag_.wait_for(
            l, timeout, [this]( ) -> bool { return size_ > 0; } );
    }

    mutable std::mutex      m_{};
    std::condition_variable item_added_flag_{};
    std::condition_variable item_removed_flag_{};
    storage_type            storage_[ MaxSize ]{};
    size_type               size_{ 0 };
    size_type               begin_{ MaxSize - 1 };
    size_type               end_{ MaxSize - 1 };
};

#endif // DAQD_TRUNK_CHANNEL_QUEUE_HH
