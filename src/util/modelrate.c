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
    char*       ifo_val = 0;
    char        ifo[16];
    char*       site_val = 0;
    char        site[16];

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
        ifo_val = getenv( "IFO" );
        for ( ii = 0; ii < sizeof(ifo)-1 && ifo_val && ifo_val[ ii ] != '\0'; ii++ )
        {
            if ( isupper( ifo_val[ ii ] ) )
                ifo[ ii ] = (char)tolower( ifo_val[ ii ] );
            else
                ifo[ ii ] = ifo_val[ ii ];
        }
        ifo[ ii ] = 0;

        site_val = getenv( "SITE" );
        for ( ii = 0; ii < sizeof(site)-1 && site_val && site_val[ ii ] != '\0'; ii++ )
        {
            if ( isupper( site_val[ ii ] ) )
                site[ ii ] = (char)tolower( site_val[ ii ] );
            else
                site[ ii ] = site_val [ ii ];
        }
        site[ ii ] = 0;

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