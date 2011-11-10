# Setup aLIGO RCG
#setenv RCG_PATH /opt/rtapps/rcg-2.4
setenv RCG_PATH /opt/rtcds/llo/l1/core/trunk
#
# this should be done somewhere under userapps
userApps=/opt/rtcds/${site}/${ifo}/userapps/release
cdsModel=${userApps}/cds/test/models/${site}:${userApps}/cds/${ifo}/models
hpiModel=${userApps}/hpi/${ifo}/models:${userApps}/hpi/common/models
iooModel=${userApps}/ioo/${ifo}/models:${userApps}/ioo/common/models
iscModel=${userApps}/isc/${ifo}/models:${userApps}/isc/common/models
isiModel=${userApps}/isi/${ifo}/models:${userApps}/isi/common/models
pslModel=${userApps}/psl/${ifo}/models:${userApps}/psl/common/models
susModel=${userApps}/sus/${ifo}/models:${userApps}/sus/common/models
tcsModel=${userApps}/tcs/${ifo}/models:${userApps}/tcs/common/models
setenv USERAPPS_LIB_PATH ${cdsModel}:${hpiModel}:${iooModel}:${iscModel}:${isiModel}:${pslModel}:${susModel}:${tcsModel}
#
simDir=${RCG_PATH}/src/epics/simLink
setenv RCG_LIB_PATH ${USERAPPS_LIB_PATH}:${simDir}:${simDir}/lib
#
# ** add in RCG Simlink library
echo ${MATLABPATH} | egrep -i "${simDir}" >&/dev/null
if ($status != 0) then
   setenv MATLABPATH ${simDir}:/${MATLABPATH}
endif
#
# ** add in RCG library
rcgLib=${simDir}/lib
echo ${MATLABPATH} | egrep -i "${rcgLib}" >&/dev/null
if ($status != 0) then
   setenv MATLABPATH ${rcgLib}:/${MATLABPATH}
endif
#
# This should also be somewhere under userapps
setenv CDS_SRC ${userApps}/cds/common/src
setenv CDS_IFO_SRC ${userApps}/cds/${ifo}/src
setenv HPI_SRC ${userApps}/hpi/common/src
setenv HPI_IFO_SRC ${userApps}/hpi/${ifo}/src
setenv IOO_SRC ${userApps}/ioo/common/src
setenv IOO_IFO_SRC ${userApps}/ioo/${ifo}/src
setenv ISC_SRC ${userApps}/isc/common/src
setenv ISC_IFO_SRC ${userApps}/isc/${ifo}/src
setenv ISI_SRC ${userApps}/isi/common/src
setenv ISI_IFO_SRC ${userApps}/isi/${ifo}/src
setenv PSL_SRC ${userApps}/psl/common/src
setenv PSL_IFO_SRC ${userApps}/psl/${ifo}/src
setenv SUS_SRC ${userApps}/sus/common/src
setenv SUS_IFO_SRC ${userApps}/sus/${ifo}/src
setenv TEST_SRC ${userApps}/cds/test/src
#
cdsMedm=${userApps}/cds/common/medm
hpiMedm=${userApps}/hpi/common/medm
iooMedm=${userApps}/ioo/common/medm
iscMedm=${userApps}/isc/common/medm
isiMedm=${userApps}/isi/common/medm
pslMedm=${userApps}/psl/common/medm
susMedm=${userApps}/sus/common/medm
testMedm=${userApps}/cds/test/medm
setenv CDS_MEDM_PATH ${cdsMedm}:$hpiMedm:$iooMedm:$iscMedm:$isiMedm:$pslMedm:$susMedm:$testMedm
#
cdsScr=${userApps}/cds/common/scripts
hpiScr=${userApps}/hpi/common/scripts
iooScr=${userApps}/ioo/common/scripts
iscScr=${userApps}/isc/common/scripts
isiScr=${userApps}/isi/common/scripts
pslScr=${userApps}/psl/common/scripts
susScr=${userApps}/sus/common/scripts
testScr=${userApps}/cds/test/scripts
setenv CDS_SCRIPTS_PATH ${cdsScr}:$hpiScr:$iooScr:$iscScr:$isiScr:$pslScr:$susScr:$testScr

