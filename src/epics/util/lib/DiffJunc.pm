package CDS::DiffJunc;
use Exporter;
@ISA = ('Exporter');

sub partType {
	return DiffJunc;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
}

# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {
        my ($i) = @_;
}


# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;
	print ::OUT "double \L$::xpartName[$i]\[16\];\n";
}

# Check inputs are connected
sub checkInputConnect {
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
        return "\L$::xpartName[$from]". "\[" . $fromPort . "\]";
}

# Return front end initialization code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndInitCode {
	my ($i) = @_;
	return "";
}


# Return front end code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndCode {
	my ($i) = @_;
        my $calcExp = "// DiffJunc (MultiSubtract):  $::xpartName[$i]\n";
        my $zz = 0;
        for (my $qq = 0; $qq < 16; $qq += 2) {
          my $yy = $qq + 1;
          $calcExp .= "\L$::xpartName[$i]";
          $calcExp .= "\[";
          $calcExp .= $zz;
          $calcExp .= "\] = ";
          $calcExp .= $::fromExp[$yy];
          $calcExp .= " - ";
          $calcExp .= $::fromExp[$qq];
          $calcExp .= ";\n";
          $zz ++;
        }
	return $calcExp;
}
