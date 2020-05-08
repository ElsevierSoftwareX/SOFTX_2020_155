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
	my ($medmDir,$mdlName,$ifo,$dcuid,$medmTarget,$scriptTarget,$scriptArgs,$adcCnt,$dacCnt,$adcMaster,@dactype) = @_;
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
	my $xpos = 0; my $ypos = 0; my $width = 400; my $height = 350;
	$medmdata = ("CDS::medmGen::medmGenFile") -> ($medmDir,$fname,$width,$height);

	# ************* Create Banner ******************************************************************************
	# Put blue rectangle banner at top of screen
	$height = 22;
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{blue},"","","");
	# Put blue rectangle banner at bottom of screen
	$xpos = 5; $ypos = 32; $width = 400; $height = 300;
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


	# Add proc info label
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



	# Add Display Name
	$xpos = 70; $ypos = 4; $width = 200; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"$mdlName\_FE_STAT",$ecolors{white});
	# Add time string to banner
	$xpos = 270; $ypos = 4; $width = 200; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$ifo\:FEC-$dcuid\_TIME_STRING",$ecolors{white},$ecolors{blue},"static");


print OUTMEDM "$medmdata \n";
close OUTMEDM;

}
