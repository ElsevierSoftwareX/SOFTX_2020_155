package CDS::SeiWd;
use Exporter;
@ISA = ('Exporter');

sub partType {
	$::useWd++;
	return SeiWd;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
	$::useWdName[$::useWdCounter] = $::xpartName[$i];
	$::useWdName[$::useWdCounter] =~ s/^([A-Z]*)_.*$/\1/g;
	$::useWdCounter++;
        print ::OUTH "\tSEI_WATCHDOG $::xpartName[$i];\n";
$here = <<END;
\tchar	$::xpartName[$i]\_MAX_S_mask;\n
\tchar	$::xpartName[$i]\_MAX_PV_mask;\n
\tchar	$::xpartName[$i]\_MAX_PH_mask;\n
\tchar	$::xpartName[$i]\_MAX_GV_mask;\n
\tchar	$::xpartName[$i]\_MAX_GH_mask;\n
\tchar	$::xpartName[$i]\_MAX_SF_mask;\n
\tchar	$::xpartName[$i]\_MAX_PVF_mask;\n
\tchar	$::xpartName[$i]\_MAX_PHF_mask;\n
\tchar	$::xpartName[$i]\_MAX_GVF_mask;\n
\tchar	$::xpartName[$i]\_MAX_GHF_mask;\n
END
	return $here;
}

# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {
        my ($i) = @_;
        print ::EPICS "DUMMY $::xpartName[$i]\_STATUS int ai 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_TRIP $::systemName\.$::xpartName[$i]\.trip int ai 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_S0 $::systemName\.$::xpartName[$i]\.status[0] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_S1 $::systemName\.$::xpartName[$i]\.status[1] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_S2 $::systemName\.$::xpartName[$i]\.status[2] int ao 0 \n";
        print ::EPICS "DUMMY $::xpartName[$i]\_RESET int ai 0 \n";
        print ::EPICS "INVARIABLE $::xpartName[$i]\_MAX_S $::systemName\.$::xpartName[$i]\.tripSetR\[0\] int ai 0 \n";
        print ::EPICS "INVARIABLE $::xpartName[$i]\_MAX_PV $::systemName\.$::xpartName[$i]\.tripSetR\[1\] int ai 0 \n";
        print ::EPICS "INVARIABLE $::xpartName[$i]\_MAX_PH $::systemName\.$::xpartName[$i]\.tripSetR\[2\] int ai 0 \n";
        print ::EPICS "INVARIABLE $::xpartName[$i]\_MAX_GV $::systemName\.$::xpartName[$i]\.tripSetR\[3\] int ai 0 \n";
        print ::EPICS "INVARIABLE $::xpartName[$i]\_MAX_GH $::systemName\.$::xpartName[$i]\.tripSetR\[4\] int ai 0 \n";
        print ::EPICS "INVARIABLE $::xpartName[$i]\_MAX_SF $::systemName\.$::xpartName[$i]\.tripSetF\[0\] int ai 0 \n";
        print ::EPICS "INVARIABLE $::xpartName[$i]\_MAX_PVF $::systemName\.$::xpartName[$i]\.tripSetF\[1\] int ai 0 \n";
        print ::EPICS "INVARIABLE $::xpartName[$i]\_MAX_PHF $::systemName\.$::xpartName[$i]\.tripSetF\[2\] int ai 0 \n";
        print ::EPICS "INVARIABLE $::xpartName[$i]\_MAX_GVF $::systemName\.$::xpartName[$i]\.tripSetF\[3\] int ai 0 \n";
        print ::EPICS "INVARIABLE $::xpartName[$i]\_MAX_GHF $::systemName\.$::xpartName[$i]\.tripSetF\[4\] int ai 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_STXF $::systemName\.$::xpartName[$i]\.filtMaxHold\[0\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_STYF $::systemName\.$::xpartName[$i]\.filtMaxHold\[1\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_STZF $::systemName\.$::xpartName[$i]\.filtMaxHold\[2\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_PV1F $::systemName\.$::xpartName[$i]\.filtMaxHold\[3\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_PV2F $::systemName\.$::xpartName[$i]\.filtMaxHold\[4\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_PV3F $::systemName\.$::xpartName[$i]\.filtMaxHold\[5\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_PV4F $::systemName\.$::xpartName[$i]\.filtMaxHold\[6\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_PH1F $::systemName\.$::xpartName[$i]\.filtMaxHold\[7\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_PH2F $::systemName\.$::xpartName[$i]\.filtMaxHold\[8\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_PH3F $::systemName\.$::xpartName[$i]\.filtMaxHold\[9\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_PH4F $::systemName\.$::xpartName[$i]\.filtMaxHold\[10\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_GV1F $::systemName\.$::xpartName[$i]\.filtMaxHold\[11\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_GV2F $::systemName\.$::xpartName[$i]\.filtMaxHold\[12\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_GV3F $::systemName\.$::xpartName[$i]\.filtMaxHold\[13\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_GV4F $::systemName\.$::xpartName[$i]\.filtMaxHold\[14\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_GH1F $::systemName\.$::xpartName[$i]\.filtMaxHold\[15\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_GH2F $::systemName\.$::xpartName[$i]\.filtMaxHold\[16\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_GH3F $::systemName\.$::xpartName[$i]\.filtMaxHold\[17\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_GH4F $::systemName\.$::xpartName[$i]\.filtMaxHold\[18\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_STX $::systemName\.$::xpartName[$i]\.senCountHold\[0\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_STY $::systemName\.$::xpartName[$i]\.senCountHold\[1\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_STZ $::systemName\.$::xpartName[$i]\.senCountHold\[2\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_PV1 $::systemName\.$::xpartName[$i]\.senCountHold\[3\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_PV2 $::systemName\.$::xpartName[$i]\.senCountHold\[4\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_PV3 $::systemName\.$::xpartName[$i]\.senCountHold\[5\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_PV4 $::systemName\.$::xpartName[$i]\.senCountHold\[6\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_PH1 $::systemName\.$::xpartName[$i]\.senCountHold\[7\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_PH2 $::systemName\.$::xpartName[$i]\.senCountHold\[8\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_PH3 $::systemName\.$::xpartName[$i]\.senCountHold\[9\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_PH4 $::systemName\.$::xpartName[$i]\.senCountHold\[10\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_GV1 $::systemName\.$::xpartName[$i]\.senCountHold\[11\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_GV2 $::systemName\.$::xpartName[$i]\.senCountHold\[12\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_GV3 $::systemName\.$::xpartName[$i]\.senCountHold\[13\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_GV4 $::systemName\.$::xpartName[$i]\.senCountHold\[14\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_GH1 $::systemName\.$::xpartName[$i]\.senCountHold\[15\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_GH2 $::systemName\.$::xpartName[$i]\.senCountHold\[16\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_GH3 $::systemName\.$::xpartName[$i]\.senCountHold\[17\] int ao 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_GH4 $::systemName\.$::xpartName[$i]\.senCountHold\[18\] int ao 0 \n";
}

# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;
        print ::OUT "double \L$::xpartName[$i]";
        print ::OUT "Filt\[20\];\n";
        print ::OUT "double \L$::xpartName[$i]";
        print ::OUT "Raw\[20\];\n";
}

# Return front end initialization code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndInitCode {
        my ($i) = @_;
	return "";
}

# Figure out part input code
# Argument 1 is the part number
# Argument 2 is the input number
# Returns calculated input code
sub fromExp {
        my ($i, $j) = @_;
        return "";
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	my ($i) = @_;
        my $zz = 0;
        my $calcExp = "// SeiWd GOES HERE ***\n\n";
        for (my $qq = 0; $qq < $::inCnt; $qq += 2) {
          $calcExp .= "\L$::xpartName[$i]";
          $calcExp .= "Raw\[";
          $calcExp .= $zz;
          $calcExp .= "\] = ";
          $calcExp .= $::fromExp[$qq];
          $calcExp .= ";\n";
          my $yy = $qq + 1;
          $calcExp .= "\L$::xpartName[$i]";
          $calcExp .= "Filt\[";
          $calcExp .= $zz;
          $calcExp .= "\] = ";
          $calcExp .= $::fromExp[$yy];
          $calcExp .= ";\n";
          $zz ++;
        }
        $calcExp .= "seiwd1(cycle,\L$::xpartName[$i]";
        $calcExp .= "Raw, \L$::xpartName[$i]";
        $calcExp .= "Filt,\&pLocalEpics->";
        $calcExp .= $::systemName;
        $calcExp .= "\.";
        $calcExp .= $::xpartName[$i];
        $calcExp .= ");\n";
        return $calcExp;
}
