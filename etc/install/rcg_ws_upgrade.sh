#!/usr/bin/env bash
#
# script rcg_ws_upgrade.sh - Upgrade workstation for RCG 2.4
#
# save current location
thisDir=`pwd`
rcgTag="2.4.1"
#
echo "RCG "${rcgTag}" upgrade to workstation"
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
        echo " Install Kerberos - try sudo apt-get install krb5"
        exit
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
#  install new cfg
cfgUpdateDone=`grep CfgUpdate ${progFile}`
if [ -z "$cfgUpdateDone" ]; then
    echo "Install new cdscfg"
    cd ${tmpFold}
    echo " went to "${tmpFold}
    echo "try to open" ${cfgTar}
    tar -xzf ${cfgTar}
    cd cdscfg/install
    ./cfg_ws.sh
    cd ${thisDir}
    echo "CfgUpdate" `date` >> ${progFile}
    echo "Now log out and log back in, then rerun this script to continue"
else
    echo "Install new cdscfg - ALREADY DONE"
fi

# install new dataviewer
dvUpdateDone=`grep DvUpdate ${progFile}`
if [ -z "$dvUpdateDone" ]; then
   ${scrDir}/new_dv_ws.sh ${rcgTag}
   echo "DvUpdate" `date` >> ${progFile}
else
   echo "Update dataviewer - ALREADY DONE"
fi
