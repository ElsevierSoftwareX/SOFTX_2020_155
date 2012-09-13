package CDS::IPCx;
use Exporter;
@ISA = ('Exporter');
 
sub partType {
        return IPCx;
}
 
# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
        ;
}

# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {
        my ($i) = @_;
        ;
}

# Print variable declarations into front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;

        if ($::ipcxDeclDone == 0) {
           $::ipcxDeclDone = 1;

           print ::OUT "#define COMMDATA_INLINE\n";
           print ::OUT "#include \"commData2.h\"\n";
           print ::OUT "static int myIpcCount;\n";
           print ::OUT "static CDS_IPC_INFO ipcInfo[$::ipcxCnt];\n\n";
        }
}

# Return front end initialization code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndInitCode {
        my ($i) = @_;
        my $found = 0;
 
        if ($::ipcxInitDone == 0) {
           $calcExp = "\nmyIpcCount = $::ipcxCnt;\n\n";
        }
        else {
           $calcExp = "";
        }

        $::ipcxRef[$::ipcxInitDone] = $i;
 
        for ($l = 0; $l < $::ipcxCnt; $l++) {
           if ($::ipcxParts[$l][6] == $i) {
              $found = 1;
 
              if ( ($::ipcxParts[$l][7] =~ /^Ground/) || ($::ipcxParts[$l][7] =~ /\_Ground/) ) {
                 $calcExp .= "ipcInfo[$::ipcxInitDone]\.mode = IRCV;\n";
              }
              else {
                 $calcExp .= "ipcInfo[$::ipcxInitDone]\.mode = ISND;\n";
              }
 
              my $subPart = substr($::ipcxParts[$l][1], 0, 5);
              $calcExp .= "ipcInfo[$::ipcxInitDone]\.netType = $subPart;\n";
              $calcExp .= "ipcInfo[$::ipcxInitDone]\.sendRate = $::ipcxParts[$l][2];\n";
              $calcExp .= "ipcInfo[$::ipcxInitDone]\.ipcNum = $::ipcxParts[$l][4];\n";
              $calcExp .= "ipcInfo[$::ipcxInitDone]\.errFlag = 0;\n";
              $calcExp .= "ipcInfo[$::ipcxInitDone]\.name = \"$::ipcxParts[$l][0]\";\n";
              $calcExp .= "ipcInfo[$::ipcxInitDone]\.senderModelName = \"$::ipcxParts[$l][5]\";\n";

              last;
           }
        }
 
        $::ipcxInitDone++;
 
        if ($found == 0) {
           die "***ERROR: Did not find IPCx part $i\n";
        }

        if ($::ipcxInitDone eq $::ipcxCnt) {
           $calcExp .= "\ncommData2Init(myIpcCount, FE_RATE, ipcInfo);\n\n";
        }

        return $calcExp;
}

