/* X-Designer
** (c) Copyright 1996
** Imperial Software Technology (IST),
** Berkshire House, 252 King's Road.,
** Reading,
** Berkshire,
** United Kingdom RG1 4HP.
**
** Telephone: +44 118 9587055
** Fax:       +44 118 9589005
** Email:     support@ist.co.uk
*/

#if !defined(lint) && !defined(NOSCCS)
static char *sccsid = {"@(#)Flow.c	1.6"} ; /* 98/01/19 */
#endif /* lint && NOSCCS */

/*
** Flow Layout Widget:
**
** A widget for laying out widgets in a text-like manner.
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
#include "FlowP.h"
#include "Flow.h"
#include "XdResources.h"

#ifndef    MIN
#define    MIN(a, b) ((a) < (b) ? (a) : (b))
#define    MAX(a, b) ((a) > (b) ? (a) : (b))
#endif  /* MIN */

/*
** Static Widget Method Declarations
*/

#ifdef   _NO_PROTO
static void    ClassInitialize() ;
static Boolean SetValues() ;
static void    FlowLayout() ;
static void    FlowPreferredSize() ;
#else  /* _NO_PROTO */
static void    ClassInitialize(void) ;
static Boolean SetValues(Widget cw, Widget rw, Widget nw, ArgList args, Cardinal *num_args) ;
static void    FlowLayout(Widget w) ;
static void    FlowPreferredSize(Widget w, Dimension *width, Dimension *height) ;
#endif /* _NO_PROTO */
 
/*
** End Static Widget Method Declarations
*/
 
/*
** Translations Table
*/
/* NONE */
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

#define FlowConstraint(w) ((XdFlowConstraintPtr)(w)->core.constraints)

static XtResource resources[] = {
    {   
	XtNxdHorizontalAlignment,
        XtCXdHorizontalAlignment,
        XtRXdHorizontalAlignment,
        sizeof(unsigned char),
        XtOffsetOf( XdFlowRec, flow.alignment),
        XmRImmediate,
        (XtPointer) XdALIGN_CENTER
    },
    {   
	XtNxdHorizontalSpacing,
        XtCXdHorizontalSpacing,
        XtRXdHorizontalSpacing,
        sizeof(Dimension),
        XtOffsetOf( XdFlowRec, flow.hSpacing),
        XmRImmediate,
        (XtPointer) 0
    },
    {   
	XtNxdVerticalSpacing,
        XtCXdVerticalSpacing,
        XtRXdVerticalSpacing,
        sizeof(Dimension),
        XtOffsetOf( XdFlowRec, flow.vSpacing),
        XmRImmediate,
        (XtPointer) 0
    },
} ;

/*
** End Resource List
*/
 
/*
** Widget Class Record
*/

XdFlowClassRec XdflowClassRec = {
	{ /* Core class */
		(WidgetClass)&xdLayoutClassRec,/* superclass           */
		"XdFlow",		/* class_name           */
		sizeof(XdFlowRec),	/* widget_size          */
		ClassInitialize,	/* class_initialize     */
		NULL,			/* class_part_initialize*/
		FALSE,			/* class_inited         */
		NULL,			/* initialize           */
		NULL,			/* initialize_hook      */  
		XtInheritRealize,	/* realize              */
		NULL,			/* actions              */
		0,			/* num_actions          */
		resources,		/* resources            */
		XtNumber(resources),	/* num_resources        */
		NULLQUARK,		/* xrm_class            */
		TRUE,			/* compress_motion      */
		TRUE,			/* compress_exposure    */
		TRUE,			/* compress_enterleave  */
		FALSE,			/* visible_interest     */
		NULL,			/* destroy              */
		XtInheritResize,	/* resize               */
		XtInheritExpose,	/* expose               */
		SetValues,		/* set_values           */
		NULL,			/* set_values_hook      */
		XtInheritSetValuesAlmost,/* set_values_almost    */
		NULL,			/* get_values_hook      */
		NULL,			/* accept_focus         */
		XtVersion,		/* version              */
		NULL,			/* callback_offsets     */
		XtInheritTranslations,	/* tm_table             */
		XtInheritQueryGeometry,	/* query_geometry       */
		NULL,			/* display_accelerator  */
		NULL,			/* extension            */
	}, { /* Composite class */
		XtInheritGeometryManager,	/* geometry_handler     */
		XtInheritChangeManaged,		/* change_managed       */
		XtInheritInsertChild,	/* insert_child         */
		XtInheritDeleteChild,	/* delete_child         */
		NULL			/* extension            */
	}, { /* Constraint class */
		NULL,			/* resource list        */
		0,	/* num resources        */
		sizeof(XdFlowConstraintRec),	/* constraint size      */
		NULL,			/* init proc            */
		NULL,			/* destroy proc         */
		NULL,			/* set values proc      */
		NULL			/* extension            */
	}, { /* Manager class */
		XtInheritTranslations,	/* default translations */
		NULL,			/* get resources        */
		0,			/* num get_resources    */
		NULL,			/* get cont resources   */
		0,			/* num cont get_resources*/
		NULL			/* extension */
	}, { /* Layout class */
		(XdLayoutProc)FlowLayout,	/* layout */
		(XdPreferredSizeProc)FlowPreferredSize,	/* preferred_size */
	}, {
		0			/* extension */
	}
} ;

/*
** End Widget Class Record
*/
 
/*
** Widget Class Pointer
*/
 
WidgetClass xdFlowWidgetClass = (WidgetClass) &XdflowClassRec ;

/* Resource Conversion Table */

