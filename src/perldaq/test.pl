#!/usr/bin/perl -w -I.

# This script claculates an average of $mychan starting 5 seconds ago up till now

require "daq.pm";

#$mychan = "G1:ACO-TCE_SEIS_FF_X_OUT";
#$mychan = "G1:GCO-FILTERSB1POS_OUT";
#$mychan = "G1:GCO-AFTERMAT1X_IN1_DAQ";
#$mychan = "G1:GCO-DELAYDDMOD1R_OUT_DAQ";
$mychan = "H1:FE3-TIM32_T1_ADC_FILTER_5_OUT_DQ";
# get the channel list
DAQ::connect("x1nds0", 8088);
#DAQ::print_channel();
#DAQ::print_channel("C1:SUS-ETMY_OPLEV_SUM");

# get current time
$gps = DAQ::gps();

#$gps -= 300;

@data = DAQ::acquire($mychan, 3, 0, 'L');

# calculate average on the data and print
$avg = 0.0;
$min = 0xffffffff + 0.0;
$max = -0xffffffff + 0.0;
$cnt = 0;
for (@data) {
	$cnt++;
	printf "%.1f ", $_;
	$avg += $_;
	if ($_ < $min) { $min = $_; }
	if ($_ > $max) { $max = $_; }
}
$avg /= $cnt;
printf "at gps=$gps $mychan averaged %.5f for 3 seconds; min=%.5f; max=%.5f\n", $avg, $min, $max;
