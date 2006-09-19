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

$names1 = "%TYPE% %NAME%[MAX_MODULES];\nassign %NAME% to\n{\n";
$names2 = "%%static  fmSubSysMap  fmmap0 [MAX_MODULES] = { \n%%";

$do_epics_input = 0;

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
    } elsif (substr($_,0,10) eq "gds_config") {
	($junk, $gds_excnum_base, $gds_tpnum_base, $gds_exc_sys_inc, $gds_tp_sys_inc ) = split(/\s+/, $_);
	$gds_specified = 1;
	$gds_datarate = $gds_excnum_base < 10000? 16384: 2048;
	$gds_ifo = 1;
	$gds_rmid = 0;
	$gds_exc_dcu_id = 13 + (int ($gds_excnum_base >= 10000)) * 2;
	$gds_tp_dcu_id = 13 + int ($gds_tpnum_base / 10000);
    } elsif (substr($_,0,5) eq "EPICS") {
	($junk, $epics_type, $epics_filt_var, $epics_coeff_var, $epics_epics_var ) = split(/\s+/, $_);	
#	print "$junk, $epics_addr, $epics_type\n";
	$epics_specified = 1;
    } elsif (substr($_,0,10) eq "INVARIABLE") {
	die "Unspecified EPICS parameters" unless $epics_specified;
	($junk, $v_name, $v_var, $v_type, $ve_type, $v_init, $v_efield1, $v_efield2, $v_efield3, $v_efield4 ) = split(/\s+/, $_);
	$vdecl .= "$v_type evar_$v_name;\n";
	$vdecl .= "assign evar_$v_name to \"{ifo}:{sys}-{subsys}${v_name}\";\n";

	$vinit .= "%% evar_$v_name  = $v_init;\n";
	$vinit .= "pvPut(evar_$v_name);\n";
	$vinit .= "%%       pEpics->${v_var} = evar_$v_name;\n";

	$vupdate .= "pvGet(evar_$v_name);\n";
	$vupdate .= "rfm_assign(pEpics->${v_var}, evar_$v_name);\n";

	$vardb .= "grecord(${ve_type},\"%IFO%:%SYS%-%SUBSYS%${v_name}\")\n";
	$vardb .= "{\n";
#	$vardb .= "    field(PREC,\"3\")\n";
	$vardb .= "    $v_efield1\n";
	$vardb .= "    $v_efield2\n";
	$vardb .= "    $v_efield3\n";
	$vardb .= "    $v_efield4\n";
	$vardb .= "}\n";
    } elsif (substr($_,0,9) eq "WFS_PHASE") {
	die "Unspecified EPICS parameters" unless $epics_specified;
	($junk, $v_name, $v_var, $v_type, $ve_type, $v_init, $v_efield1, $v_efield2, $v_efield3, $v_efield4 ) = split(/\s+/, $_);
	$vdecl .= "$v_type evar_${v_name}_d;\n";
	$vdecl .= "$v_type evar_${v_name}_r;\n";
	$vdecl .= "assign evar_$v_name_d to \"{ifo}:{sys}-{subsys}${v_name}_D\";\n";
	$vdecl .= "assign evar_$v_name_d to \"{ifo}:{sys}-{subsys}${v_name}_R\";\n";

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
	$vupdate .= "rfm_assign(pEpics->${v_var}[0][0], sin(evar_${v_name}_r + evar_${v_name}_d)/sin(evar_${v_name}_d));\n";
	$vupdate .= "rfm_assign(pEpics->${v_var}[0][1], cos(evar_${v_name}_r + evar_${v_name}_d)/sin(evar_${v_name}_d));\n";
	$vupdate .= "rfm_assign(pEpics->${v_var}[1][0], sin(evar_${v_name}_r)/sin(evar_${v_name}_d));\n";
	$vupdate .= "rfm_assign(pEpics->${v_var}[1][1], cos(evar_${v_name}_r)/sin(evar_${v_name}_d));\n";

	$vardb .= "grecord(${ve_type},\"%IFO%:%SYS%-%SUBSYS%${v_name}_D\")\n";
	$vardb .= "{\n";
#	$vardb .= "    field(PREC,\"3\")\n";
	$vardb .= "    $v_efield1\n";
	$vardb .= "    $v_efield2\n";
	$vardb .= "    $v_efield3\n";
	$vardb .= "    $v_efield4\n";
	$vardb .= "}\n";
	$vardb .= "grecord(${ve_type},\"%IFO%:%SYS%-%SUBSYS%${v_name}_R\")\n";
	$vardb .= "{\n";
#	$vardb .= "    field(PREC,\"3\")\n";
	$vardb .= "    $v_efield1\n";
	$vardb .= "    $v_efield2\n";
	$vardb .= "    $v_efield3\n";
	$vardb .= "    $v_efield4\n";
	$vardb .= "}\n";
    } elsif (substr($_,0,5) eq "PHASE") {
	die "Unspecified EPICS parameters" unless $epics_specified;
	($junk, $v_name, $v_var, $v_type, $ve_type, $v_init, $v_efield1, $v_efield2, $v_efield3, $v_efield4 ) = split(/\s+/, $_);
	$vdecl .= "$v_type evar_$v_name;\n";
	$vdecl .= "assign evar_$v_name to \"{ifo}:{sys}-{subsys}${v_name}\";\n";

	$vinit .= "%% evar_$v_name  = $v_init;\n";
	$vinit .= "pvPut(evar_$v_name);\n";
	$vinit .= "%%       pEpics->${v_var}[0] = sin(evar_$v_name);\n";
	$vinit .= "%%       pEpics->${v_var}[1] = cos(evar_$v_name);\n";

	$vupdate .= "pvGet(evar_$v_name);\n";
	$vupdate .= "rfm_assign(pEpics->${v_var}[0], sin(evar_$v_name));\n";
	$vupdate .= "rfm_assign(pEpics->${v_var}[1], cos(evar_$v_name));\n";

	$vardb .= "grecord(${ve_type},\"%IFO%:%SYS%-%SUBSYS%${v_name}\")\n";
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
	$vdecl .= "assign evar_$v_name to \"{ifo}:{sys}-{subsys}${v_name}\";\n";

	$vinit .= "%% evar_$v_name  = $v_init;\n";
	$vinit .= "pvPut(evar_$v_name);\n";
	$vinit .= "%%       pEpics->${v_var} = evar_$v_name;\n";

	$vupdate .= "pvGet(evar_$v_name);\n";
	$vupdate .= "%% if(evar_$v_name != 0) {\n";
	$vupdate .= "rfm_assign(pEpics->${v_var}, evar_$v_name);\n";
	$vupdate .= "%% }\n";
	$vupdate .= "%% evar_$v_name  = $v_init;\n";
	$vupdate .= "pvPut(evar_$v_name);\n";

	$vardb .= "grecord(${ve_type},\"%IFO%:%SYS%-%SUBSYS%${v_name}\")\n";
	$vardb .= "{\n";
#	$vardb .= "    field(PREC,\"3\")\n";
	$vardb .= "    $v_efield1\n";
	$vardb .= "    $v_efield2\n";
	$vardb .= "    $v_efield3\n";
	$vardb .= "    $v_efield4\n";
	$vardb .= "}\n";
    } elsif (substr($_,0,11) eq "OUTVARIABLE") {
	die "Unspecified EPICS parameters" unless $epics_specified;
	($junk, $v_name, $v_var, $v_type, $ve_type, $v_init, $v_efield1, $v_efield2, $v_efield3, $v_efield4 ) = split(/\s+/, $_);
	$vdecl .= "$v_type evar_$v_name;\n";
	$vdecl .= "assign evar_$v_name to \"{ifo}:{sys}-{subsys}${v_name}\";\n";

	$vinit .= "%% evar_$v_name  = $v_init;\n";
	$vinit .= "pvPut(evar_$v_name);\n";
	$vinit .= "%%       pEpics->${v_var} = evar_$v_name;\n";

	if ($v_type eq "float") {
	    $vupdate .= "evar_$v_name = fpvalidate(pEpics->${v_var});\n";
	} else {
	    $vupdate .= "evar_$v_name = pEpics->${v_var};\n";
	}
	$vupdate .= "pvPut(evar_$v_name);\n";

	$vardb .= "grecord(${ve_type},\"%IFO%:%SYS%-%SUBSYS%${v_name}\")\n";
	$vardb .= "{\n";
	if ($v_type eq "float") {
		$vardb .= "    field(PREC,\"3\")\n";
	}
	$vardb .= "    $v_efield1\n";
	$vardb .= "    $v_efield2\n";
	$vardb .= "    $v_efield3\n";
	$vardb .= "    $v_efield4\n";
	$vardb .= "}\n";
    } elsif (substr($_,0,6) eq "DAQVAR") {
	die "Unspecified EPICS parameters" unless $epics_specified;
	($junk, $v_name, $v_type, $ve_type, $v_init, $v_efield1, $v_efield2, $v_efield3, $v_efield4 ) = split(/\s+/, $_);

	$vardb .= "grecord(${ve_type},\"%IFO%:DAQ-%SYS%_%SUBSYS%${v_name}\")\n";
    } elsif (substr($_,0,5) eq "DUMMY") {
	die "Unspecified EPICS parameters" unless $epics_specified;
	($junk, $v_name, $v_type, $ve_type, $v_init, $v_efield1, $v_efield2, $v_efield3, $v_efield4 ) = split(/\s+/, $_);
	$vardb .= "grecord(${ve_type},\"%IFO%:%SYS%-%SUBSYS%${v_name}\")\n";

    } elsif (substr($_,0,6) eq "MATRIX") {
	die "Unspecified EPICS parameters" unless $epics_specified;
	($junk, $m_name, $m_dims, $m_var ) = split(/\s+/, $_);
	$m_dims =~ s/\s//g;
	($x, $y) = split (/x/, $m_dims);
#	print "$x $y\n";
	$xXy = $x * $y;
	$mdecl .= "float matrix${m_name}[$xXy];\n";
	$mdecl .= "assign matrix${m_name} to { \n";
	$minit .= "%%  for (ii = 0; ii < ${x}; ii++)\n";
	$minit .= "%%    for (jj = 0; jj < ${y}; jj++) {\n";
	$minit .= "         matrix${m_name}[ii * ${y} + jj] = 0.;\n";
	$minit .= "         pvPut(matrix${m_name}[ii * ${y} + jj]);\n";
	$minit .= "%%       pEpics->${m_var}[ii][jj] = 0.;\n";
	$minit .= "%%    }\n";

	$mupdate .= "%%  for (ii = 0; ii < ${x}; ii++)\n";
	$mupdate .= "%%    for (jj = 0; jj < ${y}; jj++) {\n";
	$mupdate .= "       pvGet(matrix${m_name}[ii * ${y} + jj]);\n";
	$mupdate .= "%%     rfm_assign(pEpics->${m_var}[ii][jj], matrix${m_name}[ii * ${y} + jj]);\n";
	$mupdate .= "%%    }\n";

	for ($i = 1; $i < $x+1; $i++) {
	    for ($j = 1; $j < $y+1; $j++) {
		$mdecl .= sprintf("\"{ifo}:{sys}-{subsys}${m_name}%x%x\"", $i, $j);
		if ($i != ($x) || $j != ($y)) {
		    $mdecl .= ", ";
		}
		$matdb .= "grecord(ai,\"%IFO%:%SYS%-%SUBSYS%${m_name}" . sprintf("%x%x\")\n", $i, $j);
		$matdb .= "{\n";
		$matdb .= "    field(PREC,\"3\")\n";
		$matdb .= "}\n";
	    }
	    $mdecl .= "\n";
	}
	$mdecl .= "};\n";

#	print "$mdecl\n";
	$mcnt++;
    } else {
	s/\s//g;
	if ($_ && !(substr($_,0,1) eq "#")) {
	    $names[$cnt] = $_;
	    if ($cnt) {
		$names1 .= " ,";
		$names2 .= " ,";
	    }
	    $names1 .= '"{ifo}:{sys}-{subsys}' . $_ . '_%PAR%" ';
#	    $names2 .= '{"'. $_ . '", FLT_' . $_ . ' } ';
	    $names2 .= '{"'. $_ . '", ' . $cnt . ' } ';
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

@a = ( \%fpar, "float", \%ipar, "int", \%spar, "string" );

$decl1 .= $vdecl . "\n" . $mdecl;
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
}
select(OUT);

$minit .= $vinit;
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
    s/%DECL4%/$mupdate/g;
    print;
}

