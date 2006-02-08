/*************************************************************************
 * Myricom GM networking software and documentation			 *
 * Copyright (c) 2001-2003 by Myricom, Inc.				 *
 * All rights reserved.	 See the file `COPYING' for copyright notice.	 *
 *									 *
 * Portions Copyright 1999 by Cam Macdonnell - University of Alberta.	 *
 * Used with permission.						 *
 *************************************************************************/

/**
   @file gm_simple_example_recv.c

 * Simple Send/Recv for GM
 * Author: Max Stern (Myricom)
 * Based on a program by Cam Macdonell - University of Alberta
 *
 * Syntax: gm_simple_example_recv <sender_nodename> [<local_board_num>]
 *
 *         <sender_nodename> is required.  Include the board_num suffix if the
 *         sender is using a board_num other than 0; e.g., hosta:1 for board 1
 *         on host "hosta".
 *
 *         The default for <local_board_num> is 0.
 *
 * Description:
 *
 *   This program, together with gm_simple_example_send, serves as a super-
 *   simple example of sending and receiving in GM, using both the basic send
 *   (gm_send type) and the directed send (gm_directed_send type) paradigms.
 *
 *   The programs are written to be portable and interoperable without
 *   respect to the pointer size or the "endianness" of the host platforms.
 *
 *   This program, the "receiver" or slave process, takes the following main
 *   actions (see commentary in the code below for the "how" details):
 *
 *   - uses gm_send_with_callback() to transmit its global id and its
 *     directed-receive buffer address to the "sender" process (which may be
 *     on the same or a different host);
 *
 *   - loops on gm_receive() until both the send callback from the above send
 *     and an incoming message from gm_simple_example_send have been received
 *     (the incoming message serves as notification that the directed-receive
 *     buffer has been modified);
 *
 *   - displays the contents of its directed-receive buffer, showing the
 *     contents stored there by gm_simple_example_send;
 *
 *   - exits.
 *
 *   See also the Description and the Usage Note in gm_simple_example_send.c.
 *
 * */

#include <stdlib.h>
#include <stdio.h>
#include "gm.h"
#include "gm_simple_example.h"

/** A structure used to store the state of this simple example program. */

typedef struct
{
  int messages_expected;
  int callbacks_pending;
} gm_s_e_context_t;

static void wait_for_events (struct gm_port *p,
			     gm_s_e_context_t *the_context);
static void my_send_callback (struct gm_port *port,
			      void *the_context,
			      gm_status_t the_status);


