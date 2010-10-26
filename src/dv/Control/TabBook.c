/* 
TabBook.h - TabBook widget (Like Motif Notebook)
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

/*
;+
TabBook.c	-	TabBook widget (Like Motif Notebook)

MODULE NAME:	$RCSfile: TabBook.c,v $ 

AUTHOR:		$Author: aivanov $

FUNCTIONS:	Boolean XcgTabBookSetActivePage(Widget w, int page, int option );
		int XcgTabBookGetActivePage(Widget w );
		Widget XcgTabBookGetActivePageWidget(Widget w);

USAGE:		See Actual Functions For Description
;-
*/

/* Revision History:

$Log: TabBook.c,v $
Revision 1.1  2004/07/06 23:01:52  aivanov
Added real tabs.

Revision 1.3  1997/05/30 12:28:24  gary
Guard against realizing with no children

Revision 1.2  1997/04/25 16:40:04  gary
Chenged default of XcgNanchorChild from XcgANCHOR_NORTHWEST to XcgANCHOR_NORTH

Revision 1.1  1997/04/10 18:16:25  jeff
Initial revision


$log
Guard against realizing with no children
$log
*/

#define 	VERSION "$Revision: 1.1 $"
#ident		"@(#)$Id: TabBook.c,v 1.1 2004/07/06 23:01:52 aivanov Exp $ $Revision: 1.1 $"



#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#ifdef MOTIF
#include <Xm/BulletinB.h>
#include <Xm/BulletinBP.h>
#include <Xm/PushB.h>
#else
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Command.h>
#endif

/*
#include <X11/Xmu/Converters.h>
#include <X11/Xmu/CharSet.h>
#include <X11/Xaw/XawInit.h>
*/

#include "TabBookP.h"

/* debugging */
#define TRACE_ON(a) fprintf a 
#define TRACE_OFF(a) 

#define CheckWidgetClass(routine) \
	if (XtClass(w) != xcgTabBookWidgetClass) \
		wrong_widget(routine)

/* Private Definitions */

static void ResizeAllManagers( TabBookWidget cw );
static void FrameInsideArea(TabBookWidget cw, Boolean erase);
static void draw_tab_bottom(TabBookWidget cw , Widget child, Boolean active);
static void draw_tab_for_button(TabBookWidget cw , Widget child);
static void create_GC(TabBookWidget cw );
static void InsertChild(Widget w);
static void ExposeMeth(Widget w, XEvent *event, Region region );
static void ClassInitialize(), ClassPartInitialize(), Initialize();
static void Resize(Widget w);

static Boolean ConstraintSetValues(Widget current, Widget request, Widget new,
    ArgList args,Cardinal *num_args);

static Boolean SetValues(Widget current, Widget request, Widget new,
    ArgList args,Cardinal *num_args);

static XtGeometryResult GeometryManager(Widget w,XtWidgetGeometry *request,
    XtWidgetGeometry *reply);

XtGeometryResult TabBookPreferredGeometry(Widget widget,
    XtWidgetGeometry *request, XtWidgetGeometry *reply);

static void ChangeManaged(Widget w);
static void Realize(Widget w, XtValueMask * valueMask, 
	XSetWindowAttributes * attributes );
static void DoLayout(TabBookWidget cw);
static void unmanage_all_Pages(TabBookWidget cw );
static void activate_cb(Widget  w, XtPointer fieldp ,XtPointer   cb_data);
static Widget find_nth_button(TabBookWidget cw , int page);
static void wrong_widget(char * routine);

#define offset(field) XtOffsetOf(TabBookRec, field)
static XtResource resources[] =
{
	{XcgNautoManage, XcgCAutoManage, XtRBoolean , sizeof(Boolean),
		offset(tabBook.auto_manage), XtRImmediate,  (XtPointer) True},
	{XcgNactivePage, XcgCActivePage, XtRInt , sizeof(int),
		offset(tabBook.active_page), XtRImmediate,  (XtPointer) 1},

	{ XcgNnewPageCallback, XcgCNewPageCallback, XmRCallback,sizeof(XtCallbackList),
	  offset(tabBook.newPageCallback), XmRCallback, NULL },
};
#undef offset

#define offset(field) XtOffsetOf(TabBookConstraintRec, field)
static XtResource constraint_resources[] =
{
	{XcgNresizeChild, XcgCResizeChild, XtRInt , sizeof(int),
		offset(tabBook.resize_child), XtRImmediate,  (XtPointer) XcgRESIZE_NONE},
	{XcgNanchorChild, XcgCAnchorChild, XtRInt , sizeof(int),
		offset(tabBook.anchor_child), XtRImmediate,  (XtPointer) XcgANCHOR_NORTH},
};
#undef offset




