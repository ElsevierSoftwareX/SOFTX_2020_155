#include "channel.hh"
#include "archive.hh"
#include "daqc.h"

/*
  Signal configuration file looks like this:
  [datatype]
  minutetrend
  [Signals]
  H2:PSL-ISS_ISERR_Fband900-7400 32bit_float

  Allowed datatypes are full, secondtrend and minutetrend.
  Allowed signal types are 16bit_integer, 32bit_integer, 32bit_float and
  64bit_double.

  If data type is secondtrend or minutetrend, then it is assumed that frames
  contain five ADC channels per name with suffixes .min, .max, .n, .rms, and
  .mean. .n signal should be 16bit_integers, .rms and .mean signals should be
  64bit_doubles and .min and .max have the data type indicated by the config
  file line.
*/

/* Read configuration file and load config data */
int
archive_c::scan( FILE* filep, bool old )
{
    int linenum = 0;
    enum
    {
        none,
        datatype,
        signals
    } stype = none;
    char  buf[ 1024 ];
    char* lptr;
    for ( ;; )
    {
        if ( !fgets( buf, 1024, filep ) )
            return ferror( filep ) ? linenum : 0;

        linenum++;

        /* Skip leading spaces */
        for ( lptr = buf; *lptr && isspace( *lptr ); lptr++ )
            ;

        /* Empty line */
        if ( !*lptr )
            continue;

        /* Trim trailing spaces */
        for ( int i = strlen( lptr ) - 1; i && isspace( lptr[ i ] ); i-- )
            lptr[ i ] = 0;

        /* Comment */
        if ( *lptr == '#' )
            continue;

        if ( *lptr == '[' )
        { /* Section */
            char* ptr = 0;
            lptr++;
            /* Find closing bracket */
            for ( ptr = lptr; *ptr && *ptr != ']'; ptr++ )
                ;
            if ( !*ptr )
            {
                /* Malformed section header */
                return linenum;
            }
            *ptr = 0;
            if ( !strcasecmp( lptr, "datatype" ) )
            {
                stype = datatype;
            }
            else if ( !strcasecmp( lptr, "signals" ) )
            {
                stype = signals;
            }
            else
            {
                /* Unknown section */
                return linenum;
            }
        }
        else
        {
            switch ( stype )
            {
            case datatype:
                if ( !strcasecmp( lptr, "full" ) )
                {
                    data_type = full;
                }
                else if ( !strcasecmp( lptr, "secondtrend" ) )
                {
                    data_type = secondtrend;
                }
                else if ( !strcasecmp( lptr, "minutetrend" ) )
                {
                    data_type = minutetrend;
                }
                else
                {
                    /* Unknown data type */
                    return linenum;
                }
                break;
            case signals:
            {
                /* Find end of first word */
                char* swptr;
                for ( swptr = lptr; *swptr && !isspace( *swptr ); swptr++ )
                    ;
                if ( !*swptr )
                {
                    /* Must be two or three words per line in signals section */
                    return linenum;
                }

                /* End of first word */
                *swptr = 0;

                /* Skip white space until next word */
                for ( ++swptr; *swptr && isspace( *swptr ); swptr++ )
                    ;

                if ( !*swptr )
                {
                    /* Must be two or three words per line in signals section */
                    return linenum;
                }
                /* First word is signal name
                   Second word is signal data type */
                daq_data_t chtype = _undefined;
                if ( !strncasecmp( swptr, "16bit_integer", 13 ) )
                {
                    chtype = _16bit_integer;
                }
                else if ( !strncasecmp( swptr, "32bit_integer", 13 ) )
                {
                    chtype = _32bit_integer;
                }
                else if ( !strncasecmp( swptr, "32bit_float", 11 ) )
                {
                    chtype = _32bit_float;
                }
                else if ( !strncasecmp( swptr, "64bit_double", 12 ) )
                {
                    chtype = _64bit_double;
                }
                else
                {
                    /* invalid signal type */
                    return linenum;
                }

                /* Skip the word */
                for ( ; *swptr && !isspace( *swptr ); swptr++ )
                    ;

                unsigned int rate = 0;
                if ( *swptr )
                {
                    /* Skip white space until next word */
                    for ( ++swptr; *swptr && isspace( *swptr ); swptr++ )
                        ;

                    if ( *swptr )
                    {
                        /* rate is specified */
                        rate = atoi( swptr );
                    }
                }
                if ( rate <= 0 )
                    rate = channel_t::arc_rate;

                if ( strlen( lptr ) > MAX_LONG_CHANNEL_NAME_LENGTH )
                    return linenum;
                //	  printf("%s\t%s\n", lptr, swptr);
                struct channel* ch = add_channel( );
                ch->name = strdup( lptr );
                ch->type = chtype;
                ch->old = old;
                ch->rate = rate;
            }
            break;
            default:
                return linenum;
            }
        }
    }
}

/* Scan file names  */
int
archive_c::scan( char* name, char* prefix, char* suffix, int ndirs )
{
    locker mon( this );

    if ( fsd.set_filename_attrs( suffix, prefix, name ) < 0 )
        return DAQD_ERROR;
    fsd.set_num_dirs( ndirs );
    // Scan file names in the archive
    if ( fsd.scan( ) < 0 )
        return DAQD_MALLOC;
    return DAQD_OK;
}

/* Load signal data config from a config file */
int
archive_c::load_config( char* fname )
{
    locker mon( this );

    FILE* filep;
    filep = fopen( fname, "r" );
    if ( !filep )
    {
        system_log( 1, "Couldn't open archive config file `%s'", fname );
        return 1;
    }
    free_channel_config( );
    int res = scan( filep );
    if ( res )
    {
        free_channel_config( );
        system_log( 1,
                    "Archive config file `%s'; syntax error on line %d",
                    fname,
                    res );
    }
    fclose( filep );
    return res;
}

/* Input old channel configuration */
int
archive_c::load_old_config( char* fname )
{
    locker mon( this );

    FILE* filep;
    filep = fopen( fname, "r" );
    if ( !filep )
    {
        system_log( 1, "Couldn't open archive config file `%s'", fname );
        return 0; // Ignore missing old channel config file error
    }
    int res = scan( filep, 1 );
    if ( res )
    {
        free_channel_config( );
        system_log( 1,
                    "Archive config file `%s'; syntax error on line %d",
                    fname,
                    res );
    }
    fclose( filep );
    return res;
}
