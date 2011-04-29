#!/usr/bin/perl

%fpar = (
    "input" => "INMON",
    "excite" => "EXCMON",
    "offset" => "OFFSET",
    "outgain" => "GAIN",
    "limit" =>  "LIMIT",
    "tp" => "OUTMON",
    "out16Hz" => "OUT16",
    "output" => "OUTPUT",
	 );

%ipar = (
    "sw1" => "SW1",
    "sw2" => "SW2",
    "swRset" => "RSET", 
    "switchR1" => "SW1R",
    "switchR2" => "SW2R",
    "saveSwitch1" => "SW1S",
    "saveSwitch2" => "SW2S",
	 );

%iparAlarmSev = (
    "limitHihiSev" => "LIMIT.HHSV",
    "limitHighSev" => "LIMIT.HSV",
    "limitLowSev" => "LIMIT.LSV",
    "limitLoloSev" => "LIMIT.LLSV",
    "offsetHihiSev" => "OFFSET.HHSV",
    "offsetHighSev" => "OFFSET.HSV",
    "offsetLowSev" => "OFFSET.LSV",
    "offsetLoloSev" => "OFFSET.LLSV",
    "outgainHihiSev" => "GAIN.HHSV",
    "outgainHighSev" => "GAIN.HSV",
    "outgainLowSev" => "GAIN.LSV",
    "outgainLoloSev" => "GAIN.LLSV",
    "saveSwitch1HihiSev" => "SW1R.HHSV",
    "saveSwitch1HighSev" => "SW1R.HSV",
    "saveSwitch1LowSev" => "SW1R.LSV",
    "saveSwitch1LoloSev" => "SW1R.LLSV",
    "saveSwitch2HihiSev" => "SW2R.HHSV",
    "saveSwitch2HighSev" => "SW2R.HSV",
    "saveSwitch2LowSev" => "SW2R.LSV",
    "saveSwitch2LoloSev" => "SW2R.LLSV",
	 );

%iparAlarmStat = (
    "limitStat" => "LIMIT.STAT",
    "offsetStat" => "OFFSET.STAT",
    "outgainStat" => "GAIN.STAT",
    "saveSwitch1Stat" => "SW1R.STAT",
    "saveSwitch2Stat" => "SW2R.STAT",
	 );

%spar = (
    "labels0" => "Name00",
    "labels1" => "Name01",
    "labels2" => "Name02",
    "labels3" => "Name03",
    "labels4" => "Name04",
    "labels5" => "Name05",
    "labels6" => "Name06",
    "labels7" => "Name07",
    "labels8" => "Name08",
    "labels9" => "Name09",
	 );


die "Usage: $PROGRAM_NAME <system name> [ST skeleton file name] < <system definition file>" 
	if (@ARGV != 1 && @ARGV != 2);
$skeleton = "../util/skeleton.st";
if (@ARGV == 2) { $skeleton = $ARGV[1]; }
open(IN,"<-") || die "cannot open standard input\n";

$cnt = 0;
$mcnt = 0;
$phase = 0;
my @eFields;

$names1 = "%TYPE% %NAME%[MAX_MODULES];\nassign %NAME% to\n{\n";
$names2 = "%%static  fmSubSysMap  fmmap0 [MAX_MODULES] = { \n%%";

$do_epics_input = 0;

# Determine whether passed name needs to become a top name
# i.e. whether the system/subsystem parts need to excluded 
sub is_top_name {
   ($_) =  @_;
   if (/^GDS_MON/) { return 0; } # Do not count testpoint channels
   @d = split(/_/);
   $d = shift @d;
   #print "$d @top_names\n";
   foreach $item (@top_names) {
	#print  "   $item $d\n";
	if ($item eq $d) { return 1; }
   }
   return 0;
};

# Transform record name for exculsion of sys/subsystem parts
# This function replaces first underscode with the hyphen
sub top_name_transform {
   ($name) =  @_;
   $name =~ s/_/-/;
   return $name;
};

