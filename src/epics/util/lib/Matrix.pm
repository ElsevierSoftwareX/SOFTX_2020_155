package CDS::Matrix;
use Exporter;
@ISA = ('Exporter');

#//     \page Matrix Matrix.pm
#//     Documentation for Matrix.pm
#//
#// \n


sub partType {
	return Matrix;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;

        for (0 .. $::partOutCnt[$i]-1) {
           $::portUsed[$_] = 0;
        }
        $::matOuts[$i] = 0;
        for (0 .. $::partOutCnt[$i]-1) {
          my $fromPort = $::partOutputPortUsed[$i][$_];
          if ($::portUsed[$fromPort] == 0) {
            $::portUsed[$fromPort] = 1;
            $::matOuts[$i] ++;
          }
        }
        #print "$::xpartName[$i] has $::matOuts[$i] Outputs\n";
        print ::EPICS "MATRIX $::xpartName[$i]_ $::matOuts[$i]x$::partInCnt[$i] $::systemName\.$::xpartName[$i]\n";
        print ::OUTH "\tdouble $::xpartName[$i]\[$::matOuts[$i]\]\[$::partInCnt[$i]\];\n";
}

# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {
        my ($i) = @_;
	; # already printed above
}


# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;
        print ::OUT "double \L$::xpartName[$i]\[$::matOuts[$i]\]\[$::partInCnt[$i]\];\n";
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
        return "\L$::xpartName[$from]" . "\[1\]\[" . $fromPort . "\]";
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	my ($i) = @_;
        my $calcExp = "// Matrix:  $::xpartName[$i]\n";
        $calcExp .= "for(ii=0;ii<$::matOuts[$i];ii++)\n{\n";
        $calcExp .= "\L$::xpartName[$i]\[1\]\[ii\] = \n";

        for (0 .. $::partInCnt[$i] - 1) {
          $calcExp .= "\tpLocalEpics->$::systemName\.";
          $calcExp .= $::xpartName[$i];
          $calcExp .= "\[ii\]\[";
          $calcExp .= $_;
          $calcExp .= "\] * ";
          $calcExp .= $::fromExp[$_];
          if ($_ == ($::partInCnt[$i] - 1)) { $calcExp .= ";\n";}
          else { $calcExp .= " +\n"; }
	}
        return $calcExp . "}\n";
}
