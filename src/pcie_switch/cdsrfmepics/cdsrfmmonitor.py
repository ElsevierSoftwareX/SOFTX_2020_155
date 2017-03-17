#!/usr/bin/env python
# Code to interface cdsrfmswitch kernel module code and EPICS database

import epics
import time
import string
import sys
import os

from epics import PV
import argparse

parser = argparse.ArgumentParser(description='Provides interface between cdsrfmswitch kernel code and cdsrfmswitch EPICS database. This code MUST run only on the cdsrfmswitch computer.')
parser.add_argument("ifo",
			help = "Takes IFO prefix (X2,H1,L1,etc.) as argument")
args = parser.parse_args()

epics_chan = []
chan_list = (':CDS-RFM_SWITCH_CHCNT1',
		':CDS-RFM_SWITCH_CHCNT2',
		':CDS-RFM_SWITCH_CHCNT3',
		':CDS-RFM_SWITCH_CHCNT4',
		':CDS-RFM_SWITCH_TIME',
		':CDS-RFM_SWITCH_ACTIVE01',
		':CDS-RFM_SWITCH_ACTIVE02',
		':CDS-RFM_SWITCH_ACTIVE03',
		':CDS-RFM_SWITCH_ACTIVE11',
		':CDS-RFM_SWITCH_ACTIVE12',
		':CDS-RFM_SWITCH_ACTIVE13',
		':CDS-RFM_SWITCH_ACTIVE21',
		':CDS-RFM_SWITCH_ACTIVE22',
		':CDS-RFM_SWITCH_ACTIVE23',
		':CDS-RFM_SWITCH_ACTIVE31',
		':CDS-RFM_SWITCH_ACTIVE32',
		':CDS-RFM_SWITCH_ACTIVE33',
		':CDS-RFM_SWITCH_STATUS')

for item in chan_list:
	chan_name = args.ifo + item;
	epics_chan.append(PV(chan_name))

while True:
	time.sleep(1)
	with open("/proc/cdsrfm","r") as datafile:
		first_line = datafile.readline()
	word = first_line.split()

	for ii,data in enumerate(word):
		epics_chan[ii].value = data

