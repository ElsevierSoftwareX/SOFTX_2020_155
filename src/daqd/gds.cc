#include <config.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/resource.h>
#ifndef __linux__
#include <sys/priocntl.h>
#include <sys/rtpriocntl.h>
#include <sys/lwp.h>
#endif
#include <sys/mman.h>

#include <string>
#include <iostream>
#include <fstream>
#include "circ.hh"

#include <FlexLexer.h>
#include "channel.hh"
#include "daqc.h"
#include "daqd.hh"

extern "C" {
#include "testpoint.h"
}

#include "gds.hh"

extern daqd_c daqd;

char*
gds_c::construct_req_string (char *alias[], int nptr)
{
  int req_size = 10;
  char *req;
  for (int i = 0; i < nptr; i++)
    req_size += 1 + strlen(alias [i]);

  if (!(req = (char *) malloc (req_size))) {
    system_log(1,"gds_c::construct_req_string(): out of memory");
    return 0;
  }
  strcpy (req, alias [0]);
  for (int i = 1; i < nptr; i++)
    strcat (strcat (req, " "), alias [i]);
  return req;
}

#if 0
int
gds_c::req_names (char *alias[], unsigned int *tpnum, channel_t *gds[], int nptr)
{
  locker mon (this);

  char *req = construct_req_string (alias, nptr);
  if (!req)
    return -1;
  system_log(1,"gds_c::req_names(): about to request `%s'", req);

  pthread_mutex_lock (&signal_mtx);
  signal_p = 0;
  pthread_mutex_unlock (&signal_mtx);

  int rtn = tpRequestName(req, -1, 0, 0);
  system_log(1,"gds_c::req_names(): tpRequestName() returned %d", rtn);
  if (rtn) {
    free (req);
    return rtn;
  }

  // examine alias data info structures in the reflective
  // memory and get the dataOffset. Then find corresponding
  // GDS channels. Put pointers to them into `gds[]'
  int gds_cnt = 0;
  for (int i = 0; i < nptr; i++) {
    int j;
    for (j = 0; j < daqd.num_channels; j++)
      if (daqd.channels [j].gds & 2 // alias channel
	  && ! strcmp (daqd.channels [j].name, alias [i])) {
#ifdef FILE_CHANNEL_CONFIG
	int ifoid = daqd.channels [j].ifoid;
	volatile GDS_CNTRL_BLOCK *gb =
	  (volatile GDS_CNTRL_BLOCK *)((ifoid? daqd.rm_mem_ptr1: daqd.rm_mem_ptr) + DAQ_GDS_BLOCK_ADD);
	int dcu = -1;
	int index = -1;

	// Test point number is `daqd.channels [j].chNum'
	int k;
	for (int ntries = 0; ntries < 2; ntries++) {

	  // Wait if not found and retry
	  if (ntries) sleep(1);

	  // For debugging print the test point tables
	  for (k = 0; k < 4; k++) {
	    DEBUG1(cerr << dcuName[k + DCU_ID_FIRST_GDS] << " ");
	    for (int l = 0; l < daqGdsTpNum[k]; l++) {
	      DEBUG1(cerr << " " << gb->tp[k][0][l]);
	    }
	    DEBUG1(cerr << endl);
	  }

	  // Find this number in the tables.
	  //
	  for (k = 0; k < DAQ_GDS_DCU_NUM; k++) {
	    for (int l = 0; l < daqGdsTpNum[k]; l++) {
	      if (daqd.channels [j].chNum == gb->tp[k][0][l]) {
		dcu = DCU_ID_FIRST_GDS + k; index = l;
		ntries = 100;
	      }
	    }
	  }
	}

	if (dcu == -1) {
	  system_log(1,"ETIMEDOUT: test point `%s' (tp_num=%d) was not set by the test point manager; request failed", alias[i],daqd.channels[j].chNum);
  	    tpClearName (req);
    	    free (req);
            return -1;
	}

	if (!daqd.cit_40m) {
	  // Find GDS DCU channel with corresponding index.
	  // Error if not found.
	  // Data from the second RFM net is in 50+ channels
	  index += 50 * gds_c::tpnum_to_rmnet(daqd.channels [j].chNum);
	}

	for (k = 0; k < daqd.num_channels; k++) {
	  if (daqd.channels [k].gds & 1
	      && daqd.channels [k].dcu_id == dcu
	      && daqd.channels [k].ifoid == ifoid
	      && daqd.channels [k].chNum == index) {
	    gds [gds_cnt++] = daqd.channels + k;
	    break;
	  }
	}

	if (k == daqd.num_channels) {
	  system_log(1,"NOT FOUND: test point `%s' set in dcu %d at index %d; index not configured", alias[i], dcu, index);
  	  tpClearName (req);
    	  free (req);
	  return -1;
	}
	system_log(1,"connected to `%s' dcuid %d", daqd.channels[k].name, dcu);

#else
	struct dataInfoStr *dinfo = daqd.channels [j].rm_dinfo;
	assert (dinfo);
	// check data offset
	if (bsw(dinfo->dataOffset) == 0) {
	  // wait for defined time interval
	  struct timespec to;
	  #define MYTIMEOUT 10
	  to.tv_sec = time (0) + MYTIMEOUT;
	  to.tv_nsec = 0;

	  // data offset is zero -- wait for update from the controller
  	  // wait for the refresh signal
	  pthread_mutex_lock (&signal_mtx);
	  int err;
	  while (! signal_p) {
	    err = pthread_cond_timedwait (&signal_cv, &signal_mtx, &to);
	    if (err == ETIMEDOUT)
		break;
	  }
	  pthread_mutex_unlock (&signal_mtx);

	  if (err == ETIMEDOUT) {
	    system_log(1,"ETIMEDOUT: alias channel `%s' has dataOffset==0, dinfoPtr=0x%x, rm_mem_ptr=0x%x", alias [i], dinfo, daqd.rm_mem_ptr);
  	    tpClearName (req);
    	    free (req);
            return -1;
	  }
	  if (bsw(dinfo->dataOffset) == 0) {
	    system_log(1,"second check: alias channel `%s' has dataOffset==0, dinfoPtr=0x%x, rm_mem_ptr=0x%x", alias [i], dinfo, daqd.rm_mem_ptr);
  	    tpClearName (req);
    	    free (req);
	    return -1;
	  }
	}
	// now find GDS channel
	int k;
	for (k = 0; k < daqd.num_channels; k++) 
	  if (daqd.channels [k].rm_offset == bsw(dinfo->dataOffset) + sizeof (int))
	    break;
	if (k == daqd.num_channels) {
	  system_log(1,"data offset in alias channel `%s' not found in GDS channel", alias [i]);
  	  tpClearName (req);
    	  free (req);
	  return -1;
	}
	gds [gds_cnt++] = daqd.channels + k;
#endif

	break;
      }
    if (j == daqd.num_channels) {
      system_log(1,"alias channel `%s' not found", alias [i]);
      tpClearName (req);
      free (req);
      return -1;
    }
  }

  free (req);
  return 0;
}
#endif

