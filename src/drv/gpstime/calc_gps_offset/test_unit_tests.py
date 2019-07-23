

from unittest import TestCase, main


def ref_ntp_leap_sec():
    from calc_gps_offset import NTPLeapSec
    return [
        NTPLeapSec(2272060800, 10),  # 1 Jan 1972
        NTPLeapSec(2287785600, 11),  # 1 Jul 1972
        NTPLeapSec(2303683200, 12),  # 1 Jan 1973
        NTPLeapSec(2335219200, 13),  # 1 Jan 1974
        NTPLeapSec(2366755200, 14),  # 1 Jan 1975
        NTPLeapSec(2398291200, 15),  # 1 Jan 1976
        NTPLeapSec(2429913600, 16),  # 1 Jan 1977
        NTPLeapSec(2461449600, 17),  # 1 Jan 1978
        NTPLeapSec(2492985600, 18),  # 1 Jan 1979
        NTPLeapSec(2524521600, 19),  # 1 Jan 1980
        NTPLeapSec(2571782400, 20),  # 1 Jul 1981
        NTPLeapSec(2603318400, 21),  # 1 Jul 1982
        NTPLeapSec(2634854400, 22),  # 1 Jul 1983
        NTPLeapSec(2698012800, 23),  # 1 Jul 1985
        NTPLeapSec(2776982400, 24),  # 1 Jan 1988
        NTPLeapSec(2840140800, 25),  # 1 Jan 1990
        NTPLeapSec(2871676800, 26),  # 1 Jan 1991
        NTPLeapSec(2918937600, 27),  # 1 Jul 1992
        NTPLeapSec(2950473600, 28),  # 1 Jul 1993
        NTPLeapSec(2982009600, 29),  # 1 Jul 1994
        NTPLeapSec(3029443200, 30),  # 1 Jan 1996
        NTPLeapSec(3076704000, 31),  # 1 Jul 1997
        NTPLeapSec(3124137600, 32),  # 1 Jan 1999
        NTPLeapSec(3345062400, 33),  # 1 Jan 2006
        NTPLeapSec(3439756800, 34),  # 1 Jan 2009
        NTPLeapSec(3550089600, 35),  # 1 Jul 2012
        NTPLeapSec(3644697600, 36),  # 1 Jul 2015
        NTPLeapSec(3692217600, 37),  # 1 Jan 2017
    ]


def ref_unix_leap_sec():
    from calc_gps_offset import YMD
    from calc_gps_offset import UNIXLeapSec
    return [
        UNIXLeapSec(2272060800 - 2208988800, 10, YMD(1972, 1, 1)),  # 1 Jan 1972
        UNIXLeapSec(2287785600 - 2208988800, 11, YMD(1972, 7, 1)),  # 1 Jul 1972
        UNIXLeapSec(2303683200 - 2208988800, 12, YMD(1973, 1, 1)),  # 1 Jan 1973
        UNIXLeapSec(2335219200 - 2208988800, 13, YMD(1974, 1, 1)),  # 1 Jan 1974
        UNIXLeapSec(2366755200 - 2208988800, 14, YMD(1975, 1, 1)),  # 1 Jan 1975
        UNIXLeapSec(2398291200 - 2208988800, 15, YMD(1976, 1, 1)),  # 1 Jan 1976
        UNIXLeapSec(2429913600 - 2208988800, 16, YMD(1977, 1, 1)),  # 1 Jan 1977
        UNIXLeapSec(2461449600 - 2208988800, 17, YMD(1978, 1, 1)),  # 1 Jan 1978
        UNIXLeapSec(2492985600 - 2208988800, 18, YMD(1979, 1, 1)),  # 1 Jan 1979
        UNIXLeapSec(2524521600 - 2208988800, 19, YMD(1980, 1, 1)),  # 1 Jan 1980
        UNIXLeapSec(2571782400 - 2208988800, 20, YMD(1981, 7, 1)),  # 1 Jul 1981
        UNIXLeapSec(2603318400 - 2208988800, 21, YMD(1982, 7, 1)),  # 1 Jul 1982
        UNIXLeapSec(2634854400 - 2208988800, 22, YMD(1983, 7, 1)),  # 1 Jul 1983
        UNIXLeapSec(2698012800 - 2208988800, 23, YMD(1985, 7, 1)),  # 1 Jul 1985
        UNIXLeapSec(2776982400 - 2208988800, 24, YMD(1988, 1, 1)),  # 1 Jan 1988
        UNIXLeapSec(2840140800 - 2208988800, 25, YMD(1990, 1, 1)),  # 1 Jan 1990
        UNIXLeapSec(2871676800 - 2208988800, 26, YMD(1991, 1, 1)),  # 1 Jan 1991
        UNIXLeapSec(2918937600 - 2208988800, 27, YMD(1992, 7, 1)),  # 1 Jul 1992
        UNIXLeapSec(2950473600 - 2208988800, 28, YMD(1993, 7, 1)),  # 1 Jul 1993
        UNIXLeapSec(2982009600 - 2208988800, 29, YMD(1994, 7, 1)),  # 1 Jul 1994
        UNIXLeapSec(3029443200 - 2208988800, 30, YMD(1996, 1, 1)),  # 1 Jan 1996
        UNIXLeapSec(3076704000 - 2208988800, 31, YMD(1997, 7, 1)),  # 1 Jul 1997
        UNIXLeapSec(3124137600 - 2208988800, 32, YMD(1999, 1, 1)),  # 1 Jan 1999
        UNIXLeapSec(3345062400 - 2208988800, 33, YMD(2006, 1, 1)),  # 1 Jan 2006
        UNIXLeapSec(3439756800 - 2208988800, 34, YMD(2009, 1, 1)),  # 1 Jan 2009
        UNIXLeapSec(3550089600 - 2208988800, 35, YMD(2012, 7, 1)),  # 1 Jul 2012
        UNIXLeapSec(3644697600 - 2208988800, 36, YMD(2015, 7, 1)),  # 1 Jul 2015
        UNIXLeapSec(3692217600 - 2208988800, 37, YMD(2017, 1, 1)),  # 1 Jan 2017
    ]


