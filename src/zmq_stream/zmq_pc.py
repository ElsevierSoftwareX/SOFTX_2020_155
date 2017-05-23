import sys
import zmq
import epics
from epics import PV

epics_chan = []
chan_list = (
		'X1:DAQ-DC_GPS_SEC',
		'X1:DAQ-DC_DCUID_0',
		'X1:DAQ-DC_STATUS_0',
		'X1:DAQ-DC_DBS_0',
		'X1:DAQ-DC_DCUID_1',
		'X1:DAQ-DC_STATUS_1',
		'X1:DAQ-DC_DBS_1',
		'X1:DAQ-DC_DCUID_2',
		'X1:DAQ-DC_STATUS_2',
		'X1:DAQ-DC_DBS_2',
		'X1:DAQ-DC_DCUID_3',
		'X1:DAQ-DC_STATUS_3',
		'X1:DAQ-DC_DBS_3')

for item in chan_list:
	epics_chan.append(PV(item))

context = zmq.Context()
socket = context.socket(zmq.SUB)
socket.connect("tcp://scipe19_daq:7777")

zip_filter = "X1:ATS-TPORNOTTP_TRAMP"

socket.setsockopt(zmq.SUBSCRIBE, "")

# for update_nbr in range(5):
while True:
	string = socket.recv()
	word = string.split()
	for ii in range(13):
		if ii != 3 and ii !=6 and ii != 9 and ii != 12:
			epics_chan[ii].value = int(word[ii]) 
		else:
			epics_chan[ii].value = int(word[ii]) * 16 / 1000
