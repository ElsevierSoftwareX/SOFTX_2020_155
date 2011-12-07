#!/usr/bin/env bash
# stop sub-models
echo "Stop sub-models"
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
for model in $(cat models/sub_models.txt); do
  kill${model}
done

