#!/usr/bin/env bash
#
# script rcg_upgrade.sh - Upgrade RCG on front-end to 2.4
#
# save current location
thisDir=`pwd`
rcgTag="2.4.1"
#
echo "RCG "${rcgTag}" upgrade to stand-alone front-end"
tmpFold=/var/tmp/RCG2_4
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
scrTar=RCG-2.4-install-scripts.tar.gz
cfgTar=rtcdscfg-2_4.tar.gz
rcgTar=RCG_${rcgTag}.tar.gz
if [ -z "$downDone" ]; then
    echo "Downloading files using Kerberos login"
    kname=`klist | grep Default`
    if [ -n "$kname" ]; then
        echo "Kerberos login not set - restart after doing kinit <albert.einstein>@LIGO.ORG"
        exit
    else
        echo "kinit is good"
    fi 
#
    webDir="https://llocds.ligo-la.caltech.edu/daq/software/upgrades/RCG2_4"
    downScript=`grep DownloadScripts ${progFile}`
    if [ -z "$downScript" ]; then
	echo "Download installation scripts"
	rm -f ${tmpFold}/${scrTar}
        curl -u : --negotiate ${webDir}/${scrTar} > ${tmpFold}/${scrTar}
        echo "DownloadScripts" `date` >> ${progFile}
    else
	echo "Download installation scripts - ALREADY DONE"
    fi
#
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
    downRcg=`grep DownloadRcg ${progFile}`
    if [ -z $downRcg ]; then
        echo "Download new RCG"
	rm -f ${tmpFold}/${rcgTar}
        curl -u : --negotiate ${webDir}/${rcgTar} > ${tmpFold}/${rcgTar}
        echo "DownloadRcg" `date` >> ${progFile}
    else
        echo "Download new RCG - ALREADY DONE"
    fi
#  
    echo "DownloadDone" `date` >> ${progFile}
else
    echo "Downloading files using Kerberos login - ALREADY DONE"
fi
#
#  expand scripts
expScrDone=`grep ExpandScr ${progFile}`
scrDir=${tmpFold}/update
if [ -z "$expScrDone" ]; then
    echo "Expand tar-ball of scripts"  
    tar -C ${tmpFold} -xzf ${tmpFold}/${scrTar}
    echo "ExpandScr" `date` >> ${progFile}
else
    echo "Expand tar-ball of scripts - ALREADY DONE"
fi 
#
# get list of models
mdlListDone=`grep GenModelList ${progfile}`
if [ -z "$mdlListDone" ]; then
   ${scrDir}/gen_model_list.sh ${scrDir}
   echo "GenModelList" `date` >> ${progfile}
else
   echo "Get lists of models - ALREADY DONE"	
fi
#
# Create EPICS snapshots
epicsBkupDone=`grep EpicsBackup ${progfile}`
if [ -z "epicsBkupDone" ]; then
   ${scrDir}/backup_epics.sh ${scrDir}
   echo "EpicsBackup" `date` >> ${progfile}
else
   echo "Create EPICS shapshots - ALREADY DONE"	
fi
#
# Kill DAQ processes
daqKillDone=`grep DaqKilled ${progfile}`
if [ -z "daqKillDone" ]; then
   ${scrDir}/kill_daq.sh
   echo "DaqKilled" `date` >> ${progfile}
else
   echo "Kill DAQD,NDS - ALREADY DONE"	
fi
#
# Stop sub-models
subStopDone=`grep SubStop ${progfile}`
if [ -z "subStopDone" ]; then
   ${scrDir}/stop_sub.sh ${scrDir}
   echo "SubStop" `date` >> ${progfile}
else
   echo "Stop sub-models - ALREADY DONE"	
fi
#
# Stop IOP models
iopStopDone=`grep IopStop ${progfile}`
if [ -z "iopStopDone" ]; then
   ${scrDir}/stop_iops.sh ${scrDir}
   echo "IopStop" `date` >> ${progfile}
else
   echo "Stop IOP models - ALREADY DONE"	
fi
# 
# move and update userapps
#
appMoveDone=`grep UserAppMove ${progfile}`
if [ -z "appMoveDone" ]; then
   ${scrDir}/move_userapps.sh
   echo "UserAppMove" `date` >> ${progfile}
else
   echo "Move userapps to /opt/rtcds/userapps - ALREADY DONE"
