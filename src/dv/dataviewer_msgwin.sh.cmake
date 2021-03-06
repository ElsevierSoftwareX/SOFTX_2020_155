#!/bin/bash
mkdir /tmp/$$DC
DVPATH=@CMAKE_INSTALL_PREFIX@/bin
export DVPATH
if [ -d /usr/share/grace ]; then
    GRACE_HOME=/usr/share/grace
else
    GRACE_HOME=/grace
    LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${GRACE_HOME}/lib
    export LD_LIBRARY_PATH
fi
export GRACE_HOME
export LANG=C
PATH=${PATH}:${GRACE_HOME}/bin
if [ $DVPATH ]; then
xterm -fg black -bg lightyellow -T "Dataviewer Messages" -sb -geometry 100x8+3+3 -e $DVPATH/dv -s  -a 1366 -b
else
echo "** the variable DVPATH is undefined***"
fi
/bin/rm -f -r /tmp/$$DC