TabBookClassRec tabBookClassRec = {
  { /* core_class fields */
#ifdef MOTIF
    /* superclass         */    (WidgetClass) &xmBulletinBoardClassRec, 
#else
				(WidgetClass) &constraintClassRec,
#endif
    /* class_name         */    "TabBook",
    /* widget_size        */    sizeof(TabBookRec),
    /* class_initialize   */    ClassInitialize,
    /* class_part_init    */    ClassPartInitialize,
    /* class_inited       */    FALSE,
    /* initialize         */    Initialize,
    /* initialize_hook    */    NULL,
    /* realize            */    XtInheritRealize, /* Realize, */
    /* actions            */    NULL,
    /* num_actions        */    0,
    /* resources          */    (XtResourceList)resources,
    /* num_resources      */    (Cardinal)XtNumber(resources),
    /* xrm_class          */    NULLQUARK,
    /* compress_motion    */    TRUE,
    /* compress_exposure  */    TRUE,
    /* compress_enterleave*/    TRUE,
    /* visible_interest   */    FALSE,
    /* destroy            */    NULL,
    /* resize             */    Resize,
    /* expose             */    ExposeMeth,
    /* set_values         */    SetValues,
    /* set_values_hook    */    NULL,
    /* set_values_almost  */    XtInheritSetValuesAlmost,
    /* get_values_hook    */    NULL,
    /* accept_focus       */    NULL,
    /* version            */    XtVersion,
    /* callback_private   */    NULL,
    /* tm_table           */    NULL,
    /* query_geometry     */	TabBookPreferredGeometry,
    /* display_accelerator*/	XtInheritDisplayAccelerator,
    /* extension          */	NULL
  },
  { /* composite_class fields */
    /* geometry_manager   */   GeometryManager,
    /* change_managed     */   ChangeManaged,
    /* insert_child       */   InsertChild, /* XtInheritInsertChild,*/
    /* delete_child       */   XtInheritDeleteChild,
    /* extension          */   NULL
  },
  { /* constraint_class fields */
    /* subresourses       */   (XtResourceList)constraint_resources,
    /* subresource_count  */   (Cardinal)XtNumber(constraint_resources),
    /* constraint_size    */   sizeof(TabBookConstraintRec), 
    /* initialize         */   NULL, /*ConstraintInitialize,*/
    /* destroy            */   NULL,
    /* set_values         */   ConstraintSetValues,
    /* extension          */   NULL
  },
#ifdef MOTIF
    { /*** xmManager-Class ***/
    /* translations                 */	XtInheritTranslations,
    /* syn_resources                */	NULL,
    /* num_syn_resources            */	0,
    /* syn_constraint_resources     */	NULL,
    /* num_syn_constraint_resources */	0,
    /* parent_process		    */	XmInheritParentProcess,
    /* extension		    */	NULL
    }, 
  { /* Bulletin Board     */
    /* always_install_accelerators */ False,
    /* geo_matrix_create  */	XmInheritGeoMatrixCreate, /*NULL,*/
    /* focus_moved_proc   */	XmInheritFocusMovedProc,
    /* extension	  */    NULL
  },
#endif
  { /* TabBook_class fields */
    /* dummy              */   0
  }
};

WidgetClass xcgTabBookWidgetClass = (WidgetClass)&tabBookClassRec;

/****************************************************************
 *
 * Private Procedures
 *
 ****************************************************************/

static void ClassInitialize()
{
}

static void ClassPartInitialize(class)
    WidgetClass class;
{
}

/* ARGSUSED */
static void Initialize(Widget request, Widget new, ArgList args, Cardinal num_args)
{
	TabBookWidget cw = (TabBookWidget)new;
	Pixel select_color ;

	cw->tabBook.managed_before = False;
	cw->tabBook.old_active_page = 0;
	cw->tabBook.total_managers = 0;
	cw->tabBook.total_buttons  = 0;
	cw->tabBook.cur_height = 0;
	cw->tabBook.cur_width = 0;

	cw->tabBook.layout_completed = False;
	cw->tabBook.active_manager = NULL;
	cw->tabBook.active_tab = NULL;
	cw->tabBook. y_margin = 2;	/* reserved margin around each pushbutton ... */ 
	cw->tabBook. x_margin = 2;	/* Used to draw Tab around it */
	cw->tabBook. x_spacing = 2;	/* x space between each button */
	cw->tabBook. x_angle = 4;	/*  space on the right to draw andled line */
	cw->tabBook.border = cw->tabBook.x_margin + 2 ;		/* border around TabBook inside area  */ 
	cw->tabBook.clip = NULL;
	/* compute all the colors we will need */
	cw->tabBook.drawBackground_color = cw->core.background_pixel;
	XmGetColors(XtScreen(cw), cw->core.colormap,
		cw->tabBook.drawBackground_color,
		&cw->tabBook.drawForeground_color,
		&cw->tabBook.top_shadow_color,
		&cw->tabBook.bot_shadow_color,
		&select_color );

	create_GC(cw );
	/* create the clip window in which all manager children will be
	   re-parented */
    	cw->tabBook.clip = XtVaCreateManagedWidget("TabBookClip",
		xmBulletinBoardWidgetClass, new,
			XmNmarginHeight,0, XmNmarginWidth,0, NULL);
}

