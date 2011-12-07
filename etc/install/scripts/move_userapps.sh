#!/usr/bin/env bash
# script to re-locate userapps to /opt/rtcds
#
source /opt/cdscfg/rtsetup.sh
#
newHome=/opt/rtcds/userapps
newPath=${newHome}/release
if [ -d ${newHome} ]; then
    echo "Userapps already moved to /opt/rtcds/userapps"
    cd ${newHome}    
else
    mkdir -p ${newHome}
    oldHome=/opt/rtcds/${site}/${ifo}/userapps
    if [ -d ${oldHome} ]; then
       echo "Move userapps to /opt/rtcds/userapps"
       mv /opt/rtcds/${site}/${ifo}/userapps/* ${newHome}
       cd ${newHome}
       rm release
       if [ -d trunk ]; then
           ln -s trunk release
       elif [ -d cds_user_apps/trunk ]; then
           ln -s cds_user_apps/trunk release      
       else
           echo "OOPS - unable to create release - do it manually"
           exit 1
       fi
    else
       echo "No userapps found - check out a fresh one"
       appsSvn="https://redoubt.ligo-wa.caltech.edu/svn/cds_user_apps"
       cd ${newHome}
       svn co ${appsSvn}
       ln -s cds_user_apps/trunk release
    fi
fi
echo "Update userapps from cds_user_apps Subversion"
cd ${newHome}
svn update release
exit 0


