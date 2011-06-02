#!/usr/bin/perl
 
#
#  We will need the ifo name in upper case as part of the file path.
#
$ifoName = uc(substr($ARGV[0], 0, 2) );
 
#
#  Assemble file path & name of the .db file.
#
$fileName = "../../../target/";
$fileName .= $ARGV[0];
$fileName .= "epics/db/";
$fileName .= $ifoName;
$fileName .= "/";
$fileName .= $ARGV[0];
$fileName .= "1";
$fileName .= "\.db";
 
#
#  Open the .db file, read its contents, and close it.
#
if (-e $fileName)  {
   open(IN, "<" . $fileName) || die "***ERROR: Can NOT open $fileName\n";
 
   chomp(@inData=<IN>);
 
   close IN;
 
   $tooLong = 0;
   $maxPartNameLength = 34;
   $maxLength = 0;
   print "\n";
 
#
#  Loop through the lines in the .db file, looking for lines that
#  begin with 'grecord'.
#
   foreach $value (@inData)  {
      if ($value =~ /^grecord/)  {
 
#
#  Extract the part name and check its length.  Increment counter
#  if it is too long and print an error message.
#
         if ($value =~ /\,\"([\w\:\-]+)\"\)/)  {
            $partName = $1;
 
            if (length($partName) > $maxPartNameLength)  {
               $tooLong++;
	       if(length($partName) > $maxLength)
	       {
	       		$maxLength = length($partName);
		}
               print "\n***ERROR: Part name too long: $partName\n";
            }
         }
      }
   }
 
#
#  Abort if any part names were found to be too long.
#
   if ($tooLong)  {
      die"\n***ERROR - Too long part name(s) total $tooLong errors with max name length of $maxLength- ABORTING\n";
   }
}
else  {
   die "\n***ERROR: $fileName does NOT exist\n";
}

