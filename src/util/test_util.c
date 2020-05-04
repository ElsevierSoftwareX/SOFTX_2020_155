//
// Created by jonathan.hanks on 5/4/20.
//
#include "modelrate.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 16

int
main( int argc, char* argv[] )
{
    char buffer[ BUFFER_SIZE ] = "";

    setenv( "TEST_VAL", "Short", 1 );
    get_env_lower( "TEST_VAL", buffer, BUFFER_SIZE );
    if ( strcmp( buffer, "short" ) != 0 )
    {
        fprintf( stderr, "Unexpected value, '%s' != 'short'\n", buffer );
        exit( 1 );
    }

    setenv( "TEST_VAL", "Longer Value Here 01234567890", 1 );
    get_env_lower( "TEST_VAL", buffer, BUFFER_SIZE );
    if ( strcmp( buffer, "longer value he" ) != 0 )
    {
        fprintf(
            stderr, "Unexpected value, '%s' != 'longer value he'\n", buffer );
        exit( 1 );
    }

    setenv( "TEST_VAL", "Correct Length!", 1 );
    get_env_lower( "TEST_VAL", buffer, BUFFER_SIZE );
    if ( strcmp( buffer, "correct length!" ) != 0 )
    {
        fprintf(
            stderr, "Unexpected value, '%s' != 'Correct Length!'\n", buffer );
        exit( 1 );
    }

    //bad variable name should return empty string
    get_env_lower("TEST_DOESNT_EXIST", buffer, BUFFER_SIZE );
    if ( buffer[0] )
    {
        fprintf(
            stderr, "Bad environment variable does not yield empty string");
            exit(1);
    }

    // Test get_model_rate_dcuid()
    static int rate=0;
    static int dcuid=0;
    static const char *test_model_name = "testmodel";

    get_model_rate_dcuid(&rate, &dcuid, test_model_name, "test");
    if (dcuid != 59 || rate != 2048)
    {
        fprintf(
            stderr, "tpdir_arg: Got bad dcuid: %d or rate: %d", dcuid, rate);
        exit(1);
    }

    rate=0; dcuid=0;

    get_model_rate_dcuid(&rate, &dcuid, test_model_name, NULL);
    if (dcuid != 0 || rate != 0)
    {
        fprintf(
            stderr, "tpdir_missing: Got bad dcuid: %d or rate: %d", dcuid, rate);
        exit(1);
    }


    rate=0; dcuid=0;

    char * gds_tp_dir_old =getenv("GDS_TP_DIR");
    setenv("GDS_TP_DIR", "test",1);
    get_model_rate_dcuid(&rate, &dcuid, test_model_name, NULL);
    if (dcuid != 59 || rate != 2048)
    {
        fprintf(
            stderr, "tpdir_env: Got bad dcuid: %d or rate: %d", dcuid, rate);
        exit(1);
    }
    if(gds_tp_dir_old)
    {
        setenv("GDS_TP_DIR", gds_tp_dir_old, 1);
    }

    rate=0; dcuid=0;

    get_model_rate_dcuid(&rate, &dcuid, "badrate", NULL);
    if (dcuid != 59 || rate != 0)
    {
        fprintf(
            stderr, "tpdir_badrate: Got bad dcuid: %d or rate: %d", dcuid, rate);
        exit(1);
    }

    rate=0; dcuid=0;

    get_model_rate_dcuid(&rate, &dcuid, "baddcuid", NULL);
    if (dcuid != 0 || rate != 2048)
    {
        fprintf(
            stderr, "tpdir_baddcuid: Got bad dcuid: %d or rate: %d", dcuid, rate);
        exit(1);
    }

    rate=0; dcuid=0;

    get_model_rate_dcuid(&rate, &dcuid, "missingrate", NULL);
    if (dcuid != 59 || rate != 0)
    {
        fprintf(
            stderr, "tpdir_missingrate: Got bad dcuid: %d or rate: %d", dcuid, rate);
        exit(1);
    }

    rate=0; dcuid=0;

    get_model_rate_dcuid(&rate, &dcuid, "missingdcuid", NULL);
    if (dcuid != 0 || rate != 2048)
    {
        fprintf(
            stderr, "tpdir_missingdcuid: Got bad dcuid: %d or rate: %d", dcuid, rate);
        exit(1);
    }

    return 0;
}