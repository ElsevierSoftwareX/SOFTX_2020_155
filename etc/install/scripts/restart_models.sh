#!/usr/bin/env bash
#  restart models
echo "Restart models built with new RCG"
# Were we passed UPDATE_DIR information on the command line?
if [ $# -gt 0 ]; then
   UPDATE_DIR=$1
else
  if [ "$UPDATE_DIR" ]; then
     UPDATE_DIR=${UPDATE_DIR}
  else
     UPDATE_DIR=/var/update
fi
#
source /opt/cdscfg/rtsetup.sh
#
cd ${UPDATE_DIR}
for iopmodel in $(cat models/iop_models.txt); do
  start${iopmodel}
  snapFile=snapshots/${iopmodel}_backup.snap
  burtwb -f ${snapFile}
  start${iopmodel}
done
for model in $(cat models/sub_models.txt); do
  start${model}
  snapFile=snapshots/${model}_backup.snap
  burtwb -f ${snapFile}
  start${model}
done