while (<IN>) {
    s/^\s//g;
    s/\s$//g;

    if (substr($_,0,13) eq "epics_filters") {
	$do_epics_input = 1;
	$fpar{"epics_input"} = "INPUT";
	$fpar{"epics_exc"} = "EXC";
    } elsif (substr($_,0,7) eq "systems") {
	@systems = split(/\s+/, $_);	
	shift @systems;
	print "systems are @systems\n";
    } elsif (/^top_names/) {
	@top_names = split(/\s+/, $_);	
	shift @top_names;
	print "top_names are @top_names\n";
    } elsif (/^excitations/) {
	@excitations = split(/\s+/, $_);	
	shift @excitations;
	print "excitations are @excitations\n";
    } elsif (/^test_points/) {
	@testpoints = split(/\s+/, $_);	
	shift @testpoints;
	print "testpoints are @testpoints\n";
    } elsif (substr($_,0,10) eq "gds_config") {
	$gds_rmid = 0;
	$site = "";
	($junk, $gds_excnum_base, $gds_tpnum_base, $gds_exc_sys_inc, $gds_tp_sys_inc, $gds_rmid, $site, $gds_datarate, $dcuId, $ifoid) = split(/\s+/, $_);
	$gds_specified = 1;
	if ($gds_datarate eq undef) {
	  $gds_datarate = $gds_excnum_base < 10000? 16384: 2048;
	}
	if ($dcuId eq undef) {
	  $dcuId = 10;
	}
	if ($ifoid eq undef) {
	  $ifoid = 0;
	}
	$gds_ifo = 1;
	print "GDS NODE  $gds_rmid\n";
	if ($site eq undef || $site eq "") {
	  $site = "M1";
	}
	$gds_exc_dcu_id = 13 + (int ($gds_excnum_base >= 10000)) * 2;
	$gds_tp_dcu_id = 13 + int ($gds_tpnum_base / 10000);
    } elsif (substr($_,0,5) eq "EPICS") {
	($junk, $epics_type, $epics_filt_var, $epics_coeff_var, $epics_epics_var ) = split(/\s+/, $_);	
#	print "$junk, $epics_addr, $epics_type\n";
	$epics_specified = 1;
    } elsif (substr($_,0,10) eq "INVARIABLE") {
	die "Unspecified EPICS parameters" unless $epics_specified;
	($junk, $v_name, $v_var, $v_type, $ve_type, $v_init,@eFields ) = split(/\s+/, $_);
	$vdecl .= "$v_type evar_$v_name;\n";
        if ($v_name ne "BURT_RESTORE") {
          $vstat_decl .= "int evar_${v_name}_Stat;\n";
          if ($ve_type ne "bi") {
            $vhihi_decl .= "int evar_${v_name}_HihiSev;\n";
            $vhigh_decl .= "int evar_${v_name}_HighSev;\n";
            $vlow_decl  .= "int evar_${v_name}_LowSev;\n";
            $vlolo_decl .= "int evar_${v_name}_LoloSev;\n";
          }
        }
        my $top_name = is_top_name($v_name);
   	my $tv_name;
        if ($top_name) {
	  $tv_name = top_name_transform($v_name);
	  $vdecl .= "assign evar_$v_name to \"{ifo}:${tv_name}\";\n";
          if ($v_name ne "BURT_RESTORE") {
            $vstat_decl .="assign evar_${v_name}_Stat to \"{ifo}:${tv_name}.STAT\";\n";
            if ($ve_type ne "bi") {
              $vhihi_decl .="assign evar_${v_name}_HihiSev to \"{ifo}:${tv_name}.HHSV\";\n";
              $vhigh_decl .="assign evar_${v_name}_HighSev to \"{ifo}:${tv_name}.HSV\";\n";
              $vlow_decl  .="assign evar_${v_name}_LowSev to \"{ifo}:${tv_name}.LSV\";\n";
              $vlolo_decl .="assign evar_${v_name}_LoloSev to \"{ifo}:${tv_name}.LLSV\";\n";
            }
          }
 	} else {
	  $vdecl .= "assign evar_$v_name to \"{ifo}:{sys}-{subsys}${v_name}\";\n";
          if ($v_name ne "BURT_RESTORE") {
            $vstat_decl .="assign evar_${v_name}_Stat to \"{ifo}:{sys}-{subsys}${v_name}.STAT\";\n";
            if ($ve_type ne "bi") {
              $vhihi_decl .="assign evar_${v_name}_HihiSev to \"{ifo}:{sys}-{subsys}${v_name}.HHSV\";\n";
              $vhigh_decl .="assign evar_${v_name}_HighSev to \"{ifo}:{sys}-{subsys}${v_name}.HSV\";\n";
              $vlow_decl  .="assign evar_${v_name}_LowSev to \"{ifo}:{sys}-{subsys}${v_name}.LSV\";\n";
              $vlolo_decl .="assign evar_${v_name}_LoloSev to \"{ifo}:{sys}-{subsys}${v_name}.LLSV\";\n";
            }
          }
	}

	$vinit .= "%% evar_$v_name  = $v_init;\n";
	$vinit .= "pvPut(evar_$v_name);\n";
	$vinit .= "%%       pEpics->${v_var} = evar_$v_name;\n";

        if ($v_name ne "BURT_RESTORE") {
          if ($ve_type ne "bi") {
            $vinit .= "%% evar_${v_name}_HihiSev = NO_ALARM;\n";
            $vinit .= "pvPut(evar_${v_name}_HihiSev);\n";
#             $vinit .= "%%       pEpics->${v_var}_HihiSev = evar_${v_name}_HihiSev;\n";   # ????????
            $vinit .= "%% evar_${v_name}_HighSev = NO_ALARM;\n";
            $vinit .= "pvPut(evar_${v_name}_HighSev);\n";
#             $vinit .= "%%       pEpics->${v_var}_HighSev = evar_${v_name}_HighSev;\n";   # ????????
            $vinit .= "%% evar_${v_name}_LowSev = NO_ALARM;\n";
            $vinit .= "pvPut(evar_${v_name}_LowSev);\n";
#             $vinit .= "%%       pEpics->${v_var}_LowSev = evar_${v_name}_LowSev;\n";     # ????????
            $vinit .= "%% evar_${v_name}_LoloSev = NO_ALARM;\n";
            $vinit .= "pvPut(evar_${v_name}_LoloSev);\n";
#             $vinit .= "%%       pEpics->${v_var}_LoloSev = evar_${v_name}_LoloSev;\n";   # ????????
          }
        }

	$vupdate .= "pvGet(evar_$v_name);\n";
	$vupdate .= "%%  rfm_assign(pEpics->${v_var}, evar_$v_name);\n";

        if ($v_name ne "BURT_RESTORE") {
          $vupdate .= "pvGet(evar_${v_name}_Stat);\n";
#             $vupdate .= "rfm_assign(pEpics->${v_var}_Stat, evar_${v_name}_Stat);\n";     # ????????
          $vupdate .= "%%  if ( (evar_${v_name}_Stat == HIHI_ALARM) ||\n";
          $vupdate .= "%%       (evar_${v_name}_Stat == LOLO_ALARM) ) {\n";
          $vupdate .= "%%    setpointStatusCount++;\n";
          $vupdate .= "%%    if (setpointDisplayCount < 10) {\n";
          $vupdate .= "%%      strcpy(setpointErrId[setpointDisplayCount], \"$v_name\");\n";
          $vupdate .= "%%      strcpy(setpointMajorCount[setpointDisplayCount], \"1\");\n";
          $vupdate .= "%%      strcpy(setpointMinorCount[setpointDisplayCount], \"-\");\n";
          $vupdate .= "%%      setpointDisplayCount++;\n";
          $vupdate .= "%%    }\n";
          $vupdate .= "%%  }\n";
          $vupdate .= "%%  else if ( (evar_${v_name}_Stat == HIGH_ALARM) ||\n";
          $vupdate .= "%%            (evar_${v_name}_Stat == LOW_ALARM) ) {\n";
          $vupdate .= "%%    setpointStatusCount++;\n";
          $vupdate .= "%%    if (setpointDisplayCount < 10) {\n";
          $vupdate .= "%%      strcpy(setpointErrId[setpointDisplayCount], \"$v_name\");\n";
          $vupdate .= "%%      strcpy(setpointMajorCount[setpointDisplayCount], \"-\");\n";
          $vupdate .= "%%      strcpy(setpointMinorCount[setpointDisplayCount], \"1\");\n";
          $vupdate .= "%%      setpointDisplayCount++;\n";
          $vupdate .= "%%    }\n";
          $vupdate .= "%%  }\n";
        }

        if ($top_name) {
		$vardb .= "grecord(${ve_type},\"%IFO%:${tv_name}\")\n";
	} else {
		$vardb .= "grecord(${ve_type},\"%IFO%:%SYS%-%SUBSYS%${v_name}\")\n";
	}
	$vardb .= "{\n";
	for($efC=0;$efC<12;$efC++)
	{
		if($eFields[$efC])
		{
			$vardb .= "    $eFields[$efC]\n";
		}
	}
#	$vardb .= "    field(PREC,\"3\")\n";
	$vardb .= "}\n";

    } elsif (substr($_,0,9) eq "WFS_PHASE") {
	die "Unspecified EPICS parameters" unless $epics_specified;
	($junk, $v_name, $v_var, $v_type, $ve_type, $v_init, $v_efield1, $v_efield2, $v_efield3, $v_efield4 ) = split(/\s+/, $_);
	$vdecl .= "$v_type evar_${v_name}_d;\n";
	$vdecl .= "$v_type evar_${v_name}_r;\n";
        my $top_name = is_top_name($v_name);
   	my $tv_name;
        if ($top_name) {
	  	$tv_name = top_name_transform($v_name);
		$vdecl .= "assign evar_${v_name}_d to \"{ifo}:${tv_name}_D\";\n";
		$vdecl .= "assign evar_${v_name}_r to \"{ifo}:${tv_name}_R\";\n";
	} else {
		$vdecl .= "assign evar_${v_name}_d to \"{ifo}:{sys}-{subsys}${v_name}_D\";\n";
		$vdecl .= "assign evar_${v_name}_r to \"{ifo}:{sys}-{subsys}${v_name}_R\";\n";
	}

	$vinit .= "%% evar_${v_name}_d  = $v_init;\n";
	$vinit .= "%% evar_${v_name}_r  = $v_init;\n";
	$vinit .= "pvPut(evar_${v_name}_d);\n";
	$vinit .= "pvPut(evar_${v_name}_r);\n";
	$vinit .= "%%       pEpics->${v_var}[0][0] = 0.0;\n";
	$vinit .= "%%       pEpics->${v_var}[0][1] = 0.0;\n";
	$vinit .= "%%       pEpics->${v_var}[1][0] = 0.0;\n";
	$vinit .= "%%       pEpics->${v_var}[1][1] = 0.0;\n";

	$vupdate .= "pvGet(evar_${v_name}_d);\n";
	$vupdate .= "pvGet(evar_${v_name}_r);\n";
	$vupdate .= "evar_${v_name}_r *= M_PI/180.0;\n";
	$vupdate .= "evar_${v_name}_d *= M_PI/180.0;\n";
	$vupdate .= "if (evar_${v_name}_d == 0.0) evar_${v_name}_d = .1*(M_PI / 180);\n";
	$vupdate .= "pEpics->${v_var}[0][0] = sin(evar_${v_name}_r + evar_${v_name}_d)/sin(evar_${v_name}_d);\n";
	$vupdate .= "pEpics->${v_var}[0][1] = cos(evar_${v_name}_r + evar_${v_name}_d)/sin(evar_${v_name}_d);\n";
	$vupdate .= "pEpics->${v_var}[1][0] = sin(evar_${v_name}_r)/sin(evar_${v_name}_d);\n";
	$vupdate .= "pEpics->${v_var}[1][1] = cos(evar_${v_name}_r)/sin(evar_${v_name}_d);\n";

	if ($top_name) {
		$vardb .= "grecord(${ve_type},\"%IFO%:${tv_name}_D\")\n";
	} else {
		$vardb .= "grecord(${ve_type},\"%IFO%:%SYS%-%SUBSYS%${v_name}_D\")\n";
	}
	$vardb .= "{\n";
#	$vardb .= "    field(PREC,\"3\")\n";
	$vardb .= "    $v_efield1\n";
	$vardb .= "    $v_efield2\n";
	$vardb .= "    $v_efield3\n";
	$vardb .= "    $v_efield4\n";
	$vardb .= "}\n";
	if ($top_name) {
		$vardb .= "grecord(${ve_type},\"%IFO%:${tv_name}_R\")\n";
	} else {
		$vardb .= "grecord(${ve_type},\"%IFO%:%SYS%-%SUBSYS%${v_name}_R\")\n";
	}
	$vardb .= "{\n";
#	$vardb .= "    field(PREC,\"3\")\n";
	$vardb .= "    $v_efield1\n";
	$vardb .= "    $v_efield2\n";
	$vardb .= "    $v_efield3\n";
	$vardb .= "    $v_efield4\n";
	$vardb .= "}\n";
    } elsif (substr($_,0,5) eq "PHASE") {
	die "Unspecified EPICS parameters" unless $epics_specified;
        $phase++;
	($junk, $v_name, $v_var, $v_type, $ve_type, $v_init, $v_efield1, $v_efield2, $v_efield3, $v_efield4 ) = split(/\s+/, $_);
	$vdecl .= "$v_type evar_$v_name;\n";
        my $top_name = is_top_name($v_name);
   	my $tv_name;
        if ($top_name) {
	  	$tv_name = top_name_transform($v_name);
		$vdecl .= "assign evar_$v_name to \"{ifo}:${tv_name}\";\n";
	} else {
		$vdecl .= "assign evar_$v_name to \"{ifo}:{sys}-{subsys}${v_name}\";\n";
	}

	$vinit .= "%% evar_$v_name  = $v_init;\n";
	$vinit .= "pvPut(evar_$v_name);\n";
        $vinit .= "%%       rad_angle = (double)((evar_$v_name * M_PI)/180.);\n";
	$vinit .= "%%       pEpics->${v_var}[0] = sin(rad_angle);\n";
	$vinit .= "%%       pEpics->${v_var}[1] = cos(rad_angle);\n";

	$vupdate .= "pvGet(evar_$v_name);\n";
        $vupdate .= "%%  rad_angle = (double)((evar_$v_name * M_PI)/180.);\n";
	$vupdate .= "%% rfm_assign(pEpics->${v_var}[0], sin(rad_angle));\n";
	$vupdate .= "%% rfm_assign(pEpics->${v_var}[1], cos(rad_angle));\n";

	if ($top_name) {
		$vardb .= "grecord(${ve_type},\"%IFO%:${tv_name}\")\n";
	} else {
		$vardb .= "grecord(${ve_type},\"%IFO%:%SYS%-%SUBSYS%${v_name}\")\n";
	}
	$vardb .= "{\n";
#	$vardb .= "    field(PREC,\"3\")\n";
	$vardb .= "    $v_efield1\n";
	$vardb .= "    $v_efield2\n";
	$vardb .= "    $v_efield3\n";
	$vardb .= "    $v_efield4\n";
	$vardb .= "}\n";
    } elsif (substr($_,0,9) eq "MOMENTARY") {
	die "Unspecified EPICS parameters" unless $epics_specified;
	($junk, $v_name, $v_var, $v_type, $ve_type, $v_init, $v_efield1, $v_efield2, $v_efield3, $v_efield4 ) = split(/\s+/, $_);
	$vdecl .= "$v_type evar_$v_name;\n";
        my $top_name = is_top_name($v_name);
   	my $tv_name;
        if ($top_name) {
	  	$tv_name = top_name_transform($v_name);
		$vdecl .= "assign evar_$v_name to \"{ifo}:${tv_name}\";\n";
	} else {
		$vdecl .= "assign evar_$v_name to \"{ifo}:{sys}-{subsys}${v_name}\";\n";
	}

	$vinit .= "%% evar_$v_name  = $v_init;\n";
	$vinit .= "pvPut(evar_$v_name);\n";
	$vinit .= "%%       pEpics->${v_var} = evar_$v_name;\n";

	$vupdate .= "pvGet(evar_$v_name);\n";
	$vupdate .= "%% if(evar_$v_name != 0) {\n";
	$vupdate .= "%%  rfm_assign(pEpics->${v_var}, evar_$v_name);\n";
	$vupdate .= "%% }\n";
	$vupdate .= "%% evar_$v_name  = $v_init;\n";
	$vupdate .= "pvPut(evar_$v_name);\n";

        if ($top_name) {
		$vardb .= "grecord(${ve_type},\"%IFO%:${tv_name}\")\n";
	} else {
		$vardb .= "grecord(${ve_type},\"%IFO%:%SYS%-%SUBSYS%${v_name}\")\n";
	}
	$vardb .= "{\n";
#	$vardb .= "    field(PREC,\"3\")\n";
	$vardb .= "    $v_efield1\n";
	$vardb .= "    $v_efield2\n";
	$vardb .= "    $v_efield3\n";
	$vardb .= "    $v_efield4\n";
	$vardb .= "}\n";
    } elsif (substr($_,0,11) eq "OUTVARIABLE") {
	die "Unspecified EPICS parameters" unless $epics_specified;
	($junk, $v_name, $v_var, $v_type, $ve_type, $v_init,@eFields ) = split(/\s+/, $_);
	#print "$v_name = efields $eFields[0] $eFields[1] $eFields[2] $eFields[3] $eFields[4] $eFields[5] $eFields[6]\n";
	$vdecl .= "$v_type evar_$v_name;\n";
        my $top_name = is_top_name($v_name);
   	my $tv_name;
        if ($top_name) {
	  	$tv_name = top_name_transform($v_name);
		$vdecl .= "assign evar_$v_name to \"{ifo}:${tv_name}\";\n";
	} else {
		$vdecl .= "assign evar_$v_name to \"{ifo}:{sys}-{subsys}${v_name}\";\n";
	}

	$vinit .= "%% evar_$v_name  = $v_init;\n";
	$vinit .= "pvPut(evar_$v_name);\n";
	$vinit .= "%%       pEpics->${v_var} = evar_$v_name;\n";

	if ($v_type eq "float") {
	    $vupdate .= "evar_$v_name = fpvalidate(pEpics->${v_var});\n";
	} else {
	    $vupdate .= "evar_$v_name = pEpics->${v_var};\n";
	}
	$vupdate .= "pvPut(evar_$v_name);\n";

	if ($top_name) {
		$vardb .= "grecord(${ve_type},\"%IFO%:${tv_name}\")\n";
	} else {
		$vardb .= "grecord(${ve_type},\"%IFO%:%SYS%-%SUBSYS%${v_name}\")\n";
	}
	$vardb .= "{\n";
	if ($v_type eq "float") {
		$vardb .= "    field(PREC,\"3\")\n";
	}
	for($efC=0;$efC<12;$efC++)
	{
		if($eFields[$efC])
		{
			$vardb .= "    $eFields[$efC]\n";
		}
	}
	$vardb .= "}\n";
    } elsif (substr($_,0,12) eq "REMOTE_INTLK") {
	die "Unspecified EPICS parameters" unless $epics_specified;
	($junk, $v_name, $v_var, $v_type, $ve_type, $v_init, $v_efield1, $v_efield2, $v_efield3, $v_efield4 ) = split(/\s+/, $_);
	$temp = $v_name;
	$temp =~ s/\-/\_/g;
	$vdecl .= "$v_type evar_$temp;\n";

        my $top_name = is_top_name($temp);
   	my $tv_name;
        if ($top_name) {
	  	$tv_name = top_name_transform($temp);
		$vdecl .= "assign evar_$temp to \"{ifo}:${tv_name}\";\n";
	} else {
		$vdecl .= "assign evar_$temp to \"{ifo}:{sys}-${temp}\";\n";
	}
#	$vdecl .= "$v_type evar_$temp\_RI;\n";
#	$vdecl .= "assign evar_$temp\_RI to \"{ifo}:${v_name}\";\n";

	$vinit .= "%% evar_$temp  = $v_init;\n";
	$vinit .= "%%       pEpics->${v_var} = evar_$temp;\n";

	$vupdate .= "evar_$temp = pEpics->${v_var};\n";
	$vupdate .= "pvPut(evar_$temp);\n";
	$vupdate .= "if (evar_${temp} == 0) {\n";
#	$vupdate .= "\tevar_$temp\_RI = 0;\n";
#	$vupdate .= "\tpvPut(evar_$temp\_RI);\n";
	$vupdate .= "%%\tshort s[1];\n";
	$vupdate .= "%%\ts[0] = 0;\n";
	$vupdate .= "%%\tezcaPut(\"$site:$v_name\", ezcaShort,1,s);\n";
	$vupdate .= "}\n";

	if ($top_name) {
		$vardb .= "grecord(${ve_type},\"%IFO%:${tv_name}\")\n";
	} else {
		$vardb .= "grecord(${ve_type},\"%IFO%:%SYS%-%SUBSYS%${temp}\")\n";
	}
	$vardb .= "{\n";
	$vardb .= "}\n";
    } elsif (substr($_,0,11) eq "EZ_CA_WRITE") {
	($junk, $v_name, $v_var) = split(/\s+/, $_);
	#$vupdate .= "%% ezcaPut(\"$v_name\", ezcaDouble, 1, &pEpics->${v_var});\n";
    } elsif (substr($_,0,10) eq "EZ_CA_READ") {
	($junk, $v_name, $v_var) = split(/\s+/, $_);
	#$vupdate .= "%%ezcaGet(\"$v_name\", ezcaDouble, 1, &pEpics->${v_var});\n";
    } elsif (substr($_,0,6) eq "DAQVAR") {
	die "Unspecified EPICS parameters" unless $epics_specified;
	($junk, $v_name, $v_type, $ve_type, $v_init, $v_efield1, $v_efield2, $v_efield3, $v_efield4 ) = split(/\s+/, $_);

	# Do not patch in the hyphen, it already there
        if (is_top_name($v_name)) {
		$vardb .= "grecord(${ve_type},\"%IFO%:DAQ-${v_name}\")\n";
	} else {
		$vardb .= "grecord(${ve_type},\"%IFO%:DAQ-FEC_%SUBSYS%${v_name}\")\n";
	}
    } elsif (substr($_,0,5) eq "DUMMY") {
	die "Unspecified EPICS parameters" unless $epics_specified;
	($junk, $v_name, $v_type, $ve_type, $v_init, $v_efield1, $v_efield2, $v_efield3, $v_efield4 ) = split(/\s+/, $_);
        my $top_name = is_top_name($v_name);
   	my $tv_name;
        if ($top_name) {
	  	$tv_name = top_name_transform($v_name);
		$vardb .= "grecord(${ve_type},\"%IFO%:${tv_name}\")\n";
	} else {
		$vardb .= "grecord(${ve_type},\"%IFO%:%SYS%-%SUBSYS%${v_name}\")\n";
	}

    } elsif (substr($_,0,6) eq "MATRIX") {
	die "Unspecified EPICS parameters" unless $epics_specified;
	($junk, $m_name, $m_dims, $m_var ) = split(/\s+/, $_);
	$m_dims =~ s/\s//g;
	($x, $y) = split (/x/, $m_dims);
#	print "$x $y\n";
	$xXy = $x * $y;

#       $mdecl .= "float matrix${m_name}LOW_[$xXy];\n";
#       $mdecl .= "float matrix${m_name}HI_[$xXy];\n";
	$mdecl .= "float matrix${m_name}[$xXy];\n";
	$mdecl .= "assign matrix${m_name} to { \n";

        $mstat_decl .= "int matrix${m_name}Stat_[$xXy];\n";
        $mstat_decl .= "assign matrix${m_name}Stat_ to {\n";
        $mhihi_decl .= "int matrix${m_name}HihiSev_[$xXy];\n";
        $mhihi_decl .= "assign matrix${m_name}HihiSev_ to {\n";
        $mhigh_decl .= "int matrix${m_name}HighSev_[$xXy];\n";
        $mhigh_decl .= "assign matrix${m_name}HighSev_ to {\n";
        $mlow_decl  .= "int matrix${m_name}LowSev_[$xXy];\n";
        $mlow_decl  .= "assign matrix${m_name}LowSev_ to {\n";
        $mlolo_decl .= "int matrix${m_name}LoloSev_[$xXy];\n";
        $mlolo_decl .= "assign matrix${m_name}LoloSev_ to {\n";

	$minit .= "%%  for (ii = 0; ii < ${x}; ii++)\n";
	$minit .= "%%    for (jj = 0; jj < ${y}; jj++) {\n";
        $minit .= "%%       ij = ii * ${y} + jj;\n";
	$minit .= "         matrix${m_name}[ij] = 0.;\n";
	$minit .= "         pvPut(matrix${m_name}[ij]);\n";
	$minit .= "%%       pEpics->${m_var}[ii][jj] = 0.;\n";

        $minit .= "         matrix${m_name}HihiSev_[ij] = NO_ALARM;\n";
        $minit .= "         pvPut(matrix${m_name}HihiSev_[ij]);\n";
#          $minit .= "%%       pEpics->${m_var}_HihiSev[ii][jj] = NO_ALARM;\n";  # ??????
        $minit .= "         matrix${m_name}HighSev_[ij] = NO_ALARM;\n";
        $minit .= "         pvPut(matrix${m_name}HighSev_[ij]);\n";
#          $minit .= "%%       pEpics->${m_var}_HighSev[ii][jj] = NO_ALARM;\n";  # ??????
        $minit .= "         matrix${m_name}LowSev_[ij] = NO_ALARM;\n";
        $minit .= "         pvPut(matrix${m_name}LowSev_[ij]);\n";
#          $minit .= "%%       pEpics->${m_var}_LowSev[ii][jj] = NO_ALARM;\n";   # ??????
        $minit .= "         matrix${m_name}LoloSev_[ij] = NO_ALARM;\n";
        $minit .= "         pvPut(matrix${m_name}LoloSev_[ij]);\n";
#          $minit .= "%%       pEpics->${m_var}_LoloSev[ii][jj] = NO_ALARM;\n";  # ??????

	$minit .= "%%    }\n";

#       $mread .= "%%  strcpy(matrixID, \"${m_name}\");\n";
#       $mread .= "%%  rcode = readMatrixThreshFile(&fmc,\n";
#       $mread .= "%%                               matrixID,\n";
#       $mread .= "%%                               &matrix${m_name}LOW_[0],\n";
#       $mread .= "%%                               &matrix${m_name}HI_[0],\n";
#       $mread .= "%%                               ${x},\n";
#       $mread .= "%%                               ${y},\n";
#       $mread .= "%%                               0);\n";
#       $mread .= "%%  if (rcode) {\n";
#       $mread .= "%%    strcpy(msg, thresholdErrMsg[rcode + 2]);\n";
#       $mread .= "      pvPut(msg);\n";
#       $mread .= "%%    if (rcode != TF_PARSE_ERROR) {\n";
#       $mread .= "%%      strcat(msg, \"\\n\");\n";
#       $mread .= "%%      fprintf(stderr, \"%s\", msg);\n";
#       $mread .= "%%      matrixStatusFlag = 1 - rcode;\n";
#       $mread .= "%%    }\n";
#       $mread .= "%%  }\n";

	$mupdate .= "%%  for (ii = 0; ii < ${x}; ii++)\n";
	$mupdate .= "%%    for (jj = 0; jj < ${y}; jj++) {\n";
        $mupdate .= "%%      ij = ii * ${y} + jj;\n";
	$mupdate .= "        pvGet(matrix${m_name}[ij]);\n";
#       $mupdate .= "%%      if ( (matrix${m_name}[ij] < matrix${m_name}LOW_[ij]) ||\n";
#       $mupdate .= "%%           (matrix${m_name}[ij] > matrix${m_name}HI_[ij]) ) {\n";
#       $mupdate .= "%%        matrixThreshCount++;\n";
#       $mupdate .= "%%        if (matrixStatusFlag && (matrixDisplayCount < 10) ) {\n";
#       $mupdate .= "%%           sprintf(matrixErr[matrixDisplayCount], \"%s%1d%1d\",\n";
#       $mupdate .= "%%                   \"${m_name}\", (ii + 1), (jj + 1));\n";
#       $mupdate .= "%%           matrixDisplayCount++;\n";
#       $mupdate .= "%%        }\n";

#       $mupdate .= "%%      }\n";
	$mupdate .= "%%      rfm_assign(pEpics->${m_var}[ii][jj], matrix${m_name}[ij]);\n";

        $mupdate .= "        pvGet(matrix${m_name}Stat_[ij]);\n";
#          $mupdate .= "%%      rfm_assign(pEpics->${m_var}_Stat_[ii][jj],     # ??????
#                                          matrix${m_name}Stat_[ij]);\n";      # ??????
        $mupdate .= "%%      if ( (matrix${m_name}Stat_[ij] == HIHI_ALARM) ||\n";
        $mupdate .= "%%           (matrix${m_name}Stat_[ij] == LOLO_ALARM) ) {\n";
        $mupdate .= "%%        matrixStatusMajCount++;\n";
        $mupdate .= "%%      }\n";
        $mupdate .= "%%      else if ( (matrix${m_name}Stat_[ij] == HIGH_ALARM) ||\n";
        $mupdate .= "%%                (matrix${m_name}Stat_[ij] == LOW_ALARM) ) {\n";
        $mupdate .= "%%        matrixStatusMinCount++;\n";
        $mupdate .= "%%      }\n";

	$mupdate .= "%%    }\n";

        $mupdate .= "%%    if (matrixStatusMajCount || matrixStatusMinCount) {\n";
        $mupdate .= "%%      setpointStatusCount++;\n";
        $mupdate .= "%%      if (setpointDisplayCount < 10) {\n";
        $mupdate .= "%%        strcpy(setpointErrId[setpointDisplayCount], \"${m_name}\");\n";
        $mupdate .= "%%        if (matrixStatusMajCount) {\n";
        $mupdate .= "%%          sprintf(setpointMajorCount[setpointDisplayCount], \"%d\",\n";
        $mupdate .= "%%                  matrixStatusMajCount);\n";
        $mupdate .= "%%        }\n";
        $mupdate .= "%%        else {\n";
        $mupdate .= "%%          strcpy(setpointMajorCount[setpointDisplayCount], \"-\");\n";
        $mupdate .= "%%        }\n";
        $mupdate .= "%%        if (matrixStatusMinCount) {\n";
        $mupdate .= "%%          sprintf(setpointMinorCount[setpointDisplayCount], \"%d\",\n";
        $mupdate .= "%%                  matrixStatusMinCount);\n";
        $mupdate .= "%%        }\n";
        $mupdate .= "%%        else {\n";
        $mupdate .= "%%          strcpy(setpointMinorCount[setpointDisplayCount], \"-\");\n";
        $mupdate .= "%%        }\n";
        $mupdate .= "%%        setpointDisplayCount++;\n";
        $mupdate .= "%%      }\n";
        $mupdate .= "%%      matrixStatusMajCount = 0;\n";
        $mupdate .= "%%      matrixStatusMinCount = 0;\n";
        $mupdate .= "%%    }\n";

        my $top_name = is_top_name($m_name);
   	my $tv_name;
        if ($top_name) {
	  	$tv_name = top_name_transform($m_name);
	}

	for ($i = 1; $i < $x+1; $i++) {
	    for ($j = 1; $j < $y+1; $j++) {
        	if ($top_name) {
			$mdecl .= sprintf("\"{ifo}:${tv_name}%i_%i\"", $i, $j);

                        $mstat_decl .= sprintf("\"{ifo}:${tv_name}%i_%i.STAT\"", $i, $j);
                        $mhihi_decl .= sprintf("\"{ifo}:${tv_name}%i_%i.HHSV\"", $i, $j);
                        $mhigh_decl .= sprintf("\"{ifo}:${tv_name}%i_%i.HSV\"", $i, $j);
                        $mlow_decl  .= sprintf("\"{ifo}:${tv_name}%i_%i.LSV\"", $i, $j);
                        $mlolo_decl .= sprintf("\"{ifo}:${tv_name}%i_%i.LLSV\"", $i, $j);
		} else {
			$mdecl .= sprintf("\"{ifo}:{sys}-{subsys}${m_name}%i_%i\"", $i, $j);

                        $mstat_decl .= sprintf("\"{ifo}:{sys}-{subsys}${m_name}%i_%i.STAT\"", $i, $j);
                        $mhihi_decl .= sprintf("\"{ifo}:{sys}-{subsys}${m_name}%i_%i.HHSV\"", $i, $j);
                        $mhigh_decl .= sprintf("\"{ifo}:{sys}-{subsys}${m_name}%i_%i.HSV\"", $i, $j);
                        $mlow_decl  .= sprintf("\"{ifo}:{sys}-{subsys}${m_name}%i_%i.LSV\"", $i, $j);
                        $mlolo_decl .= sprintf("\"{ifo}:{sys}-{subsys}${m_name}%i_%i.LLSV\"", $i, $j);
		}
		if ($i != ($x) || $j != ($y)) {
		    $mdecl .= ", ";

                    $mstat_decl .= ", ";
                    $mhihi_decl .= ", ";
                    $mhigh_decl .= ", ";
                    $mlow_decl  .= ", ";
                    $mlolo_decl .= ", ";
		}
        	if ($top_name) {
			$matdb .= "grecord(ai,\"%IFO%:${tv_name}" . sprintf("%i_%i\")\n", $i, $j);
		} else {
			$matdb .= "grecord(ai,\"%IFO%:%SYS%-%SUBSYS%${m_name}" . sprintf("%i_%i\")\n", $i, $j);
		}
		$matdb .= "{\n";
		$matdb .= "    field(PREC,\"5\")\n";
		$matdb .= "}\n";
	    }
	    $mdecl .= "\n";

            $mstat_decl .= "\n";
            $mhihi_decl .= "\n";
            $mhigh_decl .= "\n";
            $mlow_decl  .= "\n";
            $mlolo_decl .= "\n";
	}
	$mdecl .= "};\n";

        $mstat_decl .= "};\n";
        $mhihi_decl .= "};\n";
        $mhigh_decl .= "};\n";
        $mlow_decl  .= "};\n";
        $mlolo_decl .= "};\n";

#	print "$mdecl\n";
	$mcnt++;
    } else {
	s/\s//g;
	if ($_ && !(substr($_,0,1) eq "#")) {
	    my $biquad = 0;
	    if ($_ =~ /biquad$/) {
		$biquad = 1;
		s/biquad$//g;
		s/\s//g;
	#	die $_;
	    }
	    $names[$cnt] = $_;
	    if ($cnt) {
		$names1 .= " ,";
		$names2 .= " ,";
	    }
            my $top_name = is_top_name($_);
            if ($top_name) {
                my $tv_name = top_name_transform($_);
	    	$names1 .= '"{ifo}:' . $tv_name . '_%PAR%" ';
            } else {
	    	$names1 .= '"{ifo}:{sys}-{subsys}' . $_ . '_%PAR%" ';
 	    }
#	    $names2 .= '{"'. $_ . '", FLT_' . $_ . ' } ';
	    $names2 .= '{"'. $_ . '", ' . $cnt . ', ' . $biquad . ' } ';
	    if ($cnt % 3 == 2) {
		$names1 .= "\n";
		$names2 .= "\n%%";
	    }
	    $cnt++;
	}
    }
}

