#!/usr/bin/env python3



import glob
import sys
import time

from pcaspy import SimpleServer, Driver


class RODriver(Driver):
    def __init__(self):
        super(RODriver, self).__init__()

    def write(self, reason, value):
        return False


def sanitize_name(name):
    def filter_func (x):
        return x if x in safe else '_'

    safe = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-:0123456789'
    return "".join(map(filter_func, name))


def get_child_info(pid):
    info = {}
    file_glob = "/proc/{0}/task/*/stat".format(pid)
    for fname in glob.glob(file_glob):
        with open(fname, 'rt') as f:
            data = f.read()
            parts = data.split()
            name = sanitize_name(parts[1])
            processor = int(parts[38])
            info[name] = processor
    return info


def create_db(info):
    db = {}
    for thread_name in info:
        entry = {
            'value': info[thread_name],
        }
        db[thread_name] = entry
        print(thread_name)
    print(db)
    return db


def main():
    pid = int(sys.argv[1])
    prefix = sys.argv[2]
    print("pid = {0}".format(pid))
    info = get_child_info(pid)
    db = create_db(info)

    server = SimpleServer()
    server.createPV(prefix, db)
    driver = RODriver()
    while True:
        next = time.time() + 0.25
        server.process(0.25)

        while time.time() < next:
            time.sleep(0.01)

        info = get_child_info(pid)
        for thread_name in info:
            if thread_name in db:
                driver.setParam(thread_name, info[thread_name])
        driver.updatePVs()

main()
