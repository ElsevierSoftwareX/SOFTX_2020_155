# Setup aLIGO RCG
#  -- tries to use userapps setup
# check that site and ifo are defined
if ! $?site  then
  echo "RCG setup failed - environment variable <site> is not defined"
else if ! $?ifo then
  echo "RCG setup failed - environment variable <ifo> is not defined"
else
#  check location
 if ( -e /opt/rtcds/rtscore/release ) then
  set rcgDir=/opt/rtcds/rtscore/release
 else if ( -e /opt/rtcds/${site}/${ifo}/core/release ) then
  set rcgDir=/opt/rtcds/${site}/${ifo}/core/release
 else if ( -e /opt/rtcds/${site}/${ifo}/core/trunk ) then
  set rcgDir=/opt/rtcds/${site}/${ifo}/core/trunk
 else
  echo "RCG setup failed - not in standard location"
  set rcgDir="/opt/rtcds/${site}/${ifo}/core/release"
 endif
 setenv RCG_DIR ${rcgDir}

# if USERAPPS set up not done, find it and use it
 if ! $?USERAPPS_DIR then
  echo "USERAPPS setup not done - try to find it"
  if ( -e /opt/rtcds/userapps/release ) then
    set userApps=/opt/rtcds/userapps/release
  else if ( -e /opt/rtcds/${site}/${ifo}/userapps/release ) then
    set userApps=/opt/rtcds/${site}/${ifo}/userapps/release
  else if ( -e /opt/rtcds/${site}/${ifo}/userapps/trunk ) then
    set userApps=/opt/rtcds/${site}/${ifo}/userapps/trunk
  else
    echo "USERAPPS setup failed - not in standard location"
    set userApps="/opt/rtcds/${site}/${ifo}/userapps/release"
  endif
#
  if ( -e ${userApps}/etc/userapps-user-env.csh ) then
    source ${userApps}/etc/userapps-user-env.csh
  else
    echo "No USERAPP setup available - attempt manually"
#
    setenv USERAPPS_DIR ${userApps}
    cdsModel=${userApps}/cds/${ifo}/models:${userApps}/cds/common/models
    hpiModel=${userApps}/hpi/${ifo}/models:${userApps}/hpi/common/models
    iooModel=${userApps}/ioo/${ifo}/models:${userApps}/ioo/common/models
    iscModel=${userApps}/isc/${ifo}/models:${userApps}/isc/common/models
    isiModel=${userApps}/isi/${ifo}/models:${userApps}/isi/common/models
    pslModel=${userApps}/psl/${ifo}/models:${userApps}/psl/common/models
    susModel=${userApps}/sus/${ifo}/models:${userApps}/sus/common/models
    tcsModel=${userApps}/tcs/${ifo}/models:${userApps}/tcs/common/models
    tstModel=${userApps}/cds/test/models
    setenv USERAPPS_LIB_PATH ${cdsModel}:${hpiModel}:${iooModel}:${iscModel}:${isiModel}:${pslModel}:${susModel}:${tcsModel}:${tstModel}
#
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
    setenv USERAPPS_MEDM_PATH ${cdsMedm}:$hpiMedm:$iooMedm:$iscMedm:$isiMedm:$pslMedm:$susMedm:$testMedm
#
    cdsScr=${userApps}/cds/common/scripts
    hpiScr=${userApps}/hpi/common/scripts
    iooScr=${userApps}/ioo/common/scripts
    iscScr=${userApps}/isc/common/scripts
    isiScr=${userApps}/isi/common/scripts
    pslScr=${userApps}/psl/common/scripts
    susScr=${userApps}/sus/common/scripts
    testScr=${userApps}/cds/test/scripts
    setenv USERAPPS_SCRIPTS_PATH ${cdsScr}:$hpiScr:$iooScr:$iscScr:$isiScr:$pslScr:$susScr:$testScr
  endif
 endif
#
# now that userapps is done, do the RCG specific stuff
#
#  alias to userapps install
 alias userCode "pushd ${USERAPPS_DIR}"
 alias usercode "pushd ${USERAPPS_DIR}"

# alias for RCG configuration script
 alias rcgConfig "${RCG_DIR}/configure"

# alias to get to RCG install
 alias rcgCode "pushd ${RCG_DIR}"
 alias rcgcode "pushd ${RCG_DIR}"

#  define RCG_LIB_PATH for builds
 simDir=${RCG_PATH}/src/epics/simLink
 setenv RCG_LIB_PATH ${USERAPPS_LIB_PATH}:${simDir}:${simDir}/lib

# define variables for post-build scripts
 setenv CDS_MEDM_PATH ${USERAPPS_MEDM_PATH}
 setenv CDS_SCRIPTS_PATH ${USERAPPS_SCRIPTS_PATH}

#
# ** add in RCG Simlink library
 rcgLib=${simDir}/lib
 echo ${MATLABPATH} | egrep -i "${rcgLib}" >&/dev/null
 if ($status != 0) then
   setenv MATLABPATH ${rcgLib}:${MATLABPATH}
 endif
#
# ** add in RCG Simlink
 echo ${MATLABPATH} | egrep -i "${simDir}" >&/dev/null
 if ($status != 0) then
   setenv MATLABPATH ${simDir}:${MATLABPATH}
 endif
#
# ** add in Userapps common library
 echo ${MATLABPATH} | egrep -i "${USERAPPS_LIB_PATH}" >&/dev/null
 if ($status != 0) then
   setenv MATLABPATH ${USERAPPS_LIB_PATH}:${MATLABPATH}
 endif
#
#
endif
