/* X-Designer
** (c) Copyright 1996
** Imperial Software Technology (IST),
** Berkshire House, 252 King's Road.,
** Reading,
** Berkshire,
** United Kingdom RG1 4HP.
**
** Telephone: +44 1734 587055
** Fax:       +44 1734 589005
** Email:     support@ist.co.uk
*/

#if !defined(lint) && !defined(NOSCCS)
static char *sccsid = {"@(#)Layout.c	1.5"} ; /* 98/01/19 */
#endif /* lint && NOSCCS */

/*
** Layout:
**
** A an abstract layout widget.
*/

#ifdef    __cplusplus
#undef    _NO_PROTO
#endif /* __cplusplus */

#include <X11/IntrinsicP.h>
#include <X11/Intrinsic.h>
#include <X11/CompositeP.h>
#include <X11/Composite.h>
#include <X11/Constraint.h>
#include <Xm/Xm.h>
#include <Xm/XmP.h>
#include <Xm/DrawP.h>
#include "LayoutP.h"
#include "Layout.h"
#include "XdResources.h"

#ifndef   MIN
#define   MIN(a, b) ((a) < (b) ? (a) : (b))
#define   MAX(a, b) ((a) > (b) ? (a) : (b))
#endif /* MIN */

 
/*
** Layout Widget:
**
** A general purpose super-class widget for laying out.
**
** Jeremy Huxtable 1992
*/

/*
** Static Widget Method Declarations
*/

#ifdef    _NO_PROTO
static void             Initialize();
static void             Resize();
static void             ChangeManaged();
static void             Redisplay();
static Boolean          SetValues();
static Boolean          ConstraintSetValues();
static XtGeometryResult GeometryManager();
static XtGeometryResult QueryGeometry();
static void             Layout();
static void             PreferredSize();
#else  /* _NO_PROTO */
static void             Initialize(Widget request, Widget new_widget, ArgList args, Cardinal *num_args);
static void             Resize(Widget w);
static void             ChangeManaged(Widget w);
static void             Redisplay(Widget w, XEvent *event, Region region);
static Boolean          SetValues(Widget cw, Widget rw, Widget nw, ArgList args, Cardinal *num_args);
static Boolean          ConstraintSetValues(Widget cw, Widget rw, Widget nw, ArgList args, Cardinal *num_args);
static XtGeometryResult GeometryManager(Widget w, XtWidgetGeometry *request, XtWidgetGeometry *reply);
static XtGeometryResult QueryGeometry(Widget w, XtWidgetGeometry *request, XtWidgetGeometry *preferred);
static void             Layout(Widget w);
static void             PreferredSize(Widget w, Dimension *width, Dimension *height);
#endif /* _NO_PROTO */

/* 
** Static Internal Function Declarations
*/

#ifdef    _NO_PROTO
static XtGeometryResult TryLayout();
#else  /* _NO_PROTO */
static XtGeometryResult TryLayout(XdLayoutWidget parent, Mask *mask, Dimension *wdelta, Dimension *hdelta);
#endif /* _NO_PROTO */

/* 
** End Static Internal Function Declarations
*/

/*
** End Static Widget Method Declarations
*/

/*
** Translations Table
*/

