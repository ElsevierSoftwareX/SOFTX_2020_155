#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/cursorfont.h>

#include <Xm/Xm.h>
#include <Xm/RepType.h>
#include <Xm/ArrowB.h>
#include <Xm/CascadeB.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrollBar.h>
#include <Xm/SelectioB.h>
#include <Xm/Separator.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/ToggleB.h>
#include <Xm/CascadeBG.h>
#include <Xm/LabelG.h>
#include <Xm/PushBG.h>
#include <Xm/SeparatoG.h>
#include <Xm/ToggleBG.h>
#include "TabBook.h"

#include "../Th/datasrv.h"
#include "globs.h"

/* Action Table */

/* Global Declarations */

XtAppContext app_context = (XtAppContext) 0 ;
Display     *display     = (Display *)    0 ;

struct DChList allChan[MAX_CHANNELS];

Cursor watch_cursor;

extern int plotterNum;

int
serverConnect()  {
    FILE *fp, *fp1, *fp2;
    int c, c1, c2, i, j, k;
    char tempname[1024];

    /* get channel/group information from Data Server and write to a file */
    sprintf ( tempname, "%s",  serverIP );
    if ( DataSimpleConnect(tempname, serverPort) ) {
      //fprintf ( stderr,"dc3: connection to Data Server failed\n" );
      return -1;
    }

    sprintf ( tempname, "/tmp/%schannelset0",  iniDir );
    /*printf ( "writing channel information to file: %s\n", tempname );*/
    fp = fopen(tempname, "w");
    if ( fp == NULL ) {
      fprintf ( stderr, "Can't open `%s' for writing\n", tempname );
      return -1;
    }

    sprintf ( tempname, "/tmp/%schannelset_dmt",  iniDir );
    /*printf ( "writing DMT channel information to file: %s\n", tempname );*/
    fp1 = fopen(tempname, "w");
    if ( fp1 == NULL ) {
      fprintf ( stderr, "Can't open `%s' for writing\n", tempname );
      return -1;
    }

    sprintf ( tempname, "/tmp/%schannelset_obs",  iniDir );
    /*printf ( "writing Obsolete channel information to file: %s\n", tempname );*/
    fp2 = fopen(tempname, "w");
    if ( fp2 == NULL ) {
      fprintf ( stderr, "Can't open `%s' for writing\n", tempname );
      return -1;
    }

    c = DataChanList(allChan, MAX_CHANNELS);
    c1 = 0;
    c2 = 0;
    for ( j=0; j<c; j++ ) { /* count DMT & Obsolete channels */
      if (allChan[j].group_num == 1000) 
	c1++;
      else if (allChan[j].group_num == 1001) 
	c2++;
    }
    fprintf ( fp, "%d\n", c-c1-c2 );
    fprintf ( fp1, "%d\n", c1 );
    fprintf ( fp2, "%d\n", c2 );
#if 0
    printf ( "total %d DMT channels\n", c1);
    printf ( "total %d Obsolete channels\n", c2);
#endif
    for ( j=0; j<c; j++ ) {
      FILE *file = 0;
      if (strcmp(allChan[j].units, " ")==0) strcpy(allChan[j].units, "None");

      if (allChan[j].group_num == 1000) /* DMT channel */ file = fp1;
      else if (allChan[j].group_num == 1001) /* Obsolete channel */ file = fp2;
      else file = fp;

      fprintf (file, "%s %d %s %d %d\n", allChan[j].name, allChan[j].rate, allChan[j].units, allChan[j].data_type, allChan[j].tpnum);
    }
    fclose(fp);
    fclose(fp1);
    fclose(fp2);
	  
    DataQuit();

    for ( j=0; j<16; j++ ) {
      int replace = 1;
      for (k = 0; k < c; k++) {
	if (!strcmp(chName[j], allChan[k].name)) {
	  replace = 0;
	  break;
	}
      }
      if (replace) {
        sprintf ( chName[j], "%s", allChan[j].name );
        chRate[j] = allChan[j].rate;
        sprintf ( chUnit[j], "%s", allChan[j].units );
      }
    }
    if ( c < 16 ) {
      for ( j=c; j<16; j++ ) {
        sprintf ( chName[j], "%s", allChan[j].name );
        chRate[j] = allChan[j].rate;
        sprintf ( chUnit[j], "%s", allChan[j].units );
      }
    }
    return 0;
}

