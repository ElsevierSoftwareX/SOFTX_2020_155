#include <cassert>
#include <iostream>

#include "gpstime.h"
#include "datasrv.h"

using namespace th_gps;

int main(int argc, char *argv[]) {
    std::string t1 = get_utc_time(1180000000);
    std::cout << "t1 = '" << t1 << "'" << std::endl;
    assert(t1 == "17-05-28-09-46-22");

    long t2 = get_gps_time("2017-05-28T 09:46:22z");
    std::cout << "t2 = " << t2 << std::endl;
    assert(t2 == 1180000000);

    std::string in3("17-05-28-09-46-22");
    long t3 = DataUTCtoGPS(const_cast<char *>(in3.c_str()));
    std::cout << "t3 = " << t3 << std::endl;
    assert(t3 == 1180000000);

    char tmp[60];
    DataGPStoUTC(1180000000, tmp);
    std::string t4(tmp);
    std::cout << "t4 = '" << t4 << "'" << std::endl;
    assert(t4 == "17-05-28-09-46-22");

    time_t t5 = DataTimeNow();
    std::cout << "t5 = " << t5 << std::endl;
    assert(t5 > 1180000000);
    return 0;
}
