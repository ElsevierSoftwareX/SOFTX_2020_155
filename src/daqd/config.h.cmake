#ifndef CONFIG_H
#define CONFIG_H

#cmakedefine HAVE_FCHDIR

#cmakedefine HAVE_MEMCPY

#cmakedefine HAVE_STRCHR

#cmakedefine HAVE_ERRNO_H

#cmakedefine HAVE_FCNTL_H

#cmakedefine HAVE_STDIO_H

#cmakedefine HAVE_STDLIB_H

#cmakedefine HAVE_STRING_H

#cmakedefine HAVE_SYS_TYPES_H

#cmakedefine HAVE_UNISTD_H

#cmakedefine HAVE_REGEXP_H

#cmakedefine HAVE_REGEX_H

#cmakedefine HAVE_CSTDATOMIC

#cmakedefine HAVE_ATOMIC

/* FW build settings */
#ifdef DAQD_BUILD_FW

    /* do not broadcast data */
    #define NO_BROADCAST 1
    /* run an edcu */
    #define EPICS_EDCU 1
    /* receive broadcasts */
    #define USE_BROADCAST

#endif

#endif
