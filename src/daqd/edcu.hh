#ifndef EDCU_HH
#define EDCU_HH

/// Epics data Collection 
class edcu {
public:
  edcu (int a) : fidx (0), num_chans (0), running (0),
		 con_chans (0), con_events(0), val_events(0) {
	for (int i = 0; i < MAX_CHANNELS; i++) {
#ifdef USE_BROADCAST
	  channel_status[i] = 0; ///< For now set the status to good
#else
	  channel_status[i] = 0xbad;
#endif
	  channel_value[i] = 0.0;
	}
  };
  void *edcu_main ();
  static void *edcu_static (void *a) { return ((edcu *)a) -> edcu_main ();};

  unsigned int num_chans; ///< Number of epics channels configured
  unsigned int fidx; ///< Index to the first epics channel in daqd.channels[]
  unsigned int con_chans; ///< Number of channels connected
  unsigned int con_events; ///< Counter of all connection/disconnection events processed
  unsigned int val_events; ///< Counter of all value change events processed
  pthread_t tid;
  bool running;

  /// Producer picks up 32 bit data from here (IMPORTANT: do not use memcpy()!)
  /// Epics connect/disconnect callback writing in this array
  unsigned int channel_status[MAX_CHANNELS];
  /// Epics value change callback is writing into this array
  float channel_value[MAX_CHANNELS];
};

#endif
