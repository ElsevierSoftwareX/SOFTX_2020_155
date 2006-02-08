/*************************************************************************
 * Myricom GM networking software and documentation			 *
 * Copyright (c) 2001-2003 by Myricom, Inc.				 *
 * All rights reserved.	 See the file `COPYING' for copyright notice.	 *
 *									 *
 * Portions Copyright 1999 by Cam Macdonnell - University of Alberta.	 *
 * Used with permission.						 *
 *************************************************************************/

/**
   @file gm_simple_example_send.c

 * Simple Send/Recv for GM
 * Author: Max Stern (Myricom)
 * Based on a program by Cam Macdonell - University of Alberta
 *
 * Syntax: gm_simple_example_send [<local_board_num>]
 *
 *         The default for <local_board_num> is 0.
 *
 * Description:
 *
 *   This program, together with gm_simple_example_recv, serves as a super-
 *   simple example of sending and receiving in GM, using both the basic send
 *   (gm_send type) and the directed send (gm_directed_send type) paradigms.
 *
 *   The programs are written to be portable and interoperable without
 *   respect to the pointer size or the "endianness" of the host platforms.
 *
 *   This program, the "sender" or master process, takes the following main
 *   actions (see commentary in the code below for the "how" details):
 *
 *   - loops on gm_receive() until it receives a message.  It assumes that
 *     this message (if the proper length) contains the receiver's global id
 *     and the receiver's directed-send buffer address;
 *
 *   - uses gm_directed_send_with_callback() to copy a message directly into
 *     the receiver's memory (note: the receiver does not do a gm_receive());
 *
 *   - uses gm_send_with_callback() to transmit a normal message, which
 *     the receiver will get via gm_receive();
 *
 *   - loops on gm_receive() until both the send callbacks have been consumed;
 *
 *   - exits.
 *
 * Usage Note:
 *
 *   Assume the sender is to run on board 1 on hosta, and the receiver
 *   on board 0 on hostb.  The commands would look like this:
 *
 *     First, on hostb:
 *       hostb> gm_simple_example_recv hosta:1
 *
 *     Then, on hosta:
 *       hosta> gm_simple_example_send 1
 *
 * Note: The sender and the receiver use different GM ports.  This makes it
 *       possible, if desired, to run the sender and receiver over the same
 *       interface.  The commands might look like this:
 *
 *     First, on hosta:
 *       hosta> gm_simple_example_recv hosta &
 *
 *     Then, also on hosta:
 *       hosta> gm_simple_example_send
 *
 *   See also the Description in gm_simple_example_recv.c.
 *
 * */

#include <stdlib.h>
#include <stdio.h>
#include "gm.h"
#include "gm_simple_example.h"

static void my_send_callback (struct gm_port *port,
			      void *the_context,
			      gm_status_t the_status);