static String alignmentNames[] = { "center", "beginning", "end" } ;

 
/*
** Class Initialisation Method - called once only the first time a widget of
**                               this type is instantiated
*/

#ifdef    _NO_PROTO
static void ClassInitialize()
#else  /* _NO_PROTO */
static void ClassInitialize(void)
#endif /* _NO_PROTO */
{
	/* Simply register the flow alignment names */

	XmRepTypeId alignmentID = XmRepTypeRegister(XtRXdAlignment, alignmentNames, NULL, XtNumber(alignmentNames)) ;
}

/*
** SetValues Method - handles any changes to widget instance resource set
*/
/* ARGSUSED */
#ifdef    _NO_PROTO
static Boolean SetValues(cw, rw, nw, args, num_args)
	Widget       cw ;
	Widget       rw ;
	Widget       nw ;
	ArgList      args ;
	Cardinal    *num_args ;
#else  /* _NO_PROTO */
static Boolean SetValues(Widget cw, Widget rw, Widget nw, ArgList args, Cardinal *num_args)
#endif /* _NO_PROTO */
{
	XdFlowWidget nb = (XdFlowWidget) nw ;
	XdFlowWidget cb = (XdFlowWidget) cw ;

	if (nb->flow.alignment != cb->flow.alignment ||
	    nb->flow.vSpacing != cb->flow.vSpacing ||
	    nb->flow.hSpacing != cb->flow.hSpacing) {
		nb->layout.needsLayout = True ;
	}

	return XdCheckLayout(nw, xdFlowWidgetClass) ;
}

/*
** Internal Routine
*/

#ifdef    _NO_PROTO
static void LayoutLine(w, start, end, y, width)
	XdFlowWidget w ;
	int          start ;
	int          end ;
	Position     y ;
	Dimension    width ;
#else  /* _NO_PROTO */
static void LayoutLine(XdFlowWidget w, int start, int end, Position y, Dimension width)
#endif /* _NO_PROTO */
{
	Position x = 0 ;
	int      i ;

	switch (w->flow.alignment) {
	case XdAlignmentCenter :
		x = (Position)(Width(w) - width) / 2 ;
		break ;

	case XdAlignmentEnd :
		x = Width(w) - width ;
		break ;
	}

	for (i = start ; i < end ; i++) {
		Widget child = w->composite.children[i] ;

		if (child->core.managed) {
			XtMoveWidget(child, x, y) ;
			x += Width(child) + w->flow.hSpacing ;
		}
	}
}

/*
** SuperClass (Layout Widget) XdLayoutProc Method
*/

#ifdef    _NO_PROTO
static void FlowLayout(widget)
	Widget widget ;
#else  /* _NO_PROTO */
static void FlowLayout(Widget widget)
#endif /* _NO_PROTO */
{
	XdFlowWidget w = (XdFlowWidget) widget ;
	Position     x = 0, y = 0 ;
	Dimension    maxHeight = 0 ;
	Dimension    width     = Width(w) ;
	Dimension    height    = Height(w) ;
	int i, index, start, end ;

	index = 0 ;
	start = end = 0 ;

	for (i = 0 ; i < w->composite.num_children ; i++) {
		Widget child = w->composite.children[i] ;

		if (child->core.managed) {
			if ((Dimension)(x+Width(child)) > width) {
				end       = ((i == start) ? i + 1 : i) ;
				LayoutLine(w, start, end, y, (Dimension) x) ;
				start     = end ;
				x         = 0 ;
				y        += maxHeight + w->flow.vSpacing ;
				maxHeight = 0 ;
			}

			if (Height(child) > maxHeight) {
				maxHeight = Height(child) ;
			}

			if (x) {
				x += w->flow.hSpacing ;
			}

			x += Width(child) ;

			index++ ;
		}
	}

	if (start != w->composite.num_children) {
		LayoutLine(w, start, (int) w->composite.num_children, y, (Dimension) x) ;
	}
}

 
/*
** SuperClass (Layout Widget) XdPreferredSizeProc Method
*/

#ifdef    _NO_PROTO
static void FlowPreferredSize(widget, width, height)
	Widget       widget ;
	Dimension   *width ;
	Dimension   *height ;
#else  /* _NO_PROTO */
static void FlowPreferredSize(Widget widget, Dimension *width, Dimension *height)
#endif /* _NO_PROTO */
{
	XdFlowWidget w = (XdFlowWidget) widget ;
	Dimension    maxWidth = 0, maxHeight = 0 ;
	int i, index ;

	index = 0 ;

	for (i = 0 ; i < w->composite.num_children ; i++) {
		Widget child = w->composite.children[i] ;

		if (child->core.managed) {
			if (Height(child) > maxHeight) {
				maxHeight = Height(child);
			}

			maxWidth += Width(child) ;

			if (index) {
				maxWidth += w->flow.hSpacing ;
			}

			index++ ;
		}
	}

	*width  = maxWidth ;
	*height = maxHeight ;
}

/*
** Convenience Function for creating a Border Layout Widget
*/

#ifdef    _NO_PROTO 
Widget XdCreateFlowWidget(parent, name, arglist, argcount)
	Widget   parent ;
        char    *name ;
        ArgList  arglist ;
        Cardinal argcount ;
#else  /* _NO_PROTO  */
Widget XdCreateFlowWidget(Widget parent, char *name, ArgList arglist, Cardinal argcount)
#endif /* _NO_PROTO  */
{
	return XtCreateWidget(name, xdFlowWidgetClass, parent, arglist, argcount) ;
}
