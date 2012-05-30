#!/usr/bin/perl -w

# Iterate through all files in the current directory
# Print commands to create and move each file into 
# a directory based on crc8 of the file name
#
@files = <*>;
foreach $file (@files) {
  $result = `crc8 $file`;
  chomp($result);
  print("mkdir -p $result; mv $file $result\n");
}
