#!/bin/bash

# Sources are checked out on x1boot machine into these directories
CHECKOUT_ROOT=/opt/rtcds/jenkins/x1boot/home/workspace/svn_checkouts
USERAPPS_ROOT=$CHECKOUT_ROOT/userapps_trunk
VERSION_ROOT=/opt/rtcds/jenkins/x1boot/home/workspace/svn2.8-co
export RCG_DIR=$VERSION_ROOT/rcg_branch2.8

# Locations of the model files
export RCG_LIB_PATH=$USERAPPS_ROOT/lsc/h1/models:$USERAPPS_ROOT/lsc/common/models:$USERAPPS_ROOT/omc/h1/models:$USERAPPS_ROOT/omc/common/models:$USERAPPS_ROOT/asc/h1/models:$USERAPPS_ROOT/sys/h1/models:$USERAPPS_ROOT/sys/common/models:$USERAPPS_ROOT/pem/h1/models:$USERAPPS_ROOT/asc/common/models:$USERAPPS_ROOT/cds/h1/models:$USERAPPS_ROOT/cds/common/models:$USERAPPS_ROOT/cds/l1/models:$USERAPPS_ROOT/aos/h1/models:$USERAPPS_ROOT/aos/common/models:$USERAPPS_ROOT/hpi/h1/models:$USERAPPS_ROOT/hpi/common/models:$USERAPPS_ROOT/ioo/h1/models:$USERAPPS_ROOT/ioo/common/models:$USERAPPS_ROOT/isc/h1/models:$USERAPPS_ROOT/isc/common/models:$USERAPPS_ROOT/isi/h1/models:$USERAPPS_ROOT/isi/common/models:$USERAPPS_ROOT/pem/h1/models:$USERAPPS_ROOT/pem/common/models:$USERAPPS_ROOT/psl/h1/models:$USERAPPS_ROOT/psl/l1/models:$USERAPPS_ROOT/psl/common/models:$USERAPPS_ROOT/sus/h1/models:$USERAPPS_ROOT/sus/l1/models:$USERAPPS_ROOT/sus/common/models:$USERAPPS_ROOT/tcs/h1/models:$USERAPPS_ROOT/tcs/common/models:$USERAPPS_ROOT/asc/l1/models:$USERAPPS_ROOT/lsc/l1/models:$USERAPPS_ROOT/hpi/l1/models:$USERAPPS_ROOT/isi/l1/models:$USERAPPS_ROOT/omc/l1/models:$USERAPPS_ROOT/pem/l1/models:$USERAPPS_ROOT/sys/l1/models:$USERAPPS_ROOT/aos/l1/models:$USERAPPS_ROOT/isc/l1/models:$USERAPPS_ROOT/tcs/l1/models:$USERAPPS_ROOT/ioo/l1/models:$USERAPPS_ROOT/cds/test/models:$RCG_DIR/src/epics/simLink:$RCG_DIR/src/epics/simLink/lib

# Source code locations for the model files
export AOS_IFO_SRC=$USERAPPS_ROOT/aos/h1/src
export AOS_SRC=$USERAPPS_ROOT/aos/common/src
export CDS_IFO_SRC=$USERAPPS_ROOT/cds/h1/src
export CDS_SRC=$USERAPPS_ROOT/cds/common/src
export CDS_TEST_SRC=$USERAPPS_ROOT/cds/test/src
export HPI_IFO_SRC=$USERAPPS_ROOT/hpi/h1/src
export HPI_SRC=$USERAPPS_ROOT/hpi/common/src
export IOO_IFO_SRC=$USERAPPS_ROOT/ioo/h1/src
export IOO_SRC=$USERAPPS_ROOT/ioo/common/src
export ISC_IFO_SRC=$USERAPPS_ROOT/isc/h1/src
export ISC_SRC=$USERAPPS_ROOT/isc/common/src
export ISI_IFO_SRC=$USERAPPS_ROOT/isi/h1/src
export ISI_SRC=$USERAPPS_ROOT/isi/common/src
export PEM_IFO_SRC=$USERAPPS_ROOT/pem/h1/src
export PEM_SRC=$USERAPPS_ROOT/pem/common/src
export PSL_IFO_SRC=$USERAPPS_ROOT/psl/h1/src
export PSL_SRC=$USERAPPS_ROOT/psl/common/src
export SUS_IFO_SRC=$USERAPPS_ROOT/sus/h1/src
export SUS_SRC=$USERAPPS_ROOT/sus/common/src
export SYS_SRC=$USERAPPS_ROOT/sys/common/src


export IFO=H1
export ifo=h1
export SITE=LHO
export site=lho

export CDS_MEDM_PATH=$USERAPPS_ROOT/cds/common/medm:$USERAPPS_ROOT/aos/common/medm:$USERAPPS_ROOT/hpi/common/medm:$USERAPPS_ROOT/ioo/common/medm:$USERAPPS_ROOT/isc/common/medm:$USERAPPS_ROOT/isi/common/medm:$USERAPPS_ROOT/pem/common/medm:$USERAPPS_ROOT/psl/common/medm:$USERAPPS_ROOT/sus/common/medm:$USERAPPS_ROOT/cds/test/medm

export CDS_SCRIPTS_PATH=$USERAPPS_ROOT/cds/common/scripts:$USERAPPS_ROOT/aos/common/scripts:$USERAPPS_ROOT/hpi/common/scripts:$USERAPPS_ROOT/ioo/common/scripts:$USERAPPS_ROOT/isc/common/scripts:$USERAPPS_ROOT/isi/common/scripts:$USERAPPS_ROOT/pem/common/scripts:$USERAPPS_ROOT/psl/common/scripts:$USERAPPS_ROOT/sus/common/scripts:$USERAPPS_ROOT/cds/test/scripts

export PYEPICS_LIBCA=/opt/rtapps/epics-3.14.12.2_long/base-3.14.12.2/lib/linux-x86_64/libca.so

export PATH=:/opt/rtapps/linux-x86_64/utils/bin:/opt/rtapps/gds-2.15.2/bin:/opt/rtapps/libmetaio-8.2/linux-x86_64/bin:/opt/rtapps/libframe-8.11/linux-x86_64/bin:/opt/rtapps/framecpp-1.18.2/linux-x86_64/bin:/opt/rtapps/fftw-3.2.2/linux-x86_64/bin:/opt/rtapps/dv:/opt/rtapps/epics-3.14.10_long/extensions/bin/linux-x86_64:/opt/rtapps/epics-3.14.10_long/modules/sncseq/bin/linux-x86_64:/opt/rtapps/epics-3.14.10_long/base/bin/linux-x86_64:/usr/bin:/bin:/usr/sbin:/sbin:/opt/rtcds/lho/h1/scripts:/opt/rtcds/userapps/release/cds/common/scripts:/opt/rtapps/jdk/bin
export EPICS_DB_INCLUDE_PATH=/opt/rtapps/epics/base/dbd
export EPICS_HOST_ARCH=linux-x86_64
export EPICS_BASE=/opt/rtapps/epics-3.14.12.2_long/base-3.14.12.2

export PYEPICS_LOCATION=/opt/rtapps/epics/pyext/pyepics
export PYTHONPATH=$PYEPICS_LOCATION/lib/python2.6/site-packages:/opt/rtapps/nds2-client-0.10.4_big/lib64/python2.7/site-packages:/usr/lib/portage/pym:/opt/rtcds/userapps/trunk/cds/test/scripts:/opt/rtcds/userapps/trunk/cds/test/scripts/python
echo $PYTHONPATH



