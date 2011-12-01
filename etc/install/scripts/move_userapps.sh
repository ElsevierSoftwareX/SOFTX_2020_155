#!/usr/bin/env bash
# script to re-locate userapps to /opt/rtcds
#
source /opt/cdscfg/rtsetup.sh
#
newHome=/opt/rtcds/userapps
if [ -f ${newHome}]; then
    echo "Userapps already moved to /opt/rtcds/userapps"
else
    echo "Move userapps to /opt/rtcds/userapps"
    mv /opt/rtcds/${site}/${ifo}/userapps ${newHome}
fi
echo "Update userapps from cds_user_apps Subversion"
cd ${newHome}
rm release
ln -s trunk release
svn update release


