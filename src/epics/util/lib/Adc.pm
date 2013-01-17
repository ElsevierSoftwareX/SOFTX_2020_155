package CDS::Adc;
use Exporter;
@ISA = ('Exporter');

require "lib/medmGen.pm";

#//     \file Adc.dox
#//     \brief Documentation for Adc.pm
#//
#// \n
#//     \subpage devguidercg2 "<<-- Parts Library"
#// \n

# ADC cards we support
%board_types = (
	GSC_16AI64SSA => 1, # Slow General Standards board
        GSC_18AISS6C => 1 # 18-bit 6 channel General Standards board
);

# default board type (if none specified with type=<type> in block Description)
$default_board_type = "GSC_16AI64SSA";

sub initAdc {
        my ($node) = @_;
        $::adcPartNum[$::adcCnt] = $::partCnt;
	# Set ADC type and number
	my $desc = ${$node->{FIELDS}}{"Description"};
	my ($type) = $desc =~ m/type=([^,]+)/g;
	my ($num) = $desc =~ m/card_num=([^,]+)/g;
	if ($type eq undef) {
		$type = $default_board_type;
	}
	if ($num eq undef) {
		$num = $::adcCnt;
	}
	print "ADC $::adcCnt; type='$type'; num=$num\n";
        #print "foo=$board_types{$type}\n";
	
	# Check if this is a supported board type
	if ($board_types{$type} != 1) {
		print "Unsupported board type\n";
		print "Known board types:\n";
		foreach (keys %board_types) {
			print "\t$_\n";
		}
		exit 1;
	}

        $::adcType[$::adcCnt] = $type;
        $::adcNum[$::adcCnt] = $num;
        $::adcCnt++;
        $::partUsed[$::partCnt] = 1;
        foreach (0 .. $::partCnt) {
          if ("Adc" eq $::partInputType[$_][0]) {
	  	print $_," ", $::xpartName[$_], "\n";
	  }
	}
}

sub partType {
	return Adc;
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
	my $anum = substr($::xpartName[$i],3,1);
        my $calcExp = "// ADC $anum\n";
	#print $calcExp, "\n";
	%seen = ();
        foreach (0 .. $::partCnt) {
	  foreach  $inp (0 .. $::partInCnt[$_]) {
            if ("Adc" eq $::partInputType[$_][$inp] && $anum == $::partInNum[$_][$inp]) {
	  	#print $_," ", $::xpartName[$_], " ", $::partInputPort[$_][$inp], "\n";
		$seen{$::partInputPort[$_][$inp]}=1;
	    }
	  }
	}
	foreach (sort { $a <=> $b }  keys %seen) {
	#	print $_, ",";
        	$calcExp .= "dWordUsed\[";
        	$calcExp .= $anum;
        	$calcExp .= "\]\[";
        	$calcExp .= $_;
        	$calcExp .= "\] =  1;\n";
	}
	#print "\n";
        return $calcExp;
}

# Figure out part input code
# Argument 1 is the part number
# Argument 2 is the input number
# Returns calculated input code
sub fromExp {
        my ($i, $j) = @_;
        my $card = $::partInNum[$i][$j];
        my $chan = $::partInputPort[$i][$j];
        return "dWord\[" . $card . "\]\[" . $chan . "\]";
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	my ($i) = @_;
        return "";
}

sub createAdcMedm
{
        my ($medmDir,$mdlName,$site,$dcuid,$medmTarget,$adcNum,@adcChannel) = @_;
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

        my $fname = "$mdlName\_MONITOR_ADCT$adcNum.adl";
        # Create MEDM File
        my $xpos = 0; my $ypos = 0; my $width = 726; my $height = 470;
        ("CDS::medmGen::medmGenFile") -> ($medmDir,$fname,$width,$height);

	# ************* Create Banner ******************************************************************************
        # Put blue rectangle banner at top of screen
        $height = 22;
        ("CDS::medmGen::medmGenRectangle") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,$ecolors{blue},"","","");
        # Add Display Name
        $xpos = 300; $ypos = 4; $width = 120; $height = 15;        
	("CDS::medmGen::medmGenText") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$mdlName\_MONITOR_ADC$adcNum",$ecolors{white});
        # Add time string to banner
        $xpos = 526; $ypos = 4; $width = 200; $height = 15;
        ("CDS::medmGen::medmGenTextMon") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_TIME_STRING",$ecolors{white},$ecolors{blue},"static");


        # ************* Create Background **************************************************************************
        # Add Background rectangles
        $xpos = 13; $ypos = 37; $width = 700; $height = 380;
        ("CDS::medmGen::medmGenRectangle") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,$ecolors{gray},"","","");

        # ************* Add Text  **********************************************************************************
	$xpos = 100; $ypos = 47; $width = 60; $height = 15;
	for($ii=0;$ii<16;$ii++)
	{
		if(($ii % 4) == 0) {$ypos += 10;}
		$xpos = 20; $width = 250;
		$labelName = substr $adcChannel[$adcNum][$ii],7;
		("CDS::medmGen::medmGenText") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$labelName",$ecolors{black});
		$xpos = 300; $width = 60;
		("CDS::medmGen::medmGenTextMon") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$adcChannel[$adcNum][$ii]",$ecolors{white},$ecolors{blue},"static");
		$ypos += 20;
	}

	$xpos = 300; $ypos = 47; $width = 60; $height = 15;
	for($ii=16;$ii<32;$ii++)
	{
		if(($ii % 4) == 0) {$ypos += 10;}
		$xpos = 380; $width = 250;
		$labelName = substr $adcChannel[$adcNum][$ii],7;
		("CDS::medmGen::medmGenText") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$labelName",$ecolors{black});
		$xpos = 640; $width = 60;
		("CDS::medmGen::medmGenTextMon") -> ($medmDir,$fname,$xpos,$ypos,$width,$height,"$adcChannel[$adcNum][$ii]",$ecolors{white},$ecolors{blue},"static");
		$ypos += 20;
	}

}
