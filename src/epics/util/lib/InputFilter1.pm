package CDS::InputFilter1;
use Exporter;
@ISA = ('Exporter');

sub partType {
	return InputFilter1;
}

sub printHeaderStruct {
	my ($i) = @_;
        if (length $::xpartName[$i] > 24) {
       		die "InputFilter1 name \"", $::xpartName[$i], "\" too long (max 24 charachters)";
   	}
	print ::OUTH "\tdouble $::xpartName[$i]\_OFFSET;\n";
	print ::OUTH "\tdouble $::xpartName[$i]\_K;\n";
	print ::OUTH "\tdouble $::xpartName[$i]\_P;\n";
	print ::OUTH "\tdouble $::xpartName[$i]\_Z;\n";
	print ::OUTH "\tdouble $::xpartName[$i]\_TRAMP;\n";
	print ::OUTH "\tint $::xpartName[$i]\_DORAMP;\n";
$here = <<END;
\tchar $::xpartName[$i]\_OFFSET_mask;\n
\tchar $::xpartName[$i]\_K_mask;\n
\tchar $::xpartName[$i]\_P_mask;\n
\tchar $::xpartName[$i]\_Z_mask;\n
\tchar $::xpartName[$i]\_TRAMP_mask;\n
\tchar $::xpartName[$i]\_DORAMP_mask;\n
END
	return $here;
}

sub printEpics {
   	my ($i) = @_;

	print ::EPICS "INVARIABLE $::xpartName[$i]\_OFFSET $::systemName\.$::xpartName[$i]\_OFFSET double ai 0 field(PREC,\"3\")\n";
	print ::EPICS "INVARIABLE $::xpartName[$i]\_K $::systemName\.$::xpartName[$i]\_K double ai 0 field(PREC,\"3\")\n";
	print ::EPICS "INVARIABLE $::xpartName[$i]\_P $::systemName\.$::xpartName[$i]\_P double ai 0 field(PREC,\"3\")\n";
	print ::EPICS "INVARIABLE $::xpartName[$i]\_Z $::systemName\.$::xpartName[$i]\_Z double ai 0 field(PREC,\"3\")\n";
	print ::EPICS "INVARIABLE $::xpartName[$i]\_TRAMP $::systemName\.$::xpartName[$i]\_TRAMP double ai 0 field(PREC,\"3\")\n";
	print ::EPICS "MOMENTARY $::xpartName[$i]\_DORAMP $::systemName\.$::xpartName[$i]\_DORAMP int ao 0 field(PREC,\"3\")\n";
}

sub printFrontEndVars  {
   	my ($i) = @_;
        #print ::OUT "double \L$::xpartName[$i]_offset;\n";
        print ::OUT "double \L$::xpartName[$i]_K;\n";
        print ::OUT "double \L$::xpartName[$i]_P;\n";
        print ::OUT "double \L$::xpartName[$i]_Z;\n";
        print ::OUT "unsigned long \L$::xpartName[$i]_KS;\n";
        print ::OUT "unsigned long \L$::xpartName[$i]_PS;\n";
        print ::OUT "unsigned long \L$::xpartName[$i]_ZS;\n";
        print ::OUT "double \L$::xpartName[$i];\n";
        print ::OUT "double \L$::xpartName[$i]_val;\n";
}

sub frontEndInitCode {
        my ($i) = @_;
	# Initialize from the EPICS records
#        my $calcExp = "\L$::xpartName[$i]_offset " . " = pLocalEpics->$::systemName.$::xpartName[$i]_OFFSET;\n";
        my $calcExp .= "\L$::xpartName[$i]_K " . " = pLocalEpics->$::systemName.$::xpartName[$i]_K;\n";
        $calcExp .= "\L$::xpartName[$i]_P " . " = pLocalEpics->$::systemName.$::xpartName[$i]_P;\n";
        $calcExp .= "\L$::xpartName[$i]_Z " . " = pLocalEpics->$::systemName.$::xpartName[$i]_Z;\n";
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

        $calcExp .= "inputFilterModule1($input, &\L$::xpartName[$i], &\L$::xpartName[$i]_val, "
		  . "pLocalEpics->$::systemName.$::xpartName[$i]_OFFSET, "
		  . "&\L$::xpartName[$i]_K, &\L$::xpartName[$i]_P, &\L$::xpartName[$i]_Z, "
		  . "pLocalEpics->$::systemName.$::xpartName[$i]_K, "
		  . "pLocalEpics->$::systemName.$::xpartName[$i]_P, "
		  . "pLocalEpics->$::systemName.$::xpartName[$i]_Z, "
		  . "pLocalEpics->$::systemName.$::xpartName[$i]_TRAMP, "
		  . "&(pLocalEpics->$::systemName.$::xpartName[$i]_DORAMP), "
		  . "&\L$::xpartName[$i]_KS, "
		  . "&\L$::xpartName[$i]_PS, "
		  . "&\L$::xpartName[$i]_ZS);";
        return $calcExp . "\n";
}
