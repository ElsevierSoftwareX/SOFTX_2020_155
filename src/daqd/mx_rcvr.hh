#ifndef MX_RCVR_HH
#define MX_RCVR_HH

#ifndef  USE_SYMMETRICOM
extern int controller_cycle;
#endif
extern struct rmIpcStr gmDaqIpc[DCU_COUNT];
extern void *directed_receive_buffer[DCU_COUNT];
unsigned int open_mx(void)
void receiver_mx(int)

