#!/usr/bin/env bash
if [ "$# -gt 0" ]; then
   UPDATE_DIR=$1
else
   if [ "$UPDATE_DIR" ]; then
      UPDATE_DIR=${UPDATE_DIR}
   else
      UPDATE_DIR=/var/tmp/update
      mkdir -p $UPDATE_DIR
   fi
fi
cd ${UPDATE_DIR}
mkdir -p snapshots

source /opt/cdscfg/rtsetup.sh

for model in $(cat models/all_models.txt); do
  snapFile=snapshots/${model}_backup.snap
   reqFile=/opt/rtcds/${site}/${ifo}/target/${model}/${model}epics/autoBurt.req
   burtrb -f ${reqFile} -o ${snapFile}
done
