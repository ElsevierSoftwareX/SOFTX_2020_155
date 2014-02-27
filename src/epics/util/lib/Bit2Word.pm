package CDS::Bit2Word;
use Exporter;
@ISA = ('Exporter');

sub partType {
	my $instance = 0;
	return Bit2Word;
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
        print ::OUT "unsigned int \L$::xpartName[$i];\n";
	if ($instance == 0)
	{
        print ::OUT "unsigned int powers_of_2[16] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512,\n";
        print ::OUT "                                1024, 2048, 4096, 8192, 16384, 32768};\n";
	}
	$instance += 1;
}

# Figure out part input code
# Argument 1 is the part number
# Argument 2 is the input number
# Returns calculated input code
sub fromExp {
        my ($i, $j) = @_;
        my $from = $::partInNum[$i][$j];
        return "\L$::xpartName[$from]";
}

# Return front end initialization code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndInitCode {
	my ($i) = @_;
	return "\L$::xpartName[$i] = 0;\n";
}


# Return front end code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndCode {
	my ($i) = @_;
	if($::partInCnt[$i] < 16) {
                die "\n***ERROR: $::partType[$i] with name $::xpartName[$i] has missing inputs\nRequires 16; Only $::partInCnt[$i] provided:  Please ground any unused inputs\n";
        }
        my $calcExp = "// Bit2Word:  $::xpartName[$i]\n{\n";
        $calcExp .= "double ins[$::partInCnt[$i]] = {\n";
        for (0 .. $::partInCnt[$i]-2) {
           $calcExp .= "\t$::fromExp[$_],\n";
        }
        $calcExp .= "\t$::fromExp[$partInCnt[$i]-1]\n";
        $calcExp .= "};\n";
        $calcExp .= "\L$::xpartName[$i] = 0;\n";
        $calcExp .= "for (ii = 0; ii < $::partInCnt[$i]; ii++)\n{\n";
        $calcExp .= "if (ins[ii]) {\n";
        $calcExp .= "\L$::xpartName[$i] += powers_of_2\[ii\];\n";
        $calcExp .= "}\n";
        $calcExp .= "}\n";
        $calcExp .= "}\n";
	return $calcExp;
}
