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
static char *sccsid = {"@(#)Border.c	1.13"} ; /* 98/01/19 */
#endif /* lint && NOSCCS */

/*
** Border Layout Widget:
**
** A widget for laying out main windows.
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
#include "BorderP.h"
#include "Border.h"
#include "XdResources.h"

#ifndef   MIN
#define   MIN(a, b) ((a) < (b) ? (a) : (b))
#define   MAX(a, b) ((a) > (b) ? (a) : (b))
#endif /* MIN */

/*
** Static Widget Method Declarations
*/

#ifdef    _NO_PROTO
static void    ClassInitialize() ;
static void    Initialize() ;
static Boolean SetValues() ;
static Boolean ConstraintSetValues() ;

static void    BorderLayout() ;
static void    BorderPreferredSize() ;
#else  /* _NO_PROTO */
static void    ClassInitialize(void) ;
static void    Initialize(Widget request, Widget new_widget, ArgList args, Cardinal *num_args) ;
static Boolean SetValues(Widget cw, Widget rw, Widget nw, ArgList args, Cardinal *num_args) ;
static Boolean ConstraintSetValues(Widget cw, Widget rw, Widget nw, ArgList args, Cardinal *num_args) ;

static void    BorderLayout(Widget w);
static void    BorderPreferredSize(Widget w, Dimension *width, Dimension *height) ;
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
** Resource Lists
*/

#define BorderConstraint(w) ((XdBorderConstraintPtr)(w)->core.constraints)

static XtResource constraintResources[] = {
    {   
	XtNxdBorderAlignment,
        XtCXdBorderAlignment,
        XtRXdBorderAlignment,
        sizeof(short),
        XtOffsetOf(XdBorderConstraintRec, border.alignment),
        XmRImmediate,
        (XtPointer) XdBorderAlignmentNorth
    }
} ;

static XtResource resources[] = {
    {   
	XtNxdHorizontalSpacing,
        XtCXdHorizontalSpacing,
        XtRXdHorizontalSpacing,
        sizeof(Dimension),
        XtOffsetOf(XdBorderRec, border.hSpacing),
        XmRImmediate,
        (XtPointer) 0
    },
    {   
	XtNxdVerticalSpacing,
	XtCXdVerticalSpacing,
	XtRXdVerticalSpacing,
        sizeof(Dimension),
        XtOffsetOf(XdBorderRec, border.vSpacing),
        XmRImmediate,
        (XtPointer) 0
    }
};

/*
** End Resource Lists
*/

/*
** Widget Class Record
*/

XdBorderClassRec XdborderClassRec = {
	{ /* Core class */
		(WidgetClass)&xdLayoutClassRec,/* superclass           */
		"XdBorder",		/* class_name           */
		sizeof(XdBorderRec),	/* widget_size          */
		ClassInitialize,	/* class_initialize     */
		NULL,			/* class_part_initialize*/
		FALSE,			/* class_inited         */
		Initialize,		/* initialize           */
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
		constraintResources,	/* resource list        */
		XtNumber(constraintResources),	/* num resources        */
		sizeof(XdBorderConstraintRec),	/* constraint size      */
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
	}, { /* Layout class */
		(XdLayoutProc)BorderLayout,	/* layout */
		(XdPreferredSizeProc)BorderPreferredSize,	/* preferred_size */
	}, {
		0			/* extension */
	}
};

/*
** End Widget Class Record
*/

/*
** Widget Class Pointer
*/

WidgetClass xdBorderWidgetClass = (WidgetClass) &XdborderClassRec;


/*
** Border Alignment Names for resource conversion
*/

static String borderAlignmentNames[] = { "north", "center", "south", "east", "west" } ;


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
	/* Simply register the border alignment names */

	XmRepTypeId borderAlignmentID = XmRepTypeRegister(XtRXdBorderAlignment, borderAlignmentNames, NULL, XtNumber(borderAlignmentNames));
}


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
	/* Nothing */
}


