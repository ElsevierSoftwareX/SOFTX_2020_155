package org.csstudio.archive.reader.nds;

public class TConvert {
	// This is to convert to the LIGO GPS seconds
	// TODO: this needs adjustment and refinement
	public static final long offs = 315963993;
	public static long gps(long epoch_seconds) {
		return epoch_seconds - offs;
	}
	public static long unix(long gps_seconds) {
		return gps_seconds + offs;
	}

}
