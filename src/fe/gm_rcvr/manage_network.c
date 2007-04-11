#include <linux/types.h>
#include <linux/pci.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <drv/gmnet.h>
#include <drv/myri.h>
#include <rtl_time.h>

int stop_net_manager = 0;

void *manage_network(void *foo)
{
  int i;

  /* Receive messages */
  for (i = 0;!stop_net_manager && i < 100; i++) {
    //block, waiting on receive event ie interrupt
    gm_recv_event_t *event = gm_receive (netPort);
    switch (GM_RECV_EVENT_TYPE(event)) {
	case GM_RECV_EVENT:
	case GM_HIGH_RECV_EVENT:
		{
        	  daqMessage *rcvData = (daqMessage *)gm_ntohp(event->recv.buffer);
        	  if (rcvData == 0) {
               		printk("Received zero pointer!\n");
               		break;
		  }
        	}
	case GM_NO_RECV_EVENT:
		break;
	default:
		// This is where sent message callbacks go
		gm_unknown (netPort, event);  /* gm_unknown calls the callback */
    }
    rtl_usleep(100000);
  }

  return 0;
}
