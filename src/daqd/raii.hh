#ifndef RAII_HH
#define RAII_HH

#include <unistd.h>
#include <memory>

/**
 * Some 'smart' resource wrappers using the raii technique.
 * This should be cleaned up and foled into C++11 code as
 * it becomes feasible.
 */

namespace raii
{

    template < typename T >
    class array_ptr
    {
        T* _p;

        array_ptr( array_ptr< T >& other );
        array_ptr< T > operator=( const array_ptr< T >& other );

        void
        clean_up( )
        {
            if ( _p )
                delete[] _p;
            _p = 0;
        }

    public:
        array_ptr( T* p = 0 ) : _p( p )
        {
        }
        ~array_ptr( )
        {
            clean_up( );
        }
        T*
        get( ) const
        {
            return _p;
        }
        void
        reset( T* p )
        {
            if ( p != _p )
            {
                clean_up( );
                _p == p;
            }
        }
        T*
        release( )
        {
            T* tmp( _p );
            _p = 0;
            return tmp;
        }
    };

    template < typename T >
    class lock_guard
    {
        lock_guard( lock_guard< T >& other );
        lock_guard operator=( lock_guard< T >& other );
        T&         _m;

    public:
        lock_guard( T& m ) : _m( m )
        {
            _m.lock( );
        }
        ~lock_guard( )
        {
            _m.unlock( );
        }
    };

    template <>
    class lock_guard< pthread_mutex_t >
    {
        lock_guard( lock_guard< pthread_mutex_t >& other );
        lock_guard       operator=( lock_guard< pthread_mutex_t > );
        pthread_mutex_t& _m;

    public:
        lock_guard( pthread_mutex_t& m ) : _m( m )
        {
            pthread_mutex_lock( &_m );
        }
        ~lock_guard( )
        {
            pthread_mutex_unlock( &_m );
        }
    };

    class file_handle
    {
        int _fd;
        file_handle( const file_handle& other );
        file_handle operator=( const file_handle& other );

    public:
        file_handle( int fd ) : _fd( fd )
        {
        }
        ~file_handle( )
        {
            reset( );
        }

        int
        get( ) const
        {
            return _fd;
        }
        int
        release( )
        {
            int tmp = _fd;
            _fd = -1;
            return tmp;
        }
        void
        reset( )
        {
            if ( _fd >= 0 )
            {
                ::close( _fd );
                _fd = -1;
            }
        }
    };

    // std::make_unique didn't make it into C++11, so
    // to allow this to work in a pre C++14 world, we
    // provide a simple replacement.
    //
    // A make_unique<> for C++11.  Taken from
    // "Effective Modern C++ by Scott Meyers (O'Reilly).
    // Copyright 2015 Scott Meyers, 978-1-491-90399-5"
    //
    // Permission given in the book to reuse code segments.
    //
    // @tparam T The type of the object to be managed by the unique_ptr
    // @tparam Ts The type of the arguments to T's constructor
    // @param params The arguments to forward to the constructor
    // @return a std::unique_ptr<T>
    template < typename T, typename... Ts >
    std::unique_ptr< T >
    make_unique_ptr( Ts&&... params )
    {
        return std::unique_ptr< T >( new T( std::forward< Ts >( params )... ) );
    }

} // namespace raii

#endif // RAII_HH
