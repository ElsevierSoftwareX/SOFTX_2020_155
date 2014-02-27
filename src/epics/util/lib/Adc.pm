package CDS::Adc;
use Exporter;
@ISA = ('Exporter');

require "lib/medmGen.pm";

#//     \page Adc Adc.pm
#//     Adc.pm - provides standard subroutines for handling ADC parts in CDS PARTS library.
#//
#// \n
#// \n

# ADC cards we support
%board_types = (
	GSC_16AI64SSA => 1, # Slow General Standards board
        GSC_18AISS6C => 1 # 18-bit 6 channel General Standards board
);

# default board type (if none specified with type=<type> in block Description)
$default_board_type = "GSC_16AI64SSA";

#// \n \n
#// \b sub \b initAdc \n 
#// Called by Parser3.pm to check if ADC supported and fill in ADC info. \n \n
sub initAdc {
        my ($node) = @_;
        $::adcPartNum[$::adcCnt] = $::partCnt;
	# Set ADC type and number
	my $desc = ${$node->{FIELDS}}{"Description"};
	my $src = ${$node->{FIELDS}}{"SourceBlock"};
	my $name = ${$node->{FIELDS}}{"Name"};
	my ($type) = $desc =~ m/type=([^,]+)/g;
	my ($num) = $desc =~ m/card_num=([^,]+)/g;
	my ($srcnum) = $src =~ m/ADC([^,]+)/g;
	my ($namenum) = $name =~ m/ADC([^,]+)/g;
	if ($type eq undef) {
		$type = $default_board_type;
	}
	if ($num eq undef) {
		$num = $::adcCnt;
	}
	print "ADC $::adcCnt; type='$type'; num=$num\n";
        # print "foo=$board_types{$type} and srcnum = $srcnum and namenum = $namenum\n";
	
	# Check if this is a supported board type
	if ($board_types{$type} != 1) {
		print "Unsupported board type\n";
		print "Known board types:\n";
		foreach (keys %board_types) {
			print "\t$_\n";
		}
		exit 1;
	}

	# Verify ADC number in ADC block matchs name in description field (required).
	if($srcnum != $namenum) {
		die "ERROR ***** ADC number in name ($name) does not match ADC number in ADC part block \(ADC$srcnum\)\nAfter you fix this, please verify all connected BUS selectors have valid entries.\n";
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

#// \b sub \b partType \n 
#// Required subroutine for RCG \n
#// Returns Adc \n\n
sub partType {
	return Adc;
}

#// \b sub \b printHeaderStruct \n 
#// Required subroutine for RCG \n
#// Print Epics communication structure into a header file \n
#// Current part number is passed as first argument \n
#// For ADC part, nothing required to be placed in the Epics comms struct. \n\n
sub printHeaderStruct {
        my ($i) = @_;
        ;
}

#// \b sub \b printEpics \n 
#// Required subroutine for RCG \n
#// Print Epics variable definitions \n
#// Current part number is passed as first argument \n
#// For ADC part, there are no EPICS records generated. \n\n
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

#// \b sub \b frontEndInitCode \n 
#// Required subroutine for RCG \n
#// Return front end initialization code \n
#// Argument 1 is the part number \n
#// Returns calculated code string \n \n
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

#// \b sub \b fromExp \n 
#// Required subroutine for RCG \n
#// Figure out part input code \n
#// Argument 1 is the part number \n
#// Argument 2 is the input number \n
#// Returns calculated input code \n\n
sub fromExp {
        my ($i, $j) = @_;
        my $card = $::partInNum[$i][$j];
        my $chan = $::partInputPort[$i][$j];
        return "dWord\[" . $card . "\]\[" . $chan . "\]";
}

#// \b sub \b frontEndCode \n 
#// Required subroutine for RCG \n
#// Return front end code \n
#// Argument 1 is the part number \n
#// Returns calculated code string \n\n

sub frontEndCode {
	my ($i) = @_;
        return "";
}

#// \b sub \b createAdcMedm \n 
#// Called by feCodeGen.pl to auto gen ADC channel MEDM screens \n
#// This code requires /lib/medmGen.pm \n\n
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

        my $fname = "$mdlName\_MONITOR_ADC$::adcCardNum[$adcNum].adl";
        # Create MEDM File
        print "creating file $medmDir\/$fname \n";
        open(OUTMEDM, ">$medmDir/$fname") || die "cannot open $medmDir/$fname for writing ";

        my $xpos = 0; my $ypos = 0; my $width = 900; my $height = 430;
        $medmdata = ("CDS::medmGen::medmGenFile") -> ($medmDir,$file,$width,$height);

	# ************* Create Banner ******************************************************************************
        # Put blue rectangle banner at top of screen
        $height = 22;
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{blue},"","","");
        # Add Display Name
        $xpos = 370; $ypos = 4; $width = 120; $height = 15;        
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"$mdlName\_MONITOR_ADC$::adcCardNum[$adcNum]",$ecolors{white});
        # Add time string to banner
        $xpos = 640; $ypos = 4; $width = 200; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_TIME_STRING",$ecolors{white},$ecolors{blue},"static");


        # ************* Create Background **************************************************************************
        # Add Background rectangles
        $xpos = 13; $ypos = 37; $width = 870; $height = 380;
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{gray},"","","");
        $xpos = 300; $ypos = 38; $width = 60; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"VALUE",$ecolors{black});
	$xpos = 370;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"OVERFLOW",$ecolors{black});
        $xpos = 720;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"VALUE",$ecolors{black});
	$xpos = 790;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"OVERFLOW",$ecolors{black});

        # ************* Add Text  **********************************************************************************
	$xpos = 100; $ypos = 47; $width = 60; $height = 15;
	for($ii=0;$ii<16;$ii++)
	{
		if(($ii % 4) == 0) {$ypos += 10;}
		$xpos = 20; $width = 250;
		$labelName = substr $adcChannel[$adcNum][$ii],7;
		$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"$labelName",$ecolors{black});
		if($labelName ne "") {
			$xpos = 285; $width = 10;
			$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,$ii,$ecolors{black});
			$xpos = 300; $width = 60;
			$medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$adcChannel[$adcNum][$ii]",$ecolors{white},$ecolors{blue},"static");
			$xpos = 370;
        		$medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_ADC_OVERFLOW_$adcNum\_$ii",$ecolors{white},$ecolors{blue},"static");
		}
		$ypos += 20;
	}

	$xpos = 720; $ypos = 47; $width = 60; $height = 15;
	for($ii=16;$ii<32;$ii++)
	{
		if(($ii % 4) == 0) {$ypos += 10;}
		$xpos = 460; $width = 250;
		$labelName = substr $adcChannel[$adcNum][$ii],7;
		$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"$labelName",$ecolors{black});
		if($labelName ne "") {
			$xpos = 705; $width = 10;
			$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,$ii,$ecolors{black});
			$xpos = 720; $width = 60;
			$medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$adcChannel[$adcNum][$ii]",$ecolors{white},$ecolors{blue},"static");
			$xpos = 790;
        		$medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_ADC_OVERFLOW_$adcNum\_$ii",$ecolors{white},$ecolors{blue},"static");
		}
		$ypos += 20;
	}

print OUTMEDM "$medmdata \n";
close OUTMEDM;

}
