#!/usr/bin/env python3

import threading

import pcaspy

def write_ini_file(prefix, db, fname):
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
        keys = list(db.keys())
        keys.sort()
        i = 0
        for entry in keys:
            f.write("""[{0}{1}]
datarate=16
datatype=4
chnnum={2}
""".format(prefix, entry, 40000 + i))
            i += 1
        for entry in ['EDCU_CHAN_CONN', 'EDCU_CHAN_CNT', 'EDCU_CHAN_NOCON']:
            f.write("""[{0}{1}]
datarate=16
datatype=4
chnnum={2}
""".format(prefix, entry, 40000 + i))


def read_time():
    with open("/proc/gps", "rt") as f:
        data = f.read()
        dot = data.find('.')
        return int(data[0:dot])

class myDriver(pcaspy.Driver):
    def __init__(self, offsets):
        super(myDriver, self).__init__()
        self.__offsets = offsets
        self.__lock = threading.Lock()

    def read(self, reason):
        with self.__lock:
            return self.getParam(reason)

    def write(self, reason, val):
        return False

    def update_vals(self, ref_time):
        with self.__lock:
            for entry in self.__offsets:
                val = (ref_time % 100000) + self.__offsets[entry]
                #if entry == "EDC-189--gpssmd100koff1p--24--2--16":
                #    print("X6:EDC-189--gpssmd100koff1p--24--2--16  == {0} offset of {1}".format(val, self.__offsets[entry]))
                self.setParam(entry, val)
            self.updatePVs()

prefix="X6:"
db = {}
offsets = {}

def add_entry(db, i, offset):
    global offsets
    name="EDC-{0}--gpssmd100koff1p--{1}--4--16".format(i, offset)
    db[name] = {'type': 'int' }
    offsets[name] = offset
    return db

for i in range(2000):
    db = add_entry(db, i, i%33)

write_ini_file(prefix, db, 'edcu.ini')

server = pcaspy.SimpleServer()
server.createPV(prefix, db)
driver = myDriver(offsets)
driver.update_vals(read_time())

last_sec = read_time()
while True:
    server.process(0.2)
    cur_sec = read_time()
    if cur_sec != last_sec:
        driver.update_vals(cur_sec)
        last_sec = cur_sec
