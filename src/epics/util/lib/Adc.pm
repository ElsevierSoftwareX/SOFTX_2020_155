package CDS::Adc;
use Exporter;
@ISA = ('Exporter');

sub partType {
        $::adcPartNum[$::adcCnt] = $::partCnt;
        $::adcCnt++;
        $::partUsed[$::partCnt] = 1;
	return Adc;
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
        ;
}

# Figure out part input code
# Argument 1 is the part number
# Argument 2 is the input number
# Returns calculated input code
sub fromExp {
        my ($i, $j) = @_;
        my $card = $::partInNum[$i][$j];
        my $chan = $::partInputPort[$i][$j];
        return "dWord\[" . $card . "\]\[" . $chan . "\]";
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	my ($i) = @_;
        return "";
}
