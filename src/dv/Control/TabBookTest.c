/* 
Copyright 1996 COMPUTER GENERATION, INC.,

The software is provided "as is", without warranty of any kind, express
or implied, including but not limited to the warranties of
merchantability, fitness for a particular purpose and noninfringement.
In no event shall Computer Generation, inc. nor the author be liable for
any claim, damages or other liability, whether in an action of contract,
tort or otherwise, arising from, out of or in connection with the
software or the use or other dealings in the software.

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation.

Author:
Gary Aviv 
Computer Generation, Inc.,
gary@compgen.com
*/
/*
;+
TabBookTest	--	Test program for TabBook widget

Author:		Gary Aviv (gary@compgen.com)

Functions:	Provide a test fixture for TabBook widget
 
;-
*/

/*	Revision Information:

$Log: TabBookTest.c,v $
Revision 1.1  2004/07/06 23:01:52  aivanov
Added real tabs.


$log
Initial version.
$log

*/


#define 	VERSION "$Revision: 1.1 $"
#ident		"@(#)$Id: TabBookTest.c,v 1.1 2004/07/06 23:01:52 aivanov Exp $ $Revision: 1.1 $"

#define	PGMNAME		"TabBookTest"

/*----------- Include Files -------------*/

#include <stdio.h>
#include <stdlib.h>

#ifdef NeedFunctionPrototypes
#undef NeedFunctionPrototypes
#endif
#define NeedFunctionPrototypes 1

#ifdef NeedVarargsPrototypes
#undef NeedVarargsPrototypes
#endif
#define NeedVarargsPrototypes 1


#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/BulletinB.h>
#include <Xm/PushB.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#define BUTTON xmPushButtonWidgetClass
#define LABEL xmLabelWidgetClass

#if 0
#include <X11/Intrinsic.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Command.h>
#define BUTTON commandWidgetClass
#define LABEL labelWidgetClass
#endif
#include <X11/Xmu/Editres.h>
#include <X11/Shell.h>
#include <TabBook.h>

#define	RES_CONVERT( res_name, res_value) \
	XtVaTypedArg, (res_name), XmRString, (res_value), strlen(res_value) + 1

static void change_button_cb(Widget  widg, XtPointer ctx ,XtPointer   cb_data);
static void exit_cb(Widget  widg, XtPointer ctx ,XtPointer   cb_data);
static void go3_cb(Widget  widg, XtPointer ctx ,XtPointer   cb_data);
static Widget toplevel;
static int debug = 0;
static 	XtAppContext AppContext;
static 	Widget tabBook, page1  ;

main(int 	argc, char 	*argv[])
{
	Widget menubar, popup, menub, megatb, pulldown, main_shell,
		popup_main_shell, box;


  	toplevel = XtVaAppInitialize(&AppContext,
			"TabBookTest",
			NULL, 0, /*appl cmd line options*/
			&argc, argv, 
			NULL, /* fallback resources */
			NULL);	/* top level resources, VA args */

	popup_main_shell  = XtVaCreatePopupShell( "TabBook Demo",
			applicationShellWidgetClass, toplevel, NULL);
/*
	XtAddEventHandler(popup_main_shell, (EventMask)0, True, _XEditResCheckMessages, NULL);
*/
	main_shell = XtVaCreateManagedWidget( "main_window",
/* 			xmMainWindowWidgetClass, popup_main_shell, */
			xmBulletinBoardWidgetClass, popup_main_shell,
			NULL );

	tabBook = XtVaCreateManagedWidget("Tab", xcgTabBookWidgetClass, 
		main_shell, NULL);

	page1 = XtVaCreateManagedWidget("Page1", BUTTON, 
		tabBook, NULL);

	megatb = XtVaCreateManagedWidget("Page2", BUTTON, 
		tabBook, NULL);

	megatb = XtVaCreateManagedWidget("Page3", BUTTON, 
		tabBook, NULL);

	/* ------ Page 1 */
	box = XtVaCreateManagedWidget("RC_PAge1", xmRowColumnWidgetClass, 
		tabBook, NULL);

	XtVaCreateManagedWidget("Page 1", LABEL,
		box, NULL);

	megatb = XtVaCreateManagedWidget("Exit", BUTTON, 
		box, NULL);
	XtAddCallback(megatb,   
		XmNactivateCallback, 
		(XtCallbackProc) exit_cb, NULL );

	XtVaCreateManagedWidget("Button1", BUTTON, 
		box, NULL);

	megatb = XtVaCreateManagedWidget("GoTo Page 3", BUTTON, 
		box, NULL);
	XtAddCallback(megatb,   
		XmNactivateCallback, 
		(XtCallbackProc) go3_cb, NULL );

	/* ------ Page 2 */
	box = XtVaCreateManagedWidget("RC_Page2", xmRowColumnWidgetClass, 
		tabBook, XmNorientation, XmHORIZONTAL, NULL);

	XtVaCreateManagedWidget("Page 2", LABEL,
		box, NULL);
	XtVaCreateManagedWidget("NButton1", BUTTON, 
		box, NULL);

	XtVaCreateManagedWidget("NButton2", BUTTON, 
		box, NULL);
	megatb = XtVaCreateManagedWidget("Change Tab", BUTTON, 
		box, NULL);

	XtAddCallback(megatb,   
		XmNactivateCallback, 
		(XtCallbackProc) change_button_cb, NULL );

	/* ------ Page 3 */
	box = XtVaCreateManagedWidget("RC_Page3", xmRowColumnWidgetClass, 
		tabBook, XmNorientation, XmHORIZONTAL, NULL);

	XtVaCreateManagedWidget("Page 3", LABEL,
		box, NULL);

	XtVaCreateManagedWidget("XButton1", BUTTON, 
		box, NULL);

	XtVaCreateManagedWidget("XButton2", BUTTON, 
		box, NULL);


#if 0
	XtRealizeWidget(toplevel);
#else
	XtPopup(popup_main_shell ,XtGrabNone);

#endif

	XtAppMainLoop(AppContext);
}

static void exit_cb(Widget  widg, XtPointer ctx ,XtPointer   cb_data)
{
	exit(0);
}

static void go3_cb(Widget  widg, XtPointer ctx ,XtPointer   cb_data)
{
	XtVaSetValues(tabBook, XcgNactivePage, 3, NULL);
}

/*
  Demonstrate dynamic geometry change of one of the tab buttons; change
  its label
*/
static void change_button_cb(Widget  widg, XtPointer ctx ,XtPointer   cb_data)
{
	static char * new_label = "New Page 1";

	XtVaSetValues(page1, RES_CONVERT(XmNlabelString, new_label), NULL);
}

