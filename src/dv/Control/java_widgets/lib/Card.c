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
static char *sccsid = {"@(#)Card.c	1.11"} ; /* 98/09/30 */
#endif /* lint && NOSCCS */

/*
** Card Layout Widget:
**
** A widget for laying out a stack of pages.
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
#include "CardP.h"
#include "Card.h"
#include "XdResources.h"

#ifndef   MIN
#define   MIN(a, b) ((a) < (b) ? (a) : (b))
#define   MAX(a, b) ((a) > (b) ? (a) : (b))
#endif /* MIN */

/*
** Static Widget Method Declarations
*/

#ifdef    _NO_PROTO
static void    Initialize() ;
static Boolean SetValues() ;
static void    CardLayout() ;
static void    CardPreferredSize() ;
static void    InsertChild() ;
static void    DeleteChild() ;
static void    ChangeManaged() ;
static void    CalculateCardIndex() ;
static void    CardSetPage() ;
#else  /* _NO_PROTO */
static void    Initialize(Widget request, Widget new_widget, ArgList args, Cardinal *num_args);
static Boolean SetValues(Widget cw, Widget rw, Widget nw, ArgList args, Cardinal *num_args);
static void    CardLayout(Widget w);
static void    CardPreferredSize(Widget w, Dimension *width, Dimension *height);
static void    InsertChild(Widget w) ;
static void    DeleteChild(Widget w) ;
static void    ChangeManaged(Widget w) ;
static void    CalculateCardIndex(Widget, int, XrmValue *) ;
static void    CardSetPage(XdCardWidget, Widget) ;
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

#define CardConstraint(w) ((XdCardConstraintPtr)(w)->core.constraints)

static XtResource resources[] = {
    {   
	XtNxdHorizontalSpacing,
        XtCXdHorizontalSpacing,
        XtRXdHorizontalSpacing,
        sizeof(Dimension),
        XtOffsetOf(XdCardRec, card.hSpacing),
        XmRImmediate,
        (XtPointer) 0
    },
    {   
	XtNxdVerticalSpacing,
        XtCXdVerticalSpacing,
        XtRXdVerticalSpacing,
        sizeof(Dimension),
        XtOffsetOf(XdCardRec, card.vSpacing),
        XmRImmediate,
        (XtPointer) 0
    },
    {   
	XtNxdCurrentPage,
        XtCXdCurrentPage,
        XtRXdCurrentPage,
        sizeof(int),
        XtOffsetOf(XdCardRec, card.currentPageNum),
        XmRImmediate,
        (XtPointer) 0
    }
} ;

static XtResource constraints[] = {
    {   
	XtNxdPageNumber,
        XtCXdPageNumber,
        XtRXdPageNumber,
        sizeof(short),
        XtOffsetOf(XdCardConstraintRec, card.pageNumber),
        /*XmRImmediate,*/
        /*(XtPointer) 0*/
	XtRCallProc,
	(XtPointer) CalculateCardIndex
    }
} ;

 
/*
** End Resource Lists
*/
  
/*
** Widget Class Record
*/

