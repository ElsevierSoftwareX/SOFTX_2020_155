package CDS::medmGenStatus;
use Exporter;
@ISA = ('Exporter');

require "lib/medmGen.pm";

#//     \file medmGenGdsTp.dox
#//     \brief Documentation for medmGenGdsTp.pm
#//
#// \n
#//     \subpage devguidercg2 "<<-- Parts Library"
#// \n

sub createStatusMedm
{
	my ($medmDir,$mdlName,$ifo,$dcuid,$medmTarget,$scriptTarget,$scriptArgs,$adcCnt,$dacCnt,$iopModel,@dactype) = @_;
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
             "yellow" => "55",
	     "warning" => "31"
           );


	my $fname = "$mdlName\_FE_STATS.adl";
        print "creating file $medmDir\/$fname \n";
        open(OUTMEDM, ">$medmDir/$fname") || die "cannot open $medmDir/$fname for writing ";


	# Create MEDM File
	my $xpos = 0; my $ypos = 400; my $width = 800; my $height = 650;
	$medmdata = ("CDS::medmGen::medmGenFile") -> ($medmDir,$fname,$width,$height);

	# ************* Create Banner ******************************************************************************
	# Put blue rectangle banner at top of screen
	$height = 22;
	$xpos = 0; $ypos = 0; $width = 800; $height = 22;
    $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{black},"","","");

	# Add Display Name
	$xpos = 300; $ypos = 4; $width = 200; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"$mdlName\_FE_STAT",$ecolors{white});
	$xpos = 570; $ypos = 4; $width = 200; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_TIME_STRING",$ecolors{white},$ecolors{black},"static");

	# Add time string to banner
	# Put blue rectangle banner at bottom of screen
	$xpos = 5; $ypos = 32; $width = 790; $height = 600;
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{black},"","","");
	# Add build info label
	$xpos = 10; $ypos = 40; $width = 45; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"MODEL BUILD INFO",$ecolors{white});
	# Add RCG VERSION label
	$xpos = 60; $ypos = 60; $width = 45; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"RCG #",$ecolors{white});
	# Add RCG Number
	$xpos = 160; $ypos = 60; $width = 50; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_RCG_VERSION",$ecolors{white},$ecolors{black},"static");
	# Add SVN label
	# Add build date label
	$xpos = 60; $ypos = 80; $width = 45; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"BUILD DATE:",$ecolors{white});
	# Add build date
	$xpos = 160; $ypos = 80; $width = 150; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_BUILD_DATE",$ecolors{white},$ecolors{black},"static");


	# Add runtime info
	$xpos = 10; $ypos = 140; $width = 45; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"RUN TIME INFO",$ecolors{white});
	# Add gps start time label
	$xpos = 60; $ypos = 160; $width = 50; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"GPS START",$ecolors{white});
	# Add GPS Start Time Monitor
	$xpos = 160; $ypos = 160; $width = 100; $height = 15;
    $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_START_GPS",$ecolors{white},$ecolors{black},"static");
	# Add uptime label
	$xpos = 60; $ypos = 200; $width = 50; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"UPTIME",$ecolors{white});
	$xpos = 160; $ypos = 180; $width = 30; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"Days",$ecolors{white});
	$xpos = 210; $ypos = 180; $width = 30; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"Hour",$ecolors{white});
	$xpos = 260; $ypos = 180; $width = 30; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"Min",$ecolors{white});
	# Add up Time Monitors
	$xpos = 160; $ypos = 200; $width = 30; $height = 15;
    $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_UPTIME_DAY",$ecolors{white},$ecolors{black},"static");
	$xpos = 210; $ypos = 200; $width = 30; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_UPTIME_HOUR",$ecolors{white},$ecolors{black},"static");
	$xpos = 260; $ypos = 200; $width = 30; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_UPTIME_MINUTE",$ecolors{white},$ecolors{black},"static");

    # Add Clock Period Info
	$xpos = 10; $ypos = 250; $width = 300; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"ADC CLOCK PERIOD (usec)",$ecolors{white});
	$xpos = 43; $ypos = 279; $width = 35; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"Min",$ecolors{white});
	$xpos = 84; $ypos = 279; $width = 35; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"Mean",$ecolors{white});
	$xpos = 124; $ypos = 279; $width = 35; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"Max",$ecolors{white});
	$xpos = 43; $ypos = 295; $width = 35; $height = 15;
    $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_ADC_WAIT_MIN",$ecolors{white},$ecolors{black},"static");
	$xpos = 84; $ypos = 295; $width = 35; $height = 15;
    $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_ADC_WAIT",$ecolors{white},$ecolors{black},"static");
	$xpos = 124; $ypos = 295; $width = 35; $height = 15;
    $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_ADC_WAIT_MAX",$ecolors{white},$ecolors{black},"static");



    # Add ADC status labels
	$xpos = 10; $ypos = 330; $width = 100; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"ADC STATUS",$ecolors{white});
	$xpos = 20; $ypos = 353; $width = 70; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"ON LINE",$ecolors{white});
    $ypos = 373;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"CHAN HOP",$ecolors{white});
    $ypos = 393;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"OVERRANGE",$ecolors{white});
    $ypos = 413;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"AUTOCAL",$ecolors{white});

    # Add ADC status info
	$xpos = 155; $ypos = 330; $width = 50; $height = 15;
    for($ii=0;$ii<$adcCnt;$ii++)
    {
        $label = "ADC".$ii;
	    $medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"$label",$ecolors{white});
        $xpos += 55;
    }
	$xpos = 173; $ypos = 351; $width = 15; $height = 15;
    for($ii=0;$ii<$adcCnt;$ii++)
    {
        $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_ADC_STAT_$ii","0","0",$ecolors{green},$ecolors{red});
        $ypos += 20;
        $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_ADC_STAT_$ii","1","1",$ecolors{green},$ecolors{red});
        $ypos += 20;
        $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_ADC_STAT_$ii","2","2",$ecolors{green},$ecolors{yellow});
        $ypos += 20;
        $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_ADC_STAT_$ii","3","3",$ecolors{green},$ecolors{red});

        $xpos += 55;
        $ypos = 351;
    }


    #Add DAC status
	$xpos = 10; $ypos = 455; $width = 100; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"DAC STATUS",$ecolors{white});
	$xpos = 20; $ypos = 480; $width = 100; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"TYPE",$ecolors{white});
    $ypos += 15;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"ON LINE",$ecolors{white});
    $ypos += 15;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"WATCHDOG",$ecolors{white});
    $ypos += 15;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"OVERRANGE",$ecolors{white});
    $ypos += 15;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"AI CHASSIS",$ecolors{white});
    $ypos += 15;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"FIFO STATUS",$ecolors{white});
    $ypos += 15;
	$xpos = 35; $ypos = 570; $width = 100; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"EMPTY",$ecolors{white});
    $ypos += 15;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"HIGH QTR",$ecolors{white});
    $ypos += 15;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"FULL",$ecolors{white});

    # Add DAC status info
	$xpos = 155; $ypos = 467; $width = 50; $height = 15;
    for($ii=0;$ii<$dacCnt;$ii++)
    {
        $label = "DAC".$ii;
	    $medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"$label",$ecolors{white});
        $xpos += 55;
    }
	$xpos = 174; $ypos = 483; $width = 12; $height = 12;
    for($ii=0;$ii<$dacCnt;$ii++)
    {
        if($dactype[$ii] eq "GSC_18AO8" ) {
            $label = "18AO8";
        } elsif($dactype[$ii] eq "GSC_20AO8" ) {
            $label = "20AO8";
        } else {
            $label = "16AO16";
        }
	    $medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"$label",$ecolors{white});

        $ypos += 15;
        $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_DAC_STAT_$ii","0","0",$ecolors{green},$ecolors{red});
        $ypos += 15;
        $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_DAC_STAT_$ii","1","1",$ecolors{green},$ecolors{red});
        $ypos += 15;
        $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_ADC_STAT_$ii","2","2",$ecolors{green},$ecolors{red});
        $ypos += 15;
        $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_DAC_STAT_$ii","4","4",$ecolors{green},$ecolors{red});
        $ypos += 15;
        $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_DAC_STAT_$ii","3","3",$ecolors{green},$ecolors{red});
        $ypos += 15;
        $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_DAC_STAT_$ii","5","5",$ecolors{red},$ecolors{green});
        $ypos += 15;
        $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_DAC_STAT_$ii","6","6",$ecolors{red},$ecolors{green});
        $ypos += 15;
        $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_DAC_STAT_$ii","7","7",$ecolors{red},$ecolors{green});

        $xpos += 55;
        $ypos = 483;
    }

print OUTMEDM "$medmdata \n";
close OUTMEDM;

}
