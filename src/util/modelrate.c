#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define ENV_BUF_SIZE 16

/*!
 * @brief get a string from the environment and return a copy converted to
 * lowercase
 * @param env_name the name of the environment variable
 * @param dest destination buffer (non-null)
 * @param dest_size size of dest (must be > 1)
 * @details If the environment variable env_name is not set, sets dest to the
 * empty string, else copies the value into dest and converts it to lowercase.
 * @note This truncates the value if needed to fit in dest.  Dest is always a
 * null terminated string after this call.
 */
void
get_env_lower( const char* env_name, char* dest, size_t dest_size )
{
    char* from_env = 0;
    int   i = 0;

    if( !dest )
    {
        return;
    }
    from_env = getenv( env_name );
    if ( !from_env )
    {
        *dest = '\0';
        return;
    }
    strncpy( dest, from_env, dest_size );
    dest[ dest_size - 1 ] = '\0';
    while ( dest[ i ] )
    {
        if ( isupper( dest[ i ] ) )
        {
            dest[ i ] = tolower( dest[ i ] );
        }
        ++i;
    }
}

// **********************************************************************************************
// Get control model loop rates from GDS param files
// Needed to properly size TP data into the data stream
int
get_model_rate_dcuid( int* rate, int* dcuid, const char* modelname, char* gds_tp_dir )
{
    char        gdsfile[ 128 ];
    int         ii = 0;
    FILE*       f = 0;
    char*       token = 0;
    const char* search = "=";
    char        line[ 80 ];
    char        ifo[ ENV_BUF_SIZE ];
    char        site[ ENV_BUF_SIZE ];

    if ( !gds_tp_dir )
    {
        gds_tp_dir = getenv( "GDS_TP_DIR" );
    }
    if ( gds_tp_dir )
    {
        sprintf( gdsfile, "%s/tpchn_%s.par", gds_tp_dir, modelname );
    }
    else
    {
        /// Need to get IFO and SITE info from environment
        /// variables.
        get_env_lower( "IFO", ifo, ENV_BUF_SIZE );
        get_env_lower( "SITE", site, ENV_BUF_SIZE );

        sprintf( gdsfile,
                 "/opt/rtcds/%s/%s/target/gds/param/tpchn_%s.par",
                 site,
                 ifo,
                 modelname );
    }
    f = fopen( gdsfile, "rt" );
    if ( !f )
        return 0;
    while ( fgets( line, 80, f ) != NULL )
    {
        line[ strcspn( line, "\n" ) ] = 0;
        if ( strstr( line, "datarate" ) != NULL )
        {
            token = strtok( line, search );
            token = strtok( NULL, search );
            if ( !token )
                continue;
            while ( *token && *token == ' ' )
            {
                ++token;
            }
            *rate = atoi( token );
            break;
        }
    }
    fclose( f );
    f = fopen( gdsfile, "rt" );
    if ( !f )
        return 0;
    while ( fgets( line, 80, f ) != NULL )
    {
        line[ strcspn( line, "\n" ) ] = 0;
        if ( strstr( line, "rmid" ) != NULL )
        {
            token = strtok( line, search );
            token = strtok( NULL, search );
            if ( !token )
                continue;
            while ( *token && *token == ' ' )
            {
                ++token;
            }
            *dcuid = atoi( token );
            break;
        }
    }
    fclose( f );

    return 0;
}