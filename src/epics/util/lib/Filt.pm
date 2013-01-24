package CDS::Filt;
use Exporter;
@ISA = ('Exporter');

#//     \file Filt.dox
#//     \brief Documentation for Filt.pm
#//
#// \n
#//     \subpage devguidercg2 "<<-- Parts Library"
#// \n


sub partType {
#	if (length $::xpartName[$::partCnt] > $::max_name_len) {
#		die "Filter name \"", $::xpartName[$::partCnt], "\" too long (max $::max_name_len charachters)";
#	}
        print ::OUTH "#define $::xpartName[$::partCnt] \t $::filtCnt\n";
	if ($::allBiquad || $::biQuad[$::partCnt]) { 
        	print ::EPICS "$::xpartName[$::partCnt] biquad\n";
	} else {
        	print ::EPICS "$::xpartName[$::partCnt]\n";
	}
        $::filterName[$::filtCnt] = $::xpartName[$::partCnt];
        $::filtCnt ++;

	return Filt;
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
        print ::OUT "double \L$::xpartName[$i] = 0.0;\n";
}

# Return front end initialization code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndInitCode {
        my ($i) = @_;
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
        my $calcExp = "// FILTER MODULE";
        if ($::ppFIR[$i] == 1) {
           $calcExp .= " (PolyPhase FIR)";
        }
        $calcExp .= ":  $::xpartName[$i]\n";

        $calcExp .= "\L$::xpartName[$i]";
        $calcExp .= " = ";
        if ($::cpus > 2) {
             $calcExp .= "filterModuleD(dspPtr[0],dspCoeff,";
        } else {
             $calcExp .= "filterModuleD(dsp_ptr,dspCoeff,";
        }
        $calcExp .= $::xpartName[$i];
        $calcExp .= ",";
        $calcExp .= $::fromExp[0];
        $calcExp .= ",0,0);\n";
        return $calcExp;
}
sub createFiltMedm
{
        my ($medmDir,$mdlName,$site,$dcuid,$medmTarget,$filterName,$filtChan,$relDisp) = @_;
 # Define colors to be sent to screen gen.
        my %ecolors = ( "white" => "0",
             "black" => "14",
             "red" => "20",
             "green" => "60",
             "blue" => "54",
             "brown" => "34",
             "gray" => "8",
             "ltgray" => "4",
             "ltblue" => "50",
             "mdblue" => "42",
             "dacblue" => "44",
             "dyellow" => "30",
             "yellow" => "55"
           );

        my $ii=0;

        my $fname = "$mdlName\_$filterName.adl";
        # Create MEDM File
        my $xpos = 0; my $ypos = 0; my $width = 979; my $height = 187;
        ("CDS::medmGen::medmGenFile") -> ($medmDir,$fname,$width,$height);
	# ************* Create Banner ******************************************************************************
        # Put black rectangle banner at top of screen
        $height = 22;
        ("CDS::medmGen::medmGenRectangle") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,$ecolors{black},"","","");
        # Add Display Name
        $xpos = 450; $ypos = 4; $width = 120; $height = 15;
	$title = substr $mdlName,0,5;
        ("CDS::medmGen::medmGenText") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$title\-$filterName",$ecolors{white});
        # Add IFO Name
        $xpos = 8; $ypos = 4; $width = 40; $height = 15;
	$title = substr $mdlName,0,2;
        ("CDS::medmGen::medmGenText") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,$title,$ecolors{white});
	# Add time string to banner
        $xpos = 770; $ypos = 4; $width = 200; $height = 15;
        ("CDS::medmGen::medmGenTextMon") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_TIME_STRING",$ecolors{white},$ecolors{black},"static");

 	# ************* Create Background **************************************************************************
        # Add Background rectangles
        $xpos = 183; $ypos = 29; $width = 680; $height = 150;
        ("CDS::medmGen::medmGenRectangle") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,$ecolors{gray},"","","");

	# Add Lines
        $xpos = 87; $ypos = 118; $width = 824; $height = 3;
        ("CDS::medmGen::medmGenLine") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"3",$ecolors{black});
        $xpos = 169; $ypos = 53; $width = 3; $height = 65;
        ("CDS::medmGen::medmGenLine") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"3",$ecolors{black});
        $xpos = 119; $ypos = 53; $width = 52; $height = 3;
        ("CDS::medmGen::medmGenLine") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"3",$ecolors{black});
        $xpos = 726; $ypos = 119; $width = 3; $height = 40;
        ("CDS::medmGen::medmGenLine") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"3",$ecolors{black});
        $xpos = 825; $ypos = 87; $width = 3; $height = 33;
        ("CDS::medmGen::medmGenLine") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"3",$ecolors{black});
        $xpos = 824; $ypos = 85; $width = 56; $height = 3;
        ("CDS::medmGen::medmGenLine") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"3",$ecolors{black});
        $xpos = 726; $ypos = 156; $width = 156; $height = 3;
        ("CDS::medmGen::medmGenLine") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"3",$ecolors{black});


	# Add EXCMON Components
        # Add EXCMON Label
        $xpos = 85; $ypos = 25; $width = 20; $height = 15;
        ("CDS::medmGen::medmGenText") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"EXCMON",$ecolors{black});
        $xpos = 150; $ypos = 25; $width = 40; $height = 15;
        ("CDS::medmGen::medmGenText") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"EXC",$ecolors{brown});
        $xpos = 164; $ypos = 38; $width = 11; $height = 16;
	@xpts = (164,38,175,38,169,54,164,38);
        ("CDS::medmGen::medmGenTriangle") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"3",$ecolors{brown},@xpts);
	# EXCMON
        $xpos = 57; $ypos = 46; $width = 77; $height = 15;
	("CDS::medmGen::medmGenTextMon") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_EXCMON",$ecolors{white},$ecolors{black},"alarm");

	# Add INMON Components
	# IN1 Arrow
        $xpos = 96; $ypos = 88; $width = 12; $height = 17;
	@xpts = (108,105,96,105,102,88,108,105);
        ("CDS::medmGen::medmGenTriangle") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"3",$ecolors{yellow},@xpts);
	# Add IN1 Text
        $xpos = 91; $ypos = 74; $width = 20; $height = 15;
        ("CDS::medmGen::medmGenText") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"IN1",$ecolors{yellow});
	# Add INMON Text
        $xpos = 50; $ypos = 89; $width = 20; $height = 15;
        ("CDS::medmGen::medmGenText") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"INMON",$ecolors{black});
	#INMON Reading
        $xpos = 20; $ypos = 110; $width = 77; $height = 15;
	("CDS::medmGen::medmGenTextMon") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_INMON",$ecolors{white},$ecolors{black},"alarm");

	# Add OUT16 Decimation ON/OFF button
        $xpos = 795; $ypos = 69; $width = 61; $height = 20;
        ("CDS::medmGen::medmGenMessage") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_SW2",$ecolors{black},$ecolors{ltgray},"DECIMATION","512");
       # Add OUT16 Decimation  ON/OFF Monitor
        $xpos = 800; $ypos = 89; $width = 51; $height = 7;
        ("CDS::medmGen::medmGenByte") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_SW2R","9","9",$ecolors{green},$ecolors{red});
	# Add OUT16 HOLD ON/OFF button
        $xpos = 880; $ypos = 36; $width = 72; $height = 20;
        ("CDS::medmGen::medmGenMessage") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_SW2",$ecolors{black},$ecolors{ltgray},"HOLD OUTPUT","2048");
       # Add OUT16 HOLD  ON/OFF Monitor
        $xpos = 890; $ypos = 56; $width = 51; $height = 7;
        ("CDS::medmGen::medmGenByte") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_SW2R","11","11",$ecolors{green},$ecolors{red});
	# Add OUT16 Text
        $xpos = 907; $ypos = 94; $width = 20; $height = 15;
        ("CDS::medmGen::medmGenText") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"OUT16",$ecolors{black});
	#OUT16
        $xpos = 879; $ypos = 78; $width = 77; $height = 15;
	("CDS::medmGen::medmGenTextMon") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_OUT16",$ecolors{white},$ecolors{black},"alarm");

	# OUT TP Arrow
        $xpos = 718; $ypos = 88; $width = 12; $height = 17;
	@xpts = (718,104,730,104,724,88,718,104);
        ("CDS::medmGen::medmGenTriangle") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"3",$ecolors{yellow},@xpts);
	# Add OUT Text
        $xpos = 695; $ypos = 75; $width = 60; $height = 15;
        ("CDS::medmGen::medmGenText") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"OUT",$ecolors{yellow});
	# Add OUTPUT Text
        $xpos = 907; $ypos = 128; $width = 20; $height = 15;
        ("CDS::medmGen::medmGenText") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"OUTPUT",$ecolors{black});
	#OUTPUT
        $xpos = 879; $ypos = 110; $width = 77; $height = 15;
	("CDS::medmGen::medmGenTextMon") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_OUTPUT",$ecolors{white},$ecolors{black},"alarm");

	# Add OUTMON Text
        $xpos = 907; $ypos = 168; $width = 20; $height = 15;
        ("CDS::medmGen::medmGenText") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"OUTMON",$ecolors{black});
	#OUTMON
        $xpos = 879; $ypos = 150; $width = 77; $height = 15;
	("CDS::medmGen::medmGenTextMon") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_OUTMON",$ecolors{white},$ecolors{black},"alarm");

	# Add INPUT ON/OFF button
        $xpos = 105; $ypos = 108; $width = 47; $height = 20;
        ("CDS::medmGen::medmGenMessage") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_SW1",$ecolors{black},$ecolors{ltgray},"ON/OFF","4");
       # Add INPUT ON/OFF Monitor
        $xpos = 101; $ypos = 128; $width = 51; $height = 7;
        ("CDS::medmGen::medmGenByte") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_SW1R","2","2",$ecolors{green},$ecolors{red});

	# Add OFFSET Components
	# Add IN2 arrow
        $xpos = 184; $ypos = 88; $width = 12; $height = 17;
	@xpts = (200,105,188,105,194,88,200,105);
        ("CDS::medmGen::medmGenTriangle") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"3",$ecolors{yellow},@xpts);
	# Add IN2 Text
        $xpos = 170; $ypos = 74; $width = 47; $height = 15;
        ("CDS::medmGen::medmGenText") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"IN2",$ecolors{yellow});
	# OFFSET Input
        $xpos = 204; $ypos = 108; $width = 50; $height = 20;
	("CDS::medmGen::medmGenTextEntry") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_OFFSET",$ecolors{white},$ecolors{black},"alarm");
	# Add OFFSET ON/OFF button
        $xpos = 205; $ypos = 85; $width = 47; $height = 20;
        ("CDS::medmGen::medmGenMessage") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_SW1",$ecolors{black},$ecolors{ltgray},"OFFSET","8");
       # Add OFFSET ON/OFF Monitor
        $xpos = 202; $ypos = 131; $width = 51; $height = 7;
        ("CDS::medmGen::medmGenByte") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_SW1R","3","3",$ecolors{green},$ecolors{red});
        # OFFSET RAMP INDICATOR
        $xpos = 200; $ypos = 82; $width = 57; $height = 70;
        ("CDS::medmGen::medmGenRectangle") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,$ecolors{dyellow},"calc","A&8192","$filtChan\_SW2R");

	# GAIN RECTANGLE
        $xpos = 556; $ypos = 94; $width = 69; $height = 50;
	@xpts = (556,95,556,145,625,119,556,95);
        ("CDS::medmGen::medmGenTriangle") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"3",$ecolors{black},@xpts);
	# GAIN RAMP RECTANGLE
        $xpos = 556; $ypos = 94; $width = 69; $height = 50;
	@xpts = (556,95,556,145,625,119,556,95);
        ("CDS::medmGen::medmGenTriangleDyn") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"3",$ecolors{dyellow},"A&4096","$filtChan\_SW2R",@xpts);
	# GAIN Input
        $xpos = 556; $ypos = 110; $width = 50; $height = 20;
	("CDS::medmGen::medmGenTextEntry") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_GAIN",$ecolors{white},$ecolors{black},"alarm");

	# LIMIT Input
        $xpos = 631; $ypos = 110; $width = 50; $height = 20;
	("CDS::medmGen::medmGenTextEntry") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_LIMIT",$ecolors{white},$ecolors{black},"alarm");
	# Add LIMIT ON/OFF button
        $xpos = 632; $ypos = 85; $width = 47; $height = 20;
        ("CDS::medmGen::medmGenMessage") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_SW2",$ecolors{black},$ecolors{ltgray},"LIMIT","256");
       # Add LIMIT ON/OFF Monitor
        $xpos = 630; $ypos = 131; $width = 51; $height = 7;
        ("CDS::medmGen::medmGenByte") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_SW2R","8","8",$ecolors{green},$ecolors{red});

        # Add RAMP Time Label
        $xpos = 560; $ypos = 157; $width = 76; $height = 18;
        ("CDS::medmGen::medmGenText") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"Ramp Time (sec):",$ecolors{yellow});
	# RAMP TIME Input
        $xpos = 657; $ypos = 152; $width = 50; $height = 20;
	("CDS::medmGen::medmGenTextEntry") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_TRAMP",$ecolors{white},$ecolors{black},"alarm");

	# Add OUTPUT ON/OFF button
        $xpos = 753; $ypos = 108; $width = 47; $height = 20;
        ("CDS::medmGen::medmGenMessage") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_SW2",$ecolors{black},$ecolors{ltgray},"ON/OFF","1024");
       # Add OUTPUT ON/OFF Monitor
        $xpos = 750; $ypos = 128; $width = 51; $height = 7;
        ("CDS::medmGen::medmGenByte") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_SW2R","10","10",$ecolors{green},$ecolors{red});


	# Add FILTER ON/OFF Buttons
	# Add FM1 ON/OFF button
        $xpos = 290; $ypos = 81; $width = 46; $height = 22;
        ("CDS::medmGen::medmGenMessage") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_SW1",$ecolors{black},$ecolors{ltgray},"FM1","16");
        $xpos = 343; $ypos = 81; $width = 46; $height = 22;
        ("CDS::medmGen::medmGenMessage") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_SW1",$ecolors{black},$ecolors{ltgray},"FM2","64");
        $xpos = 395; $ypos = 81; $width = 46; $height = 22;
        ("CDS::medmGen::medmGenMessage") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_SW1",$ecolors{black},$ecolors{ltgray},"FM3","256");
        $xpos = 446; $ypos = 81; $width = 46; $height = 22;
        ("CDS::medmGen::medmGenMessage") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_SW1",$ecolors{black},$ecolors{ltgray},"FM4","1024");
        $xpos = 498; $ypos = 81; $width = 46; $height = 22;
        ("CDS::medmGen::medmGenMessage") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_SW1",$ecolors{black},$ecolors{ltgray},"FM5","4096");
        $xpos = 290; $ypos = 140; $width = 46; $height = 22;
        ("CDS::medmGen::medmGenMessage") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_SW1",$ecolors{black},$ecolors{ltgray},"FM6","16384");
        $xpos = 343; $ypos = 140; $width = 46; $height = 22;
        ("CDS::medmGen::medmGenMessage") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_SW2",$ecolors{black},$ecolors{ltgray},"FM7","1");
        $xpos = 395; $ypos = 140; $width = 46; $height = 22;
        ("CDS::medmGen::medmGenMessage") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_SW2",$ecolors{black},$ecolors{ltgray},"FM8","4");
        $xpos = 446; $ypos = 140; $width = 46; $height = 22;
        ("CDS::medmGen::medmGenMessage") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_SW2",$ecolors{black},$ecolors{ltgray},"FM9","16");
        $xpos = 498; $ypos = 140; $width = 46; $height = 22;
        ("CDS::medmGen::medmGenMessage") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_SW2",$ecolors{black},$ecolors{ltgray},"FM10","64");
       # Add FILTER ON/OFF Monitors
        $xpos = 288; $ypos = 103; $width = 256; $height = 11;
        ("CDS::medmGen::medmGenByte") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_SW1R","4","13",$ecolors{green},$ecolors{red});
        $xpos = 288; $ypos = 162; $width = 51; $height = 11;
        ("CDS::medmGen::medmGenByte") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_SW1R","14","15",$ecolors{green},$ecolors{red});
        $xpos = 340; $ypos = 162; $width = 205; $height = 11;
        ("CDS::medmGen::medmGenByte") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_SW2R","0","7",$ecolors{green},$ecolors{red});
       # Add FILTER NAME Monitor
        $xpos = 288; $ypos = 64; $width = 51; $height = 15;
	("CDS::medmGen::medmGenTextMon") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_Name00",$ecolors{black},$ecolors{ltgray},"static");
        $xpos = 342; $ypos = 64; $width = 51; $height = 15;
	("CDS::medmGen::medmGenTextMon") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_Name01",$ecolors{black},$ecolors{ltgray},"static");
        $xpos = 394; $ypos = 64; $width = 51; $height = 15;
	("CDS::medmGen::medmGenTextMon") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_Name02",$ecolors{black},$ecolors{ltgray},"static");
        $xpos = 447; $ypos = 64; $width = 51; $height = 15;
	("CDS::medmGen::medmGenTextMon") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_Name03",$ecolors{black},$ecolors{ltgray},"static");
        $xpos = 499; $ypos = 64; $width = 51; $height = 15;
	("CDS::medmGen::medmGenTextMon") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_Name04",$ecolors{black},$ecolors{ltgray},"static");
        $xpos = 288; $ypos = 123; $width = 51; $height = 15;
	("CDS::medmGen::medmGenTextMon") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_Name05",$ecolors{black},$ecolors{ltgray},"static");
        $xpos = 342; $ypos = 123; $width = 51; $height = 15;
	("CDS::medmGen::medmGenTextMon") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_Name06",$ecolors{black},$ecolors{ltgray},"static");
        $xpos = 394; $ypos = 123; $width = 51; $height = 15;
	("CDS::medmGen::medmGenTextMon") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_Name07",$ecolors{black},$ecolors{ltgray},"static");
        $xpos = 447; $ypos = 123; $width = 51; $height = 15;
	("CDS::medmGen::medmGenTextMon") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_Name08",$ecolors{black},$ecolors{ltgray},"static");
        $xpos = 499; $ypos = 123; $width = 51; $height = 15;
	("CDS::medmGen::medmGenTextMon") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_Name09",$ecolors{black},$ecolors{ltgray},"static");

	# CLEAR HISTORY
        $xpos = 282; $ypos = 33; $width = 205; $height = 25;
        ("CDS::medmGen::medmGenMessage") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_RSET",$ecolors{black},$ecolors{ltgray},"CLEAR HISTORY","2");
        $xpos = 487; $ypos = 33; $width = 205; $height = 25;
        ("CDS::medmGen::medmGenMessage") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$filtChan\_RSET",$ecolors{black},$ecolors{ltgray},"LOAD COEFFICIENTS","1");

	#Add Snapshot Shell commands
        $xpos = 4; $ypos = 165; $width = 75; $height = 20;
	$largs[0] = "Update SnapShot";
	$largs[1] = "Current SnapShot";
	$largs[2] = "Previous SnapShot";
	$cmd = "$relDisp/src/epics/util/medmsnap.pl";
	@nargs = ($cmd,$cmd,$cmd);
	$aargs[0] = "U &A &X";
	$aargs[1] = "V &A";
	$aargs[2] = "P &A";
        ("CDS::medmGen::medmGenShellCmd") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,$ecolors{black},$ecolors{yellow},"3","Snapshots",@largs,@nargs,@aargs);

	 # Add FILTALH related display
        $xpos = 81; $ypos = 165; $width = 100; $height = 20;
	$relDisp .= "/src/epics/util/FILTALH.adl";
	$filtName = substr $filtChan, 3;
	$dargs = "FPREFIX=$site,FNAME=$filtName,DCUID=$dcuid";
        ("CDS::medmGen::medmGenRelDisp") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,$relDisp,$ecolors{white},$ecolors{brown},"GUARDIAN SET",$dargs);

}

