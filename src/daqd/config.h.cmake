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

#cmakedefine FLEX_USES_IOSTREAM_REF

/* Generic daqd with shmem interface */

#define DATA_CONCENTRATOR 1
/* Using Symmetricom GPS card */
#define USE_SYMMETRICOM 1
/* run an ecdu */
#define EPICS_EDCU 1
/* Interface with the GDS Testpoint server */
#define GDS_TESTPOINTS 1


#endif /* CONFIG_H */