int
gds_c::clear_names (char *alias[], int nptr)
{
  locker mon (this);

  char *req = construct_req_string (alias, nptr);
  if (!req)
    return -1;
  system_log(1,"gds_c::clear_names(): about to clear `%s'", req);
  int rtn = tpClearName (req);
  system_log(1,"gds_c::clear_names(): tpClearName() returned %d", rtn);
  free (req);
  return rtn;
}


int
gds_c::gds_initialize () {
  int nodes = 1;
  char *gds_servers[10];

  /* set test point manager address for node 0 */
  if (tpSetHostAddress (0, gds_server, gds_server_rpc_program,
                                       gds_server_rpc_version) != 0) {
     printf ("unable to initialize test point interface\n");
     return 1;
  }
  gds_servers[0] = gds_server;

  /* set test point manager address for node 1; LHO only */
  if (gds_server1) {
    if (strlen(gds_server1) > 0) {
      nodes |= 2;
      if (tpSetHostAddress (1, gds_server1, gds_server_rpc_program,
                                       gds_server_rpc_version) != 0) {
         printf ("unable to initialize test point interface 1\n");
         return 1;
      }
    }
    gds_servers[1] = gds_server1;
  }
  if (gds_server2) {
    nodes |= 4;
    if (tpSetHostAddress (2, gds_server2, gds_server_rpc_program,
                                       gds_server_rpc_version) != 0) {
       printf ("unable to initialize test point interface 2\n");
       return 1;
    }
    gds_servers[2] = gds_server2;
  }
  if (gds_server3) {
    nodes |= 8;
    if (tpSetHostAddress (3, gds_server3, gds_server_rpc_program,
                                       gds_server_rpc_version) != 0) {
       printf ("unable to initialize test point interface 3\n");
       return 1;
    }
    gds_servers[3] = gds_server3;
  }
  if (gds_server4) {
    nodes |= 16;
    if (tpSetHostAddress (4, gds_server4, gds_server_rpc_program,
                                       gds_server_rpc_version) != 0) {
       printf ("unable to initialize test point interface 4\n");
       return 1;
    }
    gds_servers[4] = gds_server4;
  }
  if (gds_server5) {
    nodes |= 32;
    if (tpSetHostAddress (5, gds_server5, gds_server_rpc_program,
                                       gds_server_rpc_version) != 0) {
       printf ("unable to initialize test point interface 5\n");
       return 1;
    }
    gds_servers[5] = gds_server5;
  }
//  testpoint_cleanup ();
  testpoint_client ();

  // Check that we connected to all testpoint managers
  for (int i = 0; i < 7; i++) {
	if (nodes & (1 << i)) { // Node configured
	  testpoint_t tp[1] = {-1};
	  int ret = tpRequest(i, tp, 1, -1, 0, 0);
	  if (ret < 0) {
		printf("Failed to connect to the test point manager node %d\n", i);
		printf("Test point manager on %s may be down\n", gds_servers[i]);
		printf("tpRequest() returned %d\n", ret);
		if (!daqd.allow_tpman_connect_failure)
			return 1;
		else printf("Error ignored because \"allow_tpman_connect_failure\" is set in daqdrc file\n");
	  }
	}
  }
  return 0;
}

