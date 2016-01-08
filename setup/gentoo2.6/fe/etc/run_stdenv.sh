#!/bin/bash --noprofile 
#
# invoke excutable under stdenv

EXEPAR=$1

# get definitions from stdenv
#  FIRST we clear existing command-line inputs
set --
source /opt/cdscfg/stdenv.sh

# add script directory to path
PATH=$PATH:$SCRIPTDIR
export PATH
# add EPICS_DB def
EPICS_DB_INCLUDE_PATH=$EPICS_BASE/dbd
export EPICS_DB_INCLUDE_PATH
# execute command

source $EXEPAR

