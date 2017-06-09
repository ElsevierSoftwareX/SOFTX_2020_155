#ifndef CONFIG_H
#define CONFIG_H

/* Define this if you want runtime configuration using .zlibrc */
#cmakedefine RUNTIME_CONF

/* Define this if you want configuration using environmental variables */
#cmakedefine ENV_CONF

/* Define if DAQD to run as daemon */
#cmakedefine DAEMONIC

/* Define if using FrameCPP */
#cmakedefine LDAS_FRAMECPP

/* Define if broadcast should be disabled */
#cmakedefine NO_BROADCAST

#endif