int
gds_c::clear_tps (long_channel_t *ac[], int nptr)
{
  locker mon (this);
  testpoint_t tps[nptr];
  int ntps = 0;
  int rtn = 0;

  for (int s = 0; s < n_gds_servers; s++) {
    /* Clear all 4k IFO test points */
    for (int i = 0; i < nptr; i++) {
#ifdef _ADVANCED_LIGO
      if (ac[i]->tp_node == s) 
#else
      if (ac[i]->ifoid == s) 
#endif
      {
  	system_log(1,"About to clear `%s' %d on node %d", ac[i]->name, ac[i]->chNum, s);
	tps[ntps++] = ac[i]->chNum;
      }
    }
    if (ntps) rtn = tpClear(s, tps, ntps);
  }
#if 0
  if (gds_server) {
    /* Clear all 4k IFO test points */
    for (int i = 0; i < nptr; i++) {
#ifdef _ADVANCED_LIGO
      if (ac[i]->tp_node == 0) 
#else
      if (ac[i]->ifoid == 0) 
#endif
      {
  	system_log(1,"About to clear `%s' %d on node 0", ac[i]->name, ac[i]->chNum);
	tps[ntps++] = ac[i]->chNum;
      }
    }
    if (ntps) rtn = tpClear(0, tps, ntps);
  }
  
  if (gds_server1) {
    ntps = 0;
    /* Request all 2k IFO test points */
    for (int i = 0; i < nptr; i++) {
#ifdef _ADVANCED_LIGO
      if (ac[i]->tp_node == 1) 
#else
      if (ac[i]->ifoid == 1)
#endif
      {
  	system_log(1,"About to clear `%s' %d on node 1", ac[i]->name, ac[i]->chNum);
	tps[ntps++] = ac[i]->chNum;
      }
    }
    if (ntps) rtn |= tpClear(1, tps, ntps);
  }
#endif
  return rtn;
}

#if defined(_ADVANCED_LIGO)
extern "C" {
//#ifdef USE_GM
     //extern int gdsTpCounter[DCU_COUNT];
     //extern int gdsTpTable[DCU_COUNT][GM_DAQ_MAX_TPS];
//#else
extern struct cdsDaqNetGdsTpNum * gdsTpNum[2][DCU_COUNT];
//#endif
}
#endif