void initApp(int argc, char **argv)
{
  int c, c1, c2, i, j;
  static char optstring[] = "a:b:x:s:p:w:r:f:hozFX";
  char   lst[201], ch[2];
  int  lcount=0;
  FILE   *fp, *fp1, *fp2;

  for ( i=1; i<argc; i++ ) {
    if ( (strcmp(argv[i], "h" ) == 0) || (strcmp(argv[i], "-h" ) == 0)) {
      printf("Usage: dataviewer\n"
	     "\t[-s server IP address (required unless using -F)]\n"
	     "\t[-x grace display IP address]\n"
	     "\t[-p server port]\n"
	     "\t[-r restore file] \n"
	     "\t[-F (frame file option)]\n"
	     "\t[-z (grace zoom option)] \n");
      
      printf("Usage Example: dataviewer -s fb0\n");
      printf("           or: dataviewer -x 131.215.114.15:1.0 -s red.ligo-wa -p 8090\n");
      printf("           or: dataviewer -F\n");
      exit(0);
    }
  }

  opterr = 0;
  serverPort = 8088;
  webPort = 8080;
  strcpy(serverIP, "fb0");
  { char *p; strcpy(displayIP, (p=getenv("DISPLAY"))?p:"0.0"); }
  strcpy(iniDir, "50");
  strcpy(origDir, "");
  strcpy(frameFileName, " ");
  mmMode = 0;
  restoreFile = 0;
  zoomflag = 0;
  nolimit = 0;
#ifdef __APPLE__
  extern char *optarg;
#endif
  while (  (c = getopt(argc, argv, optstring)) != -1 ) {
    /* options for starting dc3 */
    switch ( c ) {
    case 'X':
	plotterNum = 3; /* Use XMGR */
	break;
    case 'x': 
      strcpy(displayIP, optarg);
      break;
    case 's': 
      strcpy(serverIP, optarg);
      break;
    case 'p': 
      serverPort = atoi(optarg);
      break;
    case 'w': 
      webPort = atoi(optarg);
      break;
    case 'r': /* restore from file */
      strcpy(restoreFileName, optarg);
      restoreFile = 1;
      break;
    case 'F': /* file option with no file input */
      mmMode = 1;
      break;
    case 'f': /* file option with frame file input */
      mmMode = 1;
      strcpy(frameFileName, optarg);
      break;
    case 'a': /* for creating a unique dir in /tmp, may not be needed */
      strcpy(iniDir, optarg);
      break;
    case 'b': /* to get the path variable */
      strcpy(origDir, optarg);
      /*printf ( "Program directory: %s\n", origDir );*/
      break;
    case 'z': /* -free indicator for grace */
      zoomflag = 1;
      break;
    case 'o': /* for operator stations - no time limit*/
      nolimit = 1;
      break;
    case '?':
      printf ( "Invalid option found.\n" );
    }
           
  }
  if (strcmp(origDir, "") == 0) {
    fprintf ( stderr,"Error: the -b argument is required.\nUsually this means the shell variable $DVPATH is undefined.  Exit.\n" );
    exit(0);
  }
  strcat(origDir, "/");

#if 0
  /* check lockfile */
  sprintf ( lst, "%schecklock", origDir );
  if ( fork() == 0 ) {
    if ( fork() == 0 ) {
      execlp(lst,"checklock",(char*)NULL);
      perror("Can't check lockfile");
      exit(0);
    }
    else {
      exit(0);
    }
  }
  else {
    wait(0);
  }
  //sleep(1);
  fp = fopen("lockfile", "r");
  if (fp != NULL) {
    while( fgets(lst, 200, fp) != NULL) {
      lcount++;
    }
  }
  fclose(fp);
  if (lcount > 1) {
    if (lcount == 2)
      printf ( "Warning: There is another copy of Dataviewer running on this machine.\n");
    else
      printf ( "Warning: There are %d copies of Dataviewer running on this machine.\n",  lcount-1);
    printf ( "Continue? [y] or [n]\n" );
    scanf("%s",ch);
    switch ( ch[0] ) {
    case 'y': 
    case 'Y': 
      printf("Continue.\n");
      break;
    default: 
      printf("Exit.\n");
      exit(1);
      break;
    }
  }
#endif
	
#if 0
  if (mmMode == 0)
    printf ( "Start %s: <displayIP %s> <serverIP %s> <serverPort %d> <webPort %d>\n",argv[0],displayIP,serverIP,serverPort,webPort );
  else 
    printf ( "Start %s: <displayIP %s> <frame file %s>\n",argv[0],displayIP, frameFileName);
#endif
  /*
    getcwd(origDir, sizeof(origDir));
    if ( strcmp(origDir, "/") != 0 )
    strcat(origDir, "/");
  */
  strcat(iniDir, "DC/");

  if ( mmMode == 0 ) serverConnect();

  strcpy(version_n, "5.0");
  strcpy(version_m, "June");
  strcpy(version_y, "2004");
  /*printf ( "Dataviewer Version %s; %s %s\n", version_n, version_m, version_y );*/
}


