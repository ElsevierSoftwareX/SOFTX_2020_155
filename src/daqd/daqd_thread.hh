//
// Created by jonathan.hanks on 3/26/20.
//

#ifndef DAQD_TRUNK_DAQD_THREAD_HH
#define DAQD_TRUNK_DAQD_THREAD_HH

#include <functional>
#include <mutex>
#include <vector>

#include <pthread.h>

using thread_action_t = std::function< void( void ) >;

/*!
 * @brief A more generic interface around pthread_create
 * @details pthread_create has a limited interface, pass a void *,
 * std::thread doesn't allow specifying stack size, scheduling, ...
 * so provide a wrapper on pthread create that takes a more generic
 * argument
 * @param tid The pthread_t thread id structure
 * @param attr Structure describing pthread attributes to set
 * @param handler the code to run on the new thread
 */
extern int launch_pthread( pthread_t&            tid,
                           const pthread_attr_t& attr,
                           thread_action_t       handler );

/*!
 * @brief wrap the pthread scope macros in a enum to
 * bring them into the C++ type system.
 */
enum class thread_scope_t
{
    SYSTEM = PTHREAD_SCOPE_SYSTEM,
    PROCESS = PTHREAD_SCOPE_PROCESS,
};

/*!
 * @brief a strong type for a stacksize.
 */
class thread_stacksize_t
{
public:
    explicit thread_stacksize_t( std::size_t size ) : size_( size )
    {
    }
    thread_stacksize_t( const thread_stacksize_t& ) = default;
    thread_stacksize_t&
    operator=( const thread_stacksize_t& ) throw( ) = default;
    thread_stacksize_t&
    operator=( const std::size_t new_size ) throw( )
    {
        size_ = new_size;
        return *this;
    }
    std::size_t
    get( ) const
    {
        return size_;
    }

private:
    std::size_t size_;
};

/*!
 * @brief a RAII wrapper for pthread_attr_t, ensuring that it is always
 * destroyed.  The constructor also uses strong types to all an arbitrary
 * set of attributes to be set at construction time.
 */
class thread_attr_t
{
public:
    thread_attr_t( ) : attr_{}
    {
        pthread_attr_init( &attr_ );
    }

    /*!
     * @brief construct a thread_attr_t and set attributes on it.
     * @tparam T one of the attribute types that can be set
     * @tparam Args list of additional attribute types
     * @param t the first attribute to set
     * @param args additional (0 or more) attributes to set
     */
    template < typename T, typename... Args >
    explicit thread_attr_t( T t, Args... args ) : attr_{}
    {
        pthread_attr_init( &attr_ );
        set( t, args... );
    }

    thread_attr_t( const thread_attr_t& ) = delete;
    thread_attr_t( thread_attr_t&& ) = delete;

    ~thread_attr_t( )
    {
        pthread_attr_destroy( &attr_ );
    }

    thread_attr_t& operator=( const thread_attr_t& ) = delete;
    thread_attr_t& operator=( thread_attr_t&& ) = delete;

    /*!
     * @brief set the PTHREAD_SCOPE value
     * @param scope the scope value as an enum
     */
    void
    set( thread_scope_t scope )
    {
        pthread_attr_setscope( &attr_, static_cast< int >( scope ) );
    }
    /*!
     * @brief set the stack size
     * @param stack_size the size of the stack
     */
    void
    set( thread_stacksize_t stack_size )
    {
        pthread_attr_setstacksize( &attr_, stack_size.get( ) );
    }
    /*!
     * @brief take an arbitrary list of attribute types and set them.
     * @tparam T First attribute type to set
     * @tparam Args Additional (0+) types of attributes
     * @param t the first attribute
     * @param args additional attributes
     */
    template < typename T, typename... Args >
    void
    set( T t, Args... args )
    {
        set( t );
        set( args... );
    }

    pthread_attr_t&
    at( )
    {
        return attr_;
    }
    pthread_attr_t*
    get( )
    {
        return &attr_;
    }

private:
    pthread_attr_t attr_;
};

/*!
 * @brief a container of threads, that ensure that each thread is joined
 * @details The handler is given a stop function which is called (once) to stop
 * all the threads that are associated with it.  It is the responsibility of the
 * caller to ensure that the stop function will indeed stop all the threads that
 * will be managed by the thread_handler_t.
 */
class thread_handler_t
{
public:
    using stop_function_t = std::function< void( void ) >;

    explicit thread_handler_t( stop_function_t stopper )
        : stopper_{ std::move( stopper ) } {};
    thread_handler_t( const thread_handler_t& ) = delete;
    thread_handler_t( thread_handler_t&& ) = delete;
    thread_handler_t& operator=( const thread_handler_t ) = delete;
    thread_handler_t& operator=( thread_handler_t&& ) = delete;

    ~thread_handler_t( );

    void
    push_back( thread_action_t action, thread_attr_t& attr )
    {
        std::lock_guard< std::mutex > l_{ m_ };
        std::vector< pthread_t >      ids( thread_ids_.begin( ),
                                      thread_ids_.end( ) );
        ids.emplace_back( );
        if ( launch_pthread( ids.back( ), attr.at( ), std::move( action ) ) !=
             0 )
        {
            throw std::runtime_error( "Unable to create thread" );
        }
        thread_ids_.swap( ids );
    }

    void clear( );

private:
    std::vector< pthread_t > thread_ids_{};
    std::mutex               m_{};
    stop_function_t          stopper_;
};

#endif // DAQD_TRUNK_DAQD_THREAD_HH