/*
 Creates the various graphic contexts we will need 
*/
static void create_GC(TabBookWidget cw )
{
	XtGCMask valuemask;
	XGCValues myXGCV;


	valuemask = GCForeground | GCBackground | GCLineWidth | GCFillStyle ;
	myXGCV.foreground = cw->tabBook.drawForeground_color;
	myXGCV.background = cw->tabBook.drawBackground_color;
	myXGCV.fill_style = FillSolid; 
	myXGCV.line_width = cw->tabBook.x_margin ;

	/* 	| GCJoinStyle ;
	myXGCV.join_style = JoinRound; */

	if (cw->tabBook.drawForeground_GC )
		XtReleaseGC((Widget) cw, cw->tabBook.drawForeground_GC );
	cw->tabBook.drawForeground_GC = XtGetGC((Widget)cw, valuemask, &myXGCV);

	myXGCV.foreground = cw->tabBook.drawBackground_color;
	if (cw->tabBook.drawBackground_GC )
		XtReleaseGC((Widget) cw, cw->tabBook.drawBackground_GC );
	cw->tabBook.drawBackground_GC = XtGetGC((Widget)cw, valuemask, &myXGCV);

	myXGCV.foreground = cw->tabBook.bot_shadow_color;
	if (cw->tabBook.bot_shadow_GC )
		XtReleaseGC((Widget) cw, cw->tabBook.bot_shadow_GC );
	cw->tabBook.bot_shadow_GC = XtGetGC((Widget)cw, valuemask, &myXGCV);

	myXGCV.foreground = cw->tabBook.top_shadow_color;
	if (cw->tabBook.top_shadow_GC )
		XtReleaseGC((Widget) cw, cw->tabBook.top_shadow_GC );
	cw->tabBook.top_shadow_GC = XtGetGC((Widget)cw, valuemask, &myXGCV);

}


static void Resize(Widget w)
{
	TabBookWidget cw = (TabBookWidget)w;
	Position x, y;
	Dimension width, height;
	Boolean bigger;

	TRACE_OFF((stderr,"Resize Old=%dx%d New=%dx%d \n",cw->tabBook.cur_width,cw->tabBook.cur_height,cw->core.width,cw->core.height )); 

	if (cw->tabBook.cur_height )
		FrameInsideArea(cw, True );	/* erase old frame */

	if (cw->core.height <= cw->tabBook.cur_height 
	&&  cw->core.width  <= cw->tabBook.cur_width )
		bigger = False;
	else
		bigger = True;

	cw->tabBook.cur_height = cw->core.height;
	cw->tabBook.cur_width = cw->core.width;
	/* new frame will be drawn when expose is next called unless it
	   got smaller */
	if (bigger == False)
		FrameInsideArea(cw, False);	/* redraw new frame */
	/* now resize all manager children */
	ResizeAllManagers( cw );
}

/*
Resize all managers to fit (or to enlarge out) in the TabBook
inside area
*/
static void ResizeAllManagers( TabBookWidget cw )
{
	XmBulletinBoardRec * clip = (XmBulletinBoardRec *) cw->tabBook.clip;
	WidgetList widget_list = clip->composite.children;
	Cardinal num_children = clip->composite.num_children;
	Cardinal n = 0;
	Dimension border = 2 * cw->tabBook.border;	
	Dimension inside_width = cw->tabBook.cur_width - border;
	Dimension inside_height = cw->tabBook.cur_height -
			cw->tabBook.button_bot - border;

	XtResizeWidget(cw->tabBook.clip, inside_width,
				inside_height  ,
				clip->core.border_width) ;
	XtMoveWidget(cw->tabBook.clip, cw->tabBook.border , 
			cw->tabBook.button_bot + cw->tabBook.border);


#define _TOP  0
#define _BOTTOM (inside_height - child->core.height)
#define _RIGHT (inside_width - child->core.width)
#define _LEFT 0
#define _CENTER_VERT  (inside_height - child->core.height)/2 
#define _CENTER_HORIZ (inside_width - child->core.width)/2

	for (n=0; n < num_children; n++)
	{
		Widget child = widget_list[n];
		if ( XtIsSubclass(child, xmManagerWidgetClass ) )
		{
			TabBookConstraints cw_c = (TabBookConstraints)
					child->core.constraints;
			switch (cw_c->tabBook.resize_child)
			{
#if 0
			    case XcgRESIZE_NONE:
				break;

			    case XcgRESIZE_BOTH:
#endif
		default:
			    XtResizeWidget(child, inside_width,inside_height,
				child->core.border_width) ;
			    break;

#if 0
			    case XcgRESIZE_HORIZONTAL:
			    XtResizeWidget(child, inside_width,
				child->core.height,
				child->core.border_width) ;
			    break;

			    case XcgRESIZE_VERTICAL:
			    XtResizeWidget(child, child->core.width,
				inside_height,
				child->core.border_width) ;
			    break;
#endif
				
			}
			switch (cw_c->tabBook.anchor_child)
			{
			    case XcgANCHOR_CENTER:
				XtMoveWidget(child, _CENTER_HORIZ, _CENTER_VERT);
			        break;

			    case XcgANCHOR_NORTH:
				XtMoveWidget(child, _CENTER_HORIZ, _TOP);
			        break;

			    case XcgANCHOR_EAST:
				XtMoveWidget(child, _RIGHT, _CENTER_VERT);
			        break;

			    case XcgANCHOR_SOUTH:
				XtMoveWidget(child, _CENTER_HORIZ, _BOTTOM);
			        break;

			    case XcgANCHOR_WEST:
				XtMoveWidget(child, _LEFT, _CENTER_VERT);
			        break;

			    case XcgANCHOR_NORTHEAST:
				XtMoveWidget(child, _RIGHT, _TOP);
			        break;

			    case XcgANCHOR_NORTHWEST:
				XtMoveWidget(child, _LEFT, _TOP);
			        break;

			    case XcgANCHOR_SOUTHEAST:
				XtMoveWidget(child, _RIGHT,_BOTTOM);
			        break;

			    case XcgANCHOR_SOUTHWEST:
				XtMoveWidget(child, _LEFT,_BOTTOM);
			        break;

			}
		}
	}
}

