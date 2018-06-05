package CDS::medmGenGdsTp;
use Exporter;
@ISA = ('Exporter');

require "lib/medmGen.pm";

#//     \file medmGenGdsTp.dox
#//     \brief Documentation for medmGenGdsTp.pm
#//
#// \n
#//     \subpage devguidercg2 "<<-- Parts Library"
#// \n

sub createGdsMedm
{
	my ($medmDir,$mdlName,$site,$dcuid,$medmTarget,$scriptTarget,$scriptArgs,$adcCnt,$dacCnt,$adcMaster,@dactype) = @_;
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


	my $fname = "$mdlName\_GDS_TP.adl";
        print "creating file $medmDir\/$fname \n";
        open(OUTMEDM, ">$medmDir/$fname") || die "cannot open $medmDir/$fname for writing ";


	# Create MEDM File
	my $xpos = 0; my $ypos = 0; my $width = 800; my $height = 350;
	$medmdata = ("CDS::medmGen::medmGenFile") -> ($medmDir,$fname,$width,$height);

	# ************* Create Banner ******************************************************************************
	# Put blue rectangle banner at top of screen
	$height = 22;
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{blue},"","","");
	# Add RCG VERSION label
	$xpos = 9; $ypos = 4; $width = 45; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"RCG #",$ecolors{white});
	# Add RCG Number
	$xpos = 54; $ypos = 4; $width = 35; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_RCG_VERSION",$ecolors{white},$ecolors{blue},"static");
	$xpos = 100; $ypos = 3; $width = 12; $height = 17; $vis = "calc"; $calc = "(a<1)";
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{warning},$vis,$calc,"$site\:FEC-$dcuid\_RCG_VERSION");
	# Add SVN label
	$xpos = 140; $ypos = 4; $width = 45; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"SVN #",$ecolors{white});
	# Add SVN Number
	$xpos = 183; $ypos = 4; $width = 35; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_BUILD_SVN",$ecolors{white},$ecolors{blue},"static");
	# Add Display Name
	$xpos = 270; $ypos = 4; $width = 200; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"$mdlName\_GDS_TP",$ecolors{white});
	# Add time string to banner
	$xpos = 570; $ypos = 4; $width = 200; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_TIME_STRING",$ecolors{white},$ecolors{blue},"static");
	# Create STATE WORD Status Section ***************************************************************************
	$xpos = 11; $ypos = 24; $width = 539; $height = 43;
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{black},"","","");
	$xpos = 157; $ypos = 26; $width = 20; $height = 14; 
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"FE",$ecolors{white});
	$xpos = 185; $ypos = 26; $width = 20; $height = 14; 
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"TIM",$ecolors{white});
	$xpos = 213; $ypos = 26; $width = 20; $height = 14; 
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"ADC",$ecolors{white});
	$xpos = 239; $ypos = 26; $width = 20; $height = 15; 
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"DAC",$ecolors{white});
	$xpos = 265; $ypos = 26; $width = 20; $height = 15; 
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"DAQ",$ecolors{white});
	$xpos = 292; $ypos = 26; $width = 20; $height = 15; 
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"IPC",$ecolors{white});
	$xpos = 318; $ypos = 26; $width = 20; $height = 15; 
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"AWG",$ecolors{white});
	$xpos = 346; $ypos = 26; $width = 20; $height = 15; 
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"DK",$ecolors{white});
	$xpos = 373; $ypos = 26; $width = 20; $height = 15; 
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"EXC",$ecolors{white});
	$xpos = 400; $ypos = 26; $width = 20; $height = 15; 
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"OVF",$ecolors{white});
	$xpos = 429; $ypos = 26; $width = 20; $height = 15; 
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"CFC",$ecolors{white});
	$xpos = 78; $ypos = 47; $width = 60; $height = 15; 
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"STATE WORD",$ecolors{white});
	$xpos = 152; $ypos = 47; $width = 218; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_STATE_WORD","0","7",$ecolors{red},$ecolors{green});
	$xpos = 369; $ypos = 47; $width = 28; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_STATE_WORD","8","8",$ecolors{blue},$ecolors{green});
	$xpos = 396; $ypos = 47; $width = 56; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_STATE_WORD","9","10",$ecolors{warning},$ecolors{green});

	# ************* Create Background **************************************************************************
	# Add Background rectangles
	$xpos = 11; $ypos = 69; $width = 775; $height = 255;
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{gray},"","","");
	$xpos = 555; $ypos = 24; $width = 230; $height = 43;
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{gray},"","","");
	$xpos = 11; $ypos = 69; $width = 775; $height = 18;
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{blue},"","","");
	$xpos = 15; $ypos = 93; $width = 182; $height = 225;
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{yellow},"","","");
	$xpos = 201; $ypos = 93; $width = 170; $height = 225;
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{yellow},"","","");
	$xpos = 375; $ypos = 93; $width = 175; $height = 225;
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{yellow},"","","");
	# BURT Background
	$xpos = 555; $ypos = 93; $width = 215; $height = 80;
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{yellow},"","","");
	# Coeff load background
	$xpos = 555; $ypos = 184; $width = 215; $height = 70;
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{yellow},"","","");
	# DAQ Reload background
	$xpos = 555; $ypos = 268; $width = 215; $height = 50;
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{yellow},"","","");
	# File loading
	$xpos = 625; $ypos = 72; $width = 69; $height = 15; 
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"CONFIGURATION FILES",$ecolors{white});
	# BURT STUFF ******************************************************************************************************
	# Add BURT related display
	$xpos = 562; $ypos = 106; $width = 160; $height = 18;
	$mdlNamelc = lc($mdlName);
	$relDisp = "$medmTarget\/$mdlNamelc\/$mdlName\_SDF_TABLE.adl";
        $medmdata .= ("CDS::medmGen::medmGenRelDisp") -> ($xpos,$ypos,$width,$height,$relDisp,$ecolors{white},$ecolors{blue},"SDF TABLE");
	# BURT Diffs Label
	$xpos = 733; $ypos = 94; $width = 24; $height = 12;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"DIFFS",$ecolors{black});
	# Add Guardian setpoint error  Counter Monitor
	$xpos = 725; $ypos = 108; $width = 35; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_SDF_DIFF_CNT",$ecolors{white},$ecolors{black},"alarm");
	# BURT Partial File Loaded
	$xpos = 562; $ypos = 128; $width = 200; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_SDF_LOADED",$ecolors{white},$ecolors{blue},"static");
	# BURT Reload Time
	$xpos = 562; $ypos = 147; $width = 200; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_SDF_RELOAD_TIME",$ecolors{white},$ecolors{blue},"static");

	$xpos = 210; $ypos = 117; $width = 153; $height = 18;
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{blue},"","","");
	$xpos = 384; $ypos = 200; $width = 158; $height = 18;
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{blue},"","","");
	$xpos = 125; $ypos = 123; $width = 50; $height = 18;
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{black},"","","");
	# ADD RED FE running alarm block
	$xpos = 15; $ypos = 91; $width = 182; $height = 74; $vis = "calc"; $calc = "(a&1)==1";
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{red},$vis,$calc,"$site\:FEC-$dcuid\_STATE_WORD");

	# ************* Create Main Status Banner *********************************************************************
	# ADD Blinking FE running indicator
	$xpos = 19; $ypos = 69; $width = 12; $height = 17; $vis = "calc"; $calc = "(a&1)";
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{ltblue},$vis,$calc,"$site\:FEC-$dcuid\_TIME_DIAG");
	# Add timing label
	$xpos = 63; $ypos = 72; $width = 45; $height = 15; 
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"TIMING STATUS",$ecolors{white});
	# Add i/o status label
	$xpos = 254; $ypos = 72; $width = 69; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"I/O STATUS",$ecolors{white});
	# Add DAQ status label
	$xpos = 429; $ypos = 72; $width = 69; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"DAQ STATUS",$ecolors{white});
	# Add DCUID Monitor
	$xpos = 507; $ypos = 72; $width = 25; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:DAQ-FEC_$dcuid\_DCU_ID",$ecolors{white},$ecolors{blue},"static");

	# ************* Create Timing Info Block *********************************************************************
	# Add CPU Meter
	$xpos = 19; $ypos = 93; $width = 85; $height = 65;
        $medmdata .= ("CDS::medmGen::medmGenMeter") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_CPU_METER",$ecolors{ltblue},$ecolors{mdblue});
	# Add DIAG RESET button
	$xpos = 617; $ypos = 44; $width = 100; $height = 20;
        $medmdata .= ("CDS::medmGen::medmGenMessage") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_DIAG_RESET",$ecolors{white},$ecolors{blue},"Diag Reset","1");

	# Add GPS Time label
	$xpos = 550; $ypos = 27; $width = 69; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"GPS Time",$ecolors{black});
	# Add GPS Time Monitor
	$xpos = 617; $ypos = 26; $width = 100; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_TIME_DIAG",$ecolors{white},$ecolors{blue},"static");

	# Add Sync Source label
	$xpos = 125; $ypos = 108; $width = 50; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"Sync Source",$ecolors{black});
	# Add PCIe Net Sync Monitor
	$xpos = 125; $ypos = 125; $width = 50; $height = 16;
	$medmdata .= ("CDS::medmGen::medmGenTextDyn") -> ($xpos,$ypos,$width,$height,"PCIeNet",$ecolors{green},"A&1","$site\:FEC-$dcuid\_TIME_ERR");
	# Add 1PPS Sync Monitor
	$xpos = 125; $ypos = 125; $width = 50; $height = 16;
	$medmdata .= ("CDS::medmGen::medmGenTextDyn") -> ($xpos,$ypos,$width,$height,"1PPS",$ecolors{green},"A&2","$site\:FEC-$dcuid\_TIME_ERR");
	# Add TDS Sync Monitor
	$xpos = 126; $ypos = 125; $width = 50; $height = 16;
	$medmdata .= ("CDS::medmGen::medmGenTextDyn") -> ($xpos,$ypos,$width,$height,"TDS",$ecolors{green},"A&4","$site\:FEC-$dcuid\_TIME_ERR");
	# Add IOP Sync Monitor
	$xpos = 126; $ypos = 125; $width = 50; $height = 16;
	$medmdata .= ("CDS::medmGen::medmGenTextDyn") -> ($xpos,$ypos,$width,$height,"IOP",$ecolors{green},"A&8","$site\:FEC-$dcuid\_TIME_ERR");
	# Add NO SYNC Alaram Monitor
	$xpos = 125; $ypos = 125; $width = 50; $height = 16;
	$medmdata .= ("CDS::medmGen::medmGenTextDyn") -> ($xpos,$ypos,$width,$height,"NO SYNC",$ecolors{red},"(A&255) == 0","$site\:FEC-$dcuid\_TIME_ERR");

	# Add cycle/user time label
	$xpos = 18; $ypos = 200; $width = 50; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"CYC/USR",$ecolors{black});
	# Add Cycle Time Monitor
	$xpos = 75; $ypos = 200; $width = 40; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_ADC_WAIT",$ecolors{white},$ecolors{black},"alarm");
	# Add User Time Monitor
	$xpos = 125; $ypos = 200; $width = 50; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_USR_TIME",$ecolors{white},$ecolors{black},"alarm");

	# Add cpu max time label
	$xpos = 18; $ypos = 218; $width = 50; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"CPU Max",$ecolors{black});
	# Add CPU Time Monitor
	$xpos = 75; $ypos = 218; $width = 40; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_CPU_METER",$ecolors{white},$ecolors{black},"alarm");
	# Add CPU Time Max Monitor
	$xpos = 125; $ypos = 218; $width = 50; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_CPU_METER_MAX",$ecolors{white},$ecolors{black},"alarm");

	# Following only for IOP
	if($adcMaster == 1)
	{
		# Add ADC Duotone Diag label
		$xpos = 18; $ypos = 236; $width = 50; $height = 15;
		$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"DT ADC",$ecolors{black});
		# Add ADC Duotone Monitor
		$xpos = 75; $ypos = 236; $width = 40; $height = 15;
		$medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_DUOTONE_TIME",$ecolors{white},$ecolors{black},"alarm");
		# Add IRIG-B Diag label
		$xpos = 18; $ypos = 254; $width = 50; $height = 15;
		$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"IRIG-B",$ecolors{black});
		# Add IRIG-B Monitor
		$xpos = 75; $ypos = 254; $width = 40; $height = 15;
		$medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_IRIGB_TIME",$ecolors{white},$ecolors{black},"alarm");
		# Add DAC Duotone Diag label
		$xpos = 18; $ypos = 272; $width = 50; $height = 15;
		$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"DT DAC",$ecolors{black});
		# Add DAC Duotone Monitor
		$xpos = 75; $ypos = 272; $width = 40; $height = 15;
		$medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_DUOTONE_TIME_DAC",$ecolors{white},$ecolors{black},"alarm");
		# Add arrow to DAC DT On/Off
		$xpos = 115; $ypos = 272; $width = 30; $height = 15;
		$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"---->",$ecolors{black});
		# Add DAC DT On/Off
		$xpos = 144; $ypos = 276; $width = 50; $height = 35;
		$medmdata .= ("CDS::medmGen::medmGenChoice") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_DACDT_ENABLE",$ecolors{white},$ecolors{blue});
	}

	# ************* Create I/O Status Info Block *****************************************************************
	# Add Overflow counter reset
	$xpos = 210; $ypos = 93; $width = 70; $height = 18;
        $medmdata .= ("CDS::medmGen::medmGenMessage") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_OVERFLOW_RESET",$ecolors{white},$ecolors{blue},"OVERFLOWS","1");
	# Add Overflow Counter Monitor
	$xpos = 282; $ypos = 95; $width = 80; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_ACCUM_OVERFLOW",$ecolors{white},$ecolors{black},"alarm");

	# Add IPC Monitor related display
	$xpos = 210; $ypos = 117; $width = 85; $height = 18;
	$mdlNamelc = lc($mdlName);
	$relDisp = "$medmTarget\/$mdlNamelc\/$mdlName\_IPC_STATUS.adl";
        $medmdata .= ("CDS::medmGen::medmGenRelDisp") -> ($xpos,$ypos,$width,$height,$relDisp,$ecolors{white},$ecolors{blue},"RT NET STAT");
	# Add IPC Status Monitor
	$xpos = 310; $ypos = 118; $width = 10; $height = 15; $vis = "calc"; $calc = "(a&1)";
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{red},$vis,$calc,"$site\:FEC-$dcuid\_IPC_STAT");
	$xpos = 322; $ypos = 118; $width = 10; $height = 15; $vis = "calc"; $calc = "(a&2)";
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{red},$vis,$calc,"$site\:FEC-$dcuid\_IPC_STAT");
	$xpos = 334; $ypos = 118; $width = 10; $height = 15; $vis = "calc"; $calc = "(a&4)";
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{red},$vis,$calc,"$site\:FEC-$dcuid\_IPC_STAT");
	$xpos = 346; $ypos = 118; $width = 10; $height = 15; $vis = "calc"; $calc = "(a&8)";
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{red},$vis,$calc,"$site\:FEC-$dcuid\_IPC_STAT");

	$xpos = 310; $ypos = 118; $width = 10; $height = 15; $vis = "calc"; $calc = "!(a&1) && (a&16)";
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{warning},$vis,$calc,"$site\:FEC-$dcuid\_IPC_STAT");
	$xpos = 322; $ypos = 118; $width = 10; $height = 15; $vis = "calc"; $calc = "!(a&2) && (a&32)";
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{warning},$vis,$calc,"$site\:FEC-$dcuid\_IPC_STAT");
	$xpos = 334; $ypos = 118; $width = 10; $height = 15; $vis = "calc"; $calc = "!(a&4) && (a&64)";
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{warning},$vis,$calc,"$site\:FEC-$dcuid\_IPC_STAT");
	$xpos = 346; $ypos = 118; $width = 10; $height = 15; $vis = "calc"; $calc = "!(a&8) && (a&128)";
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{warning},$vis,$calc,"$site\:FEC-$dcuid\_IPC_STAT");

	$xpos = 310; $ypos = 118; $width = 10; $height = 15; $vis = "calc"; $calc = "!(a&1) && !(a&16)";
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{green},$vis,$calc,"$site\:FEC-$dcuid\_IPC_STAT");
	$xpos = 322; $ypos = 118; $width = 10; $height = 15; $vis = "calc"; $calc = "!(a&2) && !(a&32)";
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{green},$vis,$calc,"$site\:FEC-$dcuid\_IPC_STAT");
	$xpos = 334; $ypos = 118; $width = 10; $height = 15; $vis = "calc"; $calc = "!(a&4) && !(a&64)";
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{green},$vis,$calc,"$site\:FEC-$dcuid\_IPC_STAT");
	$xpos = 346; $ypos = 118; $width = 10; $height = 15; $vis = "calc"; $calc = "!(a&8) && !(a&128)";
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{green},$vis,$calc,"$site\:FEC-$dcuid\_IPC_STAT");


	# Add Coeff Reload
	$xpos = 562; $ypos = 190; $width = 200; $height = 18;
        $medmdata .= ("CDS::medmGen::medmGenMessage") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_LOAD_NEW_COEFF",$ecolors{white},$ecolors{black},"COEFF LOAD","1");
	# Add Coeff load time Monitor
	$xpos = 562; $ypos = 210; $width = 200; $height = 18;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_MSG",$ecolors{white},$ecolors{blue},"static");
	# Add Coeff Msg2 Monitor
	$xpos = 562; $ypos = 230; $width = 160; $height = 18;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_MSG2",$ecolors{white},$ecolors{blue},"static");
	$xpos = 722; $ypos = 230; $width = 40; $height = 18;
	$arg = "/opt/rtcds/";
	$fileCmd = $scriptTarget . " &";
        # $medmdata .= ("CDS::medmGen::medmGenShellCmd") -> ($xpos,$ypos,$width,$height,$ecolors{black},$ecolors{yellow},1,"Diff","Coeff Diff","xterm -e view",$scriptTarget," &");
        $medmdata .= ("CDS::medmGen::medmGenShellCmd") -> ($xpos,$ypos,$width,$height,$ecolors{black},$ecolors{yellow},1,"Diff","Coeff Diff","xterm -e view",$fileCmd);

	# ************* Create DAQ Status Info Block *****************************************************************

	# Add DAQ Status label
	$xpos = 425; $ypos = 94; $width = 110; $height = 12;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"Status  CPS   SUM",$ecolors{black});
	# Add DAQ Status label
	$xpos = 387; $ypos = 113; $width = 24; $height = 12;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"DC",$ecolors{black});
	# Add DAQ Status Monitor
	$xpos = 407; $ypos = 112; $width = 14; $height = 15; $vis = "if zero"; $calc = "";
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{green},$vis,$calc,"$site\:DAQ-DC0\_$mdlName\_STATUS");
	$xpos = 407; $ypos = 112; $width = 14; $height = 15; $vis = "if not zero"; $calc = "";
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{red},$vis,$calc,"$site\:DAQ-DC0\_$mdlName\_STATUS");
	# Add DAQ Status label
	$xpos = 387; $ypos = 146; $width = 24; $height = 12;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"FE",$ecolors{black});
	# Add DAQ Net Monitor
	$xpos = 407; $ypos = 145; $width = 14; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_FB_NET_STATUS","0","1",$ecolors{green},$ecolors{red});

	# Add DAQ Status label
	$xpos = 407; $ypos = 131; $width = 110; $height = 12;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"    NET CHN   DRATE  TRATE",$ecolors{black});
	# Add DAQ DC chan count Monitor
	$xpos = 424; $ypos = 112; $width = 35; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:DAQ-DC0_$mdlName\_STATUS",$ecolors{white},$ecolors{black},"alarm","hexadecimal");
	# Add DAQ DC CRC Error Monitor
	$xpos = 464; $ypos = 112; $width = 35; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:DAQ-DC0_$mdlName\_CRC_CPS",$ecolors{white},$ecolors{black},"alarm");
	# Add DAQ DC CRC Error Sum Monitor
	$xpos = 504; $ypos = 112; $width = 35; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:DAQ-DC0_$mdlName\_CRC_SUM",$ecolors{white},$ecolors{black},"alarm");

	# Add DAQ chan count Monitor
	$xpos = 424; $ypos = 145; $width = 35; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:DAQ-FEC_$dcuid\_CHAN_CNT",$ecolors{white},$ecolors{black},"alarm");
	$xpos = 424; $ypos = 165; $width = 35; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:DAQ-FEC_$dcuid\_EPICS_CHAN_CNT",$ecolors{white},$ecolors{black},"alarm");
	# Add DAQ + TP chan counter Monitor
	$xpos = 464; $ypos = 145; $width = 35; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:DAQ-FEC_$dcuid\_TOTAL",$ecolors{white},$ecolors{black},"alarm");
	# Add DAQ chan data rate Monitor
	$xpos = 504; $ypos = 145; $width = 35; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_DAQ_BYTE_COUNT",$ecolors{white},$ecolors{black},"alarm");

	# Add DAQ Reload
	$xpos = 562; $ypos = 274; $width = 200; $height = 18;
        $medmdata .= ("CDS::medmGen::medmGenMessage") -> ($xpos,$ypos,$width,$height,"$site\:DAQ-FEC_$dcuid\_LOAD_CONFIG",$ecolors{white},$ecolors{black},"DAQ LOAD","1");
	# Add DAQ load time Monitor
	$xpos = 562; $ypos = 294; $width = 200; $height = 18;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_MSGDAQ",$ecolors{white},$ecolors{blue},"static");

	# Add TP label
	$xpos = 386; $ypos = 202; $width = 80; $height = 12; 
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"Test Points",$ecolors{white});
	# Add AWG label
	$xpos = 497; $ypos = 202; $width = 20; $height = 12; 
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"AWG",$ecolors{white});
	# Add TP Counter Monitor
	$xpos = 467; $ypos = 202; $width = 20; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_TP_CNT",$ecolors{white},$ecolors{black},"alarm");
	# Add AWG Monitor
	$xpos = 525; $ypos = 201; $width = 10; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_AWGTPMAN_STAT","0","0",$ecolors{red},$ecolors{green});

	# Add Individual TP channel number monitors
	$xpos = 384; $ypos = 221; $width = 50; $height = 15;
	for(my $ii=0;$ii<5;$ii++)
	{
		$medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_GDS_MON_$ii",$ecolors{white},$ecolors{blue},"static");
		$ypos += 18;
	}
	$xpos = 438; $ypos = 221;
	for($ii=5;$ii<10;$ii++)
	{
		$medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_GDS_MON_$ii",$ecolors{white},$ecolors{blue},"static");
		$ypos += 18;
	}
	$xpos = 492; $ypos = 221;
	for(my $ii=10;$ii<15;$ii++)
	{
		$medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_GDS_MON_$ii",$ecolors{white},$ecolors{blue},"static");
		$ypos += 18;
	}
	# ****************** ADD ADC / DAC related disp *******************************************************************
	my $totalCards = 0;
	$mdlNamelc = lc($mdlName);
	$xpos = 215; $ypos = 200; $width = 30; $height = 20;
	$bxpos = 247; $bypos = 201; $bwidth = 21; $bheight = 18;
	for($ii=0;$ii<$adcCnt;$ii++)
	{
		$relDisp = "$medmTarget\/$mdlNamelc\/$mdlName\_MONITOR_ADC$::adcCardNum[$ii].adl";
        	$medmdata .= ("CDS::medmGen::medmGenRelDisp") -> ($xpos,$ypos,$width,$height,$relDisp,$ecolors{white},$ecolors{brown},"A$::adcCardNum[$ii]");
		$medmdata .= ("CDS::medmGen::medmGenByte") -> ($bxpos,$bypos,$bwidth,$bheight,"$site\:FEC-$dcuid\_ADC_STAT_$ii","0","2",$ecolors{green},$ecolors{red});
		$ypos += 22;
		$bypos += 22;
		$totalCards ++;
		if(($totalCards % 5) == 0) {
			$xpos = 295;
			$ypos = 200;
			$bxpos = 327;
			$bypos = 201;
		}
	}
	$bwidth = 28;
	$b1width = 14;
	for($ii=0;$ii<$dacCnt;$ii++)
	{
		$relDisp = "$medmTarget\/$mdlNamelc\/$mdlName\_DAC_MONITOR_$ii.adl";
		if($dactype[$ii] eq "GSC_18AO8" ) {
			$medmdata .= ("CDS::medmGen::medmGenRelDisp") -> ($xpos,$ypos,$width,$height,$relDisp,$ecolors{white},$ecolors{dacblue},"D$::dacCardNum[$ii]");
			if($adcMaster == 1)
			{
				$medmdata .= ("CDS::medmGen::medmGenByte") -> ($bxpos,$bypos,$bwidth,$bheight,"$site\:FEC-$dcuid\_DAC_STAT_$ii","0","4",$ecolors{green},$ecolors{red});
			} else {
				$medmdata .= ("CDS::medmGen::medmGenByte") -> ($bxpos,$bypos,$b1width,$bheight,"$site\:FEC-$dcuid\_DAC_STAT_$ii","1","2",$ecolors{green},$ecolors{red});
			}
		} elsif($dactype[$ii] eq "GSC_20AO8" ) {
			$medmdata .= ("CDS::medmGen::medmGenRelDisp") -> ($xpos,$ypos,$width,$height,$relDisp,$ecolors{black},$ecolors{ltblue},"D$::dacCardNum[$ii]");
			if($adcMaster == 1)
			{
				$medmdata .= ("CDS::medmGen::medmGenByte") -> ($bxpos,$bypos,$bwidth,$bheight,"$site\:FEC-$dcuid\_DAC_STAT_$ii","0","4",$ecolors{green},$ecolors{red});
			} else {
				$medmdata .= ("CDS::medmGen::medmGenByte") -> ($bxpos,$bypos,$b1width,$bheight,"$site\:FEC-$dcuid\_DAC_STAT_$ii","1","2",$ecolors{green},$ecolors{red});
			}
		} else {
			$medmdata .= ("CDS::medmGen::medmGenRelDisp") -> ($xpos,$ypos,$width,$height,$relDisp,$ecolors{white},$ecolors{blue},"D$::dacCardNum[$ii]");
			if($adcMaster == 1)
			{
			$medmdata .= ("CDS::medmGen::medmGenByte") -> ($bxpos,$bypos,$bwidth,$bheight,"$site\:FEC-$dcuid\_DAC_STAT_$ii","0","3",$ecolors{green},$ecolors{red});
			} else {
				$medmdata .= ("CDS::medmGen::medmGenByte") -> ($bxpos,$bypos,$b1width,$bheight,"$site\:FEC-$dcuid\_DAC_STAT_$ii","1","2",$ecolors{green},$ecolors{red});
			}
		}
		$ypos += 22;
		$bypos += 22;
		$totalCards ++;
		if(($totalCards % 5) == 0) {
			$xpos = 295;
			$ypos = 200;
			$bxpos = 327;
			$bypos = 201;
		}
	}

print OUTMEDM "$medmdata \n";
close OUTMEDM;

}
