#!/usr/bin/env python3

"""
DAQ frame wiper python script.

Keeps the dynamic part of the frames file system at a certain ammount
full by selectively deleting old frame files (full, second and minute frame files).

It is imperative that there is always sufficent free disk space for the frame writer to
write into. If there is not, then the frame writer will crash and no new gravitational
wave data will be written. In other words, new data will be lost while old
archived data will be retained.

This program is designed to be ran every hour on the Solaris QFS-SAMFS writer.
On non main-line production systems (e.g. test stands, auxillary production writers)
this program could be ran on the frame writer itself.

The LHO frame writer's QFS disk system can be broken into two sections: archived data and
dynamic data. The archived data comprised of old raw minute trend files offloaded from the
trend writer and a small amount of other files.
The dynamic section is where the frame writer writes the frame files (full, second-trend,
minute-trends). This wiper script keeps this section close to full (but never full)
by selectively deleting old files after LDAS has been given sufficient time to archive the
data off of this disk system.

Here is a schematic of the disk layout:

 -----------------------------------------------------------------------------------------
|                               D I S K     S Y S T E M                                   |
|-----------------------------------------------------------------------------------------|
|                               D Y N A M I C                   |    A R C H I V E D      |
|---------------------------------------------------------------|                         |
|       F U L L       | SECOND-TREND | MINUTE-TREND | OVERHEAD  |                         |
|---------------------|--------------|--------------|-----------|                         |
|    GPS-DIRS         |  GPS-DIRS    |   GPS-DIRS   |           |                         |
|---------------------|--------------|--------------|           |                         |
| FILE/64sec          | FILE/10min   | FILE/hour    |           |                         |
 -----------------------------------------------------------------------------------------

FULL FILE DIRECTORY (frames/full/):

    Contains GPS named directories with first 5 digits in GPS time.
    Each directory contains 64S full gwf frame file and the associated MD5 checksum file

    e.g. H-H1_R-1174257088-64.gwf and H-H1_R-1174257088-64.md5

    directory contents span 100,000 seconds (27hrs46mins) and holds 1562 gwf files and 1562 md5 files

SECOND TRENDS FILE DIRECTORY (frames/trend/second/):

    Contains GPS named directories with first 5 digits in GPS time.
    Each directory contains 600S second trend gwf frame files and associated MD5 checksum file

    e.g. H-H1_T-1174199400-600.gwf and H-H1_T-1174199400-600.md5

    directory contents span 100,000 seconds (27hrs46mins) and holds 166 gwf files and 166 md5 files

MINUTE TRENDS FILE DIRECTORY (frames/trend/minute/):

    Contains GPS named directories with first 5 digits in GPS time.
    Each directory contains 3600S minute trend gwf frame files and associated MD5 checksum file

    e.g. H-H1_M-1174096800-3600.gwf and H-H1_M-1174096800-3600.md5

    directory contents span 100,000 seconds (27hrs46mins) and holds 28 gwf files and 28 md5 files

OVERHEAD:

    The overhead area allows the dynamic section to grow between executions of this code. It
    also allows the archive area to grow (for example when offloading raw minute trends).

Look back:

    To maintain an almost full dynamic section, oldest files are deleted. When deleting the
    oldest file, the data look back for that type of data is of course reduced.
    Which files to delete is governed by the following rules:

        minute trends: minimum look back of 3 days (to permit LDAS to catch up after a weekend)

        second trends: about one month look back to support commissioning

        full frames: the rest of the available disk, hopefully no less that one week look back

    Disk Space Recovery To Maintain Healthy Overhead:

        The oldest GWF frame files are deleted according to the rules given above.

        The deletion sequence is:

        1. minute trends going back further than the minimum are deleted. If more space is needed,
           step 2 is ran.

        2. The oldest one hour of full and second trend data is deleted.
           If more space is needed, then each oldest hour of data is deleted util
           enough space is freed.

        If during a frame file deletion a GPS directory contains no GWF files (only contains MD5 files),
        the MD5 files are tarred into a compressed tar file (e.g. 11715_full_md5_checksums.tar.gz,
        11715_second_md5_checksums.tar.gz, 11715_minute_md5_checksums.tar.gz).
        At this point the GPS directory contains this one file and can be considered archival.
        The directory is renamed from 12345 to 12345_md5_archive to show it contains no framed data.
        Any orphaned dot-frame files (beginning with dot) in non-current directories are deleted.
23mar2017 LHO D.Barker

PEP8 and python3 compliant
23jan2020 LHO D.Barker
"""
import os
import argparse
from datetime import datetime

# static variables
TERRABYTES = 1000000000000
ONE_HOUR = 1
FULL_FRAME_FILES = 1
SECOND_TREND_FILES = 2
MINUTE_TREND_FILES = 3

FULLFRAMEPATH = ""
SECONDTRENDFRAMEPATH = ""
MINUTETRENDFRAMEPATH = ""

