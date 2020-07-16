//
// Created by jonathan.hanks on 7/15/20.
//

#ifndef DAQD_TRUNK_LOCAL_DC_UTILS_H
#define DAQD_TRUNK_LOCAL_DC_UTILS_H

#include <string.h>
#include <stdlib.h>

/*!
 * @brief given the tail of a system name + dcu + rate, extract the dcu + rate.
 * @param input The text to parse must be either ":dcuid:rate" or ":dcuid"
 * @param dcuIdDest Destination for the dcuid
 * @param rateDest Destination for the rate
 * @note if the rate is not specified, it defaults to 16, if the dcu is invalid
 * if either the dcu or rate are invalid, 0 is used.
 */
static void
extract_dcu_rate_from_name( const char* input, int* dcuIdDest, int* rateDest )
{
    size_t size = 0;
    char*  buffer = 0;
    char*  sep = 0;
    int    dcuid = 0;
    int    rate = 0;

    if ( !input || !*input )
    {
        return;
    }
    ++input;
    size = strlen( input );
    buffer = malloc( size + 1 );
    strncpy( buffer, input, size + 1 );
    sep = strchr( buffer, ':' );
    if ( sep )
    {
        /* dcuid + rate */
        *sep = '\0';
        rate = (int)atoi( sep + 1 );
        if ( *( sep + 1 ) == '\0' )
        {
            rate = 16;
        }
    }
    else
    {
        /* dcuid only, default the rate */
        rate = 16;
    }
    dcuid = (int)atoi( buffer );
    free( buffer );
    if ( dcuIdDest )
    {
        *dcuIdDest = dcuid;
    }
    if ( rateDest )
    {
        *rateDest = rate;
    }
}

#endif // DAQD_TRUNK_LOCAL_DC_UTILS_H
