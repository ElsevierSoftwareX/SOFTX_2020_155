package CDS::ModelRate;
use Exporter;
@ISA = ('Exporter');

#//     \page stateWord StateWord.pm
#//     Documentation for StateWord.pm
#//
#// \n

sub partType {
        return ModelRate;
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
}

# Check inputs are connected
sub checkInputConnect {
        my ($i) = @_;
        return "";
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
	return "FE_RATE"

}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
        return "";
}