class TestSecondsInYear(TestCase):
    def test_seconds_in_year(self):
        from calc_gps_offset import seconds_in_year
        secs_in_day = 86400
        leap_year = 366 * secs_in_day
        regular = 365 * secs_in_day
        test_cases = {
            1700: regular,
            1900: regular,
            1997: regular,
            2000: leap_year,
            2001: regular,
            2003: regular,
            2004: leap_year,
            2008: leap_year,
            2010: regular,
            2014: regular,
            2016: leap_year,
            2017: regular,
            2018: regular,
            2020: leap_year,
            2021: regular,
            2400: leap_year,
            3000: regular,
        }
        for year in list(test_cases.keys()):
            expected = test_cases[year]
            self.assertEqual(seconds_in_year(year), expected)


class TestLeapSecDBParser(TestCase):
    def test_parse_leap_sec_db(self):
        from calc_gps_offset import parse_leap_sec_db
        expected = ref_ntp_leap_sec()
        result = parse_leap_sec_db('leap-seconds.list')
        self.assertListEqual(result, expected)


class TestYMD(TestCase):
    def test_ymd_from_str(self):
        from calc_gps_offset import YMD

        y0 = YMD("2016", "12", "31")
        self.assertEqual(y0.year(), 2016)
        self.assertEqual(y0.month(), 12)
        self.assertEqual(y0.day(), 31)

    def test_ymd_ops(self):
        from calc_gps_offset import YMD

        y0 = YMD(2016, 12, 31)
        y1 = YMD(2017, 11, 17)
        y1_dup = YMD(2017, 11, 17)
        y2 = YMD(2018, 1, 1)

        self.assertTrue(y0 < y1)
        self.assertTrue(y1 < y2)
        self.assertTrue(y0 < y2)

        self.assertTrue(y1 > y0)
        self.assertTrue(y2 > y1)
        self.assertTrue(y2 > y0)

        self.assertTrue(y0 <= y1)
        self.assertTrue(y1 <= y2)
        self.assertTrue(y0 <= y2)

        self.assertTrue(y1 >= y0)
        self.assertTrue(y2 >= y1)
        self.assertTrue(y2 >= y0)

        self.assertTrue(y0 == y0)
        self.assertTrue(y1 == y1_dup)

        self.assertTrue(y0 != y1)

        self.assertFalse(y1 > y2)
        self.assertFalse(y2 < y1)


class TestUnixToYMD(TestCase):
    def test_unix_to_ymd(self):
        from calc_gps_offset import unix_to_ymd
        from calc_gps_offset import YMD
        self.assertEqual(unix_to_ymd(1510948294), YMD(2017, 11, 17))


class TestOffsets(TestCase):
    def test_ntp_to_unix(self):
        from calc_gps_offset import ntp_to_unix
        self.assertEqual(ntp_to_unix(2208988800), 0)

    def test_offsets_1980(self):
        from calc_gps_offset import seconds_in_year
        offset = 0
        for i in range(1900, 1980):
            sec_year = seconds_in_year(i)
            offset += sec_year
            # print("{0} {1} {2}".format(i, sec_year, offset))
        print("2524521600 - 2524521600 = {0}".format(2524521600 - 2524521600))

    def test_offsets_1970_unix_epoch(self):
        from calc_gps_offset import seconds_in_year
        offset = 0
        for i in range(1900, 1970):
            sec_year = seconds_in_year(i)
            offset += sec_year
            # print("{0} {1} {2}".format(i, sec_year, offset))
        print("2272060800 - 2208988800 = {0}".format(2272060800 - 2208988800))
        print(seconds_in_year(1970) + seconds_in_year(1971))


