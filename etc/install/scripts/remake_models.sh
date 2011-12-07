#!/usr/bin/env bash
#  rebuild models with new RCG
echo "Rebuild models with new RCG"
# Were we passed UPDATE_DIR information on the command line?
if [ $# -gt 0 ]; then
   UPDATE_DIR=$1
else
  if [ "$UPDATE_DIR" ]; then
     UPDATE_DIR=${UPDATE_DIR}
  else
     UPDATE_DIR=/var/update
fi
mdlDir=${UPDATE_DIR}/models
#
source /opt/cdscfg/rtsetup.sh
#
cdsCode
#
for iopmodel in $(cat ${mdlDir}/iop_models.txt); do
  make ${iopmodel}
  make install-${iopmodel}
done
for model in $(cat ${mdlDir}/sub_models.txt); do
  make ${model}
  make install-${model}
done