int
main (int argc, char **argv)
{
  struct gm_port *my_port;
  char sender_nodename[64];
  gm_s_e_context_t context;
  gm_status_t main_status;
  int my_board_num = 0;		/* Default board_num is 0 */
  gm_u32_t my_node_id, sender_node_id;
  unsigned int my_global_id = 0;
  void *directed_receive_buffer;
  void *in_buffer;
  gm_s_e_id_message_t *id_message;
  gm_size_t alloc_length;

  context.messages_expected = 0;
  context.callbacks_pending = 0;

  gm_init();

  /* Parse the program argument(s) */
  
  if ((argc < 2) || (argc > 3))
    {
      gm_printf ("USAGE: gm_simple_example_recv <sender_nodename> "
		 "[<local_board_num>]\n");
      main_status = GM_INVALID_PARAMETER;
      goto abort_with_nothing;
    }
  if (gm_strlen (argv[1]) + 1 > sizeof (sender_nodename))
    {
      gm_printf ("[recv] *** ERROR: "
		 "sender nodename length %ld exceeds maximum of %ld\n",
		 gm_strlen (argv[1]), sizeof (sender_nodename) - 1);
      main_status = GM_INVALID_PARAMETER;
      goto abort_with_nothing;
    }
  gm_strncpy (sender_nodename,	/* Mandatory 1st parameter */
	      argv[1],
	      sizeof (sender_nodename) - 1);
  if (argc > 2)
    {
      my_board_num = atoi (argv[2]);	/* Optional 2nd parameter  */
    }

  /* Open a port on our local interface. */

  main_status = gm_open (&my_port, my_board_num,
			 GM_SIMPLE_EXAMPLE_PORT_NUM_RECV,
			 "gm_simple_example_recv",
			 (enum gm_api_version) GM_API_VERSION_1_1);
  if (main_status == GM_SUCCESS)
    {
      gm_printf ("[recv] Opened board %d port %d\n",
		 my_board_num, GM_SIMPLE_EXAMPLE_PORT_NUM_RECV);
    }
  else
    {
      gm_perror ("[recv] Couldn't open GM port", main_status);
      goto abort_with_nothing;
    }

  gm_get_node_id (my_port, &my_node_id);
  main_status = gm_node_id_to_global_id (my_port, my_node_id, &my_global_id);
  if (main_status != GM_SUCCESS)
    {
      gm_perror ("[recv] Couldn't convert node ID to global ID", main_status);
      goto abort_with_open_port;
    }

  /* Try to convert the sender's hostname into a node ID. */
  
  main_status = gm_host_name_to_node_id_ex
    (my_port, 10000000, sender_nodename, &sender_node_id);
  if (main_status == GM_SUCCESS)
    {
      gm_printf ("[recv] sender node ID is %d\n", sender_node_id);
    }
  else
    {
      gm_printf ("[recv] Conversion of nodename %s to node id failed\n",
		 sender_nodename);
      gm_perror ("[recv]", main_status);
      goto abort_with_open_port;
    }
  if (my_node_id == sender_node_id)
    {
      gm_printf ("[recv] NOTE: sender and receiver are same node, id=%d\n",
		 my_node_id);
    }

  /* Allocate DMAable message buffers. */

  alloc_length = sizeof(*id_message);
  id_message = (gm_s_e_id_message_t *)gm_dma_calloc (my_port, 1, alloc_length);
  if (id_message == 0)
    {
      gm_printf ("[recv] Couldn't allocate output buffer for id_message\n");
      main_status = GM_OUT_OF_MEMORY;
      goto abort_with_open_port;
    }

  in_buffer = gm_dma_calloc (my_port, GM_SIMPLE_EXAMPLE_BUFFER_COUNT,
			     GM_SIMPLE_EXAMPLE_BUFFER_LENGTH);
  if (in_buffer == 0)
    {
      gm_printf ("[recv] Couldn't allocate in_buffer\n");
      main_status = GM_OUT_OF_MEMORY;
      goto abort_with_id_message;
    }

  directed_receive_buffer = gm_dma_calloc (my_port,
					   GM_SIMPLE_EXAMPLE_BUFFER_COUNT,
					   GM_SIMPLE_EXAMPLE_BUFFER_LENGTH);
  if (directed_receive_buffer == 0)
    {
      gm_printf ("[recv] Couldn't allocate directed_receive_buffer\n");
      main_status = GM_OUT_OF_MEMORY;
      goto abort_with_in_buffer;
    }

  /* Allow any GM process to modify any of the local DMAable buffers. */

  gm_allow_remote_memory_access (my_port);

  /* Compose and send the message to tell sender our node_id and our
     directed-receive-buffer address */

  /* Here we take advantage of the fact that gm_remote_ptr_t is 64 bits for
     all platforms and architectures. */
  id_message->directed_recv_buffer_addr =
    gm_hton_u64((gm_size_t)directed_receive_buffer);
  id_message->global_id = gm_hton_u32(my_global_id);

  /* If, through a programming error, we had defined things such that
     sizeof(*id_message) > gm_max_length_for_size(GM_SIMPLE_EXAMPLE_SIZE),
     the following GM API call would return an error at run time. */
  gm_send_with_callback (my_port,
			 id_message,
			 GM_SIMPLE_EXAMPLE_SIZE,
			 sizeof(*id_message),
			 GM_SIMPLE_EXAMPLE_PRIORITY,
			 sender_node_id,
			 GM_SIMPLE_EXAMPLE_PORT_NUM_SEND,
			 my_send_callback,
			 &context);
  context.callbacks_pending++;

  gm_provide_receive_buffer (my_port, in_buffer, GM_SIMPLE_EXAMPLE_SIZE,
			     GM_SIMPLE_EXAMPLE_PRIORITY);

  /* Prefill the directed-receive buffer with error message;
     we should see it change. If not, an error has occurred. */
  sprintf ((char *)directed_receive_buffer, "*** AN ERROR HAS OCCURRED!");

  context.messages_expected = 1; /* Now, we do expect a message from sender */

  /* Nothing more to do but wait for callbacks and the incoming messages */

  wait_for_events (my_port, &context);

  gm_printf ("[recv] "
	     "Having received the incoming message from the sender, the\n"
	     "       directed-receive buffer contains \"%s\"\n",
	     directed_receive_buffer);

  gm_printf ("[recv] gm_simple_example_recv completed successfully\n");

  gm_dma_free (my_port, directed_receive_buffer);
  main_status = GM_SUCCESS;

 abort_with_in_buffer:
  gm_dma_free (my_port, in_buffer);
 abort_with_id_message:
  gm_dma_free (my_port, id_message);
 abort_with_open_port:
  gm_close (my_port);
 abort_with_nothing:
  gm_finalize();
  gm_exit (main_status);
}

static void
wait_for_events (struct gm_port *my_port, gm_s_e_context_t *the_context)
{
  gm_recv_event_t *event;
  void *buffer;
  unsigned int size;


  while ((the_context->callbacks_pending > 0)
	 || (the_context->messages_expected > 0))
    {
      event = gm_receive (my_port);

      switch (GM_RECV_EVENT_TYPE(event))
	{
	case GM_RECV_EVENT:
	case GM_PEER_RECV_EVENT:
	case GM_FAST_PEER_RECV_EVENT:
	  gm_printf ("[recv] Received incoming message: \"%s\"\n",
		     (char *) gm_ntohp (event->recv.message));
	  the_context->messages_expected--;
	  /* Return the buffer for reuse */
	  buffer = gm_ntohp (event->recv.buffer);
	  size = (unsigned int)gm_ntoh_u8 (event->recv.size);
	  gm_provide_receive_buffer (my_port, buffer, size,
				     GM_SIMPLE_EXAMPLE_PRIORITY);
	  break;

	case GM_NO_RECV_EVENT:
	  break;

	default:
	  gm_unknown (my_port, event);	/* gm_unknown calls the callback */
	}
    }
}

/* This function is called inside gm_unknown() when there is a callback
   ready to be processed.  It tells us that a send has completed, either
   successfully or with error. */

static void
my_send_callback (struct gm_port *port, void *the_context,
		  gm_status_t the_status)
{
  /* One pending callback has been received */
  ((gm_s_e_context_t *)the_context)->callbacks_pending--;

  switch (the_status)
    {
    case GM_SUCCESS:
      break;

    case GM_SEND_DROPPED:
      gm_printf ("**** Send dropped!\n");
      break;

    default:
      gm_perror ("Send completed with error", the_status);
    }
}
