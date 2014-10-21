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
parser.add_argument('-rcgd',dest='rcgdir')
args = parser.parse_args()
basedir = args.medmdir
filtalhdisplay = args.rcgdir + 'FILTALH.adl'

ec = PV(args.chname)
grdchan = ec.value

modelname = args.model.upper()


if '_SW1S' in grdchan or '_SW2S' in grdchan:
	x = len(grdchan) - 5
	medmDisplay = ''
	temp = grdchan[:x]
	ifo,filt = temp.split(':')
	temp1 = grdchan[:x] + '.adl'
	word = temp1.split('-')
	temp2 = temp1.replace(":","")
	temp3 = modelname + '_' + word[1]
	medmDisplay = basedir + temp3
	myargs = '"FPREFIX=' + ifo +',FNAME=' + filt +',RDISP=' + medmDisplay
	#call(["medm","-x","-attach","-macro",myargs,filtalhdisplay])
	call(["medm","-attach","-macro",myargs,filtalhdisplay])
else:
	print 'NOT A SWITCH CHANNEL'
