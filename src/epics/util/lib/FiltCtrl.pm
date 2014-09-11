package CDS::FiltCtrl;
use Exporter;
@ISA = ('Exporter');

#//     \page FiltCtrl FiltCtrl.pm
#//     Documentation for FiltCtrl.pm
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

	return FiltCtrl;
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
	   $calcExp = "(($dsp_ptr->inputs[$modNum].opSwitchE & ~0xAAAAA0) | ($dsp_ptr->inputs[$modNum].opSwitchP & 0xAAAAA0))";
	}
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	my ($i) = @_;
        my $calcExp = "// FILTER MODULE with CONTROL:  $::xpartName[$i]\n";
	if($::partInCnt[$i] != 3) {
                die "\n***ERROR: $::partType[$i] with name $::xpartName[$i] has missing inputs\nRequires 3; Only $::partInCnt[$i] provided:  Please ground any unused inputs\n";
        }
	$calcExp .= "pLocalEpics->$::systemName\.$::xpartName[$i]\_MASK = $::fromExp[2];\n";
        $calcExp .= "\L$::xpartName[$i]";
        $calcExp .= " = ";
        if ($::cpus > 2) {
             $calcExp .= "filterModuleD(dspPtr[0],dspCoeff,";
        } else {
             $calcExp .= "filterModuleD(dsp_ptr,dspCoeff,";
        }
        $calcExp .= $::xpartName[$i];
        $calcExp .= ",";
        $calcExp .= $::fromExp[0];
        $calcExp .= ",";
	if ($::partInCnt[$i] > 2) {
        	$calcExp .= "$::fromExp[1], $::fromExp[2]";
	} else {
        	$calcExp .= "$::fromExp[1], 0";
	}
        $calcExp .= ");\n";
        return $calcExp;
}
