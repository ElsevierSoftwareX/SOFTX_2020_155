dnl Process this file with autoconf to produce a configure script.
AC_INIT(nds.hh)
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS(config.h)

dnl checks for programs
dnl ===================
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_RANLIB
AC_PATH_XTRA

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


dnl checks for library functions
dnl ============================


dnl AC_CHECK_FUNCS(strsep strpbrk memset)

dnl recommended by the autoconf doc, for HEADER_STDC
AC_CHECK_FUNCS(strchr memcpy fchdir)


dnl checks for system services
dnl ==========================

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
PKG_CHECK_MODULES(FRAMECPP, framecpp, have_framecpp="yes", have_framecpp="no")
if test "$have_framecpp" = "yes"; then
  dnl======================================================
  dnl  framecpp 2+ supplies the proper set of libs w/ pkg-config, framecpp 1 does not
  dnl======================================================
  LDASLIBS="$FRAMECPP_LIBS"
  LDASLIBFLAGS="$LDASLIBS"
  LDASFLAGS="$CPPFLAGS $FRAMECPP_CFLAGS"
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
      [ found_framecpp="no", AC_MSG_ERROR([No framecpp in default location])])
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
      [ found_framecpp="no", AC_MSG_ERROR([Could not find custom framecpp])])
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

dnl Enable configuration with environmental variables
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

AC_OUTPUT(Makefile)
