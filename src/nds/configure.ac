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
	string.h sys/socket.h sys/types.h time.h unistd.h tnf/probe.h)
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
dnl   Check if framecpp is installed
dnl======================================================================
have_framecpp=
PKG_CHECK_MODULES(FRAMECPP, framecpp, have_framecpp="yes", have_framecpp="no")
if test "$have_framecpp" = "yes"; then
  LDASLIBS="$FRAMECPP_LIBS"
  LDASLIBFLAGS="$LDASLIBS"
  for specvsn in 8 7 6 4 3 cmn; do
    LDASLIBFLAGS="$LDASLIBFLAGS -lframecpp$specvsn"
  done
  LDASLIBFLAGS="$LDASLIBFLAGS -lgeneral"
  LDASFLAGS="$FRAMECPP_CFLAGS"
else
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
