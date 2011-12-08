#!/usr/bin/env bash
#
# script rcg_stand_upgrade.sh - Upgrade RCG on stand-alone front-end to 2.4
#
# save current location
thisDir=`pwd`
rcgTag="2.4.1"
#
echo "RCG "${rcgTag}" upgrade to stand-alone front-end"
tmpFold=/var/tmp/RCG_Update
progFile=${tmpFold}/upgrade_progress.txt
if [ -f ${progFile} ]; then
    echo "Resume upgrade"
else
    while true; do
        read -p "Do you want to proceed with the install?" yn
        case $yn in
            [Yy]* ) break;;
	    [Nn]* ) exit;;
                * ) "Please answer yes or no";;
        esac
    done
#
    mkdir -p ${tmpFold}
    rm -rf ${tmpFold}/*
    touch ${progFile}
fi
#
# check if downloads complete
# 
downDone=`grep DownloadDone ${progFile}`
cfgTar=cdscfg-2_4.tar.gz
echo 'downDone' $downDone
if [ -z "$downDone" ]; then
    echo "Downloading files using Kerberos login"
    klLoc=`which klist` 
    if [ -z "$klLoc" ]; then
        echo " Kerberos not installed, emerge krb5, curl"
        sudo emerge krb5
        sudo emerge curl
    fi    
    kname=`klist | grep Default`
    if [ -z "$kname" ]; then
        echo "Kerberos login not set - restart after doing kinit <albert.einstein>@LIGO.ORG"
        exit
    else
        echo "kinit is good"
    fi 
#
    webDir="https://llocds.ligo-la.caltech.edu/daq/software/upgrades"
    downCfg=`grep DownloadCfg ${progFile}`
    if [ -z "$downCfg" ]; then
	    echo "Download new environment configuration"
	    rm -f ${tmpFold}/${cfgTar}
	    curl -u : --negotiate ${webDir}/${cfgTar} > ${tmpFold}/${cfgTar}
	    echo "DownloadCfg" `date` >> ${progFile}
    else
	    echo "Download new environment configuration - ALREADY DONE"
    fi
#  
    echo "DownloadDone" `date` >> ${progFile}
else
    echo "Downloading files using Kerberos login - ALREADY DONE"
fi
#
#  copy scripts
cpScrDone=`grep CopyScr ${progFile}`
scrDir=${tmpFold}/scripts
if [ -z "$cpScrDone" ]; then
    echo "Copy install scripts to work area"
    mkdir -p ${scrDir}
    cp ${thisDir}/scripts/* ${scrDir}
    echo "CopyScr" `date` >> ${progFile}
else
    echo "Copy install scripts to work area - ALREADY DONE"
fi 
#
# get list of models
mdlListDone=`grep GenModelList ${progFile}`
if [ -z "$mdlListDone" ]; then
   ${scrDir}/gen_model_list.sh ${scrDir}
   echo "GenModelList" `date` >> ${progFile}
else
   echo "Get lists of models - ALREADY DONE"	
fi
#
# Create EPICS snapshots
epicsBkupDone=`grep EpicsBackup ${progFile}`
if [ -z "$epicsBkupDone" ]; then
   ${scrDir}/backup_epics.sh ${scrDir}
   echo "EpicsBackup" `date` >> ${progFile}
else
   echo "Create EPICS shapshots - ALREADY DONE"	
fi
#
# Kill DAQ processes
daqKillDone=`grep DaqKilled ${progFile}`
if [ -z "$daqKillDone" ]; then
   ${scrDir}/kill_daq.sh
   echo "DaqKilled" `date` >> ${progFile}
else
   echo "Kill DAQD,NDS - ALREADY DONE"	
fi
#
# Stop sub-models
subStopDone=`grep SubStop ${progFile}`
if [ -z "$subStopDone" ]; then
   ${scrDir}/stop_sub.sh ${scrDir}
   echo "SubStop" `date` >> ${progFile}
else
   echo "Stop sub-models - ALREADY DONE"	
fi
#
# Stop IOP models
iopStopDone=`grep IopStop ${progFile}`
if [ -z "$iopStopDone" ]; then
   ${scrDir}/stop_iops.sh ${scrDir}
   echo "IopStop" `date` >> ${progFile}
else
   echo "Stop IOP models - ALREADY DONE"	
fi
# 
# move and update userapps
appMoveDone=`grep UserAppMove ${progFile}`
if [ -z "$appMoveDone" ]; then
   ${scrDir}/move_userapps.sh 
   retCode=#?
   echo "UserAppMove" `date` >> ${progFile}
   if [ "$retCode -ne 0" ]; then
       exit 1
   fi
else
   echo "Move userapps to /opt/rtcds/userapps - ALREADY DONE"
fi
# 
# Copy RCG to /opt/rtcds/rtscore
cpRcgDone=`grep CopyRcg ${progFile}`
rcgDir=/opt/rtcds/rtscore
if [ -z "$cpRcgDone" ]; then
    echo "Copy this RCG in /opt/rtcds/rtscore" 
    mkdir -p ${rcgDir} 
    rcgPath=${thisDir%%/etc/install}
    cp -Rp ${rcgPath} ${rcgDir}   
    rcgPtr=${rcgPath##/*/}
    cd ${rcgDir}
    rm -f release
    ln -s ${rcgPtr} release
    cd ${thisDir}
    echo "CopyRcg" `date` >> ${progFile}