int
gds_c::req_tps (long_channel_t *ac[], channel_t *gds[], int nptr)
{
  locker mon (this);
  testpoint_t tps[nptr];
  int ntps = 0;

  printf("req_tps; n_gds_servers=%d\n", n_gds_servers);
  for (int s = 0; s < n_gds_servers; s++) {
    ntps = 0;
    for (int i = 0; i < nptr; i++) {
#ifdef _ADVANCED_LIGO
      if (ac[i]->tp_node == s) 
#else
      if (ac[i]->ifoid == s) 
#endif
      {
  	system_log(1,"About to request `%s' %d on node %d", ac[i]->name, ac[i]->chNum, s);
	tps[ntps] = ac[i]->chNum;
	ntps++;
      }
    }
    if (ntps) {
      system_log(1,"Requesting %d testpoints; tp[0]=%d; tp[1]=%d\n",
			ntps, tps[0], tps[1]);
      int rtn = tpRequest(s, tps, ntps, -1, 0, 0);
      if (rtn) {
	system_log(1, "tpRequest(%d) failed; returned %d\n", s, rtn);
#ifdef _ADVANCED_LIGO
	if (daqd.avoid_reconnect)  _exit(1);
#endif
      	return rtn;
      }
    }
  }
#if 0
  if (gds_server) {
    /* Request all 4k IFO test points */
    for (int i = 0; i < nptr; i++) {
#ifdef _ADVANCED_LIGO
      if (ac[i]->tp_node == 0) 
#else
      if (ac[i]->ifoid == 0) 
#endif
      {
  	system_log(1,"About to request `%s' %d on node 0", ac[i]->name, ac[i]->chNum);
	tps[ntps++] = ac[i]->chNum;
      }
    }
    if (ntps) {
      int rtn = tpRequest(0, tps, ntps, -1, 0, 0);
      if (rtn) {
	system_log(1, "tpRequest(0) failed; returned %d\n", rtn);
      	return rtn;
      }
    }
  }
  
  if (gds_server1) {
    ntps = 0;
    /* Request all 2k IFO test points */
    for (int i = 0; i < nptr; i++) {
#ifdef _ADVANCED_LIGO
      if (ac[i]->tp_node == 1) 
#else
      if (ac[i]->ifoid == 1)
#endif
      {
  	system_log(1,"About to request `%s' %d on node 1", ac[i]->name, ac[i]->chNum);
	tps[ntps++] = ac[i]->chNum;
      }
    }
    if (ntps) {
      int rtn = tpRequest(1, tps, ntps, -1, 0, 0);
      if (rtn) {
      	system_log(1, "tpRequest(1) failed; returned %d\n", rtn);
	return rtn;
      }
    }
  }
#endif

#if defined(_ADVANCED_LIGO)
  //int gds_cnt = 0;

  int label1_jumps = 0;

label1:

  int nfound = 0;

  for (int i = 0; i < nptr; i++) {
     int dcuId = 0;
     int index = -1;
     int out = 0;

#if defined(VMICRFM_PRODUCER)
#if defined(COMPAT_INITIAL_LIGO)
   /* Myrinet connected to nodes 2 and higher */
   if (ac[i]->tp_node > 1) 
#else
   /* Myrinet connected to node 1 */
   if (ac[i]->tp_node > 0) 
#endif
    {
#endif

     int ifo = ac[i]->ifoid;

     for (int ntries = 0; ntries < 30; ntries++) {

       // Wait if not found and retry
       if (ntries)  {
	 usleep(100000);
	 if (nfound)  {
		if (label1_jumps > 30) {
			break; // Give up on trying
		} else {
			label1_jumps++;
			goto label1;
		}
	 }
       }

#ifdef USE_BROADCAST
	// Include checking for the legacy GDS TP DCUs if build as broadcast receiver
	int dcu_from = DCU_ID_FIRST_GDS;
	int dcu_to = DAQ_GDS_DCU_NUM + DCU_ID_FIRST_GDS; // last GDS dcu + 1
#if defined(COMPAT_INITIAL_LIGO)
	// Myrinet nodes: figure out the exact dcu id
        if (ac[i]->tp_node > 1)
#endif
	{
		if (ac[i]->tp_node < max_gds_servers) {
			dcu_from = dcu_to = dcuid[ac[i]->tp_node];
			dcu_to++;
		} else break; // Invalid GDS node id
	}
       for (dcuId = dcu_from; dcuId < dcu_to; dcuId++)
#else
       for (dcuId = DCU_ID_SUS_1; dcuId < DCU_COUNT; dcuId++)
#endif
        {
         out = 0;
//#ifdef USE_GM
         //for (int j = 0; j < gdsTpCounter[dcuId]; j++) {
           //if (ac[i]->chNum == gdsTpTable[dcuId][j]) {
		//index = j;
		//out = 1;
		//break;
	   //}
	 //}
//#else
	 if (gdsTpNum[ifo][dcuId] != 0) {
           for (int j = 0; j < gdsTpNum[ifo][dcuId]->count; j++) {
	     system_log(1,"ifo %d DCU %d tp %d\n", ifo, dcuId, gdsTpNum[ifo][dcuId]->tpNum[j]);
             if (ac[i]->chNum == gdsTpNum[ifo][dcuId]->tpNum[j]) {
                index = j;
                out = 1;
		nfound++;
                break;
             }
           }
	 }
//#endif
         if (out) break;
       }
       if (index  > -1) break;
     }

     if (index < 0) {
	  system_log(1,"ETIMEDOUT: test point `%s' (tp_num=%d) was not set by the test point manager; request failed", ac[i]->name, ac[i]->chNum);
#ifdef _ADVANCED_LIGO
	  if (daqd.avoid_reconnect) _exit(1);
#endif
          return -1;
     }
     system_log(1,"dcu %d test point %d at index %d\n", dcuId, ac[i]->chNum, index);

#if 0
     for (int k = 0; k < daqd.num_channels; k++) {
       if (daqd.channels [k].gds & 1
	  && daqd.channels [k].chNum == index) {
	      gds [gds_cnt++] = daqd.channels + k;
              printf("test point %d assigned to channel %d\n", i, k);
	      break;
       }
     }
#endif

    // No gds & 1 channels with Myrinet
    // Calculate offset and be done
    ac [i] -> offset  = daqd.dcuTPOffset[ifo][dcuId];

    // Move to the channel using index
    ac [i] -> offset += 4 * daqd.dcuRate[ifo][dcuId] * index;
#if 0
    if (IS_2K_DCU(dcuId)) ac [i] -> offset += 2048*4*index;
    else if (IS_32K_DCU(dcuId)) ac [i] -> offset += 4*16384*4*index;
    else  ac [i] -> offset += 16384*4*index;
#endif

    // seq_num is set here to the first DAQ channel
    // of the DCU which sending data
    ac [i] -> seq_num = 0;
    for (int k = 0; k < daqd.num_channels; k++) {
	if ( daqd.channels [k].dcu_id == dcuId) {
    	  ac [i] -> seq_num = k;
	  break;
	}
    }
#if defined(VMICRFM_PRODUCER)
   }
#endif
  }
#if !defined(VMICRFM_PRODUCER)
  return 0;
#endif
#endif

#if defined(VMICRFM_PRODUCER)

  // examine alias data info structures in the reflective
  // memory and get the dataOffset. Then find corresponding
  // GDS channels. Put pointers to them into `gds[]'
  int gds_cnt = 0;
  for (int i = 0; i < nptr; i++) {
	int ifoid = ac[i]->ifoid;
	volatile GDS_CNTRL_BLOCK *gb =
	  (volatile GDS_CNTRL_BLOCK *)((ifoid? daqd.rm_mem_ptr1: daqd.rm_mem_ptr) + DAQ_GDS_BLOCK_ADD);
	int dcu = -1;
	int index = -1;

#if defined(_ADVANCED_LIGO)
#if defined(COMPAT_INITIAL_LIGO)
   /* RMEM goes to nodes 0, 1 */
   if (ac[i]->tp_node < 2)
#else
   /* Rmem connected to node 0 */
   if (ac[i]->tp_node == 0) 
#endif
     {
#endif

	// Test point number is `ac[i]->chNum'
	int k;
	for (int ntries = 0; ntries < 2; ntries++) {

	  // Wait if not found and retry
	  if (ntries) sleep(1);

	  // For debugging print the test point tables
	  for (k = 0; k < 4; k++) {
	    DEBUG1(cerr << dcuName[k + DCU_ID_FIRST_GDS] << " ");
	    for (int l = 0; l < daqGdsTpNum[k]; l++) {
	      DEBUG1(cerr << " " << gb->tp[k][0][l]);
	    }
	    DEBUG1(cerr << endl);
	  }

	  // Find this number in the tables.
	  //
	  for (k = 0; k < DAQ_GDS_DCU_NUM; k++) {
	    for (int l = 0; l < daqGdsTpNum[k]; l++) {
	      if (ac[i]->chNum == gb->tp[k][0][l]) {
		dcu = DCU_ID_FIRST_GDS + k; index = l;
		ntries = 100;
	      }
	    }
	  }
	}


	if (dcu == -1) {
	  system_log(1,"ETIMEDOUT: test point `%s' (tp_num=%d) was not set by the test point manager; request failed", ac[i]->name, ac[i]->chNum);
            return -1;
	}

#if defined(DATA_CONCENTRATOR)
// Swap test point slots; required as the test point manager is running on a big-endian machine
	index ^= 1;
#endif

	if (!daqd.cit_40m) {
	  // Find GDS DCU channel with corresponding index.
	  // Error if not found.
	  // Data from the second RFM net is in 50+ channels
	  index += 50 * gds_c::tpnum_to_rmnet(ac[i]->chNum);
	}

	for (k = 0; k < daqd.num_channels; k++) {
	  if (daqd.channels [k].gds & 1
	      && daqd.channels [k].dcu_id == dcu
	      && daqd.channels [k].ifoid == ifoid
	      && daqd.channels [k].chNum == index) {
#if defined(_ADVANCED_LIGO)
	    gds [i] = daqd.channels + k;
#else
	    gds [gds_cnt++] = daqd.channels + k;
#endif
	    break;
	  }
	}

	if (k == daqd.num_channels) {
	  system_log(1,"NOT FOUND: test point `%s' set in dcu %d at index %d; index not configured", ac[i]->name, dcu, index);
	  return -1;
	}
	system_log(1,"connected to `%s' dcuid %d", daqd.channels[k].name, dcu);
#if defined(_ADVANCED_LIGO)
   }
#endif
  }
#endif

  return 0;
}

