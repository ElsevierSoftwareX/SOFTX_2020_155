#!/usr/bin/env python3
# Automated test support classes

import epics
import time
import string
import sys, argparse
import os
import os.path

from epics import PV
from epics import caget,caput
from subprocess import call


parser = argparse.ArgumentParser(description='Process some integers.')
parser.add_argument('-p',dest='prefix')
parser.add_argument('-d',dest='dcuid')
parser.add_argument('-l',dest='line_num')
parser.add_argument('-m',dest='medm_dir')

#parser.add_argument('-i',dest='chname')
#parser.add_argument('-m',dest='model')
#parser.add_argument('-d',dest='medmdir')
#parser.add_argument('-rcgd',dest='rcgdir')

args = parser.parse_args()

basedir = args.medm_dir
display = os.path.join(basedir, 'SDF_FILT_MONITOR.adl')

if args.prefix.endswith(':'):
	args.prefix=args.prefix[:-1]

filter_base = PV("{0}:FEC-{1}_SDF_SP_STAT{2}".format(args.prefix, args.dcuid, args.line_num))
basename = ''
for char in filter_base.value:
	if char == 0:
		break
	basename = basename + chr(char)
basename=basename.split(':',1)[1]

filter_num_map = PV("{0}:FEC-{1}_SDF_FM_LINE_{2}".format(args.prefix, args.dcuid, args.line_num))
fmnum = filter_num_map.value

if fmnum < 0:
	print('NOT A SWITCH CHANNEL')
else:
	myargs = "FPREFIX={0},DCUID={1},FMNUM={2},FNAME={3}".format(args.prefix, args.dcuid,int(fmnum),basename)
	medm_args = ['medm', '-attach', '-x', '-macro', str(myargs), str(display)]
	for arg in medm_args:
		sys.stderr.write("{0} - {1}\n".format(arg, type(arg)))
	sys.stderr.write("\n\n"+str(medm_args)+"\n\n\n\n")
	call(medm_args)
