#!/usr/bin/python

# to run these tests: python3 -m unittest <this-file>

import unittest
import os
import os.path as path
import shutil

test_dir = "checkdaqconfig_testfiles"
test_hash = "7d134aad8c722085dda3b78b2ba20233"

def get_test_dir(test_folder):
    global test_dir
    base_dir, _ = path.split(path.abspath(__file__))
    abs_test_dir = path.join(base_dir, test_dir)
    return path.join(abs_test_dir, test_folder)

def test_master(self, files):
    """
    Test that the master file is pointing to precisely the set of files specified by files
    and that it is pointing into the right hash directory.

    :param self: A TestCase with self.hash_path defined
    :param files:
    :return: None
    """
    with open(f"{self.hash_path}/master", "rt") as f:
        for l in f.readlines():
            line = l.strip()
            if len(line) > 0:
                fpath, fname = path.split(line)
                self.assertEqual(fpath, self.hash_path, "master file pointing to wrong directory")
                self.assertIn(fname, files, f"{fname} not among expected files in master")
                if fname in files:
                    files.remove(fname)
        self.assertEquals(len(files), 0, "some files expected in master were not found")

def test_link(self, daq_name):
    """
    Test that the running directory is a link to the right hash file.

    :param self:
    :return:
    """
    link_path = f"{self.base_path}/{daq_name}/running"
    self.assertTrue(path.lexists(link_path), "running doesn't exist")
    self.assertTrue(path.islink(link_path), "running not a link")
    print(f"link path is: {link_path}")
    real_path = path.realpath(link_path)
    print(f"real path is: {real_path}")
    real_hash_path = path.realpath(self.hash_path)
    self.assertEqual(real_path, self.hash_path, f"'running' link pointing to wrong path")

class TargetTestCase(unittest.TestCase):
    def setUp(self):
        self.base_path = get_test_dir("target_path")
        self.archive_path = f"{self.base_path}/target/daq/archive"
        self.hash_path = f"{self.archive_path}/hash_archive/{test_hash}"
        self.tearDown()

    def tearDown(self):
        shutil.rmtree(self.archive_path, ignore_errors=True)
        shutil.rmtree(f"{self.base_path}/daq0", ignore_errors=True)

    def test_default_paths(self):
        status = os.system(f"./checkdaqconfig.py -t {self.base_path} daq0")
        self.assertEqual(status, 0, f"default run exited with status {status}")
        self.assertTrue(path.exists(self.hash_path), "hash directory not created or created with the wrong hash")
        files = set(["test1.ini", "test1.par"])
        test_master(self, files)
        test_link(self, "daq0")
        self.assertTrue(path.exists(f"{self.archive_path}/daqconfig.log"), "log file not created")

    def test_specified_paths(self):
        status = os.system(f"./checkdaqconfig.py -a {self.base_path}/target/daq/archive -m {self.base_path}/target/daq/master.in -i {self.base_path}/chans/daq -p {self.base_path}/target/gds/param -b {self.base_path} daq0")
        self.assertEqual(status, 0, f"default run exited with status {status}")
        self.assertTrue(path.exists(self.hash_path), "hash directory not created or created with the wrong hash")
        files = set(["test1.ini", "test1.par"])
        test_master(self, files)
        test_link(self, "daq0")
        self.assertTrue(path.exists(f"{self.archive_path}/daqconfig.log"), "log file not created")

class ReplaceTestCase(unittest.TestCase):
    def setUp(self):
        self.base_path = get_test_dir("replace")
        self.archive_path = f"{self.base_path}/target/daq/archive"
        self.hash_path = f"{self.archive_path}/hash_archive/{test_hash}"
        shutil.rmtree(self.base_path, ignore_errors=True)
        shutil.copytree(get_test_dir("replace.base"), self.base_path, symlinks=True)
        with open(f"{self.base_path}/target/daq/master.in", "at") as f:
            f.write(f"\n$base_path/test1.par\n")

    def tearDown(self):
        shutil.rmtree(self.base_path, ignore_errors=True)

    def test_replace_link(self):
        status = os.system(f"./checkdaqconfig.py -t {self.base_path} daq0")
        self.assertEqual(status, 0, f"default run exited with status {status}")
        self.assertTrue(path.exists(self.hash_path), "hash directory not created or created with the wrong hash")
        files = set(["test1.ini", "test1.par"])
        test_master(self, files)
        test_link(self, "daq0")
        self.assertTrue(path.exists(f"{self.archive_path}/daqconfig.log"), "log file not created")

class NotALinkTestCase(unittest.TestCase):
    def setUp(self):
        self.base_path = get_test_dir("not_a_link")
        self.archive_path = f"{self.base_path}/target/daq/archive"
        self.hash_path = f"{self.archive_path}/hash_archive/{test_hash}"
        self.tearDown()

    def tearDown(self):
        shutil.rmtree(self.archive_path, ignore_errors=True)

    def test_not_a_link_blocks(self):
        status = os.system(f"./checkdaqconfig.py -t {self.base_path} daq0")
        self.assertNotEqual(status, 0, f"default run should have failed")
        self.assertFalse(path.islink(f"{self.base_path}/daq0/running"), "should not be a link")

class HardPathTestCase(unittest.TestCase):
    def setUp(self):
        self.base_path = get_test_dir("hard_path")
        self.archive_path = f"{self.base_path}/target/daq/archive"
        self.hash_path = f"{self.archive_path}/hash_archive/{test_hash}"
        self.master_file = f"{self.base_path}/target/daq/master.in"
        self.tearDown()

        with open(self.master_file, "wt") as mf:
            mf.write(f"{self.base_path}/chans1/daq/test1.ini\n")
            mf.write(f"{self.base_path}/target/gds1/param/test1.par\n")

    def tearDown(self):
        shutil.rmtree(self.archive_path, ignore_errors=True)
        shutil.rmtree(f"{self.base_path}/daq0", ignore_errors=True)
        try:
            os.remove(self.master_file)
        except FileNotFoundError:
            pass

    def test_hardcoded_paths(self):
        status = os.system(f"./checkdaqconfig.py -t {self.base_path} daq0")
        self.assertEqual(status, 0, f"default run exited with status {status}")
        self.assertTrue(path.exists(self.hash_path), "hash directory not created or created with the wrong hash")
        files = set(["test1.ini", "test1.par"])
        test_master(self, files)
        test_link(self, "daq0")
        self.assertTrue(path.exists(f"{self.archive_path}/daqconfig.log"), "log file not created")

if __name__ == "__main__":
    unittest.main()
