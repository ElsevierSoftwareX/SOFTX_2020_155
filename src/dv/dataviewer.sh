#!/bin/bash
mkdir /tmp/$$DC
DVPATH=/usr/bin
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
$DVPATH/dv -s ${LIGONDSIP} -a $$ -b $DVPATH $*
/bin/rm -f -r /tmp/$$DC
