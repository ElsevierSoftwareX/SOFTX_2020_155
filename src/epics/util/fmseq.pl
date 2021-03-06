#!/usr/bin/perl
use Cwd;

#//     \page fmseq fmseq.pl
#//     \brief Documentation for fmseq.pl
#//
#// \n

$currWorkDir = &Cwd::cwd();
$rcg_src_dir = $ENV{"RCG_SRC_DIR"};
if (! length $rcg_src_dir) { $rcg_src_dir = "$currWorkDir/../../.."; }

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
    "swStatus" => "SWSTAT",
    "swReq" => "SWREQ",
    "swMask" => "SWMASK",
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
$skeleton = "$rcg_src_dir/src/epics/util/skeleton.st";
if (@ARGV == 2) { $skeleton = $ARGV[1]; }
open(IN,"<-") || die "cannot open standard input\n";

$cnt = 0;
$mcnt = 0;
$phase = 0;
$edcuSizeI = 0;
$edcuSizeD = 0;
$edcuTpNum = 40000;
$edcuFiltNum = 50000;
$casdf = 0;
my @eFields;

$names1 = "%TYPE% %NAME%[MAX_MODULES];\nassign %NAME% to\n{\n";
$names2 = "%%static  fmSubSysMap  fmmap0 [MAX_MODULES] = { \n%%";

$do_epics_input = 0;

#// \b sub \b is_top_name \n
#// Determine whether passed name needs to become a top name
#// i.e. whether the system/subsystem parts need to excluded  \n
#// Returns 1 if top name, else returns 0 \n\n
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

#// \b su \b top_name_transform \n
#// Transform record name for exculsion of sys/subsystem parts
#// This function replaces first underscore with the hyphen \n\n
sub top_name_transform {
   ($name) =  @_;
   $name =~ s/_/-/;
   return $name;
};

