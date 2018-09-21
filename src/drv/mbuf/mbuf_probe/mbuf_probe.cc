//
// Created by jonathan.hanks on 1/19/18.
//
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>

#include <stdio.h>
#include <string.h>

#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <unistd.h>
#include <sstream>

#include "drv/shmem.h"
#include "mbuf.h"



void shmem_inc_segment_count(const char *sys_name)
{
    int fd = -1;
    size_t name_len = 0;

    if (!sys_name)
        return;
    name_len = strlen(sys_name);
    if (name_len == 0 || name_len > MBUF_NAME_LEN) {
        return;
    }

    if ((fd = open ("/dev/mbuf", O_RDWR | O_SYNC)) < 0) {
        fprintf(stderr, "Couldn't open /dev/mbuf read/write\n");
        return;
    }
    struct mbuf_request_struct req;
    req.size = 1;
    strcpy(req.name, sys_name);
    ioctl (fd, IOCTL_MBUF_ALLOCATE, &req);
    ioctl (fd, IOCTL_MBUF_ALLOCATE, &req);
    close(fd);
}

void shmem_dec_segment_count(const char *sys_name)
{
    int fd = -1;
    size_t name_len = 0;

    if (!sys_name)
        return;
    name_len = strlen(sys_name);
    if (name_len == 0 || name_len > MBUF_NAME_LEN) {
        return;
    }

    if ((fd = open ("/dev/mbuf", O_RDWR | O_SYNC)) < 0) {
        fprintf(stderr, "Couldn't open /dev/mbuf read/write\n");
        return;
    }
    struct mbuf_request_struct req;
    req.size = 1;
    strcpy(req.name, sys_name);
    ioctl (fd, IOCTL_MBUF_DEALLOCATE, &req);
    close(fd);
}

class safe_file {
    FILE *_f;
    safe_file();
    safe_file(const safe_file& other);
    safe_file& operator=(const safe_file& other);
public:
    safe_file(FILE *f): _f(f) {};
    ~safe_file() {
        if (_f) {
            fclose(_f);
            _f = NULL;
        }
    }
    FILE *get() const { return _f; }
};

int copy_shmem_buffer(const volatile void* buffer, const std::string& output_fname, size_t req_size)
{
    std::vector<char> dest_buffer(req_size);

    safe_file f(fopen(output_fname.c_str(), "wb"));
    if (!f.get())
        return 1;
    const volatile char *src = reinterpret_cast<const volatile char*>(buffer);
    memcpy(dest_buffer.data(), const_cast<const char*>(src), req_size);
    fwrite(dest_buffer.data(), 1, dest_buffer.size(), f.get());
    return 0;
}

void list_shmem_segments()
{
    std::vector<char> buf(100);
    std::ifstream f("/sys/kernel/mbuf/status");
    while (f.read(buf.data(), buf.size())) {
        std::cout.write(buf.data(), buf.size());
    }
    if (f.gcount() > 0) {
        std::cout.write(buf.data(), f.gcount());
    }
}

void usage(const char *prog)
{
    std::cout << "Usage:\n" << prog << " [options]" << std::endl;
}

int main(int argc, char* argv[])
{
    int ch = 0;
    size_t req_size = 1;
    std::string output_fname = "probe_out.bin";

    while  ((ch = getopt(argc, argv, "hs:o:")) != EOF) {
        switch (ch) {
            case 's':
                {
                    std::istringstream os(optarg);
                    os >> req_size;
                }
                break;
            case 'o':
                output_fname = optarg;
                break;
            default:
                usage(argv[0]);
                return 1;
        }
    }
    if (optind + 1 > argc) {
        usage(argv[0]);
        return 1;
    }

    std::string action = argv[optind];
    if (action == "list") {
        list_shmem_segments();
        return 0;
    }
    if (optind + 2 > argc) {
        usage(argv[0]);
        return 1;
    }

    std::string buffer_name = argv[optind + 1];
    volatile void *buffer = shmem_open_segment(buffer_name.c_str(), req_size);
    if (!buffer) {
        std::cerr << "Unable to create shmem buffer " << buffer_name << std::endl;
        return 1;
    }
    if (action == "create") {
        shmem_inc_segment_count(buffer_name.c_str());
        return 0;
    } else if (action == "copy") {
        return copy_shmem_buffer(buffer, output_fname, req_size);
    } else if (action == "delete") {
        shmem_dec_segment_count(buffer_name.c_str());
        return 0;
    } else {
        usage(argv[0]);
        return 1;
    }
    return 0;
}