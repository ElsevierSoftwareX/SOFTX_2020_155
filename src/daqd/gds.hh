#ifndef GDS_C_H
#define GDS_C_H

#include "channel.hh"
#if  defined(sun) || !defined(_ADVANCED_LIGO)
#include "gdsLib.h"
#endif
#if defined(COMPAT_INITIAL_LIGO)
#include "../../../rts/src/include/gdsLib.h"
#endif

class gds_c {
 private:
  int signal_p;
  pthread_mutex_t signal_mtx;
  pthread_cond_t signal_cv;

  char* construct_req_string (char *alias[], int nptr);

  // Scope locker
  pthread_mutex_t bm;
  void lock (void) {pthread_mutex_lock (&bm);}
  void unlock (void) {pthread_mutex_unlock (&bm);}
  class locker;
  friend class gds_c::locker;
  class locker {
    gds_c *dp;
  public:
    locker (gds_c *objp) {(dp = objp) -> lock ();}
    ~locker () {dp -> unlock ();}
  };

  // GDS server name, rpc program number and version
  char *gds_server;
  char *gds_server1;
  char *gds_server2;
  char *gds_server3;
  char *gds_server4;
  char *gds_server5;
  char *gds_server6;
  char *gds_server7;
  char *gds_server8;
  char *gds_server9;
  char *gds_server10;
  int gds_server_rpc_program;
  int gds_server_rpc_version;
  int n_gds_servers; // How many servers we have got

  static const int max_gds_servers = 11;

  // DCU id for each GDS server
  int dcuid[max_gds_servers];

public:
//#ifdef USE_GM
  //int gdsTpCounter[DCU_COUNT];
  //int gdsTpTable[DCU_COUNT][GM_DAQ_MAX_TPS];
//#endif

#ifdef DATA_CONCENTRATOR
  // Construct test point data block for broadcasting in concentrator
  char *build_tp_data(int *l, char *data, int bn);
#endif

  // Update test point tables with received broadcast
  void update_tp_data(unsigned int *d, char *dest);

 public:
  gds_c () : signal_p (0), gds_server (0), gds_server1 (0), gds_server2 (0),
	gds_server3 (0), gds_server4 (0), gds_server5 (0),
	gds_server6 (0), gds_server7 (0), gds_server8 (0),
	gds_server9 (0), gds_server10 (0),
	gds_server_rpc_program (0), gds_server_rpc_version (0), n_gds_servers(0) {
    pthread_mutex_init (&bm, NULL);
    pthread_mutex_init (&signal_mtx, NULL);
    pthread_cond_init (&signal_cv, NULL);
#ifdef _ADVANCED_LIGO
//#ifdef USE_GM
    //for (int i = 0; i < DCU_COUNT; i++) {
      //gdsTpCounter[i] = 0;
      //for (int j = 0; j < GM_DAQ_MAX_TPS; j++) {
        //gdsTpTable[i][j] = 0;
      //}
    //}
//#endif
#endif
    for (int i = 0; i < max_gds_servers; i++) dcuid[i] = 0;
  }
  ~gds_c () {
    pthread_mutex_destroy (&bm);
    pthread_mutex_destroy (&signal_mtx);
    pthread_cond_destroy (&signal_cv);
  }

  //
  // Gets the array of aliases `alias' for which
  // the request to GDS server is made.
  // Then it waits for the requested test point channels
  // to appear in the reflective memory and writes
  // to `gds[]' pointers to GDS channels that have
  // the data corresponding to each `alias[]' entry.
  // The caller must provide space sufficient to write
  // `nptr' elements into `gds[]'.
  // Returns -1 on error, 0 on success
  // Logs error into the error log
  //
#if 0
  int req_names (char *alias[], unsigned int *tpnum, channel_t *gds[], int nptr);
#endif
  int req_tps (long_channel_t *ac[], channel_t *gds[], int nptr);

  //
  // Sends channel clear request to the GDS server.
  // This disconnects named test points.
  // Return -1 on error, 0 on success
  // Logs error into error log.
  //
  int clear_names (char *alias[], int nptr);
  int clear_tps (long_channel_t *ac[], int nptr);