XdCardClassRec XdcardClassRec = {
	{ /* Core class */
		(WidgetClass)&xdLayoutClassRec,/* superclass           */
		"XdCard",		/* class_name           */
		sizeof(XdCardRec),	/* widget_size          */
		NULL,			/* class_initialize     */
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
		XtInheritGeometryManager, /* geometry_handler     */
		ChangeManaged,  	  /* change_managed       */
		InsertChild,	          /* insert_child         */
		DeleteChild,	          /* delete_child         */
		NULL		  	  /* extension            */
	}, { /* Constraint class */
		constraints,		/* resource list        */
		XtNumber(constraints),	/* num resources        */
		sizeof(XdCardConstraintRec),	/* constraint size      */
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
		(XdLayoutProc) CardLayout,                      /* layout         */
		(XdPreferredSizeProc) CardPreferredSize,	/* preferred_size */
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

WidgetClass xdCardWidgetClass = (WidgetClass) &XdcardClassRec ;

 
/*
** Widget Initialisation Method - called per-instance on widget create
*/

/* ARGSUSED */
#ifdef    _NO_PROTO
static void Initialize(request, new_widget, args, num_args)
	Widget       request ;
	Widget       new_widget ;
	ArgList      args ;
	Cardinal    *num_args ;
#else  /* _NO_PROTO */
static void Initialize(Widget request, Widget new_widget, ArgList args, Cardinal *num_args)
#endif /* _NO_PROTO */
{
	XdCardWidget cw = (XdCardWidget) new_widget ;

	cw->card.currentPage = (Widget) 0 ;
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
	XdCardWidget nb = (XdCardWidget) nw ;
	XdCardWidget cb = (XdCardWidget) cw ;

	if (nb->card.vSpacing != cb->card.vSpacing ||
	    nb->card.hSpacing != cb->card.hSpacing) {
		nb->layout.needsLayout = True ;
	}

	if (nb->card.currentPageNum != cb->card.currentPageNum) {
		XdCardShowPage(nw, nb->card.currentPageNum) ;
	}

	return XdCheckLayout(nw, xdCardWidgetClass) ;
}


/*
** CalculateCardIndex: returns unique page id for new child initialisation
*/

#ifndef    _NO_PROTO
static void CalculateCardIndex(Widget widget, int offset, XrmValue *value)
#else  /* _NO_PROTO */
static void CalculateCardIndex(widget, offset, value)
	Widget    widget ;
	int       offset ;
	XrmValue *value ;
#endif /* _NO_PROTO */
{
	/* 
	** Widget here, being a constraint resource, is the CHILD
	** not the Card
	*/
	static short page = 0 ;

	Widget              parent   = XtParent(widget) ;
	Widget              child    = (Widget)              0 ;
	XdCardWidget        card     = (XdCardWidget)        parent ;
	XdCardConstraintPtr cp       = (XdCardConstraintPtr) 0 ;
	int                 i ;

	page = 0 ;

	for (i = 0 ; i < card->composite.num_children ; i++) {
		child = card->composite.children[i] ;

		if ((child != (Widget) 0) && (child != widget)) {
			if ((cp = CardConstraint(child)) != (XdCardConstraintPtr) 0) {
				if (cp->card.pageNumber >= page) {
					page = cp->card.pageNumber + 1 ;
				}
			}
		}
	}

	value->addr = (XtPointer) &page ;
}

/*
** InsertChild: used to initialise the first page
*/

#ifdef    _NO_PROTO
static void InsertChild(child)
	Widget child ;
#else  /* _NO_PROTO */
static void InsertChild(Widget child)
#endif /* _NO_PROTO */
{
	XdCardWidget        cw   = (XdCardWidget) XtParent(child) ;
	XdCardConstraintPtr cp   = (XdCardConstraintPtr) 0 ;
	Boolean             show = False ;

	/* Use the Manager Class InsertChild function to perform the dirty work */

	(*((XmManagerWidgetClass) xmManagerWidgetClass)->composite_class.insert_child)(child) ;

	cp = CardConstraint(child) ;

	/* Initialise the current page pointer if not set */
			
	if (cw->card.currentPage == (Widget) 0) {
		if (XtIsRectObj(child)) {
			if (cp->card.pageNumber == cw->card.currentPageNum) {
				cw->card.currentPage = child ;
				show                 = True ;
			}
		}
	}
				
	XtVaSetValues(child, XmNmappedWhenManaged, show, 0) ;
}


/*
** DeleteChild: used to clear the current page pointer
*/

#ifdef    _NO_PROTO
static void DeleteChild(child)
	Widget child ;
#else  /* _NO_PROTO */
static void DeleteChild(Widget child)
#endif /* _NO_PROTO */
{
	XdCardWidget        cw = (XdCardWidget) XtParent(child) ;
	XdCardConstraintPtr cp = (XdCardConstraintPtr) 0 ;
	Widget              w  = (Widget) 0 ;
	Widget              n  = (Widget) 0 ;
	int                 i, j ;

	/* Zero the current page pointer if set to this child */

	if (cw->card.currentPage == child) {
		cw->card.currentPage = (Widget) 0 ;

		/* Pick a new page... 
		** Previous, else next managed child
		*/

		for (i = 0, j = -1 ; i < cw->composite.num_children ; i++) {
			w = cw->composite.children[i] ;

			if (w == child) {
				if (j == -1) {
					for (j = i + 1 ; j < cw->composite.num_children ; j++) {
						w = cw->composite.children[j] ;

						if (w->core.managed) {
							n = w ;

							break ;
						}
					}
				}
				else {
					n = cw->composite.children[j] ;
				}

				break ;
			}
			else {
				if (w->core.managed) {
					j = i ;
				}
			}
		}
	}
	
	/* Use the Manager Class DeleteChild function to perform the dirty work */

	(*((XmManagerWidgetClass) xmManagerWidgetClass)->composite_class.delete_child)(child) ;

	/* Reset Current Page */

	if (n != (Widget) 0) {
		cp                      = CardConstraint(n) ;
		cw->card.currentPageNum = cp->card.pageNumber ;

		CardSetPage(cw, n) ;
	}
}


/*
** ChangeManaged: clears the current page pointer if the child becomes unmanaged
*/

#ifdef    _NO_PROTO
static void ChangeManaged(child)
	Widget child ;
#else  /* _NO_PROTO */
static void ChangeManaged(Widget child)
#endif /* _NO_PROTO */
{
	XdCardWidget        cw = (XdCardWidget) XtParent(child) ;
	XdCardConstraintPtr cp = (XdCardConstraintPtr) 0 ;
	Widget              n  = (Widget) 0 ;
	Widget              w  = (Widget) 0 ;
	int                 i, j ;

	/* Reset the current page pointer if set to this child and the child is unmanaging */

	if (!child->core.managed) {
		if (cw->card.currentPage == child) {
			cw->card.currentPage = (Widget) 0 ;

			/*
			** Pick a new page
			** Previous, else next managed child
			*/

			for (i = 0, j = -1 ; i < cw->composite.num_children ; i++) {
				w = cw->composite.children[i] ;

				if (w == child) {
					if (j == -1) {
						for (j = i + 1 ; j < cw->composite.num_children ; j++) {
							w = cw->composite.children[j] ;

							if (w->core.managed) {
								n = w ;

								break ;
							}
						}
					}
					else {
						n = cw->composite.children[j] ;
					}

					break ;
				}
				else {
					if (w->core.managed) {
						j = i ;
					}
				}
			}
		}
	}
	else {
		/*
		** First child to show itself...
		*/

		if (cw->card.currentPage == (Widget) 0) {
			n = child ;
		}
	}
	
	/* Use the Layout Class ChangeManaged function to perform the dirty work */

	(*(xdLayoutClassRec.composite_class.change_managed))(child) ;

	if (n != (Widget) 0) {
		cp                      = CardConstraint(n) ;
		cw->card.currentPageNum = cp->card.pageNumber ;

		CardSetPage(cw, n) ;
	}
}

 
/*
** SuperClass (Layout Widget) XdLayoutProc Method
*/

#ifdef    _NO_PROTO
static void CardLayout(widget)
	Widget widget ;
#else  /* _NO_PROTO */
static void CardLayout(Widget widget)
#endif /* _NO_PROTO */
{
	XdCardWidget cw     = (XdCardWidget) widget ;
	Dimension    hs     = cw->manager.shadow_thickness + cw->card.hSpacing ;
	Dimension    vs     = cw->manager.shadow_thickness + cw->card.vSpacing ;
	Dimension    width  = Width(cw)  - (2 * hs) ;
	Dimension    height = Height(cw) - (2 * vs) ;
	int          i ;

	for (i = 0 ; i < cw->composite.num_children ; i++) {
		Widget child = cw->composite.children[i] ;

		if (child->core.managed) {
			XtConfigureWidget(child, hs, vs, width, height, child->core.border_width) ;
		}
	}
}


/*
** SuperClass (Layout Widget) XdPreferredSizeProc Method
*/

#ifdef    _NO_PROTO
static void CardPreferredSize(widget, width, height)
	Widget       widget ;
	Dimension   *width ;
	Dimension   *height ;
#else  /* _NO_PROTO */
static void CardPreferredSize(Widget widget, Dimension *width, Dimension *height)
#endif /* _NO_PROTO */
{
	XdCardWidget cw        = (XdCardWidget) widget ;
	Dimension    maxWidth  = (Dimension) 0 ;
	Dimension    maxHeight = (Dimension) 0 ;
	int          i ;

	for (i = 0 ; i < cw->composite.num_children ; i++) {
		Widget child = cw->composite.children[i] ;

		if (child->core.managed) {
			if (Height(child) > maxHeight) {
				maxHeight = Height(child) ;
			}
			if (Width(child) > maxWidth) {
				maxWidth = Width(child) ;
			}
		}
	}

	*width  = maxWidth  + (2 * cw->manager.shadow_thickness) ;
	*height = maxHeight + (2 * cw->manager.shadow_thickness) ;
}


/*
** Internal Routine
*/

#ifdef    _NO_PROTO
static void CardSetPage(cw, page)
	XdCardWidget cw ;
	Widget       page ;
#else  /* _NO_PROTO */
static void CardSetPage(XdCardWidget cw, Widget page)
#endif /* _NO_PROTO */
{
	Widget              child = (Widget) 0 ;
	XdCardConstraintPtr cp    = (XdCardConstraintPtr) 0 ;

	if (cw->card.currentPage) cp = CardConstraint(cw->card.currentPage) ;

	if (page != cw->card.currentPage) {
		if ((child = cw->card.currentPage) != (Widget) 0) {
			if (!child->core.being_destroyed) {
				XtVaSetValues(child, XmNmappedWhenManaged, False, 0) ;
			}
		}

		cw->card.currentPage = (Widget) 0 ;

		if (page != (Widget) 0) {
			if (!page->core.being_destroyed && page->core.managed) {
				XtVaSetValues(page, XmNmappedWhenManaged, True, 0) ;
			}

			cw->card.currentPage = page ;
		}
	}
}

 
/*
** Public Function for displaying a specific card page
*/

#ifdef    _NO_PROTO
void XdCardShowPage(widget, page)
	Widget widget ;
	int    page ;
#else  /* _NO_PROTO */
void XdCardShowPage(Widget widget, int page)
#endif /* _NO_PROTO */
{
	XdCardWidget        cw      = (XdCardWidget) widget ;
	XdCardConstraintPtr cp      = (XdCardConstraintPtr) 0 ;
	Widget              newPage = (Widget) 0 ;
	Widget              child   = (Widget) 0 ;
	int                 i ;

	for (i = 0 ; i < cw->composite.num_children ; i++) {
		child = cw->composite.children[i] ;

		if (child->core.managed) {
			cp = CardConstraint(child) ;

			if (cp->card.pageNumber == page) {
				newPage = child ;
				break ;
			}
		}
	}

	cw->card.currentPageNum = page ;

	CardSetPage(cw, newPage) ;
}

/*
** Convenience Function for creating a Card Layout Widget
*/

#ifdef    _NO_PROTO
Widget XdCreateCardWidget(parent, name, arglist, argcount)
	Widget   parent ;
	char    *name ;
	ArgList  arglist ;
	Cardinal argcount ;
#else  /* _NO_PROTO */
Widget XdCreateCardWidget(Widget parent, char *name, ArgList arglist, Cardinal argcount)
#endif /* _NO_PROTO */
{
	return XtCreateWidget(name, xdCardWidgetClass, parent, arglist, argcount) ;
}
