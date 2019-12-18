#dnl Process this file with autoconf to produce a configure script.
AC_INIT(NEWS)

# find mbuf symvers file
foundmbuf=
mbufsym=
mbufdkms=
AC_MSG_CHECKING([for mbuf Module.symvers])
if test -x /sbin/dkms
then
  mbufstat=`/sbin/dkms status -m mbuf`
  if test x"${mbufstat}"  = "x"
  then
    mbufdkms="no"
  else
    mbufdkms="yes"
    mmodvers=`echo ${mbufstat} | awk '{print $2}' | sed 's/.$//'`
    mkvers=`echo ${mbufstat} | awk '{print $3}' | sed 's/.$//'`
    march=`echo ${mbufstat} | awk '{print $4}' | sed 's/.$//'`
    mtestpath="/var/lib/dkms/mbuf/${mmodvers}/${mkvers}/${march}/Module.symvers"
    if test -f $mtestpath
    then
      mbufsym=$mtestpath
      foundmbuf="yes"
    fi
  fi
else
  mbufdkms="no"
fi
if test x$foundmbuf = "x"
then
  mtestpath="/var/cache/mbuf/Module.symvers"
  if test -f $mtestpath
  then
    mbufsym=$mtestpath
    foundmbuf="yes"
  else
    foundmbuf="no"
  fi
fi
if test x$foundmbuf = "xyes"
then
  AC_MSG_RESULT([${mbufsym}])  
else
  AC_MSG_RESULT([no])
fi
# look for gpstime symvers
foundgpstime=
gpstimesym=
gpstimedkms=
AC_MSG_CHECKING([for gpstime Module.symvers])
if test -x /sbin/dkms
then
  gbufstat=`/sbin/dkms status -m gpstime `
  if test x"${gbufstat}"  = "x"
  then
    gpstimedkms="no"
  else
    gpstimedkms="yes"
    gmodvers=`echo ${gbufstat} | awk  '{print $2}' | sed 's/.$//'`
    gkvers=`echo ${gbufstat} | awk  '{print $3}' | sed 's/.$//'`
    garch=`echo ${gbufstat} | awk  '{print $4}' | sed 's/.$//'`
    gtestpath="/var/lib/dkms/gpstime/$gmodvers/$gkvers/$garch/Module.symvers"
    if test -f $gtestpath
    then
      gpstimesym=$gtestpath
      foundgpstime="yes"
    fi
  fi
else
  gpstimedkms="no"
fi
if test x$foundgpstime = "x"
  then
  gtestpath="/var/cache/gpstime/Module.symvers"
  if test -f $gtestpath
  then
    gpstimesym=$gtestpath
    foundgpstime="yes"
  else
    foundgpstime="no"
  fi
fi
if test x$foundgpstime = "xyes"
then
  AC_MSG_RESULT([${gpstimesym}])  
else
  AC_MSG_RESULT([no])
fi
# set paths if they are found
MBUFSYMBOLPATH=
if test "$foundmbuf" = "yes"; then
    MBUFSYMBOLPATH="$mbufsym"
fi
GPSSYMBOLPATH=
if test "$foundgpstime" = "yes"; then
    GPSSYMBOLPATH="$gpstimesym"
fi

AC_SUBST([MBUFSYMBOLPATH])
AC_SUBST([GPSSYMBOLPATH])

AC_OUTPUT( Makefile )
AC_OUTPUT( src/epics/util/Makefile )
AC_OUTPUT( doc/doxygen.cfg )
AC_OUTPUT( doc/doxScript )
