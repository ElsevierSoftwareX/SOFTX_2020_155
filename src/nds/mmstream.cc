
#define _LIGO_MMSTREAMBUF_CC
#include "mmstream.hh"
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

namespace gds
{

    bool
    map_file( const char*             filename,
              void*&                  addr,
              int&                    len,
              std::ios_base::openmode which )
    {
        // open mode
        int prot = 0;
        if ( which & std::ios_base::in )
            prot |= PROT_READ;
        if ( which & std::ios_base::out )
            prot |= PROT_WRITE;
        // open file
        int fd = ::open( filename,
                         ( which & std::ios_base::out ) ? O_RDWR : O_RDONLY );
        if ( fd == -1 )
        {
            return false;
        }
        // get its size
        struct stat info;
        int         ret = ::fstat( fd, &info );
        if ( ret )
        {
            return false;
        }
        // map file into memory
        void* data = ::mmap( 0, info.st_size, prot, MAP_SHARED, fd, 0 );
        // QFS requires the exec flag to be set, so try again
        if ( data == MAP_FAILED )
        {
            data =
                ::mmap( 0, info.st_size, prot | PROT_EXEC, MAP_SHARED, fd, 0 );
        }
        close( fd );
        if ( data == MAP_FAILED )
        {
            return false;
        }
        // set values and return
        addr = data;
        len = info.st_size;
        return true;
    }

    bool
    unmap_file( void* addr, int len )
    {
        return ( ::munmap( (char*)addr, len ) == 0 );
    }

} // namespace gds
