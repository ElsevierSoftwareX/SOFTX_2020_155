#include <stdio.h>
#include <string.h>

#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <unistd.h>

#include "drv/shmem.h"
#include "mbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

int shmem_format_name(char *dest, const char *src, size_t n)
{
    char *cur = NULL;

    static const char *prefix = "/rtl_mem_";
    size_t dest_len = 0;
    int tmp = 0;

    if (!dest || !src || n < 1)
        return 0;
    if (snprintf(dest, n, "%s%s", prefix, src) > n)
        return 0;
    for (cur = dest; *cur; ++cur) {
        *cur = (char)tolower(*cur);
    }
    return 1;
}

volatile void* shmem_open_segment(const char *sys_name, size_t req_size)
{
    int fd = 0;
    int buffer_len = 0;
    size_t name_len = 0;
    volatile void *addr = NULL;

    if (!sys_name || req_size == 0)
        return NULL;
    name_len = strlen(sys_name);
    if (name_len == 0 || name_len > MBUF_NAME_LEN) {
        return NULL;
    }

    if ((fd = open ("/dev/mbuf", O_RDWR | O_SYNC)) < 0) {
        fprintf(stderr, "Couldn't open /dev/mbuf read/write\n");
        return 0;
    }
    struct mbuf_request_struct req;
    req.size = req_size;
    strcpy(req.name, sys_name);
    buffer_len = ioctl (fd, IOCTL_MBUF_ALLOCATE, &req);

    if (buffer_len < (int)req_size) {
        close(fd);
        return NULL;
    }

    addr = (volatile void *)mmap(0, req_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        close(fd);
        return NULL;
    }
    /* printf(" %s mmapped address is 0x%lx\n", sys,(long)addr); */
    return addr;
}

#ifdef __cplusplus
}
#endif