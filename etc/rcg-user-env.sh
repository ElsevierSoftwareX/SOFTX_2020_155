# Setup aLIGO RCG
#  -- tries to use userapps setup
# check that site and ifo are defined
if [ "${site}set" = "set" ]; then
  echo "RCG setup failed - environment variable <site> is not defined"
elif [ "${ifo}set" = "set" ] ; then
  echo "RCG setup failed - environment variable <ifo> is not defined"
else
#  check location
 if [ -d /opt/rtcds/rtscore/release ]; then
  rcgDir=/opt/rtcds/rtscore/release
 elif [ -d /opt/rtcds/${site}/${ifo}/core/release ]; then
  rcgDir=/opt/rtcds/${site}/${ifo}/core/release
 elif [ -d /opt/rtcds/${site}/${ifo}/core/trunk ]; then
  rcgDir=/opt/rtcds/${site}/${ifo}/core/trunk
 else
  echo "RCG setup failed - not in standard location"
  rcgDir=/opt/rtcds/${site}/${ifo}/core/release
 fi
 export RCG_DIR=${rcgDir}

# if USERAPPS setup not done, find it and use it
 if [ "${USERAPPS_DIR}set" = "set" ]; then
  echo "USERAPPS setup not done - try to find it"
  if [ -d /opt/rtcds/userapps/release ]; then
    userApps=/opt/rtcds/userapps/release
  elif [ -d /opt/rtcds/${site}/${ifo}/userapps/release ]; then
    userApps=/opt/rtcds/${site}/${ifo}/userapps/release
  elif [ -d /opt/rtcds/${site}/${ifo}/userapps/trunk ]; then
    userApps=/opt/rtcds/${site}/${ifo}/userapps/trunk
  else
    echo "Could not find USERAPPS setup failed - not in standard location"
    userApps=/opt/rtcds/${site}/${ifo}/userapps/release
  fi
  if [ -f ${userApps}/etc/userapps-user-env.sh ] ; then
    source ${userApps}/etc/userapps-user-env.sh
  else
    echo "No USERAPP setup available - attempt manually"
#
    USERAPPS_DIR=${userApps}
    cdsModel=${userApps}/cds/${ifo}/models:${userApps}/cds/common/models
    hpiModel=${userApps}/hpi/${ifo}/models:${userApps}/hpi/common/models
    iooModel=${userApps}/ioo/${ifo}/models:${userApps}/ioo/common/models
    iscModel=${userApps}/isc/${ifo}/models:${userApps}/isc/common/models
    isiModel=${userApps}/isi/${ifo}/models:${userApps}/isi/common/models
    pslModel=${userApps}/psl/${ifo}/models:${userApps}/psl/common/models
    susModel=${userApps}/sus/${ifo}/models:${userApps}/sus/common/models
    tcsModel=${userApps}/tcs/${ifo}/models:${userApps}/tcs/common/models
    tstModel=${userApps}/cds/test/models
    export USERAPPS_LIB_PATH=${cdsModel}:${hpiModel}:${iooModel}:${iscModel}:${isiModel}:${pslModel}:${susModel}:${tcsModel}:${tstModel}
#
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
    export USERAPPS_MEDM_PATH=${cdsMedm}:$hpiMedm:$iooMedm:$iscMedm:$isiMedm:$pslMedm:$susMedm:$testMedm
#
    cdsScr=${userApps}/cds/common/scripts
    hpiScr=${userApps}/hpi/common/scripts
    iooScr=${userApps}/ioo/common/scripts
    iscScr=${userApps}/isc/common/scripts
    isiScr=${userApps}/isi/common/scripts
    pslScr=${userApps}/psl/common/scripts
    susScr=${userApps}/sus/common/scripts
    testScr=${userApps}/cds/test/scripts
    export USERAPPS_SCRIPTS_PATH=${cdsScr}:$hpiScr:$iooScr:$iscScr:$isiScr:$pslScr:$susScr:$testScr
  fi
 fi
#
# now that userapps is done, do the RCG specific stuff
#
#  alias to userapps install
 alias userCode="pushd ${USERAPPS_DIR}"
 alias usercode="pushd ${USERAPPS_DIR}"

# alias for RCG configuration script
 alias rcgConfig="${RCG_DIR}/configure"

# alias to get to RCG install
 alias rcgCode="pushd ${RCG_DIR}"
 alias rcgcode="pushd ${RCG_DIR}"

#  define RCG_LIB_PATH for builds
 simDir=${RCG_DIR}/src/epics/simLink
 export RCG_LIB_PATH=${USERAPPS_LIB_PATH}:${simDir}:${simDir}/lib

# define variables for post-build scripts
 export CDS_MEDM_PATH=${USERAPPS_MEDM_PATH}
 export CDS_SCRIPTS_PATH=${USERAPPS_SCRIPTS_PATH}
#
#  add models to MATLABPATH
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
#
fi
#