NUM_FULL_FRAME_FILES_PER_HOUR = 56  # one minute trend file every hour
NUM_MINUTE_TREND_FILES_PER_HOUR = 1  # one minute trend file every hour
NUM_SECOND_TREND_FILES_PER_HOUR = 6  # one second trend file every 10 minutes
NUM_MINUTE_TREND_FILES_PER_DAY = 24 * NUM_MINUTE_TREND_FILES_PER_HOUR  # one minute trend file every hour
NUM_SECOND_TREND_FILES_PER_DAY = 24 * NUM_SECOND_TREND_FILES_PER_HOUR  # one second trend file every 10 minutes
MIN_NUM_MINUTE_TREND_FILES = 3 * NUM_MINUTE_TREND_FILES_PER_DAY  # keep at least 3 days of minute trend files
MIN_NUM_SECOND_TREND_FILES = 30 * NUM_SECOND_TREND_FILES_PER_DAY  # keep at least 30 days of second trend files

# DB REQUIREDFREEBYTES = 2 * TERRABYTES # maintain at least this amount of free space
REQUIREDFREEBYTES = int(0.25 * TERRABYTES)  # maintain at least this amount of free space
#  TODO: calculate this figure on the fly to represent one day of data.


# -----------------------------------------------------------------------------------------------
# FIND_OLDEST_FILES function.
#
def find_oldest_files(hours, file_type):
    global INDENT
    framefiles = []
    counter = 0
    num_files = 0
    walk_path = ""
    if file_type == FULL_FRAME_FILES:
        file_type_string = "full frame"
        print(INDENT + "finding oldest {} hour(s) of full frame files...".format(hours))
        print(INDENT + "full frame core directory is {}".format(FULLFRAMEPATH))
        walk_path = FULLFRAMEPATH
        num_files = NUM_FULL_FRAME_FILES_PER_HOUR * hours

    elif file_type == SECOND_TREND_FILES:
        file_type_string = "second trend files"
        print(INDENT + "finding oldest {} hour(s) of second trend files...".format(hours))
        print(INDENT + "second trend core directory is {}".format(SECONDTRENDFRAMEPATH))
        walk_path = SECONDTRENDFRAMEPATH
        num_files = NUM_SECOND_TREND_FILES_PER_HOUR * hours

    elif file_type == MINUTE_TREND_FILES:
        file_type_string = "minute trend files"
        print(INDENT + "finding oldest {} hour(s) of minute trend files...".format(hours))
        print(INDENT + "minute trend core directory is {}".format(MINUTETRENDFRAMEPATH))
        walk_path = MINUTETRENDFRAMEPATH
        num_files = NUM_MINUTE_TREND_FILES_PER_HOUR * hours
    else:
        print(INDENT + "Error: unknown frame file type %d".format(file_type))
        exit()

    for root, dirs, files in os.walk(walk_path):
        # split root path into components
        components = root.split("/")
        last = str(components[-1])
        if last.isdigit():
            #  print "directory is a GPS epoch %s"%last
            for name in files:
                if name.endswith(".gwf"):
                    if name.startswith("."):
                        print(INDENT + "warning, dot file {}".format(name))
                    else:
                        # print "appending file %s"%name
                        framefiles.append(name)
                        counter += 1

    print(INDENT + "found {} {} gwf files in total".format(counter, file_type_string))
    # numeric sort
    framefiles = sorted(framefiles)

    print(INDENT + "oldest {} {}".format(num_files, file_type_string))
    # oldest_files = framefiles[0:num_files]
    # print oldest_files

    return framefiles[0:num_files]


def indent_increase():
    global INDENT
    INDENT = INDENT + '\t'


def indent_decrease():
    global INDENT
    INDENT = INDENT[1:]


def full_path(frame_file_name):
    """ Expand frame_file_name into its full path
    on the file system
    """
    # extract GPS time out of file name
    split_string = frame_file_name.replace('-', ' ')
    digit_list = [int(i) for i in split_string.split() if i.isdigit()]
    gps_time = digit_list[0]
    # directory is named by first 5 digits
    gps_epoch = str(gps_time)[0:5]
    if "_R-" in frame_file_name:
        # Raw frame file
        return FULLFRAMEPATH + '/' + gps_epoch + '/' + frame_file_name
    elif "_T-" in frame_file_name:
        # Second trend frame file
        return SECONDTRENDFRAMEPATH + '/' + gps_epoch + '/' + frame_file_name
    elif "_M-" in frame_file_name:
        # Minute trend frame file
        return MINUTETRENDFRAMEPATH + '/' + gps_epoch + '/' + frame_file_name
    else:
        print("Error: unknown frame file type")
        print("File: {}".format(frame_file_name))
        exit(-1)


