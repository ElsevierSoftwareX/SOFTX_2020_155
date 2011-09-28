

extern void drop_v1(Widget, XtPointer, XtPointer) ;
extern void drop_v2(Widget, XtPointer, XtPointer) ;
extern void drop_v3(Widget, XtPointer, XtPointer) ;
extern void drop_v4(Widget, XtPointer, XtPointer) ;
extern void drop_v5(Widget, XtPointer, XtPointer) ;
extern void drop_v6(Widget, XtPointer, XtPointer) ;
extern void drop_v7(Widget, XtPointer, XtPointer) ;
extern void drop_v8(Widget, XtPointer, XtPointer) ;
extern void drop_v9(Widget, XtPointer, XtPointer) ;
extern void drop_v10(Widget, XtPointer, XtPointer) ;
extern void drop_v11(Widget, XtPointer, XtPointer) ;
extern void drop_v12(Widget, XtPointer, XtPointer) ;
extern void drop_v13(Widget, XtPointer, XtPointer) ;
extern void drop_v14(Widget, XtPointer, XtPointer) ;
extern void drop_v15(Widget, XtPointer, XtPointer) ;
extern void drop_v16(Widget, XtPointer, XtPointer) ;

int  optind, opterr;
char displayIP[80], serverIP[80];
char origDir[1024], iniDir[80];
int  serverPort, webPort, mmMode; /* 0-server, 1-tocfile, 2-frame dir */
 
char chName[16][MAX_LONG_CHANNEL_NAME_LENGTH], chUnit[16][80], frameFileName[240], restoreFileName[240];
int  restoreFile;
int  chRate[16];
 
char version_n[8];  /* version number */
char version_m[8], version_y[8];  /* version date */
int  zoomflag, /* -free indicator for grace  */ nolimit;
 
Atom COMPOUND_TEXT;
Atom import_list[1];

#include "dc3.h"
