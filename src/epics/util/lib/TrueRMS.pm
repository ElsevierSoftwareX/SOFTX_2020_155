package CDS::TrueRMS;
use Exporter;
@ISA = ('Exporter');
 
sub partType {
	return TrueRMS;
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
	print ::OUTH "#define \U$::xpartName[$i]\_WINSZ \t $::windowSize\n\n";
	print ::OUT "static int \L$::xpartName[$i]\_first\_time\_through;\n";
	print ::OUT "static int \L$::xpartName[$i]\_is\_first\_cycle;\n";
	print ::OUT "int \L$::xpartName[$i]\_index;\n";
	print ::OUT "double \L$::xpartName[$i]\_sqrin;\n";
	print ::OUT "double \L$::xpartName[$i]\_sqrsum;\n";
	print ::OUT "double \L$::xpartName[$i]\_sqrsumrun;\n";
	print ::OUT "double \L$::xpartName[$i]\_indatsqrd\[\U$::xpartName[$i]\_WINSZ\];\n";
	print ::OUT "double \L$::xpartName[$i]\_sqrval;\n";
	print ::OUT "double \L$::xpartName[$i];\n";

	print ::OUT "int \L$::xpartName[$i]\_n;\n";


}

# Check inputs are connected
sub checkInputConnect {
        my ($i) = @_;
	if($::partInCnt[$i] < 1) {
                print ::CONN_ERRORS "***\n$::partType[$i] with name $::xpartName[$i] has no input connected.\n\n";
        	return "ERROR";
        }
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

# Return front end initialization code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndInitCode {
	my ($i) = @_;
$here = <<END;
\L$::xpartName[$i] = 0.0;
\L$::xpartName[$i]_first_time_through = 1;
END
	return $here;
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndCode {
	my ($i) = @_;
$here = <<END;
// TrueRMS:  $::xpartName[$i]
// Square the input value 
\L$::xpartName[$i]_sqrin \E= $::fromExp[0] * $::fromExp[0];
if (\L$::xpartName[$i]_first_time_through) {
	\L$::xpartName[$i]_first_time_through = 0;
	\L$::xpartName[$i]_is_first_cycle = 1;
	\L$::xpartName[$i]_index = 0;
	\L$::xpartName[$i]_sqrsum = 0.0;
	\L$::xpartName[$i]_sqrsumrun = \L$::xpartName[$i]_sqrin;
	\L$::xpartName[$i]_indatsqrd[0] = \L$::xpartName[$i]_sqrin;
	\L$::xpartName[$i]_sqrval = 0.0;
	\L$::xpartName[$i] \E = $::fromExp[0];

} else {
	// update sums with new input value
	\L$::xpartName[$i]_sqrsum = \L$::xpartName[$i]_sqrsum + \L$::xpartName[$i]_sqrin;
	\L$::xpartName[$i]_sqrsumrun = \L$::xpartName[$i]_sqrsumrun +  \L$::xpartName[$i]_sqrin;

	// update running sum
	\L$::xpartName[$i]_index ++;
	if(\L$::xpartName[$i]_index < \U$::xpartName[$i]_WINSZ) {
                \Lif(! \L$::xpartName[$i]_is_first_cycle)
		    \L$::xpartName[$i]_sqrsumrun = \L$::xpartName[$i]_sqrsumrun - \L$::xpartName[$i]_indatsqrd[\L$::xpartName[$i]_index];
	} else {
		\L$::xpartName[$i]_is_first_cycle = 0;
		\L$::xpartName[$i]_index = 0;
		\L$::xpartName[$i]_sqrsumrun = \L$::xpartName[$i]_sqrsum;
		\L$::xpartName[$i]_sqrsum = 0;
	}

	// store input value squared
	\L$::xpartName[$i]_indatsqrd[\L$::xpartName[$i]_index] = \L$::xpartName[$i]_sqrin;

	// compute output value
	\L$::xpartName[$i]_sqrval = \L$::xpartName[$i]_sqrsumrun / \U$::xpartName[$i]_WINSZ;
	\Lif(\L$::xpartName[$i]_sqrval > 0.0)
             \L$::xpartName[$i] = lsqrt(\L$::xpartName[$i]_sqrval);
	else
             \L$::xpartName[$i] = 0.0;
}
	
END
	return $here;
}

