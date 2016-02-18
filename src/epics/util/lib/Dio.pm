package CDS::Dio;
use Exporter;
@ISA = ('Exporter');

#//     \file Dio.dox
#//     \brief Documentation for Dio.pm
#//
#// \n
#//     \subpage devguidercg2 "<<-- Parts Library"
#// \n

sub initDio {
	my ($node) = @_;
	$::boPartNum[$::boCnt] = $::partCnt;
	my $desc = ${$node->{FIELDS}}{"Name"};
	my $l = length($desc);
        my $num = substr($desc, ($l-1), 1);
        if ($num =~ m/\D/) {
           die "Last character of module name must be digit\: $desc\n";
        } 
	$::boType[$::boCnt] = "ACS_24DIO";
	$::boNum[$::boCnt] = $num;
	$::boCnt ++;
}

sub partType {
	return Dio;
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
        my $card =  substr($::partInput[$i][$j], 4, 1);
        return "dioInput\[" . $card . "\]";
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	my ($i) = @_;
        my $dioNum = substr($::xpartName[$i], 4, 1);
        my $calcExp = "// DIO number is $dioNum\n";
        my $fromType = $::partInputType[$i][$_];
        if (($fromType ne "GROUND") && ($::partInput[$i][0] ne "NC")) {
                $calcExp .= "dioOutput\[";
                $calcExp .= $dioNum;
                $calcExp .= "\] = ";
                $calcExp .= $::fromExp[0];
                $calcExp .= ";\n";
        }
        return $calcExp;

        return "";
}
