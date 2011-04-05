#!/usr/bin/perl -w -I.

require "daq.pm";

# get the channel list
DAQ::connect("fb", 8088);
#DAQ::print_channel();
#DAQ::print_channel("C1:SUS-ETMY_OPLEV_SUM");

# get current time
$gps = DAQ::gps();

# get three seconds of data starting 5 seconds ago
@data = DAQ::acquire("C1:SUS-ETMY_OPLEV_SUM", 3, $gps - 5);

# calculate average on the data and print
$avg = 0.0;
$cnt = 0;
for (@data) {
	$cnt++;
	#printf "%.1f ", $_;
	$avg += $_;
}
$avg /= $cnt;
print $avg, "\n";
