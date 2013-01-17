package CDS::Dac;
use Exporter;
@ISA = ('Exporter');

require "lib/medmGen.pm";

#//     \file Dac.dox
#//     \brief Documentation for Dac.pm
#//
#// \n
#//     \subpage devguidercg2 "<<-- Parts Library"
#// \n

# DAC cards we support
%board_types = (
	GSC_16AO16 => 1, # General Standards board
        GSC_18AO8 => 1 # 18-bit General Standards DAC board
);
$default_board_type = "GSC_16AO16";


# See Parser3.pm function sortDacs(), where the information in global arrays is sorted
# if there is a new global array introduced, it will need to be addressed in sortDacs()
#
sub initDac {
        my ($node) = @_;
        $::dacPartNum[$::dacCnt] = $::partCnt;
        for (0 .. 15) {
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


sub partType {
	return Dac;
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


# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;
        ;
}

# Return front end initialization code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndInitCode {
	my ($i) = @_;
	my $dacNum = $::card2array[$i];
        my $calcExp = "// DAC number is $dacNum\n";
        for (0 .. 15) {
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

# Figure out part input code
# Argument 1 is the part number
# Argument 2 is the input number
# Returns calculated input code
sub fromExp {
        my ($i, $j) = @_;
        return "";
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	my ($i) = @_;
	my $dacNum = $::card2array[$i];
        my $calcExp = "// DAC number is $dacNum\n";
        for (0 .. 15) {
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
sub createDac16Medm
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
        my $xpos = 0; my $ypos = 0; my $width = 171; my $height = 430;
        ("CDS::medmGen::medmGenFile") -> ($medmDir,$fname,$width,$height);

	# ************* Create Banner ******************************************************************************
        # Put blue rectangle banner at top of screen
        $height = 22;
        ("CDS::medmGen::medmGenRectangle") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,$ecolors{blue},"","","");
        # Add Display Name
        $xpos = 25; $ypos = 4; $width = 120; $height = 15;        
	("CDS::medmGen::medmGenText") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$mdlName\_DAC_MONITOR_$dacNum",$ecolors{white});

        # ************* Create Background **************************************************************************
        # Add Background rectangles
        $xpos = 13; $ypos = 37; $width = 140; $height = 380;
        ("CDS::medmGen::medmGenRectangle") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,$ecolors{gray},"","","");

        # ************* Add Text  **********************************************************************************
        # Add DAC top label
        $xpos = 13; $ypos = 43; $width = 140; $height = 15;
        ("CDS::medmGen::medmGenText") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"DAC $dacNum - 16bit",$ecolors{black});
        # Add DAC OUT label
        $xpos = 49; $ypos = 64; $width = 45; $height = 15;
        ("CDS::medmGen::medmGenText") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"OUT",$ecolors{black});
        # Add OFC OUT label
        $xpos = 99; $ypos = 64; $width = 45; $height = 15;
        ("CDS::medmGen::medmGenText") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"OFC",$ecolors{black});

        # Add DAC Channel labels
        $xpos = 14; $ypos = 93; $width = 35; $height = 15;
	for($ii=0;$ii<16;$ii+=2)
	{
		("CDS::medmGen::medmGenText") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"CH $ii",$ecolors{black});
		$ypos += 40;
	}

        # ************* Add Data Monitors  ***************************************************************************
        # Add Output Status Monitor
        $xpos = 62; $ypos = 77; $width = 20; $height = 12;
        ("CDS::medmGen::medmGenByte") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_DAC_STAT_$dacNum","1","1",$ecolors{green},$ecolors{red});
        # Add Overflow Status Monitor
        $xpos = 112; $ypos = 77; $width = 20; $height = 12;
        ("CDS::medmGen::medmGenByte") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_DAC_STAT_$dacNum","2","2",$ecolors{green},$ecolors{red});

        # Add DAC Data Monitors
        $xpos = 49; $ypos = 92; $width = 45; $height = 15;
	for($ii=0;$ii<16;$ii++)
	{
        	("CDS::medmGen::medmGenTextMon") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_DAC_OUTPUT_$dacNum\_$ii",$ecolors{white},$ecolors{blue},"static");
		$ypos += 20;
	}
        $xpos = 99; $ypos = 92; $width = 45; $height = 15;
	for($ii=0;$ii<16;$ii++)
	{
        	("CDS::medmGen::medmGenTextMon") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_DAC_OVERFLOW_$dacNum\_$ii",$ecolors{white},$ecolors{blue},"static");
		$ypos += 20;
	}


}
