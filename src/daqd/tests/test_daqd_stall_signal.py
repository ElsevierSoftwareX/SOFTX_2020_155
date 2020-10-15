#!/usr/bin/python3

import os.path
import tempfile
import shutil
import subprocess
import time
import epics

TDIR = ""
PID_MULTI_STREAM = None
PID_DAQD = None


def cleanup():
    if TDIR != "":
        shutil.rmtree(TDIR, ignore_errors=True)
    if PID_MULTI_STREAM is not None:
        PID_MULTI_STREAM.kill()
    if PID_DAQD is not None:
        PID_DAQD.kill()


def check_access(path):
    with open(path, "w"):
        pass


def stall_cb(pvname, value, status, **kwargs):
    print("{0}={1} ({2})".format(pvname, value, status))


try:
    MULTI_STREAM = "../fe_stream_test/fe_multi_stream_test"
    if not os.path.exists(MULTI_STREAM):
        raise (RuntimeError("Unable to find the streamer at {0}".format(MULTI_STREAM)))
    DAQD = "./daqd"
    if not os.path.exists(DAQD):
        raise (RuntimeError("Unable to file the daqd at {0}".format(DAQD)))

    check_access("/dev/gpstime")
    check_access("/dev/mbuf")

    TDIR = tempfile.mkdtemp()
    INI_DIR = os.path.join(TDIR, "ini_files")
    MASTER_FILE = os.path.join(INI_DIR, 'master')
    TESTPOINT_FILE = ""
    os.mkdir(INI_DIR)
    LOG_DIR = os.path.join(TDIR, "logs")
    os.mkdir(LOG_DIR)
    FRAME_DIR = os.path.join(TDIR, "frames")
    os.mkdir(FRAME_DIR)
    FULL_FRAME_DIR = os.path.join(FRAME_DIR, "full")
    os.mkdir(FULL_FRAME_DIR)

    print("Ini dir = {0}".format(INI_DIR))

    with open(os.path.join(LOG_DIR, 'multi_stream.log'), 'wt') as multi_log:

        PID_MULTI_STREAM = subprocess.Popen(args=[
            MULTI_STREAM,
            '-i', INI_DIR,
            '-M', MASTER_FILE,
            '-b', 'local_dc',
            '-m', '100',
            '-k', '700',
            '-R', '100',
        ],
            stdin=None,
            stdout=multi_log,
            stderr=subprocess.STDOUT,
        )
        print("Streamer PID = {0}".format(PID_MULTI_STREAM.pid))

        time.sleep(1)

        with open('daqdrc_live_test', 'rt') as f:
            data = f.read()
            data = data.replace('MASTER', MASTER_FILE)
            data = data.replace('TESTPOINT', TESTPOINT_FILE)
            with open('daqdrc_stall_test_final', 'wt') as out_f:
                out_f.write(data)

        with open(os.path.join(LOG_DIR, 'daqd.log'), 'wt') as daqd_log:

            PID_DAQD = subprocess.Popen(args=[DAQD,
                                              '-c', 'daqdrc_stall_test_final'],
                                        stdin=None,
                                        stdout=daqd_log,
                                        stderr=subprocess.STDOUT)

            print("Daqd PID = {0}".format(PID_DAQD.pid))

            tries = 0
            NotStalled = epics.PV("X3:DAQ-SHM0_PRDCR_NOT_STALLED", callback=stall_cb)
            if NotStalled.wait_for_connection(timeout=20):
                print("Not connected after 10s")
            while NotStalled.get() != 1:
                print('X3:DAQ-SHM0_PRDCR_NOT_STALLED={0}'.format(NotStalled.get()))
                tries += 1
                if tries > 1000:
                    raise RuntimeError("The daqd did not leave the stalled state")
                time.sleep(2)

            print("Killing the streamer")
            PID_MULTI_STREAM.kill()
            PID_MULTI_STREAM = None

            time.sleep(2)

            tries = 0
            while NotStalled.get() == 1:
                tries += 1
                if tries > 10:
                    raise RuntimeError("The daqd did not enter the stalled state")
                time.sleep(1)
            NotStalled.disconnect()
            del NotStalled

finally:
    cleanup()
