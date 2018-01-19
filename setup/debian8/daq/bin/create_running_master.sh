#!/bin/sh

# Copy INI and PAR files to a secure location and create a master file which references them
# This will permit restarts to frame writers and nds machines against the channel configuration
# at the time of the data concentrator start, and prevent modified INI and PAR files from being used.
# D.Barker 23May2012
# 
#  First parameter - 1/0 if data concentrator
#  Second parameter - master file
#  Third parameter - destination directory
dcflag=$1
if [ ${dcflag} != 1 ]; then
   exit 0
fi  
masterfile=$2
daqdir=$3

# make sure these exist
if [ ! -e $masterfile ]; then
   echo "DAQ master file $masterfile does not exist!"
   exit 1
fi
if [ ! -d $daqdir ]; then
   echo "DAQ directory $daqdir does not exist - create it"
   mkdir $daqdir
fi

# INI and PAR files copied into a directory named by the current date-time
# a symlink called running is pointed to the latest directory
DATETIME=`date +%d%m%y_%H:%M:%S`
newdir=${daqdir}/${DATETIME}
mkdir -p ${newdir}
rm -rf ${daqdir}/running
ln -s ${newdir} ${daqdir}/running
#
#  loop through channel list files in 'masterfile'
for f in `grep "^/opt" ${masterfile}`
do
   cp $f ${newdir}
done

# create new master file referencing this running configuration
rm -f ${daqdir}/running/master
for f in ${daqdir}/running/*
do
   echo $f >> ${daqdir}/running/master
done

exit 0