#if defined(DATA_CONCENTRATOR) || defined(USE_BROADCAST)

static char *tp_data = 0;


#ifdef DATA_CONCENTRATOR
/*
  Build the block of test point data along with aux information
  for the broadcaster/concentrator
  
*/
char *
gds_c::build_tp_data (int *l, char *data, int bn)
{
  static int tp_data_len = 0;
  static const int max_tp_data_len = 5*1024*1024; // Maximum room we'll have
  if (tp_data == 0) {
	tp_data = (char *) malloc(max_tp_data_len);
  	if (tp_data == 0) {
		fprintf(stderr, "Failed to allocate TP data broadcast buffer\n");
		exit(1);
	}
  }
  unsigned int *tp_ptr = (unsigned int *) tp_data;

#if 0
extern struct cdsDaqNetGdsTpNum * gdsTpNum[2][DCU_COUNT];
typedef struct cdsDaqNetGdsTpNum {
   int count; /* test points count */
   int tpNum[DAQ_GDS_MAX_TP_NUM];
} cdsDaqNetGdsTpNum;
#endif

  int ndcu = 0;
  unsigned int tidx = 1; // table index (integers)
  unsigned int data_size = 0; // test point data size
for(int ifo = 0; ifo < daqd.data_feeds; ifo++) {
  for (int i = DCU_ID_SUS_1; i < DCU_COUNT; i++) {
    unsigned int tp_count = 0;
    unsigned int tp_table[DAQ_GDS_MAX_TP_ALLOWED]; // Legacy testpoint table
    unsigned int tp_table_len = 0;
#if defined(COMPAT_INITIAL_LIGO)
    if (IS_TP_DCU(i)) {
	// This is the legacy RFM testpoint DCU (2/16K TP/EXC; 4 DCUs total)
	// Examine test point table to find out how many test point numbers
	// are set.

	// Pointer into the testpoint table in the RFM
	volatile GDS_CNTRL_BLOCK *gb =
	   (volatile GDS_CNTRL_BLOCK *)((ifo? daqd.rm_mem_ptr1: daqd.rm_mem_ptr) + DAQ_GDS_BLOCK_ADD);

  	// For debugging print the test point tables
    	DEBUG(2,cerr << dcuName[i] << " ");
	int tp_index = i - DCU_ID_FIRST_GDS;
	tp_table_len = daqGdsTpNum[tp_index];
	if (tp_table_len > DAQ_GDS_MAX_TP_ALLOWED)  {
		tp_table_len = DAQ_GDS_MAX_TP_ALLOWED;
	}
    	for (int l = 0; l < daqGdsTpNum[tp_index]; l++) {
		// Swap test point slots; required as the test point manager is running on a big-endian machine
		int index = l;
	 	index ^= 1;
		unsigned int tpnum = gb->tp[tp_index][0][index];
       		DEBUG(2,cerr << " " << tpnum);
		// :TODO: look at the status of AWG channel data
		// and only transmit excitations with good status
		if (tpnum) tp_count++;
    		if (tp_count > DAQ_GDS_MAX_TP_ALLOWED) {
			tp_count = DAQ_GDS_MAX_TP_ALLOWED;
		}
		tp_table[l] = tpnum;
    	}
    	DEBUG(2,cerr << endl);
	if (tp_count == 0) continue; // No testpoint defined
    } else
#endif
 	   {
	// Check Myrinet DCUs
	if (gdsTpNum[ifo][i] == 0) continue;
	if (gdsTpNum[ifo][i] -> count == 0) continue;
	tp_count = gdsTpNum[ifo][i] -> count;
    }

    // Send DCU number
    tp_ptr[tidx++] = ntohl(i + (ifo? 32: 0));

    // Send TP count
    if (tp_count > DAQ_GDS_MAX_TP_ALLOWED) tp_count = DAQ_GDS_MAX_TP_ALLOWED;
    tp_ptr[tidx++] = ntohl(tp_count);
    unsigned int dcu_rate = daqd.dcuRate[ifo][i];

    // Send DCU rate
    tp_ptr[tidx++] = ntohl(dcu_rate);
    unsigned int block_bytes = dcu_rate * 4 / 16; // size of single TP data 
    unsigned int block_bytes_ps = dcu_rate * 4;

    //printf("DCU %d has %d tps\n", i, tp_count);

#if defined(COMPAT_INITIAL_LIGO)
    if (IS_TP_DCU(i)) {
      // Legacy testpoint DCU

      // Send TP numbers table
      for (int j = 0; j < tp_table_len; j++) {
	if (tp_table[j]) { // test point set
	  tp_ptr[tidx++] = ntohl(tp_table[j]);
	}
      }
      // Send test point data
      char *data_src = data + daqd.dcuTPOffset[ifo][i]; // data source address

      for (int j = 0; j < tp_table_len; j++) {
	if (tp_table[j]) { // test point set; send data
	  unsigned int rfm_offs = 0;

          // Offset to 5579 RFM data in the second half of DCU
          if (gds_c::tpnum_to_rmnet(tp_table[j])) {
	    rfm_offs = 16 * daqd.dcuSize[ifo][i] / 2;
          } else {
	    rfm_offs = 0;
	  }

	DEBUG1(printf("testpoint %d rfm_offs=%d\n", tp_table[j], rfm_offs));
	DEBUG1(printf("data %d %d %d\n", data_src[
                                + block_bytes * bn 
                                + block_bytes_ps * j
                                + rfm_offs],
					data_src[
                                + block_bytes * bn
                                + block_bytes_ps * j
                                + rfm_offs + 1],
					data_src[
                                + block_bytes * bn
                                + block_bytes_ps * j
                                + rfm_offs + 2 ]));

	  memcpy(tp_ptr + tidx, data_src
				+ block_bytes * bn
				+ block_bytes_ps * j
				+ rfm_offs,
		block_bytes);
	  tidx += block_bytes/4;
	}
      }
    } else
#endif
	   {
      // Myrinet DCU

      // Send TP numbers table
      for (int j = 0; j < tp_count; j++) {
	  //if (gdsTpNum[ifo][i] -> tpNum[j] == 0) continue;
	  //printf("DCU %d TP %d\n", i, gdsTpNum[ifo][i] -> tpNum[j]);
	  tp_ptr[tidx++] = ntohl(gdsTpNum[ifo][i] -> tpNum[j]);
      }

      // Copy data
      char *data_src = data + daqd.dcuTPOffset[ifo][i]; // data source address
      for (int j = 0; j < tp_count; j++) {
	  memcpy(tp_ptr + tidx, data_src
				+ block_bytes * bn
				+ block_bytes_ps * j,
		block_bytes);
	  tidx += block_bytes/4;
      }
    }
    if (tp_count) ndcu++;
  }
}
  *tp_ptr = ntohl(ndcu);

  tp_data_len =  4 * tidx;
  *l = tp_data_len;
  return tp_data;
}

