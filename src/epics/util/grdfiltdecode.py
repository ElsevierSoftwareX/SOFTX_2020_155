#!/usr/bin/env python
# Automated test support classes

import epics
import time
import string
import sys, argparse
import os

from epics import PV
from epics import caget,caput
from subprocess import call


parser = argparse.ArgumentParser(description='Process some integers.')
parser.add_argument('-i',dest='chname')
parser.add_argument('-m',dest='model')
parser.add_argument('-d',dest='medmdir')
args = parser.parse_args()
basedir = args.medmdir

ec = PV(args.chname)
grdchan = ec.value

modelname = args.model.upper()


if '_SW1S' in grdchan or '_SW2S' in grdchan:
	x = len(grdchan) - 5
	medmDisplay = ''
	temp1 = grdchan[:x] + '.adl'
	word = temp1.split('-')
	for x in word:
		print "Split ",x
	temp2 = temp1.replace(":","")
	temp3 = modelname + '_' + word[1]
	medmDisplay = basedir + temp3
	print 'Value of ',grdchan, ' is ', medmDisplay
	print 'Model is ',modelname
	call(["medm","-attach",medmDisplay])
else:
	print 'NOT A SWITCH CHANNEL'
