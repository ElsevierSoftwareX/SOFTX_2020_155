package CDS::InputFilter;
use Exporter;
@ISA = ('Exporter');

sub partType {
	return InputFilter;
}

sub printHeaderStruct {
	my ($i) = @_;
        if (length $::xpartName[$i] > 24) {
       		die "InputFilter name \"", $::xpartName[$i], "\" too long (max 24 charachters)";
   	}
	print ::OUTH "\tfloat $::xpartName[$i]\_OFFSET;\n";
	print ::OUTH "\tfloat $::xpartName[$i]\_K;\n";
	print ::OUTH "\tfloat $::xpartName[$i]\_P;\n";
	print ::OUTH "\tfloat $::xpartName[$i]\_Z;\n";
	print ::OUTH "\tfloat $::xpartName[$i]\_K_TRAMP;\n";
	print ::OUTH "\tfloat $::xpartName[$i]\_P_TRAMP;\n";
	print ::OUTH "\tfloat $::xpartName[$i]\_Z_TRAMP;\n";
}

sub printEpics {
   	my ($i) = @_;

	print ::EPICS "INVARIABLE $::xpartName[$i]\_OFFSET $::systemName\.$::xpartName[$i]\_OFFSET float ai 0 field(PREC,\"3\")\n";
	print ::EPICS "INVARIABLE $::xpartName[$i]\_K $::systemName\.$::xpartName[$i]\_K float ai 0 field(PREC,\"3\")\n";
	print ::EPICS "INVARIABLE $::xpartName[$i]\_P $::systemName\.$::xpartName[$i]\_P float ai 0 field(PREC,\"3\")\n";
	print ::EPICS "INVARIABLE $::xpartName[$i]\_Z $::systemName\.$::xpartName[$i]\_Z float ai 0 field(PREC,\"3\")\n";
	print ::EPICS "INVARIABLE $::xpartName[$i]\_K_TRAMP $::systemName\.$::xpartName[$i]\_K_TRAMP float ai 0 field(PREC,\"3\")\n";
	print ::EPICS "INVARIABLE $::xpartName[$i]\_P_TRAMP $::systemName\.$::xpartName[$i]\_P_TRAMP float ai 0 field(PREC,\"3\")\n";
	print ::EPICS "INVARIABLE $::xpartName[$i]\_Z_TRAMP $::systemName\.$::xpartName[$i]\_Z_TRAMP float ai 0 field(PREC,\"3\")\n";
}

sub printFrontEndVars  {
   	my ($i) = @_;
        print ::OUT "double \L$::xpartName[$i]_offset;\n";
        print ::OUT "double \L$::xpartName[$i]_K;\n";
        print ::OUT "double \L$::xpartName[$i]_P;\n";
        print ::OUT "double \L$::xpartName[$i]_Z;\n";
        print ::OUT "double \L$::xpartName[$i]_K_tramp;\n";
        print ::OUT "double \L$::xpartName[$i]_P_tramp;\n";
        print ::OUT "double \L$::xpartName[$i]_Z_tramp;\n";
        print ::OUT "double \L$::xpartName[$i];\n";
        print ::OUT "double \L$::xpartName[$i]_val;\n";
}

sub frontEndInitCode {
        my ($i) = @_;
        my $calcExp = "\L$::xpartName[$i]_offset = 0.0;\n";
        $calcExp .= "\L$::xpartName[$i]_K = 0.0;\n";
        $calcExp .= "\L$::xpartName[$i]_P = 0.0;\n";
        $calcExp .= "\L$::xpartName[$i]_Z = 0.0;\n";
        $calcExp .= "\L$::xpartName[$i]_K_tramp = 0.0;\n";
        $calcExp .= "\L$::xpartName[$i]_P_tramp = 0.0;\n";
        $calcExp .= "\L$::xpartName[$i]_Z_tramp = 0.0;\n";
        $calcExp .= "\L$::xpartName[$i] = 0.0;\n";
        $calcExp .= "\L$::xpartName[$i]_val = 0.0;\n";
        return $calcExp;
}

# Figure out part input code
# Argument 1 is the part number
# Argument 2 is the input number
# Returns calculated input code
sub fromExp {
        return "";
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	my ($i) = @_;
        my $calcExp = "// Input Filter:  $::xpartName[$i]\n";
	my $input = "$::fromExp[0]";

# inline void inputFilterModule(double in, double &old_out, double &old_val, double offset, double k, double p, double z)
#
        $calcExp .= "inputFilterModule($input, &\L$::xpartName[$i], &\L$::xpartName[$i]_val, \L$::xpartName[$i]_offset, \L$::xpartName[$i]_K, \L$::xpartName[$i]_P, \L$::xpartName[$i]_Z);";
        return $calcExp . "\n";
}
