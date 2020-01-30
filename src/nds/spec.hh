#ifndef CDS_NDS_SPEC_HH

#define CDS_NDS_SPEC_HH

#include <string>
#include <iostream>
#include <vector>
#include "config.h"
#include "debug.h"
#include <assert.h>

namespace CDS_NDS
{
    class Spec;
}

namespace CDS_NDS
{

    /// Job specification.
    class Spec
    {
    public:
        /// Requested data type.
        typedef enum
        {
            FullData,
            TrendData,
            MinuteTrendData,
            RawMinuteTrendData
        } DataClassType;

        /// Sample data type.
        typedef enum
        {
            _undefined = 0,
            _16bit_integer = 1,
            _32bit_integer = 2,
            _64bit_integer = 3,
            _32bit_float = 4,
            _64bit_double = 5,
            _32bit_complex = 6,
            _32bit_uint = 7
        } DataTypeType;

        /// Get the data type string
        inline static const std::string
        dataTypeString( DataTypeType d )
        {
            switch ( d )
            {
            default:
            case _undefined:
                return "unknown";
            case _16bit_integer:
                return "_16bit_integer";
            case _32bit_integer:
                return "_32bit_integer";
            case _64bit_integer:
                return "_64bit_integer";
            case _32bit_float:
                return "_32bit_float";
            case _64bit_double:
                return "_64bit_double";
            case _32bit_complex:
                return "_32bit_complex";
            case _32bit_uint:
                return "_32bit_uint";
            }
            return "unknown";
        }

        /// Default constructor.
        Spec( );

        /// Parse the job specification file.
        ///
        ///     @param[in] s	Job specification file name.
        ///     @return True if he input file was parse successfully.
        bool parse( std::string s );

        /// Auxiliary data archive outside of the frame builder domain.
        class AddedArchive
        {
        public:
            /// Default constructor.
            AddedArchive( ) : prefix( ), suffix( ), gps( )
            {
            }
            /// Constructor.
            ///	@param[in] &d Directory.
            ///	@param[in] &p Prefix.
            ///	@param[in] &s Suffix.
            ///	@param[in] &g A vector of GPS time ranges.
            AddedArchive(
                std::string&                                              d,
                std::string&                                              p,
                std::string&                                              s,
                std::vector< std::pair< unsigned long, unsigned long > >& g )
                : dir( d ), prefix( p ), suffix( s ), gps( g )
            {
            }
            /// See whether all fields are set.
            inline bool
            complete( )
            {
                return ( dir.length( ) > 0 && prefix.length( ) > 0 &&
                         suffix.length( ) > 0 && gps.size( ) > 0 );
            }
            /// Archive directory.
            std::string dir;
            /// Archive file name prefix.
            std::string prefix;
            /// Archive file name suffix.
            std::string                                              suffix;
            std::vector< std::pair< unsigned long, unsigned long > > gps;
        };