else
    echo "Copy this RCG to /opt/rtcds/rtscore - ALREADY DONE"
fi 
#
#  install new cfg
cfgUpdateDone=`grep CfgUpdate ${progFile}`
if [ -z "$cfgUpdateDone" ]; then
    echo "Install new cdscfg"
    cd ${tmpFold}
    echo " went to "${tmpFold}
    echo "try to open" ${cfgTar}
    tar -xzf ${cfgTar}
    cd cdscfg/install
    ./cfg_fe.sh
    cd ${thisDir}
    echo "CfgUpdate" `date` >> ${progFile}
    echo "Now log out and log back in, then rerun this script to continue"
else
    echo "Install new cdscfg - ALREADY DONE"
fi
#
# install RCG scripts
rcgScrDone=`grep RcgScripts ${progFile}`
if [ -z "$rcgScrDone" ]; then
    echo "Copy RCG scripts to scripts area"
    cd ${rcgDir}/release
    cp src/epics/util/iniChk.pl /opt/rtcds/${site}/${ifo}/scripts
    cd ${thisDir}
    echo "RcgScripts" `date` >> ${progFile}
else
    echo "Install RCG scripts - ALREADY DONE"
fi

#  install new mbuf
#
mbufUpdateDone=`grep MbufUpdate ${progFile}`
if [ -z "$mbufUpdateDone" ]; then
   ${scrDir}/new_mbuf.sh ${rcgTag}
   echo "MbufUpdate" `date` >> ${progFile}
else
   echo "Update mbuf - ALREADY DONE"
fi
#
# install new awgtpman
awgTpUpdateDone=`grep AwgTpUpdate ${progFile}`
if [ -z "$awgTpUpdateDone" ]; then
   ${scrDir}/new_awgtpman.sh ${rcgTag}
   echo "AwgTpUpdate" `date` >> ${progFile}
else
   echo "Update awgtpman - ALREADY DONE"
fi

# install new real-time dataviewer
dvUpdateDone=`grep DvUpdate ${progFile}`
if [ -z "$dvUpdateDone" ]; then
   ${scrDir}/new_dv.sh ${rcgTag}
   echo "DvUpdate" `date` >> ${progFile}
else
   echo "Update dataviewer - ALREADY DONE"
fi

# create new build area
cfgRtBuildDone=`grep CfgRtBuild ${progFile}`
if [ -z "$cfgRtBuildDone" ]; then
   echo "Configure real-time build area"	
   cd ${RTCDSROOT}
   mkdir -p rtbuild
   cd rtbuild
   ${RCG_DIR}/configure	
   echo "CfgRtBuild" `date` >> ${progFile}
else
   echo "Configure real-time build area - ALREADY DONE"
fi

# Rebuild models
mdlBuildDone=`grep ModelBuild ${progFile}`
if [ -z "$mdlBuildDone" ]; then
   ${scrDir}/remake_models.sh ${scrDir}
   echo "ModelBuild" `date` >> ${progFile}
else
   echo "Rebuild models with new RCG - ALREADY DONE"	
fi

# Rebuild stand-alone daqd
daqdBuildDone=`grep DaqdBuild ${progFile}`
if [ -z "$daqdBuildDone" ]; then
   ${scrDir}/new_daqd.sh ${rcgTag}
   echo "DaqdBuild" `date` >> ${progFile}
else
   echo "Build new stand-alone daqd - ALREADY DONE"	
fi

# Rebuild nds
ndsBuildDone=`grep NdsBuild ${progFile}`
if [ -z "$ndsBuildDone" ]; then
   ${scrDir}/new_nds.sh ${rcgTag}
   echo "NdsBuild" `date` >> ${progFile}
else
   echo "Build new nds - ALREADY DONE"	
fi

# Restart the models
mdlRestartDone=`grep ModelRestart ${progFile}`
if [ -z "$mdlRestartDone" ]; then
   ${scrDir}/restart_models.sh ${scrDir}
   echo "ModelRestart" `date` >> ${progFile}
else
   echo "Restart models built with new RCG - ALREADY DONE"	
fi

# Restart DAQ, NDS
daqRestartDone=`grep DaqRestart ${progFile}`
if [ -z "$daqRestartDone" ]; then
   ${scrDir}/restart_daq.sh
   echo "DaqRestart" `date` >> ${progFile}
else
   echo "Restart DAQ built with new RCG - ALREADY DONE"	
fi
