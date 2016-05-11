package CDS::IPCmonitor;
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
	for(my $ii=0;$ii<20;$ii++) {
		my $varname = "_IPC_MON_$ii";
		# Add Err rate var ie errors/sec
		print ::EPICS "OUTVARIABLE $varname $::systemName\.$varname int ao 0\n";
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
           print ::OUT "#include \"commData3.h\"\n";
           print ::OUT "static int myIpcCount;\n";
           print ::OUT "static CDS_IPC_INFO ipcInfo[$::ipcxCnt];\n\n";
        }
}

# Check inputs are connected
sub checkInputConnect {
        my ($i) = @_;
        return "";
}

#// \b sub \b frontEndInitCode ---------------------------------------------------------\n
#//  Return front end initialization code for all IPC parts on first call\n
#// Parameters: \n
#// 	- \b Param[in] \b $i = Current part number is passed as first argument \n
#// 	- \b Param[out] \b $::ipcxInitDone = Flag indicating IPC init is complete \n
#//	
sub frontEndInitCode {
        my ($i) = @_;
 
	# Print this in code initialization area only once
           $calcExp = "\nmyIpcCount = $::ipcxCnt;\n\n";
 

	# Need to call commData3.c during initialization only once.
           $calcExp .= "\ncommData3InitSwitch(myIpcCount, FE_RATE, ipcInfo);\n\n";

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

# Continue if IPC parts were found

   # Find the IPC count limits
   ($maxIpcCount, $maxRfmIpcCount) = CDS::Util::findDefine("src/include/commData3.h", "MAX_IPC", "MAX_IPC_RFM");

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
   $::ipcxCnt = 0;
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

            if ($typeString =~ /^PCIE/) {
               	$typeIndex = 3;
            } else {
		$typeIndex = 0;
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
		if($typeIndex == 0) {
			$ipcxParamCnt --;
		}
         }
         elsif ($value =~ /^ipcModel=(.+)/) {
            $ipcxData[$ipcxParamCnt][5] = $1;
         }
      }
   }

   $ipcxParamCnt++;
	$::ipcxCnt = $ipcxParamCnt;
print "Found $::ipcxCnt IPC CONNECTS \n";


      for ($jj = 0; $jj < $::ipcxCnt; $jj++) {

            #
            # Make sure no IPCx parameters are missing
            #
            for ($kk = 0; $kk < 6; $kk++) {
               if (defined($ipcxData[$jj][$kk]) ) {
                  $::ipcxParts[$jj][$kk] = $ipcxData[$jj][$kk];
               } else {
                  $::ipcxParts[$jj][$kk] = "Unknown";
               }
            }
		if($jj<10) {
			print "IPC CONNECT LIST - Part $jj\n";
            	for ($kk = 0; $kk < 6; $kk++) {
			print "Field $kk = $::ipcxParts[$jj][$kk] \n";
		}
			
		}


            #
            # If the input to an IPCx part is 'Ground',
            # then this part is a RECEIVER of data,
            # which means there should be either one
            # or two outputs
            #
		# Create EPICS variable name for use in auto screen generation.
		my $pfile = substr($::mdlfile,5);
		my @fnam = split(/\./,$pfile);
		my $ov = uc($fnam[0]);
		my $ss = substr($::ipcxParts[$kk][0],3);
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
