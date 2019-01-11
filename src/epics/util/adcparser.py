#!/usr/bin/env python

# Purpose: Produce a listing of all ADC inputs to their corresponding RCG part connections.
# This script was added primarily to handle MUX/DEMUX connections, as these are not taken
# out by the main RCG parser.

# Called From: feCodeGen.pl
# Input: diags.txt file, produced by the feCodeGen main parser, which lists all parts and
#	 their connections just before code compilation.
# Output: diags2.txt file, which lists all ADC channel connections.  

from subprocess import call
import sys
import time
import os
import string
#import argparse

class RCG_PART(object):
	"""
	Attributes:
		name 		Name of the part
		ptype		RCG part type
		inputCnta	Number of inputs
		outputCnt	Number of outputs
		hasadc		One or more inputs from an ADC
		Following are array variables to track part inputs
			inpartname	Names of input connection parts
			inparttype	Types of input part 
			inpartnum	RCG assined number of the input part
			inpartport	Input port to which the part is connected
		Following are array variables to track part outputs
			outpartname	Names of output connection parts
			outparttype	RCG part type at the output connection
			outpartnum	RCG assigned number of the output part
			outpartport	Port number to which output is connected
			outportused	Input port number of connected RCG part
	"""
	def __init__(self):
		self.name = 'empty'
		self.ptype = 'none'
		self.inputCnt = 0
		self.outputCnt = 0
		self.hasadc = 0
		self.inpartname = []
		self.inparttype = []
		self.inpartnum = []
		self.inpartport = []
		self.outpartname = []
		self.outparttype = []
		self.outpartnum = []
		self.outpartport =[]
		self.outportused = []

rcgparts = [ RCG_PART() for i in range(6000)]
partCnt = 0
ininputs = 0
inoutputs = 0
bustype = 0

#parser = argparse.ArgumentParser(description='Create ADC connection list')
#parser.add_argument("fName")
#args = parser.parse_args()

#f = open(args.fName,'r')
print os.getcwd()
time.sleep(10)
f = open('./diags.txt','r')

for line in f:
	word = line.split()
	x = len(word)
	if "Part" in line and "Part Name" not in line and "Parameters" not in line and x > 10:
		rcgparts[partCnt].name = word[2]
		rcgparts[partCnt].ptype = word[5]
		rcgparts[partCnt].inputCnt = word[7]
		rcgparts[partCnt].outputCnt = word[10]
		ii = partCnt
		partCnt += 1
		ininputs = 0
		inoutputs = 0
		bustype = 0
		if 'BUSS' in line:
			bustype = 1
	if "INS FROM" in line:
		ininputs = 1
		inoutputs = 0
	if "OUT TO" in line:
		ininputs = 0
		inoutputs = 1
	if "***********************" in line:
		ininputs = 0
		inoutputs = 0
	x = len(word)
	if ininputs == 1 and "Part Name" not in line and "INS" not in line and x > 3:
		rcgparts[ii].inpartname.append(word[0])
		rcgparts[ii].inparttype.append(word[1])
		rcgparts[ii].inpartnum.append(word[2])
		rcgparts[ii].inpartport.append(word[3])
		if "adc_" in line:
			rcgparts[ii].hasadc = 1
	if ininputs == 1 and bustype == 1 and x > 2:
		rcgparts[ii].inpartname.append(word[0])
		rcgparts[ii].inparttype.append(word[1])
		rcgparts[ii].inpartport.append(word[2])
		if "adc_" in line:
			rcgparts[ii].hasadc = 1
	if inoutputs == 1 and "Part Name" not in line and "OUT TO" not in line and x > 4:
		rcgparts[ii].outpartname.append(word[0])
		rcgparts[ii].outparttype.append(word[1])
		rcgparts[ii].outpartnum.append(word[2])
		rcgparts[ii].outpartport.append(word[3])
		rcgparts[ii].outportused.append(word[4])


f.close

for ii in range(0,partCnt):
	if 'DEMUX' in rcgparts[ii].ptype and 'MUX' in rcgparts[ii].inparttype:
		for jj in range(0,partCnt):
			if rcgparts[ii].inpartname[0] == rcgparts[jj].name:
				minnum = jj
		xx = 0
		for item in rcgparts[ii].outpartname:
			mys = int(rcgparts[ii].outportused[xx])
			for kk in range(0,partCnt):
				if rcgparts[kk].name == rcgparts[ii].outpartname[xx]:
					rcgparts[kk].inpartname[0] = rcgparts[minnum].inpartname[mys]
			xx += 1
s = ""
for ii in range(0,partCnt):
	if 'BUS' not in rcgparts[ii].ptype and 'MUX' not in rcgparts[ii].ptype:
		for item in rcgparts[ii].inpartname:
			if 'adc_' in item:
				words = item.split('_')
				s += words[1] +'\t' + words[2] +'\t' + rcgparts[ii].name + '\t' + rcgparts[ii].ptype + '\n'
f = open('./diags2.txt','w')
f.write(s)
f.close()
