//
// Created by jonathan.hanks on 11/10/17.
//

#ifndef DAQD_TRUNK_STR_SPLIT_HH
#define DAQD_TRUNK_STR_SPLIT_HH

#include <string>
#include <vector>

enum split_type
{
    INCLUDE_EMPTY_STRING = 0,
    EXCLUDE_EMPTY_STRING = 1,
};

extern std::vector< std::string >
split( const std::string& source,
       const std::string& sep,
       split_type         filter_mode = INCLUDE_EMPTY_STRING );

#endif // DAQD_TRUNK_STR_SPLIT_HH