/*
** Constraint Change Method - if any values change, just recheck everything
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
	XdBorderWidget        nb = (XdBorderWidget) XtParent(nw) ;
	XdBorderConstraintPtr cc = BorderConstraint(cw) ;
	XdBorderConstraintPtr nc = BorderConstraint(nw) ;

	if (cc->border.alignment != nc->border.alignment) {
	        nb->layout.needsLayout = True ;
	}

	return XdCheckLayout((Widget) nb, xdBorderWidgetClass) ;
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
	XdBorderWidget cb = (XdBorderWidget) cw ;
	XdBorderWidget nb = (XdBorderWidget) nw ;

	if (nb->border.vSpacing != cb->border.vSpacing ||
	    nb->border.hSpacing != cb->border.hSpacing) {
		nb->layout.needsLayout = True ;
	}
	return XdCheckLayout(nw, xdBorderWidgetClass) ;
}

typedef struct _Border_s *_Border_p ;
typedef struct _Border_s
{
	Dimension left ;
	Dimension right ;
	Dimension top ;
	Dimension bottom ;
	Dimension centreWidth ;
	Dimension centreHeight ;
	Dimension width ;
	Dimension height ;
} Border_t ;


/*
** Internal Routine
*/

#ifdef    _NO_PROTO
static void CalcBorders(w, b)
	XdBorderWidget w ;
	Border_t      *b ;