$names1 .= "};\n";
$names2 .= "};\n";

printf "$cnt filters\n";
printf "$mcnt matrices\n";

if ($cnt == 0) {
	$skeleton = "../util/skeleton_nofm.st";
}

close IN;
open(IN,"<" . $skeleton) || die "cannot open sequencer Skeleton file $skeleton";
open(OUT,">./$ARGV[0].st") || die "cannot open $ARGV[0].st file for writing";

$cnt2 = $cnt*2;
$cnt10 = $cnt*10;

$fpar{"gain_ramp_time"} = "TRAMP";
$iparAlarmSev{"gain_ramp_time_hihi_Sev"} = "TRAMP.HHSV";
$iparAlarmSev{"gain_ramp_time_high_Sev"} = "TRAMP.HSV";
$iparAlarmSev{"gain_ramp_time_low_Sev"} = "TRAMP.LSV";
$iparAlarmSev{"gain_ramp_time_lolo_Sev"} = "TRAMP.LLSV";
$iparAlarmStat{"gain_ramp_time_Stat"} = "TRAMP.STAT";

@a = ( \%fpar, "double", \%ipar, "int", \%iparAlarmSev, "int", \%iparAlarmStat, "int", \%spar, "string" );

if ($phase > 0) {
   $decl1 .= "%% double rad_angle;\n\n";
}

