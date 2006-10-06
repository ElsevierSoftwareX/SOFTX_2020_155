package CDS::Filt;
use Exporter;
@ISA = ('Exporter');

sub partType {
        print ::OUTH "#define $::xpartName[$::partCnt] \t $::filtCnt\n";
        print ::EPICS "$::xpartName[$::partCnt]\n";
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


# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;
        print ::OUT "double \L$::xpartName[$i];\n";
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
        my $calcExp = "// FILTER MODULE\n";
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