# Figure out part input code
# Argument 1 is the part number
# Argument 2 is the input number
# Returns calculated input code
sub fromExp {
        my ($i, $j) = @_;
        my $from1 = $::partInNum[$i][$j];
        my $from2 = $::partInputPort[$i][$j];
        my $index = -999;

        for ($k = 0; $k < $::ipcxCnt; $k++) {
           if ($::ipcxRef[$k] == $from1) {
              $index = $k;
              last;
           }
        }

        if ($index < 0) {
           die "***ERROR: index-FE=$index\n";
        }

        if ($from2 == 0) {
           return "ipcInfo[$index]\.data";
        }
        elsif ($from2 == 1) {
           return "ipcInfo[$index]\.errTotal";
        }
        else {
           die "***ERROR: IPCx component with incorrect partInputPort = $from2\n";
        }
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndCode {
        my ($i) = @_;

        if ($::partInputType[$i][0] eq "GROUND") {
           $calcExp = "";
        }
        else {
           my $index = -999;

           for ($k = 0; $k < $::ipcxCnt; $k++) {
              if ($::ipcxRef[$k] == $i) {
                 $index = $k;
                 last;
              }
           }
 
           if ($index < 0) {
              die "***ERROR: index-FEC=$index\n";
           }

           $calcExp = "// IPCx:  $::xpartName[$i]\n";
           $calcExp .= "ipcInfo[$index]\.data = ";
           $calcExp .= "$::fromExp[0];\n";
        }

        return $calcExp;
}
sub procIpc {

my ($i) = @_;
my @ipcxMissing = ("Signal Name","ipcType","ipcRate","ipcHost","ipcNum","ipcModel");
my @ipcxType = ("SHMEM","RFM0","RFM1","PCIE");
my @ipcxMaxNum = (-999,-999,-999,-999);

#
# Find all IPCx parts and start building IPCx parts matrix
#
for ($ii = 0; $ii < $i; $ii++) {
   if ($::partType[$ii] =~ /^IPCx/) {
      $::ipcxParts[$::ipcxCnt][0] = $::xpartName[$ii];
      print "IPC $ii $::ipcxCnt is $::ipcxParts[$::ipcxCnt][0] \n";

#     $::ipcxParts[$::ipcxCnt][1] = "I" . substr($::ipcxBlockTags[$ii], 8, 3);
      $::ipcxCommMech = substr($::ipcxBlockTags[$ii], 8, 4);
      $::ipcxParts[$::ipcxCnt][1] = "I" . $::ipcxCommMech;
      if ($::ipcxCommMech eq "RFM") {
#        print "\n+++  TEST:  Found an RFM\n";
#        print "\n+++  DESCR=$blockDescr[$ii]\n";
         if ($blockDescr[$ii] =~ /^card=(\d)/) {
#           print "\nVALUE=$1\n";
            $::ipcxParts[$::ipcxCnt][1] .= $1;
         }
         else {
            die "\n***ERROR: IPCx part of type RFM with NO card number\n";
         }
      }
      print "IPC $ii $::ipcxCnt is $::ipcxParts[$::ipcxCnt][1] \n";

      $::ipcxParts[$::ipcxCnt][2] = undef;
      $::ipcxParts[$::ipcxCnt][3] = $::targetHost;
      $::ipcxParts[$::ipcxCnt][4] = undef;
      $::ipcxParts[$::ipcxCnt][5] = $::skeleton;
      $::ipcxParts[$::ipcxCnt][6] = $ii;
      $::ipcxCnt++;
   }
   #
   # We will need the location and site parameters from
   # cdsParameters, so keep track of this part as well
   #
   elsif ($::partType[$ii] eq "Parameters") {
      $oo = $ii;
   }
}

if ($::ipcxCnt > 0) {



   # Determine allowed maximum IPC number
   open(CD2, "$::rcg_src_dir/src/include/commData2.h") || die "***ERROR: could not open commData2.h header\n";
   @inData=<CD2>;
   close CD2;

   @res = grep /.*define.*MAX_IPC.*/, @inData;
   die "***ERROR: couldn't find MAX_IPC in commData2.h\n" unless @res;
   $res[0] =~ s/\D//g;
   $maxIpcCount = 0 + $res[0];
   die "**ERROR: unable to determine MAX_IPC\n" unless $maxIpcCount > 0;
   printf "The maximum allowed IPC numer is maxIpcCount=$maxIpcCount\n";
   undef @inData;

   #
   # This model does include IPCx parts, so extract location and
   # site from cdsParameters and read the IPCx parameter file
   #
   ("CDS::Parameters::printHeaderStruct") -> ($oo);


        #my ($i) = @_;
	my $iFile = "/opt/rtcds/";
	$iFile .= $::location;
        $iFile .= "/";
        $iFile .= lc $::site;
	$iFile .= "/chans/ipc/";
	$iFile .= $::site;
	$iFile .= "\.ipc";
   open(IPCIN, "<$iFile") || die "***ERROR: IPCx parameter file $iFile not found\n";
   chomp(@inData=<IPCIN>);
   close IPCIN;

   #
   # Process one line at a time from the IPCx parameter file
   #
   my $ipcxParamCnt = -1;
   $skip = 0;

   foreach $value (@inData) {
      #
      # Skip lines that begin with '#' or 'desc='
      # Also, skip blank lines and default lines
      #
      if ( ($value =~ /^#/) || ($value =~ /^desc=/) ) {
         next;
      }
      elsif ( (length($value) == 0) || ($value =~ /\s/) ) {
         next;
      }

      if ($value =~ /^\[default\]/) {
         $skip = 1;
         next;
      }

      #
      # Find the Signal Name line and copy the
      # value to the IPCx parts matrix
      #
      if ($value =~ /^\[([\w\:\-\_]+)\]/) {
         $skip = 0;
         $ipcxParamCnt++;

         $ipcxData[$ipcxParamCnt][0] = $1;
         $ipcxData[$ipcxParamCnt][1] = undef;
         $ipcxData[$ipcxParamCnt][2] = undef;
         $ipcxData[$ipcxParamCnt][3] = undef;
         $ipcxData[$ipcxParamCnt][4] = undef;
         $ipcxData[$ipcxParamCnt][5] = undef;

         next;
      }

      #
      # Copy the IPCx Communication Mechanism, the
      # Sender Data Rate, and the IPCx Number
      # to the IPCx parts matrix as well
      #
      if ($skip == 0)  {
         if ($value =~ /^ipcType=(\w+)/) {
            my $typeString = $1;
            $ipcxData[$ipcxParamCnt][1] = "I" . substr($typeString, 0, 4);

            if ($typeString =~ /^SHME/) {
               $typeIndex = 0;
            }
            elsif ($typeString =~ /^RFM0/) {
               $typeIndex = 1;
            }
            elsif ($typeString =~ /^RFM1/) {
               $typeIndex = 2;
            }
            elsif ($typeString =~ /^PCIE/) {
               $typeIndex = 3;
            }
            else {
               die "***ERROR: IPCx Communication Mechanism not recognized: $typeString\n";
            }
         }
         elsif ($value =~ /^ipcRate=(\d+)/) {
            $ipcxData[$ipcxParamCnt][2] = $1;
         }
         elsif ($value =~ /^ipcHost=(.+)/) {
            $ipcxData[$ipcxParamCnt][3] = $1;
         }
         elsif ($value =~ /^ipcNum=(\d+)/) {
            $ipcxData[$ipcxParamCnt][4] = $1;

            if ($1 > $ipcxMaxNum[$typeIndex]) {
               if ( ($typeIndex > 0) || ($::targetHost eq $ipcxData[$ipcxParamCnt][3]) ) {
                  $ipcxMaxNum[$typeIndex] = $1;
               }
            }
         }
         elsif ($value =~ /^ipcModel=(.+)/) {
            $ipcxData[$ipcxParamCnt][5] = $1;
         }
      }
   }

   $ipcxParamCnt++;

$ipcxNotFound = 0;
$ipcxRcvrCnt = 0;

   for ($ii = 0; $ii < 4; $ii++) {
      if ($ipcxMaxNum[$ii] == -999) {
         $ipcxMaxNum[$ii] = -1;
      }
   }

   #
   # Locate each IPCx module in the IPCx parts matrix
   #
   for ($ii = 0; $ii < $::ipcxCnt; $ii++) {
      $found = 0;

      if ($::ipcxParts[$ii][0] =~ /^[A-Z]\d\:/) {
          $ipcxPartComp = $::ipcxParts[$ii][0];
      }
      else {
         if ($::ipcxParts[$ii][0] =~ /^\w+([A-Z]\d\:.+)/) {
            $ipcxPartComp = $1;
         }
         else {
            $ipcxPartComp = "";
         }
      }

      for ($jj = 0; $jj < $ipcxParamCnt; $jj++) {
         if ($ipcxPartComp eq $ipcxData[$jj][0]) {
            #
            # If IPCx type is SHMEM, then make sure we're only
            # considering entries for this host
            #
            if ( ($::ipcxParts[$ii][1] eq "ISHME") && ($::targetHost ne $ipcxData[$jj][3]) ) {
	       print "***WARNING: SHMEM IPC $ipcxPartComp found and skipped. My host: $::targetHost; IPC file has: $ipcxData[$jj][3]\n";
               next;
            }

            #
            # Make sure no IPCx parameters are missing
            #
            for ($kk = 2; $kk < 6; $kk++) {
               if (defined($ipcxData[$jj][$kk]) ) {
                  $::ipcxParts[$ii][$kk] = $ipcxData[$jj][$kk];
               }
               else {
                  die "***ERROR: Data missing for IPCx component $::ipcxParts[$ii][0] - $ipcxMissing[$kk]\n";
               }
            }

            $kk = $::ipcxParts[$ii][6];
            $::ipcxParts[$ii][7] = $::partInput[$kk][0];
            $found = 1;
            $typeComp = $::ipcxParts[$ii][1];

            if ($ipcxData[$jj][1] ne $typeComp) {
               die "***ERROR: IPCx type mis-match for IPCx component $::ipcxParts[$ii][0] $::ipcxParts[$ii][1] : $typeComp vs\. $ipcxData[$jj][1]\n";
            }

            #
            # Make sure each IPCx part has exactly one input
            #
            if ($::partInCnt[$kk] != 1) {
               die "***ERROR: IPCx SENDER/RECEIVER component $::ipcxParts[$ii][0] has $::partInCnt[$kk] input(s)\n";
            }

            #
            # If the input to an IPCx part is 'Ground',
            # then this part is a RECEIVER of data,
            # which means there should be either one
            # or two outputs
            #
            if ( ($::partInput[$kk][0] =~ /^Ground/) || ($::partInput[$kk][0] =~ /\_Ground/) ) {
               if ( ($::partOutCnt[$kk] < 1) || ($::partOutCnt[$kk] > 2) ) {
                  #die "***ERROR: IPCx RECEIVER component $::ipcxParts[$ii][0] has $::partOutCnt[$kk] output(s)\n";
                  #die "***ERROR: IPCx RECEIVER component $::ipcxParts[$ii][0] has $::partOutCnt[$kk] output(s)\n";
               }
            }
            #
            # If the input to an IPCx part is NOT 'Ground',
            # then this part is a SENDER of data, which
            # means there should be no outputs
            #
            else {
               if ($::partOutCnt[$kk] != 0) {
                  die "***ERROR: IPCx SENDER component $::ipcxParts[$ii][0] has $::partOutCnt[$kk] output(s)\n";
               }
	       # Make sure we are not trying to transmit to the existing channel second time
	       if (($::ipcxParts[$ii][1] ne "ISHME") && ($ipcxData[$jj][5] ne $::skeleton)) {
		  die "***ERROR: Model name mismatch for IPCx SENDER name=$::ipcxParts[$ii][0] type=$::ipcxParts[$ii][1] : my=$::skeleton vs\. ipc_file=$ipcxData[$jj][5]\n***ERROR: If this is an attempt to move an IPCx SENDER , please remove offending entry from the IPC file\n";
	       }
            }

            last;
         }
      }

      #
      # Check if the IPCx module was found in the IPCx parts matrix
      #
      if ($found == 0) {
         print "### IPCx component ($ii) $::ipcxParts[$ii][0] not found in IPCx parameter file $iFile\n";

         $ipcxAdd[$ipcxNotFound][0] = $ii;

         $kk = $::ipcxParts[$ii][6];

         if ( ($::partInput[$kk][0] =~ /^Ground/) || ($::partInput[$kk][0] =~ /\_Ground/) ) {
            $ipcxAdd[$ipcxNotFound][1] = 2;
            $ipcxRcvrCnt++;
         }
         else {
            if ($::partOutCnt[$kk] != 0) {
               die "***ERROR: IPCx SENDER component $::ipcxParts[$ii][0] has $::partOutCnt[$kk] output(s)\n";
            }
            else {
               $ipcxAdd[$ipcxNotFound][1] = 1;
            }
         }

         $ipcxNotFound++;
      }
   }

   #
   # Check if there are any IPCx modules to add to the IPCx parts matrix
   #
   if ($ipcxNotFound > 0) {
      open (IPCOUT, ">>$iFile") || die "***ERROR: Could not open IPCx parameter file $iFile for output\n";

      $ipcxRate = 983040/$::rate;
      $ipcxNew = 0;

      for ($jj = 0; $jj < $ipcxNotFound; $jj++) {
         if ($ipcxAdd[$jj][1] == 1) {
            #
            # Get type index
            #
            $ipcxTypeIndex = -999;

            $::ipcxCommMech = substr($::ipcxParts[$ipcxAdd[$jj][0]][1], 1, 4);

            for ($kk = 0; $kk < 4; $kk++) {
               if ($::ipcxCommMech eq substr($ipcxType[$kk], 0, 4) ) {
                  $ipcxTypeIndex = $kk;
               }
            }

            if ($ipcxTypeIndex < 0) {
               die "***ERROR: IPCx Communication Mechanism not recognized: $::ipcxCommMech\n";
            }

            #
            # Add data to the IPCx parameter file
            #
            if (++$ipcxMaxNum[$ipcxTypeIndex] > $maxIpcCount) {
               die "***ERROR: IPCx number > $maxIpcCount for ipcType = $ipcxType[$ipcxTypeIndex]\n";
            }

            $signalName = $::ipcxParts[$ipcxAdd[$jj][0]][0];
            if ($signalName =~ /^\w+([A-Z]\d\:.+)/) {
               $signalName = $1;
            }

            print IPCOUT "\[$signalName\]\n";
            print IPCOUT "ipcType=$ipcxType[$ipcxTypeIndex]\n";
            print IPCOUT "ipcRate=$ipcxRate\n";
            print IPCOUT "ipcHost=$::targetHost\n";
            print IPCOUT "ipcModel=$::skeleton\n";
            print IPCOUT "ipcNum=$ipcxMaxNum[$ipcxTypeIndex]\n";
            print IPCOUT "desc=Automatically generated by IPCx\.pm on $::theTime\n\n";

            $ipcxDataAdded[$ipcxNew][0] = $::ipcxParts[$ipcxAdd[$jj][0]];
            $ipcxDataAdded[$ipcxNew][1] = "I" . substr($ipcxType[$ipcxTypeIndex], 0, 4);
            $ipcxDataAdded[$ipcxNew][2] = $ipcxRate;
            $ipcxDataAdded[$ipcxNew][3] = $::targetHost;
            $ipcxDataAdded[$ipcxNew][4] = $ipcxMaxNum[$ipcxTypeIndex];

            $::ipcxParts[$ipcxAdd[$jj][0]][2] = $ipcxRate;
            $::ipcxParts[$ipcxAdd[$jj][0]][3] = $::targetHost;
            $::ipcxParts[$ipcxAdd[$jj][0]][4] = $ipcxMaxNum[$ipcxTypeIndex];
            $::ipcxParts[$ipcxAdd[$jj][0]][5] = $::skeleton;

            $ipcxNew++;
         }
      }

      close IPCOUT;

      #
      # This code can only automatically add IPCx SENDER modules
      #
      if ($ipcxRcvrCnt > 0) {
         print "\n\n***ERROR: The following IPCx RECEIVER module(s) not found in the file $iFile:\n\n";

         for ($jj = 0; $jj < $ipcxNotFound; $jj++) {
            if ($ipcxAdd[$jj][1] == 2) {
               print "\t\t$::ipcxParts[$ipcxAdd[$jj][0]][0]\n";
            }
         }

	 die "\n***ERROR: Aborting (this code can only automatically add IPCx SENDER modules)\n\n";
      }
   }
}

}
