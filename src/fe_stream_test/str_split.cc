#include "str_split.hh"

std::vector< std::string >
split( const std::string& source,
       const std::string& sep,
       split_type         filter_mode )
{
    std::vector< std::string > results;

    if ( source == "" )
        return results;
    std::string::size_type prev = 0;
    std::string::size_type pos = 0;
    if ( sep.size( ) > 0 )
    {
        std::string::size_type sep_size = sep.size( );
        while ( pos < source.size( ) && pos != std::string::npos )
        {
            pos = source.find( sep, prev );
            if ( pos != std::string::npos )
            {
                std::string tmp = source.substr( prev, pos - prev );
                if ( tmp != "" || filter_mode == INCLUDE_EMPTY_STRING )
                    results.push_back( tmp );
                pos += sep_size;
                prev = pos;
            }
        }
    }
    else
    {
        while ( pos < source.size( ) && pos != std::string::npos )
        {
            pos = source.find_first_of( " \t\n\r", prev );
            if ( pos != std::string::npos )
            {
                std::string tmp = source.substr( prev, pos - prev );
                if ( tmp != "" || filter_mode == INCLUDE_EMPTY_STRING )
                    results.push_back( tmp );
                ++pos;
                prev = pos;
            }
        }
    }
    std::string tmp = source.substr( prev );
    if ( tmp != "" || filter_mode == INCLUDE_EMPTY_STRING )
        results.push_back( tmp );
    return results;
}