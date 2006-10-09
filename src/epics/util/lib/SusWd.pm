package CDS::SusWd;
use Exporter;
@ISA = ('Exporter');

sub partType {
	return SusWd;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
        print ::OUTH "\tint $::xpartName[$i];\n";
        print ::OUTH "\tint $::xpartName[$i]\_MAX;\n";
        print ::OUTH "\tfloat $::xpartName[$i]\_VAR\[20\];\n";
}

# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {
        my ($i) = @_;
        print ::EPICS "OUTVARIABLE $::xpartName[$i] $::systemName\.$::xpartName[$i] int ai 0 \n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_VAR_M0_F1 $::systemName\.$::xpartName[$i]\_VAR\[0\] float ai 0 field(PREC,\"1\")\n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_VAR_M0_F2 $::systemName\.$::xpartName[$i]\_VAR\[1\] float ai 0 field(PREC,\"1\")\n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_VAR_M0_F3 $::systemName\.$::xpartName[$i]\_VAR\[2\] float ai 0 field(PREC,\"1\")\n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_VAR_M0_L $::systemName\.$::xpartName[$i]\_VAR\[3\] float ai 0 field(PREC,\"1\")\n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_VAR_M0_R $::systemName\.$::xpartName[$i]\_VAR\[4\] float ai 0 field(PREC,\"1\")\n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_VAR_M0_S $::systemName\.$::xpartName[$i]\_VAR\[5\] float ai 0 field(PREC,\"1\")\n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_VAR_R0_F1 $::systemName\.$::xpartName[$i]\_VAR\[6\] float ai 0 field(PREC,\"1\")\n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_VAR_R0_F2 $::systemName\.$::xpartName[$i]\_VAR\[7\] float ai 0 field(PREC,\"1\")\n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_VAR_R0_F3 $::systemName\.$::xpartName[$i]\_VAR\[8\] float ai 0 field(PREC,\"1\")\n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_VAR_R0_L $::systemName\.$::xpartName[$i]\_VAR\[9\] float ai 0 field(PREC,\"1\")\n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_VAR_R0_R $::systemName\.$::xpartName[$i]\_VAR\[10\] float ai 0 field(PREC,\"1\")\n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_VAR_R0_S $::systemName\.$::xpartName[$i]\_VAR\[11\] float ai 0 field(PREC,\"1\")\n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_VAR_L1_UL $::systemName\.$::xpartName[$i]\_VAR\[12\] float ai 0 field(PREC,\"1\")\n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_VAR_L1_LL $::systemName\.$::xpartName[$i]\_VAR\[13\] float ai 0 field(PREC,\"1\")\n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_VAR_L1_UR $::systemName\.$::xpartName[$i]\_VAR\[14\] float ai 0 field(PREC,\"1\")\n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_VAR_L1_LR $::systemName\.$::xpartName[$i]\_VAR\[15\] float ai 0 field(PREC,\"1\")\n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_VAR_L2_UL $::systemName\.$::xpartName[$i]\_VAR\[16\] float ai 0 field(PREC,\"1\")\n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_VAR_L2_LL $::systemName\.$::xpartName[$i]\_VAR\[17\] float ai 0 field(PREC,\"1\")\n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_VAR_L2_UR $::systemName\.$::xpartName[$i]\_VAR\[18\] float ai 0 field(PREC,\"1\")\n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_VAR_L2_LR $::systemName\.$::xpartName[$i]\_VAR\[19\] float ai 0 field(PREC,\"1\")\n";
        print ::EPICS "INVARIABLE $::xpartName[$i]\_MAX $::systemName\.$::xpartName[$i]\_MAX int ai 0 field(PREC,\"0\")\n";
}

# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;
        print ::OUT "float \L$::xpartName[$i];\n";
        print ::OUT "static float \L$::xpartName[$i]\_avg\[20\];\n";
        print ::OUT "static float \L$::xpartName[$i]\_var\[20\];\n";
        print ::OUT "float vabs;\n";
}

# Return front end initialization code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndInitCode {
        my ($i) = @_;
        return "for\(ii=0;ii<20;ii++\) {\n"
           . "\t\L$::xpartName[$i]\_avg\[ii\] = 0.0;\n"
           . "\t\L$::xpartName[$i]\_var\[ii\] = 0.0;\n"
           . "}\n";
}

# Figure out part input code
# Argument 1 is the part number
# Argument 2 is the input number
# Returns calculated input code
sub fromExp {
        my ($i, $j) = @_;
        return "";
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	my ($mm) = @_;
        my $calcExp = "// SusWd Module\n";
        $calcExp .= "if((cycle \% 16) == 0) {\n";
        $calcExp .= "\L$::xpartName[$mm] = 16384;\n";
        $calcExp .= "   for\(ii=0;ii<20;ii++\) {\n";
        $calcExp .= "\tif\(ii\<16\) jj = ii;\n";
        $calcExp .= "\telse jj = ii+2;\n";
        $calcExp .= "\t\L$::xpartName[$mm]\_avg\[ii\]";
        $calcExp .= " = dWord\[0\]\[jj\] * \.00005 + ";
        $calcExp .= "\L$::xpartName[$mm]\_avg\[ii\] * 0\.99995;\n";
        $calcExp .= "\tvabs = dWord\[0\]\[jj\] - \L$::xpartName[$mm]\_avg\[ii\];\n";
        $calcExp .= "\tif\(vabs < 0) vabs *= -1.0;\n";
        $calcExp .= "\t\L$::xpartName[$mm]\_var\[ii\] = vabs * \.00005 + ";
        $calcExp .= "\L$::xpartName[$mm]\_var\[ii\] * 0\.99995;\n";
        $calcExp .= "\tpLocalEpics->$::systemName\.";
        $calcExp .= $::xpartName[$mm];
        $calcExp .= "_VAR\[ii\] = ";
        $calcExp .= "\L$::xpartName[$mm]\_var\[ii\];\n";

        $calcExp .= "\tif(\L$::xpartName[$mm]\_var\[ii\] \> ";
        $calcExp .= "pLocalEpics->$::systemName\.";
        $calcExp .= $::xpartName[$mm];
        $calcExp .= "_MAX\) ";
        $calcExp .= "\L$::xpartName[$mm] = 0;\n";
        $calcExp .= "   }\n";
        $calcExp .= "\tpLocalEpics->$systemName\.";
        $calcExp .= $::xpartName[$mm];
        $calcExp .= " = \L$::xpartName[$mm] \/ 16384;\n";
        $calcExp .= "}\n";

        return $calcExp;
}
