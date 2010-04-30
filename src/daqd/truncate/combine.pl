#!/usr/bin/perl

$new_data_dir = "/roframe1/frames/trend/minute_raw";
$old_data_dir = "/frames/trend/minute/raw";
$dest_dir = "/frames/trend/minute_raw";
$dry_run = 1;

open(DAT, "combined.list") || die("Could not open file!");
@a=<DAT>;
close(DAT);

foreach $file (@a) {
chomp $file;
print "cat " . $old_data_dir . "/$file " . $new_data_dir . "/$file > " . $dest_dir .  "/$file\n";

}

