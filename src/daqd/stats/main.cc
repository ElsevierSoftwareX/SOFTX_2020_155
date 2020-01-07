#include <stats.hh>
#include <vector>
#include <iostream>

using namespace std;

template < typename T >
inline void
INSERT_ELEMENTS( T& coll, int first, int last )
{
    for ( int i = first; i <= last; ++i )
    {
        coll.insert( coll.end( ), i );
    }
};

template < typename T >
inline void
PRINT_ELEMENTS( T& coll )
{
    for ( typename T::const_iterator i = coll.begin( ); i != coll.end( ); i++ )
        cout << *i << endl;
};

int
main( )
{
    stats         ps;
    vector< int > coll;

    INSERT_ELEMENTS( coll, 1, 9 );
    // PRINT_ELEMENTS(coll);
    // for (vector<int>::const_iterator i = coll.begin(); i != coll.end(); i++)
    // ps.accumulateNext(*i);
    for ( int i = 0; i < 1000; i++ )
    {
        ps.tick( );
        struct timespec t = { 0, 1000000 };
        nanosleep( &t, 0 );
    }
    time_t now = time( 0 );
    cout << ctime( &now );
    ps.println( cout );
}
