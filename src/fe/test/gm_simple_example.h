/******************************************************************-*-c-*-
 * Myricom GM networking software and documentation			 *
 * Copyright (c) 1999-2003 by Myricom, Inc.				 *
 * All rights reserved.	 See the file `COPYING' for copyright notice.	 *
 *************************************************************************/

/**
  @file gm_simple_example.h
 
  Include file for gm_simple_example_send.c and gm_simple_example_recv.c.
*/

#include "gm.h"

/* The following define allows the programmer to turn GM strong typeing off
 * regardless of the setting in gm.h.  Note that if this effectively changes
 * the definition in gm.h, the compiler will probably issue a warning, which
 * may safely be ignored.
 *
 * It is *not* recommended to override GM_STRONG_TYPES to 1; see the definition
 * in gm.h for the reasons.
 */
#if 0
#define GM_STRONG_TYPES 0
#endif

#define GM_SIMPLE_EXAMPLE_PORT_NUM_RECV 4
#define GM_SIMPLE_EXAMPLE_PORT_NUM_SEND 2

#define GM_SIMPLE_EXAMPLE_PRIORITY GM_LOW_PRIORITY

#define GM_SIMPLE_EXAMPLE_SIZE 7
#define GM_SIMPLE_EXAMPLE_BUFFER_COUNT 1
#define GM_SIMPLE_EXAMPLE_BUFFER_LENGTH \
 (gm_max_length_for_size(GM_SIMPLE_EXAMPLE_SIZE))


typedef struct				/* Receiver-to-sender ID message */
{
  gm_u64_n_t directed_recv_buffer_addr;	/* UVA of directed-receive buffer */
  gm_u32_n_t global_id;			/* Receiver's GM global ID */
  gm_u32_n_t slack;			/* Make length a multiple of 64 */
} gm_s_e_id_message_t;
