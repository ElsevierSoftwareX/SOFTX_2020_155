#!/usr/bin/env python

from __future__ import print_function

import json
import os
import select
import struct
import sys

from pcaspy import SimpleServer, Driver

class RODriver(Driver):
    def __init__(self):
        super(RODriver, self).__init__()

    def write(self, reason, value):
        return False

class State(object):
    def __init__(self):
        self.buf = ""
        self.messages = []
        self.msg_border = b'\xff\xff\xff\xff'
        self.min_header_len = len(self.msg_border) + struct.calcsize("@q")
        self.epics = None

    def injest_data(self, f):
        (rlist, wlist, xlist) = select.select([f,], [], [], 0.05)
        if len(rlist) == 0:
            return
        self.buf += os.read(f, 1024)

    def process_single_message(self):
        # no point in looking for a message if there is not enough data
        # to do the header
        if len(self.buf) < self.min_header_len:
            return False
        # look for the message boundary
        offset = self.buf.find(self.msg_border)
        if offset < 0:
            # we are mid message, get rid of all the data except for len(self.msg_border)
            self.buf = self.buf[:-len(self.msg_border)]
            return False
        elif offset > 0:
            self.buf = self.buf[offset:]
        # we are pretty sure at this point that the header starts at self.buf[0]
        msg_len = struct.unpack("@q", self.buf[4:12])[0]
        if msg_len > 1024*1024:
            # assume it is junk if the length is > 1MB, push past the message border and try again next time
            self.buf = self.buf[4:]
            return False
        if len(self.buf) >= msg_len + self.min_header_len:
            # we have a message
            try:
                self.messages.append(json.loads(self.buf[self.min_header_len:self.min_header_len + msg_len]))
            except:
                # malformed message, skip the header and move on
                self.buf = self.buf[4:]
                return False
            self.buf = self.buf[self.min_header_len + msg_len:]
            return True
        return False

    def process_messages(self):
        while self.process_single_message():
            pass

    def reset_epics_if_needed(self):
        if self.epics is None:
            return
        if len(self.messages) == 0:
            return

        msg = self.messages[-1]
        expected_pvs = set(self.epics['db'].keys())
        received_pvs = set([pv['name'] for pv in msg['pvs']])
        if expected_pvs != received_pvs:
            del self.epics['srv']
            self.epics = None

    def initialize_epics_if_needed(self):
        if not self.epics is None:
            return
        if len(self.messages) == 0:
            return
        # if there is a back log of messages, get rid of them
        msg = self.messages[-1]
        self.messages = []

        db = {}
        prefix = msg['prefix']
        for pv in msg['pvs']:
            # {u'name': u'RECV_MIN_MS', u'warn_high': 70, u'value': 56, u'alarm_high': 80, u'alarm_low': 45, u'warn_low': 54}
            pv_name = pv['name']
            if pv['pv_type'] == 0:
                entry = {
                    'hihi': pv['alarm_high'],
                    'hihi': pv['alarm_high'],
                    'high': pv['warn_high'],
                    'lolo': pv['alarm_low'],
                    'low': pv['warn_low'],
                    'value': pv['value'],
                }
            elif pv['pv_type'] == 1:
                entry = {
                    'value': pv['value'],
                    'type': 'string',
                }
            else:
                continue
            db[pv_name] = entry
        self.epics = { 'srv': SimpleServer(), 'db': db, 'prefix': prefix }
        self.epics['srv'].createPV(prefix, self.epics['db'])
        self.epics['drv'] = RODriver()
        print("Starting EPICS")
        print(self.epics)


    def epics_loop(self):
        if self.epics is None:
            return
        if len(self.messages) > 0:
            msg = self.messages[-1]
            drv = self.epics['drv']
            for pv in msg['pvs']:
                # print("Received message: {0}".format(msg))
                if pv['name'] in self.epics['db']:
                    drv.setParam(pv['name'], pv['value'])
            drv.updatePVs()
            self.messages = []
        self.epics['srv'].process(0.1)


def main():
    pipe_name = sys.argv[1]

    fd = os.open(pipe_name, os.O_RDONLY | os.O_NONBLOCK)
    try:
        state = State()
        while True:
            state.injest_data(fd)
            state.process_messages()
            state.reset_epics_if_needed()
            state.initialize_epics_if_needed()
            state.epics_loop()
    finally:
        os.close(fd)
main()