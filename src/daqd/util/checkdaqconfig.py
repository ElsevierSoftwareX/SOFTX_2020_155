#!/usr/bin/python3
# Check that DAQ configuration files such as
# master file, ini file and par files
# are unchanged.  If they are changed,
# create a new archive directory with the new files
# add a master file and point the daq at the new master file
# then log the changes.

import os.path as path
import argparse
import os
import sys
import hashlib
import datetime
import shutil
import fcntl
from string import Template
from dateutil.tz import tzlocal
import time

# set umask write-enable all new files
# this allows controls and advligorts users
# to share files
os.umask(2)

ifo = None
site = None
try:
    ifo_upper = os.environ['IFO']
    site_upper = os.environ['SITE']
    ifo = ifo_upper.lower()
    site = site_upper.lower()
    default_target = f"/opt/rtcds/{site}/{ifo}"
except KeyError:
    default_target = ""

# process args
parser = argparse.ArgumentParser(description="Check the DAQ configuration for changes. If there are any,setup a new \
configuration directory while preserving the old for posterity.")
parser.add_argument('-t', dest="target_path",
                    help="target directory, default: /opt/rtcds/<site>/<ifo>", default=default_target)
parser.add_argument('-a', dest="archive_path",
                    help="path to archive directory, default: $target/target/daq/archive", default="")
parser.add_argument('-m', dest="master_template",
                    help="path to master file template, default: $target/target/daq/master.in", default="")
parser.add_argument('-i', dest="ini_path", help="path to .ini files, default: $target/chans/daq", default="")
parser.add_argument('-p', dest="par_path", help="path to .par files, default: $target/target/gds/param", default="")
parser.add_argument('-b', dest="daq_base_path",
                    help="base path for daq running directories, default: $target", default="")
parser.add_argument('daq_name', help="name of DAQ (daq0, daq1)")

args = parser.parse_args()

target_path = args.target_path

used_target = False

# calculate paths
if args.archive_path != "":
    archive_path = args.archive_path
else:
    archive_path = f"{target_path}/target/daq/archive"
    used_target = True
if args.master_template != "":
    master_template = args.master_template
else:
    master_template = f"{target_path}/target/daq/master.in"
    used_target = True
if args.ini_path != "":
    ini_path = args.ini_path
else:
    ini_path = f"{target_path}/chans/daq"
    used_target = True
if args.par_path != "":
    par_path = args.par_path
else:
    par_path = f"{target_path}/target/gds/param"
    used_target = True
if args.daq_base_path != "":
    daq_base_path = args.daq_base_path
else:
    daq_base_path = target_path

daq_name = args.daq_name

# make sure paths are sensible

if target_path == "" and used_target:
    print("WARNING: target path is empty.", file=sys.stderr)
    print("Either a default path could not be created, or an empty target path was specified.", file=sys.stderr)
    print("Specify a non-empty target directory with the -t option\n")

# Print all paths
print("Target directory:\t" + target_path)
print("DAQ archive:\t\t" + archive_path)
print("Master template:\t" + master_template)
print("INI file Path:\t\t" + ini_path)
print("Parameter file Path:\t" + par_path)

# create archive if it doesn't exist
hash_archives = f"{archive_path}/hash_archive"
os.makedirs(hash_archives, exist_ok=True)

# do they exist?

missing_paths = [p for p in [archive_path, master_template, daq_base_path] if not path.exists(p)]

if missing_paths:
    print("ERROR: could not find " + "\nERROR: could not find ".join(missing_paths), file=sys.stderr)
    sys.exit(2)

if not path.exists(ini_path):
    print(f"WARNING: {ini_path} does not exist.  Paths to .ini file must be hardcoded in master template.")

if not path.exists(par_path):
    print(f"WARNING: {par_path} does not exist.  Paths to .par file must be hardcoded in master template.")

def replace_dummy_path(file_path, default_path):
    """
    Replace a possible dummy path in file_path with default_path.
    Replace the path only if file_path does not exist.
    If default_path/file_path also doesn't exist, throw an exception.

    :param file_path: an absolute path to a file
    :param default_path:
    :return: if file_path exists, return file_poth, or else return default_path/basename(fname)
    """

    if path.exists(file_path):
        return file_path
    new_path = path.join(default_path, path.basename(file_path))
    if path.exists(new_path):
        return new_path
    raise Exception(f"{file_path} could not be found and {new_path} could not be found")

# get paths to all files in master file.
try:
    with open(master_template) as f:
        master_lines = f.readlines()
except IOError as e:
    print(f"Failed to open {master_template}: " + str(e), file=sys.stderr)
    sys.exit(1)

master_paths = [i.strip() for i in master_lines if i.strip()[0] != "#"]
ini_files = [replace_dummy_path(i, ini_path) for i in master_paths if i[-3:] == "ini"]
par_files = [replace_dummy_path(i, par_path) for i in master_paths if i[-3:] == "par"]



for f in par_files:
    print(f)
ini_files.sort()
par_files.sort()

missing_files = [f for f in ini_files + par_files if not path.exists(f)]

if missing_files:
    print("ERROR: could not find " + "\nERROR: could not find ".join(missing_files), file=sys.stderr)
    sys.exit(2)
print("Found all configuration files")

# get hash of files
m = hashlib.md5()