class TestNTPToUnixBulk(TestCase):
    def test_ntp_leap_to_unix(self):
        from calc_gps_offset import ntp_leap_to_unix

        ntp_sec = ref_ntp_leap_sec()

        unix_leap = ntp_leap_to_unix(ntp_sec)

        expected = ref_unix_leap_sec()

        self.assertListEqual(unix_leap, expected)


class TestParseToUnix(TestCase):
    def test_parse_leap_sec_to_unix(self):
        from calc_gps_offset import parse_leap_sec_to_unix

        unix_leap = parse_leap_sec_to_unix('leap-seconds.list')

        expected = ref_unix_leap_sec()

        self.assertListEqual(unix_leap, expected)


class TestLeapSecSearch(TestCase):
    def test_count_leaps_between_ymd(self):
        from calc_gps_offset import count_leaps_between_ymd
        from calc_gps_offset import YMD

        leaps = ref_unix_leap_sec()
        test_cases = [
            (YMD(2010, 1, 1), YMD(2015, 6, 31), 1),
            (YMD(2009, 1, 2), YMD(2015, 6, 31), 1),
            (YMD(2009, 1, 1), YMD(2015, 6, 31), 2),
            (YMD(2009, 1, 1), YMD(2015, 7, 1), 3),
        ]
        for test_case in test_cases:
            result = count_leaps_between_ymd(leaps, test_case[0], test_case[1])
            self.assertEqual(result, test_case[2])

    def test_find_last_offset(self):
        from calc_gps_offset import last_leaps_offset
        from calc_gps_offset import YMD

        leaps = ref_unix_leap_sec()
        test_cases = {
            YMD(2015, 6, 31): 35,
            YMD(2015, 7, 1): 36,
            YMD(2016, 12, 31): 36,
            YMD(2017, 1, 1): 37,
            YMD(2018, 1, 1): 37,
        }
        for test_case in test_cases:
            result = last_leaps_offset(leaps, test_case)
            self.assertEqual(result, test_cases[test_case])


class TestYearBasedOffset(TestCase):
    def test_year_based_offset(self):
        from calc_gps_offset import year_based_offset
        from calc_gps_offset import YMD
        from calc_gps_offset import CannotCalculateOffset

        test_cases = {
            YMD(2009, 12, 31): None,
            YMD(2010, 1, 1): 15,
            YMD(2011, 1, 1): 31190400 + 15,
            YMD(2015, 1,
                1): 31190400 + 15 + 31536000 + 1 + 31622400 + 31536000 + 31536000,
            YMD(2015, 6,
                31): 31190400 + 15 + 31536000 + 1 + 31622400 + 31536000 + 31536000,
            YMD(2015, 7,
                1): 31190400 + 15 + 31536000 + 1 + 31622400 + 31536000 + 31536000 + 1,
            YMD(2016, 1,
                1): 31190400 + 15 + 31536000 + 1 + 31622400 + 31536000 + 31536000 + 31536000 + 1,
            YMD(2016, 12,
                31): 31190400 + 15 + 31536000 + 1 + 31622400 + 31536000 + 31536000 + 31536000 + 1,
            YMD(2017, 1,
                1): 31190400 + 15 + 31536000 + 1 + 31622400 + 31536000 + 31536000 + 31536000 + 1 + 31622400 + 1,
            YMD(2018, 1,
                1): 31190400 + 15 + 31536000 + 1 + 31622400 + 31536000 + 31536000 + 31536000 + 1 + 31622400 + 1 +
                    31536000,
        }
        unix_leap = ref_unix_leap_sec()
        for ymd in test_cases:
            try:
                result = year_based_offset(unix_leap, ymd)
                self.assertEqual(result, test_cases[ymd])
            except CannotCalculateOffset:
                self.assertIsNone(test_cases[ymd])


class TestSystemTimeBasedOffset(TestCase):
    def test_system_time_based_offset(self):
        from calc_gps_offset import system_time_based_offset
        from calc_gps_offset import YMD
        from calc_gps_offset import CannotCalculateOffset

        test_cases = {
            YMD(2009, 12, 31): None,
            YMD(2010, 1, 1): - 315964819 + 34,
            YMD(2011, 1, 1): - 315964819 + 34,
            YMD(2015, 1, 1): - 315964819 + 35,
            YMD(2015, 6, 31): - 315964819 + 35,
            YMD(2015, 7, 1): - 315964819 + 36,
            YMD(2016, 1, 1): - 315964819 + 36,
            YMD(2016, 12, 31): - 315964819 + 36,
            YMD(2017, 1, 1): - 315964819 + 37,
            YMD(2018, 1, 1): - 315964819 + 37,
        }
        unix_leap = ref_unix_leap_sec()
        for ymd in test_cases:
            try:
                result = system_time_based_offset(unix_leap, ymd)
                self.assertEqual(result, test_cases[ymd])
            except CannotCalculateOffset:
                self.assertIsNone(test_cases[ymd])


if __name__ == '__main__':
    main()
