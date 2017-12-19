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
if [ -z "${LIGONDSIP}" ]; then
    echo "Your environment is not setup.  Plese set LIGONDSIP"
    exit 1
fi
PATH=${PATH}:${GRACE_HOME}/bin
$DVPATH/dv -s ${LIGONDSIP} -a $$ -b $DVPATH $*
/bin/rm -f -r /tmp/$$DC
