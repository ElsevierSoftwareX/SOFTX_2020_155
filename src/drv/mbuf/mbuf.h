
// Maximum length of memory buffer tag
#define MBUF_NAME_LEN 32

// Get memory buffer info
#define IOCTL_MBUF_INFO 0

// Allocate new or attach to an existing memory buffer with the tag
#define IOCTL_MBUF_ALLOCATE 1

// Kill a buffer 
#define IOCTL_MBUF_DEALLOCATE 2

struct mbuf_request_struct{
        size_t size;
        char name[MBUF_NAME_LEN+1];
};

