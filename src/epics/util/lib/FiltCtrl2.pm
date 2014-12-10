package CDS::FiltCtrl2;
use Exporter;
@ISA = ('Exporter');

#//     \page FiltCtrl2 FiltCtrl2.pm
#//     Documentation for FiltCtrl2.pm
#//
#// \n

sub partType {
#	if (length $::xpartName[$::partCnt] > $::max_name_len) {
#		die "Filter name \"", $::xpartName[$::partCnt], "\" too long (max $::max_name_len charachters)";
#	}
        print ::OUTH "#define $::xpartName[$::partCnt] \t $::filtCnt\n";
        if ($::allBiquad || $::biQuad[$::partCnt]) {
                print ::EPICS "$::xpartName[$::partCnt] biquad\n";
        } else {
                print ::EPICS "$::xpartName[$::partCnt]\n";
        }
        $::filterName[$::filtCnt] = $::xpartName[$::partCnt];
        $::filtCnt ++;

	return FiltCtrl2;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
	print ::OUTH "\tint $::xpartName[$i]\_MASK;\n";
	;
}

# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {
        my ($i) = @_;
	print ::EPICS "OUTVARIABLE $::xpartName[$i]\_MASK $::systemName\.$::xpartName[$i]\_MASK int ao 0 field(PREC,\"0\")\n";
        ;
}

# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;
        print ::OUT "double \L$::xpartName[$i] = 0.0;\n";
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
        my $from = $::partInNum[$i][$j];
        my $fromPort = $::partInputPort[$i][$j];
	my $modNum = $::xpartName[$from];
	my $dsp_ptr = "";

	if ($fromPort == 0) {
	       return "";
	} else {
           if ($::cpus > 2) {
             $dsp_ptr = "dspPtr[0]";
           } else {
             $dsp_ptr = "dsp_ptr";
           }
	   if ($fromPort == 1) {
	     $calcExp = "filtCtrlBitConvert($dsp_ptr->inputs[$modNum].opSwitchE) & 0x7fff";
	   } elsif ($fromPort == 2) { # Offset
	     $calcExp = "$dsp_ptr->inputs[$modNum].offset";
	   } elsif ($fromPort == 3) { # Gain
	     $calcExp = "$dsp_ptr->inputs[$modNum].outgain";
	   } elsif ($fromPort == 4) { # Ramp
	     $calcExp = "$dsp_ptr->inputs[$modNum].gain_ramp_time";
	   } else {
	   	die "Invalid input port";
	   }
	}
	return $calcExp;
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	my ($i) = @_;
	if($::partInCnt[$i] < 5) {
		die "\n***ERROR: $::partType[$i] with name $::xpartName[$i] has missing inputs\nRequires 5; Only $::partInCnt[$i] provided:  Please ground any unused inputs\n";
	}
        my $calcExp = "// FILTER MODULE with CONTROL:  $::xpartName[$i]\n";
        $calcExp .= "pLocalEpics->$::systemName\.$::xpartName[$i]\_MASK = ";
        $calcExp .= $::fromExp[2]? $::fromExp[2]: "0";
        $calcExp .= ";\n";
        $calcExp .= "\L$::xpartName[$i]";
        $calcExp .= " = ";
        if ($::cpus > 2) {
             $calcExp .= "filterModuleD2(dspPtr[0],dspCoeff,";
        } else {
             $calcExp .= "filterModuleD2(dsp_ptr,dspCoeff,";
        }
        $calcExp .= $::xpartName[$i];
        $calcExp .= ",";
        $calcExp .= $::fromExp[0];
        $calcExp .= ",";
        $calcExp .= "$::fromExp[1], ". ($::fromExp[2]?$::fromExp[2]:"0")
			. ", " . ($::fromExp[3]?$::fromExp[3]:"0")
			. ", " . ($::fromExp[4]?$::fromExp[4]:"0")
			. ", " . ($::fromExp[5]?$::fromExp[5]:"0");
        $calcExp .= ");\n";
        return $calcExp;
}
