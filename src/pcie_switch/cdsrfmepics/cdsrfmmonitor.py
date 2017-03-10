#!/usr/bin/env python
# Automated test support classes

import epics
import time
import string
import sys
import os

from epics import PV

with open("/proc/counter","r") as datafile:
	first_line = datafile.readline()
print first_line
word = first_line.split()
print len(word)

print 'Active Channels on PCIE = ',word[0]
print 'Active Channels on RFM0 = ',word[1]
print 'Active Channels on RFM1 = ',word[2]

cnt1 = PV('X2:CDS-RFM_SWITCH_CHCNT1')
cnt2 = PV('X2:CDS-RFM_SWITCH_CHCNT2')
cnt3 = PV('X2:CDS-RFM_SWITCH_CHCNT3')
cnt4 = PV('X2:CDS-RFM_SWITCH_CHCNT4')
act01 = PV('X2:CDS-RFM_SWITCH_ACTIVE01')
act02 = PV('X2:CDS-RFM_SWITCH_ACTIVE02')
act03 = PV('X2:CDS-RFM_SWITCH_ACTIVE03')
act11 = PV('X2:CDS-RFM_SWITCH_ACTIVE11')
act12 = PV('X2:CDS-RFM_SWITCH_ACTIVE12')
act13 = PV('X2:CDS-RFM_SWITCH_ACTIVE13')

cnt1.value = int(word[0])
cnt2.value = int(word[1])
cnt3.value = int(word[2])
cnt4.value = int(word[3])
act01.value = int(word[4])
act02.value = int(word[5])
act03.value = int(word[6])
act11.value = int(word[7])
act12.value = int(word[8])
act13.value = int(word[9])
