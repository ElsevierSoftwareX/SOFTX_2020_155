#!/usr/bin/env python3


import re
import statistics
import sys


for line in sys.stdin:
    line = line.strip()
    if line == "":
        continue
    entries = line.split('|')
    if len(entries) == 1:
        continue
    data = []
    for entry in entries:
        m = re.match("^.*\(([0-9]+)\).*$", entry)
        if not m is None:
            data.append(int(m.group(1)))
    min = data[0]
    max = data[0]
    for entry in data:
        if entry < min:
            min = entry
        if entry > max:
            max = entry
    print("min is = {0}\nmax is = {1}\ndelta is = {2}".format(min, max, max - min))
    print("mean is = {0}\nstddev is = {1}\n".format(statistics.mean(data), statistics.stdev(data)))