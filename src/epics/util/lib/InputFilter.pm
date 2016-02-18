package CDS::InputFilter;
use Exporter;
@ISA = ('Exporter');

sub partType {
	return InputFilter;
}

sub printHeaderStruct {
	my ($i) = @_;
#       if (length $::xpartName[$i] > 24) {
#      		die "InputFilter name \"", $::xpartName[$i], "\" too long (max 24 charachters)";
#  	}
	print ::OUTH "\tdouble $::xpartName[$i]\_OFFSET;\n";
	print ::OUTH "\tdouble $::xpartName[$i]\_GAIN;\n";
	print ::OUTH "\tdouble $::xpartName[$i]\_POLE;\n";
	print ::OUTH "\tdouble $::xpartName[$i]\_ZERO;\n";
	print ::OUTH "\tdouble $::xpartName[$i]\_GAIN_TRAMP;\n";
	print ::OUTH "\tdouble $::xpartName[$i]\_POLE_TRAMP;\n";
	print ::OUTH "\tdouble $::xpartName[$i]\_ZERO_TRAMP;\n";
$here = <<END;
\tchar $::xpartName[$i]\_OFFSET_mask;\n
\tchar $::xpartName[$i]\_GAIN_mask;\n
\tchar $::xpartName[$i]\_POLE_mask;\n
\tchar $::xpartName[$i]\_ZERO_mask;\n
\tchar $::xpartName[$i]\_GAIN_TRAMP_mask;\n
\tchar $::xpartName[$i]\_POLE_TRAMP_mask;\n
\tchar $::xpartName[$i]\_ZERO_TRAMP_mask;\n
END
	return $here;
}

sub printEpics {
   	my ($i) = @_;

	print ::EPICS "INVARIABLE $::xpartName[$i]\_OFFSET $::systemName\.$::xpartName[$i]\_OFFSET double ai 0 field(PREC,\"3\")\n";
	print ::EPICS "INVARIABLE $::xpartName[$i]\_GAIN $::systemName\.$::xpartName[$i]\_GAIN double ai 0 field(PREC,\"3\")\n";
	print ::EPICS "INVARIABLE $::xpartName[$i]\_POLE $::systemName\.$::xpartName[$i]\_POLE double ai 0 field(PREC,\"3\")\n";
	print ::EPICS "INVARIABLE $::xpartName[$i]\_ZERO $::systemName\.$::xpartName[$i]\_ZERO double ai 0 field(PREC,\"3\")\n";
	print ::EPICS "INVARIABLE $::xpartName[$i]\_GAIN_TRAMP $::systemName\.$::xpartName[$i]\_GAIN_TRAMP double ai 0 field(PREC,\"3\")\n";
	print ::EPICS "INVARIABLE $::xpartName[$i]\_POLE_TRAMP $::systemName\.$::xpartName[$i]\_POLE_TRAMP double ai 0 field(PREC,\"3\")\n";
	print ::EPICS "INVARIABLE $::xpartName[$i]\_ZERO_TRAMP $::systemName\.$::xpartName[$i]\_ZERO_TRAMP double ai 0 field(PREC,\"3\")\n";
}

sub printFrontEndVars  {
   	my ($i) = @_;
        #print ::OUT "double \L$::xpartName[$i]_offset;\n";
        print ::OUT "double \L$::xpartName[$i]_GAIN;\n";
        print ::OUT "double \L$::xpartName[$i]_POLE;\n";
        print ::OUT "double \L$::xpartName[$i]_ZERO;\n";
        print ::OUT "unsigned long \L$::xpartName[$i]_KS;\n";
        print ::OUT "unsigned long \L$::xpartName[$i]_PS;\n";
        print ::OUT "unsigned long \L$::xpartName[$i]_ZS;\n";
        print ::OUT "double \L$::xpartName[$i];\n";
        print ::OUT "double \L$::xpartName[$i]_val;\n";
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

sub frontEndInitCode {
        my ($i) = @_;
	# Initialize from the EPICS records
#        my $calcExp = "\L$::xpartName[$i]_offset " . " = pLocalEpics->$::systemName.$::xpartName[$i]_OFFSET;\n";
        my $calcExp .= "\L$::xpartName[$i]_GAIN " . " = pLocalEpics->$::systemName.$::xpartName[$i]_GAIN;\n";
        $calcExp .= "\L$::xpartName[$i]_POLE " . " = pLocalEpics->$::systemName.$::xpartName[$i]_POLE;\n";
        $calcExp .= "\L$::xpartName[$i]_ZERO " . " = pLocalEpics->$::systemName.$::xpartName[$i]_ZERO;\n";
        $calcExp .= "\L$::xpartName[$i]_KS = 0;\n";
        $calcExp .= "\L$::xpartName[$i]_PS = 0;\n";
        $calcExp .= "\L$::xpartName[$i]_ZS = 0;\n";
        $calcExp .= "\L$::xpartName[$i] = 0.0;\n";
        $calcExp .= "\L$::xpartName[$i]_val = 0.0;\n";
        return $calcExp;
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

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	my ($i) = @_;
        my $calcExp = "// Input Filter:  $::xpartName[$i]\n";
	my $input = "$::fromExp[0]";

        $calcExp .= "inputFilterModule($input, &\L$::xpartName[$i], &\L$::xpartName[$i]_val, "
		  . "pLocalEpics->$::systemName.$::xpartName[$i]_OFFSET, "
		  . "&\L$::xpartName[$i]_GAIN, &\L$::xpartName[$i]_POLE, &\L$::xpartName[$i]_ZERO, "
		  . "pLocalEpics->$::systemName.$::xpartName[$i]_GAIN, "
		  . "pLocalEpics->$::systemName.$::xpartName[$i]_POLE, "
		  . "pLocalEpics->$::systemName.$::xpartName[$i]_ZERO, "
		  . "pLocalEpics->$::systemName.$::xpartName[$i]_GAIN_TRAMP, "
		  . "pLocalEpics->$::systemName.$::xpartName[$i]_POLE_TRAMP, "
		  . "pLocalEpics->$::systemName.$::xpartName[$i]_ZERO_TRAMP, "
		  . "&\L$::xpartName[$i]_KS, "
		  . "&\L$::xpartName[$i]_PS, "
		  . "&\L$::xpartName[$i]_ZS);";
        return $calcExp . "\n";
}
