package CDS::IPCx;
use Exporter;
@ISA = ('Exporter');

#//	\page IPCx IPCx.pm
#//	Documentation for IPCx.pm
#//
#// \n
#// \n

#//
#// \b Required \b Modules \n
#// 		- \b lib/Util.pm \n
#// 		- \b lib/medmGen.pm \n
#//	
require "lib/Util.pm";
require "lib/medmGen.pm";
 
#// \n\n \b Key \b global \b variables \n\n
#// \b ipcxParts[][] \n
#//	- [][0] = IPC Name \n
#//	- [][1] = IPC Type \n
#//	- [][2] = sender rate \n
#//	- [][3] = sender computer host name \n 
#//	- [][4] = IPC number \n
#//	- [][5] = sender model name \n
#//	- [][6] = model part number \n
#//	- [][7] = input part name \n
#//	- [][8] = EPICS channel name (less ending eg _RCV) \n\n
#//	

#// \b Functions: ******************************************************** \n\n
#// \b sub \b partType -----------------------------------------------------------------\n
#// Returns "IPCx" as the RCG part type \n\n
sub partType {
        return IPCx;
}
 
#// \b sub \b printHeaderStruct --------------------------------------------------------\n
#// Print Epics communication structure into a header file if IPC part is RCVR type.\n
#// 	- \b Param[in] \b $i = Current part number is passed as first argument \n
#//	- \b Param[in] \b $::partInputType[][]
#//	- \b Param[out] \b $::OUTH  Output to model header file
#//	
sub printHeaderStruct {
        my ($i) = @_;
	# Only add EPICS comms if this is an IPC RCVR part ie has a GROUND at input
        if ($::partInputType[$i][0] eq "GROUND") {
		my $pfile = substr($::mdlfile,5);
		my @fnam = split(/\./,$pfile);
		my $ss = substr($::xpartName[$i],3);
		$ss =~ s/\:/_/;
		$ss =~ s/\-/_/;
		# Add Err rate var ie errors/sec
		print ::OUTH "\tint $ss\_ER;\n";
		# Add time of last error detection
		print ::OUTH "\tint $ss\_ET;\n";
		# Add status byte
		print ::OUTH "\tint $ss\_PS;\n";
	}
}

#// \n \b sub \b printEpics ------------------------------------------------------------\n
#// Print Epics variable definitions into file for later use by fmseq \n
#// This routine produces 3 EPICS channels for IPCx receivers only.
#// 	- _ER = RCVR Error rate \n
#// 	- _ET = RCVR Error timestamp \n
#// 	- _PS = RCVR Error Status \n
#//	
#// Parameters: \n
#// 	- \b Param[in] \b $i = Current part number is passed as first argument \n
#// 	- \b Param[in] \b $::mdlfile = Model file name\n
#// 	- \b Param[out] \b $::EPICS = Pointer to EPICS channel def file\b \n
#//	
sub printEpics {
        my ($i) = @_;
	# Only add EPICS channels if this is an IPC RCVR part
        if ($::partInputType[$i][0] eq "GROUND") {
		my $pfile = substr($::mdlfile,5);
		my @fnam = split(/\./,$pfile);
		my $ss = substr($::xpartName[$i],3);
		$ss =~ s/\:/_/;
		$ss =~ s/\-/_/;
		# Add Err rate var ie errors/sec
		print ::EPICS "OUTVARIABLE FEC_$::dcuId\_IPC_$ss\_ER $::systemName\.$ss\_ER int ao 0\n";
		# Add time of last error detection
		print ::EPICS "OUTVARIABLE FEC_$::dcuId\_IPC_$ss\_ET $::systemName\.$ss\_ET int ao 0\n";
		# Add status byte
		print ::EPICS "OUTVARIABLE FEC_$::dcuId\_IPC_$ss\_PS $::systemName\.$ss\_PS int ao 0\n";
	}
}

