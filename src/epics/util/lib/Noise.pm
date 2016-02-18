package CDS::Noise;
use Exporter;
@ISA = ('Exporter');

#//     \page Noise Noise.pm
#//     Documentation for Noise.pm
#//
#// \n


$printed = 0;
$init_code_printed = 0;
1;

sub partType {
	return Noise;
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
	print ::OUT "static double \L$::xpartName[$i];\n";
	if ($printed) { return; }
	print ::OUT << "END";
static unsigned long noise_seed = 4101842887655102017LL;\n
static unsigned long noise_u, noise_v, noise_w;
inline unsigned long noise_int64() {
   noise_u = noise_u * 2862933555777941757LL + 7046029254386353087LL;
   noise_v ^= noise_v >> 17; noise_v ^= noise_v << 31; noise_v ^= noise_v >> 8;
   noise_w = 4294957665U * (noise_w & 0xffffffff) + (noise_w >> 32);
   unsigned long noise_x = noise_u ^ (noise_u << 21);
   noise_x ^= noise_x >> 35; noise_x ^= noise_x <<4;
   return (noise_x + noise_v) ^ noise_w;
}
inline double noise_doub() { return 5.42101086242752217E-20 * noise_int64(); }
inline noise_ran(unsigned long j) {
   noise_v = 4101842887655102017LL;
   noise_w = 1;
   noise_u = j ^ noise_v; noise_int64();
   noise_v = noise_u; noise_int64();
   noise_w = noise_v; noise_int64();
}
END
$printed = 1;
}

# Check inputs are connected
sub checkInputConnect {
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
        return "\L$::xpartName[$from]";
}

# Return front end initialization code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndInitCode {
	my ($i) = @_;
	if ($init_code_printed) { return ""; }
        my $calcExp  =  "\L$::xpartName[$i] = 0;\n";
	$calcExp    .=  "for (;noise_seed == 4101842887655102017LL;) {\n";
        $calcExp    .=  "   rdtscl(noise_seed);\n";
        $calcExp    .=  "}\n";
	$init_code_printed = 1;
	return $calcExp;
}


# Return front end code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndCode {
	my ($i) = @_;
        my $calcExp = "// Noise\n";
        $calcExp .= "\L$::xpartName[$i] = noise_doub();\n";
}
