#!/usr/bin/perl -w -I.

require "daq.pm";

DAQ::connect("fb", 8088);
#DAQ::print_channel();
#DAQ::print_channel("C1:SUS-ETMY_OPLEV_SUM");
$gps = DAQ::gps();
@data = DAQ::acquire("C1:SUS-ETMY_OPLEV_SUM", 3, $gps - 5);
$avg = 0.0;
$cnt = 0;
for (@data) {
	$cnt++;
	#printf "%.1f ", $_;
	$avg += $_;
}
$avg /= $cnt;
print $avg, "\n";
