#!/usr/bin/env python

import epics
from Tkinter import *
import Tkconstants, tkFileDialog
from epics import PV
import sys, argparse
import time

parser = argparse.ArgumentParser(description='Input Model Info')
parser.add_argument('-s',dest='site')
parser.add_argument('-i',dest='ifo')
parser.add_argument('-m',dest='model')
parser.add_argument('-d',dest='dcuid')
parser.add_argument('-t',dest='ftype')
args = parser.parse_args()

#root = Tk()
mydir = '/opt/rtcds/' + args.site + '/' + args.ifo + '/target/' + args.model + '/' + args.model + 'epics/burt/'
defaultfile = args.model + '_safe.snap'
dcuid = 'H1:FEC-' + args.dcuid

file_opt = options = {}
options['defaultextension'] = '.snap'
options['filetypes'] = [('burt files', '.snap'),('all files', '.*')]
options['initialdir'] = mydir
#options['parent'] = root
options['title'] = 'Select BURT File'

# get filename
filename = tkFileDialog.askopenfilename(**file_opt)
if filename:
	word = filename.split('/')
	mp = len(word)
	print "I got %d file params. %s %s" % (len(word), word[mp - 2], word[mp-1])
	print "ftype = ",args.ftype
	if args.ftype is '0':
		ename = dcuid + '_SDF_NAME'
	else:
		ename = dcuid + '_SDF_NAME_SUBSET'
	e = PV(ename)
	fn = word[mp-1].split('.')
	e.value = fn[0]
