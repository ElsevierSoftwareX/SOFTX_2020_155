#!/usr/bin/perl -w -I.

# This script claculates an average of $mychan starting 5 seconds ago up till now

require "daq.pm";

$mychan = "C1:SUS-ETMY_OPLEV_SUM";
# get the channel list
DAQ::connect("fb", 8088);
#DAQ::print_channel();
#DAQ::print_channel("C1:SUS-ETMY_OPLEV_SUM");

# get current time
$gps = DAQ::gps();

$gps -= 5;

# get three seconds of data starting 5 seconds ago
@data = DAQ::acquire($mychan, 3, $gps);

# calculate average on the data and print
$avg = 0.0;
$cnt = 0;
for (@data) {
	$cnt++;
	#printf "%.1f ", $_;
	$avg += $_;
}
$avg /= $cnt;
printf "at gps=$gps $mychan averaged %.1f for 5 seconds\n", $avg;