$decl1 .= $vdecl . "\n" . $vstat_decl . "\n" . $vhihi_decl;
$decl1 .= "\n" . $vhigh_decl . "\n" . $vlow_decl . "\n" . $vlolo_decl;
$decl1 .= "\n" . $mdecl . "\n" . $mstat_decl . "\n" . $mhihi_decl;
$decl1 .= "\n" . $mhigh_decl . "\n" . $mlow_decl . "\n" . $mlolo_decl;
$decl1 .= "\n";

if ($cnt > 0) {
while (($h, $t) = splice(@a, 0, 2)) {
while ( ($n1, $n2) = each %$h ) {
#    print "$t, $n1 => $n2\n";
	$decl1 .= $names1;
	$decl1 =~ s/%TYPE%/$t/g;
	$decl1 =~ s/%NAME%/$n1/g;
	$decl1 =~ s/%PAR%/$n2/g;
	}
}
#}

$ainit .= "\n";
$ainit .= "%%  for (ii = 0; ii < MAX_MODULES; ii++) {\n";
 
while ( ($n1, $n2) = each %iparAlarmSev) {
   ($junk, $severity) = split(/\./, $n2);
 
   if ( (substr($severity, 0, 2) eq "HH") ||
        (substr($severity, 0, 2) eq "LL") ) {
#     $ainit .= "       ${n1}[ii] = MAJOR_ALARM;\n";
      $ainit .= "       ${n1}[ii] = NO_ALARM;\n";
   }
   else {
#     $ainit .= "       ${n1}[ii] = MINOR_ALARM;\n";
      $ainit .= "       ${n1}[ii] = NO_ALARM;\n";
   }
 
   $ainit .= "       pvPut(${n1}[ii]);\n";
}
 
$ainit .= "%%  }\n";
}