static XtGeometryResult GeometryManager(Widget w,XtWidgetGeometry *request,
    XtWidgetGeometry *reply)
{
	TabBookWidget cw = (TabBookWidget) XtParent(w);

	TRACE_OFF((stderr,"GeometryManager request=%dx%d\n",
		request->width, request->height )); 
	/* only honor changes in width or height */
	if (request->request_mode & ~(CWWidth | CWHeight | XtCWQueryOnly))
		return XtGeometryNo;
	if (request->request_mode & CWWidth )
		reply->width = request->width;
	else
		reply->width = w->core.width;

	if (request->request_mode & CWHeight )
		reply->height = request->height;
	else
		reply->height = w->core.height;
	if (reply->height == request->height
	&&  reply->width  == request->width)
		return XtGeometryNo;
	if (request->request_mode & XtCWQueryOnly)
		return XtGeometryYes;

	if (XtClass(w) == xmPushButtonWidgetClass )
	{
		reply->request_mode = CWWidth | CWHeight;
		XClearArea(XtDisplay(cw), XtWindow(cw), 0, 0,
			cw->tabBook.cur_width, cw->tabBook.button_bot , True);
		w->core.width = reply->width;
		w->core.height = reply->height;
		DoLayout(cw);
	}
	return(XtGeometryYes);
}


/* ARGSUSED */
static Boolean SetValues(Widget current, Widget request, Widget new,
    ArgList args,Cardinal *num_args)
{
	TabBookWidget cw_new = (TabBookWidget) new;
	TabBookWidget cw_cur = (TabBookWidget) current;

	if (cw_new->tabBook.active_page  != cw_cur->tabBook.active_page )
	{
		if (cw_new->tabBook.layout_completed )
		{
			Widget button = find_nth_button(cw_new, cw_new->tabBook.active_page  );
			/* emulate user pressing this button */
			if (button && XtIsRealized(new))
				activate_cb(button, NULL, NULL);
		}
	}

	return FALSE;
}

static Boolean ConstraintSetValues(Widget current, Widget request, Widget new,
    ArgList args,Cardinal *num_args)
{
	TabBookWidget cw_new = (TabBookWidget) new->core.parent ;
	TabBookWidget cw_cur = (TabBookWidget) current->core.parent ;
	TabBookConstraints cw_c = (TabBookConstraints) new->core.constraints;

  return( FALSE );
}

static void ChangeManaged(Widget w)
{
	TabBookWidget cw = (TabBookWidget)w;
	

	TRACE_OFF((stderr,"Change Managed\n"));

	DoLayout(cw);

	/* first time, unmanage all but active manager */
	if (!cw->tabBook.managed_before)
	{
		if (cw->tabBook.auto_manage )
			unmanage_all_Pages(cw); /* Unmanage all pages except active one */
		if (cw->tabBook.newPageCallback )
		{
			XcgTabBookCallbackData rcb_data;
			memset(&rcb_data, 0, sizeof(rcb_data));
			rcb_data.reason = XcgNewPage;	/* reason for callback */
			rcb_data.prev_active_page = 0;		/* 0 when initially called */
			rcb_data.active_page =  cw->tabBook.active_page; /* new active page */
			XtCallCallbackList((Widget)cw, cw->tabBook.newPageCallback,
				(XtPointer) &rcb_data);
		}
	}
	cw->tabBook.managed_before = True;
}


