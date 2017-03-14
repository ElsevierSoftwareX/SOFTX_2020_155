#!/usr/bin/env python
# Automated test support classes

import epics
import time
import string
import sys
import os

from epics import PV

cnt1 = PV('X2:CDS-RFM_SWITCH_CHCNT1')
cnt2 = PV('X2:CDS-RFM_SWITCH_CHCNT2')
cnt3 = PV('X2:CDS-RFM_SWITCH_CHCNT3')
cnt4 = PV('X2:CDS-RFM_SWITCH_CHCNT4')
swmonitortime = PV('X2:CDS-RFM_SWITCH_TIME')
act01 = PV('X2:CDS-RFM_SWITCH_ACTIVE01')
act02 = PV('X2:CDS-RFM_SWITCH_ACTIVE02')
act03 = PV('X2:CDS-RFM_SWITCH_ACTIVE03')
act11 = PV('X2:CDS-RFM_SWITCH_ACTIVE11')
act12 = PV('X2:CDS-RFM_SWITCH_ACTIVE12')
act13 = PV('X2:CDS-RFM_SWITCH_ACTIVE13')
act21 = PV('X2:CDS-RFM_SWITCH_ACTIVE21')
act22 = PV('X2:CDS-RFM_SWITCH_ACTIVE22')
act23 = PV('X2:CDS-RFM_SWITCH_ACTIVE23')
act31 = PV('X2:CDS-RFM_SWITCH_ACTIVE31')
act32 = PV('X2:CDS-RFM_SWITCH_ACTIVE32')
act33 = PV('X2:CDS-RFM_SWITCH_ACTIVE33')
swstat = PV('X2:CDS-RFM_SWITCH_STATUS')

while True:
	time.sleep(1)
	with open("/proc/counter","r") as datafile:
		first_line = datafile.readline()
	word = first_line.split()


	cnt1.value = int(word[0])
	cnt2.value = int(word[1])
	cnt3.value = int(word[2])
	cnt4.value = int(word[3])
	swmonitortime.value = int(word[4])
	act01.value = int(word[5])
	act02.value = int(word[6])
	act03.value = int(word[7])
	act11.value = int(word[8])
	act12.value = int(word[9])
	act13.value = int(word[10])
	act21.value = int(word[11])
	act22.value = int(word[12])
	act23.value = int(word[13])
	act31.value = int(word[14])
	act32.value = int(word[15])
	act33.value = int(word[16])
	swstat.value = int(word[17])
