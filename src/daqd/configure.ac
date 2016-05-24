dnl Process this file with autoconf to produce a configure script.
AC_INIT(daqd.hh)
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS(config.h)

dnl Checks for host
dnl ===================
AC_CANONICAL_HOST

dnl checks for programs
dnl ===================
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_RANLIB
AC_PATH_XTRA
AC_PATH_PROG([YACC],[bison],[bison],[$PATH])

dnl checks for libraries
dnl ====================
AC_CHECK_LIB(nsl, inet_addr)
AC_CHECK_LIB(socket, socket)
AC_CHECK_LIB(dl, dlopen)
AC_CHECK_LIB(intl, dgettext)
AC_CHECK_LIB(posix4, sem_init)

dnl checks for header files
dnl =======================

AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(FlexLexer.h arpa/inet.h assert.h ctype.h \
	errno.h fcntl.h fstream.h iostream.h limits.h math.h \
	netinet/in.h pthread.h signal.h stdio.h stdlib.h stream.h \
	string.h sys/socket.h sys/types.h time.h unistd.h)
AC_HEADER_TIME


dnl checks for typedefs
dnl ===================

AC_TYPE_SIZE_T
AC_TYPE_OFF_T

dnl checks for structures
dnl =====================

AC_STRUCT_TM

dnl checks for compiler characteristics
dnl ===================================

AC_C_CONST
AC_C_INLINE


dnl recommended by the autoconf doc, for HEADER_STDC
AC_CHECK_FUNCS(strchr memcpy fchdir)

dnl======================================================================
dnl   Check for specific versions of framecpp
dnl   This is used in the main framecpp section below
dnl======================================================================
have_framecpp2=
PKG_CHECK_MODULES([FRAMECPP2],[framecpp >= 2],[have_framecpp2="yes"],[have_framecpp2="no"])
have_framecpp2to233=
PKG_CHECK_MODULES([FRAMECPP2TO233],[framecpp <= 2.3.3 framecpp >= 2],[have_framecpp2to233="yes"],[have_framecpp2to233="no"])
have_framecpp25=
PKG_CHECK_MODULES([FRAMECPP25],[framecpp >= 2.5],[have_framecpp25="yes"],[have_framecpp25="no"])


dnl======================================================
dnl FrameCPP 2 to 2.3.3 do not include -stc=c++0x in the .pc file
dnl======================================================
AC_LANG_PUSH([C++])
have_cxx0x=
AX_CHECK_COMPILE_FLAG([-std=c++0x],[have_cxx0x="yes"],[have_cxx0x="no"])
have_cxx11=
AX_CHECK_COMPILE_FLAG([-std=c++11],[have_cxx11="yes"],[have_cxx11="no"])
AC_LANG_POP([C++])

if test "$have_framecpp2" = "yes" && test "$have_framecpp2to233" = "yes"; then
  if test "$have_cxx11" = "yes"; then
    CPPFLAGS="$CPPFLAGS -std=c++11"
  elif test "$have_cxx0x" = "yes"; then
    CPPFLAGS="$CPPFLAGS -std=c++0x"
  fi
fi

if test "$have_framecpp2" = "yes"; then
  CPPFLAGS="$CPPFLAGS -DDAQD_CPP11"
fi

dnl======================================================
dnl FrameCPP 2.5 and above do not have LDAS_VERSION, use FRAMECPP_VERSION
dnl======================================================

if test "$have_framecpp25" = "yes"; then
  CPPFLAGS="$CPPFLAGS -DUSE_FRAMECPP_VERSION"
else
  CPPFLAGS="$CPPFLAGS -DUSE_LDAS_VERSION"
fi

dnl======================================================================
dnl   Check if framecpp is installed
dnl======================================================================
have_framecpp=
PKG_CHECK_MODULES([FRAMECPP],[framecpp],[have_framecpp="yes"],[have_framecpp="no"])
if test "$have_framecpp" = "yes"; then
  dnl======================================================
  dnl  framecpp 2+ supplies the proper set of libs w/ pkg-config, framecpp 1 does not
  dnl======================================================
  LDASLIBS="$FRAMECPP_LIBS"
  LDASLIBFLAGS="$LDASLIBS"
  LDASFLAGS="$FRAMECPP_CFLAGS"
  if test "$have_framecpp2" = "no"; then
    LDASLIBS="$FRAMECPP_LIBS"
    LDASLIBFLAGS="$LDASLIBS"
    for specvsn in 8 7 6 4 3 cmn; do
      LDASLIBFLAGS="$LDASLIBFLAGS -lframecpp$specvsn"
    done
    LDASLIBFLAGS="$LDASLIBFLAGS -lgeneral"
  fi
