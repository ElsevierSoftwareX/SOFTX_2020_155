package CDS::FiltCtrl;
use Exporter;
@ISA = ('Exporter');

sub partType {
	if (length $::xpartName[$::partCnt] > 19) {
		die "Filter name \"", $::xpartName[$::partCnt], "\" too long (max 19 charachters)";
	}
        print ::OUTH "#define $::xpartName[$::partCnt] \t $::filtCnt\n";
        print ::EPICS "$::xpartName[$::partCnt]\n";
        $::filterName[$::filtCnt] = $::xpartName[$::partCnt];
        $::filtCnt ++;

	return FiltCtrl;
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
        print ::OUT "double \L$::xpartName[$i];\n";
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
	   $calcExp = "$dsp_ptr->inputs[$modNum].opSwitchP";
#	   $calcExp .= "| $dsp_ptr->inputs[$modNum].opSwitchE";
# All on/off switching stuff was missing so I am adding this code
# to get the bits from opSwitchE variable
#define OPSWITCH_INPUT_ENABLE 0x4
#define OPSWITCH_OFFSET_ENABLE 0x8
#define OPSWITCH_LIMITER_ENABLE 0x1000000
#define OPSWITCH_DECIMATE_ENABLE 0x2000000
#define OPSWITCH_OUTPUT_ENABLE 0x4000000
#define OPSWITCH_HOLD_ENABLE 0x8000000
	   $calcExp .= "| ((0x4|0x8|0x1000000|0x2000000|0x4000000|0x8000000) & $dsp_ptr->inputs[$modNum].opSwitchE)";
	}
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
        $calcExp .= ",";
        $calcExp .= $::fromExp[1];
        $calcExp .= ");\n";
        return $calcExp;
}
