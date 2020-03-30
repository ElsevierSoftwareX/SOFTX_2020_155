//
// Created by jonathan.hanks on 3/26/20.
//
#include "thread_launcher.hh"
#include "raii.hh"

namespace
{
    /*!
     * @brief this is the landing function for launch_pthread which
     * takes ownership of the argument and launches the requested function.
     * @param arg
     * @return nullptr
     *
     * @details This function is a little clever in passing arguments.  It
     * extends the interface for launching threads to run a std::function.
     * Due to the C api, we need to be careful in passing ownership.  The
     * std::function needs to live on the heap.  It can be 'safely' held in a
     * smart pointer before the pthreads_create call is called, but ownership
     * is transfered through this.  The first thing we do is take ownership of
     * the pointer again.
     */
    void*
    thread_trampoline( void* arg )
    {
        std::unique_ptr< thread_handler_t > arg_ptr{
            reinterpret_cast< thread_handler_t* >( arg )
        };
        ( *arg_ptr )( );
        return nullptr;
    }
} // namespace

int
launch_pthread( pthread_t&            tid,
                const pthread_attr_t& attr,
                thread_handler_t      handler )
{
    auto arg_ptr =
        raii::make_unique_ptr< thread_handler_t >( std::move( handler ) );
    auto result = pthread_create( &tid,
                                  &attr,
                                  thread_trampoline,
                                  reinterpret_cast< void* >( arg_ptr.get( ) ) );
    // on success, ownership is transfered to the thread
    if ( result == 0 )
    {
        arg_ptr.release( );
    }
    return result;
}