doStuff() {
  int ac;
  Arg al[64];

#if 1
	if (v1) {
	ac = 0;
	COMPOUND_TEXT = XmInternAtom( XtDisplay( v1 ), "COMPOUND_TEXT", False );
	import_list[0] = COMPOUND_TEXT;
	XtSetArg (al[ac], XmNimportTargets, import_list);  ac++;
	XtSetArg (al[ac], XmNnumImportTargets, 1); ac++;
	XtSetArg (al[ac], XmNdropProc, drop_v1); ac++;
	XmDropSiteUpdate (v1, al, ac);
	}
	if (v2) {
	ac = 0;
	COMPOUND_TEXT = XmInternAtom( XtDisplay( v2 ), "COMPOUND_TEXT", False );
	import_list[0] = COMPOUND_TEXT;
	XtSetArg (al[ac], XmNimportTargets, import_list);  ac++;
	XtSetArg (al[ac], XmNnumImportTargets, 1); ac++;
	XtSetArg (al[ac], XmNdropProc, drop_v2); ac++;
	XmDropSiteUpdate (v2, al, ac);
	}
	if (v3) {
	ac = 0;
	COMPOUND_TEXT = XmInternAtom( XtDisplay( v3 ), "COMPOUND_TEXT", False );
	import_list[0] = COMPOUND_TEXT;
	XtSetArg (al[ac], XmNimportTargets, import_list);  ac++;
	XtSetArg (al[ac], XmNnumImportTargets, 1); ac++;
	XtSetArg (al[ac], XmNdropProc, drop_v3); ac++;
	XmDropSiteUpdate (v3, al, ac);
	}
	if (v4) {
	ac = 0;
	COMPOUND_TEXT = XmInternAtom( XtDisplay( v4 ), "COMPOUND_TEXT", False );
	import_list[0] = COMPOUND_TEXT;
	XtSetArg (al[ac], XmNimportTargets, import_list);  ac++;
	XtSetArg (al[ac], XmNnumImportTargets, 1); ac++;
	XtSetArg (al[ac], XmNdropProc, drop_v4); ac++;
	XmDropSiteUpdate (v4, al, ac);
	}
	if (v5) {
	ac = 0;
	COMPOUND_TEXT = XmInternAtom( XtDisplay( v5 ), "COMPOUND_TEXT", False );
	import_list[0] = COMPOUND_TEXT;
	XtSetArg (al[ac], XmNimportTargets, import_list);  ac++;
	XtSetArg (al[ac], XmNnumImportTargets, 1); ac++;
	XtSetArg (al[ac], XmNdropProc, drop_v5); ac++;
	XmDropSiteUpdate (v5, al, ac);
	}
	if (v6) {
	ac = 0;
	COMPOUND_TEXT = XmInternAtom( XtDisplay( v6 ), "COMPOUND_TEXT", False );
	import_list[0] = COMPOUND_TEXT;
	XtSetArg (al[ac], XmNimportTargets, import_list);  ac++;
	XtSetArg (al[ac], XmNnumImportTargets, 1); ac++;
	XtSetArg (al[ac], XmNdropProc, drop_v6); ac++;
	XmDropSiteUpdate (v6, al, ac);
	}
	if (v7) {
	ac = 0;
	COMPOUND_TEXT = XmInternAtom( XtDisplay( v7 ), "COMPOUND_TEXT", False );
	import_list[0] = COMPOUND_TEXT;
	XtSetArg (al[ac], XmNimportTargets, import_list);  ac++;
	XtSetArg (al[ac], XmNnumImportTargets, 1); ac++;
	XtSetArg (al[ac], XmNdropProc, drop_v7); ac++;
	XmDropSiteUpdate (v7, al, ac);
	}
	if (v8) {
	ac = 0;
	COMPOUND_TEXT = XmInternAtom( XtDisplay( v8 ), "COMPOUND_TEXT", False );
	import_list[0] = COMPOUND_TEXT;
	XtSetArg (al[ac], XmNimportTargets, import_list);  ac++;
	XtSetArg (al[ac], XmNnumImportTargets, 1); ac++;
	XtSetArg (al[ac], XmNdropProc, drop_v8); ac++;
	XmDropSiteUpdate (v8, al, ac);
	}
	if (v9) {
	ac = 0;
	COMPOUND_TEXT = XmInternAtom( XtDisplay( v9 ), "COMPOUND_TEXT", False );
	import_list[0] = COMPOUND_TEXT;
	XtSetArg (al[ac], XmNimportTargets, import_list);  ac++;
	XtSetArg (al[ac], XmNnumImportTargets, 1); ac++;
	XtSetArg (al[ac], XmNdropProc, drop_v9); ac++;
	XmDropSiteUpdate (v9, al, ac);
	}
	if (v10) {
	ac = 0;
	COMPOUND_TEXT = XmInternAtom( XtDisplay( v10 ), "COMPOUND_TEXT", False );
	import_list[0] = COMPOUND_TEXT;
	XtSetArg (al[ac], XmNimportTargets, import_list);  ac++;
	XtSetArg (al[ac], XmNnumImportTargets, 1); ac++;
	XtSetArg (al[ac], XmNdropProc, drop_v10); ac++;
	XmDropSiteUpdate (v10, al, ac);
	}
	if (v11) {
	ac = 0;
	COMPOUND_TEXT = XmInternAtom( XtDisplay( v11 ), "COMPOUND_TEXT", False );
	import_list[0] = COMPOUND_TEXT;
	XtSetArg (al[ac], XmNimportTargets, import_list);  ac++;
	XtSetArg (al[ac], XmNnumImportTargets, 1); ac++;
	XtSetArg (al[ac], XmNdropProc, drop_v11); ac++;
	XmDropSiteUpdate (v11, al, ac);
	}
	if (v12) {
	ac = 0;
	COMPOUND_TEXT = XmInternAtom( XtDisplay( v12 ), "COMPOUND_TEXT", False );
	import_list[0] = COMPOUND_TEXT;
	XtSetArg (al[ac], XmNimportTargets, import_list);  ac++;
	XtSetArg (al[ac], XmNnumImportTargets, 1); ac++;
	XtSetArg (al[ac], XmNdropProc, drop_v12); ac++;
	XmDropSiteUpdate (v12, al, ac);
	}
	if (v13) {
	ac = 0;
	COMPOUND_TEXT = XmInternAtom( XtDisplay( v13 ), "COMPOUND_TEXT", False );
	import_list[0] = COMPOUND_TEXT;
	XtSetArg (al[ac], XmNimportTargets, import_list);  ac++;
	XtSetArg (al[ac], XmNnumImportTargets, 1); ac++;
	XtSetArg (al[ac], XmNdropProc, drop_v13); ac++;
	XmDropSiteUpdate (v13, al, ac);
	}
	if (v14) {
	ac = 0;
	COMPOUND_TEXT = XmInternAtom( XtDisplay( v14 ), "COMPOUND_TEXT", False );
	import_list[0] = COMPOUND_TEXT;
	XtSetArg (al[ac], XmNimportTargets, import_list);  ac++;
	XtSetArg (al[ac], XmNnumImportTargets, 1); ac++;
	XtSetArg (al[ac], XmNdropProc, drop_v14); ac++;
	XmDropSiteUpdate (v14, al, ac);
	}
	if (v15) {
	ac = 0;
	COMPOUND_TEXT = XmInternAtom( XtDisplay( v15 ), "COMPOUND_TEXT", False );
	import_list[0] = COMPOUND_TEXT;
	XtSetArg (al[ac], XmNimportTargets, import_list);  ac++;
	XtSetArg (al[ac], XmNnumImportTargets, 1); ac++;
	XtSetArg (al[ac], XmNdropProc, drop_v15); ac++;
	XmDropSiteUpdate (v15, al, ac);
	}
	if (v16) {
	ac = 0;
	COMPOUND_TEXT = XmInternAtom( XtDisplay( v16 ), "COMPOUND_TEXT", False );
	import_list[0] = COMPOUND_TEXT;
	XtSetArg (al[ac], XmNimportTargets, import_list);  ac++;
	XtSetArg (al[ac], XmNnumImportTargets, 1); ac++;
	XtSetArg (al[ac], XmNdropProc, drop_v16); ac++;
	XmDropSiteUpdate (v16, al, ac);
	}
#endif
}