#// \b sub \b printFrontEndVars --------------------------------------------------------\n
#// Print variable declarations into front-end file \n
#// Returns IPC comms data structure only once per file.
#// Parameters: \n
#// 	- \b Param[in] \b $i = Current part number is passed as first argument \n
#// 	- \b Param[out] \b $::OUT = Pointer to RT C code file \n
#//	
sub printFrontEndVars  {
        my ($i) = @_;

	# Print this in code header only once
        if ($::ipcxDeclDone == 0) {
           $::ipcxDeclDone = 1;

           print ::OUT "#define COMMDATA_INLINE\n";
           print ::OUT "#include \"commData2.h\"\n";
           print ::OUT "static int myIpcCount;\n";
           print ::OUT "static CDS_IPC_INFO ipcInfo[$::ipcxCnt];\n\n";
        }
}

#// \b sub \b frontEndInitCode ---------------------------------------------------------\n
#//  Return front end initialization code for all IPC parts on first call\n
#// Parameters: \n
#// 	- \b Param[in] \b $i = Current part number is passed as first argument \n
#// 	- \b Param[out] \b $::ipcxInitDone = Flag indicating IPC init is complete \n
#//	
sub frontEndInitCode {
        my ($i) = @_;
        my $found = 0;
 
	# Print this in code initialization area only once
        if ($::ipcxInitDone == 0) {
           $calcExp = "\nmyIpcCount = $::ipcxCnt;\n\n";
        }
        else {
           $calcExp = "";
        }

        $::ipcxRef[$::ipcxInitDone] = $i;
 
	# Write out initialization of IPC parts all at one time
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

	# Need to call commData2.c during initialization only once.
        if ($::ipcxInitDone eq $::ipcxCnt) {
           $calcExp .= "\ncommData2Init(myIpcCount, FE_RATE, ipcInfo);\n\n";
        }

        return $calcExp;
}

#// \b sub \b fromExp ------------------------------------------------------------------\n
#// Figure out part input code \n
#// Returns calculated input code \n
#// Parameters: \n
#// 	- \b Param[in] \b $i = Current part number is passed as first argument \n
#// 	- \b Param[in] \b $j = Input number \n
#//	
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

#// \b sub \b frontEndCode -------------------------------------------------------------\n
#// Return front end code \n
#// Argument 1 is the part number \n
#// Returns calculated code string \n\n
sub frontEndCode {
        my ($i) = @_;

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
	# Code only generated if RCV module
        if ($::partInputType[$i][0] eq "GROUND") {
		my $ss = substr($::xpartName[$i],3);
		$ss =~ s/\:/_/;
		$ss =~ s/\-/_/;
           $calcExp = "// IPCx RCVR DIAGNOSTICS:  $::xpartName[$i]\n";
	   $calcExp .= "if(!cycle && ipcInfo[$index]\.errTotal) {\n";
	   $calcExp .= "\tpLocalEpics->$::systemName\.$ss\_ET = ";
           $calcExp .= "timeSec;\n";
	   $calcExp .= "\tpLocalEpics->$::systemName\.$ss\_ER = ";
           $calcExp .= "ipcInfo[$index]\.errTotal;\n";
	   $calcExp .= "\tpLocalEpics->$::systemName\.$ss\_PS = 0;\n";
	   $calcExp .= "} \n";
	   $calcExp .= "if(!cycle && pLocalEpics->epicsInput.ipcDiagReset) { \n";
	   $calcExp .= "\t pLocalEpics->$::systemName\.$ss\_PS = 1;\n";
	   $calcExp .= "\t pLocalEpics->$::systemName\.$ss\_ET = 0;\n";
	   $calcExp .= "\t pLocalEpics->$::systemName\.$ss\_ER = 0;\n";
	   $calcExp .= "} \n";
        }
        else {

           $calcExp = "// IPCx:  $::xpartName[$i]\n";
           $calcExp .= "ipcInfo[$index]\.data = ";
           $calcExp .= "$::fromExp[0];\n";
        }

        return $calcExp;
}


