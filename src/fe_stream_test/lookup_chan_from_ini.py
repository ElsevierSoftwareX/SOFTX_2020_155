#!/usr/bin/env python3

import argparse
import json
import sys


class channel_info(object):
    def __init__(self, name=None, dcu=-1, datarate=0, datatype=0, offset=0):
        self.name = name
        self.dcu = dcu
        self.datarate = datarate
        self.datatype = datatype
        self.offset = offset

    def copy(self):
        return channel_info(name=self.name, dcu=self.dcu, datarate=self.datarate, datatype=self.datatype, offset=self.offset)

    def size(self):
        return datatype_size(self.datatype) * (self.datarate // 16)

    def __unicode__(self):
        data = {
            'name': self.name,
            'dcu': self.dcu,
            'datarate': self.datarate,
            'datatype': self.datatype,
            'datatypename': datatypename(self.datatype),
            'datatypesize': datatype_size(self.datatype),
            'size': self.size(),
            'offset': self.offset,
        }
        return json.dumps(data)

    def __str__(self):
        return self.__unicode__()


def datatypename(data_type):
    mapping = {
        1: 'int16',
        2: 'int32',
        3: 'int64',
        4: 'float32',
        5: 'float64',
        6: 'complex',
        7: 'uint32',
    }
    return mapping[data_type]


def datatype_size(data_type):
    if data_type == 1:
        return 2
    elif data_type in (2,4,7):
        return 4
    elif data_type in (3, 5, 6):
        return 8
    raise RuntimeError("Invalid datatype {0}".format(data_type))


print(sys.argv)


parser = argparse.ArgumentParser(description="Lookup channel information from an ini file")
parser.add_argument("-i", "--ini", help="Specify the ini file")
parser.add_argument("-c", "--channel", help="Specify the channel")

args = parser.parse_args()

channels = []

default_settings = channel_info()

with open(args.ini, "rt") as f:
    cur_info = default_settings.copy()
    cur_offset = 0

    for line in f:
        line = line.strip()
        if line == "":
            continue
        if line.startswith("["):
            if not line.endswith("]"):
                raise RuntimeError("Unparsable line {0}".format(line))

            # first store the old entry
            if cur_info.name == 'default':
                default_settings = cur_info
                default_settings.offset = 0
            elif cur_info.name is not None:
                channels.append(cur_info)
                cur_offset += cur_info.size()

            cur_info = default_settings.copy()
            cur_info.name = line[1:-1]
            cur_info.offset = cur_offset
        else:
            parts = line.split("=")
            if len(parts) != 2:
                continue
            if parts[0] == "dcuid":
                cur_info.dcu = int(parts[1])
            elif parts[0] == "datatype":
                cur_info.datatype = int(parts[1])
            elif parts[0] == "datarate":
                cur_info.datarate = int(parts[1])

    if cur_info.name is not None:
        channels.append("{0}".format(cur_info))

for entry in channels:
    print(entry)
