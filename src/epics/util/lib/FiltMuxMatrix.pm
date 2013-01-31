package CDS::FiltMuxMatrix;
use Exporter;
@ISA = ('Exporter');

#//     \page FiltMuxMatrix FiltMuxMatrix.pm
#//     Documentation for FiltMuxMatrix.pm
#//
#// \n

sub partType {
	return FiltMuxMatrix;
}

sub printHeaderStruct {
        my ($i) = @_;
#	if (length $::xpartName[$i] > 18) {
#               die "FilterMuxMatrix name \"", $::xpartName[$i], "\" too long (max 18 charachters)";
#       }

	my $matOuts = $::partOutputs[$::partOutNum[$i][0]];
	#print("$::partOutNum[$artCnt][0]"); die;
	my $matIns = $::partInCnt[$::partInNum[$i][0]];
        print("$matOuts $matIns\n");
	for ($input = 0; $input < $matIns; $input++) {
	  for ($output = 0; $output < $matOuts; $output++) {
	    my $input_plus_one = $input + 1;
	    my $output_plus_one = $output + 1;

	    my $outhOut = "#define $::xpartName[$i]_$output_plus_one" . "_$input_plus_one " ."\t $::filtCnt\n"; 
	    print ::OUTH $outhOut;
	    my $epicsOut = "$::xpartName[$i]" . "_$output_plus_one" . "_$input_plus_one";

            if ($::allBiquad || $::biQuad[$::partCnt]) {
                print ::EPICS "$epicsOut biquad\n";
            } else {
                print ::EPICS "$epicsOut\n";
            }

            $::filterName[$::filtCnt] = "$::xpartName[$i]" . "_$output_plus_one" . "_$input_plus_one";
            $::filtCnt ++;
	  }
	}
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub commented_printHeaderStruct {
        my ($i) = @_;
	# Make sure input is a MUX and Output is DEMUX
	if ($::partInCnt[$i] != 1) {
		die "Part $::xpartName[$i] needs a single input\n";
	}
	if ($::partInputType[$i][0] ne "MUX") {
		die "Part $::xpartName[$i] needs a single MUX input, detected $::partInputType[$i][0]\n";
	}
	if ($::partOutCnt[$i] != 1) {
		die "Part $::xpartName[$i] needs a single output\n";
	}
	if ($::partOutputType[$i][0] ne "DEMUX") {
		die "Part $::xpartName[$i] needs a single DEMUX output, detected $::partOutputType[$i][0]\n";
	}
	#my $matOuts = $::partOutputs[$::partOutNum[$i][0]];
	#my $matIns = $::partInCnt[$::partInNum[$i][0]];
        #print ::EPICS "MATRIX $::xpartName[$i]_ $matOuts" . "x$matIns $::systemName\.$::xpartName[$i]\n";
        #print ::OUTH "\tdouble $::xpartName[$i]\[$matOuts\]\[$matIns\];\n";
}

# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {}

# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;
	my $matOuts = $::partOutputs[$::partOutNum[$i][0]];
	my $matIns = $::partInCnt[$::partInNum[$i][0]];
	my $filtName = "";
        #print ::OUT "double \L$::xpartName[$i]\[$matOuts\]\[$matIns\];\n";
        #print ::OUT "double \L$::xpartName[$i]\[$matOuts\];\n";
	for ($input = 0; $input < $matIns; $input++) {
	  for ($output = 0; $output < $matOuts; $output++) {
	    my $input_plus_one = $input + 1;
            my $output_plus_one = $output + 1;

	    $filtName = "\L$::xpartName[$i]_$output_plus_one" . "_$input_plus_one";
	    print ::OUT "double $filtName;\n";
	  }
	}
	print ::OUT "double \L$::xpartName[$i]\[$matOuts\];\n";

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
        #my $fromPort = $::partInputPort[$i][$j];
        #return "\L$::xpartName[$from]" . "\[1\]\[" . $fromPort . "\]";
        return "\L$::xpartName[$from]";
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	my ($i) = @_;
	my $matOuts = $::partOutputs[$::partOutNum[$i][0]];
	my $matIns = $::partInCnt[$::partInNum[$i][0]];
	my $muxName = "\L$::xpartName[$::partInNum[$i][0]]";
        my $calcExp = "//Filter(s) of FiltMuxMatrix:  $::xpartName[$i]\n";
	
	for ($input = 0; $input < $matIns; $input++) {
	  for ($output = 0; $output < $matOuts; $output++) {
	    my $input_plus_one = $input + 1;
	    my $output_plus_one = $output + 1;

	    $calcExp .= "\n// FILTER MODULE: $::xpartName[$i]";
	    $calcExp .= "_$output_plus_one" . "_$input_plus_one\n";
	    $calcExp .= "\L$::xpartName[$i]" . "_$output_plus_one" . "_$input_plus_one = ";
	    if ($::cpus > 2) {
	      $calcExp .= "filterModuleD(dspPtr[0],dspCoeff,";
	    } else {
	      $calcExp .= "filterModuleD(dsp_ptr,dspCoeff,";
	    }
	    $calcExp .= "$::xpartName[$i]" . "_$output_plus_one" . "_$input_plus_one";
	    $calcExp .= ",";
	    $calcExp .= $muxName . "\[$input\]";
	    $calcExp .= ",0,0);\n";
	    }
	}

        $calcExp .= "\n//Mux of FiltMuxMatrix:  $::xpartName[$i]\n";
	
	for ($output = 0; $output < $matOuts; $output++) {
	  $calcExp .= "\n\L$::xpartName[$i]\[$output\] = \n";
	  for ($input = 0; $input < $matIns; $input++) {

	    my $input_plus_one = $input + 1;
	    my $output_plus_one = $output + 1;
	    $calcExp .= "\t\L$::xpartName[$i]" . "_$output_plus_one" . "_$input_plus_one";
            if ($input == ($matIns - 1)) { $calcExp .= ";\n";}
            else { $calcExp .= " +\n"; }
          }
	}
        return $calcExp . "\n";
}