#ifndef   XT_HAS_TRANS_FIX
static char defaultTranslations[] = "\
<Key>osfHelp:ManagerGadgetHelp()\n\
<Key>space:ManagerGadgetSelect()\n\
<Key>Return:ManagerParentActivate()\n\
<Key>osfActivate:ManagerParentActivate()\n\
<Key>osfCancel:ManagerParentCancel()\n\
<Key>osfSelect:ManagerGadgetSelect()\n\
<Key>:ManagerGadgetKeyInput()\n\
<BtnMotion>:ManagerGadgetButtonMotion()\n\
<Btn1Down>:ManagerGadgetArm()\n\
<Btn1Down>,<Btn1Up>:ManagerGadgetActivate()\n\
<Btn1Up>:ManagerGadgetActivate()\n\
<Btn1Down>(2+):ManagerGadgetMultiArm()\n\
<Btn1Up>(2+):ManagerGadgetMultiActivate()\n\
<Btn2Down>:ManagerGadgetDrag()";
#else  /* XT_HAS_TRANS_FIX */
static char defaultTranslations[] = "\
<BtnMotion>:ManagerGadgetButtonMotion()\n\
<Btn1Down>:ManagerGadgetArm()\n\
<Btn1Down>,<Btn1Up>:ManagerGadgetActivate()\n\
<Btn1Up>:ManagerGadgetActivate()\n\
<Btn1Down>(2+):ManagerGadgetMultiArm()\n\
<Btn1Up>(2+):ManagerGadgetMultiActivate()\n\
<Btn2Down>:ManagerGadgetDrag()\n\
:<Key>osfHelp:ManagerGadgetHelp()\n\
:<Key>osfActivate:ManagerParentActivate()\n\
:<Key>osfCancel:ManagerParentCancel()\n\
:<Key>osfSelect:ManagerGadgetSelect()\n\
<Key>space:ManagerGadgetSelect()\n\
<Key>Return:ManagerParentActivate()\n\
<Key>:ManagerGadgetKeyInput()";
#endif /* XT_HAS_TRANS_FIX */

/*
** End Translations Table
*/

/*
** Actions Table
*/
/* NONE */
/*
** End Actions Table
*/

/*
** Resource List
*/
/* NONE */
/*
** End Resource List
*/

/*
** Widget Class Record
*/

XdLayoutClassRec xdLayoutClassRec = {
	{ /* Core class */
		(WidgetClass)&xmManagerClassRec,/* superclass           */
		"XdLayout",		/* class_name           */
		sizeof(XdLayoutRec),	/* widget_size          */
		NULL,			/* class_initialize     */
		NULL,			/* class_part_initialize*/
		FALSE,			/* class_inited         */
		Initialize,		/* initialize           */
		NULL,			/* initialize_hook      */  
		XtInheritRealize,	/* realize              */
		NULL,			/* actions              */
		0,			/* num_actions          */
		0,			/* resources            */
		0,			/* num_resources        */
		NULLQUARK,		/* xrm_class            */
		TRUE,			/* compress_motion      */
		TRUE,			/* compress_exposure    */
		TRUE,			/* compress_enterleave  */
		FALSE,			/* visible_interest     */
		NULL,			/* destroy              */
		Resize,			/* resize               */
		Redisplay,		/* expose               */
		SetValues,		/* set_values           */
		NULL,			/* set_values_hook      */
		XtInheritSetValuesAlmost,/* set_values_almost    */
		NULL,			/* get_values_hook      */
		NULL,			/* accept_focus         */
		XtVersion,		/* version              */
		NULL,			/* callback_offsets     */
		defaultTranslations,	/* tm_table             */
		QueryGeometry,		/* query_geometry       */
		NULL,			/* display_accelerator  */
		NULL,			/* extension            */
	}, { /* Composite class */
		GeometryManager,	/* geometry_handler     */
		ChangeManaged,		/* change_managed       */
		XtInheritInsertChild,	/* insert_child         */
		XtInheritDeleteChild,	/* delete_child         */
		NULL			/* extension            */
	}, { /* Constraint class */
		0,			/* resource list        */
		0,			/* num resources        */
		0,			/* constraint size      */
		NULL,			/* init proc            */
		NULL,			/* destroy proc         */
		ConstraintSetValues,	/* set values proc      */
		NULL			/* extension            */
	}, { /* Manager class */
		XtInheritTranslations,	/* default translations */
		NULL,			/* get resources        */
		0,			/* num get_resources    */
		NULL,			/* get cont resources   */
		0,			/* num cont get_resources*/
		NULL			/* extension */
	}, {
		Layout,			/* layout */
		PreferredSize,		/* preferred_size */
	}
};

/*
** End Widget Class Record
*/
 
/*
** Widget Class Pointer
*/

WidgetClass xdLayoutWidgetClass = (WidgetClass) &xdLayoutClassRec;

 
/*
** Widget Initialisation Method - called per-instance on widget create
*/
 
/* ARGSUSED */
#ifdef    _NO_PROTO
static void Initialize(request, new_widget, args, num_args)
        Widget    request ;
        Widget    new_widget ;
        ArgList   args ;
        Cardinal *num_args ;
