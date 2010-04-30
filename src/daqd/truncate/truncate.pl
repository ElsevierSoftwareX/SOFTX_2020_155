#!/usr/bin/perl

$dir = "/frames/trend/minute_raw";
$dry_run = 1;

@a= <$dir/H*>;
foreach $file (@a) {
          $fs = -s $file;
          $frac = $fs % 40;
	  if ($frac) {
	    $ts = $fs - $frac; # target size
            print $file . " size=" . $frac. " target size=" . $ts . "\n";  

            if (!$dry_run) {
		truncate $file, $ts;
            }
	  }
}
