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
	my ($medmDir,$mdlName,$site,$dcuid,$medmTarget,$adcCnt,$dacCnt,$adcMaster,@dactype) = @_;
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


	my $fname = "$mdlName\_GDS_TP.adl";
        print "creating file $medmDir\/$fname \n";
        open(OUTMEDM, ">$medmDir/$fname") || die "cannot open $medmDir/$fname for writing ";


	# Create MEDM File
	my $xpos = 0; my $ypos = 0; my $width = 573; my $height = 310;
	$medmdata = ("CDS::medmGen::medmGenFile") -> ($medmDir,$fname,$width,$height);

	# ************* Create Banner ******************************************************************************
	# Put blue rectangle banner at top of screen
	$height = 22;
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{blue},"","","");
	# Add SVN label
	$xpos = 7; $ypos = 4; $width = 45; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"SVN #",$ecolors{white});
	# Add SVN Number
	$xpos = 50; $ypos = 4; $width = 100; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_BUILD_SVN",$ecolors{white},$ecolors{blue},"static");
	# Add Display Name
	$xpos = 195; $ypos = 4; $width = 200; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"$mdlName\_GDS_TP",$ecolors{white});
	# Add time string to banner
	$xpos = 380; $ypos = 4; $width = 200; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_TIME_STRING",$ecolors{white},$ecolors{blue},"static");

	# ************* Create Background **************************************************************************
	# Add Background rectangles
	$xpos = 11; $ypos = 29; $width = 550; $height = 270;
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{gray},"","","");
	$xpos = 11; $ypos = 29; $width = 550; $height = 18;
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{blue},"","","");
	$xpos = 15; $ypos = 53; $width = 182; $height = 240;
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{yellow},"","","");
	$xpos = 201; $ypos = 53; $width = 170; $height = 240;
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{yellow},"","","");
	$xpos = 375; $ypos = 53; $width = 175; $height = 240;
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{yellow},"","","");
	$xpos = 210; $ypos = 77; $width = 153; $height = 18;
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{blue},"","","");
	$xpos = 384; $ypos = 167; $width = 158; $height = 18;
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{blue},"","","");
	$xpos = 125; $ypos = 150; $width = 50; $height = 18;
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{black},"","","");
	# ADD RED FE running alarm block
	$xpos = 15; $ypos = 51; $width = 182; $height = 74; $vis = "calc"; $calc = "(a&1)==1";
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{red},$vis,$calc,"$site\:FEC-$dcuid\_STATE_WORD");

	# ************* Create Main Status Banner *********************************************************************
	# Add ADC TIMEOUT Monitor
	$xpos = 17; $ypos = 29; $width = 12; $height = 17;
        $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_DIAG_WORD","0","0",$ecolors{red},$ecolors{green});
	# ADD Blinking FE running indicator
	$xpos = 29; $ypos = 29; $width = 12; $height = 17; $vis = "calc"; $calc = "(a&1)";
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{ltblue},$vis,$calc,"$site\:FEC-$dcuid\_TIME_DIAG");
	# Add timing label
	$xpos = 63; $ypos = 32; $width = 45; $height = 15; 
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"TIMING STATUS",$ecolors{white});
	# Add i/o status label
	$xpos = 254; $ypos = 32; $width = 69; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"I/O STATUS",$ecolors{white});
	# Add DAQ status label
	$xpos = 429; $ypos = 32; $width = 69; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"DAQ STATUS",$ecolors{white});
	# Add DCUID Monitor
	$xpos = 507; $ypos = 32; $width = 25; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:DAQ-FEC_$dcuid\_DCU_ID",$ecolors{white},$ecolors{blue},"static");

	# ************* Create Timing Info Block *********************************************************************
	# Add CPU Meter
	$xpos = 19; $ypos = 53; $width = 85; $height = 65;
        $medmdata .= ("CDS::medmGen::medmGenMeter") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_CPU_METER",$ecolors{ltblue},$ecolors{mdblue});
	# Add BURT Status Monitor
	$xpos = 128; $ypos = 65; $width = 47; $height = 22;
        $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_BURT_RESTORE","0","0",$ecolors{green},$ecolors{red});
	# Add BURT button
	$xpos = 134; $ypos = 66; $width = 35; $height = 20;
        $medmdata .= ("CDS::medmGen::medmGenMessage") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_BURT_RESTORE",$ecolors{white},$ecolors{blue},"BURT","1");
	# Add DIAG RESET button
	$xpos = 117; $ypos = 95; $width = 70; $height = 20;
        $medmdata .= ("CDS::medmGen::medmGenMessage") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_DIAG_RESET",$ecolors{white},$ecolors{blue},"Diag Reset","1");

	# Add GPS Time label
	$xpos = 18; $ypos = 134; $width = 69; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"GPS Time",$ecolors{black});
	# Add GPS Time Monitor
	$xpos = 81; $ypos = 134; $width = 100; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_TIME_DIAG",$ecolors{white},$ecolors{blue},"static");

	# Add Sync Source label
	$xpos = 18; $ypos = 153; $width = 50; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"Sync Source",$ecolors{black});
	# Add 1PPS Sync Monitor
	$xpos = 125; $ypos = 153; $width = 50; $height = 16;
	$medmdata .= ("CDS::medmGen::medmGenTextDyn") -> ($xpos,$ypos,$width,$height,"1PPS",$ecolors{green},"A&2","$site\:FEC-$dcuid\_TIME_ERR");
	# Add TDS Sync Monitor
	$xpos = 126; $ypos = 153; $width = 50; $height = 16;
	$medmdata .= ("CDS::medmGen::medmGenTextDyn") -> ($xpos,$ypos,$width,$height,"TDS",$ecolors{green},"A&4","$site\:FEC-$dcuid\_TIME_ERR");
	# Add IOP Sync Monitor
	$xpos = 126; $ypos = 153; $width = 50; $height = 16;
	$medmdata .= ("CDS::medmGen::medmGenTextDyn") -> ($xpos,$ypos,$width,$height,"IOP",$ecolors{green},"A&8","$site\:FEC-$dcuid\_TIME_ERR");
	# Add NO SYNC Alaram Monitor
	$xpos = 125; $ypos = 153; $width = 50; $height = 16;
	$medmdata .= ("CDS::medmGen::medmGenTextDyn") -> ($xpos,$ypos,$width,$height,"NO SYNC",$ecolors{red},"(A&255) == 0","$site\:FEC-$dcuid\_TIME_ERR");

	# Add cycle/user time label
	$xpos = 18; $ypos = 172; $width = 50; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"CYC/USR",$ecolors{black});
	# Add Cycle Time Monitor
	$xpos = 75; $ypos = 172; $width = 40; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_ADC_WAIT",$ecolors{white},$ecolors{black},"alarm");
	# Add User Time Monitor
	$xpos = 125; $ypos = 172; $width = 50; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_USR_TIME",$ecolors{white},$ecolors{black},"alarm");

	# Add cpu max time label
	$xpos = 18; $ypos = 192; $width = 50; $height = 15;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"CPU Max",$ecolors{black});
	# Add CPU Time Monitor
	$xpos = 75; $ypos = 192; $width = 40; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_CPU_METER",$ecolors{white},$ecolors{black},"alarm");
	# Add CPU Time Max Monitor
	$xpos = 125; $ypos = 192; $width = 50; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_CPU_METER_MAX",$ecolors{white},$ecolors{black},"alarm");

	# Following only for IOP
	if($adcMaster == 1)
	{
		# Add ADC Duotone Diag label
		$xpos = 18; $ypos = 212; $width = 50; $height = 15;
		$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"DT ADC",$ecolors{black});
		# Add ADC Duotone Monitor
		$xpos = 75; $ypos = 212; $width = 40; $height = 15;
		$medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_DUOTONE_TIME",$ecolors{white},$ecolors{black},"alarm");
		$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"DT ADC",$ecolors{black});
		# Add IRIG-B Diag label
		$xpos = 18; $ypos = 232; $width = 50; $height = 15;
		$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"IRIG-B",$ecolors{black});
		# Add IRIG-B Monitor
		$xpos = 75; $ypos = 232; $width = 40; $height = 15;
		$medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_IRIGB_TIME",$ecolors{white},$ecolors{black},"alarm");
		# Add DAC Duotone Diag label
		$xpos = 18; $ypos = 252; $width = 50; $height = 15;
		$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"DT DAC",$ecolors{black});
		# Add DAC Duotone Monitor
		$xpos = 75; $ypos = 252; $width = 40; $height = 15;
		$medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_DUOTONE_TIME_DAC",$ecolors{white},$ecolors{black},"alarm");
		# Add arrow to DAC DT On/Off
		$xpos = 115; $ypos = 252; $width = 30; $height = 15;
		$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"---->",$ecolors{black});
		# Add DAC DT On/Off
		$xpos = 144; $ypos = 246; $width = 50; $height = 35;
		$medmdata .= ("CDS::medmGen::medmGenChoice") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_DACDT_ENABLE",$ecolors{white},$ecolors{blue});
	}

	# ************* Create I/O Status Info Block *****************************************************************
	# Add Overflow counter reset
	$xpos = 210; $ypos = 53; $width = 70; $height = 18;
        $medmdata .= ("CDS::medmGen::medmGenMessage") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_OVERFLOW_RESET",$ecolors{white},$ecolors{blue},"OVERFLOWS","1");
	# Add Overflow Counter Monitor
	$xpos = 282; $ypos = 55; $width = 80; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_ACCUM_OVERFLOW",$ecolors{white},$ecolors{black},"alarm");

	# Add IPC Monitor related display
	$xpos = 210; $ypos = 77; $width = 85; $height = 18;
	$mdlNamelc = lc($mdlName);
	$relDisp = "$medmTarget\/$mdlNamelc\/$mdlName\_IPC_STATUS.adl";
        $medmdata .= ("CDS::medmGen::medmGenRelDisp") -> ($xpos,$ypos,$width,$height,$relDisp,$ecolors{white},$ecolors{blue},"RT NET STAT");
	# Add IPC Status Monitor
	$xpos = 314; $ypos = 79; $width = 40; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_DIAG1","0","3",$ecolors{red},$ecolors{green});

	# Add Guardian Alarm Monitor related display
	$xpos = 210; $ypos = 102; $width = 85; $height = 18;
	$mdlNamelc = lc($mdlName);
	$relDisp = "$medmTarget\/$mdlNamelc\/$mdlName\_ALARM_MONITOR.adl";
        $medmdata .= ("CDS::medmGen::medmGenRelDisp") -> ($xpos,$ypos,$width,$height,$relDisp,$ecolors{white},$ecolors{blue},"Guard (S/R)");
	# Add Guardian setpoint error  Counter Monitor
	$xpos = 299; $ypos = 104; $width = 30; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_GRD_SP_ERR_CNT",$ecolors{white},$ecolors{black},"alarm");
	# Add Guardian readback error  Counter Monitor
	$xpos = 331; $ypos = 104; $width = 30; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_GRD_RB_ERR_CNT",$ecolors{white},$ecolors{black},"alarm");

	# Add Coeff Reload
	$xpos = 210; $ypos = 126; $width = 152; $height = 18;
        $medmdata .= ("CDS::medmGen::medmGenMessage") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_LOAD_NEW_COEFF",$ecolors{white},$ecolors{blue},"Coeff Load","1");
	# Add Coeff load time Monitor
	$xpos = 210; $ypos = 146; $width = 152; $height = 18;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_MSG",$ecolors{white},$ecolors{blue},"static");

	# ************* Create DAQ Status Info Block *****************************************************************

	# Add DAQ Status label
	$xpos = 425; $ypos = 54; $width = 110; $height = 12;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"Status  CPS   SUM",$ecolors{black});
	# Add DAQ Status label
	$xpos = 387; $ypos = 73; $width = 24; $height = 12;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"DC",$ecolors{black});
	# Add DAQ Status Monitor
	$xpos = 407; $ypos = 72; $width = 14; $height = 15; $vis = "if zero"; $calc = "";
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{green},$vis,$calc,"$site\:DAQ-DC0\_$mdlName\_STATUS");
	$xpos = 407; $ypos = 72; $width = 14; $height = 15; $vis = "if not zero"; $calc = "";
        $medmdata .= ("CDS::medmGen::medmGenRectangle") -> ($xpos,$ypos,$width,$height,$ecolors{red},$vis,$calc,"$site\:DAQ-DC0\_$mdlName\_STATUS");
	# Add DAQ Status label
	$xpos = 387; $ypos = 106; $width = 24; $height = 12;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"FE",$ecolors{black});
	# Add DAQ Net Monitor
	$xpos = 407; $ypos = 105; $width = 14; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_FB_NET_STATUS","0","1",$ecolors{green},$ecolors{red});

	# Add DAQ Status label
	$xpos = 407; $ypos = 91; $width = 110; $height = 12;
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"    NET CHN   DRATE  TRATE",$ecolors{black});
	# Add DAQ DC chan count Monitor
	$xpos = 424; $ypos = 72; $width = 35; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:DAQ-DC0_$mdlName\_STATUS",$ecolors{white},$ecolors{black},"alarm","hexadecimal");
	# Add DAQ DC CRC Error Monitor
	$xpos = 464; $ypos = 72; $width = 35; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:DAQ-DC0_$mdlName\_CRC_CPS",$ecolors{white},$ecolors{black},"alarm");
	# Add DAQ DC CRC Error Sum Monitor
	$xpos = 504; $ypos = 72; $width = 35; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:DAQ-DC0_$mdlName\_CRC_SUM",$ecolors{white},$ecolors{black},"alarm");

	# Add DAQ chan count Monitor
	$xpos = 424; $ypos = 105; $width = 35; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:DAQ-FEC_$dcuid\_CHAN_CNT",$ecolors{white},$ecolors{black},"alarm");
	# Add DAQ + TP chan counter Monitor
	$xpos = 464; $ypos = 105; $width = 35; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:DAQ-FEC_$dcuid\_TOTAL",$ecolors{white},$ecolors{black},"alarm");
	# Add DAQ chan data rate Monitor
	$xpos = 504; $ypos = 105; $width = 35; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_DAQ_BYTE_COUNT",$ecolors{white},$ecolors{black},"alarm");

	# Add DAQ Reload
	$xpos = 384; $ypos = 126; $width = 158; $height = 18;
        $medmdata .= ("CDS::medmGen::medmGenMessage") -> ($xpos,$ypos,$width,$height,"$site\:DAQ-FEC_$dcuid\_LOAD_CONFIG",$ecolors{white},$ecolors{blue},"DAQ Reload","1");
	# Add DAQ load time Monitor
	$xpos = 384; $ypos = 146; $width = 158; $height = 18;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_MSGDAQ",$ecolors{white},$ecolors{blue},"static");

	# Add TP label
	$xpos = 386; $ypos = 169; $width = 80; $height = 12; 
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"Test Points",$ecolors{white});
	# Add AWG label
	$xpos = 497; $ypos = 169; $width = 20; $height = 12; 
	$medmdata .= ("CDS::medmGen::medmGenText") -> ($xpos,$ypos,$width,$height,"AWG",$ecolors{white});
	# Add TP Counter Monitor
	$xpos = 467; $ypos = 169; $width = 20; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_TP_CNT",$ecolors{white},$ecolors{black},"alarm");
	# Add AWG Monitor
	$xpos = 525; $ypos = 168; $width = 10; $height = 15;
        $medmdata .= ("CDS::medmGen::medmGenByte") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_AWGTPMAN_STAT","0","0",$ecolors{red},$ecolors{green});

	# Add Individual TP channel number monitors
	$xpos = 384; $ypos = 188; $width = 50; $height = 15;
	for(my $ii=0;$ii<5;$ii++)
	{
		$medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_GDS_MON_$ii",$ecolors{white},$ecolors{blue},"static");
		$ypos += 18;
	}
	$xpos = 438; $ypos = 188;
	for($ii=5;$ii<10;$ii++)
	{
		$medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_GDS_MON_$ii",$ecolors{white},$ecolors{blue},"static");
		$ypos += 18;
	}
	$xpos = 492; $ypos = 188;
	for(my $ii=10;$ii<15;$ii++)
	{
		$medmdata .= ("CDS::medmGen::medmGenTextMon") -> ($xpos,$ypos,$width,$height,"$site\:FEC-$dcuid\_GDS_MON_$ii",$ecolors{white},$ecolors{blue},"static");
		$ypos += 18;
	}
	# ****************** ADD ADC / DAC related disp *******************************************************************
	my $totalCards = 0;
	$mdlNamelc = lc($mdlName);
	$xpos = 215; $ypos = 172; $width = 30; $height = 20;
	$bxpos = 247; $bypos = 173; $bwidth = 21; $bheight = 18;
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
			$ypos = 172;
			$bxpos = 327;
			$bypos = 173;
		}
	}
	$bwidth = 28;
	$b1width = 14;
	for($ii=0;$ii<$dacCnt;$ii++)
	{
		$relDisp = "$medmTarget\/$mdlNamelc\/$mdlName\_DAC_MONITOR_$::dacCardNum[$ii].adl";
		if($dactype[$ii] eq "GSC_18AO8" ) {
			$medmdata .= ("CDS::medmGen::medmGenRelDisp") -> ($xpos,$ypos,$width,$height,$relDisp,$ecolors{white},$ecolors{dacblue},"D$::dacCardNum[$ii]");
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
			$ypos = 172;
			$bxpos = 327;
			$bypos = 173;
		}
	}

print OUTMEDM "$medmdata \n";
close OUTMEDM;

}
