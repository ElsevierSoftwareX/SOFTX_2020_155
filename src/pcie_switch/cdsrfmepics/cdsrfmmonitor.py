#!/usr/bin/env python3
# Code to interface cdsrfmswitch kernel module code and EPICS database

import time
import string
import sys
import os
import argparse
import threading

# get CDS EPICS environment
sys.path.append('/opt/cdscfg')
import subprocess
import stdenv as cds
cds.INIT_ENV()

# now get EPICS python
from pcaspy import SimpleServer, Driver


parser = argparse.ArgumentParser(description='Provides interface between cdsrfmswitch kernel code and cdsrfmswitch EPICS database. This code MUST run only on the cdsrfmswitch computer.')
parser.add_argument("ifo",
			help = "Takes IFO prefix (X2,H1,L1,etc.) as argument")
args = parser.parse_args()

prefix = args.ifo + ':CDS-RFM_LRS_'

epics_chan = []
value = 0
chan_list = ('EX2CS_CHCNT',
		'CS2EX_CHCNT',
		'CS2EY_CHCNT',
		'EY2CS_CHCNT',
		'TIME',
		'EX2CS_ACTIVE1',
		'EX2CS_ACTIVE2',
		'ACTIVE03',
		'CS2EX_ACTIVE1',
		'CS2EX_ACTIVE2',
		'ACTIVE13',
		'CS2EY_ACTIVE1',
		'CS2EY_ACTIVE2',
		'ACTIVE23',
		'EY2CS_ACTIVE1',
		'EY2CS_ACTIVE2',
		'ACTIVE33',
		'STATUS')
pvdb = {
    	chan_list[0] : {
		'type' : 'int', 
		'scan' : 1,
	},
    	chan_list[1] : {
		'type' : 'int', 
		'scan' : 1,
	},
    	chan_list[2] : {
		'type' : 'int', 
		'scan' : 1,
	},
    	chan_list[3] : {
		'type' : 'int', 
		'scan' : 1,
	},
    	chan_list[4] : {
		'type' : 'int', 
		'scan' : 1,
	},
    	chan_list[5] : {
		'type' : 'int', 
		'scan' : 1,
	},
    	chan_list[6] : {
		'type' : 'int', 
		'scan' : 1,
	},
    	chan_list[7] : {
		'type' : 'int', 
		'scan' : 1,
	},
    	chan_list[8] : {
		'type' : 'int', 
		'scan' : 1,
	},
    	chan_list[9] : {
		'type' : 'int', 
		'scan' : 1,
	},
    	chan_list[10] : {
		'type' : 'int', 
		'scan' : 1,
	},
    	chan_list[11] : {
		'type' : 'int', 
		'scan' : 1,
	},
    	chan_list[12] : {
		'type' : 'int', 
		'scan' : 1,
	},
    	chan_list[13] : {
		'type' : 'int', 
		'scan' : 1,
	},
    	chan_list[14] : {
		'type' : 'int', 
		'scan' : 1,
	},
    	chan_list[15] : {
		'type' : 'int', 
		'scan' : 1,
	},
    	chan_list[16] : {
		'type' : 'int', 
		'scan' : 1,
	},
    	chan_list[17] : {
		'type' : 'int', 
		'scan' : 1,
	},
}

class myDriver(Driver):
	def __init__(self):
		super(myDriver, self).__init__()

	def read(self, reason):
		value = 0
		if reason == chan_list[4]:
			try:
				# Read /proc file produced by kmod code
				with open("/proc/cdsrfm","r") as datafile:
					first_line = datafile.readline()
				word = first_line.split()

				# Relay info to EPICS
				for ii,data in enumerate(word):
					self.setParam(chan_list[ii], int(data))
				value = self.getParam(reason)
				datafile.close()
			except:
				# If /proc file does not exist, send fault status to EPICS
				self.setParam(chan_list[ii], 0)
		else:
			value = self.getParam(reason)
		return value

if __name__ == '__main__':
	server = SimpleServer()
	server.createPV(prefix, pvdb)
	driver = myDriver()


while True:
	server.process(1)
