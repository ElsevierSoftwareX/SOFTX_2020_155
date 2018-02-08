#!/usr/bin/env python

from __future__ import print_function
import glob
import os
import os.path
import sys

def usage():
    print("Manage a test master file")
    print("Options:")
    print(" delete <directory> - delete all .ini files and .par and master files in the current directory")
    print(" create <directory> - create a master file in the given directory that list all the .ini and .par files in that directory")
    sys.exit(1)

def create_master(master_dir):
    file_list = []
    for ftype in ['*.ini', '*.par']:
        files = glob.glob(os.path.join(master_dir, ftype))
        file_list.extend(files)
    f = open(os.path.join(master_dir, 'master'), 'wt')
    for line in file_list:
        # if line.endswith('.par'):
        #     f.write('# ')
        f.write(line + '\n')
    f.close()

def delete_master(master_dir):
    for ftype in ['*.ini', '*.par']:
        files = glob.glob(os.path.join(master_dir, ftype))
        for fname in files:
            os.unlink(fname)
            print("Removed {0}".format(fname))
    fname = os.path.join(master_dir, 'master')
    if os.path.exists(fname):
        os.unlink(fname)
        print("Removed {0}".format(fname))

command = None
master_dir = None

if len(sys.argv) == 3:
    master_dir = sys.argv[2]
    command = sys.argv[1]

if command not in set(['create', 'delete']) or master_dir is None or not os.path.isdir(master_dir):
    usage()

if command == "create":
    create_master(master_dir)
elif command == "delete":
    delete_master(master_dir)
else:
    usage()