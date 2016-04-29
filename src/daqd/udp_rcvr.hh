#ifndef UDP_RCVR_HH
#define UDP_RCVR_HH

#ifndef  USE_SYMMETRICOM
extern int controller_cycle;
#endif
extern struct rmIpcStr gmDaqIpc[DCU_COUNT];
extern void *directed_receive_buffer[DCU_COUNT];
void receiver_udp(int);

#endif