$aupdate .= "\n";

while ( ($n1, $n2) = each %iparAlarmStat) {
   ($param, $junk) = split(/\./, $n2);

   $aupdate .= "       pvGet(${n1}[ii]);\n";

   $aupdate .= "%%     if (${n1}[ii]) {\n";
#  $aupdate .= "%%        filterStatusCount++;\n";
   $aupdate .= "%%        thisFilterStatusCount++;\n";

   $aupdate .= "%%        if (filterDisplayCount < 10) {\n";
#  $aupdate .= "%%           strcpy(filterErrId[filterDisplayCount], ";
#  $aupdate .= "fmmap0[ii].name);\n";
#  $aupdate .= "%%           strcpy(filterParam[filterDisplayCount], ";

   if (substr($param, 0, 5) eq "TRAMP") {
#     $aupdate .= "\"Ramp Time\");\n";
      $index = 3;
      $aupdate .= "%%           fltrParamHelp[3] = \'R\';\n";
   }
   elsif (substr($param, 0, 2) eq "SW") {
#     $help = substr($param, 0, 3);
#     $aupdate .= "\"$help\");\n";
      $help = substr($param, 2, 1);
      $index = $help + 3;
      $aupdate .= "%%           fltrParamHelp[$index] = \'$help\';\n";
   }
   else {
#     $aupdate .= "\"$param\");\n";
      $paramStrLength = length($param);
      $index = $paramStrLength - 4;
      $help = substr($param, 0, 1);
      $aupdate .= "%%           fltrParamHelp[$index] = \'$help\';\n";
   }

   $aupdate .= "%%           if ( (${n1}[ii] == HIHI_ALARM) ||\n";
   $aupdate .= "%%                (${n1}[ii] == LOLO_ALARM) ) {\n";
#  $aupdate .= "%%              strcpy(filterSeverity[filterDisplayCount], ";
#  $aupdate .= "\"MAJOR\");\n";
   $aupdate .= "%%              fltrSevHelp[$index] = \'M\';\n";
   $aupdate .= "%%           }\n";
   $aupdate .= "%%           else if ( (${n1}[ii] == HIGH_ALARM) ||\n";
   $aupdate .= "%%                     (${n1}[ii] == LOW_ALARM) ) {\n";
#  $aupdate .= "%%              strcpy(filterSeverity[filterDisplayCount], ";
#  $aupdate .= "\"MINOR\");\n";
   $aupdate .= "%%              fltrSevHelp[$index] = \'m\';\n";
   $aupdate .= "%%           }\n";
   $aupdate .= "%%           else {\n";
#  $aupdate .= "%%              strcpy(filterSeverity[filterDisplayCount], ";
#  $aupdate .= " \"OTHER\");\n";
   $aupdate .= "%%              fltrSevHelp[$index] = \'O\';\n";
   $aupdate .= "%%           }\n";

#  $aupdate .= "%%           filterDisplayCount++;\n";
   $aupdate .= "%%        }\n";
   $aupdate .= "%%     }\n";
}

