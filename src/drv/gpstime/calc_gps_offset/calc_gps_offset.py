#!/usr/bin/env python3



import argparse
import collections
import os.path
import sys
import time

NTPLeapSec = collections.namedtuple("NTPLeapSec", ['ntp', 'offset'])
UNIXLeapSec = collections.namedtuple("UNIXLeapSec", ['unix', 'offset', 'ymd'])
YMD = collections.namedtuple("YMD", ['year', 'month', 'day'])


class CannotCalculateOffset(RuntimeError):
    pass


class YMD(object):
    def __init__(self, year, month, day):
        year = int(year)
        month = int(month)
        day = int(day)
        if month < 1 or month > 12 or day < 1 or day > 31 or year < 1900 or year > 9999:
            raise ValueError("Invalid day")
        self.__year = year
        self.__month = month
        self.__day = day
        self.__hash = year * 10000 + month * 100 + day

    def year(self):
        return self.__year

    def month(self):
        return self.__month

    def day(self):
        return self.__day

    def __unicode__(self):
        return "({0}, {1}, {2})".format(self.__year,
                                        self.__month,
                                        self.__day)

    def __lt__(self, other):
        return self.__hash < other.__hash

    def __gt__(self, other):
        return self.__hash > other.__hash

    def __le__(self, other):
        return self.__hash <= other.__hash

    def __ge__(self, other):
        return self.__hash >= other.__hash

    def __eq__(self, other):
        return self.__hash == other.__hash

    def __hash__(self):
        return self.__hash

    @staticmethod
    def convert(s):
        parts = s.split('-')
        return YMD(parts[0], parts[1], parts[2])


class Config(object):
    def __init__(self, do_auto, do_year, do_system, leap_filename, ref_time, status_only, dry_run):
        def valid_flags():
            if do_auto:
                return do_year == False and do_system == False
            return do_year != do_system

        def get_offset_type():
            with open("/sys/kernel/gpstime/offset_type", "rt") as f:
                return int(f.readline().strip())

        if not valid_flags() and not status_only:
            errmsg = "You must choose auto, year based, system based offsets"
            raise ValueError(errmsg)
        if do_auto:
            offset_type = get_offset_type()
            do_year = False
            do_system = False
            if offset_type == 0:
                pass
            elif offset_type == 1:
                do_year = True
            elif offset_type == 2:
                do_system = True
            else:
                raise ValueError("Unkown gps offset type, do not know how to calculate the offset")
        self.__do_year = do_year
        self.__do_system = do_system
        self.__ref_time = YMD.convert(ref_time)
        self.__filename = leap_filename
        self.__status_only = status_only
        self.__dry_run = dry_run

    def year_based(self):
        return self.__do_year

    def system_time_based(self):
        return self.__do_system

    def leapsec_filename(self):
        return self.__filename

    def reference_time(self):
        return self.__ref_time

    def status_only(self):
        return self.__status_only

    def dry_run(self):
        return self.__dry_run

    def __unicode__(self):
        mode = "year"
        if self.system_time_based():
            mode = "system-time"
        return "(mode: {0}, time: {1}, db: {2}".format(mode,
                                                       self.reference_time().__unicode__(),
                                                       self.leapsec_filename())


def parse_arguments(argv):
    parser = argparse.ArgumentParser(description="""Generate GPS offsets
for the LIGO gpstime driver.""")
    help_txt = "Autodetect the type of offset needed"
    parser.add_argument("-a", "--auto", help=help_txt,
                        action="store_true", default="False")
    help_txt = "Calculate offset where the year is not in the IRIG-B signal"
    parser.add_argument("-y", "--year-based", help=help_txt,
                        action="store_true", default=False)
    help_txt = "Calculate offset where there is no IRIG-B signal"
    parser.add_argument("-s", "--system-time", help=help_txt,
                        action="store_true", default=False)
    default = "/var/cache/ietf-leap-seconds/leap-seconds.list"
    help_txt = "Location of a leap-second database [{0}]".format(default)
    parser.add_argument("-l", "--leap-second-db", help=help_txt,
                        default=default)

    help_txt = "Do NOT attempt to set the gps_offset to the computed value"
    parser.add_argument("-n", "--dry-run", help=help_txt, action="store_true",
                        default=False)
    default = None
    help_txt = "Time to calculate the offset for "
    "(UNIX time in seconds) defaults to now"
    parser.add_argument("-t", "--time", help=help_txt,
                        default=default)
    parser.add_argument("--status", help="Print driver status", action="store_true")

    opts = parser.parse_args(argv[1:])

    try:
        tm = opts.time
        if tm is None:
            tm = time.gmtime(time.time())
            tm = "{0}-{1}-{2}".format(
                tm.tm_year,
                tm.tm_mon,
                tm.tm_mday,
            )
        return Config(opts.auto,
                      opts.year_based,
                      opts.system_time,
                      opts.leap_second_db,
                      tm,
                      opts.status,
                      opts.dry_run)
    except ValueError as e:
        parser.error(e)


