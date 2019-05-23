//
// Created by jonathan.hanks on 10/20/17.
//

#ifndef TRANSFER_GPS_H
#define TRANSFER_GPS_H

#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

/**
 * Operations on GPS time.
 * Note for purposes of this testing, this does not return GPS time but system time.
 */
namespace GPS {

    struct gps_time
    {
        long sec;
        long nanosec;

        gps_time(): sec(0), nanosec(0) {}
        explicit gps_time(long s): sec(s), nanosec(0) {}
        gps_time(long s, long ns): sec(s), nanosec(ns) {}
        gps_time(const gps_time& other): sec(other.sec), nanosec(other.nanosec) {}

        gps_time operator-(const gps_time& other) const
        {

            gps_time result(sec - other.sec, nanosec - other.nanosec);
            while (result.nanosec < 0) {
                result.nanosec += 1000000000;
                --result.sec;
            }
            return result;
        }

        gps_time operator+(const gps_time& other) const
        {
            gps_time result(sec + other.sec, nanosec + other.nanosec);
            while (result.nanosec >= 1000000000) {
                result.nanosec -= 1000000000;
                ++result.sec;
            }
            return result;
        }

        bool operator==(const gps_time& other) const
        {
            return (sec == other.sec && nanosec == other.nanosec);
        }

        bool operator!=(const gps_time& other) const
        {
            return !(*this == other);
        }

        bool operator<(const gps_time& other) const
        {
            if (sec < other.sec) return true;
            if (sec > other.sec) return false;
            return (nanosec < other.nanosec);
        }
    };

    std::ostream& operator<<(std::ostream& os, const gps_time& gps)
    {
        os << gps.sec << ":" << gps.nanosec;
        return os;
    }


    class gps_clock
    {
    public:
        gps_time now() const
        {
            gps_time result;

            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            result.sec = ts.tv_sec;
            result.nanosec = ts.tv_nsec;
            return result;
        }
    };
}

#endif //TRANSFER_GPS_H