#// \b sub \b procIpc ------------------------------------------------------------------\n
#// This sub will parse through all parts looking for IPC parts. \n
#// Once identified, code will determine all the parameters necessary to set up
#// the communications table. \n
#// Argument passed is total model part count. \n\n
sub procIpc {

my ($i) = @_;
# IPC parameter list
my @ipcxMissing = ("Signal Name","ipcType","ipcRate","ipcHost","ipcNum","ipcModel");
# IPC comm network types
my @ipcxType = ("SHMEM","RFM0","RFM1","PCIE");
my @ipcxMaxNum = (-999,-999,-999,-999);

#
# Loop thru all parts and find all IPCx parts and start building IPCx parts matrix
#
for ($ii = 0; $ii < $i; $ii++) {
   if ($::partType[$ii] =~ /^IPCx/) {
      # Add signal name to info table
      $::ipcxParts[$::ipcxCnt][0] = $::xpartName[$ii];

      $::ipcxCommMech = substr($::ipcxBlockTags[$ii], 8, 4);
      # Add IPC net type to info table
      $::ipcxParts[$::ipcxCnt][1] = "I" . $::ipcxCommMech;
      # If comm type RFM, need to get the card number as possible to have 2
      if ($::ipcxCommMech eq "RFM") {
         if ($::blockDescr[$ii] =~ /^card=(\d)/) {
            $::ipcxParts[$::ipcxCnt][1] .= $1;
         }
         else {
            die "\n***ERROR: IPCx part of type RFM with NO card number\n";
         }
      }

      $::ipcxParts[$::ipcxCnt][2] = undef;
      # Add name of sending computer to info table
      $::ipcxParts[$::ipcxCnt][3] = $::targetHost;
      $::ipcxParts[$::ipcxCnt][4] = undef;
      # Add name of model to info table
      $::ipcxParts[$::ipcxCnt][5] = $::skeleton;
      # Save model part number to info table
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
} #End of loop thru all parts

# Continue if IPC parts were found
if ($::ipcxCnt > 0) {

   # Find the IPC count limits
   ($maxIpcCount, $maxRfmIpcCount) = CDS::Util::findDefine("src/include/commData2.h", "MAX_IPC", "MAX_IPC_RFM");

   #
   # This model does include IPCx parts, so extract location and
   # site from cdsParameters and read the IPCx parameter file
   #
   ("CDS::Parameters::printHeaderStruct") -> ($oo);


   # Develop name of IPC parameter file based on return from above.
        #my ($i) = @_;
	my $iFile = "/opt/rtcds/";
	$iFile .= $::location;
        $iFile .= "/";
        $iFile .= lc $::site;
	$iFile .= "/chans/ipc/";
	$iFile .= $::site;
	$iFile .= "\.ipc";
   # Open and input data from IPC parameter file
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
		# Create EPICS variable name for use in auto screen generation.
		my $pfile = substr($::mdlfile,5);
		my @fnam = split(/\./,$pfile);
		my $ov = uc($fnam[0]);
		my $ss = substr($::xpartName[$kk],3);
		$ss =~ s/\:/_/;
		$ss =~ s/\-/_/;

		my $eVar = $::site;
		$eVar .= ":";
		$eVar .= "FEC-";
		$eVar .= $::dcuId;
		$eVar .= "_IPC_";
		$eVar .= $ss;
		$::ipcxParts[$ii][8] = $eVar;
		$::ipcxParts[$ii][9] = 0;
		
      		 print "IPC $ii is $::ipcxParts[$ii][8] $::systemName $ov $ss \n";
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
		$::ipcxParts[$ii][9] = 1;
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

	    # See if this is an RFM IPC and use the appropriate limit
	    my $myIpcLimit = $maxIpcCount;
	    if ($ipcxTypeIndex == 1 || $ipcTypeIndex == 2) {
		$myIpcLimit = $maxRfmIpcCount;
	    }

            $signalName = $::ipcxParts[$ipcxAdd[$jj][0]][0];
            if ($signalName =~ /^\w+([A-Z]\d\:.+)/) {
               $signalName = $1;
            }

            #
            # Add data to the IPCx parameter file
            #
            if (++$ipcxMaxNum[$ipcxTypeIndex] > $myIpcLimit) {
               die "***ERROR: IPC signal = $signalName; IPC number = $ipcxMaxNum[$ipcxTypeIndex]\n***ERROR: IPCx number > $myIpcLimit for ipcType = $ipcxType[$ipcxTypeIndex]\n";
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

# Subroutine to create IPC RCV status screen for all models
sub createIpcMedm 
{
my ($medmDir,$mdlName,$site,$dcuid,$medmTarget,$ipcxCnt1) = @_;
	# Define colors to be sent to screen gen.
	my %ecolors = ( "white" => "0",
             "black" => "14",
	     "red" => "20",
	     "green" => "60",
	     "blue" => "54"
           );

	# Calculate screen height based on number of IPC RCV signals
	my $dispH = 50;
	for(my $ii=0;$ii<$ipcxCnt1;$ii++)
	{
		if($::ipcxParts[$ii][9] == 0)
		{
			$dispH += 20;
		}
	}
       my $fname = "$mdlName\_IPC_STATUS.adl";
        # Create MEDM File
        print "creating file $medmDir\/$fname \n";
        open(OUTMEDM, ">$medmDir/$fname") || die "cannot open $medmDir/$fname for writing ";


	# Generate the base screen file, with name and height/width information
	$medmdata = ("CDS::medmGen::medmGenFile") -> ($medmDir,$fname,"740",$dispH);
	my $xpos = 0;
	my $ypos = 0;
	my $width = 740;
	my $height = 22;
	# Put blue rectangle banner at top of screen
	$medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{blue});
	# Add time string to banner
	$medmdata .= ("CDS::medmGen::medmGenTextMon") -> ("540","3","160","15","$::site\:FEC-$::dcuId\_TIME_STRING",$ecolors{white},$ecolors{blue});
	# Add screen title to banner
	$medmdata .= ("CDS::medmGen::medmGenText") -> ("310","3","100","15","IPC RCV STATUS",$ecolors{white});
	# Add the IPC column headings
	$medmdata .= ("CDS::medmGen::medmGenText") -> ("50","30","100","15","SIGNAL NAME",$ecolors{black});
	$medmdata .= ("CDS::medmGen::medmGenText") -> ("220","30","100","15","SEND COMP",$ecolors{black});
	$medmdata .= ("CDS::medmGen::medmGenText") -> ("320","30","100","15","SENDER MODEL",$ecolors{black});
	$medmdata .= ("CDS::medmGen::medmGenText") -> ("420","30","100","15","IPC TYPE",$ecolors{black});
	$medmdata .= ("CDS::medmGen::medmGenText") -> ("518","30","100","15","ERR/SEC",$ecolors{black});
	$medmdata .= ("CDS::medmGen::medmGenText") -> ("30","100","15","ERR TIME",$ecolors{black});
	#print "My IPC count = @_[1]\n";
	$ypos = 50;
	$width = 50;
	$height = 15;
	# Place IPC info into the screen for each IPC RCV signal
	for($ii=0;$ii<$ipcxCnt1;$ii++)
	{
		$xpos = 40;
		# Verify that this is a RCV signal
		if($::ipcxParts[$ii][9] == 0)
		{
			# Add signal name to screen file.
			$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,"140",$height,$::ipcxParts[$ii][0],$ecolors{black});
			$xpos += 200;
			# Add name of sending computer to screen file.
			$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,$::ipcxParts[$ii][3],$ecolors{black});
			$xpos += 100;
			# Add name of sending model to screen file.
			$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,$::ipcxParts[$ii][5],$ecolors{black});
			$xpos += 100;
			# Add IPC type to screen file.
			$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,$::ipcxParts[$ii][1],$ecolors{black});
			$xpos += 80;
			# Add IPC status byte to screen file; holds util diag reset.
			$medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,"15",$height,"$::ipcxParts[$ii][8]\_PS","0","0",$ecolors{green},$ecolors{red});
			$xpos += 20;
			# Add IPC errors/sec to screen file; holds until diag reset, unless errors are continuing.
			$medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$::ipcxParts[$ii][8]\_ER",$ecolors{white},$ecolors{black});
			$xpos += 70;
			$width = 100;
			# Add time of last detected IPC errors to screen file; holds until diag reset, unless errors are continuing.
			$medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$::ipcxParts[$ii][8]\_ET",$ecolors{white},$ecolors{black});
			$ypos += 20;
			$width = 50;
		}
	}

# Write data to file and close file.
print OUTMEDM "$medmdata \n";
close OUTMEDM;

}