else
  dnl framecpp cannot be found by pkg-config, use defaults for ldas-tools 1.9.x
  LDAS_DEF="/opt/rtapps/framecpp"
  AC_CHECK_FILE(${LDAS_DEF}/linux-x86_64/lib/libframecpp.so,
  [found_framecpp="yes"],
  [found_framecpp="no"])
  if test "$found_framecpp" = "yes"; then
    LDASLIBS="-L$LDAS_DEF/linux-x86_64/lib -lframecpp"
  else
      AC_CHECK_FILE(${LDAS_DEF}/lib/libframecpp.so,
      [ found_framecpp="yes"],
      [ found_framecpp="no"
        AC_MSG_ERROR([No framecpp in default location])])
      if test "$found_framecpp" = "yes"; then
         LDASLIBS="-L$LDAS_DEF/lib -lframecpp"
      fi
  fi
  if test "$found_framecpp" = "yes"; then
      LDASLIBFLAGS="$LDASLIBS"
      for specvsn in 8 7 6 4 3 cmn; do
         LDASLIBFLAGS="$LDASLIBFLAGS -lframecpp$specvsn"
      done
      LDASLIBFLAGS="$LDASLIBFLAGS -lgeneral"
      LDASFLAGS="-I$LDAS_DEF/include"
  else
      LDASLIBFLAGS=
      LDASFLAGS=
  fi
fi

dnl=====================================================================
dnl Change location of the FrameCPP
dnl=====================================================================
AC_ARG_WITH(framecpp,
[  --with-framecpp=DIRECTORY           Specify LDAS software directory ],
[ if test x$withval != x; then
     LDAS_DIR=$withval
  else
     LDAS_DIR="/opt/rtapps/framecpp"
  fi
  AC_CHECK_FILE($LDAS_DIR/linux-x86_64/lib/libframecpp.so,
  [ found_framecpp="yes", LDASLIBS="-L$LDAS_DIR/linux-x86_64/lib -lframecpp"],
  [ found_framecpp="no"])
  if test "$found_framecpp" = "yes"; then
      AC_CHECK_FILE($LDAS_DIR/lib/libframecpp.so,
      [ found_framecpp="yes", LDASLIBS="-L$LDAS_DIR/lib -lframecpp"],
      [ found_framecpp="no"
        AC_MSG_ERROR([Could not find custom framecpp])])
  fi
  if test "$found_framecpp" = "yes"; then
      LDASLIBFLAGS="$LDASLIBS"
      for specvsn in 8 7 6 4 3 cmn; do
         LDASLIBFLAGS="$LDASLIBFLAGS -lframecpp$specvsn"
      done
      LDASLIBFLAGS="$LDASLIBFLAGS -lgeneral"
      LDASFLAGS="-I$LDAS_DIR/include"
  else
      LDASLIBFLAGS=
      LDASFLAGS=
  fi ],
[])
AC_SUBST(LDASLIBFLAGS)
AC_SUBST(LDASFLAGS)

dnl======================================================================
dnl   Get EPICS HOST and OS architecure
dnl======================================================================
host_arch=
os_arch=
case $host in
    *solaris*)
        osarch=solaris 
	if test `uname -m` = "i86pc" ;
	then
	    host_arch=solaris-x86-gnu
	else
	    host_arch=solaris-sparc	
	fi
	;;
    *linux*)
        osarch=Linux
	if test `uname -m` = "x86_64" ;
	then
	    host_arch=linux-x86_64
	else
	    host_arch=linux-x86
	fi
	;;
    *darwin*)
        osarch=Darwin
	if test `uname -m` = "x86_64" ;
	then
	    host_arch=darwin-x86
	else
	    host_arch=darwin-x86
	fi
	;;
    *)
        osarch=Linux
        host_arch=linux-x86
esac

dnl======================================================================
dnl   Check if EPICS is installed
dnl======================================================================
pkg_epics=
have_epics=
dnl ========
dnl  strategy is to check for pkgconfig, then search for it
dnl ========
PKG_CHECK_MODULES(EPICSBASE, epics-base, pkg_epics="yes", pkg_epics="no")
if test "${pkg_epics}" = "yes"; then
    EPICSLIBS="$EPICSBASE_LIBS"
    EPICSFLAGS="$EPICSBASE_CFLAGS"
    have_epics="yes"