#endif
 
// Update test point tables
void
gds_c::update_tp_data (unsigned int *d, char *dest)
{
  // How many DCUs we have comes first
  unsigned int ndcu = ntohl(*d);
  d++;
  //printf("ndcu=%d\n", ndcu);
  for (int i = 0; i < ndcu && i < DCU_COUNT; i++) {
    // Received data per DCU header contains these variables
    unsigned int dcuid = ntohl(*d++);
    unsigned int ifo = 0;
    if (dcuid > 32) { // This is LHO H2 data
	dcuid -= 32;
	ifo = 1;
    }
    unsigned int ntp = ntohl(*d++);
    unsigned int rate = ntohl(*d++);
    DEBUG1(printf("ifo %d DCU %d rate %d ntp %d gdsTpNum=0x%x\n", ifo, dcuid, rate, ntp, gdsTpNum[ifo][dcuid]));
    if (ntp > DAQ_GDS_MAX_TP_ALLOWED) ntp = DAQ_GDS_MAX_TP_ALLOWED;
    if (gdsTpNum[ifo][dcuid]) gdsTpNum[ifo][dcuid] -> count  = ntp;

    // Table of testpoints goes next
    for (int j = 0; j < ntp; j++) {
	unsigned int tpnum = ntohl(*d++);
	DEBUG1(printf("%d\n", tpnum));
	if (gdsTpNum[ifo][dcuid]) gdsTpNum[ifo][dcuid] -> tpNum[j] = tpnum;
    }

    // Test points' data follows
    unsigned int dcu_rate = daqd.dcuRate[ifo][dcuid];
    dcu_rate *= 4; // Size of a test point sample (float)
    dcu_rate /= DAQ_NUM_DATA_BLOCKS_PER_SECOND; // Number of blocks per second (16HZ always)
    unsigned int dcu_offs  = daqd.dcuTPOffsetRmem[ifo][dcuid];

    //printf("dcu_offs = 0x%x; first data sample = 0x%x\n", dcu_offs, *((int *)d) );
    if (dcu_offs) {
    	//printf("copying %d bytes\n", dcu_rate * ntp);
	memcpy(dest + dcu_offs, d, dcu_rate * ntp);
	//for (int j = 0; j < ntp; j++)
		//memcpy(dest + dcu_offs + j*dcu_rate, d, dcu_rate);
    }
    d += dcu_rate / 4 * ntp;

#if 0
// Do not delete, testing code
#define byteswap(a,b) ((char *)&a)[0] = ((char *)&b)[3]; ((char *)&a)[1] = ((char *)&b)[2];((char *)&a)[2] = ((char *)&b)[1];((char *)&a)[3] = ((char *)&b)[0];
    for (int j = 0; j < dcu_rate/4; j+= 4) {
	float d = dcuid + j;
	float b;
	byteswap(b,d);
	char *a = dest + dcu_offs + j;
	*((float *)a) = b;
    }
#endif
  }
}

#endif