XtGeometryResult TabBookPreferredGeometry(Widget widget,
    XtWidgetGeometry *request, XtWidgetGeometry *reply)
{
	TabBookWidget cw = (TabBookWidget)widget;
	Dimension width /*, height */;

	TRACE_OFF((stderr,"Query Geometry\n"));
	if (! cw->tabBook.layout_completed)
		DoLayout(cw);

	request->request_mode &= CWWidth | CWHeight;
	reply->request_mode = CWWidth | CWHeight;
	reply->width = cw->tabBook.cur_width ;
	reply->height = cw->tabBook.cur_height ;

	/* called with NULL request */
	if (! request->request_mode )
	{
		if (reply->width != cw->core.width
		||  reply->height != cw->core.height)
		    return XtGeometryNo;	/* current and desired size the same */
		else
		    return XtGeometryAlmost;	/* this is our desired size */
	}
	
	/* called with non-NULL request */
	if ( request->width == reply->width &&
	    request->height == reply->height)
	    return XtGeometryYes;	/* requested change is correct */
	if (reply->width != cw->core.width
	||  reply->height != cw->core.height)
	    return XtGeometryNo;	/* we want to stay the size we are now */

	return XtGeometryAlmost;	/* this is our desired size */
}

/*
Layout the children 
*/
static void DoLayout(TabBookWidget cw)
{
	XmBulletinBoardRec * clip = (XmBulletinBoardRec *) cw->tabBook.clip;
	WidgetList widget_list = cw->composite.children;
	Cardinal num_children = cw->composite.num_children;
	Cardinal n = 0;
	XtWidgetGeometry size;
	Dimension max_x, max_y, low_button_y ;
	XtGeometryResult GResult ;
	Dimension width_return, height_return;
	int max_button_height = 0;

	max_x = max_y = 0;
	/* place all the buttons side by side */
	for (n=0; n < num_children; n++)
	{
		Widget child = widget_list[n];
		if ( XtClass(child) == xmPushButtonWidgetClass)
		{
			XtQueryGeometry(child, NULL, &size);
			if (max_x)	/* all but first button, space from its left neighbor */
				max_x += cw->tabBook.x_spacing;
			max_x += cw->tabBook.x_margin ;
			XtMoveWidget(child, max_x, cw->tabBook.y_margin );
			max_x += size.width + cw->tabBook.x_margin + cw->tabBook.x_angle ;
			if (max_button_height < size.height)
				max_button_height = size.height;
			if (max_y < size.height)
				max_y = size.height;	/* max height */
			continue;
		}
	}
	cw->tabBook.button_bot = low_button_y = max_y + cw->tabBook.y_margin ;
	low_button_y += cw->tabBook.border;	/* allow space for inside border */

	/* Set all buttons to the same height, the max height */
	for (n=0; n < num_children; n++)
	{
		Widget child = widget_list[n];
		if ( XtClass(child) == xmPushButtonWidgetClass)
		{
			XtResizeWidget(child, child->core.width, 
				max_button_height, child->core.border_width) ;
		}
	}

	/* now position all manager ancestors below buttons, all at x=0,
	   y=low_button_y (plus margin) and expand the TabBook width and
	   height to accomodate all these managers */
	widget_list = clip->composite.children;
	num_children = clip->composite.num_children;

	for (n=0; n < num_children; n++)
	{
		Widget child = widget_list[n];
		if ( XtIsSubclass(child, xmManagerWidgetClass ) )
		{
			XtQueryGeometry(child, NULL, &size);
			size.height += low_button_y ;
			if (max_y < size.height)
				max_y = size.height;	/* max height */
			size.width += cw->tabBook.border;
			if (max_x < size.width)
				max_x = size.width;	/* max width */
		}
	}

	cw->tabBook.cur_width = max_x + cw->tabBook.border ;
	cw->tabBook.cur_height = max_y + cw->tabBook.border ;	/* space for right border */
	cw->tabBook.layout_completed = True;

	if (cw->core.height != cw->tabBook.cur_height
	||  cw->core.width  != cw->tabBook.cur_width)
	{
		GResult = XtMakeResizeRequest((Widget)cw, cw->tabBook.cur_width, cw->tabBook.cur_height,
		       &width_return, &height_return);
		cw->tabBook.cur_height = cw->core.height;
		cw->tabBook.cur_width = cw->core.width;
	}
	ResizeAllManagers( cw );
}

/*
 Use standard method to chain Realize request. Use this opportunity
 to position child widgets. 
*/
static void Realize(Widget w, XtValueMask * valueMask, 
	XSetWindowAttributes * attributes )
{
	TabBookWidget new = (TabBookWidget) w;
	TRACE_OFF((stderr,"Realize\n"));

	(*(&coreClassRec)->core_class.realize)(w, valueMask, attributes );
}

