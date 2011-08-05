#!/usr/bin/perl

die "Usage: ./iniChk.pl <INI file>\n" if (@ARGV != 1);

###  ######################################################  ###
#                                                              #
###  This subroutine processes the initial default section.  ###
#                                                              #
###  ######################################################  ###
sub processDefaultSection  {

   my ($line) = @_;
   my $returnValue = 1;

#
#  The defult section should begin with the line "[default]", followed
#  by nine "keyword=value" lines and a zero-length blank line.
#
   if ($line =~ /^\[default\]/)  {
      if ($lineCount != 1)  {
         print "\n***ERROR: The file does NOT begin with \'\[default\]\' section\n";
         $errorCount++;
      }
      else  {
         $defaultCount++;
      }
   }
   elsif ($line =~ /^acquire=(\d+)/)  {
      $defaultAcquireValue = $1;
      $defaultCount++;

      if ( ($defaultAcquireValue != 0) && ($defaultAcquireValue != 1) )  {
         print "\n***ERROR: Incorrect acquire value in default section - $defaultAcquireValue\n";
         $defaultAcquireValue = -1;
         $errorCount++;
      }
   }
   elsif ($line =~ /^datarate=(\d+)/)  {
      $dataRate = $1;
      $defaultCount += 10;

      my $returnValue = &processDataRate($dataRate);

      if ($returnValue == -1)  {
         print "\n***ERROR: Incorrect datarate value in default section - $dataRate\n";
         $errorCount++;
      }
   }
   elsif ($value =~ /^datatype=(\d+)/)  {
      my $dataType = $1;
      $defaultCount += 100;

      if ( ($dataType != 1) && ($dataType != 2) && ($dataType != 3) && ($dataType != 4) )  {
         print "\n***ERROR: Incorrect datatype value in default section - $dataType\n";
         $errorCount++;
      }
   }
   elsif ($line =~ /^dcuid=/)  {
      $defaultCount += 1000;
   }
   elsif ($line =~ /^gain=/)  {
      $defaultCount += 10000;
   }
   elsif ($line =~ /^ifoid=/)  {
      $defaultCount += 100000;
   }
   elsif ($line =~ /^offset=/)  {
      $defaultCount += 1000000;
   }
   elsif ($line =~ /^slope=/)  {
      $defaultCount += 10000000;
   }
   elsif ($line =~ /^units=/)  {
      $defaultCount += 100000000;
   }
   elsif (length($line) > 0)  {
      print "\n***ERROR: Found unidentified non-zero length line in default section - $line\n";
      $errorCount++;
   }
   elsif (length($line) == 0)  {
      $returnValue = 0;
   }
   else  {
      print "\n***ERROR: Found unidentified entry in default section - $line\n";
      $errorCount++;
   }

   return $returnValue;
}

###  ###########################################################  ###
#                                                                   #
###  This subroutine processes the concluding parameter section.  ###
#                                                                   #
###  ###########################################################  ###
sub processParameterSection  {

   my ($line) = @_;

#
#  Check all the lines that are not in the default section.
#  There is no need to check commented out lines (i.e.,
#  lines that begin with a "#" character).
#
   if ($line !~ /^#/)  {
      if ($line =~ /^\[[A-Z]\d\:\w+\-\w+\]/)  {
         return 0;
      }
      elsif ($line =~ /^acquire=(\d+)/)  {
         my $acquireValue = $1;

         if ($acquireValue == 0)  {
            $acquireCount[0]++;
         }
         elsif ($acquireValue == 1)  {
            $acquireCount[1]++;
         }
         else  {
            print "\n***ERROR: Incorrect acquire value - $acquireValue\n";
            $errorCount++;
         }
      }
      elsif ($line =~ /^datarate=(\d+)/)  {
         $dataRate = $1;

         my $returnValue = &processDataRate($dataRate);

         if ($returnValue != -1)  {
            $rateCount[$returnValue]++;
         }
         else  {
            print "\n***ERROR: Incorrect datarate value - $dataRate\n";
            $errorCount++;
         }
      }
      elsif ($line =~ /^datatype=(\d+)/)  {
         my $dataType = $1;

         if ( ($dataType != 1) && ($dataType != 2) && ($dataType != 3) && ($dataType != 4) )  {
            print "\n***ERROR: Incorrect datatype value - $dataType\n";
            $errorCount++;
         }
      }
      elsif ($line =~ /^chnnum=(\d+)/)  {
         my $channelNum = $1;

         if ($channelNum == 0)  {
            print "\n***ERROR: Incorrect chnnum value - $channelNum\n";
            $errorCount++;
         }
      }
      else  {
         print "\n***ERROR: Found unidentified/incorrect entry - $line\n";
         $errorCount++;
      }
   }

   return 1;
}