        /// Get the type of data requested.
        DataClassType
        getDataType( ) const throw( )
        {
            return mDataType;
        };
        /// Get the GPS timestamp of the first data sample.
        unsigned long
        getStartGpsTime( ) const throw( )
        {
            return mStartGpsTime;
        };
        /// Get the GPS timestamp of the last data sample.
        unsigned long
        getEndGpsTime( ) const throw( )
        {
            return mEndGpsTime;
        };
        /// Get the name of decimation filter.
        std::string
        getFilter( ) const throw( )
        {
            return mFilter;
        };
        /// Get the vector of signal names.
        const std::vector< std::string >&
        getSignalNames( ) const throw( )
        {
            return mSignalNames;
        };
        /// Get the offsets of the signals.
        const std::vector< float >&
        getSignalOffsets( ) const throw( )
        {
            return mSignalOffsets;
        };
        /// Get the slopes of the data channels.
        const std::vector< float >&
        getSignalSlopes( ) const throw( )
        {
            return mSignalSlopes;
        };
        /// Get sampling rates of the data channels.
        const std::vector< unsigned int >&
        getSignalRates( ) const throw( )
        {
            return mSignalRates;
        };
        /// Get signal types of the data channels.
        const std::vector< DataTypeType >&
        getSignalTypes( ) const throw( )
        {
            return mSignalTypes;
        };
        /// Get bytes per second values for the data channels.
        const std::vector< unsigned int >&
        getSignalBps( ) const throw( )
        {
            return mSignalBps;
        };
        /// Return the name of the results file.
        const std::string
        getDaqdResultFile( ) const throw( )
        {
            return mDaqdResultFile;
        };
        /// Return the name of the primary archive directory.
        const std::string
        getArchiveDir( ) const throw( )
        {
            return mArchiveDir;
        };
        /// Return the prefix of the primary archive.
        const std::string
        getArchivePrefix( ) const throw( )
        {
            return mArchivePrefix;
        };
        /// Return the suffix of the primary archive.
        const std::string
        getArchiveSuffix( ) const throw( )
        {
            return mArchiveSuffix;
        };
        /// Return the vector of additional archives.
        const std::vector< AddedArchive >&
        getAddedArchives( ) const throw( )
        {
            return mAddedArchives;
        };
        /// Return the vector if the additional flags.
        const std::vector< std::string >&
        getAddedFlags( ) const throw( )
        {
            return mAddedFlags;
        };
        /// Split the string on whie space into  vector if strings.
        static const std::vector< std::string > split( std::string ) throw( );
        /// Get the vector of GPS ranges for the additional archives.
        const std::vector< std::pair< unsigned long, unsigned long > >
        getArchiveGps( ) const throw( )
        {
            return mArchiveGps;
        };

        /// Assign the main data archive.
        void
        setMainArchive( const AddedArchive& a )
        {
            mArchiveDir = a.dir;
            mArchivePrefix = a.prefix;
            mArchiveSuffix = a.suffix;
            mArchiveGps = a.gps;
        }
        /// Set the type of the data requested.
        void
        setDataType( DataClassType t )
        {
            mDataType = t;
        }
        /// Get the vector of sampling rates of the data channels.
        void
        setSignalRates( const std::vector< unsigned int >& v )
        {
            mSignalRates = v;
        }
        /// Set the all the data channel slopes to the passed value.
        void
        setSignalSlopes( double val )
        {
            for ( int i = 0; i < mSignalSlopes.size( ); i++ )
                mSignalSlopes[ i ] = val;
        }

        /// Is the values a power of r?
        static int
        power_of( int value, int r )
        {
            int rm;

            assert( value > 0 && r > 1 );
            if ( value == 1 )
                return 1;

            do
            {
                if ( value % r )
                    return 0;
                value /= r;
            } while ( value > 1 );

            return 1;
        }

    private:
        DataClassType              mDataType; //< Type of the data.
        unsigned long              mStartGpsTime;
        unsigned long              mEndGpsTime;
        std::string                mFilter; //< How to decimate data
        std::vector< std::string > mSignalNames; //< The list of signals.
        std::vector< unsigned int >
            mSignalRates; //< The list of requested signal rates.
        std::vector< unsigned int >
                                    mSignalBps; //< List of signals' bytes per sample.
        std::vector< DataTypeType > mSignalTypes; //< List of signal data types.
        std::vector< float >
            mSignalOffsets; //< List of signal offset conversion values.
        std::vector< float >
                    mSignalSlopes; //< List of signal slope conversion values.
        std::string mDaqdResultFile; //< Results produced by the DAQD.
        std::string mArchiveDir; //< Location of main data archive
        std::string mArchivePrefix;
        std::string mArchiveSuffix;
        std::vector< AddedArchive > mAddedArchives; //< List of added archives
                                                    //(identified by dir namea).
        std::vector< std::string >
            mAddedFlags; //< Establishes signal membership in archives, "0"
                         //means main archive, dir name for added archive
        /// Gps times for the data archive "Data*" directories.
        std::vector< std::pair< unsigned long, unsigned long > > mArchiveGps;
    };

} // namespace CDS_NDS

#endif
