#include <stdlib.h>
#include <fstream>
#include "spec.hh"

using namespace CDS_NDS;

Spec::Spec( )
    : mDataType( FullData ), mStartGpsTime( 0 ), mEndGpsTime( 0 ),
      mFilter( "average" ), mSignalNames( ), mSignalOffsets( ),
      mSignalSlopes( ), mSignalRates( ), mSignalBps( ), mSignalTypes( ),
      mDaqdResultFile( ), mArchiveDir( ), mArchivePrefix( ), mArchiveSuffix( ),
      mAddedArchives( ), mAddedFlags( ), mArchiveGps( )
{
}

const std::vector< std::string >
Spec::split( std::string value ) throw( )
{
    std::vector< std::string > res;
    for ( ;; )
    {
        int idx = value.find_first_of( " " );
        if ( idx == std::string::npos )
        {
            if ( value.size( ) > 0 )
                res.push_back( value );
            break;
        }
        std::string sname = value.substr( 0, idx );
        if ( sname != " " && sname.size( ) > 0 )
            res.push_back( sname );
        value = value.substr( idx + 1 );
    }
    return res;
}

char buf[ 1024 * 1024 * 2 ];

bool
Spec::parse( std::string fname )
{
    char*         ser = (char*)"%s:%d: syntax error";
    std::ifstream in( fname.c_str( ) );
    if ( !in )
    {
        system_log( 1, "%s: open failed", fname.c_str( ) );
        return false;
    }

    // Archive section variables
    std::string archive_dir;
    std::string archive_prefix;
    std::string archive_suffix;

    for ( int line = 1; !in.eof( ); line++ )
    {
        in.getline( buf, 1024 * 1024 * 2 );
        std::string s( buf );
        // DEBUG(5, std::cerr << s <<endl);
        if ( s[ 0 ] == '#' )
            continue;
        std::string key, value;

        int idx = s.find_first_of( "=" );
        if ( idx == s.npos )
        {
            // Check if this is a section header
            int ob_idx = s.find_first_of( "[" );
            int cb_idx = s.find_first_of( "]" );
            if ( ob_idx == s.npos || cb_idx == s.npos || cb_idx <= ob_idx + 1 )
            {
                //	system_log(1, "%s:%d semantic error; invalid section
                //header", fname.c_str(), line); 	return false;
                continue;
            }
            archive_dir = s.substr( ob_idx + 1, cb_idx - ob_idx - 1 );
            DEBUG( 5,
                   std::cerr << "Added archives section: " << archive_dir
                             << std::endl );
            continue;
        }
        key = s.substr( 0, idx );
        value = s.substr( idx + 1 );
        DEBUG( 5, std::cerr << key << "\t" << value << std::endl );
        if ( key == "added_flags" )
        {
            mAddedFlags.clear( );
            mAddedFlags = split( value );
        }
        else if ( key == "added_prefix" )
        {
            archive_prefix = value;
        }
        else if ( key == "added_suffix" )
        {
            archive_suffix = value;
        }
        else if ( key == "added_times" )
        {
            std::vector< std::string > v = split( value );

            if ( v.size( ) % 2 )
            {
                system_log(
                    1,
                    "%s:%d semantic error; must be even number of values",
                    fname.c_str( ),
                    line );
                return false;
            }

            std::vector< std::pair< unsigned long, unsigned long > >
                archive_gps;
            for ( int i = 0; i < v.size( ); i++ )
            {
                char*         endptr;
                unsigned long gps1 = strtoul( v[ i++ ].c_str( ), &endptr, 0 );
                if ( *endptr )
                {
                    system_log( 1, ser, fname.c_str( ), line );
                    return false;
                }
                unsigned long gps2 = strtoul( v[ i ].c_str( ), &endptr, 0 );
                if ( *endptr )
                {
                    system_log( 1, ser, fname.c_str( ), line );
                    return false;
                }
                archive_gps.push_back(
                    std::pair< unsigned long, unsigned long >( gps1, gps2 ) );
            }

            // This is the last line in the archive section -- put the archive
            // in
            AddedArchive a(
                archive_dir, archive_prefix, archive_suffix, archive_gps );
            if ( !a.dir.length( ) || !a.prefix.length( ) ||
                 !a.suffix.length( ) )
            {
                DEBUG( 5,
                       std::cerr << archive_dir << " " << archive_prefix << " "
                                 << archive_suffix << " " << archive_gps.size( )
                                 << std::endl );
                system_log(
                    1,
                    "%s:%d: semantic error (incomplete added archives section)",
                    fname.c_str( ),
                    line );
                return false;
            }
            mAddedArchives.push_back( a );
        }
        else if ( key == "datatype" )
        {
            if ( value == "full" )
                mDataType = FullData;
            else if ( value == "secondtrend" )
                mDataType = TrendData;
            else if ( value == "minutetrend" )
                mDataType = MinuteTrendData;
            else if ( value == "rawminutetrend" )
                mDataType = RawMinuteTrendData;
            else
            {
                system_log( 1, ser, fname.c_str( ), line );
                return false;
            }
        }
        else if ( key == "startgps" || key == "endgps" )
        {
            char*         endptr;
            unsigned long lval = strtoul( value.c_str( ), &endptr, 0 );
            if ( *endptr )
            {
                system_log( 1, ser, fname.c_str( ), line );
                return false;
            }
            if ( key == "endgps" )
                mEndGpsTime = lval;
            else
                mStartGpsTime = lval;
        }
        else if ( key == "decimate" )
        {
            if ( value == "average" || value == "nofilter" )
                mFilter = value;
            else
            {
                system_log( 1, ser, fname.c_str( ), line );
                return false;
            }
        }
        else if ( key == "signals" )
        {
            mSignalNames.clear( );
            mSignalNames = split( value );
        }
        else if ( key == "signal_offsets" )
        {
            mSignalOffsets.clear( );
            std::vector< std::string > v = split( value );
            for ( int i = 0; i < v.size( ); i++ )
            {
                char* endptr;
                float lval = (float)strtod( v[ i ].c_str( ), &endptr );
                if ( *endptr )
                {
                    system_log( 1,
                                "%s:%d: syntax error (not a number)",
                                fname.c_str( ),
                                line );
                    return false;
                }
                mSignalOffsets.push_back( lval );
            }
        }
        else if ( key == "signal_slopes" )
        {
            mSignalSlopes.clear( );
            std::vector< std::string > v = split( value );
            for ( int i = 0; i < v.size( ); i++ )
            {
                char* endptr;
                float lval = (float)strtod( v[ i ].c_str( ), &endptr );
                if ( *endptr )
                {
                    system_log( 1,
                                "%s:%d: syntax error (not a number)",
                                fname.c_str( ),
                                line );
                    return false;
                }
                mSignalSlopes.push_back( lval );
            }
        }
        else if ( key == "rates" )
        {
            mSignalRates.clear( );
            std::vector< std::string > v = split( value );
            for ( int i = 0; i < v.size( ); i++ )
            {
                char*         endptr;
                unsigned long lval = strtoul( v[ i ].c_str( ), &endptr, 0 );
                if ( *endptr )
                {
                    system_log( 1,
                                "%s:%d: syntax error (not a number)",
                                fname.c_str( ),
                                line );
                    return false;
                }
                if ( !power_of( lval, 2 ) )
                {
                    system_log( 1,
                                "%s:%d: syntax error (not power of 2)",
                                fname.c_str( ),
                                line );
                    return false;
                }
                mSignalRates.push_back( lval );
            }
        }
        else if ( key == "types" )
        {
            mSignalTypes.clear( );
            mSignalBps.clear( );
            std::vector< std::string > v = split( value );
            for ( int i = 0; i < v.size( ); i++ )
            {
                if ( v[ i ] == "_16bit_integer" )
                {
                    mSignalTypes.push_back( _16bit_integer );
                    mSignalBps.push_back( 2 );
                }
                else if ( v[ i ] == "_32bit_integer" )
                {
                    mSignalTypes.push_back( _32bit_integer );
                    mSignalBps.push_back( 4 );
                }
                else if ( v[ i ] == "_64bit_integer" )
                {
                    mSignalTypes.push_back( _64bit_integer );
                    mSignalBps.push_back( 8 );
                }
                else if ( v[ i ] == "_32bit_float" )
                {
                    mSignalTypes.push_back( _32bit_float );
                    mSignalBps.push_back( 4 );
                }
                else if ( v[ i ] == "_64bit_double" )
                {
                    mSignalTypes.push_back( _64bit_double );
                    mSignalBps.push_back( 8 );
                }
                else if ( v[ i ] == "_32bit_complex" )
                {
                    mSignalTypes.push_back( _32bit_complex );
                    mSignalBps.push_back( 8 );
                }
                else if ( v[ i ] == "_32bit_uint" )
                {
                    mSignalTypes.push_back( _32bit_uint );
                    mSignalBps.push_back( 4 );
                }
                else if ( v[ i ] == "unknown" )
                {
                    mSignalTypes.push_back( _undefined );
                    mSignalBps.push_back( 0 );
                }
                else
                {
                    system_log( 1, ser, fname.c_str( ), line );
                    return false;
                }
            }
        }
        else if ( key == "daqd_result" )
        {
            mDaqdResultFile = value;
        }
        else if ( key == "archive_dir" )
        {
            mArchiveDir = value;
        }
        else if ( key == "archive_prefix" )
        {
            mArchivePrefix = value;
        }
        else if ( key == "archive_suffix" )
        {
            mArchiveSuffix = value;
        }
        else if ( key == "times" )
        {
            std::vector< std::string > v = split( value );

            if ( v.size( ) % 2 )
            {
                system_log(
                    1,
                    "%s:%d semantic error; must be even number of values",
                    fname.c_str( ),
                    line );
                return false;
            }
            for ( int i = 0; i < v.size( ); i++ )
            {
                char*         endptr;
                unsigned long gps1 = strtoul( v[ i++ ].c_str( ), &endptr, 0 );
                if ( *endptr )
                {
                    system_log( 1, ser, fname.c_str( ), line );
                    return false;
                }
                unsigned long gps2 = strtoul( v[ i ].c_str( ), &endptr, 0 );
                if ( *endptr )
                {
                    system_log( 1, ser, fname.c_str( ), line );
                    return false;
                }
                mArchiveGps.push_back(
                    std::pair< unsigned long, unsigned long >( gps1, gps2 ) );
            }
        }
        else
        {
            system_log( 5,
                        "unknown key `%s' in %s:%d",
                        key.c_str( ),
                        fname.c_str( ),
                        line );
        }
    }

    if ( mDataType == TrendData )
    { // sampled at 1Hz always
        mSignalRates.clear( );
        mSignalRates = std::vector< unsigned int >( mSignalNames.size( ), 1 );
    }

    if ( ( mSignalNames.size( ) != mSignalRates.size( ) &&
           mDataType != MinuteTrendData && mDataType != RawMinuteTrendData ) ||
         ( mSignalNames.size( ) != mSignalBps.size( ) ) ||
         ( mSignalNames.size( ) != mSignalTypes.size( ) ) )
    {
        system_log(
            1, "%s: semantic error;  vector length mismatch", fname.c_str( ) );
        return false;
    }

    if ( mDataType == RawMinuteTrendData &&
         ( ( mSignalNames.size( ) != mSignalOffsets.size( ) ) ||
           ( mSignalNames.size( ) != mSignalSlopes.size( ) ) ) )
    {
        system_log(
            1,
            "%s: semantic error; conversion data vector length mismatch",
            fname.c_str( ) );
        return false;
    }

    if ( ( mDataType == MinuteTrendData || mDataType == RawMinuteTrendData ) &&
         mSignalNames.size( ) != mSignalRates.size( ) )
    {
        // All signals will be sampled at 1 (actual sampling rates is 1/60)
        setSignalRates(
            std::vector< unsigned int >( mSignalNames.size( ), 1 ) );
    }

    // Make sure this is request for minute trend if there are DMT channels
    // present
    if ( mDataType == TrendData || mDataType == FullData )
    {
        int num_channels = mSignalNames.size( );
        for ( int i = 0; i < num_channels; i++ )
        {
            int idx = mSignalNames[ i ].find( ":DMT",
                                              0 ); // See if this is DMT channel
            if ( idx != std::string::npos )
            { // Found a DMT channel
                // Bad request
                system_log( 1,
                            "%s: DMT request denied, not minute trend\n",
                            fname.c_str( ) );
                return false;
            }
        }
    }

    return true;
}
