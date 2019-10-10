#!/usr/bin/env python3

from __future__ import print_function

import threading

import pcaspy

def write_ini_file(prefix, db, fname, datatypes):
    print("Writing ini file '{0}'".format(fname))
    with open(fname, 'wt') as f:
        f.write("""[default]
gain=1.0
acquire=3
dcuid=12
ifoid=0
datatype=4
datarate=16
offset=0
slope=1.0
units=undef
""")
        i = 0

        for entry in ['EDCU_CHAN_CONN', 'EDCU_CHAN_CNT',]:
            f.write("""[{0}{1}]
datarate=16
datatype=4
chnnum={2}
""".format(prefix, entry, 40000 + i))
            i = i + 1

        f.write("""[{0}{1}]
datarate=16
datatype=2
chnnum={2}
""".format(prefix, 'EDCU_CHAN_NOCON', 40000 + i))
        i = i + 1

        keys = list(db.keys())
        keys.sort()
        for entry in keys:
            f.write("""[{0}{1}]
datarate=16
datatype={3}
chnnum={2}
""".format(prefix, entry, 40000 + i, datatypes[entry]))
            i += 1


def read_time():
    with open("/proc/gps", "rt") as f:
        data = f.read()
        dot = data.find('.')
        return int(data[0:dot])

class myDriver(pcaspy.Driver):
    def __init__(self, offsets, datatypes):
        super(myDriver, self).__init__()
        self.__params = {}
        for entry in offsets.keys():
            self.__params[entry] = (offsets[entry], datatypes[entry])
        self.__lock = threading.Lock()

    def read(self, reason):
        with self.__lock:
            return self.getParam(reason)

    def write(self, reason, val):
        return False

    def update_vals(self, ref_time):
        with self.__lock:
            for entry in self.__params:
                if self.__params[entry][1] == 1:
                    val = (ref_time % 30000) + self.__params[entry][0]
                else:
                    val = (ref_time % 100000) + self.__params[entry][0]
                #if entry == "EDC-189--gpssmd100koff1p--24--2--16":
                #    print("X6:EDC-189--gpssmd100koff1p--24--2--16  == {0} offset of {1}".format(val, self.__offsets[entry]))
                self.setParam(entry, val)
            self.updatePVs()

prefix="X6:"
db = {}
offsets = {}
datatypes = {}

def add_entry(db, i, offset):
    global offsets
    global datatypes

    # 5 = double
    # 4 = float
    # 2 = 32 bit int
    # 1 = 16 bit int

    mod4 = i % 4
    if mod4 == 0:
        name="EDC-{0}--gpssmd100koff1p--{1}--5--16".format(i, offset)
        db[name] = {'type': 'float' }
        datatypes[name] = 5
    elif mod4 == 1:
        name="EDC-{0}--gpssmd100koff1p--{1}--4--16".format(i, offset)
        db[name] = {'type': 'float' }
        datatypes[name] = 4
    elif mod4 == 2:
        name="EDC-{0}--gpssmd100koff1p--{1}--2--16".format(i, offset)
        db[name] = {'type': 'int' }
        datatypes[name] = 2
    elif mod4 == 3:
        name="EDC-{0}--gpssmd30koff1p--{1}--1--16".format(i, offset)
        db[name] = {'type': 'int' }
        datatypes[name] = 1
    db[name] = {'type': 'int' }
    offsets[name] = offset
    return db

for i in range(2000):
    db = add_entry(db, i, i%33)

write_ini_file(prefix, db, 'edcu.ini', datatypes)

server = pcaspy.SimpleServer()
server.createPV(prefix, db)
driver = myDriver(offsets, datatypes)
driver.update_vals(read_time())

last_sec = read_time()
while True:
    server.process(0.2)
    cur_sec = read_time()
    if cur_sec != last_sec:
        driver.update_vals(cur_sec)
        last_sec = cur_sec
