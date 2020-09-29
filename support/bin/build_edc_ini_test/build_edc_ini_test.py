import unittest
import os
import os.path as path
import filecmp

class NoNewlineTestCase(unittest.TestCase):
    """
    A basic set of tests around a set of files where one does not have a newline at the end of the last line.
    """
    def setUp(self):
        self.working_dir = "nonewline"
        self.out_file = f"output.ini"
        self.out_path = path.join(self.working_dir, self.out_file)

    def test_no_newline_first(self):
        status = os.system(f"../build_edc_ini -m {self.working_dir}/nonewlinefirstmaster.txt -o {self.out_file}")
        self.assertEqual(status, 0, f"no_newline_first exited with exit code {status}, should be 0")
        self.assertTrue(path.exists(self.out_path), f"output file {self.out_file} was not created")
        self.assertTrue(filecmp.cmp(self.out_path, f"{self.working_dir}/newline_first_target.ini"))

    def test_no_newline_second(self):
        status = os.system(f"../build_edc_ini -m {self.working_dir}/nonewlinesecondmaster.txt -o {self.out_file}")
        self.assertEqual(status, 0, f"no_newline_second exited with exit code {status}, should be 0")
        self.assertTrue(path.exists(self.out_path), f"output file {self.out_file} was not created")
        self.assertTrue(filecmp.cmp(self.out_path, f"{self.working_dir}/newline_second_target.ini"))

    def test_missing_file(self):
        status = os.system(f"../build_edc_ini -m {self.working_dir}/missingfilemaster.txt -o {self.out_file}")
        self.assertEqual(status, 256, f"build_edc_ini should throw an error on missing file")

    def test_empty_file(self):
        status = os.system(f"../build_edc_ini -m {self.working_dir}/emptymaster.txt -o {self.out_file}")
        self.assertEqual(status, 0, f"exited with exit code {status}, should be 0")
        self.assertTrue(path.exists(self.out_path), f"output file {self.out_file} was not created")

    def tearDown(self):
        os.remove(self.out_path)

if __name__ == "__main__":
    unittest.main()