#else  /* _NO_PROTO */
static void Initialize(Widget request, Widget new_widget, ArgList args, Cardinal *num_args)
#endif /* _NO_PROTO */
{
	XdLayoutWidget rw = (XdLayoutWidget) request ;
	
	if (rw->core.width <= 0)
		rw->core.width = 16;
	if (rw->core.height <= 0)
		rw->core.height = 16;
	rw->layout.needsLayout = False;
}

/*
** Default Layout Widget XdLayoutProc
** Meant to be overridden in derived classes
*/

/* ARGSUSED */
#ifdef    _NO_PROTO
static void Layout(w)
	Widget w ;
#else  /* _NO_PROTO */
static void Layout(Widget w)
#endif /* _NO_PROTO */
{
}

/*
** Default Layout Widget XdPreferredSizeProc
** Meant to be overridden in derived classes
*/
/* ARGSUSED */
#ifdef    _NO_PROTO
static void PreferredSize(w, width, height)
	Widget     w ;
	Dimension *width ;
	Dimension *height ;
#else  /* _NO_PROTO */
static void PreferredSize(Widget w, Dimension *width, Dimension *height)
#endif /* _NO_PROTO */
{
	*width = *height = 100;
}

/*
** Resize Method - called by the parent geometry manager after the resize request is ok
**                 and the window of this widget has been resized.
**                 This method is responsible for implementing the size internally.
*/

#ifdef    _NO_PROTO
static void Resize(w)
	Widget w ;
#else  /* _NO_PROTO */
static void Resize(Widget w)
#endif /* _NO_PROTO */
{
	XdCallLayout(w);
}

/*
** ConstraintSetValues Method - Override in derived class.
*/
/* ARGSUSED */
#ifdef    _NO_PROTO
static Boolean ConstraintSetValues(cw, rw, nw, args, num_args)
        Widget    cw ;
        Widget    rw ;
        Widget    nw ;
        ArgList   args ;
        Cardinal *num_args ;
#else  /* _NO_PROTO */
static Boolean ConstraintSetValues(Widget cw, Widget rw, Widget nw, ArgList args, Cardinal *num_args)
#endif /* _NO_PROTO */
{
	return False;
}

/*
** SetValues Method - handles any changes to widget instance resource set
*/
 
/* ARGSUSED */
#ifdef    _NO_PROTO
static Boolean SetValues(cw, rw, nw, args, num_args)
        Widget    cw ;
        Widget    rw ;
        Widget    nw ;
        ArgList   args ;
        Cardinal *num_args ;
#else  /* _NO_PROTO */
static Boolean SetValues(Widget cw, Widget rw, Widget nw, ArgList args, Cardinal *num_args)
#endif /* _NO_PROTO */
{
/* FIXME
**	XdLayoutWidget nb = (XdLayoutWidget) nw ;
**	XdLayoutWidget cb = (XdLayoutWidget) cw ;
**
**	if (nb->core.width != cb->core.width ||
**    		nb->core.height != cb->core.height) {
**		return True;
**	}
*/
	return False;
}

/*
** QueryGeometry Method
*/

#ifdef    _NO_PROTO
static XtGeometryResult QueryGeometry(widget, request, preferred)
	Widget            widget ;
	XtWidgetGeometry *request ;
	XtWidgetGeometry *preferred ;
#else  /* _NO_PROTO */
static XtGeometryResult QueryGeometry(Widget widget, XtWidgetGeometry *request, XtWidgetGeometry *preferred)
#endif /* _NO_PROTO */
{
	XdLayoutWidget w = (XdLayoutWidget) widget;

	if (!(request->request_mode & CWWidth) &&
	    !(request->request_mode & CWHeight))
		return XtGeometryYes;

	(*LayoutClass(w)->layout_class.preferred_size)((Widget)w, &preferred->width, &preferred->height);
	preferred->request_mode = CWWidth | CWHeight;

	if ((request->request_mode & CWWidth) &&
	    (request->request_mode & CWHeight)) {
		if (preferred->width <= request->width &&
		    preferred->height <= request->height) {
			preferred->width = request->width;
			preferred->height = request->height;
			return XtGeometryYes;
		} else if (preferred->width < request->width &&
		    preferred->height < request->height) {
			return XtGeometryNo;
		}
		return XtGeometryAlmost;
	} else if (request->request_mode & CWWidth) {
		if (preferred->width <= request->width) {
			preferred->width = request->width;
			return XtGeometryYes;
		}
		return XtGeometryNo;
	} else if (request->request_mode & CWHeight) {
		if (preferred->height <= request->height) {
			preferred->height = request->height;
			return XtGeometryYes;
		}
		return XtGeometryNo;
	}
	return XtGeometryYes;
}

