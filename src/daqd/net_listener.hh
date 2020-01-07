#ifndef NET_LISTENER_HH
#define NET_LISTENER_HH

// Network listener thread
class net_listener
{
public:
    int                listenfd;
    int                listener_port;
    pthread_t          tid; ///< listener thread ID
    struct sockaddr_in srvr_addr;
    void*              listener( );
    static void*
    listener_static( void* a )
    {
        return ( (net_listener*)a )->listener( );
    };
    int strict; ///< This is set if the listener has to support client program

    int start_listener( ostream*, int, int );
};

#endif