#// add_vproc_entry
#// Add new record to the /proc table
sub add_vproc_entry {
	($proc_name, $v_type, $out, $v_var, $nrow, $ncol) =  @_;
	$nrow = 0 if ! defined $nrow;
	$ncol = 0 if ! defined $ncol;
	$vproc .= "{\"$proc_name\", (void *)&(pLocalEpics->$v_var) - (void *)pLocalEpics, ";
	if ($out) {
		$vproc .= "0, ";
	} else {
		$vproc .= "(void *)&(pLocalEpics->${v_var}_mask) - (void *)pLocalEpics, ";
	}
	$vproc .= ($v_type eq "int"? "0": "1") .  " /* $v_type */ , $out, $nrow, $ncol}, \n";
	$vproc_size++;
}
sub add_edcu_entry {
	(my $proc_name, my $v_type, my$out, my$v_var, my $egu, $nrow, $ncol) =  @_;
	#if(($v_type ne "int") || ($v_type ne "double")) {return;}
	$egu = "undef" if ! defined $egu;
	$nrow = 0 if ! defined $nrow;
	$ncol = 0 if ! defined $ncol;
	if((index($proc_name, "BURT") == -1) && (index($proc_name, "DACDT_ENABLE") == -1)
	   && (index($proc_name, "VME_RESET") == -1) && (index($proc_name, "IPC_DIAG_RESET") == -1)
	   && (index($proc_name, "DIAG_RESET") == -1) && (index($proc_name, "OVERFLOW_RESET") == -1))
	{
	# This section handles arrays
	if($nrow || $ncol) {
		for(my $ii=1;$ii<($nrow+1);$ii++)
		{
			for(my $jj=1;$jj<($ncol+1);$jj++)
			{
				if($v_type eq "int") {
					$edcuEntryI .= "\[$proc_name\_$ii\_$jj\]\n";
					$edcuEntryI .= "acquire=3\n";
					$edcuEntryI .= "datarate=16\n";
					$edcuEntryI .= "datatype=2\n";
					$edcuEntryI .= "chnnum=$edcuTpNum\n";
					$edcuSizeI ++;
					$edcuTpNum ++;
				} else {
					$edcuEntryD .= "\[$proc_name\_$ii\_$jj\]\n";
					$edcuEntryD .= "acquire=3\n";
					$edcuEntryD .= "datarate=16\n";
					$edcuEntryD .= "datatype=4\n";
					$edcuEntryD .= "chnnum=$edcuTpNum\n";
					$edcuSizeD ++;
					$edcuTpNum ++;
				}
			}
		}
	} elsif ($v_type eq "double"){
		$edcuEntryD .= "\[$proc_name\]\n";
		$edcuEntryD .= "acquire=3\n";
		$edcuEntryD .= "datarate=16\n";
		$edcuEntryD .= "datatype=4\n";
		$edcuEntryD .= "chnnum=$edcuTpNum\n";
		$edcuEntryD .= "units=$egu\n";
		$edcuSizeD ++;
		$edcuTpNum ++;
	} elsif (($v_type eq "int") && ($ve_type ne "longout")){
		$edcuEntryI .= "\[$proc_name\]\n";
		$edcuEntryI .= "acquire=3\n";
		$edcuEntryI .= "datarate=16\n";
		$edcuEntryI .= "datatype=2\n";
		$edcuEntryI .= "chnnum=$edcuTpNum\n";
		$edcuEntryI .= "units=$egu\n";
		$edcuSizeI ++;
		$edcuTpNum ++;
	} else {	# This must be unsigned int type
		$edcuEntryI .= "\[$proc_name\]\n";
		$edcuEntryI .= "acquire=3\n";
		$edcuEntryI .= "datarate=16\n";
		$edcuEntryI .= "datatype=7\n";
		$edcuEntryI .= "chnnum=$edcuTpNum\n";
		$edcuEntryI .= "units=$egu\n";
		$edcuSizeI ++;
		$edcuTpNum ++;

	}
	}
}

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
	$ifo = "";
	($junk, $gds_excnum_base, $gds_tpnum_base, $gds_exc_sys_inc, $gds_tp_sys_inc, $gds_rmid, $ifo, $gds_datarate, $dcuId, $ifoid) = split(/\s+/, $_);
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
	if ($ifo eq undef || $ifo eq "") {
	  $ifo = "M1";
	}
	$gds_exc_dcu_id = 13 + (int ($gds_excnum_base >= 10000)) * 2;
	$gds_tp_dcu_id = 13 + int ($gds_tpnum_base / 10000);
    } elsif (substr($_,0,13) eq "sdf_flavor_ca") {
	$::casdf = 1;
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
	my $proc_name;
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
  	  $proc_name = "%SITE%:$tv_name";
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
	  $proc_name = "%SITE%:%SYS%$v_name";
	}

	add_vproc_entry($proc_name, $v_type, 0, $v_var);
	add_edcu_entry($proc_name, $v_type, 0, $v_var);
	#$vproc .= "{\"$proc_name\", "
		#. ($v_type eq "int"? "0": "1") .
		#" /* $v_type */ , 0 /* in */, (void *)&(pLocalEpics->$v_var) - (void *)pLocalEpics, ".
		#"(void *)&(pLocalEpics->${v_var}_mask) - (void *)pLocalEpics},\n";
	#$vproc_size++;

	$vinit .= "%% evar_$v_name  = $v_init;\n";
	$vinit .= "pvPut(evar_$v_name);\n";
	$vinit .= "%%       pEpics->${v_var} = evar_$v_name;\n";
	$vinit .= "%%       pEpics->${v_var}_mask = 0;\n";

        if ($ve_type ne "bi") {
	$vupdate .= "%% if (pEpics->${v_var}_mask) {\n";
	$vupdate .= "%%  evar_$v_name = pEpics->${v_var};\n";
	$vupdate .= "pvPut(evar_$v_name);\n";
	$vupdate .= "%% } else {\n";
	$vupdate .= "pvGet(evar_$v_name);\n";
	$vupdate .= "%%  rfm_assign(pEpics->${v_var}, evar_$v_name);\n";
	$vupdate .= "%% }\n";
 	} else {
	$vupdate .= "%% if (pEpics->${v_var}_mask) {\n";
	$vupdate .= "%%  evar_$v_name = pEpics->${v_var} == 1? 1:0;\n";
	$vupdate .= "pvPut(evar_$v_name);\n";
	$vupdate .= "%% } else {\n";
	$vupdate .= "pvGet(evar_$v_name);\n";
	$vupdate .= "%%  rfm_assign(pEpics->${v_var}, evar_$v_name == 1? 1:0);\n";
	$vupdate .= "%%  if(evar_$v_name > 1) { \n";
	$vupdate .= "%%		evar_$v_name = 0; \n";
	$vupdate .= "		pvPut(evar_$v_name);\n";
	$vupdate .= "%%   }\n";
	$vupdate .= "%% }\n";
	}

        if ($top_name) {
		$vardb .= "grecord(${ve_type},\"%IFO%:${tv_name}\")\n";
	} else {
		$vardb .= "grecord(${ve_type},\"%IFO%:%SYS%-%SUBSYS%${v_name}\")\n";
	}
	$vardb .= "{\n";
	foreach (@eFields) {
		$vardb .= "    $_\n";
	}
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
	$vinit .= "%%       pEpics->${v_var}_d = 0.0;\n";
	$vinit .= "%%       pEpics->${v_var}_r = 0.0;\n";
	$vinit .= "%%       pEpics->${v_var}[0][0] = 0.0;\n";
	$vinit .= "%%       pEpics->${v_var}[0][1] = 0.0;\n";
	$vinit .= "%%       pEpics->${v_var}[1][0] = 0.0;\n";
	$vinit .= "%%       pEpics->${v_var}[1][1] = 0.0;\n";

	$vupdate .= "pvGet(evar_${v_name}_d);\n";
	$vupdate .= "pvGet(evar_${v_name}_r);\n";
	$vupdate .= "pEpics->${v_var}_d = evar_${v_name}_d;\n";
	$vupdate .= "pEpics->${v_var}_r = evar_${v_name}_r;\n";
	$vupdate .= "evar_${v_name}_r *= M_PI/180.0;\n";
	$vupdate .= "evar_${v_name}_d *= M_PI/180.0;\n";
	$vupdate .= "if (evar_${v_name}_d == 0.0) evar_${v_name}_d = .1*(M_PI / 180);\n";
	$vupdate .= "pEpics->${v_var}[0][0] = sin(evar_${v_name}_r + evar_${v_name}_d)/sin(evar_${v_name}_d);\n";
	$vupdate .= "pEpics->${v_var}[0][1] = cos(evar_${v_name}_r + evar_${v_name}_d)/sin(evar_${v_name}_d);\n";
	$vupdate .= "pEpics->${v_var}[1][0] = sin(evar_${v_name}_r)/sin(evar_${v_name}_d);\n";
	$vupdate .= "pEpics->${v_var}[1][1] = cos(evar_${v_name}_r)/sin(evar_${v_name}_d);\n";

	if ($top_name) {
		$vardb .= "grecord(${ve_type},\"%IFO%:${tv_name}_D\")\n";
		$proc_name = "%SITE%:$tv_name";
		$proc_name1 = "%SITE%:$tv_name\_D";
		$proc_name2 = "%SITE%:$tv_name\_R";
	} else {
		$vardb .= "grecord(${ve_type},\"%IFO%:%SYS%-%SUBSYS%${v_name}_D\")\n";
		$proc_name = "%SITE%:%SYS%$v_name";
		$proc_name1 = "%SITE%:%SYS%$v_name\_D";
		$proc_name2 = "%SITE%:%SYS%$v_name\_R";
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
	$egu = "undef";
	add_edcu_entry($proc_name1, "double", 1, $v_var);
	add_edcu_entry($proc_name2, "double", 1, $v_var);
	add_edcu_entry($proc_name, "double", 0, $m_var, $egu, 2, 2);
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
		$proc_name1 = "%SITE%:$tv_name\_SIN";
		$proc_name2 = "%SITE%:$tv_name\_COS";
		$proc_name3 = "%SITE%:$tv_name";
	} else {
		$vdecl .= "assign evar_$v_name to \"{ifo}:{sys}-{subsys}${v_name}\";\n";
		$proc_name1 = "%SITE%:%SYS%$v_name\_SIN";
		$proc_name2 = "%SITE%:%SYS%$v_name\_COS";
		$proc_name3 = "%SITE%:%SYS%$v_name";
	}
	add_edcu_entry($proc_name3, "double", 1, $v_var);
	add_edcu_entry($proc_name1, "double", 1, $v_var);
	add_edcu_entry($proc_name2, "double", 1, $v_var);

	$vinit .= "%% evar_$v_name  = $v_init;\n";
	$vinit .= "pvPut(evar_$v_name);\n";
        $vinit .= "%%       rad_angle = (double)((evar_$v_name * M_PI)/180.);\n";
	$vinit .= "%%       pEpics->${v_var}[0] = sin(rad_angle);\n";
	$vinit .= "%%       pEpics->${v_var}[1] = cos(rad_angle);\n";

	$vupdate .= "pvGet(evar_$v_name);\n";
	$vupdate .= "pEpics->${v_var}_E = evar_${v_name};\n";
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
	#($junk, $v_name, $v_var, $v_type, $ve_type, $v_init, $v_efield1, $v_efield2, $v_efield3, $v_efield4 ) = split(/\s+/, $_);
	($junk, $v_name, $v_var, $v_type, $ve_type, $v_init, @eFields ) = split(/\s+/, $_);
	$vdecl .= "$v_type evar_$v_name;\n";
        my $top_name = is_top_name($v_name);
   	my $tv_name;
        if ($top_name) {
	  	$tv_name = top_name_transform($v_name);
		$vdecl .= "assign evar_$v_name to \"{ifo}:${tv_name}\";\n";
		$proc_name = "%SITE%:$tv_name";
	} else {
		$vdecl .= "assign evar_$v_name to \"{ifo}:{sys}-{subsys}${v_name}\";\n";
		$proc_name = "%SITE%:%SYS%$v_name";
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
	#Add field defs
	foreach (@eFields) {
		$vardb .= "    $_\n";
	}
	$vardb .= "}\n";
	add_edcu_entry($proc_name, $v_type, 1, $v_var);
    } elsif (substr($_,0,11) eq "OUTVARIABLE") {
	die "Unspecified EPICS parameters" unless $epics_specified;
	($junk, $v_name, $v_var, $v_type, $ve_type, $v_init,@eFields ) = split(/\s+/, $_);
	foreach (@eFields) {
		if(index($_, "EGU") != -1) {
			my @egustring = split(/"/, $_);
			$egu = $egustring[1];
			# print "$v_name = EGU is $egu \n";
		}
	}
	# longout denotes a UINT32 data type is to be used.
	if ($ve_type eq "longout") {
	$vdecl .= "unsigned int evar_$v_name;\n";
	} else {
	$vdecl .= "$v_type evar_$v_name;\n";
	}
        my $top_name = is_top_name($v_name);
   	my $tv_name;
	my $proc_name;
        if ($top_name) {
	  	$tv_name = top_name_transform($v_name);
		$vdecl .= "assign evar_$v_name to \"{ifo}:${tv_name}\";\n";
		$proc_name = "%SITE%:$tv_name";
	} else {
		$vdecl .= "assign evar_$v_name to \"{ifo}:{sys}-{subsys}${v_name}\";\n";
		$proc_name = "%SITE%:%SYS%$v_name";
	}

	add_vproc_entry($proc_name, $v_type, 1, $v_var);
	add_edcu_entry($proc_name, $v_type, 1, $v_var,$egu);
	#$vproc .= "{\"$proc_name\", "
		#. ($v_type eq "int"? "0": "1") .
		#" /* $v_type */ , 1 /* out */, (void *)&(pLocalEpics->$v_var) - (void *)pLocalEpics, 0},\n";
	#$vproc_size++;
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
	#for($efC=0;$efC<12;$efC++)
	#{
	$egu = "undef";
	foreach (@eFields) {
		$vardb .= "    $_\n";
		# Determine if Engineering units are defined in the record fields
		if(index($_, "EGU") != -1) {
			my @egustring = split(/"/, $_);
			$egu = $egustring[1];
			# print "$v_name = EGU is $egu \n";
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
	$vupdate .= "%%\tezcaPut(\"$ifo:$v_name\", ezcaShort,1,s);\n";
	$vupdate .= "}\n";

	if ($top_name) {
		$vardb .= "grecord(${ve_type},\"%IFO%:${tv_name}\")\n";
	} else {
		$vardb .= "grecord(${ve_type},\"%IFO%:%SYS%-%SUBSYS%${temp}\")\n";
	}
	$vardb .= "{\n";
	$vardb .= "}\n";
    } elsif (substr($_,0,11) eq "EZ_CA_WRITE") {
        ($junk, $v_name, $var_name, $v_var) = split(/\s+/, $_);
        $v_name =~ tr/:-/__/;
        $v_name_conn = $v_name . "_ERR";
        $vdecl .= "double evar_$v_name;\n";
        $vdecl .= "assign evar_$v_name to \"${var_name}\";\n";
        $vdecl .= "double evar_$v_name_conn;\n";

        $vinit .= "evar_$v_name  = 0.0;\n";
        $vinit .= "%%  pEpics->${v_var} = evar_$v_name;\n";
        $vinit .= "evar_$v_name_conn = 0.0;\n";
                                                              
        $vupdate .= "if (pvConnected(evar_$v_name)) {\n";
        $vupdate .= "  evar_$v_name_conn = 1;\n";
        $vupdate .= "  if (pvPutComplete(evar_$v_name)) {\n";
        $vupdate .= "    evar_$v_name = fpvalidate(pEpics->${v_var});\n";
        $vupdate .= "    pvPut(evar_$v_name);\n";
        $vupdate .= "  }\n";
        $vupdate .= "} else {\n";
        $vupdate .= "  evar_$v_name_conn = 0;\n";
        $vupdate .= "}\n";
        print "FOUND EZCAWRITE\n";
    } elsif (substr($_,0,10) eq "EZ_CA_READ") {
        ($junk, $v_name, $var_name, $v_var) = split(/\s+/, $_);
        $v_name =~ tr/:-/__/;
        $v_name_conn = $v_name . "_CONN";
        $v_var_conn = $v_var . "_CONN";
        $vdecl .= "double evar_$v_name;\n";
        $vdecl .= "double evar_$v_name_conn;\n";
        $vdecl .= "assign evar_$v_name to \"${var_name}\";\n";
        $vdecl .= "monitor evar_$v_name;\n";

        $vinit .= "evar_$v_name  = 0.0;\n";
        $vinit .= "%%       pEpics->${v_var} = evar_$v_name;\n";
        $vinit .= "evar_$v_name_conn  = 0.0;\n";
        $vinit .= "%%       pEpics->${v_var_conn} = evar_$v_name_conn;\n";

        $vupdate .= "if (pvConnected(evar_$v_name)) {\n";
        $vupdate .= "  evar_$v_name_conn = 1;\n";
        $vupdate .= "%%  pEpics->${v_var} = evar_$v_name;\n";
        $vupdate .= "} else {\n";
        $vupdate .= "  evar_$v_name_conn = 0;\n";
        $vupdate .= "}\n";
        $vupdate .= "%%  pEpics->${v_var_conn} = evar_$v_name_conn;\n";
        print "FOUND EZCAREAD\n";
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
	$rect = "ai";
    	if (substr($_,0,9) eq "MATRIX_RD") {
		$rect = "ao";
	}

	$mdecl .= "double matrix${m_name}[$xXy];\n";
	$mdecl .= "assign matrix${m_name} to { \n";

	$minit .= "%%  for (ii = 0; ii < ${x}; ii++)\n";
	$minit .= "%%    for (jj = 0; jj < ${y}; jj++) {\n";
        $minit .= "%%       ij = ii * ${y} + jj;\n";
	$minit .= "         matrix${m_name}[ij] = 0.;\n";
	$minit .= "         pvPut(matrix${m_name}[ij]);\n";
	$minit .= "%%       pEpics->${m_var}[ii][jj] = 0.;\n";

	$minit .= "%%    }\n";

    	if (substr($_,0,10) eq "MATRIX_SPM" || substr($_,0,9) eq "MATRIX_RD") {
		$mupdate .= "%%  {\n";
		$mupdate .= "%%  for (ii = 0; ii < ${x}; ii++)\n";
		$mupdate .= "%%    for (jj = 0; jj < ${y}; jj++) {\n";
		$mupdate .= "%%      ij = ii * ${y} + jj;\n";
		$mupdate .= "        pvGet(matrix${m_name}[ij]);\n";
		$mupdate .= "%%      if(matrix${m_name}[ij] != pEpics->${m_var}[ii][jj]) {\n";
		$mupdate .= "%%        matrix${m_name}[ij] = pEpics->${m_var}[ii][jj];\n";
		$mupdate .= "          pvPut(matrix${m_name}[ij]);\n";
		$mupdate .= "%%      }\n";
		$mupdate .= "%%    }\n";
		$mupdate .= "%%  }\n";
	} else {

		$mupdate .= "%%  {\n";
		$mupdate .= "%%  unsigned int __lcl_msk = pEpics->${m_var}_mask;\n";
		$mupdate .= "%%  for (ii = 0; ii < ${x}; ii++)\n";
		$mupdate .= "%%    for (jj = 0; jj < ${y}; jj++) {\n";
		$mupdate .= "%%      ij = ii * ${y} + jj;\n";
		$mupdate .= "%% 	if (__lcl_msk) {\n";
		$mupdate .= "%%           matrix${m_name}[ij] = pEpics->${m_var}[ii][jj];\n";
		$mupdate .= "             pvPut(matrix${m_name}[ij]);\n";
		$mupdate .= "%%		} else {\n";
		$mupdate .= "             pvGet(matrix${m_name}[ij]);\n";
		$mupdate .= "%%           rfm_assign(pEpics->${m_var}[ii][jj], matrix${m_name}[ij]);\n";
		$mupdate .= "%%		} \n";
		$mupdate .= "%%    }\n";
		$mupdate .= "%%  }\n";
	}

        my $top_name = is_top_name($m_name);
   	my $tv_name;
	my $proc_name;
        if ($top_name) {
	  	$tv_name = top_name_transform($m_name);
		$proc_name = "%SITE%:$tv_name";
	} else {
		$proc_name = "%SITE%:%SYS%$m_name";
	}

	my $s = $/; $/="_"; chomp $proc_name; $/ = $s;
	$egu = "undef";
	add_vproc_entry($proc_name, "double", 0, $m_var, $x, $y);
	add_edcu_entry($proc_name, "double", 0, $m_var, $egu, $x, $y);
	#$vproc .= "{\"$proc_name\", 1, ".
		#"0 /* in */, (void *)&(pLocalEpics->$m_var) - (void *)pLocalEpics, ".
		#"(void *)&(pLocalEpics->${m_var}_mask) - (void *)pLocalEpics},\n";
	#$vproc_size++;

	for ($i = 1; $i < $x+1; $i++) {
	    for ($j = 1; $j < $y+1; $j++) {
        	if ($top_name) {
			$mdecl .= sprintf("\"{ifo}:${tv_name}%i_%i\"", $i, $j);

		} else {
			$mdecl .= sprintf("\"{ifo}:{sys}-{subsys}${m_name}%i_%i\"", $i, $j);

		}
		if ($i != ($x) || $j != ($y)) {
		    $mdecl .= ", ";

		}
        	if ($top_name) {
			$matdb .= "grecord($rect,\"%IFO%:${tv_name}" . sprintf("%i_%i\")\n", $i, $j);
		} else {
			$matdb .= "grecord($rect,\"%IFO%:%SYS%-%SUBSYS%${m_name}" . sprintf("%i_%i\")\n", $i, $j);
		}
		$matdb .= "{\n";
		$matdb .= "    field(PREC,\"5\")\n";
		$matdb .= "}\n";
	    }
	    $mdecl .= "\n";

	}
	$mdecl .= "};\n";

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

close IN;

# Read daq definition file
#
die "GDS data not specified, internal error, this is a bug" unless $gds_specified;
open(IN,"<$ARGV[0]_daq");
while (<IN>) {
    s/^\s//g;
    s/\s$//g;
    my @nr = split /\s+/;
    next unless length $nr[0];

    my $name, $rate, $type, $science,$egu;
    $rate = $gds_datarate;
    $name = shift @nr;
    $type = "float";
    $science = 0;
    $egu = "";
    foreach $f (@nr) {
      if ($f eq "uint32") {
        $type = "uint32";
      } elsif ($f eq "science") {
        $science = 1;
      } elsif ($f eq "int32") {
        $type = "int32";
      } elsif ($f eq "double") {
        $type = "double";
      } elsif ($f =~ /^\d+$/) { # An integer
        $rate = $f;
      } else {
        $egu = $f;
      }
    }

    die "Invalid DAQ channel $name rate $rate; system rate is $gds_datarate" if $rate > $gds_datarate;
    if (is_top_name($name)) {
    	$name = "$ifo:" . top_name_transform($name);
    } else {
    	$name = "$ifo:$systems[0]$name";
    }

    $DAQ_Channels{$name} = $rate;
    $DAQ_Channels_type{$name} = $type;
    $DAQ_Channels_science{$name} = $science;
    $DAQ_Channels_egu{$name} = $egu;
    #print $name, " ", $rate, "\n";
}
close IN;

open(IN,"<" . $skeleton) || die "cannot open sequencer Skeleton file $skeleton";
open(OUT,">./$ARGV[0].st") || die "cannot open $ARGV[0].st file for writing";

$cnt2 = $cnt*2;
$cnt10 = $cnt*10;

$fpar{"gain_ramp_time"} = "TRAMP";
$fpar{"swstat_alarm_level"} = "SWSTAT.HIGH";

@a = ( \%fpar, "double", \%ipar, "int", \%spar, "string" );

if ($phase > 0) {
   $decl1 .= "%% double rad_angle;\n\n";
}

$decl1 .= $vdecl;
$decl1 .= "\n" . $mdecl . "\n";
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

$ainit .= "\n";

$aupdate .= "\n";

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
    s/%AUPDT%/$aupdate/g;
    s/%DECL4%/$mupdate/g;
    print;
}

open(OUT,">./$ARGV[0].db") || die "cannot open $ARGV[0].db file for writing";
select(OUT);

$hepi = substr($ARGV[0],0,4) eq "hepi";

foreach $i ( @names ) {
    close IN;
    open(IN,"<$rcg_src_dir/src/epics/util/skeleton.db") || die "cannot open skeleton.db file";

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
print "grecord(ao,\"%IFO%:FEC-${dcuId}_RCG_VERSION\")\n";
print "{\n      field(PREC,\"2\")\n}\n";
print "grecord(ao,\"%IFO%:FEC-${dcuId}_SDF_RELOAD\")\n";
print "grecord(ao,\"%IFO%:FEC-${dcuId}_SDF_RESET_CHAN\")\n";
print "grecord(ao,\"%IFO%:FEC-${dcuId}_SDF_MODIFY_CHAN\")\n";
print "grecord(ao,\"%IFO%:FEC-${dcuId}_SDF_SAVE_CMD\")\n";
print "grecord(ao,\"%IFO%:FEC-${dcuId}_SDF_RELOAD_STATUS\")\n";
print "grecord(ao,\"%IFO%:FEC-${dcuId}_SDF_FULL_CNT\")\n";
print "grecord(ao,\"%IFO%:FEC-${dcuId}_SDF_FILE_SET_CNT\")\n";
print "grecord(ao,\"%IFO%:FEC-${dcuId}_SDF_UNMON_CNT\")\n";
print "grecord(ao,\"%IFO%:FEC-${dcuId}_SDF_ALARM_CNT\")\n";
print "grecord(ao,\"%IFO%:FEC-${dcuId}_SDF_CONFIRM\")\n";
print "grecord(ao,\"%IFO%:FEC-${dcuId}_SDF_SELECT_SUM0\")\n";
print "grecord(ao,\"%IFO%:FEC-${dcuId}_SDF_SELECT_SUM1\")\n";
print "grecord(ao,\"%IFO%:FEC-${dcuId}_SDF_SELECT_SUM2\")\n";
print "grecord(ao,\"%IFO%:FEC-${dcuId}_SDF_SELECT_ALL\")\n";
print "grecord(stringout,\"%IFO%:FEC-${dcuId}_SDF_NAME\")\n";
print "grecord(stringout,\"%IFO%:FEC-${dcuId}_SDF_SAVE_AS_NAME\")\n";
print "grecord(stringout,\"%IFO%:FEC-${dcuId}_SDF_RELOAD_TIME\")\n";
print "grecord(stringout,\"%IFO%:FEC-${dcuId}_SDF_SAVE_TIME\")\n";
print "grecord(stringout,\"%IFO%:FEC-${dcuId}_SDF_SAVE_FILE\")\n";
print "grecord(stringout,\"%IFO%:FEC-${dcuId}_SDF_LOADED\")\n";
print "grecord(stringout,\"%IFO%:FEC-${dcuId}_SDF_LOADED_EDB\")\n";
print "grecord(stringout,\"%IFO%:FEC-${dcuId}_SDF_WC_STR\")\n";
print "grecord(stringout,\"%IFO%:FEC-${dcuId}_SDF_MSG_STR\")\n";
print "grecord(bo,\"%IFO%:FEC-${dcuId}_SDF_MON_ALL\")\n";
print "{\n	field(ZNAM,\"MASK\")\n";
print "		field(ONAM,\"ALL\")\n}\n";

print "grecord(ao,\"%IFO%:FEC-${dcuId}_SDF_ALH_CRC\")\n";
print "{\n	field(SCAN,\".5 second\")\n}\n";
print "grecord(ao,\"%IFO%:FEC-${dcuId}_SDF_DIFF_CNT\")\n";
print "{\n	field(SCAN,\".5 second\")\n}\n";
print "grecord(ao,\"%IFO%:FEC-${dcuId}_SDF_RB_ERR_CNT\")\n";
print "{\n	field(SCAN,\".5 second\")\n}\n";
print "grecord(ao,\"%IFO%:FEC-${dcuId}_SDF_DROP_CNT\")\n";
print "{\n	field(SCAN,\".5 second\")\n}\n";
print "grecord(ao,\"%IFO%:FEC-${dcuId}_SDF_UNINIT_CNT\")\n";
print "{\n	field(SCAN,\".5 second\")\n}\n";
print "grecord(ao,\"%IFO%:FEC-${dcuId}_SDF_TABLE_ENTRIES\")\n";
print "{\n	field(SCAN,\".5 second\")\n}\n";
print "grecord(mbbi,\"%IFO%:FEC-${dcuId}_SDF_WILDCARD\")\n";
print "{\n	field(ZRVL,\"0\")\n";
print "		field(ONVL,\"1\")\n";
print "		field(ZRST,\"SHOW ALL\")\n";
print "		field(ONST,\"SORT ON SUBSTRING\")\n}\n";
print "grecord(mbbi,\"%IFO%:FEC-${dcuId}_SDF_SORT\")\n";
print "{\n	field(ZRVL,\"0\")\n";
print "		field(ONVL,\"1\")\n";
print "		field(TWVL,\"2\")\n";
print "		field(THVL,\"3\")\n";
print "		field(FRVL,\"4\")\n";
if ($::casdf) {
print "		field(FVVL,\"5\")\n";
}
print "		field(ZRST,\"SETTING DIFFS\")\n";
print "		field(ONST,\"CHANS NOT FOUND\")\n";
print "		field(TWST,\"CHANS NOT INIT\")\n";
print "		field(THST,\"CHANS NOT MON\")\n";
print "		field(FRST,\"FULL TABLE\")\n";
if ($::casdf) {
print "		field(FVST,\"DISCONNECTED CHANS\")\n";
}
print "}\n";
if ($::casdf) {
print "grecord(ao,\"%IFO%:FEC-${dcuId}_SDF_DISCONNECTED_CNT\")\n";
print "grecord(ao,\"%IFO%:FEC-${dcuId}_SDF_DROPPED_CNT\")\n";
}
print "grecord(ao,\"%IFO%:FEC-${dcuId}_SDF_PAGE\")\n";
print "grecord(ao,\"%IFO%:FEC-${dcuId}_SDF_TABLE_LOCK\")\n";
print "grecord(mbbi,\"%IFO%:FEC-${dcuId}_SDF_SAVE_TYPE\")\n";
print "{\n	field(ZRVL,\"0\")\n";
print "		field(ONVL,\"1\")\n";
print "		field(ZRST,\"TABLE TO FILE\")\n";
print "		field(ONST,\"EPICS DB TO FILE\")\n}\n";
print "grecord(mbbi,\"%IFO%:FEC-${dcuId}_SDF_SAVE_OPTS\")\n";
print "{\n	field(ZRVL,\"0\")\n";
print "		field(ONVL,\"1\")\n";
print "		field(TWVL,\"2\")\n";
print "		field(ZRST,\"TIME NOW\")\n";
print "		field(ONST,\"OVERWRITE\")\n";
print "		field(TWST,\"SAVE AS\")\n}\n";
for(my $ffn = 0;$ffn < 40; $ffn ++)
{
	my $strGrdName = "%IFO%:FEC-${dcuId}_SDF_SP_STAT";
	$strGrdName .= $ffn;
	print "grecord(waveform,\"%IFO%:FEC-${dcuId}_SDF_SP_STAT$ffn\")\n";
	print "{\n	field(NELM,\"64\")\n";
	print "	field(FTVL,\"UCHAR\")\n";
	print "		field(PINI,\"YES\")\n";
	print "}\n";
	print "grecord(waveform,\"%IFO%:FEC-${dcuId}_SDF_SP_STAT$ffn\_BURT\")\n";
	print "{\n	field(NELM,\"64\")\n";
	print "		field(FTVL,\"UCHAR\")\n";
	print "}\n";
	print "grecord(waveform,\"%IFO%:FEC-${dcuId}_SDF_SP_STAT$ffn\_LIVE\")\n";
	print "{\n	field(NELM,\"64\")\n";
	print "		field(FTVL,\"UCHAR\")\n";
	print "}\n";
	print "grecord(waveform,\"%IFO%:FEC-${dcuId}_SDF_SP_STAT$ffn\_TIME\")\n";
	print "{\n	field(NELM,\"64\")\n";
	print "		field(FTVL,\"UCHAR\")\n";
	print "}\n";
	print "grecord(waveform,\"%IFO%:FEC-${dcuId}_SDF_SP_STAT$ffn\_DIFF\")\n";
	print "{\n	field(NELM,\"64\")\n";
	print "		field(FTVL,\"UCHAR\")\n";
	print "}\n";
	print "grecord(ao,\"%IFO%:FEC-${dcuId}_SDF_LINE_$ffn\")\n";
	print "grecord(ao,\"%IFO%:FEC-${dcuId}_SDF_BITS_$ffn\")\n";

	print "grecord(ao,\"%IFO%:FEC-${dcuId}_SDF_FM_LINE_$ffn\")\n";
}
print "grecord(stringout,\"%IFO%:FEC-${dcuId}_GRD_RB_STAT0\")\n";
print "{\n	field(SCAN,\".5 second\")\n}\n";
print "grecord(stringout,\"%IFO%:FEC-${dcuId}_GRD_RB_STAT1\")\n";
print "{\n	field(SCAN,\".5 second\")\n}\n";
print "grecord(stringout,\"%IFO%:FEC-${dcuId}_GRD_RB_STAT2\")\n";
print "{\n	field(SCAN,\".5 second\")\n}\n";
print "grecord(stringout,\"%IFO%:FEC-${dcuId}_GRD_RB_STAT3\")\n";
print "{\n	field(SCAN,\".5 second\")\n}\n";
print "grecord(stringout,\"%IFO%:FEC-${dcuId}_GRD_RB_STAT4\")\n";
print "{\n	field(SCAN,\".5 second\")\n}\n";
print "grecord(stringout,\"%IFO%:FEC-${dcuId}_GRD_RB_STAT5\")\n";
print "{\n	field(SCAN,\".5 second\")\n}\n";
print "grecord(stringout,\"%IFO%:FEC-${dcuId}_GRD_RB_STAT6\")\n";
print "{\n	field(SCAN,\".5 second\")\n}\n";
print "grecord(stringout,\"%IFO%:FEC-${dcuId}_GRD_RB_STAT7\")\n";
print "{\n	field(SCAN,\".5 second\")\n}\n";
print "grecord(stringout,\"%IFO%:FEC-${dcuId}_GRD_RB_STAT8\")\n";
print "{\n	field(SCAN,\".5 second\")\n}\n";
print "grecord(stringout,\"%IFO%:FEC-${dcuId}_GRD_RB_STAT9\")\n";
print "{\n	field(SCAN,\".5 second\")\n}\n";

print "grecord(ao,\"%IFO%:FEC-${dcuId}_STATE_WORD\")\n";
print "grecord(ao,\"%IFO%:FEC-${dcuId}_EPICS_SYNC_TIME\")\n";
print "{\n	field(PREC,\"4\")\n}\n";
print "grecord(ao,\"%IFO%:FEC-${dcuId}_EPICS_WAIT\")\n";
print "grecord(ao,\"%IFO%:FEC-${dcuId}_LOAD_NEW_COEFF\")\n";
print "grecord(stringout,\"%IFO%:FEC-${dcuId}_MSG\")\n";
print "grecord(stringout,\"%IFO%:FEC-${dcuId}_MSG2\")\n";
print "grecord(stringout,\"%IFO%:FEC-${dcuId}_MSGDAQ\")\n";
print "grecord(stringout,\"%IFO%:FEC-${dcuId}_MSG_FESTAT\")\n";
print "grecord(stringout,\"%IFO%:FEC-${dcuId}_TIME_STRING\")\n";
print "grecord(stringout,\"%IFO%:FEC-${dcuId}_BUILD_DATE\")\n";

for(my $fmi = 0; $fmi < 1000; $fmi ++)
{
	print "grecord(ao,\"%IFO%:FEC-${dcuId}_SDF_FM_MASK_$fmi\")\n";
	print "grecord(ao,\"%IFO%:FEC-${dcuId}_SDF_FM_MASK_CTRL_$fmi\")\n";
}

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
	 print "[$ifo:${tv_name}_EXC]\n";
	 $edcuFiltName = "[$ifo:${tv_name}";
	} else {
	 print "[$ifo:${systm}${i}_EXC]\n";
	 $edcuFiltName = "[$ifo:${systm}${i}";
	}
	print "ifoid = $gds_ifo\n";
	print "rmid = $gds_rmid\n";
	print "dcuid = $gds_exc_dcu_id\n";
	print "chnnum = $excnum\n";
	print "datatype = 4\n";	
	print "datarate = $gds_datarate\n\n";
	$excnum++;
	for(my $efn = 0;$efn < 11; $efn ++)
	{
		if($efn == 0) {$edcuFilt .= "$edcuFiltName\_OFFSET]\n";}
		if($efn == 1) {$edcuFilt .= "$edcuFiltName\_GAIN]\n";}
		if($efn == 2) {$edcuFilt .= "$edcuFiltName\_LIMIT]\n";}
		if($efn == 3) {$edcuFilt .= "$edcuFiltName\_TRAMP]\n";}
		if($efn == 4) {$edcuFilt .= "$edcuFiltName\_SWREQ]\n";}
		if($efn == 5) {$edcuFilt .= "$edcuFiltName\_SWMASK]\n";}
		if($efn == 6) {$edcuFilt .= "$edcuFiltName\_INMON]\n";}
		if($efn == 7) {$edcuFilt .= "$edcuFiltName\_EXCMON]\n";}
		if($efn == 8) {$edcuFilt .= "$edcuFiltName\_OUT16]\n";}
		if($efn == 9) {$edcuFilt .= "$edcuFiltName\_OUTPUT]\n";}
		if($efn == 10) {$edcuFilt .= "$edcuFiltName\_SWSTAT]\n";}
	 $edcuFilt .= "acquire=3\n";
	 $edcuFilt .= "datarate=16\n";
	 $edcuFilt .= "datatype=4\n";
	 $edcuFilt .= "chnnum=$edcuFiltNum\n";
	  $edcuFiltNum ++;
	}
      }
      # Print extra excitations
      foreach $i ( @excitations ) {
    	my $top_name = is_top_name($i);
    	my $tv_name;
    	if ($top_name) {
	 $tv_name = top_name_transform($i);
	 print "[$ifo:${tv_name}]\n";
	} else {
	 print "[$ifo:${systm}${i}]\n";
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
	 print "[$ifo:${tv_name}_IN1]\n";
	} else {
	 print "[$ifo:${systm}${i}_IN1]\n";
	}
	print "ifoid = $gds_ifo\n";
	print "rmid = $gds_rmid\n";
	print "dcuid = $gds_tp_dcu_id\n";
	print "chnnum = $tpnum\n";
	print "datatype = 4\n";	
	print "datarate = $gds_datarate\n\n";
	$tpnum++;
    	if ($top_name) {
	 print "[$ifo:${tv_name}_IN2]\n";
	} else {
	 print "[$ifo:${systm}${i}_IN2]\n";
	}
	print "ifoid = $gds_ifo\n";
	print "rmid = $gds_rmid\n";
	print "dcuid = $gds_tp_dcu_id\n";
	print "chnnum = $tpnum\n";
	print "datatype = 4\n";	
	print "datarate = $gds_datarate\n\n";
	$tpnum++;
    	if ($top_name) {
	 print "[$ifo:${tv_name}_OUT]\n";
	} else {
	 print "[$ifo:${systm}${i}_OUT]\n";
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
	 print "[$ifo:${tv_name}]\n";
	} else {
	 print "[$ifo:${systm}${i}]\n";
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
$header = <<END
[default]
gain=1.00
acquire=1
dcuid=$dcuId
ifoid=$ifoid
datatype=4
datarate=$gds_datarate
offset=0
slope=1.0
units=undef

END
;
print OUTG $header;
$edcuEntryI =~ s/%SITE%/$ifo/g;
$edcuEntryI =~ s/%SYS%/$systems[0]/g;
$edcuEntryD =~ s/%SITE%/$ifo/g;
$edcuEntryD =~ s/%SYS%/$systems[0]/g;
print OUTG $edcuEntryI;
print OUTG $edcuEntryD;
print OUTG $edcuFilt;

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
my $daq_name = "DQ";

my $have_daq_spec = 0;
$have_daq_spec = 1 if %DAQ_Channels;
my $dbl_suffix = "DBL";

# Print chnnum, datarate, 
foreach (sort @section_names) {
	my $comment;
	my $dblAdd = 0;

	if ($have_daq_spec) {
	    if (defined $DAQ_Channels{$_}) {
	        ${$sections{$_}}{"datarate"} = $DAQ_Channels{$_};
	        undef $comment;
	        delete $DAQ_Channels{$_};
	        # See if this is an integer channel
	        if ($DAQ_Channels_type{$_} eq "uint32") {
		        ${$sections{$_}}{"datatype"} = 7;
	        }
	        if ($DAQ_Channels_type{$_} eq "int32") {
		        ${$sections{$_}}{"datatype"} = 2;
	        }
	        if ($DAQ_Channels_type{$_} eq "double") {
		        ${$sections{$_}}{"datatype"} = 5;
		        $dblAdd = 1;
	        }
	        if ($DAQ_Channels_egu{$_} ne "") {
		        ${$sections{$_}}{"units"} = $DAQ_Channels_egu{$_};
	        }
	    }  else {
	        $comment = "#";
        }
    } else  {
		$comment = "#";
	}
	my $science = $DAQ_Channels_science{$_};
	if($dblAdd) {
		print OUTG "${comment}[${_}_$dbl_suffix\_${daq_name}]\n";
	} else {
		print OUTG "${comment}[${_}_${daq_name}]\n";
	}
	if ($science) {
		my $hds = 0;
		if ($have_daq_spec) {
			# bit 1 set to indicate storage into commissioning;
			# bit 2 set to indicate storage into science frames
			$hds = 3;
		}
        	print OUTG  "${comment}acquire=$hds\n";
	} else {
        	print OUTG  "${comment}acquire=$have_daq_spec\n";
	}
    # Write out remaining channel info
    foreach $sec (keys %{$sections{$_}}) {
       if ($sec eq "datarate" ) {
           print OUTG  "${comment}$sec=${$sections{$_}}{$sec}\n";
       }
    }
    foreach $sec (keys %{$sections{$_}}) {
       if ($sec eq "datatype" ) {
           print OUTG  "${comment}$sec=${$sections{$_}}{$sec}\n";
       }
    }
    foreach $sec (keys %{$sections{$_}}) {
       if ($sec eq "chnnum" ) {
           print OUTG  "${comment}$sec=${$sections{$_}}{$sec}\n";
       }
    }
    foreach $sec (keys %{$sections{$_}}) {
       if ($sec eq "units" ) {
           print OUTG  "${comment}$sec=${$sections{$_}}{$sec}\n";
       }
    }
}

if (keys %DAQ_Channels) {
  print STDERR "Unknown DAQ channel(s) specified in the model:\n";
  foreach $i (keys %DAQ_Channels) {
	print STDERR "$i\n";
  }
  die;
}
close OUTG;

}
open(PROC, ">$ARGV[0]_proc.h") || die "cannot open $ARGV[0]_proc.h file for writing";
print PROC "struct proc_dir_entry *proc_epics_entry[$vproc_size];\n";
print PROC "struct proc_epics proc_epics[] = {\n";
$vproc =~ s/%SITE%/$ifo/g;
$vproc =~ s/%SYS%/$systems[0]/g;
print PROC $vproc;
print PROC "};\n";
close PROC;

exit(0);