select(OUT);

$minit .= $vinit;
if ($cnt > 0) {
   $minit .= $ainit;
}
$mupdate .= $vupdate;

while (<IN>) {
    s/%SEQUENCER_NAME%/$ARGV[0]/g;
    s/%FMNUM%/$cnt/g;
    s/%FMNUMx2%/$cnt2/g;
    s/%FMNUMx10%/$cnt10/g;
    s/%DECL1%/$decl1/g;
    s/%DECL2%/$names2/g;
    s/%EPICS_TYPE%/$epics_type/g;
    s/%EPICS_FILT_VAR%/$epics_filt_var/g;
    s/%EPICS_COEFF_VAR%/$epics_coeff_var/g;
    s/%EPICS_EPICS_VAR%/$epics_epics_var/g;
    s/%DECL3%/$minit/g;
#   s/%DECL3_5%/$mread/g;
    s/%AUPDT%/$aupdate/g;
    s/%DECL4%/$mupdate/g;
    print;
}

open(OUT,">./$ARGV[0].db") || die "cannot open $ARGV[0].db file for writing";
select(OUT);

$hepi = substr($ARGV[0],0,4) eq "hepi";

foreach $i ( @names ) {
    close IN;
    open(IN,"<../util/skeleton.db") || die "cannot open skeleton.db file";

    my $top_name = is_top_name($i);
    my $tv_name;
    if ($top_name) {
	 $tv_name = top_name_transform($i);
    }
    while (<IN>) { 
        if ($top_name) {
		s/%FILTER%/$tv_name/g;
	} else {
		s/%FILTER%/%SYS%-%SUBSYS%$i/g;
	}
	if ($hepi) { s/%PREC1%/0/g; }
	else { s/%PREC1%/3/g; }
	print;
    }
    if ($do_epics_input) {
        if ($top_name) {
		print "grecord(ai,\"%IFO%:" . $tv_name . "_INPUT\")\n";
		print "grecord(ai,\"%IFO%:"  . $tv_name . "_EXC\")\n";
	} else {
		print "grecord(ai,\"%IFO%:%SYS%-%SUBSYS%" . $i . "_INPUT\")\n";
		print "grecord(ai,\"%IFO%:%SYS%-%SUBSYS%"  . $i . "_EXC\")\n";
	}
    }
}
    
