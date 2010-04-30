#include <stdio.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>

#include <stdio.h>
#include <stdlib.h>

#include <iostream.h>
#include "circ.hh"
#include "debug.h"
extern "C" {
#include "daqc.h"
}

int mtpl = 1;
int shift = 0;

#ifndef NDEBUG
// Controls volume of the debugging messages that is printed out
int _debug = 0;
#endif

Display *display;
int screen_num;
char *progname;

/* hostname of the host where the DAQD is running */
char *daqd_host = "fb0";

/* TCP port where the DAQD server is listening */
int daqd_port = 8088;

#define PLOT_WIDTH (channel.rate/16)
#define WINDOW_WIDTH 1024

#define LISTENER_PORT 9080

int listener_port = LISTENER_PORT;

void *interpreter (daq_t *);

circ_buffer *cb;

GC grcntxt1, grcntxt2, gc, clear_gc, set_gc, copy_gc;

daq_channel_t channel;

main(int argc, char *argv [])
{
  int t,i,j;
  Window win;
  char *display_name = 0;
  XEvent report;
  Pixmap pixmap;
  XSetWindowAttributes attr;
  unsigned long valuemask;
  XVisualInfo vTemplate;
  XVisualInfo *visualList;
  Colormap colormap;
  daq_t daq;
  int status;
  int chnum;
  daq_channel_t *ch;

  for (;;) {
    if (int pid = fork()) {
      wait(&i); i >>= 8;
      if (i == 5) exit(1);
      else if (i == 3) sleep (2);
    } else break;
  }

  progname = argv [0];
  if (!getenv("DISPLAY")) putenv("DISPLAY=:0.0");
  if ((display = XOpenDisplay (display_name)) == 0) {
    fprintf (stderr, "%s: cannot connect to X server %s\n", progname, XDisplayName (display_name));
    exit (5);
  }

  if (argc < 2) {
    fprintf (stderr, "Usage:  %s <LIGO signal name> [host] [gain] [shift]\n", progname);
    exit (5);
  }
  if (argc > 2) 
    daqd_host = argv [2];

  if (argc > 3)
    mtpl = atoi (argv [3]);

  if (argc > 4)
    shift = atoi (argv [4]);


  // open the connection
  memset ((void *) &daq, 0, sizeof (daq));
  daq_initialize (&daq, &listener_port, interpreter);
  if (status = daq_connect (&daq, daqd_host, daqd_port)) {
    exit (3);
  }

  // get the channels    
  ch = (daq_channel_t *) malloc (sizeof (daq_channel_t) * MAX_CHANNELS);
  if (! ch) {
    fprintf(stderr, "Out of memory on allocating channels struct");
    exit (5);
  }
    
  if (i = daq_recv_channels (&daq, ch, MAX_CHANNELS, &chnum)) {
    fprintf (stderr, "Couldn't receive channel data; errno=%d\n", i);
    exit (5);
  }
    
  DEBUG1(fprintf (stdout, "%d channels configured in DAQD\n", chnum));

  // Find the channels requested
  for (j = 0; j < chnum; j++) 
    if (! strcmp (argv [1], ch [j].name)) {
      channel = ch [j];
      printf("%s %d\n", ch [j].name, ch[j].rate);
      break;
    }
  if (j >= chnum) {
    fprintf (stderr, "Invalid channel `%s'\n", argv [1]);
    exit (5);
  }

  // create the circular buffer where the data will be
  // passed from the receiving thread into
  // this thread for displaying in the window.

  cb = new circ_buffer (0, 10 /* ten blocks */, channel.rate * channel.bps);

  char cbuf [1024];
  //  sprintf (cbuf, "start net-writer \"%d\" {\"%s\" %d};", listener_port, channel.name, PLOT_WIDTH);
  sprintf (cbuf, "start fast-writer \"%d\" {\"%s\"};", listener_port, channel.name);
  // start network writer
  if (daq_send (&daq, cbuf)) {
    exit(1);
    sprintf (cbuf, "start net-writer \"%d\" {\"%s\" %d};", listener_port, channel.name, PLOT_WIDTH);
    if (daq_send (&daq, cbuf))
      exit (5);
  }

  // at this point the data is starting to flow into the 
  // `interpreter' thread, which puts it into the circular buffer

  int cnum;
  if ((cnum = cb -> add_consumer ()) < 0) {
    abort ();
  }

#ifdef not_def
  for (;;) {
    int nb = cb -> get (cnum);

    DEBUG1(printf ("consumer: got block %d\n", nb));

    cb -> unlock (cnum);
  }
#endif

  // deal with the visuals
  screen_num = DefaultScreen (display);
  int shallow_visual_found = 0;
  
  // Try 1bit and 8bit visuals in order
#if 0
  for (int i = 1; i < 9 && ! shallow_visual_found; i += 7) {
    int visualsMatched;

    vTemplate.screen = screen_num;
    vTemplate.depth = i;
    visualList = XGetVisualInfo (display, VisualScreenMask | VisualDepthMask,
				 &vTemplate, &visualsMatched);
    if (visualsMatched) {
      colormap = XCreateColormap (display, RootWindow (display, screen_num),
				  visualList [0].visual, AllocAll);

      valuemask = CWBackPixel | CWBorderPixel | CWColormap;
      attr.colormap = colormap;
      attr.background_pixel = WhitePixel (display, screen_num);
      attr.border_pixel = BlackPixel (display, screen_num);
      win = XCreateWindow (display, RootWindow (display, screen_num),
			   0,0,WINDOW_WIDTH,512,0, vTemplate.depth,
			   InputOutput, visualList [0].visual, valuemask, &attr);
      pixmap = XCreatePixmap (display, win, WINDOW_WIDTH, 512, vTemplate.depth);
      shallow_visual_found = 1;
    }
  }
#endif

  if (!shallow_visual_found) {
    DEBUG1(fprintf (stderr, "No 1 bit or 8 bit visuals on this screen.\nUsing default visual %d bit deep. \n", DefaultDepth (display, screen_num)));

    attr.border_pixel = BlackPixel (display, screen_num);
    win = XCreateSimpleWindow (display, RootWindow (display, screen_num),
			 0,0,WINDOW_WIDTH,512,0,
			 WhitePixel (display, screen_num),
			 BlackPixel (display, screen_num));
    pixmap = XCreatePixmap (display, win, WINDOW_WIDTH, 512, DefaultDepth (display, screen_num));
  }

  // Set window properties

  XTextProperty windowName, iconName;
  char *sig = channel.name;
  if (XStringListToTextProperty (&sig, 1, &windowName) == 0) {
    abort ();
  }
  if (XStringListToTextProperty (&sig, 1, &iconName) == 0) {
    abort ();
  }

  XSetWMProperties (display, win, &windowName, &iconName, argv, argc, 0,0,0);

  {
    XGCValues values;
    int i, j;
    XPoint data [16][PLOT_WIDTH];
    for (i = 0; i < 16; i++)
      for (j = 0; j < PLOT_WIDTH; j++) {
	data [i][j].x = j*(WINDOW_WIDTH/PLOT_WIDTH);
	data [i][j].y = (short) (512.*rand()/(RAND_MAX+1.0));
      }

    values.foreground = WhitePixel (display, screen_num);
    values.background = BlackPixel (display, screen_num);
    values.fill_style = FillSolid;
    set_gc = XCreateGC (display, pixmap,
		    GCForeground | GCBackground | GCFillStyle,
		    &values);

    values.background = WhitePixel (display, screen_num);
    values.foreground = BlackPixel (display, screen_num);
    values.function = GXxor;
    gc = XCreateGC (display, pixmap,
		    GCForeground | GCBackground | GCFunction,
		    &values);

    values.background = WhitePixel (display, screen_num);
    values.foreground = BlackPixel (display, screen_num);
    grcntxt1 = copy_gc = XCreateGC (display, win,
		    GCForeground | GCBackground,
		    &values);

    values.background = BlackPixel (display, screen_num);
    values.foreground = WhitePixel (display, screen_num);
    grcntxt2 = clear_gc = XCreateGC (display, win,
			  GCForeground | GCBackground,
			  &values);

    XMapWindow (display, win);

    XSelectInput (display, win, ExposureMask);


    XFillRectangle (display, win, clear_gc, 0, 0, WINDOW_WIDTH, 512);
    XPoint d [PLOT_WIDTH];
    for (j = 0;;j++) {
      int nb = cb -> get (cnum);
      short *bp = (short *) cb -> block_ptr (nb);
      if (0)
	XDrawLines (display, win, clear_gc, d, PLOT_WIDTH, CoordModeOrigin);
      for (int i = 0; i < PLOT_WIDTH; i++) {
	d [i].x =  i*(WINDOW_WIDTH/PLOT_WIDTH);
        short v = 0;
	if (channel.data_type == 4) {
	  union { float f; short s[2]; } fs;
	  fs.s[0] = bp[2*i];
	  fs.s[1] = bp[2*i + 1];
	  v = (short) fs.f;
	} else v = ntohs(bp[i]);
	//d [i].y = (((int) (v * mtpl)) + 0x7fff + shift) / 0x7f;
	d [i].y = (((int) (-v * mtpl)) + 0x7fff + shift) / 0x7f;
      }
      cb -> unlock (cnum);
      //printf ("consumer: got block %d; %d %d\n", nb, d[0].x, d[0].y);
      XFillRectangle (display, win, grcntxt2, 0, 0, WINDOW_WIDTH, 512);
      XDrawLines (display, win, grcntxt1, d, PLOT_WIDTH, CoordModeOrigin);
      XSync(display, False);
    }

    XFillRectangle (display, pixmap, set_gc, 0, 0, WINDOW_WIDTH, 512);
 //   XClearWindow (display, win);
    t = time (0);
    for (j = 0;; j++) {
      for (i = 0; i < 16; i++) {
#ifdef not_def
	XDrawLines (display, pixmap, gc, data[i], PLOT_WIDTH, CoordModeOrigin);
	XCopyArea (display, pixmap, win, copy_gc, 0, 0, WINDOW_WIDTH, 512, 0, 0);
	XFillRectangle (display, pixmap, set_gc, 0, 0, WINDOW_WIDTH, 512);
#endif
	XFillRectangle (display, win, clear_gc, 0, 0, WINDOW_WIDTH, 512);
	//	XClearWindow (display, win);
	XDrawLines (display, win, copy_gc, data[i], PLOT_WIDTH, CoordModeOrigin);
      }
      //      printf ("%d\n", j);
      if (t < time (0)) {
	printf ("%d\n", j);
	j = -1;
	t = time (0);
      }
    }
  }

  while (1) {
    XNextEvent (display, &report);
    
    switch (report.type) {
    case Expose:
    default:
      break;
    }
  }
		       
}

