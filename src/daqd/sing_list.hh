#ifndef SING_LIST_H
#define SING_LIST_H

#include <iostream>
#include "debug.h"
#include "pthread.h"

// Virtual base class for the linkable objects
class s_link
{
public:
    int id;
    s_link( int pid = 0 ) : _next( 0 ), id( pid )
    {
    }
    s_link( s_link* next, int pid = 0 ) : _next( next ), id( pid )
    {
    }
    s_link*
    next( void )
    {
        return _next;
    }
    void
    set_next( s_link* next )
    {
        _next = next;
    }
    virtual void destroy( void ) = 0;

protected:
    s_link* _next;
};

// Singly linked list
class s_list
{
public:
    s_list( ) : _first( 0 ), _current( 0 ), _count( 0 )
    {
        pthread_mutex_init( &bm, NULL );
    }

    ~s_list( )
    {
        pthread_mutex_destroy( &bm );
        if ( _count > 0 )
        {
            s_link* p_sl = _first;
            s_link* t;

            for ( int i = 0; i < _count; i++ )
            {
                t = p_sl->next( );
                p_sl->destroy( );
                p_sl = t;
            }
        }
    }

    s_link*
    current( )
    {
        locker  mon( this );
        s_link* c;

        c = _current;
        return c;
    }

    s_link*
    first( )
    {
        locker  mon( this );
        s_link* c;

        c = _current = _first;
        return c;
    }

    s_link*
    next( )
    {
        locker mon( this );

        s_link* t = _current->next( );
        if ( t != 0 )
            _current = t;
        return t;
    }

    unsigned
    count( )
    {
        locker       mon( this );
        unsigned int c;

        c = _count;
        return c;
    }

    void
    insert( s_link* link )
    {
        locker mon( this );

        if ( _current != 0 )
        {
            link->set_next( _current->next( ) );
            _current->set_next( link );
        }
        else
        {
            _first = link;
        }

        _current = link;
        _count++;
    }

    void
    insert_first( s_link* t )
    {
        locker mon( this );

        t->set_next( _first );
        _first = t;
        _current = t;
        _count++;
    }

    s_link*
    remove( s_link* pl )
    {
        locker  mon( this );
        s_link* cl;
        int     i;

        // DEBUG(5, std::cerr << "list remove() is called on " << (unsigned int)
        // pl << std::endl);

        cl = pfind( pl );
        if ( cl )
        {
            _current = cl;
            premove( );
        }
        return cl;
    }

    s_link*
    find( s_link* pl )
    {
        locker mon( this );

        return pfind( pl );
    }

    s_link*
    find_id( int id )
    {
        locker mon( this );

        s_link* cl;
        int     i;

        for ( i = 0, cl = _first; i < _count && cl->id != id;
              i++, cl = cl->next( ) )
            ;
        if ( i != _count )
            return cl;

        return 0;
    }

private:
    s_link*
    pfind( s_link* pl )
    {
        s_link* cl;
        int     i;

        for ( i = 0, cl = _first; i < _count && cl != pl;
              i++, cl = cl->next( ) )
            ;
        if ( i != _count )
            return cl;

        return 0;
    }

public:
    void
    remove( void )
    {
        locker mon( this );

        premove( );
    }

private:
    void
    premove( void )
    {
        if ( _current == 0 )
            return;

        if ( _first == _current )
        {
            _first = _first->next( );
            DEBUG( 5, std::cerr << "calling destroy 1" << std::endl );
            _current->destroy( );
            _current = _first;
            _count--;
            DEBUG( 5,
                   std::cerr << "there are " << _count
                             << " elements in the list now" << std::endl );
        }
        else
        {
            int     i;
            s_link* p_sl;

            for ( i = 0, p_sl = _first; p_sl->next( ) != _current && i < _count;
                  i++, p_sl = p_sl->next( ) )
                ;

            if ( i != _count )
            {
                p_sl->set_next( _current->next( ) );
                DEBUG( 5, std::cerr << "calling destroy 2" << std::endl );
                _current->destroy( );
                _current = p_sl;
                _count--;
                DEBUG( 5,
                       std::cerr << "there are " << _count
                                 << " elements in the list now" << std::endl );
            }
        }
    }

protected:
    s_link*      _first;
    s_link*      _current;
    unsigned int _count;

private:
    pthread_mutex_t bm;
    void
    lock( void )
    {
        pthread_mutex_lock( &bm );
    }
    void
    unlock( void )
    {
        pthread_mutex_unlock( &bm );
    }
    class locker;
    friend class s_list::locker;
    class locker
    {
        s_list* dp;

    public:
        locker( s_list* objp )
        {
            ( dp = objp )->lock( );
        }
        ~locker( )
        {
            dp->unlock( );
        }
    };
};

#endif
