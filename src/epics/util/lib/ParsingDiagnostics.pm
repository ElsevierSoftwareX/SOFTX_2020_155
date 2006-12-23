package CDS::ParsingDiagnostics;
use Exporter;
@ISA = ('Exporter');

sub print_diagnostics {
    my ($fname) = @_;
    open(OUT,">".$fname) || die "cannot open parse diagnostics file $fname for writing\n";

    print OUT "systemName=$::systemName\n";
    print OUT "adcCnt=$::adcCnt\n";
    print OUT "nonSubCnt=$::nonSubCnt\n";
    for (0 .. $::nonSubCnt-1) {
	print OUT "nonSubPart[$_]=$::nonSubPart[$_]\n";
    }
    print OUT "partCnt=$::partCnt\n";
    #for (0 .. $::partCnt-1) {
	#print OUT "cdsPart[$_]=$::cdsPart[$_]\n";
    #}
    for (0 .. $::partCnt-1) {
      print OUT "partInCnt[$_]=$::partInCnt[$_]\n";
      foreach $pin (0 .. $::partInCnt[$_]-1) {
	print OUT "partInput[$_][$pin]=$::partInput[$_][$pin]\n";
	print OUT "partInputPort[$_][$pin]=$::partInputPort[$_][$pin]\n";
      }
      print OUT "partInputs[$_]=$::partInputs[$_]\n";
    }

    for (0 .. $::partCnt-1) {
      print OUT "partOutCnt[$_]=$::partOutCnt[$_]\n";
      foreach $pin (0 .. $::partOutCnt[$_]-1) {
	print OUT "partOutput[$_][$pin]=$::partOutput[$_][$pin]\n";
	print OUT "partOutputPort[$_][$pin]=$::partOutputPort[$_][$pin]\n";
	print OUT "partOutputPortUsed[$_][$pin]=$::partOutputPortUsed[$_][$pin]\n";
      }
    }
    for (0 .. $::partCnt-1) {
      print OUT "xpartName[$_]=$::xpartName[$_]\n";
      print OUT "partName[$_]=$::partName[$_]\n";
      print OUT "partType[$_]=$::partType[$_]\n";
      print OUT "partSubName[$_]=$::partSubName[$_]\n";
      print OUT "partSubNum[$_]=$::partSubNum[$_]\n";
    }
    print OUT "subSys=$::subSys\n";
    for (0 .. $::subSys-1) {
      print OUT "subSysName[$_]=$::subSysName[$_]\n";
      print OUT "subSysPartStart[$_]=$::subSysPartStart[$_]\n";
      print OUT "subSysPartStop[$_]=$::subSysPartStop[$_]\n";
    }
}
