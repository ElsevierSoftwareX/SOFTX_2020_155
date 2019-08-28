#ifndef LIGO_DRIVER_SHMEM_H
#define LIGO_DRIVER_SHMEM_H

#ifdef __cplusplus
extern "C" {
#endif

extern volatile void *findSharedMemory(char *sys_name);
extern volatile void *findSharedMemorySize(char *sys_name, int size);

extern int shmem_format_name(char *dest, const char *src, size_t n);
extern volatile void *shmem_open_segment(const char *sys_name, size_t req_size);

#ifdef __cplusplus
}
#endif

#endif /* LIGO_DRIVER_SHMEM_H */