for fname in ini_files + par_files:
    with open(fname, "rb") as f:
        m.update(f.read())

config_hash = m.hexdigest()

hash_file_name = "hash.md5"

print("MD5 Hash of configuration is " + config_hash)

# acquire file lock


# get existing master symlink
def hash_from_archive(archive_name):
    """
    Get the hash from the name of an archive directory

    :param archive_name: the full path to the archive directory.
    :return: the hash, or None if name cannot be found
    """
    return path.basename(archive_name)


def check_path_vs_hash(archive, hex_hash):
    """
    Check a path to see if the hash it contains matches a given hash.
    :param archive: The path to check against.
    :param hex_hash: The hash to compare to in hex format.
    :return: True if the hashes match.
    """
    return hash_from_archive(archive) == hex_hash


def check_link_vs_hash(link, hex_hash):
    """
    Check if a given hash matches the real path pointed to by the symbolic link.

    :param link: a path of a symbolic link
    :param hex_hash: a hash value in hexidecimal format
    :return: True if the hash in the link's real path matches hash
    """
    if not path.exists(link):
        print("link is broken: target does not exist.")
        return False

    realpath = path.realpath(link)

    return check_path_vs_hash(realpath, hex_hash)


def update_daq_archives(arch_path):
    global archive_path, daq_name
    daq_archives = f"{archive_path}/{daq_name}"
    os.makedirs(daq_archives, exist_ok=True)
    count = 0
    while True:
        new_link_name = datetime.datetime.now(tzlocal()).strftime("%d%m%y_%H:%M:%S")
        new_link_path = f"{daq_archives}/{new_link_name}"
        if not path.lexists(new_link_path):
            break
        time.sleep(1)
        count = count + 1
        if count == 5:
            raise Exception("Could not find a daq archive name that didn't already exist")
    print(f"Updating archive dir for {daq_name} with link {new_link_name}")
    os.symlink(arch_path, new_link_path)


logfile = f"{archive_path}/daqconfig.log"
lockfile = f"{archive_path}/archive.lock"
master_link_path = f"{daq_base_path}/{daq_name}"
master_link = f"{master_link_path}/running"
os.makedirs(master_link_path, exist_ok=True)
if path.lexists(master_link):
    if path.islink(master_link):
        print("configuration link found")
        if check_link_vs_hash(master_link, config_hash):
            # link already good
            print("Hash unchanged from current configuration.")
            update_daq_archives(path.realpath(master_link))
            sys.exit(0)
        else:
            print("Bad link or hash mismatched: current link will be replaced")
    else:  # error: we won't replace a file that's not a link.
        print(f"ERROR: configuration file {master_link} exists but is not a symbolic link.  Refusing to replace.")
        sys.exit(3)
else:
    # if link doesn't exist, just continue on. We'll make it later.
    print("no configuration link found")

print("Checking for other matches in archives")


def create_link(arch_path, link_path):
    """
    Create a link to the correct archive directory.

    :param arch_path: path to archive directory we want to link to
    :param link_path:  Path to where the link file should be created
    :return: None
    """
    if path.lexists(link_path):
        print("Removing old link")
        os.remove(link_path)
    os.symlink(arch_path, link_path)
    print(f"Link created from {link_path} to {arch_path}")


def log_change(lf_name, daq, archive, hex_hash):
    """
    Create a log of a change in config
    :param lf_name: path to log file
    :param daq: name of daq that is changing
    :param archive: path to archive directory
    :param hex_hash: hash of config
    :return: None
    """
    with open(lf_name, "a+") as lf:
        lf.write("%s,%s,%s, %s\n" % (datetime.datetime.now(tzlocal()).strftime("%Y-%m-%d %H:%M:%S%z"),
                                     daq, archive, hex_hash))


with open(lockfile, "w") as lock_f:
    fcntl.flock(lock_f, 2)  # exclusive lock, "LOCK_EX")

    # get existing archives
    matching_archives = [f for f in os.listdir(hash_archives)
                         if check_path_vs_hash(path.join(archive_path, f), config_hash)]
    if len(matching_archives) > 0:
        matching_archive = matching_archives[0]
        print("Another directory was found in the archives that matches: " + matching_archive)
        matching_path = f"{hash_archives}/{matching_archive}"
        create_link(matching_path, master_link)
        update_daq_archives(matching_path)
        log_change(logfile, daq_name, matching_archive, config_hash)
        sys.exit(0)

    print("creating new archive")

    # create directory

    # Use a format inherited from older scripts for directory names
    new_archive_basename = config_hash
    new_archive = f"{hash_archives}/{new_archive_basename}"
    new_master = f"{new_archive}/master"
    new_hash_file = f"{hash_archives}/{new_archive_basename}/{hash_file_name}"
    os.makedirs(new_archive)

    # copy in config files
    print("copy in config files")
    master_entries = []
    for f in ini_files + par_files:
        fb = path.basename(f)
        dest = f"{new_archive}/{fb}"
        shutil.copyfile(f, dest)
        master_entries.append(dest)
    print("copy complete")

    # create master file
    with open(new_master, "w") as mf:
        mf.write("\n".join(master_entries)+"\n")

    # link to master
    create_link(new_archive, master_link)
    update_daq_archives(new_archive)
    log_change(logfile, daq_name, new_archive_basename, config_hash)
