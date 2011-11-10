# Setup aLIGO RCG
#export RCG_PATH=/opt/rtapps/rcg-2.4
export RCG_PATH=/opt/rtcds/llo/l1/core/trunk
#
# this should be done somewhere under userapps
userApps=/opt/rtcds/${site}/${ifo}/userapps/release
cdsModel=${userApps}/cds/${ifo}/models:${userApps}/cds/common/models
hpiModel=${userApps}/hpi/${ifo}/models:${userApps}/hpi/common/models
iooModel=${userApps}/ioo/${ifo}/models:${userApps}/ioo/common/models
iscModel=${userApps}/isc/${ifo}/models:${userApps}/isc/common/models
isiModel=${userApps}/isi/${ifo}/models:${userApps}/isi/common/models
pslModel=${userApps}/psl/${ifo}/models:${userApps}/psl/common/models
susModel=${userApps}/sus/${ifo}/models:${userApps}/sus/common/models
tcsModel=${userApps}/tcs/${ifo}/models:${userApps}/tcs/common/models
tstModel=${userApps}/cds/test/models/${site}
export USERAPPS_LIB_PATH=${cdsModel}:${tstModel}:${hpiModel}:${iooModel}:${iscModel}:${isiModel}:${pslModel}:${susModel}:${tcsModel}
#
simDir=${RCG_PATH}/src/epics/simLink
export RCG_LIB_PATH=${USERAPPS_LIB_PATH}:${simDir}:${simDir}/lib
#
addmatlab()
{
#  addmatlab - add first to MATLABPATH if not already there
    thismat=$1
    echo ${MATLABPATH} | egrep -i ${thismat}: >&/dev/null
    if [ "$?" -ne 0 ]; then
        export MATLABPATH=${thismat}:${MATLABPATH}
    fi
}
addmatlab ${simDir}/lib
addmatlab ${simDir}
#
# This should also be somewhere under userapps
export CDS_SRC=${userApps}/cds/common/src
export CDS_IFO_SRC=${userApps}/cds/${ifo}/src
export HPI_SRC=${userApps}/hpi/common/src
export HPI_IFO_SRC=${userApps}/hpi/${ifo}/src
export IOO_SRC=${userApps}/ioo/common/src
export IOO_IFO_SRC=${userApps}/ioo/${ifo}/src
export ISC_SRC=${userApps}/isc/common/src
export ISC_IFO_SRC=${userApps}/isc/${ifo}/src
export ISI_SRC=${userApps}/isi/common/src
export ISI_IFO_SRC=${userApps}/isi/${ifo}/src
export PSL_SRC=${userApps}/psl/common/src
export PSL_IFO_SRC=${userApps}/psl/${ifo}/src
export SUS_SRC=${userApps}/sus/common/src
export SUS_IFO_SRC=${userApps}/sus/${ifo}/src
export TEST_SRC=${userApps}/cds/test/src
#
cdsMedm=${userApps}/cds/common/medm
hpiMedm=${userApps}/hpi/common/medm
iooMedm=${userApps}/ioo/common/medm
iscMedm=${userApps}/isc/common/medm
isiMedm=${userApps}/isi/common/medm
pslMedm=${userApps}/psl/common/medm
susMedm=${userApps}/sus/common/medm
testMedm=${userApps}/cds/test/medm
export CDS_MEDM_PATH=${cdsMedm}:$hpiMedm:$iooMedm:$iscMedm:$isiMedm:$pslMedm:$susMedm:$testMedm
#
cdsScr=${userApps}/cds/common/scripts
hpiScr=${userApps}/hpi/common/scripts
iooScr=${userApps}/ioo/common/scripts
iscScr=${userApps}/isc/common/scripts
isiScr=${userApps}/isi/common/scripts
pslScr=${userApps}/psl/common/scripts
susScr=${userApps}/sus/common/scripts
testScr=${userApps}/cds/test/scripts
export CDS_SCRIPTS_PATH=${cdsScr}:$hpiScr:$iooScr:$iscScr:$isiScr:$pslScr:$susScr:$testScr
#

