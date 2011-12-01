#!/usr/bin/env bash
# stop  IOP models
echo "Stop IOP models"
#  allow input of update directory
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
#
for model in $(cat models/iop_models.txt); do
  kill${model}
done