  // Set GDS server RPC connection attributes
  void
  set_gds_server (int dcu, char *server,
		  int dcu1, char *server1,
		  int dcu2, char *server2,
		  int dcu3, char *server3,
		  int dcu4, char *server4,
		  int dcu5, char *server5,
		  int dcu6, char *server6,
		  int dcu7, char *server7,
		  int dcu8, char *server8,
		  int dcu9, char *server9,
		  int dcu10, char *server10,
	          int program, int version) {
    if (server) {
      free (this -> gds_server);
      this -> gds_server = server;
      this -> dcuid[0] = dcu;
      this -> n_gds_servers = 1;
    }
    if (server1) {
      free (this -> gds_server1);
      this -> gds_server1 = server1;
      this -> dcuid[1] = dcu1;
      this -> n_gds_servers = 2;
    }
    if (server2) {
      free (this -> gds_server2);
      this -> gds_server2 = server2;
      this -> dcuid[2] = dcu2;
      this -> n_gds_servers = 3;
    }
    if (server3) {
      free (this -> gds_server3);
      this -> gds_server3 = server3;
      this -> dcuid[3] = dcu3;
      this -> n_gds_servers = 4;
    }
    if (server4) {
      free (this -> gds_server4);
      this -> gds_server4 = server4;
      this -> dcuid[4] = dcu4;
      this -> n_gds_servers = 5;
    }
    if (server5) {
      free (this -> gds_server5);
      this -> gds_server5 = server5;
      this -> dcuid[5] = dcu5;
      this -> n_gds_servers = 6;
    }
    if (server6) {
      free (this -> gds_server6);
      this -> gds_server6 = server6;
      this -> dcuid[6] = dcu6;
      this -> n_gds_servers = 7;
    }
    if (server7) {
      free (this -> gds_server7);
      this -> gds_server7 = server7;
      this -> dcuid[7] = dcu7;
      this -> n_gds_servers = 8;
    }
    if (server8) {
      free (this -> gds_server8);
      this -> gds_server8 = server8;
      this -> dcuid[8] = dcu8;
      this -> n_gds_servers = 9;
    }
    if (server9) {
      free (this -> gds_server9);
      this -> gds_server9 = server9;
      this -> dcuid[9] = dcu9;
      this -> n_gds_servers = 10;
    }
    if (server10) {
      free (this -> gds_server10);
      this -> gds_server10 = server10;
      this -> dcuid[10] = dcu10;
      this -> n_gds_servers = 11;
    }

    if (program >= 0)
      this -> gds_server_rpc_program = program;
    if (version >= 0)
      this -> gds_server_rpc_version = version;
  }

  // Initialize GDS testpoint library
  int gds_initialize ();

  // Notify about alias table change
  void signal () {
    pthread_mutex_lock (&signal_mtx);
    signal_p++;
    pthread_cond_signal (&signal_cv);
    pthread_mutex_unlock (&signal_mtx);
  }

#if  defined(sun) || defined(COMPAT_INITIAL_LIGO)
  // Translate test point number to RFM network (0 - 5565 or 1 - 5579)
  unsigned int tpnum_to_rmnet(unsigned int tpnum) {
    return IS_GDS_ETMX_TP(tpnum) || IS_GDS_ETMY_TP(tpnum)
      || IS_GDS_HEPIX_TP(tpnum) || IS_GDS_HEPIY_TP(tpnum)
      || IS_GDS_HEPI1_TP(tpnum) || IS_GDS_HEPI2_TP(tpnum)
      || IS_GDS_ADCU_TP(tpnum);
  }

  // Translate test point number (but not the EXC number) to the DCU ID
  // Returns 0 if DCU is unknown
  unsigned int tpnum_to_dcuid(unsigned int tpnum) {
    if (IS_GDS_ETMX_TP(tpnum)) return  DCU_ID_SUS_ETMX;
    else if (IS_GDS_ETMY_TP(tpnum)) return  DCU_ID_SUS_ETMY;

    else if (IS_GDS_HEPIX_TP(tpnum)) return  DCU_ID_HEPI_EX;
    else if (IS_GDS_HEPIY_TP(tpnum)) return  DCU_ID_HEPI_EY;
    else if (IS_GDS_HEPI1_TP(tpnum)) return  DCU_ID_HEPI_1;
    else if (IS_GDS_HEPI2_TP(tpnum)) return  DCU_ID_HEPI_2;

    else if (IS_GDS_ASC_TP(tpnum)) return  DCU_ID_ASC;
    else if (IS_GDS_LSC_TP(tpnum)) return  DCU_ID_LSC;

    else if (IS_GDS_SOS_TP(tpnum)) return  DCU_ID_SUS_SOS;
    else if (IS_GDS_SUS1_TP(tpnum)) return  DCU_ID_SUS_1;
    else if (IS_GDS_SUS2_TP(tpnum)) return  DCU_ID_SUS_2;
    else if (IS_GDS_SUS3_TP(tpnum)) return  DCU_ID_SUS_3;

    else if (IS_GDS_ADCU1_TP(tpnum)) return  DCU_ID_ADCU_1;
    else if (IS_GDS_ADCU2_TP(tpnum)) return  DCU_ID_ADCU_2;
    else if (IS_GDS_ADCU3_TP(tpnum)) return  DCU_ID_ADCU_3;
    else if (IS_GDS_ADCU4_TP(tpnum)) return  DCU_ID_ADCU_4;

    else return 0;
  }
#endif
};

#endif