# add msg and load coeff records
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_BUILD_SVN\")\n";
print "grecord(ai,\"%IFO%:FEC-${dcuId}_LOAD_NEW_COEFF\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_MSG\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_TIME_STRING\")\n";
print "grecord(ai,\"%IFO%:FEC-${dcuId}_STAT_ERR_CNT\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FESTAT_1\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FESTAT_2\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FESTAT_3\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FESTAT_4\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FESTAT_5\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FESTAT_6\")\n";
print "grecord(ai,\"%IFO%:FEC-${dcuId}_MTRX_STAT_ERR_CNT\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_MTRXSTAT_1\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_MTRXSTAT_2\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_MTRXSTAT_3\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_MTRXSTAT_4\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_MTRXSTAT_5\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_MTRXSTAT_6\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_MTRXSTAT_7\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_MTRXSTAT_8\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_MTRXSTAT_9\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_MTRXSTAT_10\")\n";
print "grecord(ai,\"%IFO%:FEC-${dcuId}_FILTER_STATUS_COUNT\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FILTER_ID_1\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FILTER_ID_2\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FILTER_ID_3\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FILTER_ID_4\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FILTER_ID_5\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FILTER_ID_6\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FILTER_ID_7\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FILTER_ID_8\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FILTER_ID_9\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FILTER_ID_10\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FILTER_PARAM_1\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FILTER_PARAM_2\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FILTER_PARAM_3\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FILTER_PARAM_4\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FILTER_PARAM_5\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FILTER_PARAM_6\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FILTER_PARAM_7\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FILTER_PARAM_8\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FILTER_PARAM_9\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FILTER_PARAM_10\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FILTER_SEVERITY_1\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FILTER_SEVERITY_2\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FILTER_SEVERITY_3\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FILTER_SEVERITY_4\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FILTER_SEVERITY_5\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FILTER_SEVERITY_6\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FILTER_SEVERITY_7\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FILTER_SEVERITY_8\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FILTER_SEVERITY_9\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_FILTER_SEVERITY_10\")\n";
print "grecord(ai,\"%IFO%:FEC-${dcuId}_SETPOINT_STATUS_COUNT\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_SETPOINT_ID_1\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_SETPOINT_ID_2\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_SETPOINT_ID_3\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_SETPOINT_ID_4\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_SETPOINT_ID_5\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_SETPOINT_ID_6\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_SETPOINT_ID_7\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_SETPOINT_ID_8\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_SETPOINT_ID_9\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_SETPOINT_ID_10\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_SETPOINT_MAJOR_COUNT_1\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_SETPOINT_MAJOR_COUNT_2\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_SETPOINT_MAJOR_COUNT_3\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_SETPOINT_MAJOR_COUNT_4\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_SETPOINT_MAJOR_COUNT_5\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_SETPOINT_MAJOR_COUNT_6\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_SETPOINT_MAJOR_COUNT_7\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_SETPOINT_MAJOR_COUNT_8\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_SETPOINT_MAJOR_COUNT_9\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_SETPOINT_MAJOR_COUNT_10\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_SETPOINT_MINOR_COUNT_1\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_SETPOINT_MINOR_COUNT_2\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_SETPOINT_MINOR_COUNT_3\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_SETPOINT_MINOR_COUNT_4\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_SETPOINT_MINOR_COUNT_5\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_SETPOINT_MINOR_COUNT_6\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_SETPOINT_MINOR_COUNT_7\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_SETPOINT_MINOR_COUNT_8\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_SETPOINT_MINOR_COUNT_9\")\n";
print "grecord(stringin,\"%IFO%:FEC-${dcuId}_SETPOINT_MINOR_COUNT_10\")\n";

