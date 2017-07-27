//
// Created by jonathan.hanks on 7/27/17.
//

#include <time.h>
#include <unistd.h>

extern "C" {

void get_time(long &gps, long &gps_n) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    gps = static_cast<long>(ts.tv_sec);
    gps_n = static_cast<long>(ts.tv_nsec);
}

void wait_for(long gps, long gps_n) {
    if (gps == 0) {
        long dummy;
        get_time(gps, dummy);
    }
    while (true) {
        long cur_gps, cur_gps_n;
        get_time(cur_gps, cur_gps_n);
        if (gps < cur_gps) break;
        if (gps == cur_gps && gps_n <= cur_gps_n) break;
        usleep(1);
    }
}

}