void waiter(int pid) {
   signal(SIGCHLD, waiter);
   wait(0);
}

int main(int argc, char **argv)
{
	signal(SIGCHLD, waiter);
	/* Enable Localisation of the Application */

	XtSetLanguageProc((XtAppContext) 0, (XtLanguageProc) 0, (XtPointer) 0) ;

	/* Initialise the X Toolkit */

	XtToolkitInitialize ();

	/* Create a Global Application Context */

	app_context = XtCreateApplicationContext ();

        /* Action Procedure Registration */

        /*XtAppAddActions(app_context, actionsTable, XtNumber(actionsTable)) ;*/

	/* Open the Display */

	display = XtOpenDisplay(app_context, (String) 0, argv[0], "DataViewer",
	                       (XrmOptionDescRec *) 0, 0,
	                       &argc, argv);
	if (display == (Display *) 0) {
		printf("%s: can't open display, exiting...\n", argv[0]);
		exit (-1);
	}

	initApp(argc, argv);

	/* This converter is not registered internally by Motif */

	XmRepTypeInstallTearOffModelConverter();

	/* Call the Create Procedures for the Dialogs in the Application */

	create_shell1 ( display, argv[0], argc, argv );
{
	int ac = 0;
	Arg al[10];
	        XtSetArg(al[ac], XcgNresizeChild, 3); ac++;
        XtSetArg(al[ac], XcgNanchorChild, 4); ac++;
	XtSetValues(displayForm, al, ac);
}
	create_frame_win ( shell1 );
	create_saveWin( shell1 );
	create_restoreWin( shell1 );
	create_helpWin( shell1 );
	create_errorDialog( shell1 );

	doStuff();

        /* Set server in the text entry field */
        //XmTextFieldSetString(serverIpText, "");
        XmTextFieldSetString(serverIpText, serverIP);
	{
	  char buf[128];
	  sprintf(buf, "%d", serverPort);
          //XmTextFieldSetString(serverPortText, "");
          XmTextFieldSetString(serverPortText, buf);
	}

	/* Set plot display in the text entry field */
	//XmTextFieldSetString(plotDisplay, "");
	XmTextFieldSetString(plotDisplay, displayIP);

        if (plotterNum == 3) 
  	  XmToggleButtonSetState(xmgrToggle, TRUE, TRUE);

 	watch_cursor = XCreateFontCursor(display, XC_watch);

	/* Unregister drop sites */
	unregisterDropSites();

	/* Display the Primary Application Shell */
	XtRealizeWidget (shell1);

	{
	  extern void initializeWindows();
	  initializeWindows();
	}

	/* Entering X Main Loop... */

	XtAppMainLoop (app_context);

	/* NOTREACHED */

	exit (0);
}

