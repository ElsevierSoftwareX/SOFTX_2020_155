#ifndef FFERRORS_HH
#define FFERRORS_HH

#include <stdexcept>

//--------------------------------------  Error classes
class BadFile : public std::runtime_error {
public:
    BadFile( const std::string& msg );
};

class NoData : public std::runtime_error {
public:
    NoData( const std::string& msg );
};

inline
BadFile::BadFile( const std::string& msg )
        : std::runtime_error( msg )
{}

inline
NoData::NoData( const std::string& msg )
        : std::runtime_error( msg )
{}

#endif
