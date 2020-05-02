#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// **********************************************************************************************
// Get control model loop rates from GDS param files
// Needed to properly size TP data into the data stream
int
getmodelrate( int*        rate,
              int*        dcuid,
              const char* modelname,
              char*       gds_tp_dir )
{
    char        gdsfile[ 128 ];
    int         ii = 0;
    FILE*       f = 0;
    char*       token = 0;
    const char* search = "=";
    char        line[ 80 ];
    char*       s = 0;
    char*       s1 = 0;

    if ( !gds_tp_dir )
    {
        gds_tp_dir = getenv( "GDS_TP_DIR" );
    }
    if ( gds_tp_dir )
    {
        sprintf(
            gdsfile, "%s/tpchn_%s.par", gds_tp_dir, modelname );
    }
    else
    {
        /// Need to get IFO and SITE info from environment
        /// variables.
        s = getenv( "IFO" );
        for ( ii = 0; s[ ii ] != '\0'; ii++ )
        {
            if ( isupper( s[ ii ] ) )
                s[ ii ] = (char)tolower( s[ ii ] );
        }
        s1 = getenv( "SITE" );
        for ( ii = 0; s1[ ii ] != '\0'; ii++ )
        {
            if ( isupper( s1[ ii ] ) )
                s1[ ii ] = (char)tolower( s1[ ii ] );
        }
        sprintf( gdsfile,
                 "/opt/rtcds/%s/%s/target/gds/param/tpchn_%s.par",
                 s1,
                 s,
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