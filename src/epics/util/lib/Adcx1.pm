package CDS::Adcx1;
use Exporter;
@ISA = ('Exporter');

# ADC cards we support
%board_types = (
	GSC_16AI64SSA => 1, # Slow General Standards board
        GSC_18AISS6C => 1 # 18-bit 6 channel General Standards board
);

# default board type (if none specified with type=<type> in block Description)
$default_board_type = "GSC_16AI64SSA";

sub initAdc {
        my ($node) = @_;
        $::adcPartNum[$::adcCnt] = $::partCnt;
	# Set ADC type and number
	my $desc = ${$node->{FIELDS}}{"Description"};
	my ($type) = $desc =~ m/type=([^,]+)/g;
	my ($num) = $desc =~ m/card_num=([^,]+)/g;
	if ($type eq undef) {
		$type = $default_board_type;
	}
	if ($num eq undef) {
		$num = $::adcCnt;
	}
	print "ADC $::adcCnt; type='$type'; num=$num\n";
        #print "foo=$board_types{$type}\n";
	
	# Check if this is a supported board type
	if ($board_types{$type} != 1) {
		print "Unsupported board type\n";
		print "Known board types:\n";
		foreach (keys %board_types) {
			print "\t$_\n";
		}
		exit 1;
	}

        $::adcType[$::adcCnt] = $type;
        $::adcNum[$::adcCnt] = $num;
        $::adcCnt++;
        $::partUsed[$::partCnt] = 1;
        foreach (0 .. $::partCnt) {
          if ("Adc" eq $::partInputType[$_][0]) {
	  	print $_," ", $::xpartName[$_], "\n";
	  }
	}
}

sub partType {
	return Adc;
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
        my $calcExp = "// ADC $i From adcx1\n";
	#print $calcExp, "\n";
	%seen = ();
        foreach (0 .. $::partCnt) {
	  foreach  $inp (0 .. $::partInCnt[$_]) {
            if ("Adc" eq $::partInputType[$_][$inp] && $i == $::partInNum[$_][$inp]) {
	  	#print $_," ", $::xpartName[$_], " ", $::partInputPort[$_][$inp], "\n";
		$seen{$::partInputPort[$_][$inp]}=1;
	    }
	  }
	}
	foreach (sort { $a <=> $b }  keys %seen) {
	#	print $_, ",";
        	$calcExp .= "dWordUsed\[";
        	$calcExp .= $i;
        	$calcExp .= "\]\[";
        	$calcExp .= $_;
        	$calcExp .= "\] =  1;\n";
	}
	#print "\n";
        return $calcExp;
}

# Figure out part input code
# Argument 1 is the part number
# Argument 2 is the input number
# Returns calculated input code
sub fromExp {
        my ($i, $j) = @_;
        my $card = $::partInNum[$i][$j];
        my $chan = $::partInputPort[$i][$j];
        return "dWord\[" . $card . "\]\[" . $chan . "\]";
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	my ($i) = @_;
        return "";
}
