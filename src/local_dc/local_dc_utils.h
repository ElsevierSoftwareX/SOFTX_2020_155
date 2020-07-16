//
// Created by jonathan.hanks on 7/15/20.
//

#ifndef DAQD_TRUNK_LOCAL_DC_UTILS_H
#define DAQD_TRUNK_LOCAL_DC_UTILS_H

#include <string.h>
#include <stdlib.h>

/*!
 * @brief given the tail of a system name + dcu + rate, extract the dcu + rate.
 * @param input The text to parse must be either "model:dcuid:rate" or
 * "model:dcuid"
 * @param dcuIdDest Destination for the dcuid
 * @param rateDest Destination for the rate
 * @note this will always set *dcuIdDest & *rateDest if they are non-null
 * If a dcuid is specified but a rate is not, the rate defaults to 16
 * If a dcuid or rate is invalid it is set to 0
 * If no dcuid (and thus also no rate) is specified then both are set to 0
 */
static void
extract_dcu_rate_from_name( const char* input, int* dcuIdDest, int* rateDest )
{
    size_t size = 0;
    char*  buffer = 0;
    char*  sep = 0;
    int    dcuid = 0;
    int    rate = 0;

    if ( dcuIdDest )
    {
        *dcuIdDest = 0;
    }
    if ( rateDest )
    {
        *rateDest = 0;
    }
    if ( !input )
    {
        return;
    }
    sep = strchr( input, ':' );
    if ( sep == NULL )
    {
        return;
    }
    input = sep + 1;
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

/*!
 * @brief Trim off any :dcuid:rate or :dcuid portion from a model name
 * @param name The name to trim
 * @note safe to call with a NULL pointer.  Modifies the string in place.
 */
void
trim_dcuid_and_rate_from_name( char* name )
{
    char* sep = NULL;

    if ( name )
    {
        sep = strchr( name, ':' );
        if ( sep != NULL )
        {
            *sep = '\0';
        }
    }
}

#endif // DAQD_TRUNK_LOCAL_DC_UTILS_H
