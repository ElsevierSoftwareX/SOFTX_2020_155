#!/usr/bin/perl

die unless @ARGV == 1;
my $fname = $ARGV[0];
open(IN, $fname) || die "***ERROR: could not open $fname\n";
@inData=<IN>;
close IN;

@res =  grep /^WARNING:.*undefined/, @inData;
if (@res) { print STDERR  "***ERROR: undefined symbols in the front-end code.\n" }
exit(@res);