def seconds_in_year(year):
    regular_year = 365 * 86400
    leap_year = 366 * 86400

    if year % 4 == 0:
        if year % 100 == 0:
            if year % 400 == 0:
                return leap_year
            return regular_year
        return leap_year
    return regular_year


def parse_leap_sec_db(fname):
    results = []
    with open(fname, 'rt') as f:
        for line in f:
            data = line.split('#')[0].strip()
            if data == "":
                continue
            parts = data.split()
            if len(parts) != 2:
                continue
            try:
                entry = NTPLeapSec(int(parts[0]), int(parts[1]))
            except ValueError:
                continue
            results.append(entry)
    return results


def unix_to_ymd(secs):
    tm = time.gmtime(secs)
    return YMD(tm.tm_year, tm.tm_mon, tm.tm_mday)


def ntp_to_unix(sec_ntp):
    return sec_ntp - 2208988800


def ntp_leap_to_unix(ntp_leap):
    results = []
    for ntp in ntp_leap:
        unix = ntp_to_unix(ntp.ntp)
        entry = UNIXLeapSec(unix, ntp.offset, unix_to_ymd(unix))
        results.append(entry)
    return results


def parse_leap_sec_to_unix(filename):
    ntp_leap = parse_leap_sec_db(filename)
    return ntp_leap_to_unix(ntp_leap)


def count_leaps_between_ymd(unix_leaps, ymd_start, ymd_end):
    count = 0
    for entry in unix_leaps:
        if ymd_start <= entry.ymd <= ymd_end:
            count += 1
    return count


def last_leaps_offset(unix_leaps, ymd):
    last_offset = 0
    for entry in unix_leaps:
        if entry.ymd <= ymd:
            last_offset = entry.offset
        else:
            break
    return last_offset


def year_based_offset(unix_leaps, ymd):
    # print("YBO")
    offset = 15
    if ymd.year() < 2010:
        raise CannotCalculateOffset()
    year = 2010
    while year < ymd.year():
        if year == 2010:
            delta = 31190400
        else:
            delta = seconds_in_year(year)
        # print("{0} adding {1}".format(year, delta))
        offset += delta
        year += 1
    delta = count_leaps_between_ymd(unix_leaps, YMD(2010, 1, 1), ymd)
    # print("Add leaps {0}".format(delta))
    offset += delta
    return offset


def system_time_based_offset(unix_leaps, ymd):
    # print("STBO")
    offset = - 315964819

    # do the same range as the year based offset
    if ymd.year() < 2010:
        raise CannotCalculateOffset()
    delta = last_leaps_offset(unix_leaps, ymd)
    # print("Add offset {0}".format(delta))
    offset += delta
    return offset


def print_card_status():
    def read_val(fname):
        with open(os.path.join("/sys/kernel/gpstime", fname), "rt") as f:
            return f.readline().strip()

    def explain_card_type(card_present, card_type):
        if card_present == 0:
            return "none"
        elif card_type == 0:
            return "symmetricom"
        elif card_type == 1:
            return "spectracom"
        else:
            return "unkown"

    def explain_offset_type(val):
        if val == 0:
            return "none"
        elif val == 1:
            return "year"
        elif val == 2:
            return "system time"
        return "unknown"

    card_present = int(read_val("card_present"))
    card_type = int(read_val("card_type"))
    offset_type = int(read_val("offset_type"))
    offset = int(read_val("offset"))
    status = int(read_val("status"))

    print(
        "Card Present: {0}\nCard Type: {1} ({2})\nOffset Type: {3} ({4})\nOffset: {5}\nStatus: {6}\nCur Time: {7}".format(
            card_present,
            card_type,
            explain_card_type(
                card_present,
                card_type),
            offset_type,
            explain_offset_type(
                offset_type),
            offset,
            status,
            read_val("time")))


def apply_offset(offset):
    fname = '/sys/kernel/gpstime/offset'
    try:
        with open('/sys/kernel/gpstime/offset', 'wt') as f:
            f.write("{0}".format(offset))
    except:
        print("Unable to apply offset of {0} to {1}.  Please make sure the gpstime module is loaded and you have "
              "permission to write to the offset file.".format(offset, fname))
        sys.exit(1)

def main(argv):
    cfg = parse_arguments(argv)
    leap_sec_db = parse_leap_sec_to_unix(cfg.leapsec_filename())
    if cfg.status_only():
        print_card_status()
        return
    if cfg.year_based():
        offset = year_based_offset(leap_sec_db, cfg.reference_time())
    elif cfg.system_time_based():
        offset = system_time_based_offset(leap_sec_db, cfg.reference_time())
    else:
        offset = 0
    if cfg.dry_run():
        print("{0}".format(offset))
    else:
        apply_offset(offset)


if __name__ == '__main__':
    main(sys.argv)
    sys.exit(0)