/*
** GeometryManager Method
*/

#ifdef    _NO_PROTO
static XtGeometryResult GeometryManager(w, request, reply)
        Widget            w ;
        XtWidgetGeometry *request ;
        XtWidgetGeometry *reply ;
#else  /* _NO_PROTO */
static XtGeometryResult GeometryManager(Widget w, XtWidgetGeometry *request, XtWidgetGeometry *reply)
#endif /* _NO_PROTO */
{
	XdLayoutWidget rw = (XdLayoutWidget)w->core.parent;
	Mask mask;
	XtGeometryResult result;
	Dimension wdelta, hdelta;

	if (((request->request_mode & CWX) && request->x != w->core.x) ||
	    ((request->request_mode & CWY) && request->y != w->core.y))
		return XtGeometryNo;

	if (request->request_mode & (CWWidth | CWHeight | CWBorderWidth)) {
		Dimension savewidth = w->core.width;
		Dimension saveheight = w->core.height;
		Dimension savelayoutwidth = w->core.border_width;

		if (request->request_mode & CWWidth)
			w->core.width = request->width;
		if (request->request_mode & CWHeight)
			w->core.height = request->height;
		if (request->request_mode & CWBorderWidth)
			w->core.border_width = request->border_width;

		result = TryLayout(rw, &mask, &wdelta, &hdelta);

		if (result == XtGeometryNo) {
			w->core.width = savewidth;
			w->core.height = saveheight;
			w->core.border_width = savelayoutwidth;
			return XtGeometryNo;
		}

		if (result == XtGeometryAlmost) {
			reply->request_mode = request->request_mode;
			if (!(mask & CWWidth)) {
				reply->width = w->core.width = savewidth;
				reply->border_width = w->core.border_width = savelayoutwidth;
			}
			if (!(mask & CWHeight))
				reply->height = w->core.height = saveheight;
			return XtGeometryAlmost;
		}

		XdCallLayout((Widget)rw);
		return XtGeometryYes;
	}
	return XtGeometryYes;
}

/*
** Internal Routine
*/

#ifdef    _NO_PROTO
static XtGeometryResult TryLayout(w, mask, wdelta, hdelta)
	XdLayoutWidget w ;
	Mask          *mask ;
	Dimension     *wdelta ;
	Dimension     *hdelta ;
#else  /* _NO_PROTO */
static XtGeometryResult TryLayout(XdLayoutWidget w, Mask *mask, Dimension *wdelta, Dimension *hdelta)
#endif /* _NO_PROTO */
{
	Dimension maxWidth, maxHeight;

	(*LayoutClass(w)->layout_class.preferred_size)((Widget)w, &maxWidth, &maxHeight);

	if (maxWidth > w->core.width || maxHeight > w->core.height) {
		XtGeometryResult result;
		Dimension replyWidth, replyHeight;
		Dimension width, height;

		width = MAX(maxWidth, w->core.width);
		height = MAX(maxHeight, w->core.height);
		result = XtMakeResizeRequest((Widget) w, width, height, &replyWidth, &replyHeight);
		*mask = 0;

		if (maxWidth == replyWidth)
			*mask = CWWidth;
		if (maxHeight == replyHeight)
			*mask |= CWHeight;

		if (result == XtGeometryAlmost)
			XtMakeResizeRequest((Widget) w, replyWidth, replyHeight, NULL, NULL);
		*wdelta = maxWidth - w->core.width;
		*hdelta = maxHeight - w->core.height;
		return result;
	}
	*mask = CWWidth | CWHeight;
	return XtGeometryYes;
}

