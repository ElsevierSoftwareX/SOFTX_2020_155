package CDS::OscFixedPhase;
use Exporter;
@ISA = ('Exporter');

#//     \page Osc OscFixedPhase.pm
#//     Documentation for OscFixedPhase.pm
#//
#// \n


sub partType {
	return OscFixedPhase;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
        print ::OUTH "\tdouble $::xpartName[$i]\_FREQ;\n";
        print ::OUTH "\tdouble $::xpartName[$i]\_CLKGAIN;\n";
        print ::OUTH "\tdouble $::xpartName[$i]\_SINGAIN;\n";
        print ::OUTH "\tdouble $::xpartName[$i]\_COSGAIN;\n";
	print ::OUTH "\tdouble $::xpartName[$i]\_TRAMP;\n";
$here = <<END;
\tchar $::xpartName[$i]\_FREQ_mask;\n
\tchar $::xpartName[$i]\_CLKGAIN_mask;\n
\tchar $::xpartName[$i]\_SINGAIN_mask;\n
\tchar $::xpartName[$i]\_COSGAIN_mask;\n
\tchar $::xpartName[$i]\_TRAMP_mask;\n
END
	return $here;
}

# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {
        my ($i) = @_;
	print ::EPICS "INVARIABLE $::xpartName[$i]\_FREQ $::systemName\.$::xpartName[$i]\_FREQ double ai 0 field(PREC,\"1\")\n";
        print ::EPICS "INVARIABLE $::xpartName[$i]\_CLKGAIN $::systemName\.$::xpartName[$i]\_CLKGAIN double ai 0 field(PREC,\"1\")\n";
        print ::EPICS "INVARIABLE $::xpartName[$i]\_SINGAIN $::systemName\.$::xpartName[$i]\_SINGAIN double ai 0 field(PREC,\"1\")\n";
        print ::EPICS "INVARIABLE $::xpartName[$i]\_COSGAIN $::systemName\.$::xpartName[$i]\_COSGAIN double ai 0 field(PREC,\"1\")\n";
	print ::EPICS "INVARIABLE $::xpartName[$i]\_TRAMP $::systemName\.$::xpartName[$i]\_TRAMP double ai 0 field(PREC,\"1\")\n";
}


# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;
        print ::OUT "static double \L$::xpartName[$i]\[3\];\n";
        print ::OUT "static double \L$::xpartName[$i]\_freq;\n";
        print ::OUT "static double \L$::xpartName[$i]\_delta;\n";
        print ::OUT "static double \L$::xpartName[$i]\_alpha;\n";
        print ::OUT "static double \L$::xpartName[$i]\_beta;\n";
        print ::OUT "static double \L$::xpartName[$i]\_cos_prev;\n";
        print ::OUT "static double \L$::xpartName[$i]\_sin_prev;\n";
        print ::OUT "static double \L$::xpartName[$i]\_cos_new;\n";
        print ::OUT "static double \L$::xpartName[$i]\_sin_new;\n";
	print ::OUT "static RampParamState \L$::xpartName[$i]\_clkgain_state;\n";
	print ::OUT "static RampParamState \L$::xpartName[$i]\_singain_state;\n";
	print ::OUT "static RampParamState \L$::xpartName[$i]\_cosgain_state;\n";
	print ::OUT "static double \L$::xpartName[$i]\_freq_cycle_count;\n";
        print ::OUT "static double \L$::xpartName[$i]\_delta_freq;\n";
	print ::OUT "static double \L$::xpartName[$i]\_freq_request;\n";
	print ::OUT "static double \L$::xpartName[$i]\_freq_request_limited;\n";
	print ::OUT "static double \L$::xpartName[$i]\_extra_phase_shift_per_cycle;\n";
	print ::OUT "static int \L$::xpartName[$i]\_freq_max_count;\n";
	print ::OUT "static double \L$::xpartName[$i]\_temp;\n";
        if ($::oscUsed == 0) {
          print ::OUT "double lsinx, lcosx, valx;\n";
          $::oscUsed = 1;
        }
}

