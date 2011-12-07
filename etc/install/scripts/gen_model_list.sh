#!/usr/bin/env bash
# script to get lists of models on test stand front-end
echo "Get lists of models"
# Were we passed UPDATE_DIR information on the command line?
if [ $# -gt 0 ]; then
   UPDATE_DIR=$1
   echo 'OR HERE'
else
  if [ "$UPDATE_DIR" ]; then
     UPDATE_DIR=${UPDATE_DIR}
  else
     UPDATE_DIR=/var/tmp/update
     mkdir -p $UPDATE_DIR
  fi   
fi
echo 'GOT HER'
mdlDir=${UPDATE_DIR}/models
mkdir -p ${mdlDir}
#
source /opt/cdscfg/rtsetup.sh
#
cd /opt/rtcds/${site}/${ifo}/target
#
ls -d ${ifo}* > ${mdlDir}/all_models.txt
echo "IOP models found:"
cat ${mdlDir}/all_models.txt | grep iop
cat ${mdlDir}/all_models.txt | grep iop > ${mdlDir}/iop_models.txt
#
echo "sub models found:"
cat ${mdlDir}/all_models.txt | grep -v iop
cat ${mdlDir}/all_models.txt | grep -v iop > ${mdlDir}/sub_models.txt