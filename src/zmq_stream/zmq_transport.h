//
// Created by jonathan.hanks on 3/26/18.
//

#ifndef DAQD_TRUNK_ZMQ_TRANSPORT_H
#define DAQD_TRUNK_ZMQ_TRANSPORT_H

#include "daq_core.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int zmq_send_daq_multi_dcu_t( daq_multi_dcu_data_t* src,
                                     void*                 socket,
                                     int*                  xmit_size );
extern int zmq_recv_daq_multi_dcu_t( daq_multi_dcu_data_t* dest, void* socket );
// extern int zmq_recv_daq_multi_dcu_t_into_buffer(daq_multi_dcu_data_t
// *dest_buffers, void *socket, int buffer_count);
extern int
zmq_recv_daq_multi_dcu_t_into_buffer( daq_multi_dcu_data_t* dest_buffers,
                                      pthread_spinlock_t*   locks,
                                      void*                 socket,
                                      int                   buffer_count,
                                      unsigned int*         dest_gps_sec,
                                      int*                  dest_cycle );

#ifdef __cplusplus
};
#endif

#endif // DAQD_TRUNK_ZMQ_TRANSPORT_H