if __name__ == "__main__":

    global INDENT
    INDENT = '\t'

    # Define and Parse arguments
    parser = argparse.ArgumentParser(description="daq_wiper: maintain frame files disk space",
                                     epilog="D.Barker LHO March 2017")
    parser.add_argument("-d", "--delete", action="store_true",
                        help="delete files to free disk space (in dry-run mode report what would have been deleted)")
    parser.add_argument("-v", "--verbose", action="store_true", help="turn on verbose output")
    parser.add_argument("framepath", help="path to frames directory, e.g. /cds-h1-frames/frames")

    args = parser.parse_args()

    DELETE_FILES = False
    if args.delete is True:
        DELETE_FILES = True
    VERBOSE = args.verbose
    FRAMEPATH = str(args.framepath)
    if not FRAMEPATH.endswith('/'):
        FRAMEPATH += "/"
    FULLFRAMEPATH = FRAMEPATH + "full"
    SECONDTRENDFRAMEPATH = FRAMEPATH + "trend/second"
    MINUTETRENDFRAMEPATH = FRAMEPATH + "trend/minute"

    # print program start information
    print(INDENT + " ")
    print(INDENT + "daq_wiper running...")
    print(INDENT + "Start time {}".format(str(datetime.now())))
    print(INDENT + " ")

    indent_increase()

    # print summary of arguments given
    if DELETE_FILES is True:
        print(INDENT + "RUNNING IN DELETION MODE")
        print(INDENT + "Oldest files will be deleted and orphaned md5 files will be tarred")
    else:
        print(INDENT + "Running in dry-run mode, files will not be deleted")
    if VERBOSE is True:
        print(INDENT + "Verbose output will be generated")
    print(INDENT + "Full path to frames directory is %s".format(FRAMEPATH))

    # Determine how much, if any, disk space needs to be freed
    # note distiction between 'free' and 'avail', free it total not used but not all is
    # available to the user, avail is the actual space we can use
    statvfs = os.statvfs(FRAMEPATH)
    disk_system_size_bytes = statvfs.f_frsize * statvfs.f_blocks
    disk_system_free_bytes = statvfs.f_frsize * statvfs.f_bfree
    disk_system_avail_bytes = statvfs.f_frsize * statvfs.f_bavail
    disk_system_used_bytes = disk_system_size_bytes - disk_system_free_bytes  # use 'free' to calc used

    disk_sytem_used_percentage = 100.0 * disk_system_used_bytes / disk_system_size_bytes

    print(INDENT + "Stats for file system {}".format(FRAMEPATH))
    indent_increase()
    print(INDENT + "size {:,} bytes".format(disk_system_size_bytes))
    print(INDENT + "used {:,} bytes".format(disk_system_used_bytes))
    print(INDENT + "used {} %%".format(disk_sytem_used_percentage))
    print(INDENT + "free {:,} bytes".format(disk_system_free_bytes))
    print(INDENT + "avail{:,} bytes".format(disk_system_avail_bytes))
    print(INDENT + "req  {:,} bytes".format(REQUIREDFREEBYTES))
    indent_decrease()

    if disk_system_avail_bytes < REQUIREDFREEBYTES:
        # need to make more space available
        print(INDENT + "Insufficient free space, will delete file(s) to free up space")
        space_needed_bytes = REQUIREDFREEBYTES - disk_system_avail_bytes
        print(INDENT + "Need to clear {:,} bytes".format(space_needed_bytes))

        # find oldest one hour of full frame files
        oldest_hour_full_frame_files = find_oldest_files(ONE_HOUR, FULL_FRAME_FILES)
        print(INDENT + "oldest_hour_full_frame_files:")
        print(oldest_hour_full_frame_files)

        # find oldest one hour of second trend frame files
        oldest_hour_second_trend_files = find_oldest_files(ONE_HOUR, SECOND_TREND_FILES)
        print(INDENT + "oldest_hour_second_trend_files:")
        print(oldest_hour_second_trend_files)

        # find oldest one hour of minute trend frame files
        oldest_hour_minute_trend_files = find_oldest_files(ONE_HOUR, MINUTE_TREND_FILES)
        print(INDENT + "oldest_hour_second_trend_files:")
        print(oldest_hour_minute_trend_files)

        if DELETE_FILES is True:
            recovered_bytes = 0
            for filename in oldest_hour_full_frame_files:
                frame_full_path = full_path(filename)
                filesize = os.stat(frame_full_path).st_size
                recovered_bytes += filesize
                print("Removing {} {}".format(filesize, frame_full_path))
                os.remove(frame_full_path)
            for filename in oldest_hour_second_trend_files:
                frame_full_path = full_path(filename)
                filesize = os.stat(frame_full_path).st_size
                recovered_bytes += filesize
                print("Removing {} {}".format(filesize, frame_full_path))
                os.remove(frame_full_path)
            for filename in oldest_hour_minute_trend_files:
                frame_full_path = full_path(filename)
                filesize = os.stat(frame_full_path).st_size
                recovered_bytes += filesize
                print("Removing {} {}".format(filesize, frame_full_path))
                os.remove(frame_full_path)

            print("one hour of files were deleted, recovered {:,} ({:,} needed)".
                  format(recovered_bytes, space_needed_bytes))

    else:
        # do not need to make any more space available, exit
        print(INDENT + "System has enough available space, no need to delete any files")

    # print program stop information
    print(" ")
    print("\t...done")
    print("\tStop time {}".format(str(datetime.now())))
    print(" ")
