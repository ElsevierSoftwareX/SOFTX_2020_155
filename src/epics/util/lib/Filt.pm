package CDS::Filt;
use Exporter;
@ISA = ('Exporter');

sub partType {
#	if (length $::xpartName[$::partCnt] > $::max_name_len) {
#		die "Filter name \"", $::xpartName[$::partCnt], "\" too long (max $::max_name_len charachters)";
#	}
        print ::OUTH "#define $::xpartName[$::partCnt] \t $::filtCnt\n";
	if ($::biQuad[$::partCnt]) { 
        	print ::EPICS "$::xpartName[$::partCnt] biquad\n";
	} else {
        	print ::EPICS "$::xpartName[$::partCnt]\n";
	}
        $::filterName[$::filtCnt] = $::xpartName[$::partCnt];
        $::filtCnt ++;

	return Filt;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
	;
}

# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {
        my ($i) = @_;
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
        return "";
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	my ($i) = @_;
        my $calcExp = "// FILTER MODULE";
        if ($::ppFIR[$i] == 1) {
           $calcExp .= " (PolyPhase FIR)";
        }
        $calcExp .= ":  $::xpartName[$i]\n";

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
        $calcExp .= ",0);\n";
        return $calcExp;
}
