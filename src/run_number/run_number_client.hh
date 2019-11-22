#ifndef DAQD_RUN_NUMBER_CLIENT_HH
#define DAQD_RUN_NUMBER_CLIENT_HH

#include <string>

namespace daqd_run_number
{

    int get_run_number( const std::string& target, const std::string& hash );

}

#endif // DAQD_RUN_NUMBER_CLIENT_HH