int
main (int argc, char **argv)
{
  static struct gm_port *my_port = 0;
  static void *out_buffer = 0, *in_buffer = 0, *directed_send_buffer = 0;
  int my_board_num = 0;			/* Default board_num is 0 */
  gm_u32_t receiver_node_id;
  gm_u32_t receiver_global_id;
  unsigned int send_length;
  int expected_messages = 1;
  gm_remote_ptr_t directed_send_addr;
  gm_recv_event_t *event;
  gm_status_t main_status;
/* expected_callbacks makes sure we don't exit until we have handled all
   the callbacks we expect.                                              */
  static int expected_callbacks = 0;

  gm_init();

  /* Parse the program argument(s) */

  if (argc > 2)
    {
      gm_printf ("USAGE: gm_simple_example_send [<local_board_num>]\n");
      main_status = GM_INVALID_PARAMETER;
      goto abort_with_nothing;
    }
  if (argc > 1)
    {
      my_board_num = atoi (argv[1]);	/* Optional parameter */
    }

  /* Open a port on our local interface. */

  main_status = gm_open (&my_port, my_board_num,
			 GM_SIMPLE_EXAMPLE_PORT_NUM_SEND,
			 "gm_simple_example_send",
			 (enum gm_api_version) GM_API_VERSION_1_1);
  if (main_status == GM_SUCCESS)
    {
      gm_printf ("[send] Opened board %d port %d\n",
		 my_board_num, GM_SIMPLE_EXAMPLE_PORT_NUM_SEND);
    }
  else
    {
      gm_perror ("[send] Couldn't open GM port", main_status);
      goto abort_with_nothing;
    }

  /* Allocate DMAable message buffers. */

  out_buffer = gm_dma_calloc (my_port, GM_SIMPLE_EXAMPLE_BUFFER_COUNT,
			      GM_SIMPLE_EXAMPLE_BUFFER_LENGTH);
  if (out_buffer == 0)
    {
      gm_printf ("[send] Couldn't allocate out_buffer\n");
      main_status = GM_OUT_OF_MEMORY;
      goto abort_with_open_port;
    }

  in_buffer = gm_dma_calloc (my_port, GM_SIMPLE_EXAMPLE_BUFFER_COUNT,
			     GM_SIMPLE_EXAMPLE_BUFFER_LENGTH);
  if (in_buffer == 0)
    {
      gm_printf ("[send] Couldn't allocate in_buffer\n");
      main_status = GM_OUT_OF_MEMORY;
      goto abort_with_out_buffer;
    }

  directed_send_buffer = gm_dma_calloc (my_port,
					GM_SIMPLE_EXAMPLE_BUFFER_COUNT,
					GM_SIMPLE_EXAMPLE_BUFFER_LENGTH);
  if (directed_send_buffer == 0)
    {
      gm_printf ("[send] Couldn't allocate directed_send_buffer\n");
      main_status = GM_OUT_OF_MEMORY;
      goto abort_with_in_buffer;
    }

  /* Note that all send tokens are available */

  /* Tell GM where our receive buffer is */

  gm_provide_receive_buffer (my_port, in_buffer, GM_SIMPLE_EXAMPLE_SIZE,
			     GM_SIMPLE_EXAMPLE_PRIORITY);

  /* Wait for the message from gm_simple_example_recv */

  while (expected_messages)
    {
      void *recv_buffer;
      unsigned int size;
      gm_s_e_id_message_t *id_message;
      gm_u32_t recv_length;

      event = gm_receive (my_port);
      recv_length = gm_ntoh_u32 (event->recv.length);

      switch (GM_RECV_EVENT_TYPE(event))
	{
	case GM_RECV_EVENT:
	case GM_PEER_RECV_EVENT:
	case GM_FAST_PEER_RECV_EVENT:
	  if (recv_length != sizeof (gm_s_e_id_message_t))
	    {
	      gm_printf ("[send] *** ERROR: incoming message length %d "
			 "incorrect; should be %ld\n",
			 recv_length, sizeof (gm_s_e_id_message_t));
	      main_status = GM_FAILURE; /* Unexpected incoming message */
	      goto abort_with_directed_send_buffer;
	    }

	  id_message = gm_ntohp (event->recv.message);
	  receiver_global_id = gm_ntoh_u32(id_message->global_id);
	  directed_send_addr =
	    gm_ntoh_u64(id_message->directed_recv_buffer_addr);
	  main_status = gm_global_id_to_node_id(my_port,
						receiver_global_id,
						&receiver_node_id);
	  if (main_status != GM_SUCCESS)
	    {
	      gm_perror ("[send] Couldn't convert global ID to node ID",
			 main_status);
	      goto abort_with_directed_send_buffer;
	    }

	  expected_messages--;

	  /* Return the buffer for reuse */

	  recv_buffer = gm_ntohp (event->recv.buffer);
	  size = (unsigned int)gm_ntoh_u8 (event->recv.size);
	  gm_provide_receive_buffer (my_port, recv_buffer, size, 
				     GM_SIMPLE_EXAMPLE_PRIORITY);
	  break;

	case GM_NO_RECV_EVENT:
	  break;

	default:
	  gm_unknown (my_port, event);	/* gm_unknown calls the callback */
	}
    } /* while */

  sprintf (directed_send_buffer, "Directed send was successful!");

  /*  Copy the buffer directly into the receiver's memory */

  gm_directed_send_with_callback (my_port,
				  directed_send_buffer,
				  (gm_remote_ptr_t) directed_send_addr,
				  (unsigned long)
				  gm_strlen (directed_send_buffer) + 1,
				  GM_SIMPLE_EXAMPLE_PRIORITY,
				  receiver_node_id,
				  GM_SIMPLE_EXAMPLE_PORT_NUM_RECV,
				  my_send_callback,
				  &expected_callbacks);
  expected_callbacks++;

  sprintf (out_buffer, "This is the sender!!");

  /*  Now send a regular message, which will signal the receiver to look at
      its directed-send buffer                                              */

  send_length = (unsigned long) gm_strlen (out_buffer) + 1;
  gm_send_with_callback (my_port,
			 out_buffer,
			 GM_SIMPLE_EXAMPLE_SIZE,
			 send_length,
			 GM_SIMPLE_EXAMPLE_PRIORITY,
			 receiver_node_id,
			 GM_SIMPLE_EXAMPLE_PORT_NUM_RECV,
			 my_send_callback,
			 &expected_callbacks);
  expected_callbacks++;

  /*  Now we wait for the callbacks for the sends we did above  */
  while (expected_callbacks)
    {
      event = gm_receive (my_port);

      switch (GM_RECV_EVENT_TYPE(event))
	{
	case GM_RECV_EVENT:
	case GM_PEER_RECV_EVENT:
	case GM_FAST_PEER_RECV_EVENT:
	  gm_printf ("[send] Receive Event (UNEXPECTED)\n");
	  main_status = GM_FAILURE; /* Unexpected incoming message */
	  goto abort_with_directed_send_buffer;

	case GM_NO_RECV_EVENT:
	  break;

	default:
	  gm_unknown (my_port, event);	/* gm_unknown calls the callback */
	}
    }

  gm_printf ("[send] gm_simple_example_send completed successfully\n");
  main_status = GM_SUCCESS;

 abort_with_directed_send_buffer:
  gm_dma_free (my_port, directed_send_buffer);
 abort_with_in_buffer:
  gm_dma_free (my_port, in_buffer);
 abort_with_out_buffer:
  gm_dma_free (my_port, out_buffer);
 abort_with_open_port:
  gm_close (my_port);
 abort_with_nothing:
  gm_finalize();
  gm_exit (main_status);
}

/* This function is called inside gm_unknown() when there is a callback
   ready to be processed.  It tells us that a send has completed, either
   successfully or with error. */

static void
my_send_callback (struct gm_port *port, void *the_context,
		  gm_status_t the_status)
{
  /* One pending callback has been received */
  (*(int *)the_context)--;

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
