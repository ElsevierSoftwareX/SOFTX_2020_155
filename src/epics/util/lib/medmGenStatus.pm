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
if($iopModel == 1) {
	my $xpos = 0; my $ypos = 400; my $width = 800; my $height = 650;
	$medmdata = ("CDS::medmGen::medmGenFile") -> ($medmDir,$fname,$width,$height);
} else {
	my $xpos = 0; my $ypos = 400; my $width = 800; my $height = 350;
	$medmdata = ("CDS::medmGen::medmGenFile") -> ($medmDir,$fname,$width,$height);
}

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
if($iopModel == 1) {
	$xpos = 5; $ypos = 32; $width = 790; $height = 600;
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{black},"","","");
} else {
	$xpos = 5; $ypos = 32; $width = 790; $height = 310;
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{black},"","","");
}
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
	$xpos = 380; $ypos = 40; $width = 150; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"RUN TIME INFO",$ecolors{white});
	# Add gps start time label
	$xpos = 410; $ypos = 60; $width = 90; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"GPS START",$ecolors{white});
	# Add GPS Start Time Monitor
	$xpos = 530; $ypos = 60; $width = 100; $height = 15;
    $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_START_GPS",$ecolors{white},$ecolors{black},"static");
	# Add uptime label
	$xpos = 420; $ypos = 100; $width = 60; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"UPTIME",$ecolors{white});
	$xpos = 530; $ypos = 80; $width = 30; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"Days",$ecolors{white});
	$xpos = 580; $ypos = 80; $width = 30; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"Hour",$ecolors{white});
	$xpos = 630; $ypos = 80; $width = 30; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"Min",$ecolors{white});
	# Add up Time Monitors
	$xpos = 530; $ypos = 100; $width = 30; $height = 15;
    $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_UPTIME_DAY",$ecolors{white},$ecolors{black},"static");
	$xpos = 580; $ypos = 100; $width = 30; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_UPTIME_HOUR",$ecolors{white},$ecolors{black},"static");
	$xpos = 630; $ypos = 100; $width = 30; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_UPTIME_MINUTE",$ecolors{white},$ecolors{black},"static");

	$xpos = 6; $ypos = 119; $width = 785; $height = 3;
    $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{white},"","","");

    # Add Binary I/O Info
	$xpos = 10; $ypos = 137; $width = 220; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"BINARY I/O CARDS MAPPED",$ecolors{white});
	$xpos = 20; $ypos = 157; $width = 70; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"IIRO-8",$ecolors{white});
    $ypos += 20;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"Contec 32L",$ecolors{white});
    $ypos += 20;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"Contec 1616",$ecolors{white});
    $ypos += 20;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"Contec 6464",$ecolors{white});
	$xpos = 170; $ypos = 157; $width = 30; $height = 15;
    $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_BIO_MON_0",$ecolors{white},$ecolors{black},"static");
    $ypos += 20;
    $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_BIO_MON_1",$ecolors{white},$ecolors{black},"static");
    $ypos += 20;
    $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_BIO_MON_2",$ecolors{white},$ecolors{black},"static");
    $ypos += 20;
    $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_BIO_MON_3",$ecolors{white},$ecolors{black},"static");

    # Add Timing Error Info
if($iopModel == 1) {
	$xpos = 380; $ypos = 137; $width = 125; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"TIMING ERRORS",$ecolors{white});
	$xpos = 409; $ypos = 157; $width = 75; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"ADC HOLD",$ecolors{white});
	$xpos = 409; $ypos = 177; $width = 75; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"PROC TIME",$ecolors{white});
	$xpos = 409; $ypos = 197; $width = 75; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"IRIG-B",$ecolors{white});
    
	$xpos = 501; $ypos = 157; $width = 20; $height = 15;
    $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_DIAG_WORD","1","1",$ecolors{red},$ecolors{green});
	$xpos = 501; $ypos = 177; $width = 20; $height = 15;
    $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_DIAG_WORD","3","3",$ecolors{red},$ecolors{green});
	$xpos = 501; $ypos = 197; $width = 20; $height = 15;
    $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_DIAG_WORD","4","4",$ecolors{red},$ecolors{green});
}

    # Add Clock Period Info
	$xpos = 10; $ypos = 280; $width = 150; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"ADC CLOCK PERIOD",$ecolors{white});
	$xpos = 165; $ypos = 264; $width = 35; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"Min",$ecolors{white});
	$xpos = 220;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"Mean",$ecolors{white});
	$xpos = 275;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"Max",$ecolors{white});
	$xpos = 165; $ypos = 280; $width = 35; $height = 15;
    $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_ADC_WAIT_MIN",$ecolors{white},$ecolors{black},"static");
	$xpos = 220;
    $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_ADC_WAIT",$ecolors{white},$ecolors{black},"static");
	$xpos = 280;
    $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_ADC_WAIT_MAX",$ecolors{white},$ecolors{black},"static");



if($iopModel == 1) {
    # Add ADC status labels
	$xpos = 10; $ypos = 315; $width = 100; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"ADC STATUS",$ecolors{white});
	$xpos = 20; $ypos = 338; $width = 70; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"ON LINE",$ecolors{white});
    $ypos = 358;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"CHAN HOP",$ecolors{white});
    $ypos = 378;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"OVERRANGE",$ecolors{white});
    $ypos = 398;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"AUTOCAL",$ecolors{white});
    $ypos = 418;
	$medmdata .= ("CDS::medmGen::medmGenTextLeft") -> ($xpos,$ypos,$width,$height,"TIMING",$ecolors{white});

    # Add ADC status info
	$xpos = 155; $ypos = 315; $width = 50; $height = 15;
    for($ii=0;$ii<$adcCnt;$ii++)
    {
        $label = "ADC".$ii;
	    $medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"$label",$ecolors{white});
        $xpos += 55;
    }
	$xpos = 173; $ypos = 336; $width = 15; $height = 15;
    for($ii=0;$ii<$adcCnt;$ii++)
    {
        $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_ADC_STAT_$ii","0","0",$ecolors{green},$ecolors{red});
        $ypos += 20;
        $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_ADC_STAT_$ii","1","1",$ecolors{green},$ecolors{red});
        $ypos += 20;
        $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_ADC_STAT_$ii","2","2",$ecolors{green},$ecolors{red});
        $ypos += 20;
        $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_ADC_STAT_$ii","3","3",$ecolors{green},$ecolors{red});
        $ypos += 20;
        $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_ADC_STAT_$ii","4","4",$ecolors{green},$ecolors{red});

        $xpos += 55;
        $ypos = 336;
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
        $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_DAC_STAT_$ii","2","2",$ecolors{green},$ecolors{red});
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
}

print OUTMEDM "$medmdata \n";
close OUTMEDM;

}