sub checkInputConnect {
        my ($i) = @_;
        if($::partInCnt[$i] < 1) {
                print ::CONN_ERRORS "***\n$::partType[$i] with name $::xpartName[$i] has missing inputs\nRrequires 1; Only $::partInCnt[$i] provided:  \n";
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
        my $from = $::partInNum[$i][$j];
        my $fromPort = $::partInputPort[$i][$j];
        return "\L$::xpartName[$from]" . "\[" . $fromPort . "\]";
}

# Return front end initialization code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndInitCode {
	my ($i) = @_;
        my $calcExp =  "\L$::xpartName[$i]_freq = ";
        $calcExp .=  "pLocalEpics->$::systemName\.$::xpartName[$i]\_FREQ;\n";
        $calcExp .= "\L$::xpartName[$i]\_delta = \UM_TWO_PI * ";
        $calcExp .=  "\L$::xpartName[$i]_freq / \UFE_RATE;\n";
        $calcExp .= "valx = \L$::xpartName[$i]\_delta \/ 2.0;\n";
        $calcExp .= "sincos\(valx, \&lsinx, \&lcosx\);\n";
        $calcExp .= "\L$::xpartName[$i]\_alpha = 2.0 * lsinx * lsinx;\n";
        $calcExp .= "valx = \L$::xpartName[$i]\_delta\;\n";
        $calcExp .= "sincos\(valx, \&lsinx, \&lcosx\);\n";
        $calcExp .= "\L$::xpartName[$i]\_beta = lsinx;\n";
        $calcExp .= "\L$::xpartName[$i]\_cos_prev = 1.0;\n";
        $calcExp .= "\L$::xpartName[$i]\_sin_prev = 0.0;\n";
	$calcExp .= "RampParamInit(&\L$::xpartName[$i]\_clkgain_state,0, ";
	$calcExp .= "FE_RATE);\n";
	$calcExp .= "RampParamInit(&\L$::xpartName[$i]\_singain_state,0, ";
	$calcExp .= "FE_RATE);\n";
	$calcExp .= "RampParamInit(&\L$::xpartName[$i]\_cosgain_state,0, ";
	$calcExp .= "FE_RATE);\n";
	$calcExp .= "\L$::xpartName[$i]_freq_request = 0.0;\n";
	$calcExp .= "\L$::xpartName[$i]_freq_request_limited = 0;\n";
	$calcExp .= "\L$::xpartName[$i]\_freq_cycle_count = 0;\n";
	$calcExp .= "\L$::xpartName[$i]\_delta_freq = 0;\n";
	$calcExp .= "\L$::xpartName[$i]\_extra_phase_shift_per_cycle = 0;\n";
	$calcExp .= "\L$::xpartName[$i]\_freq_max_count = 0;\n";
	$calcExp .= "\L$::xpartName[$i]\_temp = 0;\n";
	return $calcExp;
}


# Return front end code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndCode {
	my ($i) = @_;
        my $calcExp = "// OscFixedPhase:  $::xpartName[$i]\n";

	$calcExp .= "if (\L$::xpartName[$i]\_clkgain_state.req != ";
	$calcExp .= "pLocalEpics->$::systemName\.$::xpartName[$i]\_CLKGAIN)\n";
	$calcExp .= "{\n";
	$calcExp .= "   RampParamLoad(&\L$::xpartName[$i]\_clkgain_state, ";
	$calcExp .= "pLocalEpics->$::systemName\.$::xpartName[$i]\_CLKGAIN, pLocalEpics->$::systemName\.$::xpartName[$i]\_TRAMP,FE_RATE);\n";
	$calcExp .="}\n";

	$calcExp .= "if (\L$::xpartName[$i]\_singain_state.req != ";
	$calcExp .= "pLocalEpics->$::systemName\.$::xpartName[$i]\_SINGAIN)\n";
        $calcExp .= "{\n";
        $calcExp .= "   RampParamLoad(&\L$::xpartName[$i]\_singain_state, ";
	$calcExp .= "pLocalEpics->$::systemName\.$::xpartName[$i]\_SINGAIN, pLocalEpics->$::systemName\.$::xpartName[$i]\_TRAMP,FE_RATE);\n";
        $calcExp .="}\n";

	$calcExp .= "if (\L$::xpartName[$i]\_cosgain_state.req != ";
	$calcExp .= "pLocalEpics->$::systemName\.$::xpartName[$i]\_COSGAIN)\n";
        $calcExp .= "{\n";
        $calcExp .= "   RampParamLoad(&\L$::xpartName[$i]\_cosgain_state, ";
	$calcExp .= "pLocalEpics->$::systemName\.$::xpartName[$i]\_COSGAIN, pLocalEpics->$::systemName\.$::xpartName[$i]\_TRAMP,FE_RATE);\n";
        $calcExp .= "}\n";

	#Initialization of the frequency change
        $calcExp .= "if(\L$::xpartName[$i]_freq_request \!= ";
        $calcExp .= "pLocalEpics->$::systemName\.$::xpartName[$i]\_FREQ)\n";
        $calcExp .= "{\n";

	$calcExp .= "\tif (pLocalEpics->$::systemName\.$::xpartName[$i]\_TRAMP > (1.0/FE_RATE))\n";
	$calcExp .= "\t{\n";

	$calcExp .= "\t\t\L$::xpartName[$i]_freq_request = ";
        $calcExp .= "pLocalEpics->$::systemName\.$::xpartName[$i]\_FREQ;\n";
	$calcExp .= "\t\t\L$::xpartName[$i]_freq_request_limited = ";
	$calcExp .= "((int) (\L$::xpartName[$i]_freq_request * 1000)) / 1000.0;\n";
	$calcExp .= "\t\t\L$::xpartName[$i]_freq_cycle_count = 1;\n";
	#Max cycle count and delta frequency
	$calcExp .= "\t\t\L$::xpartName[$i]_freq_max_count = ((double) \UFE_RATE) * ";
	$calcExp .= "pLocalEpics->$::systemName\.$::xpartName[$i]\_TRAMP;\n";
	$calcExp .= "\t\t\L$::xpartName[$i]_delta_freq = (\L$::xpartName[$i]_freq_request_limited - \L$::xpartName[$i]_freq)/((double) \L$::xpartName[$i]_freq_max_count);\n";
	#Extra phase shift per cycle to achieve correct phase at the end of the ramp
        #Calculate 
	#Calculate cycles shift due to changing frequency over ramp time plus current frequency effect
	$longCalculation = <<END;
