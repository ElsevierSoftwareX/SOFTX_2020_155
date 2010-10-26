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
static char *sccsid = {"@(#)Grid.c	1.8"} ; /* 98/01/19 */
#endif /* lint && NOSCCS */

/*
** Grid Layout Widget:
**
** A widget for laying out grids.
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
#include "GridP.h"
#include "Grid.h"
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
static void    GridLayout() ;
static void    GridPreferredSize() ;
#else  /* _NO_PROTO */
static void    ClassInitialize(void) ;
static void    Initialize(Widget request, Widget new_widget, ArgList args, Cardinal *num_args) ;
static Boolean SetValues(Widget cw, Widget rw, Widget nw, ArgList args, Cardinal *num_args) ;
static void    GridLayout(Widget w) ;
static void    GridPreferredSize(Widget w, Dimension *width, Dimension *height) ;
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

static XtResource resources[] = {
    {   
	XtNxdNumColumns,
        XtCXdNumColumns,
        XtRXdNumColumns,
        sizeof(short),
        XtOffsetOf( XdGridRec, grid.numColumns),
        XmRImmediate,
        (XtPointer) 1
    },
    {   
	XtNxdHorizontalSpacing,
        XtCXdHorizontalSpacing,
        XtRXdHorizontalSpacing,
        sizeof(Dimension),
        XtOffsetOf( XdGridRec, grid.hSpacing),
        XmRImmediate,
        (XtPointer) 0
    },
    {   
	XtNxdVerticalSpacing,
        XtCXdVerticalSpacing,
        XtRXdVerticalSpacing,
        sizeof(Dimension),
        XtOffsetOf( XdGridRec, grid.vSpacing),
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
 
XdGridClassRec XdgridClassRec = {
	{ /* Core class */
		(WidgetClass)&xdLayoutClassRec,/* superclass           */
		"XdGrid",		/* class_name           */
		sizeof(XdGridRec),	/* widget_size          */
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
		NULL,			/* resource list        */
		0,			/* num resources        */
		sizeof(XdGridConstraintRec),	/* constraint size      */
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
		(XdLayoutProc)GridLayout,	/* layout */
		(XdPreferredSizeProc)GridPreferredSize,	/* preferred_size */
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

WidgetClass xdGridWidgetClass = (WidgetClass) &XdgridClassRec;

 
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
	/* Nothing */
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
** Internal Routine
*/

#ifdef    _NO_PROTO
static int CountManaged(w)
	XdGridWidget w ;
#else  /* _NO_PROTO */
static int CountManaged(XdGridWidget w)
#endif /* _NO_PROTO */
{
	int i;
	int numManaged = 0;

	for (i = 0; i < w->composite.num_children; i++) {
		if (w->composite.children[i]->core.managed)
			numManaged++;
	}
	return numManaged;
}


typedef struct _CellInfo {
	int numRows, numCols;
	Dimension maxWidth, maxHeight;
} CellInfo;


/*
** Internal Routine
*/

#ifdef    _NO_PROTO
static int CalcCellSizes(w, info)
	XdGridWidget w ;
	CellInfo    *info ;
#else  /* _NO_PROTO */
static int CalcCellSizes(XdGridWidget w, CellInfo *info)
#endif /* _NO_PROTO */
{
	int i, index;
	Dimension maxWidth = 1, maxHeight = 1;
	Widget child;
	int numManaged;
	int numRows, numCols = w->grid.numColumns;

	numManaged = CountManaged(w);
	numRows = (numManaged+numCols-1)/numCols;

	index = 0;
	for (i = 0; i < w->composite.num_children; i++) {
		child = w->composite.children[i];
		if (child->core.managed) {
			index++;
			if (maxWidth < Width(child))
				maxWidth = Width(child);
			if (maxHeight < Height(child))
				maxHeight = Height(child);
		}
	}

	info->numRows = numRows;
	info->numCols = numCols;
	info->maxWidth = maxWidth;
	info->maxHeight = maxHeight;

	return numManaged;
}

 
/*
** SuperClass (Layout Widget) XdLayoutProc Method
*/
 
#ifdef    _NO_PROTO
static void GridLayout(widget)
	Widget widget ;
#else  /* _NO_PROTO */
static void GridLayout(Widget widget)
#endif /* _NO_PROTO */
{
	XdGridWidget w = (XdGridWidget) widget;
	CellInfo info;
	int i, index;
	int numManaged;
	Dimension cellWidth, cellHeight;

	numManaged = CalcCellSizes(w, &info);

	if (numManaged) {
		index = 0;

		cellWidth = ((Dimension)(Width(w)+w->grid.hSpacing)) / ((Dimension) info.numCols) - w->grid.hSpacing ;
		cellHeight = ((Dimension)(Height(w)+w->grid.vSpacing)) / ((Dimension) info.numRows) - w->grid.vSpacing;
		
		if ( cellWidth > 0 && cellHeight > 0 )  {

		    for (i = 0; i < w->composite.num_children; i++) {
			int row = index / info.numCols;
			int col = index % info.numCols;
			Widget child = w->composite.children[i];
			if (child->core.managed) {
			    Position x, y;
				
			    index++;
				
			    x = (cellWidth+w->grid.hSpacing) * col;
			    y = (cellHeight+w->grid.vSpacing) * row;
				
			    XtConfigureWidget( child, x, y, 
					       cellWidth, cellHeight, 
					       child->core.border_width ) ;
			}
		    }

		}
	}
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
	XdGridWidget nb = (XdGridWidget) nw ;
	XdGridWidget cb = (XdGridWidget) cw ;

	if (nb->grid.numColumns != cb->grid.numColumns ||
	    nb->grid.hSpacing != cb->grid.hSpacing ||
	    nb->grid.vSpacing != cb->grid.vSpacing) {
		nb->layout.needsLayout = True;
	}
	return XdCheckLayout(nw, xdGridWidgetClass);
}

 
/*
** SuperClass (Layout Widget) XdPreferredSizeProc Method
*/

#ifdef    _NO_PROTO
static void GridPreferredSize(widget, width, height)
	Widget     widget ;
	Dimension *width ;
	Dimension *height ;
#else  /* _NO_PROTO */
static void GridPreferredSize(Widget widget, Dimension *width, Dimension *height)
#endif /* _NO_PROTO */
{
	XdGridWidget w = (XdGridWidget) widget ;
	CellInfo info;
	int numManaged ;
	int nwidth ;
	int nheight ;

	numManaged = CalcCellSizes(w, &info);
	nwidth  = (info.maxWidth + w->grid.hSpacing) * info.numCols - w->grid.hSpacing;
	nheight = (info.maxHeight + w->grid.vSpacing) * info.numRows - w->grid.vSpacing;

	if (nwidth < 0)
		nwidth = 0;
	if (nheight < 0)
		nheight = 0;

	*width  = (Dimension) nwidth ;
	*height = (Dimension) nheight ;
}

/*
** Convenience Function for creating a Grid Layout Widget
*/

#ifdef    _NO_PROTO
Widget XdCreateGridWidget(parent, name, arglist, argcount)
	Widget   parent ;
	char    *name ;
	ArgList  arglist ;
	Cardinal argcount ;
#else  /* _NO_PROTO */
Widget XdCreateGridWidget(Widget parent, char *name, ArgList arglist, Cardinal argcount)
#endif /* _NO_PROTO */
{
	return XtCreateWidget(name, xdGridWidgetClass, parent, arglist, argcount);
}
