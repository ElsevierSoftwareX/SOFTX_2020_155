package CDS::Dac;
use Exporter;
@ISA = ('Exporter');

sub partType {
        $::dacPartNum[$::dacCnt] = $::partCnt;
        for (0 .. 15) {
          $::partInput[$::partCnt][$_] = "NC";
        }
        $::dacCnt++;

	return DAC;
}
