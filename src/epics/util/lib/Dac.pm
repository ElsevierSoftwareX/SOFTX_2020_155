package CDS::Dac;
use Exporter;
@ISA = ('Exporter');

#//     \file Dac.dox
#//     \brief Documentation for Dac.pm
#//
#// \n
#//     \subpage devguidercg2 "<<-- Parts Library"
#// \n

# DAC cards we support
%board_types = (
	GSC_16AO16 => 1, # General Standards board
        GSC_18AO8 => 1 # 18-bit General Standards DAC board
);
$default_board_type = "GSC_16AO16";


# See Parser3.pm function sortDacs(), where the information in global arrays is sorted
# if there is a new global array introduced, it will need to be addressed in sortDacs()
#
sub initDac {
        my ($node) = @_;
        $::dacPartNum[$::dacCnt] = $::partCnt;
        for (0 .. 15) {
          $::partInput[$::partCnt][$_] = "NC";
        }

	my $desc = ${$node->{FIELDS}}{"Description"};
	printf "DAC PART TYPE description `$desc'\n";
           my ($type) = $desc =~ m/type=([^,]+)/g;
	printf "DAC PART TYPE $type\n";
	my ($num) = $desc =~ m/card_num=([^,]+)/g;
	if ($type eq undef) {
		$type = $default_board_type;
	}
	if ($num eq undef) {
		$num = $::dacCnt;
	}
	print "DAC $::dacCnt; type=$type; num=$num\n";
	
	# Check if this is a supported board type
	if ($board_types{$type} != 1) {
		print "Unsupported board type\n";
		print "Known board types:\n";
		foreach (keys %board_types) {
			print "\t$_\n";
		}
		exit 1;
	}

        $::dacType[$::dacCnt] = $type;
        $::dacNum[$::dacCnt] = $num;
	$::card2array[$::partCnt] = $::dacCnt;
        $::dacCnt++;
}


sub partType {
	return Dac;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
	;
}

# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {
        my ($i) = @_;
        ; 
}


# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;
        ;
}

# Return front end initialization code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndInitCode {
	my ($i) = @_;
	my $dacNum = $::card2array[$i];
        my $calcExp = "// DAC number is $dacNum\n";
        for (0 .. 15) {
          my $fromType = $::partInputType[$i][$_];
          if (($fromType ne "GROUND") && ($::partInput[$i][$_] ne "NC")) {
                $calcExp .= "dacOutUsed\[";
                $calcExp .= $dacNum;
                $calcExp .= "\]\[";
                $calcExp .= $_;
                $calcExp .= "\] =  1;\n";
          }
        }
	return $calcExp;
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

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	my ($i) = @_;
	my $dacNum = $::card2array[$i];
        my $calcExp = "// DAC number is $dacNum\n";
        for (0 .. 15) {
          my $fromType = $::partInputType[$i][$_];
          if (($fromType ne "GROUND") && ($::partInput[$i][$_] ne "NC")) {
                $calcExp .= "dacOut\[";
                $calcExp .= $dacNum;
                $calcExp .= "\]\[";
                # $calcExp .= $::partOutputPort[$i][$_];
                $calcExp .= $_;
                $calcExp .= "\] = ";
                $calcExp .= $::fromExp[$_];
                $calcExp .= ";\n";
          }
        }
	return $calcExp;
}