/*
Draw (or erase) the  shadow around the inside area of the TabBook
*/
static void FrameInsideArea(TabBookWidget cw, Boolean erase)
{
	Dimension left_x, top_y, p_width, p_height, thickness;
	XPoint points[4];
	GC gc1, gc2;

	if (!XtIsRealized((Widget)cw))
		return;
	if (erase)
		gc1 = gc2 = cw->tabBook.drawBackground_GC ;
	else
	{
		gc1 = cw->tabBook.top_shadow_GC ;
		gc2 = cw->tabBook.bot_shadow_GC ;
	}
	/* draw shadow around inside area */
	thickness = cw->tabBook.x_margin ;	
	left_x = 0 + thickness/2 ;
	top_y = cw->tabBook.button_bot + thickness/2 ;
	p_width = cw->tabBook.cur_width ;
	p_height = cw->tabBook.cur_height - cw->tabBook.button_bot ;

	/* note that lines are drawn centered on their (x,y) pixel. we
	   want to specify the outside pixel to contain the line so we
	   have to add in the thickness/2 
	*/
	points[0].x = left_x ;
	points[0].y = top_y + p_height;	/* lower left */
	points[1].x = 0;
	points[1].y = - p_height;	/* upper left */
	points[2].x =  p_width - thickness;
	points[2].y = 0;	/* upper right */
	/* draw left and top edges */
	XDrawLines(XtDisplay(cw), XtWindow(cw), gc1 ,
		points, 3, CoordModePrevious);

	points[0].x = left_x + p_width - thickness;
	points[0].y = top_y ;	/* upper right */
	points[1].x = 0;
	points[1].y = p_height - thickness;	/* lower right*/
	points[2].x = - p_width;
	points[2].y =  0;	/* lower left */
	/* draw right and bottom edge */
	XDrawLines(XtDisplay(cw), XtWindow(cw), gc2 ,
		points, 3, CoordModePrevious);
}


/*
Draw shadows around tab buttons and the inside area
*/
static void draw_frames(TabBookWidget cw )
{
	WidgetList widget_list = cw->composite.children;
	Cardinal num_children = cw->composite.num_children;
	Cardinal n = 0;

	/* draw shadow around each button */
	for (n=0; n < num_children; n++)
	{
		Widget child = widget_list[n];
		if ( XtClass(child) == xmPushButtonWidgetClass)
		{
			draw_tab_for_button(cw, child);
		}
	}

	FrameInsideArea(cw, False);
	/* activate the current page's tab */
	draw_tab_bottom(cw , cw->tabBook.active_tab, True);
}

static void ExposeMeth(Widget w, XEvent *xevent, Region region)
{
	TabBookWidget cw = (TabBookWidget) w;
	XExposeEvent * event = (XExposeEvent *) xevent;
	WidgetList widget_list = cw->composite.children;
	Cardinal num_children = cw->composite.num_children;
	Cardinal n = 0;

	TRACE_OFF((stderr,"Expose %d \n", event->count)); 
	if (!XtIsRealized(w))
		return;
	if (event->count != 0)
	    return;

	draw_frames(cw);
}

/*
Draw the Tab looking border around the child widget which is a push button
*/
static void draw_tab_for_button(TabBookWidget cw , Widget child)
{
	XtWidgetGeometry size;
	Dimension left_x, top_y, p_width, p_height, thickness;
	XPoint points[4];

	if (!child)
		return;
	if (!XtIsRealized((Widget)cw))
		return;
	thickness = cw->tabBook.x_margin ;	
	left_x = child->core.x - cw->tabBook.x_margin + thickness/2 ;
	top_y = child->core.y - cw->tabBook.y_margin + thickness/2 ;
	p_width = child->core.width + 2*(cw->tabBook.x_margin) ;
	p_height = cw->tabBook.button_bot - child->core.y +1;

	/* note that lines are drawn centered on their (x,y) pixel. we
	   want to specify the outside pixel to contain the line so we
	   have to add in the thickness/2 
	*/

	points[0].x = left_x ;
	points[0].y = top_y + p_height;	/* lower left */
	points[1].x = 0;
	points[1].y = - p_height;	/* upper left */
	points[2].x =  p_width - thickness;
	points[2].y = 0;	/* upper right */
	/* draw left and top edges */
	XDrawLines(XtDisplay(cw), XtWindow(cw), cw->tabBook.top_shadow_GC ,
		points, 3, CoordModePrevious);

	points[0].x = left_x + p_width - thickness;
	points[0].y = top_y ;	/* upper right */
	points[1].x = cw->tabBook.x_angle ;
	points[1].y = cw->tabBook.x_angle ;	/* angle out */
	points[2].x = 0;
	points[2].y =  p_height - cw->tabBook.x_angle ;	/* lower right */
	/* draw right edge */
	XDrawLines(XtDisplay(cw), XtWindow(cw), cw->tabBook.bot_shadow_GC ,
		points, 3, CoordModePrevious);

}

/*
We draw the bottom line of the Tab to either activate or deactivate the
page
*/
static void draw_tab_bottom(TabBookWidget cw , Widget child, Boolean active)
{
	XtWidgetGeometry size;
	Dimension left_x, top_y, p_width, p_height, thickness;
	XPoint points[4];

	if (!XtIsRealized((Widget)cw) || child == (Widget) 0)
		return;
	thickness = cw->tabBook.x_margin ;	
	left_x = child->core.x - cw->tabBook.x_margin + 1 + thickness/2 ;
	top_y = child->core.y - cw->tabBook.y_margin + thickness/2 ;
	p_width = child->core.width + 2*(cw->tabBook.x_margin) ;
	p_height = cw->tabBook.button_bot - child->core.y + 1 + thickness/2 ;

	points[0].x = left_x ;
	points[0].y = top_y + p_height;	/* lower left */
	points[1].x = p_width ;
	points[1].y = 0;	/* lower right */

	XDrawLines(XtDisplay(cw), XtWindow(cw), 
		active ? cw->tabBook.drawBackground_GC : cw->tabBook.top_shadow_GC ,
		points, 2, CoordModePrevious);

}