// Data receiving thread

void *
interpreter (daq_t *daq)
{
  daq_recv_block_num (daq);
  int oread = 0;
  int seq_num = ~0;
  for (int j = 0;; j++)
    {
      int bread;

      if ((bread = daq_recv_block (daq)) < 0) {
	if (bread == -2) {
	  // reconfiguration;
	  int i;
	  printf("%s status 0x%x ", channel.name, daq->s[i].signal_status);
          if (daq->s[i].signal_status == 0) printf("(OK)");
	  else {
		if (daq->s[i].signal_status & 0xbad == 0xbad) printf("(BAD)");
		if (daq->s[i].signal_status & 0x1000) printf("(CRC)");
	  }
	 
	  printf("\n");
	  if (daq->s[0].signal_status) {
		grcntxt1 = clear_gc;
		grcntxt2 = copy_gc;
 	  } else {
		grcntxt1 = copy_gc;
		grcntxt2 = clear_gc;
	  }
	
	  continue;
	} else {
	  fprintf (stderr, "daq_recv_block() error; bread=%d\n", bread);
	  exit(1);
	}
      }

      if (daq -> tb -> seq_num != seq_num + 1) {
	printf ("%d block(s) dropped by the server\n", daq -> tb -> seq_num - seq_num - 1);
	printf ("seq_num=%d; status=%d\n", daq -> tb -> seq_num, seq_num);
      }

      seq_num = daq -> tb -> seq_num;

      /* Process received data here */
      //printf ("%d bytes read; time=%d--%d; seq_num=%d\n", bread, daq -> tb -> gps, daq -> tb -> gpsn, daq -> tb -> seq_num);

      circ_buffer_block_prop_t prop;
      prop.gps = daq -> tb -> gps;
      prop.gps_n = daq -> tb -> gpsn;

      cb -> put (daq -> tb -> data, bread, &prop);

      if (!bread)
	  break;

      oread += bread;

    }

  printf ("%d bytes received\n", oread);
  fflush (stdout);

  daq_recv_shutdown (daq);
  return NULL;
}
