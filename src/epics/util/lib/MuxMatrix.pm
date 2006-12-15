package CDS::MuxMatrix;
use Exporter;
@ISA = ('Exporter');

sub partType {
	return MuxMatrix;
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
		die "Part $::xpartName[$i] needs a single MUX input, detected $::partOutputType[$i][0]\n";
	}
	my $matOuts = $::partOutCnt[$::partOutNum[$i][0]];
	my $matIns = $::partInCnt[$::partInNum[$i][0]];
        print ::EPICS "MATRIX $::xpartName[$i]_ $matOuts" . "x$matIns $::systemName\.$::xpartName[$i]\n";
        print ::OUTH "\tfloat $::xpartName[$i]\[$matOuts\]\[$matIns\];\n";
}

# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {}

# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;
	my $matOuts = $::partOutCnt[$::partOutNum[$i][0]];
	my $matIns = $::partInCnt[$::partInNum[$i][0]];
        print ::OUT "double \L$::xpartName[$i]\[$matOuts\]\[$matIns\];\n";
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
        return "\L$::xpartName[$from]" . "\[1\]\[" . $fromPort . "\]";
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	my ($i) = @_;
	my $matOuts = $::partOutCnt[$::partOutNum[$i][0]];
	my $matIns = $::partInCnt[$::partInNum[$i][0]];
	my $muxName = "\L$::xpartName[$::partInNum[$i][0]]";
	my $calcExp = "// MuxMatrix\n";
        $calcExp .= "for(ii=0;ii<$matOuts;ii++)\n{\n";
        $calcExp .= "\L$::xpartName[$i]\[1\]\[ii\] = \n";

        for (0 .. $matIns - 1) {
          $calcExp .= "\tpLocalEpics->$::systemName\.";
          $calcExp .= $::xpartName[$i];
          $calcExp .= "\[ii\]\[";
          $calcExp .= $_;
          $calcExp .= "\] * ";
          $calcExp .= $muxName . "\[$_\]";
          #$calcExp .= $::fromExp[$_];
          if ($_ == ($matIns - 1)) { $calcExp .= ";\n";}
          else { $calcExp .= " +\n"; }
	}
        return $calcExp . "}\n";
}