open(OUT,">./$ARGV[0].db") || die "cannot open $ARGV[0].db file for writing";
select(OUT);

$hepi = substr($ARGV[0],0,4) eq "hepi";

foreach $i ( @names ) {
    close IN;
    open(IN,"<../util/skeleton.db") || die "cannot open skeleton.db file";

    while (<IN>) { 
	s/%FILTER%/$i/g;
	if ($hepi) { s/%PREC1%/0/g; }
	else { s/%PREC1%/3/g; }
	print;
    }
    if ($do_epics_input) {
	print "grecord(ai,\"%IFO%:%SYS%-%SUBSYS%" . $i . "_INPUT\")\n";
	print "grecord(ai,\"%IFO%:%SYS%-%SUBSYS%"  . $i . "_EXC\")\n";
    }
}
    
# add msg and load coeff records
print "grecord(ai,\"%IFO%:%SYS%-%SUBSYS%LOAD_NEW_COEFF\")\n";
print "grecord(stringin,\"%IFO%:%SYS%-%SUBSYS%MSG\")\n";

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
	print "[%IFO%:${systm}${i}_EXC]\n";
	print "ifoid = $gds_ifo\n";
	print "rmid = $gds_rmid\n";
	print "dcuid = $gds_exc_dcu_id\n";
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
	print "[%IFO%:${systm}${i}_IN1]\n";
	print "ifoid = $gds_ifo\n";
	print "rmid = $gds_rmid\n";
	print "dcuid = $gds_tp_dcu_id\n";
	print "chnnum = $tpnum\n";
	print "datatype = 4\n";	
	print "datarate = $gds_datarate\n\n";
	$tpnum++;
	print "[%IFO%:${systm}${i}_IN2]\n";
	print "ifoid = $gds_ifo\n";
	print "rmid = $gds_rmid\n";
	print "dcuid = $gds_tp_dcu_id\n";
	print "chnnum = $tpnum\n";
	print "datatype = 4\n";	
	print "datarate = $gds_datarate\n\n";
	$tpnum++;
	print "[%IFO%:${systm}${i}_OUT]\n";
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
}
exit(0);