/*
Return page number (1,2,..) coressponding to this child's
tab page
*/
static int find_page(TabBookWidget cw , Widget tab)
{
	int page=1;
	WidgetList widget_list = cw->composite.children;
	Cardinal num_children = cw->composite.num_children;
	Cardinal n = 0;

	for (n=0; n < num_children; n++ )
	{
		Widget child = widget_list[n];
		if (tab == child)
			return page;
		if ( XtClass(child) == xmPushButtonWidgetClass)
			page++;
	}
	return 0;	/* error */
}

/*
Return page Widget of n'th manager 
*/
static Widget find_nth_manager(TabBookWidget cw , int page)
{
	XmBulletinBoardRec * clip = (XmBulletinBoardRec *) cw->tabBook.clip;
	WidgetList widget_list = clip->composite.children;
	Cardinal num_children = clip->composite.num_children;
	Cardinal n = 0;
	int page_count ;

	for (page_count=0, n=0; n < num_children; n++ )
	{
		Widget child = widget_list[n];

		if ( XtIsSubclass(child, xmManagerWidgetClass ) )
		{
			page_count++;
			if (page == page_count)
				return child;
		}
	}
	return NULL;	/* error */
}

/*
Return page Widget of n'th button
*/
static Widget find_nth_button(TabBookWidget cw , int page)
{
	WidgetList widget_list = cw->composite.children;
	Cardinal num_children = cw->composite.num_children;
	Cardinal n = 0;
	int page_count ;

	for (page_count=0, n=0; n < num_children; n++ )
	{
		Widget child = widget_list[n];

		if ( XtClass(child) == xmPushButtonWidgetClass)
		{
			page_count++;
			if (page == page_count)
				return child;
		}
	}
	return NULL;	/* error */
}

/*
Unmanage all pages except active one
*/
static void unmanage_all_Pages(TabBookWidget cw )
{
	XmBulletinBoardRec * clip = (XmBulletinBoardRec *) cw->tabBook.clip;
	WidgetList widget_list = clip->composite.children;
	Cardinal num_children = clip->composite.num_children;
	Cardinal n = 0;

	for (n=0; n < num_children; n++ )
	{
		Widget child = widget_list[n];

		if ( XtIsSubclass(child, xmManagerWidgetClass ) )
		{
			if (cw->tabBook.active_manager == child )
				XtManageChild(child);
			else
				XtUnmanageChild(child);
		}
	}
}



/*
Tab PushButton callback - activate this tab and deactivate
the formerly activated tab
*/
static void activate_cb(Widget  w, XtPointer ctx ,XtPointer   cb_data)
{
	TabBookWidget cw = (TabBookWidget) XtParent(w);
	XcgTabBookCallbackData rcb_data;
	XmPushButtonCallbackStruct * but_cb_data =
			(XmPushButtonCallbackStruct * ) cb_data;
	int option;

	if (cw->tabBook.active_tab == w)
		return;	/* pressed the already active tab button */

	if (ctx)	/* callback called internally rather from button event */
		option = *(int *) ctx;
	else
		option = 0;
	rcb_data.reason = XcgNewPage;	/* reason for callback */
	if (but_cb_data)
		rcb_data.event = but_cb_data->event;		/* button event  */
	else
		rcb_data.event = NULL;		/* not a real event, emulated */
	rcb_data.prev_active_page = cw->tabBook.active_page ;		/* 0 when initially called */
	rcb_data.active_page = find_page(cw, w);		/* new active page */
	rcb_data.button = w;			/* the button widget which was
					   pressed (emulated or actual*/
	rcb_data.ret_veto = False;
	rcb_data.future1 = 0;
	rcb_data.future2 = NULL;

	/* callback not called if we are here as a result of user
	   calling XcgTabBookSetActivePage with option set to
	   XcgTabBook_OPT_NO_CB */
	if (cw->tabBook.newPageCallback && !(option & XcgTabBook_OPT_NO_CB) )
		XtCallCallbackList((Widget)cw, cw->tabBook.newPageCallback,
				(XtPointer) &rcb_data);
	if (rcb_data.ret_veto )
		return;

	draw_tab_bottom(cw , cw->tabBook.active_tab, False);
	draw_tab_bottom(cw , w, True);
	cw->tabBook.active_tab = w;

	cw->tabBook.old_active_page = cw->tabBook.active_page ;
	if (cw->tabBook.auto_manage &&  cw->tabBook.active_manager )
		XtUnmanageChild(cw->tabBook.active_manager);
	cw->tabBook.active_page = rcb_data.active_page ;
	cw->tabBook.active_manager =  find_nth_manager(cw, cw->tabBook.active_page );
	if (cw->tabBook.auto_manage &&  cw->tabBook.active_manager )
		XtManageChild(cw->tabBook.active_manager);

}