else
    EPICS_DEF="/opt/rtapps/epics/base"
    AC_CHECK_FILE(${EPICS_DEF}/lib/${host_arch}/libca.a, have_epics="yes", have_epics="no")
    if test "${have_epics}" = "yes" ; then
       EPICSLIBS="-L$EPICS_DEF/lib/$host_arch"
       EPICSFLAGS="-I$EPICS_DEF/include -I$EPICS_DEF/include/os/$osarch"
    else
      AC_MSG_ERROR([No EPICS Base in default location])
    fi
fi

dnl=====================================================================
dnl Change location of EPICS
dnl=====================================================================

found_epics=
AC_ARG_WITH(epics,
[  --with-epics=<dir> enable EDCU, IOC server, optional EPICS Base location],
[ if test x$withval != xno; then
    EPICS_OBJS=\$\{EPICS_OBJS\}
    EPICS_SRCS=\$\{EPICS_SRCS\}
    if test x$withval != xyes; then
      EPICS_TRY=$withval
      AC_CHECK_FILE(${EPICS_TRY}/lib/${host_arch}/libca.a, found_epics="yes", found_epics="no")
      if test $found_epics = "yes"; then
        have_epics = "yes"
        EPICSLIBS="L$EPICS_TRY/lib/$host_arch"
        EPICSFLAGS="-I$EPICS_TRY/include -I$EPICS_TRY/include/os/$osarch"
      else
        AC_MSG_ERROR([could not find EPICS base libraries])
      fi
    fi
    if test $have_epics = "yes"; then
       EPICSLIBFLAGS="$EPICSLIBS -lca -lcas -lasHost -lCom -lgdd -lm -lrecIoc -lsoftDevIoc"
       AC_DEFINE(EPICS_EDCU)
    else
       AC_MSG_ERROR([Do not have EPICS, Skipping EPICS EDUC, IOC in build])
    fi
  else
    EPICSLIBFLAGS=
    EPICSFLAGS= 
    EPICS_SRCS=
    EPICS_OBJS=
  fi
],
[
   EPICSLIBFLAGS=
   EPICSFLAGS= 
   EPICS_SRCS=
   EPICS_OBJS=
])
AC_SUBST(EPICSLIBFLAGS)
AC_SUBST(EPICSFLAGS)
AC_SUBST(EPICS_SRCS)
AC_SUBST(EPICS_OBJS)

dnl Check for configuration options
dnl ==========================

AC_ARG_WITH(broadcast,
[ --with-broadcast            Get data receiving a broadcast],
[ if test x$withval != xno; then
   AC_DEFINE([USE_BROADCAST],[],[Get data receiving a broadcast])
  fi
])

AC_ARG_WITH(concentrator,
[ --with-concentrator         Build data concentrator],
[ if test x$withval != xno; then
   AC_DEFINE(DATA_CONCENTRATOR)
  fi
])

AC_ARG_WITH(myrinet,
[  --with-myrinet             Get data from Myrinet],
[ if test x$withval != xno; then
   MYRINET_OBJS=\$\{MYRINET_OBJS\}
   MYRINET_SRCS=\$\{MYRINET_SRCS\}
   MYRINET_FLAGS=\$\{MYRINET_FLAGS\}
   MYRINET_LDFLAGS=\$\{MYRINET_LDFLAGS\}
   AC_DEFINE(USE_GM)
 fi
])
AC_SUBST(USE_MYRINET)
AC_SUBST(MYRINET_OBJS)
AC_SUBST(MYRINET_SRCS)
AC_SUBST(MYRINET_FLAGS)
AC_SUBST(MYRINET_LDFLAGS)

AC_ARG_WITH(omx,
[  --with-omx             Get data from Open MX],
[ if test x$withval != xno; then
   MX_OBJS=\$\{MX_OBJS\}
   MX_SRCS=\$\{MX_SRCS\}
   MX_FLAGS=\$\{OMX_FLAGS\}
   MX_LDFLAGS=\$\{OMX_LDFLAGS\}
   AC_DEFINE(USE_MX)
 fi
])