/*
** ChangedManage Method - set of managed children has changed
**                        change the Layout Widget size if possible
*/

#ifdef    _NO_PROTO
static void ChangeManaged(widget)
	Widget widget ;
#else  /* _NO_PROTO */
static void ChangeManaged(Widget widget)
#endif /* _NO_PROTO */
{
	XdLayoutWidget w = (XdLayoutWidget) widget ;

	XtGeometryResult result;
	Dimension width, height;
	Mask mask;

	result = TryLayout(w, &mask, &width, &height);
	XdCallLayout((Widget)w);
}


/*
** Redisplay Method
*/

#ifdef    _NO_PROTO
static void Redisplay(w, event, region)
        Widget  w ;
        XEvent *event ;
        Region  region ;
#else  /* _NO_PROTO */
static void Redisplay(Widget w, XEvent *event, Region region)
#endif /* _NO_PROTO */
{   
	XdLayoutWidget r = (XdLayoutWidget)w;

 	_XmRedisplayGadgets(w, event, region) ;

	if (r->manager.shadow_thickness) {   
        	_XmDrawShadows(XtDisplay(w), 
			XtWindow(w),
			r->manager.top_shadow_GC,
			r->manager.bottom_shadow_GC,
			0, 0,
			r->core.width, r->core.height,
			r->manager.shadow_thickness, 
			XmSHADOW_OUT);
        }
}


/*
** Public to Derived Classes
*/

#ifdef    _NO_PROTO
Boolean XdCheckLayout(w, c)
	Widget      w ;
	WidgetClass c ;
#else  /* _NO_PROTO */
Boolean XdCheckLayout(Widget w, WidgetClass c)
#endif /* _NO_PROTO */
{
	if (XtClass(w) == c && XtIsRealized(w)) {
		XdLayoutWidget r = (XdLayoutWidget)w;
		if (r->layout.needsLayout) {
			XdCallLayout(w);
			r->layout.needsLayout = False;
			return True;
		}
	}
	return False;
}

/* 
** Public to Derived Classes
*/

#ifdef    _NO_PROTO
void XdAlignInCell(r, cell, alignment, fill)
	XRectangle *r ;
	XRectangle *cell ;
	int         alignment ;
	int         fill ;
#else  /* _NO_PROTO */
void XdAlignInCell(XRectangle *r, XRectangle *cell, int alignment, int fill)
#endif /* _NO_PROTO */
{
	r->x = cell->x;
	r->y = cell->y;

	/* Horizontal fill */
	switch (fill) {
	case XdCELL_FILL_BOTH:
	case XdCELL_FILL_HORIZONTAL:
		r->width = cell->width;
		break;
	}

	/* Vertical fill */
	switch (fill) {
	case XdCELL_FILL_BOTH:
	case XdCELL_FILL_VERTICAL:
		r->height = cell->height;
		break;
	}

	/* Horizontal alignment */
	switch (alignment) {
	case XdCELL_ALIGN_CENTER:
	case XdCELL_ALIGN_NORTH:
	case XdCELL_ALIGN_SOUTH:
		r->x += (Position)(cell->width - r->width)/2;
		break;
	case XdCELL_ALIGN_WEST:
	case XdCELL_ALIGN_NORTHWEST:
	case XdCELL_ALIGN_SOUTHWEST:
		break;
	case XdCELL_ALIGN_EAST:
	case XdCELL_ALIGN_NORTHEAST:
	case XdCELL_ALIGN_SOUTHEAST:
		r->x += cell->width - r->width;
		break;
	}

	/* Vertical alignment */
	switch (alignment) {
	case XdCELL_ALIGN_CENTER:
	case XdCELL_ALIGN_WEST:
	case XdCELL_ALIGN_EAST:
		r->y += (Position)(cell->height - r->height)/2;
		break;
	case XdCELL_ALIGN_NORTH:
	case XdCELL_ALIGN_NORTHWEST:
	case XdCELL_ALIGN_NORTHEAST:
		break;
	case XdCELL_ALIGN_SOUTH:
	case XdCELL_ALIGN_SOUTHWEST:
	case XdCELL_ALIGN_SOUTHEAST:
		r->y += cell->height - r->width;
		break;
	}

}
