package CDS::Dac20;
use Exporter;
@ISA = ('Exporter');

require "lib/medmGen.pm";

#//     \page Dac20 Dac20.pm
#//     Dac20.pm - provides RCG support for General Standards 20 bit, 8 channel DAC modules.
#//
#// \n
#// \n

# DAC cards we support
%board_types = (
	GSC_16AO16 => 1, # General Standards board
        GSC_18AO8 => 1, # 18-bit General Standards DAC board
        GSC_20AO8 => 1 # 20-bit General Standards DAC board
);
$default_board_type = "GSC_16AO16";

#// \n \n
#// \b sub \b initDac \n 
#// Called by Parser3.pm to check if ADC supported and fill in ADC info. \n
#// See Parser3.pm function sortDacs(), where the information in global arrays is sorted \n
#// if there is a new global array introduced, it will need to be addressed in sortDacs() \n\n
#
sub initDac {
        my ($node) = @_;
        $::dacPartNum[$::dacCnt] = $::partCnt;
        for (0 .. 7) {
          $::partInput[$::partCnt][$_] = "NC";
        }

	my $desc = ${$node->{FIELDS}}{"Description"};
	printf "DAC PART TYPE description `$desc'\n";
           my ($type) = $desc =~ m/type=([^,]+)/g;
	printf "DAC PART TYPE $type\n";
	my ($num) = $desc =~ m/card_num=([^,]+)/g;
	if ($type eq undef) {
		$type = $default_board_type;
	}
	if ($num eq undef) {
		$num = $::dacCnt;
	}
	print "DAC $::dacCnt; type=$type; num=$num\n";
	
	# Check if this is a supported board type
	if ($board_types{$type} != 1) {
		print "Unsupported board type\n";
		print "Known board types:\n";
		foreach (keys %board_types) {
			print "\t$_\n";
		}
		exit 1;
	}

        $::dacType[$::dacCnt] = $type;
        $::dacNum[$::dacCnt] = $num;
	$::card2array[$::partCnt] = $::dacCnt;
        $::dacCnt++;
}


#// \b sub \b partType \n 
#// Required subroutine for RCG \n
#// Returns Dac20 \n\n
sub partType {
	return Dac20;
}

#// \b sub \b printHeaderStruct \n 
#// Required subroutine for RCG \n
#// Print Epics communication structure into a header file \n
#// Current part number is passed as first argument \n
#// For DAC part, nothing required to be placed in the Epics comms struct. \n\n
sub printHeaderStruct {
        my ($i) = @_;
	;
}

#// \b sub \b printEpics \n 
#// Required subroutine for RCG \n
#// Print Epics variable definitions \n
#// Current part number is passed as first argument \n
#// For DAC part, there are no EPICS records generated. \n\n
sub printEpics {
        my ($i) = @_;
        ; 
}


#// \b sub \b printFrontEndVars \n 
#// Required subroutine for RCG \n
#// Print variable declarations int front-end file \n
#// Current part number is passed as first argument \n\n
sub printFrontEndVars  {
        my ($i) = @_;
        ;
}

# Check inputs are connected
sub checkInputConnect {
        my ($i) = @_;
        return "";
}

#// \b sub \b frontEndInitCode \n 
#// Required subroutine for RCG \n
#// Return front end initialization code \n
#// Argument 1 is the part number \n
#// Returns calculated code string \n \n
sub frontEndInitCode {
	my ($i) = @_;
	my $dacNum = $::card2array[$i];
        my $calcExp = "// DAC number is $dacNum\n";
        for (0 .. 7) {
          my $fromType = $::partInputType[$i][$_];
          if (($fromType ne "GROUND") && ($::partInput[$i][$_] ne "NC")) {
                $calcExp .= "dacOutUsed\[";
                $calcExp .= $dacNum;
                $calcExp .= "\]\[";
                $calcExp .= $_;
                $calcExp .= "\] =  1;\n";
          }
        }
	return $calcExp;
        return "";
}

#// \b sub \b fromExp \n 
#// Required subroutine for RCG \n
#// Figure out part input code \n
#// Argument 1 is the part number \n
#// Argument 2 is the input number \n
#// Returns calculated input code \n\n
sub fromExp {
        my ($i, $j) = @_;
        return "";
}


