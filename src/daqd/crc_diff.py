#!/usr/bin/env python

import argparse
import sys


def get_line(f):
    expected = ['crc', 'gps', 'gps_n', 'seq', 'time']
    txt = f.readline()
    txt = txt.strip()
    obj = {}
    parts = txt.split(' ')
    for pair in parts:
        components = pair.split('=', 1)
        if len(components) == 1:
            continue
        if components[0] in expected:
            obj[components[0]] = components[1]
    for field in expected:
        if field not in obj.keys():
            return None
    obj['gps'] = int(obj['gps'])
    obj['gps_n'] = int(obj['gps_n'])
    obj['seq'] = int(obj['seq'])
    obj['time'] = int(obj['time'])
    return obj


def get_entry(f):
    entry = None
    while entry is None:
        entry = get_line(f)
    return entry


def create_key(gps, gps_n):
    return gps * 10000000000 + gps_n


def entry_key(entry):
    return create_key(entry['gps'], entry['gps_n'])


def get_next_entry(f, start, start_n):
    key = create_key(start, start_n)
    entry = get_entry(f)
    while entry_key(entry) < key:
        entry = get_entry(f)
    return entry


def do_compare(f1, f2, start, end):
    cur = start
    cur_n = 0
    e1 = {'gps': -1, 'gps_n': 0}
    e2 = {'gps': -1, 'gps_n': 0}

    while cur < end:
        # print "cycle %d %d" % (cur, cur_n)
        key = create_key(cur, cur_n)
        e1_key = entry_key(e1)
        e2_key = entry_key(e2)

        if e1_key < key:
            e1 = get_next_entry(f1, cur, cur_n)
            # print "%s" % str(entry_key(e1))
        if e2_key < key:
            e2 = get_next_entry(f2, cur, cur_n)
            # print "%s" % str(entry_key(e2))

        if e1['gps'] == e2['gps'] and e1['gps_n'] == e2['gps_n']:
            if e1['crc'] != e2['crc'] or e1['seq'] != e2['seq']:
                args = (e1['gps'], e1['gps_n'], e1['crc'], e2['crc'])
                print "MISMATCH in CRC at %d.%d %s != %s\n" % args
            else:
                # print "match at %d.%d" % (e1['gps'], e1['gps_n'])
                pass
            cur = e1['gps']
            cur_n = e1['gps_n'] + 1
        else:
            if entry_key(e1) < entry_key(e2):
                cur = e2['gps']
                cur_n = e2['gps_n']
            else:
                cur = e1['gps']
                cur_n = e1['gps_n']


def main():
    help_txt = """Read in two network crc files from daqd and compare the
crc values per 1/16s segment.

The scirpt only prints differences where both files have values at that time.

The time range works [start, end).

The files have entries of the form:
crc=<crc in hex> gps=<time> gps_n=<nanosecs> time=<unix time>
"""
    parser = argparse.ArgumentParser(description=help_txt)
    parser.add_argument("file1", help="input file 1")
    parser.add_argument("file2", help="input file 2")

    parser.add_argument("-s", "--start-gps", type=int,
                        help="Beginning GPS second to" +
                        " start comparing entries, defaults to 0",
                        dest="start", default=0)
    parser.add_argument("-e", "--end-gps", type=int,
                        help="Beginning GPS second to" +
                        " stop comparing entries, defaults to " +
                        str(sys.maxint),
                        dest="end", default=sys.maxint)

    args = parser.parse_args()
    with open(args.file1, 'rt') as f1:
        with open(args.file2, 'rt') as f2:
            do_compare(f1, f2, args.start, args.end)

if __name__ == "__main__":
    main()
