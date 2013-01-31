package CDS::CDI64;
use Exporter;
@ISA = ('Exporter');

#//     \page CDI64 CDI64.pm
#//     Documentation for CDI64.pm
#//
#// \n
#// \n


sub initCDI64 {
	my ($node) = @_;
	$::boPartNum[$::boCnt] = $::partCnt;
	my $desc = ${$node->{FIELDS}}{"Description"};
	my ($CDI64num) = $desc =~ m/card_num=([^,]+)/g;
        if ($CDI64num =~ m/\D/) {
           die "Last character of module name must be digit\: $desc\n";
        } 
	$::boType[$::boCnt] = "CDI64";
	$::boNum[$::boCnt] = $CDI64num;
	$::card2array[$::partCnt] = $CDI64num;
	$::boCnt ++;
	$::bi64Cnt ++;
}

sub partType {
	return CDI64;
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
	my $j = $::partInNum[$i][$j];
	my $card = $::card2array[$j];
	#print "Coding CDI64 with part number $j array number $card\n";
        return "CDIO6464InputInput\[" . $card . "\]";
}


# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	my ($i) = @_;
        return "";
}
