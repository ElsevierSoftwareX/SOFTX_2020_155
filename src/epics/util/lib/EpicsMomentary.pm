package CDS::EpicsMomentary;
use Exporter;
@ISA = ('Exporter');

#//     \page EpicsMomentary EpicsMomentary.pm
#//     Documentation for EpicsMomentary.pm
#//
#// \n

sub partType {
        return EpicsMomentary;
}
 
# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
        print ::OUTH "\tint $::xpartName[$i];\n";
}
 
# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {
        my ($i) = @_;
        print ::EPICS "MOMENTARY $::xpartName[$i] $::systemName\.$::xpartName[$i] int ao 0\n";
}
 
# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;
        print ::OUT "static int \L$::xpartName[$i];\n";
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
        return "\L$::xpartName[$i] = 0;\n";
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
        my $calcExp = "// EpicsMomentary:  $::xpartName[$i]\n";
        $calcExp .= "\t\L$::xpartName[$i] = ";
        $calcExp .= "pLocalEpics->$::systemName\.$::xpartName[$i];\n";
        $calcExp .= "\tpLocalEpics->$::systemName\.$::xpartName[$i] = 0;\n";
        return $calcExp;
}