#else  /* _NO_PROTO */
static void CalcBorders(XdBorderWidget w, Border_t *b)
#endif /* _NO_PROTO */
{
	int i, index ;

#ifndef   JAVA_BEHAVIOUR_OFF
	int done = 0 ;
#endif /* JAVA_BEHAVIOUR_OFF */

	b->left         = 0 ;
	b->top          = 0 ;
	b->right        = 0 ;
	b->bottom       = 0 ;
	b->centreWidth  = 0 ;
	b->centreHeight = 0 ;
	b->width        = 0 ;
	b->height       = 0 ;

	index = 0 ;

#ifndef   JAVA_BEHAVIOUR_OFF
	for (i = w->composite.num_children - 1 ; i >= 0 ; i--) {
#else  /* JAVA_BEHAVIOUR_OFF */
	for (i = 0 ; i < w->composite.num_children ; i++ ) {
#endif /* JAVA_BEHAVIOUR_OFF */
		Widget child = w->composite.children[i] ;

		if (child->core.managed) {
			XdBorderConstraintPtr c = BorderConstraint(child) ;
			XtWidgetGeometry      intended, preferred ;
			Dimension             width,    height ;

#ifndef   JAVA_BEHAVIOUR_OFF
			if (done & (1 << c->border.alignment)) {
			        continue ;
			}

			done |= (1 << c->border.alignment) ;
#endif /* JAVA_BEHAVIOUR_OFF */

			intended.request_mode = CWWidth | CWHeight ;
			intended.width        = Width(child) ;
			intended.height       = Height(child) ;

			XtQueryGeometry(child, &intended, &preferred) ;

			width  = preferred.width ;
			height = preferred.height ;

			switch (c->border.alignment) {
			case XdBorderAlignmentNorth : /* FALLTHROUGH */
			case XdBorderAlignmentSouth :
				b->width = MAX(b->width, width) ;
				break ;

			case XdBorderAlignmentWest :
				b->left += width ;
				break ;

			case XdBorderAlignmentCenter :
				b->centreWidth = MAX(b->width, width) ;
				break ;

			case XdBorderAlignmentEast :
				b->right += width ;
				break ;
			}

			switch (c->border.alignment) {
			case XdBorderAlignmentNorth :
				b->top += height ;
				break ;

			case XdBorderAlignmentWest : /* FALLTHROUGH */
			case XdBorderAlignmentEast :
				b->centreHeight = MAX(b->height, height) ;
				break ;

			case XdBorderAlignmentCenter :
				b->centreHeight = MAX(b->height, height) ;
				break ;

			case XdBorderAlignmentSouth :
				b->bottom += height ;
				break ;
			}

			index++ ;
		}
	}

	b->left   += w->border.hSpacing ;
	b->right  += w->border.hSpacing ;
	b->top    += w->border.vSpacing ;
	b->bottom += w->border.vSpacing ;
	b->width   = MAX(b->width, b->left + b->right + b->centreWidth) ;
}


/*
** SuperClass (Layout Widget) XdLayoutProc Method
*/

#ifdef    _NO_PROTO
static void BorderLayout(widget)
	Widget widget ;
#else  /* _NO_PROTO */
static void BorderLayout(Widget widget)
#endif /* _NO_PROTO */
{
	XdBorderWidget w      = (XdBorderWidget) widget ;
	Dimension      cLeft  = 0, cTop = 0, cRight = 0, cBottom = 0 ;
	Dimension      width  = Width(w) ;
	Dimension      height = w->core.height ;
	Border_t       b ;
	int            i, index ;

	Position       cx, cy ;
	Dimension      cw, ch ;
#ifndef   JAVA_BEHAVIOUR_OFF
	int            done = 0 ;
#endif /* JAVA_BEHAVIOUR_OFF */

	CalcBorders(w, &b) ;

	if ((Dimension)(b.left + b.right) >= width) {
		width  = b.left + b.right + 1 ;
	}

	if ((Dimension)(b.top + b.bottom) >= height) {
		height = b.top + b.bottom + 1 ;
	}

	index = 0 ;

#ifndef   JAVA_BEHAVIOUR_OFF
	for (i = w->composite.num_children - 1 ; i >= 0; i--) {
#else  /* JAVA_BEHAVIOUR_OFF */
	for (i = 0 ; i < w->composite.num_children ; i++ ) {
#endif /* JAVA_BEHAVIOUR_OFF */
		Widget child = w->composite.children[i] ;

		if (child->core.managed) {
			XdBorderConstraintPtr c = BorderConstraint(child) ;
#ifndef   JAVA_BEHAVIOUR_OFF
			if (done & (1 << c->border.alignment)) {
			        continue ;
			}

			done |= (1 << c->border.alignment) ;
#endif /* JAVA_BEHAVIOUR_OFF */

			switch (c->border.alignment) {
			case XdBorderAlignmentNorth :
				cx    = 0 ; 
				cy    = cTop ; 
				cw    =  width ; 
				ch    = child->core.height ; 

				break ;

			case XdBorderAlignmentWest :
				cx     = cLeft ; 
				cy     = b.top ; 
				cw     = child->core.width ; 
				ch     = (height - b.top - b.bottom) ; 

				break ;

			case XdBorderAlignmentCenter :
				cx = b.left ; 
				cy = b.top ; 
				cw = (width - b.left - b.right) ; 
				ch = (height - b.top - b.bottom) ;

				break ;

			case XdBorderAlignmentEast :
				cRight += Width(child) ;
				cx      = (width - cRight) ; 
				cy      = b.top ; 
				cw      = child->core.width ; 
				ch      = (height - b.top - b.bottom) ;

				break ;

			case XdBorderAlignmentSouth :
				cBottom += Height(child); 
				cx       = 0 ; 
				cy       = (height - cBottom) ; 
				cw       = width ; 
				ch       = child->core.height ;

				break ;
			}

			XtConfigureWidget(child, cx, cy, cw, ch, child->core.border_width) ;

			switch (c->border.alignment) {
			case XdBorderAlignmentNorth : cTop += Height(child) ; break ;
			case XdBorderAlignmentWest  : cLeft += Width(child) ; break ;
			}

			index++ ;
		}
	}
}


/*
** SuperClass (Layout Widget) XdPreferredSizeProc Method
*/

#ifdef    _NO_PROTO
static void BorderPreferredSize(widget, width, height)
	Widget         widget ;
	Dimension     *width ;
	Dimension     *height ;
#else  /* _NO_PROTO */
static void BorderPreferredSize(Widget widget, Dimension *width, Dimension *height)
#endif /* _NO_PROTO */
{
	XdBorderWidget w = (XdBorderWidget) widget ;
	Border_t       b ;

	CalcBorders(w, &b) ;

	*width  = b.left + b.right  + b.centreWidth ;
	*height = b.top  + b.bottom + b.centreHeight ;
}


/*
** Convenience Function for creating a Border Layout Widget
*/

#ifdef    _NO_PROTO
Widget XdCreateBorderWidget(parent, name, arglist, argcount)
	Widget   parent ;
	char    *name ;
	ArgList  arglist ;
	Cardinal argcount ;
#else  /* _NO_PROTO */
Widget XdCreateBorderWidget(Widget parent, char *name, ArgList arglist, Cardinal argcount)
#endif /* _NO_PROTO */
{
	return XtCreateWidget(name, xdBorderWidgetClass, parent, arglist, argcount) ;
}