###  ###############################################  ###
#                                                       #
###  This subroutine processes the datarate keyword.  ###
#                                                       #
###  ###############################################  ###
sub processDataRate  {

   my ($rate) = @_;

   my $correctDataRate = -1;
   my $dataRateHelp = 256;

   for (my $i = 0; $i < 9; $i++)  {
      if ($rate == $dataRateHelp)  {
         $correctDataRate = $i;
         last;
      }

      $dataRateHelp *= 2;
   }

   return $correctDataRate;
}

##############################################
# # # # # # #                    # # # # # # #
##############   Main routine.  ##############
# # # # # # #                    # # # # # # #
##############################################

#
#  Begin by verifying the .ini file exists and, if it does, open
#  the .ini file, read its contents, and close the file.
#
if (-e $ARGV[0])  {
   open(IN, "<" . $ARGV[0]) || die "\n***ERROR: Can NOT open $ARGV[0]!\n";

   chomp(@inData = <IN>);

   close(IN);
}
else  {
   die "\n***ERROR: $ARGV[0] does NOT exist!\n";
}

#
#  Initialize three counters, a combined flag/index,
#  and a default section flag.
#
$defaultCount = -1;
$errorCount = 0;
$lineCount = 0;

$defaultAcquireValue = -1;
$defaultSection = 1;

#
#  Cycle through the file contents, one line at a time.
#
foreach $value (@inData)  {
   $lineCount++;

#  $valLen = length($value);
#  print "\nValue=$value \t Length=$valLen\n";

#
#  The file should begin with a 10-line default section.
#
   if ($defaultSection == 1)  {
      $defaultSection = &processDefaultSection($value);

#
#  Check if the end of the default section has been found.
#  If it has, first verify that exactly one of each of the
#  default section "keyword=value" lines were found.
#  Then, initialize counters for the "acquire=" values
#  (0 and 1) and also for the different data rates
#  (ranging from 256 to 65,536).
#
      if ($defaultSection == 0)  {
         if ($defaultCount != 111111111)  {
            print "\n***ERROR: Incorrect \'\[default\]\' section - $defaultCount\n";
            $errorCount++;
         }

         $acquireCount[0] = 0;
         $acquireCount[1] = 0;

#        if ($defaultAcquireValue != -1)  {
#           $acquireCount[$defaultAcquireValue]++;
#        }

         for (my $i = 0; $i < 9; $i++)  {
            $rateCount[$i] = 0;
         }
      }
   }
   else  {
      &processParameterSection($value);
   }
}

#
#  All lines have been checked.  Print the counts for the
#  number of lines with "acquire=0" and "acquire=1", as
#  well as the total of these two values.
#
$acquireTotal = $acquireCount[0] + $acquireCount[1];

print "\nTotal count of \'acquire=0\' is $acquireCount[0]";
print "\nTotal count of \'acquire=1\' is $acquireCount[1]";
print "\nTotal count of \'acquire=0\' and \'acquire=1\' is $acquireTotal\n";

#
#  Print the counts for each datarate, as well as
#  the total datarate (in bytes).
#
$dataRateHelp = 256;
$totalByteCount = 0;

for ($i = 0; $i < 9; $i++)  {
   $totalBytes = $dataRateHelp * $rateCount[$i];
   print "\nCounted $rateCount[$i] entries of datarate=$dataRateHelp \tfor a total of $totalBytes";

   $totalByteCount += $totalBytes;
   $dataRateHelp *= 2;
}

$totalByteCount *= 4;
print "\n\nTotal data rate is $totalByteCount bytes - ";

#
#  Check that the total datarate is 2M bytes or less.
#  Print a warning message if it is not.
#
if ($totalByteCount <= 2097152)  {
   print "OK\n";
}
else  {
   print "\* WARNING \*, this is bigger than 2M bytes!!\n";
}

#
#  Print a count of the total number of errors found.
#
print "\nTotal error count is $errorCount\n\n";