\t\t\L$::xpartName[$i]\E_temp = cycle_gps_time % 1000;
\t\t\L$::xpartName[$i]\E_temp = (\L$::xpartName[$i]\E_temp + pLocalEpics->$::systemName\.$::xpartName[$i]\_TRAMP + (((double) cycleNum) / ((double) FE_RATE))) * \L$::xpartName[$i]_freq_request_limited;
\t\t\L$::xpartName[$i]_temp = \L$::xpartName[$i]_temp - (int) \L$::xpartName[$i]_temp;
\t\t\L$::xpartName[$i]_extra_phase_shift_per_cycle = \L$::xpartName[$i]_temp;
\t\t\L$::xpartName[$i]_temp =  (\L$::xpartName[$i]_delta_freq * (((double) \L$::xpartName[$i]_freq_max_count) * 
\t\t(((double) \L$::xpartName[$i]_freq_max_count) + 1.0) / 2.0 ) + ((double) \L$::xpartName[$i]_freq_max_count) * \L$::xpartName[$i]_freq)/((double) \UFE_RATE);
\t\t\L$::xpartName[$i]_extra_phase_shift_per_cycle -= \L$::xpartName[$i]_temp;
\t\t\L$::xpartName[$i]_temp = latan2(\L$::xpartName[$i]\_sin_prev, 1.0 + \L$::xpartName[$i]\_cos_prev) / \UM_PI;
\t\t\L$::xpartName[$i]_extra_phase_shift_per_cycle -= \L$::xpartName[$i]_temp;
\t\t\L$::xpartName[$i]_extra_phase_shift_per_cycle = \UM_TWO_PI * (\L$::xpartName[$i]_extra_phase_shift_per_cycle - \
\t\t(int) \L$::xpartName[$i]_extra_phase_shift_per_cycle) / \L$::xpartName[$i]_freq_max_count;
END
	$calcExp .= $longCalculation;
	$calcExp .= "\t} else\n";
	$calcExp .= "\t{\n";
	$calcExp .= "\t\t\L$::xpartName[$i]_freq_request = ";
	$calcExp .= "pLocalEpics->$::systemName\.$::xpartName[$i]\_FREQ;\n";
	$calcExp .= "\t\t\L$::xpartName[$i]_freq_request_limited = ";
        $calcExp .= "((int) (\L$::xpartName[$i]_freq_request * 1000)) / 1000.0;\n";
	$calcExp .= "\t\t\L$::xpartName[$i]_freq_cycle_count = 1;\n";
	$calcExp .= "\t\t\L$::xpartName[$i]_freq_max_count = 0;\n";
	$calcExp .= "\t\t\L$::xpartName[$i]\E_temp = cycle_gps_time % 1000;\n";
	$calcExp .= "\t\t\L$::xpartName[$i]\E_temp = (\L$::xpartName[$i]\E_temp + (((double) cycleNum) / ((double) FE_RATE))) * \L$::xpartName[$i]_freq_request_limited;\n";
	$calcExp .= "\t\tvalx = \UM_TWO_PI * \(\L$::xpartName[$i]_temp - \(int\) \L$::xpartName[$i]_temp\);\n";
	$calcExp .= "\t\tsincos\(valx, \&lsinx,\&lcosx\);\n";
	$calcExp .= "\t\t\L$::xpartName[$i]\_cos_prev = lcosx;\n";
	$calcExp .= "\t\t\L$::xpartName[$i]\_sin_prev = lsinx;\n";
	$calcExp .= "\t}\n";
	$calcExp .= "}\n";
	
	#Execute frequency change
	$calcExp .= "if(\L$::xpartName[$i]_freq_cycle_count != 0)\n";
	$calcExp .= "{\n";

	$calcExp .= "\tif(\L$::xpartName[$i]_freq_cycle_count > \L$::xpartName[$i]_freq_max_count)\n";
	$calcExp .= "\t{\n";
	$calcExp .= "\t\t\L$::xpartName[$i]_freq_cycle_count = 0;\n";
	$calcExp .= "\t\t\L$::xpartName[$i]\_freq = \L$::xpartName[$i]\_freq_request_limited;\n";
	$calcExp .= "\t\t\L$::xpartName[$i]\_delta = \UM_TWO_PI * ";
	$calcExp .= "\L$::xpartName[$i]_freq / \UFE_RATE;\n";
	
	$calcExp .= "\t} else {\n";
	$calcExp .= "\t\t\L$::xpartName[$i]\_freq_cycle_count += 1;\n";
	$calcExp .= "\t\t\L$::xpartName[$i]\_freq += \L$::xpartName[$i]\_delta_freq;\n";

        $calcExp .= "\t\t\L$::xpartName[$i]\_delta = (\UM_TWO_PI * ";
        $calcExp .= "\L$::xpartName[$i]_freq / \UFE_RATE) + ";
	$calcExp .= " \L$::xpartName[$i]_extra_phase_shift_per_cycle;\n";
	$calcExp .= "\t}\n";

        $calcExp .= "\tvalx = \L$::xpartName[$i]\_delta \/ 2.0;\n";
        $calcExp .= "\tsincos\(valx, \&lsinx, \&lcosx\);\n";
        $calcExp .= "\t\L$::xpartName[$i]\_alpha = 2.0 * lsinx * lsinx;\n";
        $calcExp .= "\tvalx = \L$::xpartName[$i]\_delta\;\n";
        $calcExp .= "\tsincos\(valx, \&lsinx, \&lcosx\);\n";
        $calcExp .= "\t\L$::xpartName[$i]\_beta = lsinx;\n";
	$calcExp .= "}\n";


	#Calculate new cos and sin values
	$calcExp .= "\L$::xpartName[$i]\_cos_new = \(1.0 - ";
        $calcExp .= "\L$::xpartName[$i]\_alpha\) * \L$::xpartName[$i]\_cos_prev - ";
        $calcExp .= "\L$::xpartName[$i]\_beta * \L$::xpartName[$i]\_sin_prev;\n";
        $calcExp .= "\L$::xpartName[$i]\_sin_new = \(1.0 - ";
        $calcExp .= "\L$::xpartName[$i]\_alpha\) * \L$::xpartName[$i]\_sin_prev + ";
        $calcExp .= "\L$::xpartName[$i]\_beta * \L$::xpartName[$i]\_cos_prev;\n";
        $calcExp .= "\L$::xpartName[$i]\_sin_prev = \L$::xpartName[$i]\_sin_new;\n";
        $calcExp .= "\L$::xpartName[$i]\_cos_prev = \L$::xpartName[$i]\_cos_new;\n";



	#Write out Oscillation Sin/Cos/Clk
        $calcExp .= "\L$::xpartName[$i]\[0\] = \L$::xpartName[$i]\_sin_new * ";
        $calcExp .= "RampParamUpdate(&\L$::xpartName[$i]\_clkgain_state);\n";
        $calcExp .= "\L$::xpartName[$i]\[1\] = \L$::xpartName[$i]\_sin_new * ";
        $calcExp .= "RampParamUpdate(&\L$::xpartName[$i]\_singain_state);\n";
        $calcExp .= "\L$::xpartName[$i]\[2\] = \L$::xpartName[$i]\_cos_new * ";
        $calcExp .= "RampParamUpdate(&\L$::xpartName[$i]\_cosgain_state);\n";
        
	return $calcExp;
}