/*
 If child is push button, change its resources to meet our needs 
*/
static void InsertChild(Widget w)
{
	TabBookWidget cw = (TabBookWidget) XtParent(w);

	TRACE_OFF((stderr,"Insert Child\n"));

	if ( XtClass(w) == xmPushButtonWidgetClass)
	{
		cw->tabBook.total_buttons ++;
		XtVaSetValues(w, XmNfillOnArm, False,
			XmNshadowThickness, 0,
			NULL);
		XtAddCallback(w, XmNactivateCallback, 
			(XtCallbackProc) activate_cb, NULL);
		if (cw->tabBook.active_page == cw->tabBook.total_buttons)
			cw->tabBook.active_tab = w;
	}
	else if ( XtIsSubclass(w, xmManagerWidgetClass ) )
	{
		if (cw->tabBook.clip == NULL)	
			/* first child inserted is our own clipping widget */
			goto inherit_insert;
		cw->tabBook.total_managers ++;
		if (cw->tabBook.active_page == cw->tabBook.total_managers)
			cw->tabBook.active_manager = w;
		w->core.parent = cw->tabBook.clip ; /* reparent */
	}
	else
	{
		XtAppError(XtWidgetToApplicationContext((Widget)cw), 
		"TabBook: Only Buttons and manager widgets may be children of TabBook");
	}

	inherit_insert:
	/* call insert child of superclass, which we know is at least a
	   composite widget */
	(*((CompositeWidgetClass)(tabBookClassRec.core_class.superclass))->
	composite_class.insert_child) (w) ;
}

/* a routine to halt execution and force  
a core dump for debugging analysis	
when a public routine is called with the wrong class of widget
*/
static void wrong_widget(char * routine)
{
	int mypid = getpid(); 
	TRACE_OFF((stderr, "Wrong class of widget passed to %s\n", routine));
	fflush(stderr); 
	kill(mypid, SIGABRT); 
}

/* ---------------- Widget API ---------------------------- */

/*
;+
XcgTabBookSetActivePage -- The active page is changed

Func:	The active page is changed to the passed page number. Page
	numbers begin with 1. If this results in a new active page,
	the result is as if the user pressed the corresponding tab button.
	It is also equivalent to setting the resource 

C-Call:	Boolean XcgTabBookSetActivePage(Widget w, int page, int option )

Input:	w - TabBook widget
	page - The new active page number (1,2,...)
	option - bitmapped 
		XcgTabBook_OPT_NO_CB - don't call NewPage callback

Return:	True - page has been changed
	False - non-existent page number
;-
*/
Boolean XcgTabBookSetActivePage(Widget w, int page, int option )
{
#	define ROUTINE "XcgTabBookSetActivePage"
	TabBookWidget cw = (TabBookWidget) w;

	CheckWidgetClass(ROUTINE);	/* make sure we are called with a TabBook widget */
	if (cw->tabBook.layout_completed && cw->tabBook.active_page != page )
	{
		Widget button = find_nth_button(cw, page );
		/* emulate user pressing this button */
		if (button)
		{
			activate_cb(button, &option, NULL);
			return True;
		}
		else
			return False;
	}
	cw->tabBook.active_page = page ;
	return True;
#	undef ROUTINE
}


/*
;+
XcgTabBookGetActivePage -- The active page is returned

Func:	The current active page is returned. 

C-Call:	int XcgTabBookGetActivePage(Widget w)

Input:	w - TabBook widget

Return:	The current active page number (1,2,...)
	A zero means the widget has not been managed at least once
;-
*/
int XcgTabBookGetActivePage(Widget w)
{
#	define ROUTINE "XcgTabBookGetActivePage"
	TabBookWidget cw = (TabBookWidget) w;

	CheckWidgetClass(ROUTINE);	/* make sure we are called with a TabBook widget */
	return cw->tabBook.active_page ;
#	undef ROUTINE
}

/*
;+
XcgTabBookGetActivePageWidget -- The manager widget child of the active page is returned

Func:	The manager widget child of the active page is returned.

C-Call:	Widget XcgTabBookGetActivePageWidget(Widget w)

Input:	w - TabBook widget

Return:	The manager Widget representing the active page.
	A NULL means the widget has not been managed at least once
;-
*/
Widget XcgTabBookGetActivePageWidget(Widget w)
{
#	define ROUTINE "XcgTabBookGetActivePageWidget"
	TabBookWidget cw = (TabBookWidget) w;

	CheckWidgetClass(ROUTINE);	/* make sure we are called with a TabBook widget */
	return find_nth_manager(cw, cw->tabBook.active_page );
#	undef ROUTINE
}