AC_ARG_WITH(mx,
[  --with-mx             Get data from MX],
[ if test x$withval != xno; then
   MX_OBJS=\$\{MX_OBJS\}
   MX_SRCS=\$\{MX_SRCS\}
   MX_FLAGS=\$\{MX_FLAGS\}
   MX_LDFLAGS=\$\{MX_LDFLAGS\}
   AC_DEFINE(USE_MX)
 fi
])
AC_SUBST(USE_MX)
AC_SUBST(MX_OBJS)
AC_SUBST(MX_SRCS)
AC_SUBST(MX_FLAGS)
AC_SUBST(MX_LDFLAGS)

AC_ARG_WITH(udp,
[  --with-udp             Get data using UDP receive],
[ if test x$withval != xno; then
   AC_DEFINE(USE_UDP)
 fi
])
AC_SUBST(USE_UDP)

AC_ARG_ENABLE(testpoints,
[  --enable-testpoints         Enable GDS testpoints (default)],
[if test x$enableval = xyes; then
  AC_DEFINE(GDS_TESTPOINTS)
fi],
[
  AC_DEFINE(GDS_TESTPOINTS)
])

AC_ARG_ENABLE(daemon,
[  --enable-daemon         Be a Daemon],
[if test x$enableval = xyes; then
  AC_DEFINE(DAEMONIC)
fi],)

DEVFLAGS=\$\{PERFORMANCEFLAGS\}

AC_ARG_ENABLE(debug,
[  --enable-debug          Use debugging compiler flags],
[if test x$enableval = xyes; then
  DEVFLAGS=\$\{DEBUGFLAGS\}
fi],)

AC_ARG_ENABLE(release,
[  --enable-release        Use production compiler flags],
[if test x$enableval = xyes; then
  DEVFLAGS=\$\{RELEASEFLAGS\}
fi],)
AC_SUBST(DEVFLAGS)

GDSOBJECTS=\$\{CDS_GDSOBJECTS\}
GDSFLAGS=\$\{CDS_GDSFLAGS\}
GDSLIBFLAGS=\$\{CDS_GDSLIBFLAGS\}
AC_SUBST(GDS_ARCHIVE)
AC_SUBST(GDSFLAGS)
AC_SUBST(GDSLIBFLAGS)
AC_SUBST(GDSOBJECTS)

dnl Use Symmetricom GPS card as the timing source
AC_ARG_ENABLE(symmetricom,
[  --enable-symmetricom		Use Symmetricom GPS card as timing source],
[if test x$enableval != xno; then
  AC_DEFINE(USE_SYMMETRICOM)
dnl  SYMMETRICOM_CFLAGS=-DUNIX
dnl  SYMMETRICOM_LDFLAGS=-lbcsdk
fi;
],)
AC_SUBST(USE_SYMMETRICOM)
AC_SUBST(SYMMETRICOM_CFLAGS)
AC_SUBST(SYMMETRICOM_LDFLAGS)

dnl Use Symmetricom GPS card as the timing source
AC_ARG_ENABLE(local-timing,
[  --enable-local-timing		Use localtime() as timing source],
[if test x$enableval != xno; then
  AC_DEFINE(USE_LOCAL_TIME)
fi;
],)
AC_SUBST(USE_LOCAL_TIME)

dnl Get the GPS and the cycle from the IOP
AC_ARG_ENABLE(iop,
[  --enable-iop		Get the GPS time and the timing from the IOP],
[if test x$enableval != xno; then
#  AC_DEFINE(USE_SYMMETRICOM)
  AC_DEFINE(USE_IOP)
  AC_DEFINE(USE_SYMMETRICOM)
  SYMMETRICOM_CFLAGS=
  SYMMETRICOM_LDFLAGS=
fi;
],)
AC_SUBST(USE_SYMMETRICOM)
AC_SUBST(USE_IOP)
AC_SUBST(SYMMETRICOM_CFLAGS)
AC_SUBST(SYMMETRICOM_LDFLAGS)

AC_SUBST(BROADCASTOBJECTS)

BROADCASTOBJECTS=\$\{BROADCAST_OBJECTS\}

AC_ARG_ENABLE(broadcast,
[  --enable-broadcast    . (enabled by default)],
[if test x$enableval = xno; then
  AC_DEFINE(NO_BROADCAST)
  BROADCASTOBJECTS=
fi;
],)
AC_SUBST(BROADCASTOBJECTS)

AC_ARG_ENABLE(mini-nds,
[  --enable-mini-nds   configure to build mini-NDS],
[AC_DEFINE(MINI_NDS)
],)

AC_OUTPUT( Makefile scope/Makefile )
