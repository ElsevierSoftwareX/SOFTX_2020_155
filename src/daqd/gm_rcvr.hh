#ifndef GM_RCVR_HH
#define GM_RCVR_HH

extern int controller_cycle;
extern struct rmIpcStr gmDaqIpc[DCU_COUNT];
extern void *directed_receive_buffer[DCU_COUNT];
int gm_setup(void);
void gm_recv(void);
void gm_cleanup(void);

#endif
