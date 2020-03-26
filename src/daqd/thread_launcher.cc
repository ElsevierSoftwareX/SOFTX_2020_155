//
// Created by jonathan.hanks on 3/26/20.
//
#include "thread_launcher.hh"
#include "raii.hh"

namespace
{
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