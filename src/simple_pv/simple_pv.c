#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "simple_pv.h"

static void
write_all( int fd, char* buffer, size_t count )
{
    int     attempts = 0;
    ssize_t cur = 0;

    if ( fd < 0 || !buffer || count < 0 )
    {
        return;
    }
    while ( count && attempts < 5 )
    {
        cur = write( fd, buffer, count );
        if ( cur == 0 )
        {
            return;
        }
        else if ( cur < 0 )
        {
            if ( errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR )
            {
                ++attempts;
            }
            else
            {
                return;
            }
        }
        else
        {
            count -= cur;
            buffer += cur;
        }
    }
}

// write out a list of pv updates to the given file.
// The data is written out as a json text blob prefixed by a binary length
// (sizeof size_t host byte order) if there is a failure, nothing happens it is
// safe to call with a negative fd
void
send_pv_update( int fd, const char* prefix, SimplePV* pvs, int pv_count )
{
    char   buffer[ 10000 ];
    char*  dest = 0;
    size_t max = 0;
    size_t remaining = 0;
    size_t size = 0;
    size_t count = 0;
    int    i = 0;

    if ( fd < 0 || !pvs || pv_count < 1 )
    {
        return;
    }
    max = sizeof( buffer ) - sizeof( size ) - 4;
    remaining = max;
    dest = buffer + sizeof( size ) + 4;

    for ( i = 0; i < 4; ++i )
    {
        buffer[ i ] = (char)0xff;
    }

    count = snprintf( dest,
                      remaining,
                      "{ \"prefix\": \"%s\", \"pvs\": [ ",
                      ( prefix ? prefix : "" ) );
    if ( count >= remaining )
    {
        return;
    }
    remaining -= count;
    dest += count;
    size += count;
    for ( i = 0; i < pv_count; ++i )
    {
        count = 0;
        if ( pvs[ i ].pv_type == SIMPLE_PV_INT )
        {
            count = snprintf(
                dest,
                remaining,
                "%s{ \"name\": \"%s\", \"value\": %d, \"alarm_high\": %d, "
                "\"pv_type\": 0,"
                "\"alarm_low\": %d, \"warn_high\": %d, \"warn_low\": %d }",
                ( i ? ", " : "" ),
                pvs[ i ].name,
                *( (int*)pvs[ i ].data ),
                pvs[ i ].alarm_high,
                pvs[ i ].alarm_low,
                pvs[ i ].warn_high,
                pvs[ i ].warn_low );
        }
        else if ( pvs[ i ].pv_type == SIMPLE_PV_STRING )
        {
            char tmp[ 80 ];
            strncpy( tmp, pvs[ i ].data, sizeof( tmp ) );
            tmp[ sizeof( tmp ) - 1 ] = '\0';
            char* cur_tmp = tmp;
            while ( *cur_tmp )
            {
                char ch = *cur_tmp;
                if ( ch == '"' || ch == '\\' )
                {
                    *cur_tmp = ' ';
                }
                ++cur_tmp;
            }
            count = snprintf(
                dest,
                remaining,
                "%s{ \"name\": \"%s\", \"value\": \"%s\", \"pv_type\": 1 }",
                ( i ? ", " : "" ),
                pvs[ i ].name,
                tmp );
        }
        if ( count >= remaining )
        {
            return;
        }
        remaining -= count;
        dest += count;
        size += count;
    }
    count = snprintf( dest, remaining, " ] }" );
    if ( count >= remaining )
    {
        return;
    }
    remaining -= count;
    dest += count;
    size += count;

    memcpy( buffer + 4, &size, sizeof( size ) );
    write_all( fd, buffer, size + sizeof( size ) + 4 );
}