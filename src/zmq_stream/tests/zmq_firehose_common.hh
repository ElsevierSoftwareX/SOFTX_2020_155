//
// Created by jonathan.hanks on 7/27/17.
//

#ifndef DAQD_TRUNK_ZMQ_FIREHOSE_COMMON_H
#define DAQD_TRUNK_ZMQ_FIREHOSE_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

extern void get_time(long &gps, long &gps_n);
extern void wait_for(long gps, long gps_n);

#ifdef __cplusplus
}
#endif

#endif //DAQD_TRUNK_ZMQ_FIREHOSE_COMMON_H
