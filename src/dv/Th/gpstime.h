#ifndef TH_GPSTIME_H
#define TH_GPSTIME_H

#include <stdio.h>

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <sstream>

namespace th_gps {

struct pcloser {
    void operator()(FILE *f) {
        if (f) {
            pclose(f);
        }
    }
};
using pclose_ptr = std::unique_ptr<FILE, pcloser>;

static inline std::string get_utc_time(long gpsin) {
    const int str_len = (6*2)+5;
    std::ostringstream os;
    os << "/usr/bin/gpstime -u -f \"%y-%m-%d-%H-%M-%S\" " << gpsin;
    std::string cmd_string(os.str());

    pclose_ptr _p(popen(cmd_string.c_str(), "r"));

    std::array<char, 512> buf;

    int count = fread(reinterpret_cast<void *>(&buf[0]), 1, buf.size(), _p.get());
    if (count < str_len) throw std::runtime_error("Unknown time format returned");
    return std::string(buf.begin(), buf.begin()+str_len);
}

static inline long get_gps_time(const std::string &date_str) {
    std::ostringstream os;
    os << "/usr/bin/gpstime -g " << date_str;
    std::string cmd_string(os.str());

    pclose_ptr _p(popen(cmd_string.c_str(), "r"));

    std::array<char, 512> buf;

    int count = fread(reinterpret_cast<void *>(&buf[0]), 1, buf.size(), _p.get());
    std::array<char, 512>::iterator it = std::find(buf.begin(), buf.begin()+count, '.');
    if (it != buf.end()) {
        *it = '\0';
    }

    std::istringstream is(std::string(buf.begin(), it));
    long result;
    is >> result;
    return result;
}

}

#endif /* TH_GPS_TIME_H */