fi
# 
# Expand RCG to /opt/rtcds/coreapps
expRcgDone=`grep ExpandRcg ${progFile}`
rcgDir=/opt/rtcds/coreapps
if [ -z "$expRcgDone" ]; then
    echo "Expand tar-ball of RCG in /opt/rtcds/coreapps" 
    mkdir -p ${rcgDir} 
    tar -C ${rcgDir} -xzf ${tmpFold}/${rcgTar}
    rcgPtr=`ls | grep "2.4"`
    cd ${rcgDir}
    rm -f release
    ln -s ${rcgPtr} release
    cd ${thisDir}
    echo "ExpandRcg" `date` >> ${progFile}
else
    echo "Expand tar-ball of RCG in /opt/rtcds/coreapps - ALREADY DONE"
fi 
#
#  install new cfg
cfgUpdateDone=`grep CfgUpdate ${progfile}`
if [ -z "cfgUpdateDone" ]; then
    echo "Install new cdscfg"
    tar -C /opt/cdscfg -xzf ${tmpFold}/${cfgTar}
    cd /opt/cdscfg
    install/cfg_fe.sh
    cd ${thisDir}
    echo "CfgUpdate" `date` >> ${progFile}
    echo "Now log out and log back in, then rerun this script to continue"
else
    echo "Install new cdscfg - ALREADY DONE"
fi

#  install new mbuf
#
mbufUpdateDone=`grep MbufUpdate ${progfile}`
if [ -z "mbufUpdateDone" ]; then
   ${scrDir}/new_mbuf.sh
   echo "MbufUpdate" `date` >> ${progfile}
else
   echo "Update mbuf - ALREADY DONE"
fi

# install new awgtpman
awgTpUpdateDone=`grep AwgTpUpdate ${progfile}`
if [ -z "awgTpUpdateDone" ]; then
   ${scrDir}/new_awgtpman.sh ${rcgTag}
   echo "AwgTpUpdate" `date` >> ${progfile}
else
   echo "Update awgtpman - ALREADY DONE"
fi

# install new real-time dataviewer
dvUpdateDone=`grep DvUpdate ${progfile}`
if [ -z "dvUpdateDone" ]; then
   ${scrDir}/new_dv.sh ${rcgTag}
   echo "DvUpdate" `date` >> ${progfile}
else
   echo "Update dataviewer - ALREADY DONE"
fi

# create new build area
cfgRtBuildDone=`grep CfgRtBuild ${progfile}`
if [ -z "cfgRtBuildDone" ]; then
   echo "Configure real-time build area"	
   cd ${RTCDSROOT}
   mkdir -p rtbuild
   ${RCG_DIR}/configure	
   echo "CfgRtBuild" `date` >> ${progfile}
else
   echo "Configure real-time build area - ALREADY DONE"
fi

# Rebuild models
mdlBuildDone=`grep ModelBuild ${progfile}`
if [ -z "$mdlBuildDone" ]; then
   ${scrDir}/remake_models.sh ${scrDir}
   echo "ModelBuild" `date` >> ${progfile}
else
   echo "Rebuild models with new RCG - ALREADY DONE"	
fi

# Rebuild stand-alone daqd
daqdBuildDone=`grep DaqdBuild ${progfile}`
if [ -z "$daqdBuildDone" ]; then
   ${scrDir}/new_daqd.sh ${rcgTag}
   echo "DaqdBuild" `date` >> ${progfile}
else
   echo "Build new stand-alone daqd - ALREADY DONE"	
fi

# Rebuild nds
ndsBuildDone=`grep NdsBuild ${progfile}`
if [ -z "$ndsBuildDone" ]; then
   ${scrDir}/new_nds.sh ${rcgTag}
   echo "NdsBuild" `date` >> ${progfile}
else
   echo "Build new nds - ALREADY DONE"	
fi

# Restart the models
mdlRestartDone=`grep ModelRestart ${progfile}`
if [ -z "$mdlRestartDone" ]; then
   ${scrDir}/restart_models.sh ${scrDir}
   echo "ModelRestart" `date` >> ${progfile}
else
   echo "Restart models built with new RCG - ALREADY DONE"	
fi

# Restart DAQ, NDS
daqRestartDone=`grep DaqRestart ${progfile}`
if [ -z "$mdlRestartDone" ]; then
   ${scrDir}/restart_daq.sh
   echo "DaqRestart" `date` >> ${progfile}
else
   echo "Restart DAQ built with new RCG - ALREADY DONE"	
fi

