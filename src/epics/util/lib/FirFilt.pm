package CDS::FirFilt;
use Exporter;
@ISA = ('Exporter');

sub partType {
	#print "$partCnt is type FIR\n";
        my $firName = $::xpartName[$::partCnt];
        print ::OUTH "#define $firName \t $::filtCnt\n";
        print ::EPICS "$firName\n";
        $::filtCnt ++;
        $firName = $::xpartName[$::partCnt] . "_DF";
        print ::OUTH "#define $firName \t $::filtCnt\n";
        print ::EPICS "$firName\n";
        $::filtCnt ++;
        $firName = $::xpartName[$::partCnt] . "_UF";
        print ::OUTH "#define $firName \t $::filtCnt\n";
        print ::EPICS "$firName\n";
        $::filtCnt ++;
        $firName = $::xpartName[$::partCnt] . "_CF";
        print ::OUTH "#define $firName \t $::filtCnt\n";
        print ::EPICS "$firName\n";
        $::filtCnt ++;
        #$firName = $::xpartName[$::partCnt] . "_FIR";
        #print ::OUTH "#define $firName \t $::filtCnt\n";
        $::firCnt ++;

	return FirFilt;
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
        return "";
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

# :TODO: This code needs work???

sub frontEndCode {
	my ($mm) = @_;
        my $calcExp = "";
        my $from = $::partInNum[$mm][0];
        my $fromType = $::partInputType[$mm][0];
        my $fromPort = $::partInputPort[$mm][0];
        my $to = $::partOutNum[$mm][0];
        my $toType = $::partType[$to];
        my $toName = $::xpartName[$to];
        print "Found FIR $from $fromType $fromPort\n";
        my $firName = $::xpartName[$mm] . "_FIR";
        if ($cpus > 2) {
          $calcExp = "filterPolyPhase(dspPtr[0],firCoeff,$::xpartName[$mm],$firName,";
        } else {
          $calcExp = "filterPolyPhase(dsp_ptr,firCoeff,a::$xpartName[$mm],$firName,";
        }
        if ($toType eq "Matrix") {
           my $toPort = $::partOutputPort[$mm][0];
           print "\t$::xpartName[$to]\[0\]\[$toPort\] = ";
        }
        if ($fromType eq "Adc") {
           my $card = $from;
           my $chan = $fromPort;
           $calcExp .= "dWord\[";
           $calcExp .= $card;
           $calcExp .= "\]\[";
           $calcExp .= $chan;
           $calcExp .= "\],0);\n";
        }

        return $calcExp;
}
