#if MX_KERNEL == 1
#include <myriexpress.h>
#endif

/*
  Two remote IPC parts will be defined, one input, another output.
  Single input or output connection.
  Part will have a remote node name specified (hostname).
  Part will also have remote address specified (e.g. 0, 1, 2). Sizeof double.

  The code will establish connections to all nodes specified in IPC parts upon startup
  and will try to reestablish all lost connections at the end of a cycle.

*/

// This function will transmit all signal buffers to all nodes we have assigned
void mx_send() {};

/*
  Data receiving will be a simple access into a receive buffer (handled by the RCG).
*/
