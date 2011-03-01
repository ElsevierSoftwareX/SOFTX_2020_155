
// Maximum length of memory buffer tag
#define MBUF_NAME_LEN 32

struct mbuf_request_struct{
        size_t size;
        char name[MBUF_NAME_LEN+1];
};

// Get memory buffer info
#define IOCTL_MBUF_INFO _IO(0,0)

// Allocate new or attach to an existing memory buffer with the tag
#define IOCTL_MBUF_ALLOCATE _IOW(0,1,struct mbuf_request_struct)

// Kill a buffer 
#define IOCTL_MBUF_DEALLOCATE _IOW(0,2,struct mbuf_request_struct)


