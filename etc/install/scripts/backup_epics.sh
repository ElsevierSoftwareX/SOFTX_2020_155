#!/bin/bash
source /opt/cdscfg/rtsetup.sh
cd /var/update
mkdir -p snapshots
for model in $(cat models/all_models.txt); do
  snapFile=snapshots/${model}_backup.snap
   reqFile=/opt/rtcds/${site}/${ifo}/target/${model}/${model}epics/autoBurt.req
   burtrb -f ${reqFile} -o ${snapFile}
done