#add matrix records
print $matdb;

#add variable records
print $vardb;

close (OUT);

if ($gds_specified) {
    open(OUT,">$ARGV[0].par") || die "cannot open $ARGV[0].par file for writing";
    select(OUT);
    print "#$ARGV[0] excitations\n";
  foreach $systm (@systems) {
    $excnum = $gds_excnum_base;
    if ($systm eq "SKIP") {
	;
    } else {
      foreach $i ( @names ) {
    	my $top_name = is_top_name($i);
    	my $tv_name;
    	if ($top_name) {
	 $tv_name = top_name_transform($i);
	 print "[$site:${tv_name}_EXC]\n";
	} else {
	 print "[$site:${systm}${i}_EXC]\n";
	}
	print "ifoid = $gds_ifo\n";
	print "rmid = $gds_rmid\n";
	print "dcuid = $gds_exc_dcu_id\n";
	print "chnnum = $excnum\n";
	print "datatype = 4\n";	
	print "datarate = $gds_datarate\n\n";
	$excnum++;
      }
      # Print extra excitations
      foreach $i ( @excitations ) {
    	my $top_name = is_top_name($i);
    	my $tv_name;
    	if ($top_name) {
	 $tv_name = top_name_transform($i);
	 print "[$site:${tv_name}]\n";
	} else {
	 print "[$site:${systm}${i}]\n";
	}
	print "ifoid = $gds_ifo\n";
	print "rmid = $gds_rmid\n";
	print "dcuid = $gds_tp_dcu_id\n";
	print "chnnum = $excnum\n";
	print "datatype = 4\n";	
	print "datarate = $gds_datarate\n\n";
	$excnum++;
      }
      $gds_excnum_base += $gds_exc_sys_inc;
    }
  }
  foreach $systm (@systems) {
    $tpnum = $gds_tpnum_base;
    if ($systm eq "SKIP") {
	;
    } else {
      foreach $i ( @names ) {
    	my $top_name = is_top_name($i);
    	my $tv_name;
    	if ($top_name) {
	 $tv_name = top_name_transform($i);
	 print "[$site:${tv_name}_IN1]\n";
	} else {
	 print "[$site:${systm}${i}_IN1]\n";
	}
	print "ifoid = $gds_ifo\n";
	print "rmid = $gds_rmid\n";
	print "dcuid = $gds_tp_dcu_id\n";
	print "chnnum = $tpnum\n";
	print "datatype = 4\n";	
	print "datarate = $gds_datarate\n\n";
	$tpnum++;
    	if ($top_name) {
	 print "[$site:${tv_name}_IN2]\n";
	} else {
	 print "[$site:${systm}${i}_IN2]\n";
	}
	print "ifoid = $gds_ifo\n";
	print "rmid = $gds_rmid\n";
	print "dcuid = $gds_tp_dcu_id\n";
	print "chnnum = $tpnum\n";
	print "datatype = 4\n";	
	print "datarate = $gds_datarate\n\n";
	$tpnum++;
    	if ($top_name) {
	 print "[$site:${tv_name}_OUT]\n";
	} else {
	 print "[$site:${systm}${i}_OUT]\n";
 	}
	print "ifoid = $gds_ifo\n";
	print "rmid = $gds_rmid\n";
	print "dcuid = $gds_tp_dcu_id\n";
	print "chnnum = $tpnum\n";
	print "datatype = 4\n";	
	print "datarate = $gds_datarate\n\n";
	$tpnum++;
      }
      # Print extra test points
      foreach $i ( @testpoints ) {
    	my $top_name = is_top_name($i);
    	my $tv_name;
    	if ($top_name) {
	 $tv_name = top_name_transform($i);
	 print "[$site:${tv_name}]\n";
	} else {
	 print "[$site:${systm}${i}]\n";
	}
	print "ifoid = $gds_ifo\n";
	print "rmid = $gds_rmid\n";
	print "dcuid = $gds_tp_dcu_id\n";
	print "chnnum = $tpnum\n";
	print "datatype = 4\n";	
	print "datarate = $gds_datarate\n\n";
	$tpnum++;
      }
    }
    $gds_tpnum_base += $gds_tp_sys_inc;
  }
    close (OUT);

# Create DAQ config file (default section and a few ADC input channels)
my $daqFile = "$ARGV[0].ini";
open(OUTG,">".$daqFile) || die "cannot open $daqFile file for writing";
print OUTG      "[default]\n".
                "gain=1.00\n".
                "acquire=0\n".
                "dcuid=$dcuId\n".
                "ifoid=$ifoid\n".
                "datatype=4\n".
                "datarate=" . $gds_datarate . "\n".
                "offset=0\n".
                "slope=6.1028e-05\n".
                "units=V\n".
                "\n";


# Open testpoints file
my $parFile = "$ARGV[0].par";
open(INTP,"<".$parFile) || die "cannot open $parFile file for reading";
# Read all lines into the array
@tp_data=<INTP>;
close INTP;

my %sections;
my @section_names;
my $section_name;
my $def_datarate;
foreach (@tp_data) {
 s/\s+//g;
 if (@a = m/\[(.+)\]/) { $section_name = $a[0]; push @section_names, $a[0]; }
 elsif (@a = m/(.+)=(.+)/) {
        $sections{$section_name}{$a[0]} = $a[1];
        if ($a[0] eq "datarate") { $def_datarate = $a[1]; }
 }
}
my $cnt = 0;
my $daq_name = "DAQ";
# Print chnnum, datarate, 
foreach (sort @section_names) {
        if ($cnt < 2 && m/_OUT$/) {
                $comment = "";
                $cnt++;
        } else {
                $comment = "#";
        }
#        print OUTG "${comment}[${_}_${def_datarate}]\n";
        print OUTG "${comment}[${_}_${daq_name}]\n";
        print OUTG  "${comment}acquire=0\n";
        foreach $sec (keys %{$sections{$_}}) {
          if ($sec eq "chnnum" || $sec eq "datarate" || $sec eq "datatype") {
                print OUTG  "${comment}$sec=${$sections{$_}}{$sec}\n";
          }
        }
}
close OUTG;


}


exit(0);
