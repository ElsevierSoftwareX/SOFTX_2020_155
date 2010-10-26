
/* TabBook widget private definitions */

#ifndef _XcgTabBookP_h
#define _XcgTabBookP_h

#include "TabBook.h"
#include <X11/ConstrainP.h>

#ifdef MOTIF
#include <Xm/XmP.h>
# if XmVersion >= 1002	/* If motif version >= 1.2 */
#   include <Xm/ManagerP.h>
# endif
#include <Xm/BulletinBP.h>
#include <Xm/PrimitiveP.h>   
#endif

#define XtInheritLayout ((Boolean (*)())_XtInherit)

typedef struct {
  int dummy;
} TabBookClassPart;


typedef struct _TabBookClassRec {
    CoreClassPart	core_class;
    CompositeClassPart	composite_class;
    ConstraintClassPart	constraint_class;
#ifdef MOTIF
    XmManagerClassPart  manager_class;
    XmBulletinBoardClassPart    bulletin_class;
#endif
    TabBookClassPart tabBook_class;
} TabBookClassRec;

extern TabBookClassRec tabBookClassRec;

typedef struct _TabBookPart 
{
	int y_margin ;	/* reserved margin around each pushbutton ... */
	int x_margin ;	/* Used to draw Tab around it */
	int x_spacing;	/* x space between each button */
	int x_angle;	/* space on the right to draw andled line */
	int border;	/* border around entire TabBook */
	Dimension button_bot;	/* largest button goes this far */
	Dimension cur_width;
	Dimension cur_height;
	Boolean layout_completed;	/* layout was done at least once */
	Boolean managed_before;		/* manged was done at least once */
	Boolean auto_manage;	/* manager children are automatically mapped when appropriate */
	Widget active_tab;	/* the current page's tab Button */
	Widget active_manager;	/* the current page's manager widget */
	int active_page;	/* 1,2,.. */
	int total_buttons;
	int total_managers;
	int old_active_page;	/* 1,2,.. 0 means none, i.e. init */
	GC drawBackground_GC ;	/* the drawing area background */
	Pixel drawBackground_color;	/* ... and its color */
	GC drawForeground_GC ;	/* the drawing area foreground */
	Pixel drawForeground_color;	/* ... and its color */
	GC bot_shadow_GC;	/* for drawing lines, bottom shadow */
	Pixel bot_shadow_color;	/* ... and its color  */
	GC top_shadow_GC;	/* for drawing lines, top shadow */
	Pixel top_shadow_color;	/* ... and its color  */
	XtCallbackList newPageCallback;
	Widget clip;		/* clip the manager children */
} TabBookPart;

typedef struct _TabBookRec {
    CorePart		core;
    CompositePart	composite;
    ConstraintPart	constraint;
#ifdef MOTIF
    XmManagerPart  manager;
    XmBulletinBoardPart	bulletin_board;
#endif
    TabBookPart	tabBook;
} TabBookRec;


typedef struct _TabBookConstraintPart 
{
	int resize_child;	/* constraint resources */
	int anchor_child;
} TabBookConstraintPart;


typedef struct _TabBookConstraintRec {
    TabBookConstraintPart	tabBook;
} TabBookConstraintRec, *TabBookConstraints;


#endif /* _XcgTabBookP_h */
