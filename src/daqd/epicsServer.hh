#ifndef EPICS_SERVER_HH
#define EPICS_SERVER_HH
#include <string>

/// Epics soft IOC Channel Server
class epicsServer
{
public:
    epicsServer( ) : running( 0 ), prefix( "" ), prefix1( "" ), prefix2( "" ){};
    void* epics_main( );
    static void*
    epics_static( void* a )
    {
        return ( (epicsServer*)a )->epics_main( );
    };

    pthread_t   tid;
    bool        running;
    std::string prefix;
    std::string prefix1;
    std::string prefix2;
};

#endif
