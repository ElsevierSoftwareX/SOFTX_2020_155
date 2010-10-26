/* 
TabBook.h - Public definitions for TabBook widget
	See TabBook documentation

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

/* Revision History:
$Log: TabBook.h,v $
Revision 1.1  2004/07/06 23:01:52  aivanov
Added real tabs.

Revision 1.1  1997/05/01 22:20:51  eric
Initial revision



$log
Initial
$log
*/
#ifndef _XcgTabBook_h
#define _XcgTabBook_h

/***********************************************************************
 *
 * TabBook Widget
 *
 ***********************************************************************/

/*
  New resource names/classes
*/

#define XcgNautoManage	 "autoManage"
#define XcgCAutoManage	"AutoManage"

#define XcgNactivePage	 "activePage"
#define XcgCActivePage	"ActivePage"

#define XcgNnewPageCallback "newPageCallback,"
#define XcgCNewPageCallback "NewPageCallback,"

/*
  Constraint resource names/classes
*/

#define XcgNresizeChild	 "resizeChild"
#define XcgCResizeChild	"ResizeChild"

#define XcgRESIZE_NONE 0
#define XcgRESIZE_VERTICAL 1
#define XcgRESIZE_HORIZONTAL 2
#define XcgRESIZE_BOTH (XcgRESIZE_VERTICAL | XcgRESIZE_HORIZONTAL)

#define XcgNanchorChild	 "anchorChild"
#define XcgCAnchorChild	 "AnchorChild"


#define XcgANCHOR_CENTER	0
#define XcgANCHOR_EAST		1
#define XcgANCHOR_NORTH		2
#define XcgANCHOR_NORTHEAST	3
#define XcgANCHOR_NORTHWEST	4
#define XcgANCHOR_SOUTH		5
#define XcgANCHOR_SOUTHEAST	6
#define XcgANCHOR_SOUTHWEST	7
#define XcgANCHOR_WEST		8


#if defined(__cplusplus) || defined(c_plusplus)
extern "C" { 
#endif

typedef struct _TabBookClassRec	*TabBookWidgetClass;
typedef struct _TabBookRec		*TabBookWidget;

extern WidgetClass xcgTabBookWidgetClass;

#define XcgTabBook_OPT_NO_CB 1
Boolean XcgTabBookSetActivePage(Widget w, int page, int option );
int XcgTabBookGetActivePage(Widget w );
Widget XcgTabBookGetActivePageWidget(Widget w);


#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

/*
 * Callback reasons.  Try to stay out of range of the Motif XmCR_* reasons.
 */
typedef enum _XcgTabBookgReasonType {
    XcgNewPage = 202,		/* New page is active */
} XcgTabBookReasonType;


/*
** Struct passed to application when called back
*/
typedef struct _XcgTabBookCallbackData
{
	XcgTabBookReasonType reason;	/* reason for callback */
	XEvent *event;			/* button event, NULL if emulated  */
	int prev_active_page;		/* 0 when initially called */
	int active_page;		/* new active page */
	Widget button;			/* the button widget which was
					   pressed (emulated or actual*/
	Boolean ret_veto;		/* caller may set to True to stop page change */
	int future1;
	void * future2;
} XcgTabBookCallbackData;

 
#endif /* _XcgTabBook_h */