#// \b sub \b frontEndCode \n 
#// Required subroutine for RCG \n
#// Return front end code \n
#// Argument 1 is the part number \n
#// Returns calculated code string \n\n
sub frontEndCode {
	my ($i) = @_;
	my $dacNum = $::card2array[$i];
        my $calcExp = "// DAC number is $dacNum\n";
        for (0 .. 7) {
          my $fromType = $::partInputType[$i][$_];
          if (($fromType ne "GROUND") && ($::partInput[$i][$_] ne "NC")) {
                $calcExp .= "dacOut\[";
                $calcExp .= $dacNum;
                $calcExp .= "\]\[";
                # $calcExp .= $::partOutputPort[$i][$_];
                $calcExp .= $_;
                $calcExp .= "\] = ";
                $calcExp .= $::fromExp[$_];
                $calcExp .= ";\n";
          }
        }
	return $calcExp;
}


#// \b sub \b createDac20Medm \n 
#// Called by feCodeGen.pl to auto gen DAC20 channel MEDM screens \n
#// This code requires /lib/medmGen.pm \n\n
sub createDac20Medm
{
        my ($medmDir,$mdlName,$site,$dcuid,$medmTarget,$dacNum) = @_;
 # Define colors to be sent to screen gen.
        my %ecolors = ( "white" => "0",
             "black" => "14",
             "red" => "20",
             "green" => "60",
             "blue" => "54",
             "brown" => "34",
             "gray" => "2",
             "ltblue" => "50",
             "mdblue" => "42",
             "dacblue" => "44",
             "yellow" => "55"
           );

	my $ii=0;

        my $fname = "$mdlName\_DAC_MONITOR_$dacNum.adl";
        # Create MEDM File
        print "creating file $medmDir\/$fname \n";
        open(OUTMEDM, ">$medmDir/$fname") || die "cannot open $medmDir/$fname for writing ";

        my $xpos = 0; my $ypos = 0; my $width = 235; my $height = 275;
        $medmdata = ("CDS::medmGen::medmGenFile") -> ($medmDir,$fname,$width,$height);

	# ************* Create Banner ******************************************************************************
        # Put blue rectangle banner at top of screen
        $height = 22;
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{blue},"","","");
        # Add Display Name
        $xpos = 55; $ypos = 4; $width = 120; $height = 15;        
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"$mdlName\_DAC_MONITOR_$::dacCardNum[$dacNum]",$ecolors{white});

        # ************* Create Background **************************************************************************
        # Add Background rectangles
        $xpos = 13; $ypos = 27; $width = 210; $height = 238;
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{gray},"","","");

        # ************* Add Text  **********************************************************************************
        # Add DAC top label
        $xpos = 46; $ypos = 32; $width = 140; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"DAC $::dacCardNum[$dacNum] - 20bit",$ecolors{black});
        # Add DAC OUT label
        $xpos = 49; $ypos = 52; $width = 45; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"OUT",$ecolors{black});
        # Add OFC OUT label
        $xpos = 134; $ypos = 52; $width = 45; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"OVERFLOW",$ecolors{black});
        $xpos = 108; $ypos = 70; $width = 45; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"PER SEC",$ecolors{black});
        $xpos = 160; $ypos = 70; $width = 45; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"ACCUM",$ecolors{black});

        # Add DAC Channel labels
        $xpos = 14; $ypos = 103; $width = 35; $height = 15;
	for($ii=0;$ii<8;$ii+=2)
	{
		$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"CH $ii",$ecolors{black});
		$ypos += 40;
	}

        # ************* Add Data Monitors  ***************************************************************************
        # Add Output Status Monitor
        $xpos = 49; $ypos = 88; $width = 45; $height = 12;
        $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_DAC_STAT_$dacNum","1","1",$ecolors{green},$ecolors{red});
        # Add Overflow Status Monitor
        $xpos = 109; $ypos = 88; $width = 45; $height = 12;
        $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_DAC_STAT_$dacNum","2","2",$ecolors{green},$ecolors{red});

        # Add DAC Data Monitors
        $xpos = 49; $ypos = 102; $width = 45; $height = 15;
	for($ii=0;$ii<8;$ii++)
	{
        	$medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_DAC_OUTPUT_$dacNum\_$ii",$ecolors{white},$ecolors{blue},"static");
		$ypos += 20;
	}
        $xpos = 109; $ypos = 102; $width = 45; $height = 15;
	for($ii=0;$ii<8;$ii++)
	{
        	$medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_DAC_OVERFLOW_$dacNum\_$ii",$ecolors{white},$ecolors{blue},"static");
		$ypos += 20;
	}
        $xpos = 159; $ypos = 102; $width = 55; $height = 15;
	for($ii=0;$ii<8;$ii++)
	{
        	$medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_DAC_OVERFLOW_ACC_$dacNum\_$ii",$ecolors{white},$ecolors{blue},"static");
		$ypos += 20;
	}

print OUTMEDM "$medmdata \n";
close OUTMEDM;


}
