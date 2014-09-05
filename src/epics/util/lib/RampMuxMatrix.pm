package CDS::RampMuxMatrix;
use Exporter;
@ISA = ('Exporter');


#//     \page RampMuxMatrix RampMuxMatrix.pm
#//     Documentation for RampMuxMatrix.pm
#//
#// \n

sub partType {
	return RampMuxMatrix;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
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
	my $matOuts = $::partOutputs[$::partOutNum[$i][0]];
	my $matIns = $::partInCnt[$::partInNum[$i][0]];
        print ::OUTH "\tdouble $::xpartName[$i]\[$matOuts\]\[$matIns\];\n";
        print ::OUTH "\tdouble $::xpartName[$i]" . "_SETTING\[$matOuts\]\[$matIns\];\n";
	print ::OUTH "\tdouble $::xpartName[$i]" . "_RAMPING\[$matOuts\]\[$matIns\];\n";
        print ::OUTH "\tdouble $::xpartName[$i]_LOAD_MATRIX;\n";
        print ::OUTH "\tdouble $::xpartName[$i]" . "_TRAMP;\n";
	   
$here = <<END;
\tchar $::xpartName[$i]\_LOAD_MATRIX_mask;\n
\tchar $::xpartName[$i]\_mask;\n
\tchar $::xpartName[$i]\_SETTING_mask;\n
\tchar $::xpartName[$i]\_RAMPING_mask;\n
\tchar $::xpartName[$i]\_TRAMP_mask;\n
END
	return $here;
}

# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {
	my ($i) = @_;
	my $matOuts = $::partOutputs[$::partOutNum[$i][0]];
	my $matIns = $::partInCnt[$::partInNum[$i][0]];
        print ::EPICS "MATRIX $::xpartName[$i]_ $matOuts" . "x$matIns $::systemName\.$::xpartName[$i]\n";
        print ::EPICS "MATRIX $::xpartName[$i]_SETTING_ $matOuts" . "x$matIns $::systemName\.$::xpartName[$i]_SETTING\n";
	print ::EPICS "MATRIX $::xpartName[$i]_RAMPING_ $matOuts" . "x$matIns $::systemName\.$::xpartName[$i]_RAMPING\n";
	print ::EPICS "MOMENTARY $::xpartName[$i]_LOAD_MATRIX $::systemName\.$::xpartName[$i]_LOAD_MATRIX double ao 0\n";
    	print ::EPICS "INVARIABLE $::xpartName[$i]\_TRAMP $::systemName\.$::xpartName[$i]\_TRAMP double ai 0 field(PREC,\"1\")\n";
}

# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
    my ($i) = @_;
    my $matOuts = $::partOutputs[$::partOutNum[$i][0]];
    my $matIns = $::partInCnt[$::partInNum[$i][0]];
    print ::OUT "double \L$::xpartName[$i]\[$matOuts\];\n";
    print ::OUT "static RampParamState \L$::xpartName[$i]_state\[$matOuts\]\[$matIns\];\n";
    $::feTailCode .= "pLocalEpics->" . $::systemName . "\." . "$::xpartName[$i]_LOAD_MATRIX" . " =  0;\n";
    if ($::rampMatrixUsed == 0) {
          print ::OUT "int matrixInputCount, matrixOutputCount;\n";
          $::rampMatrixUsed = 1;
        }

    return "";

}

# Return front end initialization code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndInitCode {
        my ($i) = @_;
        my $matOuts = $::partOutputs[$::partOutNum[$i][0]];
        my $matIns = $::partInCnt[$::partInNum[$i][0]];
        for ($input = 0; $input < $matIns; $input++) {
            for ($output = 0; $output < $matOuts; $output++) {
                $calcExp .= "RampParamInit(&\L$::xpartName[$i]_state\[$output\]\[$input\],0, ";
                $calcExp .= "FE_RATE);\n";
            }
        }
        return $calcExp;
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
    my $calcExp = "// RampMuxMatrix:  $::xpartName[$i]\n";

    $calcExp .= "pLocalEpics->$::systemName\.$::xpartName[$i]_mask = 1;\n";
    $calcExp .= "pLocalEpics->$::systemName\.$::xpartName[$i]_RAMPING_mask = 1;\n";
    $calcExp .= "if (pLocalEpics->". $::systemName . "\." . "$::xpartName[$i]_LOAD_MATRIX" . " == 1)\n";
    $calcExp .= "{\n";
    $calcExp .= "pLocalEpics->". $::systemName . "\." . "$::xpartName[$i]_LOAD_MATRIX = 0;\n";
    $calcExp .= "\tfor (matrixOutputCount = 0; matrixOutputCount < $matOuts; matrixOutputCount++) {\n";
    $calcExp .= "\t\tfor (matrixInputCount = 0; matrixInputCount < $matIns; matrixInputCount++) {\n";
    $calcExp .= "\t\t\tRampParamLoad(&\L$::xpartName[$i]_state" . "\[matrixOutputCount\]\[matrixInputCount\],";
    $calcExp .= "pLocalEpics->$::systemName\.$::xpartName[$i]_SETTING\[matrixOutputCount\]\[matrixInputCount\], pLocalEpics->$::systemName\.$::xpartName[$i]\_TRAMP,FE_RATE);\n";
    $calcExp .= "\t\t}\n";
    $calcExp .= "\t}\n";
    $calcExp .= "}\n";    

    $calcExp .= "for(ii=0;ii<$matOuts;ii++)\n{\n";
    $calcExp .= "\L$::xpartName[$i]\[ii\] = \n";

    for (0 .. $matIns - 1) {
        $calcExp .= "RampParamUpdate(&\L$::xpartName[$i]_state";
        $calcExp .= "\[ii\]\[";
        $calcExp .= $_;
        $calcExp .= "\]) * ";
        $calcExp .= $muxName . "\[$_\]";
        if ($_ == ($matIns - 1)) { $calcExp .= ";\n";}
        else { $calcExp .= " +\n"; }
    }
    $calcExp .= "}\n";
    $calcExp .= "for (matrixOutputCount = 0; matrixOutputCount < $matOuts; matrixOutputCount++) {\n";
    $calcExp .= "\tfor (matrixInputCount = 0; matrixInputCount < $matIns; matrixInputCount++) {\n";
    $calcExp .= "\t\tpLocalEpics->$::systemName\.$::xpartName[$i]\[matrixOutputCount\]\[matrixInputCount\] = RampParamGetVal(&\L$::xpartName[$i]_state" ."\[matrixOutputCount\]\[matrixInputCount\]);\n";
    $calcExp .= "\t\tpLocalEpics->$::systemName\.$::xpartName[$i]_RAMPING\[matrixOutputCount\]\[matrixInputCount\] = RampParamGetIsRamping(&\L$::xpartName[$i]_state" ."\[matrixOutputCount\]\[matrixInputCount\]);\n";
    $calcExp .= "\t}\n";
    $calcExp .= "}\n";
    return $calcExp; 
}
